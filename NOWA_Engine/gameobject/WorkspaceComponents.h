#ifndef WORKSPACE_COMPONENT_H
#define WORKSPACE_COMPONENT_H

#include "GameObjectComponent.h"
#include "OgreConstBufferPool.h"
#include "OgreHlmsBufferManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"
#include "OgrePlanarReflectionActor.h"
#include "OgrePlanarReflections.h"

#include "main/Events.h"

#if 0
#include "ocean/OgreHlmsOcean.h"
#endif

#include "shader/HlmsWind.h"

namespace Ogre
{
    class ParallaxCorrectedCubemapAuto;
    class Terra;
    class CompositorNodeDef;
    class CompositorTargetDef;
    class CompositorPassSceneDef;
}

namespace NOWA
{
    class CameraComponent;
    class OceanComponent;
    class PlanarReflectionsWorkspaceListener;
    // class HlmsFogListener;
    // class HlmsDebugLogListener;
    class HlmsComputeJob;

    class EXPORTED WorkspaceBaseComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::WorkspaceBaseComponent> WorkspaceBaseCompPtr;

    public:
        friend class CompositorEffectBaseComponent;
        friend class TerraComponent;
        friend class OceanComponent;
        friend class WorkspaceModule;
        friend class CameraComponent;

        WorkspaceBaseComponent();

