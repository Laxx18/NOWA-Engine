/*
Copyright (c) 2025 Lukas Kalinowski

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
            return "Usage: Renders this camera's view into its own split screen texture, which is combined with all other activated split screen cameras into the final image. "
                   "Must be placed on a camera game object, below its CameraComponent and a WorkspaceBaseComponent (e.g. WorkspacePbsComponent).\n"
                   "Activation: The split screen scenario starts as soon as the FIRST activated SplitScreenComponent in the scene connects (simulation start), and ends once the LAST one disconnects, at which point the main camera is activated "
                   "again. "
                   "Every camera that shall take part in the split screen needs this component with Activated = true.\n"
                   "Geometry: Position and size of this camera's tile on screen, relative to the window (0..1 range), format Vector4(pos.x, pos.y, width, height). "
                   "Can be set by hand, or computed automatically for all activated split screen cameras at once via SplitPreset.\n"
                   "Example: 2 player vertical split: geometry1 0.5 0 0.5 1 geometry2 0 0 0.5 1\n"
                   "Example: 2 player horizonal split: geometry1 0 0.5 1 0.5 geometry2 0 0 1 0.5\n"
                   "Example: 3 player vertical split: geometry1 0 0 0.3333 1 geometry2 0.3333 0 0.3333 1 geometry3 0.6666 0 0.3333 1\n"
                   "Example: 4 player vertical/horizontal split: geometry1 0 0.5 0.5 0.5 geometry2 0.5 0.5 0.5 0.5 geometry3 0 0 0.5 0.5  geometry4 0.5 0 0.5 0.5\n"
                   "Per-camera visibility: What THIS camera renders is controlled by the camera game object's OWN RenderCategory attribute (GameObject::RenderCategory). "
                   "Example: give an obstacle game object RenderCategory 'Object', then set this split screen camera's own RenderCategory to 'All-Object' to hide that obstacle only for this camera, while a second split screen camera with "
                   "RenderCategory 'All' still renders it.\n"
                   "Camera behavior: CameraBehaviorGameObjectId is optional. Leave it at 0 for a statically placed split screen camera. Set it to a game object id that owns a CameraBehaviorComponent (FirstPersonCamera, ThirdPersonCamera etc.) to "
                   "let that behavior drive this split screen camera.";
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
        static const Ogre::String AttrApplyPreset(void) { return "Apply Preset"; }
	private:
		Ogre::TextureGpu* createSplitScreenTexture(const Ogre::String& name);

		void setupSplitScreen(void);

		void cleanupSplitScreen(void);

		Ogre::Vector4 computeGeometryFromPreset(const Ogre::String& preset, size_t index, size_t totalCount);

		void applyPreset(void);
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

		Variant* activated;
		Variant* textureSize;
		Variant* geometry;
		Variant* cameraBehaviorGameObjectId;
        Variant* splitPreset;    // List: "Custom", "2Vertical", "2Horizontal", "3Vertical", "3Horizontal", "4Grid". Transient, not saved/loaded.
	};

}; // namespace end

#endif

