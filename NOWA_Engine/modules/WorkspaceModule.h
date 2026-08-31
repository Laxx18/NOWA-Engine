#ifndef WORKSPACE_MODULE_H
#define WORKSPACE_MODULE_H

#include "defines.h"

namespace NOWA
{

    class WorkspaceBaseComponent;
    class CameraComponent;

    class EXPORTED WorkspaceModule
    {
    public:
        WorkspaceModule* getPrimaryWorkspaceModule(void);

        WorkspaceModule* getAdditionalWorkspaceModule(Ogre::Camera* camera);

        /**
         * @brief		Destroys all workspaces of this app state.
         * @param[in]	sceneManager	Optional filter. When given, only workspaces whose camera
         *								belongs to that scene manager are removed. Needed because one
         *								app state can load several scenes in sequence.
         */
        void destroyContent(Ogre::SceneManager* sceneManager = nullptr);

        Ogre::Hlms* getHlms(void) const;

        Ogre::HlmsPbs* getHlmsPbs(void) const;

        Ogre::HlmsUnlit* getHlmsUnlit(void) const;

        Ogre::HlmsManager* getHlmsManager(void) const;

        Ogre::CompositorManager2* getCompositorManager(void) const;

        /*
         * @brief Sets the shadow quality filter.
         * @param[in] shadowFilter The shadow filter to set. Possible values:
         * 	PCF_2x2: Standard quality. Very fast.
         * 	PCF_3x3: Good quality. Still quite fast on most modern hardware.
         * 	PCF_4x4: High quality. Very slow in old hardware (i.e. DX10 level hw and below). Use RSC_TEXTURE_GATHER to check whether it will be slow or not.
         * 	ExponentialShadowMaps: High quality. Produces soft shadows. It's much more expensive but given its blurry results, you can reduce resolution and/or use less PSSM splits
         * 	which gives you very competing performance with great results. ESM stands for Exponential Shadow Maps.
         * @param[in] recreateWorkspace Whether the workspace should be recreated (should be set to true, when shadows quality is changed during runtime).
         */
        void setShadowQuality(Ogre::HlmsPbs::ShadowFilter shadowFilter, bool recreateWorkspace);

        Ogre::HlmsPbs::ShadowFilter getShadowQuality(void) const;

        /*
         * @brief Sets the ambient light mode.
         * @param[in] ambientLightMode The ambient light mode. Possible values:
         * 	AmbientAuto: Use fixed-color ambient lighting when upper hemisphere = lower hemisphere, use hemisphere lighting when they don't match. Disables ambient lighting if the colours are black
         * 	AmbientFixed: Force fixed-color ambient light. Only uses the upper hemisphere paramter.
         * 	AmbientHemisphere: Force hemisphere ambient light. Useful if you plan on adjusting the colours dynamically very often and this might cause swapping shaders.
         * 	AmbientNone: Disable ambient lighting.
         */

        void setAmbientLightMode(Ogre::HlmsPbs::AmbientLightMode ambientLightMode);

        Ogre::HlmsPbs::AmbientLightMode getAmbientLightMode(void) const;

        void setPrimaryWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, WorkspaceBaseComponent* workspaceBaseComponent);

        void setPrimaryWorkspace2(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::CompositorWorkspace* workspace);

