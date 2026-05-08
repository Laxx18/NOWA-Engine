#ifndef ENGINE_RESOURCE_LISTENER_H
#define ENGINE_RESOURCE_LISTENER_H

#include "MyGUI.h"
#include "MyGUI_Ogre2Platform.h"
#include "defines.h"
#include "utilities/FaderProcess.h"

namespace NOWA
{

    class EXPORTED EngineResourceListener : public Ogre::ResourceGroupListener
    {
    public:
        virtual void showLoadingBar(unsigned int numGroupsInit = 1, unsigned int numGroupsLoad = 1, Ogre::Real initProportion = 0.7) = 0;

        virtual std::pair<Ogre::Real, Ogre::Real> hideLoadingBar(void) = 0;
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED EngineResourceRotateListener : public EngineResourceListener
    {
    public:
        EngineResourceRotateListener(Ogre::Window* renderWindow);
        virtual ~EngineResourceRotateListener();
        virtual void showLoadingBar(unsigned int numGroupsInit = 1, unsigned int numGroupsLoad = 1, Ogre::Real initProportion = 0.7) override;
        virtual std::pair<Ogre::Real, Ogre::Real> hideLoadingBar(void) override;
        virtual void resourceGroupScriptingStarted(const Ogre::String& resourceGroupName, size_t scriptCount) override;
        virtual void scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript) override;
        virtual void scriptParseEnded(const Ogre::String& scriptName, bool skipped) override;
        virtual void resourceGroupScriptingEnded(const Ogre::String& resourceGroupName) override;
        virtual void resourceGroupLoadStarted(const Ogre::String& resourceGroupName, size_t resourceCount) override;
        virtual void resourceLoadStarted(const Ogre::ResourcePtr& resource) override;
        virtual void resourceLoadEnded() override;
        virtual void resourceGroupLoadEnded(const Ogre::String& resourceGroupName) override;

    private:
        Ogre::Window* renderWindow;
        Ogre::Real groupInitProportion;
        Ogre::Real groupLoadProportion;
        Ogre::Real loadInc;
        MyGUI::ImageBox* backgroundImage;
        MyGUI::ImageBox* rotatingImage;
        MyGUI::RotatingSkin* rotatingSkin;
        MyGUI::EditBox* progressLabel;
        MyGUI::EditBox* actionLabel;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED EngineResourceFadeListener : public EngineResourceListener
    {
    public:
        EngineResourceFadeListener(Ogre::Window* renderWindow);
        virtual ~EngineResourceFadeListener();
        virtual void showLoadingBar(unsigned int numGroupsInit = 1, unsigned int numGroupsLoad = 1, Ogre::Real initProportion = 0.7) override;
        virtual std::pair<Ogre::Real, Ogre::Real> hideLoadingBar(void) override;
        virtual void resourceGroupScriptingStarted(const Ogre::String& resourceGroupName, size_t scriptCount) override;
        virtual void scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript) override;
        virtual void scriptParseEnded(const Ogre::String& scriptName, bool skipped) override;
        virtual void resourceGroupScriptingEnded(const Ogre::String& resourceGroupName) override;
        virtual void resourceGroupLoadStarted(const Ogre::String& resourceGroupName, size_t resourceCount) override;
        virtual void resourceLoadStarted(const Ogre::ResourcePtr& resource) override;
        virtual void resourceLoadEnded() override;
        virtual void resourceGroupLoadEnded(const Ogre::String& resourceGroupName) override;

    private:
        Ogre::Window* renderWindow;
        Ogre::Real groupInitProportion;
        Ogre::Real groupLoadProportion;
        Ogre::Real loadInc;
        FaderProcess* faderProcess;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Pure UI widget manager for scene loading progress.
     * DotSceneImportModule registers itself as ResourceGroupListener and calls
     * updateProgress() / updatePostInitPhase() on this object.
     */
    class EXPORTED EngineResourceSceneListener
    {
    public:
        EngineResourceSceneListener(Ogre::Window* renderWindow, bool showProgressBar = true, bool showLoadingDetails = true);
        ~EngineResourceSceneListener();

        // Creates widgets on render thread via enqueueAndWait
        void showLoadingBar();

        // Destroys widgets on render thread via enqueueAndWait
        void hideLoadingBar();

        // Called from main thread — throttled enqueueAndWait
        void updateProgress(const Ogre::String& objectName, size_t current, size_t total);

        // Called from render thread (inside postInitData enqueueAndWait) — direct MyGUI
        void updatePostInitPhase(const Ogre::String& phase, size_t current, size_t total);

    private:
        Ogre::Window* renderWindow;
        bool showProgressBarWidget;
        bool showLoadingDetailsWidget;
        float lastUpdateMs;

        MyGUI::ImageBox* backgroundImage;
        MyGUI::ImageBox* rotatingImage;
        MyGUI::RotatingSkin* rotatingSkin;
        MyGUI::ProgressBar* progressBar;
        MyGUI::EditBox* statusLabel;
        MyGUI::EditBox* progressLabel;
    };

}; // namespace end

#endif