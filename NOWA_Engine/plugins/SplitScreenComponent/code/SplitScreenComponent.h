/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef SPLITSCREENCOMPONENT_H
#define SPLITSCREENCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class CameraComponent;
	class WorkspaceBaseComponent;

	/**
	  * @brief		Can be used to render a workspace in a split screen texture.
	  */
	class EXPORTED SplitScreenComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<SplitScreenComponent> SplitScreenCompPtr;
	public:

		SplitScreenComponent();

		virtual ~SplitScreenComponent();

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
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setTextureSize(const Ogre::Vector2& textureSize);

		Ogre::Vector2 getTextureSize(void) const;

		void setGeometry(const Ogre::Vector4& geometry);

		Ogre::Vector4 getGeometry(void) const;

		/**
		 * @brief Sets the camera behavior game object id in order if this camera bevhavior shall be used for an other camera, e.g. Splitscreen.
		 * @param[in] cameraBehaviorGameObjectId The cameraBehaviorGameObjectId to set.
		 * @note: If 0 (not set), the currently active camera is used.
		 */
		void setCameraBehaviorGameObjectId(const unsigned long cameraBehaviorGameObjectId);

		/**
		 * @brief Gets the camera behavior game object id in order if this camera bevhavior shall be used for an other camera, e.g. Splitscreen.
		 * @return cameraBehaviorGameObjectId The cameraBehaviorGameObjectId to get
		 */
		unsigned long getCameraBehaviorGameObjectId(void) const;

		Ogre::TextureGpu* getSplitScreenTexture(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("SplitScreenComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "SplitScreenComponent";
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
			return "Usage: Can be used to render a workspace in a split screen texture.\n"
				"Example: 2 player vertical split : geometry1 0.5 0 0.5 1 geometry2 0 0 0.5 1\n"
				"Example: 2 player horizonal split: geometry1 0 0.5 1 0.5 geometry2 0 0 1 0.5\n"
				"Example: 3 player vertical split: geometry1 0 0 0.3333 1 geometry2 0.3333 0 0.3333 1 geometry3 0.6666 0 0.3333 1\n"
				"Example: 4 player vertical/horizontal split: geometry1 0 0.5 0.5 0.5 geometry2 0.5 0.5 0.5 0.5 geometry3 0 0 0.5 0.5  geometry4 0.5 0 0.5 0.5\n";
		}

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTextureSize(void) { return "Texture Size (w, h)"; }
		static const Ogre::String AttrGeometry(void) { return "Geometry"; }
		static const Ogre::String AttrCameraBehaviorGameObjectId(void) { return "Camera Behavior GameObject Id"; }
		
	private:
		Ogre::TextureGpu* createSplitScreenTexture(const Ogre::String& name);

		void setupSplitScreen(void);

		void cleanupSplitScreen(void);
	private:
		Ogre::String name;
		Ogre::TextureGpu* splitScreenTexture;
		Ogre::TextureGpuManager* textureManager;
		CameraComponent* cameraComponent;
		WorkspaceBaseComponent* workspaceBaseComponent;
		Ogre::CompositorChannelVec externalChannels;
		bool componentBeingLoaded;
		Ogre::Camera* tempCamera;
		Ogre::CompositorWorkspace* finalCombinedWorkspace;
		bool bIsInSimulation;

		Variant* activated;
		Variant* textureSize;
		Variant* geometry;
		Variant* cameraBehaviorGameObjectId;
	};

}; // namespace end

#endif