        virtual ~WorkspaceBaseComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index);

        /**
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void) override;

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see		GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        /**
         * @see		GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("WorkspaceBaseComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "WorkspaceBaseComponent";
        }

        /**
         * @see  GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "";
        }

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        bool createWorkspace(void);

        virtual void removeWorkspace(void);

        void nullWorkspace(void);

        void setBackgroundColor(const Ogre::Vector3& backgroundColor);

        Ogre::Vector3 getBackgroundColor(void) const;

        void setViewportRect(const Ogre::Vector4& viewportRect);

        Ogre::Vector4 getViewportRect(void) const;

        void setSuperSampling(Ogre::Real superSampling);

        Ogre::Real getSuperSampling(void) const;

        void setUseHdr(bool useHdr);

        bool getUseHdr(void) const;

        void setUseReflection(bool useReflection);

        bool getUseReflection(void) const;

        void setUseSSAO(bool useSSAO);

        bool getUseSSAO(void) const;

        void setUseDistortion(bool useDistortion);

        bool getUseDistortion(void) const;

        /**
         * @brief Activates the water volume pipeline: an extra render target (rt_waterVolume) into which every
         *        WaterVolumeComponent writes its screen-space mask plus front/back depth (render queue
         *        WATER_VOLUME_RENDER_QUEUE), and a fullscreen WaterVolume/Quad pass that applies distortion, fog,
         *        absorption, caustics and a waterline only where that mask is set. Completely independent from the
         *        ocean underwater effect (which is a camera-is-underwater fullscreen effect for 3D).
         * @param[in] useWaterVolume Whether to use the water volume pipeline
         */
        void setUseWaterVolume(bool useWaterVolume);

        bool getUseWaterVolume(void) const;

        void setUseMSAA(bool useMSAA);

        bool getUseMSAA(void) const;

        void setUsePlanarReflection(bool usePlanarReflection);

        bool getUsePlanarReflection(void) const;

        bool getUseTerra(void) const;

        bool getUseOcean(void) const;

        void setUsePCC(bool usePCC);

        bool getUsePCC(void) const;

        /**
         * @brief Sets reflection game object id in to set for cube map reflection.
         * @param[in] reflectionCameraGameObjectId The reflection camera game object Id to set
         */
        void setReflectionCameraGameObjectId(unsigned long reflectionCameraGameObjectId);

        /**
         * @brief Gets the reflection camera game object id in to set for cube map reflection.
         * @return reflectionCameraGameObjectId The reflection camera game object Id to get
         */
        unsigned long getReflectionCameraGameObjectId(void) const;

        Ogre::String getWorkspaceName(void) const;

        Ogre::String getRenderingNodeName(void) const;

        Ogre::String getFinalRenderingNodeName(void) const;

        Ogre::CompositorWorkspace* getWorkspace(void) const;

        Ogre::TextureGpu* getDynamicCubemapTexture(void) const;

        void setPlanarMaxReflections(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute, const Ogre::Vector3& position, const Ogre::Quaternion& orientation,
            const Ogre::Vector2& mirrorSize);

        void addPlanarReflectionsActor(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute, const Ogre::Vector3& position, const Ogre::Quaternion& orientation,
            const Ogre::Vector2& mirrorSize);

        void removePlanarReflectionsActor(unsigned long gameObjectId);

        Ogre::PlanarReflections* getPlanarReflections(void) const;

        void setShadowGlobalBias(Ogre::Real shadowGlobalBias);

        Ogre::Real getShadowGlobalBias(void) const;

        void setShadowGlobalNormalOffset(Ogre::Real shadowGlobalNormalOffset);

        Ogre::Real getShadowGlobalNormalOffset(void) const;

        void setShadowPSSMLambda(Ogre::Real shadowPssmLambda);

        Ogre::Real getShadowPSSMLambda(void) const;

        void setShadowSplitBlend(Ogre::Real shadowSplitBlend);

        Ogre::Real getShadowSplitBlend(void) const;

        void setShadowSplitFade(Ogre::Real shadowSplitFade);

        Ogre::Real getShadowSplitFade(void) const;

        void setShadowSplitPadding(Ogre::Real shadowSplitPadding);

        Ogre::Real getShadowSplitPadding(void) const;

        void setCustomExternalChannels(const Ogre::CompositorChannelVec& customExternalChannels);

        void setInvolvedInSplitScreen(bool involvedInSplitScreen);

        bool getInvolvedInSplitScreen(void) const;

        Ogre::ParallaxCorrectedCubemapAuto* getParallaxCorrectedCubemap(void) const;

        void setParallaxCorrectedCubemap(Ogre::ParallaxCorrectedCubemapAuto* pcc);

        void destroyPccSystem(void);

    public:
        static const Ogre::String AttrBackgroundColor(void)
        {
            return "Background Color";
        }
        static const Ogre::String AttrViewportRect(void)
        {
            return "Viewport Rect";
        }
        static const Ogre::String AttrSuperSampling(void)
        {
            return "Super Sampling";
        }
        static const Ogre::String AttrUseHdr(void)
        {
            return "Use HDR";
        }
        static const Ogre::String AttrUseReflection(void)
        {
            return "Use Reflection";
        }
        static const Ogre::String AttrUseSSAO(void)
        {
            return "Use SSAO";
        }
        static const Ogre::String AttrUseDistortion(void)
        {
            return "Use Distortion";
        }
        static const Ogre::String AttrUseWaterVolume(void)
        {
            return "Use Water Volume";
        }

        /**
         * @brief Render queue rendered exclusively into rt_distortion (see DistortionComponent). Excluded from the
         *        main scene passes whenever distortion or water volumes are active.
         */
        static constexpr Ogre::uint8 DISTORTION_RENDER_QUEUE = 16;

        /**
         * @brief Render queue rendered exclusively into rt_waterVolume (see WaterVolumeComponent). Must directly follow
         *        DISTORTION_RENDER_QUEUE, because the main scene pass is split around the contiguous range
         *        [DISTORTION_RENDER_QUEUE, WATER_VOLUME_RENDER_QUEUE]. Make sure no regular objects use either queue.
         */
        static constexpr Ogre::uint8 WATER_VOLUME_RENDER_QUEUE = 17;
        static const Ogre::String AttrUseMSAA(void)
        {
            return "Use MSAA";
        }
        static const Ogre::String AttrUsePCC(void)
        {
            return "Use PCC";
        }
        static const Ogre::String AttrReflectionCameraGameObjectId(void)
        {
            return "Reflection Camera GameObject Id";
        }
        static const Ogre::String AttrUsePlanarReflection(void)
        {
            return "Use Planar Reflection";
        }
        static const Ogre::String AttrShadowGlobalBias(void)
        {
            return "Shadow Global Bias";
        }
        static const Ogre::String AttrShadowGlobalNormalOffset(void)
        {
            return "Shadow Global Normal Offset";
        }
        static const Ogre::String AttrShadowPSSMLambda(void)
        {
            return "Shadow PSSM Lambda";
        }
        static const Ogre::String AttrShadowSplitBlend(void)
        {
            return "Shadow Split Blend";
        }
        static const Ogre::String AttrShadowSplitFade(void)
        {
            return "Shadow Split Fade";
        }
        static const Ogre::String AttrShadowSplitPadding(void)
        {
            return "Shadow Split Padding";
        }
        static Ogre::String AttrUseDepthTexture(void)
        {
            return "Use Depth Texture";
        }

    protected:
        virtual void internalInitWorkspaceData(void);

        virtual void internalCreateCompositorNode(void) = 0;

        virtual bool internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef) = 0;

        virtual void baseCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef);

        virtual void createFinalRenderNode(void);

        virtual void createDistortionNode(void);

        virtual void createUnderwaterNode(void);

        /**
         * @brief Creates the fullscreen node that applies the water volume effect (inputs: scene colour, rt_waterVolume,
         *        depthTexture). Runs on the render thread (called from baseCreateWorkspace()).
         */
        virtual void createWaterVolumeNode(void);

        /**
         * @brief Adds a render target (texture definition + render target view) for an effect render queue such as
         *        rt_distortion or rt_waterVolume. The target shares depthTexture as its depth attachment, so objects in
         *        the effect queue are correctly occluded by scene geometry in front of them. Size factor and MSAA sample
         *        count therefore have to match depthTexture exactly.
         */
        void setupEffectQueueRenderTarget(Ogre::CompositorNodeDef* compositorNodeDefinition, const Ogre::String& textureName);

        /**
         * @brief Adds the target pass which renders exactly one effect render queue into the given effect render target.
         *        Colour is cleared, the scene depth is loaded (not cleared) for occlusion.
         */
        void addEffectQueuePass(Ogre::CompositorNodeDef* compositorNodeDefinition, const Ogre::String& textureName, Ogre::uint8 renderQueue, const Ogre::ColourValue& clearColour, const Ogre::String& profilingId);

        /**
         * @brief If distortion or water volumes are active and the given main scene pass covers the effect render queues,
         *        the pass is cut at DISTORTION_RENDER_QUEUE and a second pass for everything after
         *        WATER_VOLUME_RENDER_QUEUE is appended directly after it. Without this, effect objects would also be
         *        rendered visibly into rt0. Does nothing when neither effect is used, so the merged single pass stays.
         */
        void splitScenePassAroundEffectQueues(Ogre::CompositorTargetDef* targetDef, Ogre::CompositorPassSceneDef* passScene);

        /**
         * @brief Connects sourceNodeName -> [ocean underwater node] -> [water volume node] -> final node. Shared by
         *        baseCreateWorkspace() and reconnectAllNodes() for all HDR/MSAA variants.
         */
        void connectPostSceneEffects(Ogre::CompositorWorkspaceDef* workspaceDef, const Ogre::IdString& sourceNodeName, unsigned short channelDepthTexture, unsigned short channelWaterVolume, bool oceanUnderwater);

        virtual void addWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef);

        virtual void createLocalCubemapProbeRendererNode(void);

        virtual void createLocalCubemapsProbeWorkspace(void);

        Ogre::String getDistortionNode(void) const;

        Ogre::String getUnderwaterNode(void) const;

        void changeBackgroundColor(const Ogre::ColourValue& backgroundColor);

        unsigned char getMSAA(void);

        void initializeHdr(Ogre::uint8 fsaa);

        void updateShadowGlobalBias(void);

        void setWorkspace(Ogre::CompositorWorkspace* workspace);

        bool hasAnyMirrorForPlanarReflections(void);

        enum PresetQuality
        {
            SMAA_PRESET_LOW,    // (%60 of the quality)
            SMAA_PRESET_MEDIUM, // (%80 of the quality)
            SMAA_PRESET_HIGH,   // (%95 of the quality)
            SMAA_PRESET_ULTRA   // (%99 of the quality)
        };

        enum EdgeDetectionMode
        {
            EdgeDetectionDepth, // Fastest, not supported in Ogre.
            EdgeDetectionLuma,  // Ok. The default on many implementations.
            EdgeDetectionColor, // Best quality
        };

        /** By default the SMAA shaders will be compiled using conservative settings so it
            can run on any hardware. You should call this function at startup so we can
            configure and compile (or recompile) the shaders with optimal settings for
            the current hardware the user is running.
        @param renderSystem
        @param quality
            See PresetQuality
        @param edgeDetectionMode
            See EdgeDetectionMode
        */
        void initializeSmaa(PresetQuality quality, EdgeDetectionMode edgeDetectionMode);

        void resetReflectionForAllEntities(void);

        void setDataBlockPbsReflectionTextureName(GameObject* gameObject, const Ogre::String& textureName);

        void setUseTerra(bool useTerra);

        void setUseOcean(bool useOcean, OceanComponent* oceanComponent);

        void createSSAONoiseTexture(void);

    private:
        void reconnectAllNodes(void);

        void enableEffect(const Ogre::String& effectName, bool activated);

    protected:
        Variant* backgroundColor;
        Variant* viewportRect;
        Variant* superSampling;
        Variant* useHdr;
        Variant* useReflection;
        Variant* reflectionCameraGameObjectId;
        Variant* usePlanarReflection;
        Variant* useSSAO;
        Variant* useDistortion;
        Variant* useWaterVolume;
        Variant* useMSAA;
        Variant* usePCC;
        Variant* shadowGlobalBias;
        Variant* shadowGlobalNormalOffset;
        Variant* shadowPSSMLambda;
        Variant* shadowSplitBlend;
        Variant* shadowSplitFade;
        Variant* shadowSplitPadding;

        bool canUseReflection;

        CameraComponent* cameraComponent;
        OceanComponent* oceanComponent;
        Ogre::CompositorWorkspace* workspace;
        Ogre::String workspaceName;
        Ogre::String renderingNodeName;
        Ogre::String finalRenderingNodeName;
        Ogre::String planarReflectionReflectiveWorkspaceName;
        Ogre::String planarReflectionReflectiveRenderingNode;
        Ogre::String distortionNode;
        unsigned char msaaLevel;
        Ogre::Vector3 oldBackgroundColor;
        Ogre::Hlms* hlms;
        Ogre::HlmsPbs* pbs;
        Ogre::HlmsUnlit* unlit;
        Ogre::HlmsManager* hlmsManager;
        Ogre::CompositorManager2* compositorManager;
        Ogre::TextureGpu* cubemapTexture;
        Ogre::CompositorWorkspace* workspaceCubemap;
        Ogre::CompositorChannelVec externalChannels;
        Ogre::CompositorChannelVec customExternalChannels;

        Ogre::PlanarReflections* planarReflections;
        PlanarReflectionsWorkspaceListener* planarReflectionsWorkspaceListener;
        std::vector<std::tuple<unsigned long, unsigned int, Ogre::PlanarReflectionActor*>> planarReflectionActors;

        bool useTerra;
        Ogre::Terra* terra;
        bool useOcean;
        bool canUseOcean;

        /// True if the current workspace graph was built in "ocean underwater" mode.
        /// We use this to rebuild the workspace only when the state toggles.
        bool oceanUnderwaterActive;

        /// Optional compositor node inserted between the scene output and the final node when underwater.
        Ogre::String underwaterNodeName;

        Ogre::HlmsListener* hlmsListener;

        HlmsWind* hlmsWind;

        bool involvedInSplitScreen;
        Ogre::MaterialPtr underwaterMaterial;

        /// Fullscreen node applying the water volume effect (see createWaterVolumeNode()).
        Ogre::String waterVolumeNodeName;
        Ogre::MaterialPtr waterVolumeMaterial;

        Ogre::ParallaxCorrectedCubemapAuto* parallaxCorrectedCubemap;
        Ogre::CompositorWorkspace* workspacePccProbes;

        /**
         * @brief   True while connectExternal(0, finalRenderingNodeName, 0) is declared on the current
         *          workspace definition.
         *
         *          Ogre exposes no getter for the external channel routes - _getChannelRoutes() returns
         *          the INTER-NODE routes, and connectExternal() writes into mExternalChannelRoutes
         *          instead. The state therefore has to be tracked here. It must be updated at every
         *          place where the external route is declared or dropped, which is exactly:
         *          baseCreateWorkspace() (declares it), and clearOutputConnections() / clearAll() /
         *          removeWorkspace() (drop it).
         */
        bool externalChannelConnected;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED WorkspacePbsComponent : public WorkspaceBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::WorkspacePbsComponent> WorkspacePbsCompPtr;

    public:
        WorkspacePbsComponent();

        virtual ~WorkspacePbsComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("WorkspacePbsComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "WorkspacePbsComponent";
        }

        /**
         * @see  GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: With this component a physically based workspace is created for the whole scene rendering. "
                   "Requirements: This component can only be placed under a game object that possesses a CameraComponent.";
        }

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

    protected:
        virtual void internalCreateCompositorNode(void) override;

        virtual bool internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef) override;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED WorkspaceSkyComponent : public WorkspaceBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::WorkspaceSkyComponent> WorkspaceSkyCompPtr;

    public:
        WorkspaceSkyComponent();

        virtual ~WorkspaceSkyComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("WorkspaceSkyComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "WorkspaceSkyComponent";
        }

        /**
         * @see  GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: With this component a physically based workspace with sky as background cube map is created for the whole scene rendering. "
                   "Requirements: This component can only be placed under a game object that possesses a CameraComponent.";
        }

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        void setSkyBoxName(const Ogre::String& skyBoxName);

        Ogre::String getSkyBoxName(void) const;

    public:
        static const Ogre::String AttrSkyBoxName(void)
        {
            return "Sky Box Name";
        }

    protected:
        virtual void internalCreateCompositorNode(void) override;

        virtual bool internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef) override;

    private:
        void changeSkyBox(const Ogre::String& skyBoxName);

    private:
        Variant* skyBoxName;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED WorkspaceBackgroundComponent : public WorkspaceBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::WorkspaceBackgroundComponent> WorkspaceBackgroundCompPtr;

    public:
        friend class BackgroundScrollComponent;

        WorkspaceBackgroundComponent();

        virtual ~WorkspaceBackgroundComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		WorkspaceBaseComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see		GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("WorkspaceBackgroundComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "WorkspaceBackgroundComponent";
        }

        /**
         * @see  GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: With this component a physically based workspace with a background is created for the whole scene rendering. "
                   "Note: This component can be used in conjunction with the BackgroundScrollComponent in order to create nice scroll effects e.g. for a 2.5D Jump'n' Run game."
                   "Requirements: This component can only be placed under a game object that possesses a CameraComponent.";
        }

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        void setHardwareGammaEnabled(bool hardwareGammaEnabled);

        bool getHardwareGammaEnabled(void) const;

    protected:
        virtual void internalCreateCompositorNode(void) override;

        virtual bool internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef) override;

        virtual void removeWorkspace(void);

    private:
        void changeBackground(unsigned short index, const Ogre::String& backgroundTextureName);

        void resetBackgroundScrollPosition(unsigned short index);

        void setBackgroundScrollSpeedX(unsigned short index, Ogre::Real backgroundScrollFarSpeedX);

        void setBackgroundScrollSpeedY(unsigned short index, Ogre::Real backgroundScrollFarSpeedY);

        void compileBackgroundMaterial(void);

    public:
        static const Ogre::String AttrHardwareGammaEnabled(void)
        {
            return "Hardware Gamma Enabled";
        }

    private:
        Ogre::MaterialPtr materialBackgroundPtr;
        Ogre::Pass* passBackground;
        Variant* hardwareGammaEnabled;
        Ogre::Real layerEnabled[9];
        int activeLayerCount;
        Ogre::Real speedsXValues[9];
        Ogre::Real speedsYValues[9];
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED WorkspaceCustomComponent : public WorkspaceBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::WorkspaceCustomComponent> WorkspaceCustomCompPtr;

    public:
        WorkspaceCustomComponent();

        virtual ~WorkspaceCustomComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("WorkspaceCustomComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "WorkspaceCustomComponent";
        }

        /**
         * @see  GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: With this component a physically based workspace with custom (externally specified in script) definition. "
                   "Requirements: This component can only be placed under a game object that possesses a CameraComponent.";
        }

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        void setCustomWorkspaceName(const Ogre::String& customWorkspaceName);

        Ogre::String getCustomWorkspaceName(void) const;

    public:
        static const Ogre::String AttrCustomWorkspaceName(void)
        {
            return "Custom Workspace Name";
        }

    protected:
        virtual void internalInitWorkspaceData(void) override;

        virtual void internalCreateCompositorNode(void) override;

        virtual bool internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef) override;

    private:
        Variant* customWorkspaceName;
    };

}; // namespace end

#endif