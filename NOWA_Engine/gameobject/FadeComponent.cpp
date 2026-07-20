#include "NOWAPrecompiled.h"
#include "FadeComponent.h"
#include "GameObjectController.h"
#include "main/AppStateManager.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScript.h"
#include "utilities/XMLConverter.h"

// Adjust this include if your project uses individual MyGUI headers instead
// of the umbrella header (e.g. MyGUI_Gui.h, MyGUI_ControllerManager.h,
// MyGUI_ControllerItem.h, MyGUI_Widget.h).
#include <MyGUI.h>

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    namespace
    {
        // MyGUI layer reserved for full-screen fade effects (see the layer
        // list already used by MyGUIComponent::AttrLayer()).
        const Ogre::String FADE_LAYER_NAME = "Back";

        // v2 replacement for the old FaderProcess. Not exported / not visible
        // outside this translation unit — FadeComponent owns the fade logic
        // directly, as requested, instead of a separate reusable process.
        //
        // Replicates FaderProcess::onUpdate exactly:
        //  - a fixed 1 second "stall" before the fade actually starts
        //    (matches FaderProcess::stallDuration, which was never exposed
        //    as a configurable value either)
        //  - alpha(t) = Interpolator::applyEaseFunction(0, 1, ease, t), with
        //    t running 1 -> 0 for FadeIn (revealing the scene) and 0 -> 1 for
        //    FadeOut (covering it) — identical formula/direction to the
        //    original FADE_IN / FADE_OUT branches.
        class FadeControllerItem : public MyGUI::ControllerItem
        {
        public:
            FadeControllerItem(bool isFadeIn, Ogre::Real duration, Interpolator::EaseFunctions easeFunction) :
                isFadeIn(isFadeIn),
                totalDuration(duration > 0.000001f ? duration : 1.0f),
                stallDuration(1.0f),
                elapsed(0.0f),
                selectedEaseFunction(easeFunction)
            {
            }

        protected:
            virtual void prepareItem(MyGUI::Widget* _widget) override
            {
                // Start fully opaque for FadeIn (about to reveal), fully
                // transparent for FadeOut (about to cover) — same starting
                // point as FaderProcess's initial currentAlpha.
                _widget->setAlpha(this->isFadeIn ? 1.0f : 0.0f);
            }

            virtual bool addTime(MyGUI::Widget* _widget, float _time) override
            {
                if (this->stallDuration > 0.0f)
                {
                    this->stallDuration -= _time;
                    if (this->stallDuration > 0.0f)
                    {
                        return true;
                    }
                }

                this->elapsed += _time;

                Ogre::Real t = this->elapsed / this->totalDuration;
                if (t >= 1.0f)
                {
                    _widget->setAlpha(this->isFadeIn ? 0.0f : 1.0f);
                    return false; // MyGUI fires eventPostAction, then removes this item
                }

                // FadeIn: eased input goes 1 -> 0 as time progresses.
                // FadeOut: eased input goes 0 -> 1 as time progresses.
                Ogre::Real easeInput = this->isFadeIn ? (1.0f - t) : t;
                Ogre::Real alpha = Interpolator::getInstance()->applyEaseFunction(0.0f, 1.0f, this->selectedEaseFunction, easeInput);
                _widget->setAlpha(alpha);
                return true;
            }

        private:
            bool isFadeIn;
            Ogre::Real totalDuration;
            Ogre::Real stallDuration;
            Ogre::Real elapsed;
            Interpolator::EaseFunctions selectedEaseFunction;
        };
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    FadeComponent::FadeComponent() :
        GameObjectComponent(),
        selectedEaseFunction(Interpolator::Linear),
        activated(new Variant(FadeComponent::AttrActivated(), true, this->attributes)),
        fadeMode(new Variant(FadeComponent::AttrFadeMode(), {Ogre::String("FadeIn"), Ogre::String("FadeOut")}, this->attributes)),
        duration(new Variant(FadeComponent::AttrDuration(), 5.0f, this->attributes)),
        easeFunction(new Variant(FadeComponent::AttrEaseFunction(), Interpolator::getInstance()->getAllEaseFunctionNames(), this->attributes)),
        fadeWidget(nullptr),
        controllerItem(nullptr)
    {
    }

    FadeComponent::~FadeComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[FadeComponent] Destructor fade component for game object: " + this->gameObjectPtr->getName());
    }

    bool FadeComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeMode")
        {
            this->fadeMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Duration")
        {
            this->duration->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EaseFunction")
        {
            this->setEaseFunction(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    GameObjectCompPtr FadeComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return GameObjectCompPtr();
    }

    bool FadeComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[FadeComponent] Init fade component for game object: " + this->gameObjectPtr->getName());

        return true;
    }

    void FadeComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    bool FadeComponent::connect(void)
    {
        GameObjectComponent::connect();

        // Create the full-screen fade widget once, up front — mirrors the
        // lazy-but-eager creation pattern used by
        // MyGUIFadeAlphaControllerComponent::connect() for its controllerItem.
        if (nullptr == this->fadeWidget)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->fadeWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Widget>("PanelSkin", 0.0f, 0.0f, 1.0f, 1.0f, MyGUI::Align::Stretch, FADE_LAYER_NAME, "FadeComponent_" + this->gameObjectPtr->getName());
                this->fadeWidget->setColour(MyGUI::Colour(0.0f, 0.0f, 0.0f));
                this->fadeWidget->setNeedMouseFocus(false);
                this->fadeWidget->setVisible(false);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "FadeComponent::connect");
        }

        return true;
    }

    bool FadeComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        this->fadeCompletedClosureFunctions.clear();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->controllerItem && nullptr != this->fadeWidget)
            {
                MyGUI::ControllerManager::getInstance().removeItem(this->fadeWidget);
            }
            this->controllerItem = nullptr;

            if (nullptr != this->fadeWidget)
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->fadeWidget);
                this->fadeWidget = nullptr;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "FadeComponent::disconnect");

        return true;
    }

    void FadeComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (FadeComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (FadeComponent::AttrFadeMode() == attribute->getName())
        {
            this->setFadeMode(attribute->getListSelectedValue());
        }
        else if (FadeComponent::AttrDuration() == attribute->getName())
        {
            this->setDurationSec(attribute->getReal());
        }
        else if (FadeComponent::AttrEaseFunction() == attribute->getName())
        {
            this->setEaseFunction(attribute->getListSelectedValue());
        }
    }

    void FadeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int
        // 6 = real
        // 7 = string
        // 8 = vector2
        // 9 = vector3
        // 10 = vector4 -> also quaternion
        // 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "FadeMode"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeMode->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Duration"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->duration->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "EaseFunction"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->easeFunction->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);
    }

    void FadeComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == this->bConnected)
        {
            if (true == activated)
            {
                bool isFadeIn = ("FadeIn" == this->fadeMode->getListSelectedValue());
                Ogre::Real fadeDuration = this->duration->getReal();
                Interpolator::EaseFunctions ease = this->selectedEaseFunction;

                NOWA::GraphicsModule::RenderCommand renderCommand = [this, isFadeIn, fadeDuration, ease]()
                {
                    if (nullptr == this->fadeWidget)
                    {
                        // connect() has not run yet (or disconnect() already
                        // tore the widget down) — nothing to animate.
                        return;
                    }

                    // Cancel any fade currently running on this widget.
                    if (nullptr != this->controllerItem)
                    {
                        MyGUI::ControllerManager::getInstance().removeItem(this->fadeWidget);
                        this->controllerItem = nullptr;
                    }

                    this->fadeWidget->setVisible(true);

                    FadeControllerItem* item = new FadeControllerItem(isFadeIn, fadeDuration, ease);
                    item->eventPostAction += MyGUI::newDelegate(this, &FadeComponent::controllerFinished);
                    this->controllerItem = item;

                    MyGUI::ControllerManager::getInstance().addItem(this->fadeWidget, item);
                };
                NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "FadeComponent::setActivated");
            }
        }
    }

    bool FadeComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void FadeComponent::controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller)
    {
        // MyGUI deleted the item internally — null our pointer.
        this->controllerItem = nullptr;

        bool wasFadeIn = ("FadeIn" == this->fadeMode->getListSelectedValue());

        // Reset so Lua can re-trigger via setActivated(true) again.
        this->activated->setValue(false);

        if (true == wasFadeIn)
        {
            // Scene is fully revealed — nothing left to cover it with.
            if (nullptr != sender)
            {
                sender->setVisible(false);
            }
        }
        // FadeOut: leave the widget visible and opaque — it is now covering
        // the screen, same end state the old v1 Overlay was left in.

        if (nullptr != this->getOwner()->getLuaScript())
        {
            auto* closureListPtr = &this->fadeCompletedClosureFunctions;

            if (false == closureListPtr->empty())
            {
                NOWA::AppStateManager::LogicCommand logicCommand = [closureListPtr]()
                {
                    // Copy happens HERE on the logic thread — safe for luabind::object
                    auto closures = *closureListPtr;

                    for (const auto& closure : closures)
                    {
                        if (false == closure.is_valid())
                        {
                            continue;
                        }
                        try
                        {
                            luabind::call_function<void>(closure);
                        }
                        catch (luabind::error& error)
                        {
                            luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                            std::stringstream msg;
                            msg << errorMsg;
                            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FadeComponent] Caught error in 'reactOnFadeCompleted' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
                        }
                    }
                };
                NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
            }
        }
    }

    void FadeComponent::setFadeMode(const Ogre::String& fadeMode)
    {
        this->fadeMode->setListSelectedValue(fadeMode);
    }

    Ogre::String FadeComponent::getFadeMode(void) const
    {
        return this->fadeMode->getListSelectedValue();
    }

    void FadeComponent::setDurationSec(Ogre::Real duration)
    {
        if (duration < 0.0f)
        {
            duration = 1.0f;
        }
        this->duration->setValue(duration);
    }

    Ogre::Real FadeComponent::getDurationSec(void) const
    {
        return this->duration->getReal();
    }

    void FadeComponent::setEaseFunction(const Ogre::String& easeFunction)
    {
        this->easeFunction->setListSelectedValue(easeFunction);
        this->selectedEaseFunction = Interpolator::getInstance()->mapStringToEaseFunctions(easeFunction);
    }

    Ogre::String FadeComponent::getEaseFunction(void) const
    {
        return this->easeFunction->getListSelectedValue();
    }

    void FadeComponent::reactOnFadeCompleted(luabind::object closureFunction)
    {
        this->fadeCompletedClosureFunctions.push_back(closureFunction);
    }

    Ogre::String FadeComponent::getClassName(void) const
    {
        return "FadeComponent";
    }

    Ogre::String FadeComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

}; // namespace end