        void addNthWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, WorkspaceBaseComponent* workspaceBaseComponent);

        Ogre::CompositorWorkspace* getPrimaryWorkspace(Ogre::Camera* camera);

        WorkspaceBaseComponent* getPrimaryWorkspaceComponent(void);

        Ogre::CompositorWorkspace* getWorkspace(Ogre::Camera* camera);

        WorkspaceBaseComponent* getWorkspaceComponent(void);

        CameraComponent* getPrimaryCameraComponent(void) const;

        void removeWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera);

        void removeCamera(Ogre::Camera* camera);

        void setCameraPrimaryWorkspaceActive(void);

        /**
         * @brief		Enables or disables every workspace owned by this app state.
         *
         * Called by AppState::pause() and AppState::resume(). Ogre's CompositorManager2 is global
         * and renders every registered workspace regardless of which module owns it, so a paused
         * state would otherwise keep rendering - and its MyGUI pass would run against a scene
         * manager MyGUI is no longer bound to, crashing the render thread. See the implementation
         * for the full reasoning.
         *
         * @param[in]	enabled		True to resume rendering these workspaces, false to stand down.
         */
        void setAllWorkspacesEnabled(bool enabled);

        bool hasAnyWorkspace(void) const;

        bool hasMoreThanOneWorkspace(void) const;

        Ogre::uint8 getCountCameras(void);

        void setSplitScreenScenarioActive(bool splitScreenScenarioActive);

        bool getSplitScreenScenarioActive(void) const;

        void logAllCompositorNodeDefinitions(void);

        void logLiveNodeGraph(Ogre::CompositorWorkspace* workspace, const Ogre::String& label);

    public:
    public:
        struct AdaptiveQualityLevel
        {
            Ogre::Real shadowFarDistance;
            Ogre::Real foliageDistanceMultiplier;
            // resolutionScale can join this table later once you tackle it
        };

        // Call once per render frame from GraphicsModule::renderThreadFunction.
        // Must be called FROM the render thread - it touches scene manager /
        // shadow state directly, no enqueueAndWait needed since we already are it.
        void updateAdaptiveQuality(Ogre::Real renderDt);

        void configureAdaptiveQuality(std::vector<AdaptiveQualityLevel> levels, Ogre::Real targetFrameTimeMs);

    public:
        const Ogre::String workspaceNamePbs = "NOWAPbsWorkspace";
        const Ogre::String workspaceNameSky = "NOWASkyWorkspace";
        const Ogre::String workspaceNameSkyReflection = "NOWASkyReflectionWorkspace";
        const Ogre::String workspaceNamePbsPlanarReflection = "NOWAPbsPlanarReflectionWorkspace";
        const Ogre::String workspaceNameSkyPlanarReflection = "NOWASkyPlanarReflectionWorkspace";
        const Ogre::String workspaceNameBackgroundPlanarReflection = "NOWABackgroundPlanarReflectionWorkspace";
        const Ogre::String workspaceNameTerra = "NOWATerraWorkspace";
        const Ogre::String workspaceNameBackground = "NOWABackgroundWorkspace";
        Ogre::String shadowNodeName = "NOWAShadowNode";

    private:
        // Attention: this is no longer a singleton. Every app state owns its own WorkspaceModule,
        // created and destroyed by AppState::initializeModules() / destroyModules(). A singleton
        // was wrong here, because app states can be stacked (push/pop): destroying the content of
        // one state also tore down the workspaces of the states still paused underneath, whose
        // game objects are very much alive - their WorkspaceBaseComponent pointers then dangled
        // and AppState::resume() crashed on them. Split screen state, camera counts and adaptive
        // quality are per scene too, so they belong to the state that owns the scene manager.
        friend class AppState;

        WorkspaceModule(const Ogre::String& appStateName);
        ~WorkspaceModule();

        //////////////////////////ESM Shadow optimizations////////////////////////////////////////
        int retrievePreprocessorParameter(const Ogre::String& preprocessDefines, const Ogre::String& paramName);
        /**
        @param job
            The compute job to change.
        @param kernelRadius
            The kernel radius. A radius of 8 means there's 17x17 taps (8x2 + 1).
            Changing this parameter will trigger a recompile.
        @param gaussianDeviationFactor
            The std. deviation of a gaussian filter.
        @param K
            A big K (K > 20) means softer, blurrier shadows; but may reveal some artifacts on the
            edge cases ESM does not deal well, while small K (< 8) may hide those artifacts,
            but makes the shadows fat (fatter than normal, still blurry) and very dark.
            Small K combined with large radius (radius > 8) may cause self shadowing artifacts.
            A K larger than 80 may run into Ogre::Realing point precision issues (which means light
            bleeding or weird shadows appearing caused by NaNs).
            Changing this parameter will trigger a recompile.
        */
        void setGaussianLogFilterParams(Ogre::HlmsComputeJob* job, Ogre::uint8 kernelRadius, Ogre::Real gaussianDeviationFactor, Ogre::uint16 K);

        /// Adjusts the material (pixel shader variation). See other overload for param description.
        void setGaussianLogFilterParams(const Ogre::String& materialName, Ogre::uint8 kernelRadius, Ogre::Real gaussianDeviationFactor, Ogre::uint16 K);

        Ogre::CompositorWorkspace* createDummyWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera);

        void applyAdaptiveQualityLevel(int idx);

    private:
        Ogre::String appStateName;
        Ogre::Hlms* hlms;
        Ogre::HlmsPbs* pbs;
        Ogre::HlmsUnlit* unlit;

        Ogre::HlmsManager* hlmsManager;
        Ogre::CompositorManager2* compositorManager;
        Ogre::HlmsPbs::ShadowFilter shadowFilter;
        Ogre::HlmsPbs::AmbientLightMode ambientLightMode;

        struct WorkspaceData
        {
            Ogre::CompositorWorkspace* workspace = nullptr;
            WorkspaceBaseComponent* workspaceBaseComponent = nullptr;
            bool isDummy = false;
            bool isPrimary = false;
        };

        std::map<Ogre::Camera*, WorkspaceData> workspaceMap;

        bool splitScreenScenarioActive;

        std::vector<AdaptiveQualityLevel> adaptiveLevels;
        int adaptiveCurrentLevel = 0;
        Ogre::Real adaptiveTargetMs = 16.6f;
        Ogre::Real adaptiveAvgFrameTimeMs = 0.0f;
        Ogre::Real adaptiveCooldownMs = 0.0f;
    };

}; // namespace end

#endif