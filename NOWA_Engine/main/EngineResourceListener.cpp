#include "NOWAPrecompiled.h"
#include "EngineResourceListener.h"
#include "Core.h"
#include "modules/GraphicsModule.h"
#include "utilities/MyGUIUtilities.h"

namespace NOWA
{
    EngineResourceRotateListener::EngineResourceRotateListener(Ogre::Window* renderWindow) : EngineResourceListener(), renderWindow(renderWindow), groupInitProportion(0.0f), groupLoadProportion(0.0f), loadInc(0.0f)
    {
        this->backgroundImage = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin", MyGUI::IntCoord(0, 0, renderWindow->getWidth(), renderWindow->getHeight()), MyGUI::Align::Default, "Overlapped");
        this->backgroundImage->setImageTexture("BackgroundShadeBlue.png");
        this->backgroundImage->setVisible(false);

        this->actionLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.4f, 0.4f, 0.3f, 0.05f, MyGUI::Align::Default, "Main", "ActionLabel");
        this->actionLabel->setVisible(false);
        this->actionLabel->setEditStatic(true);
        MyGUIUtilities::getInstance()->setFontSize(this->actionLabel, 14);

        this->progressLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.05f, 0.95f, 0.3f, 0.05f, MyGUI::Align::Default, "Main", "ProgressLabel");
        this->progressLabel->setVisible(false);
        this->progressLabel->setEditStatic(true);
        MyGUIUtilities::getInstance()->setFontSize(this->progressLabel, 14);

        int logoWidth = static_cast<int>(260.0f * 0.5f);
        int logoHeight = static_cast<int>(160.0f * 0.5);
        this->rotatingImage = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin", MyGUI::IntCoord(renderWindow->getWidth() - logoWidth - 20, renderWindow->getHeight() - logoHeight - 20, logoWidth, logoHeight),
            MyGUI::Align::Default, "Overlapped");
        this->rotatingImage->setImageTexture("NOWA_Logo3.png");
        this->rotatingImage->setVisible(false);

        MyGUI::ISubWidget* main = this->rotatingImage->getSubWidgetMain();
        if (nullptr == main)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[EngineResourceRotateListener] Error: Could not get MyGUI object. Check the resources.cfg, whether 'FileSystem=../../media/MyGUI_Media' is missing!");
            throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[EngineResourceRotateListener] Error: Could not get MyGUI object. Check the resources.cfg, whether 'FileSystem=../../media/MyGUI_Media' is missing!\n", "NOWA");
        }
        this->rotatingSkin = main->castType<MyGUI::RotatingSkin>();
    }

    EngineResourceRotateListener::~EngineResourceRotateListener()
    {
        MyGUI::Gui::getInstancePtr()->destroyWidget(this->backgroundImage);
        MyGUI::Gui::getInstancePtr()->destroyWidget(this->rotatingImage);
        MyGUI::Gui::getInstancePtr()->destroyWidget(this->actionLabel);
        MyGUI::Gui::getInstancePtr()->destroyWidget(this->progressLabel);
    }

    void EngineResourceRotateListener::showLoadingBar(unsigned int numGroupsInit, unsigned int numGroupsLoad, Ogre::Real initProportion)
    {
        this->backgroundImage->setVisible(true);
        this->rotatingImage->setVisible(true);
        this->actionLabel->setVisible(true);
        this->progressLabel->setVisible(true);

        Ogre::ResourceGroupManager::getSingleton().addResourceGroupListener(this);
        MyGUI::PointerManager::getInstancePtr()->setVisible(false);

        if (numGroupsInit == 0 && numGroupsLoad != 0)
        {
            this->groupInitProportion = 0;
            this->groupLoadProportion = 1;
        }
        else if (numGroupsLoad == 0 && numGroupsInit != 0)
        {
            this->groupLoadProportion = 0;
            if (numGroupsInit != 0)
            {
                this->groupInitProportion = 1;
            }
        }
        else if (numGroupsInit == 0 && numGroupsLoad == 0)
        {
            this->groupInitProportion = 0;
            this->groupLoadProportion = 0;
        }
        else
        {
            this->groupInitProportion = initProportion / numGroupsInit;
            this->groupLoadProportion = (1 - initProportion) / numGroupsLoad;
        }
    }

    std::pair<Ogre::Real, Ogre::Real> EngineResourceRotateListener::hideLoadingBar(void)
    {
        MyGUI::PointerManager::getInstancePtr()->setVisible(true);
        this->actionLabel->setVisible(false);
        this->progressLabel->setVisible(false);
        this->backgroundImage->setVisible(false);
        this->rotatingImage->setVisible(false);
        MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("ActionLabel")->setVisible(false);
        Ogre::ResourceGroupManager::getSingleton().removeResourceGroupListener(this);
        return std::make_pair(0.0f, 0.0f);
    }

    void EngineResourceRotateListener::resourceGroupScriptingStarted(const Ogre::String& resourceGroupName, size_t scriptCount)
    {
        this->loadInc = this->groupInitProportion / scriptCount;
        MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("ActionLabel")->setCaptionWithReplacing("#00ff00#{Finished}!");
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceRotateListener::scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript)
    {
        MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("ActionLabel")->setCaptionWithReplacing("#00ff00#{Parsing}: " + scriptName);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceRotateListener::scriptParseEnded(const Ogre::String& scriptName, bool skipped)
    {
        this->rotatingSkin->setAngle(this->rotatingSkin->getAngle() + this->loadInc * Ogre::Math::TWO_PI);
        this->progressLabel->setCaptionWithReplacing("#00ff00" + Ogre::StringConverter::toString(static_cast<int>(this->rotatingSkin->getAngle() / Ogre::Math::TWO_PI * 100.0f)) + " %");
        this->rotatingSkin->setCenter(MyGUI::IntPoint(this->rotatingSkin->getWidth() / 2, this->rotatingSkin->getHeight() / 2));
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceRotateListener::resourceGroupScriptingEnded(const Ogre::String& resourceGroupName)
    {
    }

    void EngineResourceRotateListener::resourceGroupLoadStarted(const Ogre::String& resourceGroupName, size_t resourceCount)
    {
        this->loadInc = (2.0f / Ogre::Math::PI * 100.0f) * Ogre::Math::ATan(this->groupLoadProportion / resourceCount * 100 / 50).valueRadians();
        this->actionLabel->setCaptionWithReplacing("#00ff00#{Loading}...");
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceRotateListener::resourceLoadStarted(const Ogre::ResourcePtr& resource)
    {
        this->actionLabel->setCaptionWithReplacing("#00ff00#{Loading}: " + resource->getName());
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceRotateListener::resourceLoadEnded()
    {
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceRotateListener::resourceGroupLoadEnded(const Ogre::String& resourceGroupName)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    EngineResourceFadeListener::EngineResourceFadeListener(Ogre::Window* renderWindow) : EngineResourceListener(), renderWindow(renderWindow), groupInitProportion(0.0f), groupLoadProportion(0.0f), loadInc(0.0f), faderProcess(nullptr)
    {
    }

    EngineResourceFadeListener::~EngineResourceFadeListener()
    {
        if (nullptr != this->faderProcess)
        {
            delete this->faderProcess;
            this->faderProcess = nullptr;
        }
    }

    void EngineResourceFadeListener::showLoadingBar(unsigned int numGroupsInit, unsigned int numGroupsLoad, Ogre::Real initProportion)
    {
        if (nullptr != this->faderProcess)
        {
            delete this->faderProcess;
            this->faderProcess = nullptr;
        }

        Ogre::ResourceGroupManager::getSingleton().addResourceGroupListener(this);
        MyGUI::PointerManager::getInstancePtr()->setVisible(false);

        if (numGroupsInit == 0 && numGroupsLoad != 0)
        {
            this->groupInitProportion = 0;
            this->groupLoadProportion = 1;
        }
        else if (numGroupsLoad == 0 && numGroupsInit != 0)
        {
            this->groupLoadProportion = 0;
            if (numGroupsInit != 0)
            {
                this->groupInitProportion = 1;
            }
        }
        else if (numGroupsInit == 0 && numGroupsLoad == 0)
        {
            this->groupInitProportion = 0;
            this->groupLoadProportion = 0;
        }
        else
        {
            this->groupInitProportion = initProportion / numGroupsInit;
            this->groupLoadProportion = (1 - initProportion) / numGroupsLoad;
        }

        this->faderProcess = new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 10.0f);
    }

    std::pair<Ogre::Real, Ogre::Real> EngineResourceFadeListener::hideLoadingBar(void)
    {
        MyGUI::PointerManager::getInstancePtr()->setVisible(true);
        auto continueData = std::make_pair(this->faderProcess->getCurrentAlpha(), this->faderProcess->getCurrentDuration());
        if (nullptr != this->faderProcess)
        {
            this->faderProcess->finished();
            delete this->faderProcess;
            this->faderProcess = nullptr;
        }
        return continueData;
    }

    void EngineResourceFadeListener::resourceGroupScriptingStarted(const Ogre::String& resourceGroupName, size_t scriptCount)
    {
        this->loadInc = this->groupInitProportion / scriptCount;
        this->faderProcess->onUpdate(0.016f);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceFadeListener::scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript)
    {
        this->faderProcess->onUpdate(0.016f);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceFadeListener::scriptParseEnded(const Ogre::String& scriptName, bool skipped)
    {
        this->faderProcess->onUpdate(0.016f);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceFadeListener::resourceGroupScriptingEnded(const Ogre::String& resourceGroupName)
    {
        this->faderProcess->onUpdate(0.016f);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceFadeListener::resourceGroupLoadStarted(const Ogre::String& resourceGroupName, size_t resourceCount)
    {
        this->loadInc = this->groupLoadProportion / resourceCount;
        this->faderProcess->onUpdate(0.016f);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceFadeListener::resourceLoadStarted(const Ogre::ResourcePtr& resource)
    {
        this->faderProcess->onUpdate(0.016f);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceFadeListener::resourceLoadEnded()
    {
        this->faderProcess->onUpdate(0.016f);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    void EngineResourceFadeListener::resourceGroupLoadEnded(const Ogre::String& resourceGroupName)
    {
        this->faderProcess->onUpdate(0.016f);
        Core::getSingletonPtr()->getOgreRoot()->renderOneFrame();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EngineResourceSceneListener
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    EngineResourceSceneListener::EngineResourceSceneListener(Ogre::Window* renderWindow, bool showProgressBar, bool showLoadingDetails) :
        renderWindow(renderWindow),
        showProgressBarWidget(showProgressBar),
        showLoadingDetailsWidget(showLoadingDetails),
        lastUpdateMs(0.0f),
        backgroundImage(nullptr),
        rotatingImage(nullptr),
        rotatingSkin(nullptr),
        progressBar(nullptr),
        statusLabel(nullptr),
        progressLabel(nullptr)
    {
    }

    EngineResourceSceneListener::~EngineResourceSceneListener()
    {
        // Caller must call hideLoadingBar() before delete
    }

    void EngineResourceSceneListener::showLoadingBar()
    {
        this->lastUpdateMs = static_cast<float>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds());

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            const int logoWidth = static_cast<int>(260.0f * 0.5f);
            const int logoHeight = static_cast<int>(160.0f * 0.5f);

            this->backgroundImage = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin", MyGUI::IntCoord(0, 0, this->renderWindow->getWidth(), this->renderWindow->getHeight()), MyGUI::Align::Default, "Overlapped");
            this->backgroundImage->setImageTexture("BackgroundShadeBlue.png");
            this->backgroundImage->setVisible(true);

            this->rotatingImage = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin", MyGUI::IntCoord(this->renderWindow->getWidth() - logoWidth - 20, this->renderWindow->getHeight() - logoHeight - 20, logoWidth, logoHeight),
                MyGUI::Align::Default, "Overlapped");
            this->rotatingImage->setImageTexture("NOWA_Logo3.png");
            this->rotatingImage->setVisible(true);

            MyGUI::ISubWidget* main = this->rotatingImage->getSubWidgetMain();
            if (nullptr != main)
            {
                this->rotatingSkin = main->castType<MyGUI::RotatingSkin>();
                this->rotatingSkin->setCenter(MyGUI::IntPoint(this->rotatingImage->getWidth() / 2, this->rotatingImage->getHeight() / 2));
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[EngineResourceSceneListener] Could not get RotatingSkin. Check resources.cfg for MyGUI_Media.");
            }

            if (true == this->showProgressBarWidget)
            {
                this->progressBar = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ProgressBar>("ProgressBar", 0.05f, 0.88f, 0.9f, 0.05f, MyGUI::Align::Default, "Overlapped", "SceneLoad_ProgressBar");
                this->progressBar->setProgressRange(1000);
                this->progressBar->setProgressPosition(0);
                this->progressBar->setVisible(true);
            }

            this->progressLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.05f, 0.93f, 0.15f, 0.05f, MyGUI::Align::Default, "Overlapped", "SceneLoad_ProgressLabel");
            this->progressLabel->setEditStatic(true);
            MyGUIUtilities::getInstance()->setFontSize(this->progressLabel, 14);
            this->progressLabel->setCaptionWithReplacing("#00ff00 0 %");
            this->progressLabel->setVisible(true);

            if (true == this->showLoadingDetailsWidget)
            {
                this->statusLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.4f, 0.82f, 0.3f, 0.05f, MyGUI::Align::Default, "Overlapped", "SceneLoad_StatusLabel");
                this->statusLabel->setEditStatic(true);
                MyGUIUtilities::getInstance()->setFontSize(this->statusLabel, 12);
                this->statusLabel->setCaptionWithReplacing("#00ff00#{Loading}...");
                this->statusLabel->setVisible(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "EngineResourceSceneListener::showLoadingBar");
    }

    void EngineResourceSceneListener::hideLoadingBar()
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (nullptr != this->backgroundImage)
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->backgroundImage);
                this->backgroundImage = nullptr;
            }
            if (nullptr != this->rotatingImage)
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->rotatingImage);
                this->rotatingImage = nullptr;
                this->rotatingSkin = nullptr;
            }
            if (nullptr != this->progressBar)
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->progressBar);
                this->progressBar = nullptr;
            }
            if (nullptr != this->progressLabel)
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->progressLabel);
                this->progressLabel = nullptr;
            }
            if (nullptr != this->statusLabel)
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->statusLabel);
                this->statusLabel = nullptr;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "EngineResourceSceneListener::hideLoadingBar");
    }

    void EngineResourceSceneListener::updateProgress(const Ogre::String& objectName, size_t current, size_t total)
    {
        // Throttle: max one enqueueAndWait per 100ms
        float nowMs = static_cast<float>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds());
        if ((nowMs - this->lastUpdateMs) < 100.0f)
        {
            return;
        }
        this->lastUpdateMs = nowMs;

        const size_t position = (total > 0) ? std::min(static_cast<size_t>(static_cast<float>(current) / total * 1000.0f), static_cast<size_t>(1000)) : 0;
        const bool showPB = this->showProgressBarWidget;
        const bool showLD = this->showLoadingDetailsWidget;

        NOWA::GraphicsModule::RenderCommand cmd = [this, objectName, position, current, total, showPB, showLD]()
        {
            if (showPB && nullptr != this->progressBar)
            {
                this->progressBar->setProgressPosition(position);
            }

            if (nullptr != this->progressLabel)
            {
                this->progressLabel->setCaptionWithReplacing("#00ff00" + Ogre::StringConverter::toString(position / 10) + " %");
            }

            if (showLD && nullptr != this->statusLabel)
            {
                this->statusLabel->setCaptionWithReplacing("#00ff00[Loading] " + objectName + " (" + Ogre::StringConverter::toString(current) + "/" + Ogre::StringConverter::toString(total) + ")");
            }

            if (nullptr != this->rotatingSkin)
            {
                this->rotatingSkin->setAngle(this->rotatingSkin->getAngle() + 0.05f * Ogre::Math::TWO_PI);
                this->rotatingSkin->setCenter(MyGUI::IntPoint(this->rotatingImage->getWidth() / 2, this->rotatingImage->getHeight() / 2));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "EngineResourceSceneListener::updateProgress");
    }

    void EngineResourceSceneListener::updatePostInitPhase(const Ogre::String& phase, size_t current, size_t total)
    {
        // Called on RENDER THREAD — direct MyGUI access, no enqueue needed
        const size_t position = 500 + ((total > 0) ? std::min(static_cast<size_t>(static_cast<float>(current) / total * 500.0f), static_cast<size_t>(500)) : 0);

        if (this->showProgressBarWidget && nullptr != this->progressBar)
        {
            this->progressBar->setProgressPosition(position);
        }

        if (nullptr != this->progressLabel)
        {
            this->progressLabel->setCaptionWithReplacing("#ffff00" + Ogre::StringConverter::toString(position / 10) + " %");
        }

        if (this->showLoadingDetailsWidget && nullptr != this->statusLabel)
        {
            this->statusLabel->setCaptionWithReplacing("#ffff00[PostInit] " + phase + " (" + Ogre::StringConverter::toString(current) + "/" + Ogre::StringConverter::toString(total) + ")");
        }

        if (nullptr != this->rotatingSkin)
        {
            this->rotatingSkin->setAngle(this->rotatingSkin->getAngle() + 0.03f * Ogre::Math::TWO_PI);
            this->rotatingSkin->setCenter(MyGUI::IntPoint(this->rotatingImage->getWidth() / 2, this->rotatingImage->getHeight() / 2));
        }
    }

}; // namespace end