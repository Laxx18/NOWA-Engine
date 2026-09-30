#include "NOWAPrecompiled.h"
#include "FaderProcess.h"
#include "modules/GraphicsModule.h"

// Adjust this include if your project uses individual MyGUI headers instead
// of the umbrella header (e.g. MyGUI_Gui.h, MyGUI_ControllerManager.h,
// MyGUI_ControllerItem.h, MyGUI_Widget.h).
#include <MyGUI.h>

// Attention: Required for the throttled fade widget diagnostics in getSharedFadeWidget().
#include "MyGUI_RenderManager.h"
#include <atomic>

namespace NOWA
{
    namespace
    {
        // Same layer FadeComponent's fade widget uses.
        // ASSUMPTION corrected: "Fade" sits above everything including UI
        // (menus got hidden by it). "FadeMiddle" is the layer meant to sit
        // below persistent UI but above the 3D scene — verify against your
        // layers.xml if menus are still affected.
        const Ogre::String FADE_LAYER_NAME = "Back";

        // Upper limit for the time a fade advances per frame. Attention: the MyGUI controllers get the Ogre frame time, and the
        // first frames after a scene load (rendering suspended, shaders compiled, loading frames throttled) report hundreds of
        // milliseconds up to seconds. Without this limit the stall and the whole fade were consumed by the very first frame, i.e.
        // the black screen vanished at once and the settling scene (camera catching up with the player) was visible.
        // With the limit a fade takes at least duration * 30 frames; at a normal frame rate nothing changes.
        const float MAX_FADE_TIME_STEP = 1.0f / 30.0f;

        // How long a FADE_IN holds the full black before it starts to fade. This is the time the freshly loaded scene gets to
        // settle behind the black screen (simulation start, player controller, game camera). A FADE_OUT has no stall anymore:
        // it held the scene fully VISIBLE for that time, so a scene change could start before the screen was black.
        const float FADE_IN_STALL_DURATION = 1.0f;

        // v2 replacement for the old shared, named v1 overlay
        // ("Overlays/FadeInOut") + "Materials/OverlayMaterial" datablock.
        // Lazily created once, on the render thread, and never destroyed —
        // same lifetime the old overlay had (looked up by name, never
        // hidden-and-forgotten, never recreated per FaderProcess instance).
        // Must only be called from the render thread.
        MyGUI::Widget* getSharedFadeWidget()
        {
            static MyGUI::Widget* sharedFadeWidget = nullptr;
            if (nullptr == sharedFadeWidget)
            {
                // Attention: Same fix as FadeComponent::connect(). createWidgetReal() with
                // (0, 0, 1, 1) routes through MyGUI::CoordConverter::convertFromRelative(),
                // which used to truncate towards zero and could end up one pixel short on the
                // right and at the bottom - exactly the "one row of the scene is visible behind
                // the fade" symptom. Creating the widget directly in pixels from the current
                // view size removes that conversion entirely, so this is exact regardless of
                // whether CoordConverter rounds or truncates.
                const MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();

                // Attention (overscan workaround): pixel-exact geometry here plus a confirmed-zero
                // D3D9 texel offset in Ogre2RenderManager still left a 1px sliver visible at the
                // right/bottom edge. Since the widget coordinates are provably correct (see the
                // throttled log below), grow the widget 1px past every edge - it is a solid-colour
                // full-screen panel, so the extra ring is never visible, and this guarantees no gap
                // regardless of whether the actual cause is GPU-rasterizer edge behaviour or the
                // "PanelSkin" skin's own tiling. Same fix applied in FadeComponent::connect().
                sharedFadeWidget = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::Widget>("PanelSkin", MyGUI::IntCoord(-1, -1, viewSize.width + 2, viewSize.height + 2), MyGUI::Align::Stretch, FADE_LAYER_NAME, "FaderProcess_SharedFadeWidget");
                sharedFadeWidget->setColour(MyGUI::Colour(0.0f, 0.0f, 0.0f));
                sharedFadeWidget->setNeedMouseFocus(false);
                sharedFadeWidget->setVisible(false);

                // Attention: Throttled diagnostics, only every 10th creation (this widget is a
                // function-local static, so in practice this fires once per process - the
                // counter guard is kept anyway for consistency with the other call sites and in
                // case the shared widget is ever recreated).
                static std::atomic<unsigned int> faderWidgetLogCounter{0};
                const unsigned int currentLogCount = faderWidgetLogCounter.fetch_add(1);
                if (0 == (currentLogCount % 10))
                {
                    const MyGUI::IntCoord absoluteCoord = sharedFadeWidget->getAbsoluteCoord();
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[FaderProcess] Shared fade widget #" + Ogre::StringConverter::toString(currentLogCount) + " created, viewSize: (" +
                                                                                          Ogre::StringConverter::toString(viewSize.width) + ", " + Ogre::StringConverter::toString(viewSize.height) + ") absolute: (" +
                                                                                          Ogre::StringConverter::toString(absoluteCoord.left) + ", " + Ogre::StringConverter::toString(absoluteCoord.top) + ", " +
                                                                                          Ogre::StringConverter::toString(absoluteCoord.width) + ", " + Ogre::StringConverter::toString(absoluteCoord.height) + ")");
                }
            }
            return sharedFadeWidget;
        }

