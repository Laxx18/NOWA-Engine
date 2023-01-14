#ifndef WORKSPACE_COMPONENT_H
#define WORKSPACE_COMPONENT_H

#include "GameObjectComponent.h"
#include "OgreHlmsBufferManager.h"
#include "OgreConstBufferPool.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"
#include "OgrePlanarReflectionActor.h"
#include "OgrePlanarReflections.h"
#if 0
#include "ocean/OgreHlmsOcean.h"
#endif

#include "shader/HlmsWind.h"

namespace NOWA
{
	class CameraComponent;
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
		friend class WorkspaceModule;

		WorkspaceBaseComponent();

		virtual ~WorkspaceBaseComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		bool createWorkspace(void);

		virtual void removeWorkspace(void);

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

		void setUsePlanarReflection(bool usePlanarReflection);

		bool getUsePlanarReflection(void) const;

		bool getUseTerra(void) const;

		bool getUseOcean(void) const;

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

		void setPlanarMaxReflections(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute,
			const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector2& mirrorSize);

		void addPlanarReflectionsActor(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute,
			const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector2& mirrorSize);

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
	public:
		static const Ogre::String AttrBackgroundColor(void) { return "Background Color"; }
		static const Ogre::String AttrViewportRect(void) { return "Viewport Rect"; }
		static const Ogre::String AttrSuperSampling(void) { return "Super Sampling"; }
		static const Ogre::String AttrUseHdr(void) { return "Use HDR"; }
		static const Ogre::String AttrUseReflection(void) { return "Use Reflection"; }
		static const Ogre::String AttrUseSSAO(void) { return "Use SSAO"; }
		static const Ogre::String AttrUseDistortion(void) { return "Use Distortion"; }
		static const Ogre::String AttrReflectionCameraGameObjectId(void) { return "Reflection Camera GameObject Id"; }
		static const Ogre::String AttrUsePlanarReflection(void) { return "Use Planar Reflection"; }
		static const Ogre::String AttrShadowGlobalBias(void) { return "Shadow Global Bias"; }
		static const Ogre::String AttrShadowGlobalNormalOffset(void) { return "Shadow Global Normal Offset"; }
		static const Ogre::String AttrShadowPSSMLambda(void) { return "Shadow PSSM Lambda"; }
		static const Ogre::String AttrShadowSplitBlend(void) { return "Shadow Split Blend"; }
		static const Ogre::String AttrShadowSplitFade(void) { return "Shadow Split Fade"; }
		static const Ogre::String AttrShadowSplitPadding(void) { return "Shadow Split Padding"; }
	protected:
		virtual void internalInitWorkspaceData(void);

		virtual void internalCreateCompositorNode(void) = 0;

		virtual bool internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef) = 0;

		virtual void baseCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef);

		virtual void createDistortionNode(void);

		Ogre::String getDistortionNode(void) const;

		void changeBackgroundColor(const Ogre::ColourValue& backgroundColor);

		void changeViewportRect(unsigned short viewportIndex, const Ogre::Vector4& viewportRect);

		unsigned char getMSAA(void);

		void initializeHdr(Ogre::uint8 fsaa);

		void createCompositorEffectsFromCode(void);

		void updateShadowGlobalBias(void);

		void setWorkspace(Ogre::CompositorWorkspace* workspace);

		bool hasAnyMirrorForPlanarReflections(void);

		enum PresetQuality
		{
			SMAA_PRESET_LOW,        // (%60 of the quality)
			SMAA_PRESET_MEDIUM,     // (%80 of the quality)
			SMAA_PRESET_HIGH,       // (%95 of the quality)
			SMAA_PRESET_ULTRA       // (%99 of the quality)
		};

		enum EdgeDetectionMode
		{
			EdgeDetectionDepth,     // Fastest, not supported in Ogre.
			EdgeDetectionLuma,      // Ok. The default on many implementations.
			EdgeDetectionColor,     // Best quality
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

		void setUseOcean(bool useOcean);

		void createSSAONoiseTexture(void);
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
		Variant* shadowGlobalBias;
		Variant* shadowGlobalNormalOffset;
		Variant* shadowPSSMLambda;
		Variant* shadowSplitBlend;
		Variant* shadowSplitFade;
		Variant* shadowSplitPadding;

		bool canUseReflection;
		
		CameraComponent* cameraComponent;
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
		Ogre::TextureGpu* cubemap;
		Ogre::CompositorWorkspace* workspaceCubemap;
		Ogre::CompositorChannelVec externalChannels;
		// Ogre::ResourceLayoutMap initialCubemapLayouts;
		// Ogre::ResourceAccessMap initialCubemapUavAccess;

		Ogre::PlanarReflections* planarReflections;
		PlanarReflectionsWorkspaceListener* planarReflectionsWorkspaceListener;
		std::vector<std::tuple<unsigned long, unsigned int, Ogre::PlanarReflectionActor*>> planarReflectionActors;

		// Special: Only a TerraComponent can manipulate this value
		bool useTerra;
		Ogre::Terra* terra;

		// Special: Only a OceanComponent can manipulate this value
		bool useOcean;
#if 0
		Ogre::Ocean* ocean;
#endif

		Ogre::HlmsListener* hlmsListener;

		HlmsWind* hlmsWind;

		Ogre::Pass* passSSAO;
		Ogre::Pass* passSSAOApply;
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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		void setSkyBoxName(const Ogre::String& skyBoxName);

		Ogre::String getSkyBoxName(void) const;
	public:
		static const Ogre::String AttrSkyBoxName(void) { return "Sky Box Name"; }
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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
			return NOWA::getIdFromName("WorkspaceBackgroundComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "WorkspaceBackgroundComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
	protected:
		virtual void internalCreateCompositorNode(void) override;

		virtual bool internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef) override;

		virtual void removeWorkspace(void);
	private:

		void changeBackground(unsigned short index, const Ogre::String& backgroundTextureName);

		void setBackgroundScrollSpeedX(unsigned short index, Ogre::Real backgroundScrollFarSpeedX);

		void setBackgroundScrollSpeedY(unsigned short index, Ogre::Real backgroundScrollFarSpeedY);

		void compileBackgroundMaterial(unsigned short index);
	private:
		Ogre::MaterialPtr materialBackgroundPtr[9];
		Ogre::Pass* passBackground[9];
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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		void setCustomWorkspaceName(const Ogre::String& customWorkspaceName);

		Ogre::String getCustomWorkspaceName(void) const;
	public:
		static const Ogre::String AttrCustomWorkspaceName(void) { return "Custom Workspace Name"; }
	protected:
		virtual void internalInitWorkspaceData(void) override;

		virtual void internalCreateCompositorNode(void) override;

		virtual bool internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef) override;
	private:
		Variant* customWorkspaceName;
	};

}; //namespace end

#endif