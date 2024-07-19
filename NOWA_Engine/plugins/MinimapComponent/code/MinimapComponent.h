/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef MINIMAPCOMPONENT_H
#define MINIMAPCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

#include <thread>

namespace NOWA
{
	class CameraComponent;
	class TerraComponent;

	/**
	  * @brief		Can be used for painting a minimap. Also fog of war is possible and the given game object id is tracked.
	  */
	class EXPORTED MinimapComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<MinimapComponent> MinimapComponentPtr;
	public:

		MinimapComponent();

		virtual ~MinimapComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		* @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
		*/
		virtual void initialise() override {};

		/**
		* @see		Ogre::Plugin::shutdown
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void shutdown() override {};

		/**
		* @see		Ogre::Plugin::uninstall
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void uninstall() override {};

		/**
		* @see		Ogre::Plugin::getName
		*/
		virtual const Ogre::String& getName() const override;
		
		/**
		* @see		Ogre::Plugin::getAbiCookie
		*/
		virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		/**
		 * @brief Sets target id for the game object, which shall be tracked.
		 * @param[in] targetId The target id to set
		 */
		void setTargetId(unsigned long targetId);

		/**
		 * @brief Gets the target id for the game object, which is tracked.
		 * @return targetId The target id to get
		 */
		unsigned long getTargetId(void) const;

		/**
		 * @brief Gets the target game object for the given target id, which is tracked.
		 */
		GameObject* getTargetGameObject(void) const;

		void setTextureSize(unsigned int textureSize);

		unsigned int getTextureSize(void) const;

		void setMinimapGeometry(const Ogre::Vector4& minimapGeometry);

		Ogre::Vector4 getMinimapGeometry(void) const;

		void setTransparent(bool transparent);

		bool getTransparent(void) const;

		void setUseFogOfWar(bool useFogOfWar);

		bool getUseFogOfWar(void) const;

		void setUseDiscovery(bool useDiscovery);

		bool getUseDiscovery(void) const;

		void setPersistDiscovery(bool persistDiscovery);

		bool getPersistDiscovery(void) const;

		void setVisibilityRadius(Ogre::Real visibilityRadius);

		Ogre::Real getVisibilityRadius(void) const;

		void setUseVisibilitySpotlight(bool useVisibilitySpotlight);

		bool getUseVisibilitySpotlight(void) const;

		void setWholeSceneVisible(bool wholeSceneVisible);

		bool getWholeSceneVisible(void) const;

		void setHideId(unsigned long hideId);

		unsigned long getHideId(void) const;

		void setUseRoundMinimap(bool useRoundMinimap);

		bool getUseRoundMinimap(void) const;

		void setRotateMinimap(bool rotateMinimap);

		bool getRotateMinimap(void) const;

		void clearDiscoveryState(void);

		bool saveDiscoveryState(void);

		bool loadDiscoveryState(void);

		Ogre::TextureGpu* getMinimapTexture(void) const;
	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MinimapComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "MinimapComponent";
		}
	
		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Can be used for painting a minimap. Also fog of war is possible and the given game object id is tracked. "
				"Requirements: This component must be placed under a separate game object with a camera component, which is not the MainCamera. "
				"Note: The minimap can only be used with default direction of -z, which is also the default graphics engine mesh direction. "
				"Note: If terra shall be rendered properly on minimap, this minimap camera game object id, must be set in the TerraComponent as camera id.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrTextureSize(void) { return "Texture Size"; }
		static const Ogre::String AttrMinimapGeometry(void) { return "Minimap Geometry"; }
		static const Ogre::String AttrTransparent(void) { return "Transparent"; }
		static const Ogre::String AttrUseFogOfWar(void) { return "Use Fog of War"; }
		static const Ogre::String AttrUseDiscovery(void) { return "Use Discovery"; }
		static const Ogre::String AttrPersistDiscovery(void) { return "Persist Discovery"; }
		static const Ogre::String AttrVisibilityRadius(void) { return "Visibility Radius"; }
		static const Ogre::String AttrUseVisibilitySpotlight(void) { return "Use Visibility Spotlight"; }
		static const Ogre::String AttrWholeSceneVisible(void) { return "Whole Scene Visible"; }
		static const Ogre::String AttrHideId(void) { return "Hide Id"; }
		static const Ogre::String AttrUseRoundMinimap(void) { return "Use Round Minimap"; }
		static const Ogre::String AttrRotateMinimap(void) { return "Rotate Minimap"; }
	private:
		void setupMinimapWithFogOfWar(void);

		Ogre::TextureGpu* createMinimapTexture(const Ogre::String& name, unsigned int width, unsigned int height);

		// Ogre::TextureGpu* createHeightMapTexture(const Ogre::String& name, unsigned int width, unsigned int height);

		Ogre::TextureGpu* createFogOfWarTexture(const Ogre::String& name, unsigned int width, unsigned int height);

		void createMinimapNode(void);

		void updateFogOfWarTexture(const Ogre::Vector3& position, Ogre::Real radius);

		void updateFogOfWarThreadFunc(void);

		// Ogre::TextureGpu* updateHeightMapTexture(Ogre::TextureGpu* texture, unsigned int width, unsigned int height);

		void removeWorkspace(void);

		Ogre::Vector2 normalizePosition(const Ogre::Vector3& position);
		
		Ogre::Vector2 mapToTextureCoordinates(const Ogre::Vector2& normalizedPosition, unsigned int textureWidth, unsigned int textureHeight);

		void adjustMinimapCamera(void);
	private:
		void deleteGameObjectDelegate(EventDataPtr eventData);
		void handleUpdateBounds(EventDataPtr eventData);
		void handleTerraChanged(EventDataPtr eventData);
	private:
		Ogre::String name;

		GameObject* targetGameObject;
		Ogre::TextureGpu* minimapTexture;
		Ogre::TextureGpu* heightMapTexture;
		Ogre::TextureGpu* fogOfWarTexture;
		Ogre::StagingTexture* heightMapStagingTexture;
		Ogre::StagingTexture* fogOfWarStagingTexture;
		Ogre::TextureGpuManager* textureManager;
		Ogre::CompositorWorkspace* workspace;
		Ogre::CompositorChannelVec externalChannels;
		Ogre::String minimapWorkspaceName;
		Ogre::String minimapNodeName;
		MyGUI::ImageBox* minimapWidget;
		MyGUI::ImageBox* heightMapWidget;
		MyGUI::ImageBox* fogOfWarWidget;

		Ogre::Vector3 minimumBounds;
		Ogre::Vector3 maximumBounds;
		CameraComponent* cameraComponent;
		TerraComponent* terraComponent;

		std::vector<std::vector<bool>> discoveryState;

		Ogre::Real timeSinceLastUpdate;

		std::thread fillThread;
		std::atomic<bool> running;
		std::mutex mutex;
		std::condition_variable condVar;
		std::atomic<bool> readyToUpdate;

		Variant* activated;
		Variant* targetId;
		Variant* textureSize;
		Variant* minimapGeometry;
		Variant* transparent;
		Variant* useFogOfWar;
		Variant* useDiscovery;
		Variant* persistDiscovery;
		Variant* visibilityRadius;
		Variant* useVisibilitySpotlight;
		Variant* wholeSceneVisible;
		Variant* hideId;
		Variant* useRoundMinimap;
		Variant* rotateMinimap;
	};

}; // namespace end

#endif