        // Runs the actual fade animation on the render thread (MyGUI
        // controllers are ticked there). Replicates FaderProcess::onUpdate's
        // original math exactly, including its FADE_IN/FADE_OUT asymmetry
        // (see the comment in addTime below) — this is a faithful port, not
        // a behavior fix.
        class FaderControllerItem : public MyGUI::ControllerItem
        {
        public:
            FaderControllerItem(bool isFadeIn, Ogre::Real startDurationValue, Ogre::Real totalDuration, Ogre::Real speedMultiplier, Interpolator::EaseFunctions easeFunction, std::atomic<bool>* finishedFlag, std::atomic<Ogre::Real>* alphaOut,
                std::atomic<Ogre::Real>* durationOut) :
                isFadeIn(isFadeIn),
                currentDurationValue(startDurationValue),
                totalDuration(totalDuration),
                speedMultiplier(speedMultiplier),
                stallDuration(0.0f),
                selectedEaseFunction(easeFunction),
                finishedFlag(finishedFlag),
                alphaOut(alphaOut),
                durationOut(durationOut)
            {
                if (true == this->isFadeIn)
                {
                    this->stallDuration = FADE_IN_STALL_DURATION;
                }
            }

        protected:
            virtual void prepareItem(MyGUI::Widget* _widget) override
            {
                // Initial alpha was already set explicitly on the shared
                // widget right before addItem() (see the constructor's render
                // command below) — nothing further needed here.
            }

            virtual bool addTime(MyGUI::Widget* _widget, float _time) override
            {
                // See MAX_FADE_TIME_STEP.
                if (_time > MAX_FADE_TIME_STEP)
                {
                    _time = MAX_FADE_TIME_STEP;
                }

                this->stallDuration -= _time * this->speedMultiplier;
                if (this->stallDuration > 0.0f)
                {
                    return true;
                }

                if (true == this->isFadeIn)
                {
                    this->currentDurationValue -= _time * this->speedMultiplier;
                    Ogre::Real t = this->currentDurationValue / this->totalDuration;

                    if (t <= 0.0f)
                    {
                        _widget->setAlpha(0.0f);
                        _widget->setVisible(false);
                        this->publish(0.0f, 0.0f, true);
                        return false; // MyGUI fires eventPostAction (none attached here), then removes this item
                    }

                    Ogre::Real eased = Interpolator::getInstance()->applyEaseFunction(0.0f, 1.0f, this->selectedEaseFunction, t);
                    _widget->setAlpha(eased);
                    this->publish(eased, this->currentDurationValue, false);
                    return true;
                }
                else // FADE_OUT
                {
                    // NOTE: the original FaderProcess incremented currentDuration
                    // by `dt` only in this branch (NOT dt*speedMultiplier, unlike
                    // the stall timer and the FADE_IN branch above) — preserved
                    // as-is; this is an inherited quirk, not something new.
                    this->currentDurationValue += _time;
                    Ogre::Real t = this->currentDurationValue / this->totalDuration;

                    if (t >= 1.0f)
                    {
                        _widget->setAlpha(1.0f);
                        this->publish(1.0f, this->totalDuration, true);
                        return false;
                    }

                    Ogre::Real eased = Interpolator::getInstance()->applyEaseFunction(0.0f, 1.0f, this->selectedEaseFunction, t);
                    _widget->setAlpha(eased);
                    this->publish(eased, this->currentDurationValue, false);
                    return true;
                }
            }

        private:
            void publish(Ogre::Real alpha, Ogre::Real durationValue, bool done)
            {
                if (nullptr != this->alphaOut)
                {
                    this->alphaOut->store(alpha);
                }
                if (nullptr != this->durationOut)
                {
                    this->durationOut->store(durationValue);
                }
                if (true == done && nullptr != this->finishedFlag)
                {
                    this->finishedFlag->store(true);
                }
            }

        private:
            bool isFadeIn;
            Ogre::Real currentDurationValue;
            Ogre::Real totalDuration;
            Ogre::Real speedMultiplier;
            Ogre::Real stallDuration;
            Interpolator::EaseFunctions selectedEaseFunction;
            std::atomic<bool>* finishedFlag;
            std::atomic<Ogre::Real>* alphaOut;
            std::atomic<Ogre::Real>* durationOut;
        };
    }

    FaderProcess::FaderProcess(FadeOperation fadeOperation, Ogre::Real duration, Interpolator::EaseFunctions selectedEaseFunction, Ogre::Real continueAlpha, Ogre::Real continueDuration, Ogre::Real speedMultiplier) :
        eFadeOperation(fadeOperation),
        selectedEaseFunction(selectedEaseFunction),
        speedMultiplier(speedMultiplier),
        isFinished(false),
        succeededAlready(false)
    {
        if (duration < 0.0f)
        {
            duration = -duration;
        }
        if (duration < 0.000001f)
        {
            duration = 1.0f;
        }
        this->totalDuration = duration;

        bool isFadeIn = (this->eFadeOperation == FadeOperation::FADE_IN);

        Ogre::Real startAlpha;
        Ogre::Real startDurationValue;

        if (true == isFadeIn)
        {
            startAlpha = (0.0f == continueAlpha) ? 1.0f : continueAlpha;
            startDurationValue = (0.0f == continueDuration) ? this->totalDuration : continueDuration;
        }
        else
        {
            startAlpha = (0.0f == continueAlpha) ? 0.0f : continueAlpha;
            startDurationValue = continueDuration; // default 0.0f, same as original
        }

        this->atomicCurrentAlpha.store(startAlpha);
        this->atomicCurrentDuration.store(startDurationValue);

        Ogre::Real capturedTotalDuration = this->totalDuration;
        Ogre::Real capturedSpeedMultiplier = this->speedMultiplier;
        Interpolator::EaseFunctions capturedEase = this->selectedEaseFunction;
        std::atomic<bool>* finishedFlagPtr = &this->isFinished;
        std::atomic<Ogre::Real>* alphaPtr = &this->atomicCurrentAlpha;
        std::atomic<Ogre::Real>* durationPtr = &this->atomicCurrentDuration;

        NOWA::GraphicsModule::RenderCommand renderCommand = [isFadeIn, startAlpha, startDurationValue, capturedTotalDuration, capturedSpeedMultiplier, capturedEase, finishedFlagPtr, alphaPtr, durationPtr]()
        {
            MyGUI::Widget* widget = getSharedFadeWidget();

            // Cancel any fade still running from a previous FaderProcess
            // instance on the shared widget (same "only one fade active at
            // a time" assumption the old shared-overlay design already made).
            MyGUI::ControllerManager::getInstance().removeItem(widget);

            widget->setAlpha(startAlpha);
            widget->setVisible(true);

            FaderControllerItem* item = new FaderControllerItem(isFadeIn, startDurationValue, capturedTotalDuration, capturedSpeedMultiplier, capturedEase, finishedFlagPtr, alphaPtr, durationPtr);

            MyGUI::ControllerManager::getInstance().addItem(widget, item);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "FaderProcess::FaderProcess");
    }

    FaderProcess::~FaderProcess()
    {
    }

    void FaderProcess::showBlackScreenImmediate(void)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = []()
        {
            MyGUI::Widget* widget = getSharedFadeWidget();
            // Cancel any running fade animation so it doesn't fight this.
            MyGUI::ControllerManager::getInstance().removeItem(widget);
            widget->setAlpha(1.0f);
            widget->setVisible(true);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "FaderProcess::showBlackScreenImmediate");
    }

    void FaderProcess::hideBlackScreenImmediate(void)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = []()
        {
            MyGUI::Widget* widget = getSharedFadeWidget();
            MyGUI::ControllerManager::getInstance().removeItem(widget);
            widget->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "FaderProcess::hideBlackScreenImmediate");
    }

    void FaderProcess::onUpdate(Ogre::Real dt)
    {
        if (true == this->isFinished.load() && false == this->succeededAlready)
        {
            this->succeededAlready = true;
            this->eFadeOperation = FadeOperation::FADE_NONE;
            this->succeed();
        }
    }

    void FaderProcess::finished(void)
    {
        // Nothing to clean up here: the fade widget is shared/persistent
        // (mirrors the old named overlay, which was never destroyed either)
        // and the MyGUI ControllerItem already removed/deleted itself once
        // addTime() returned false.
    }

    void FaderProcess::onSuccess(void)
    {
        this->finished();
    }

    Ogre::Real FaderProcess::getCurrentAlpha(void) const
    {
        return this->atomicCurrentAlpha.load();
    }

    Ogre::Real FaderProcess::getCurrentDuration(void) const
    {
        return this->atomicCurrentDuration.load();
    }

}; // namespace end