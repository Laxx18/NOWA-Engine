#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include "GameObjectComponent.h"
#include "camera/BaseCamera.h"
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED CameraComponent : public GameObjectComponent
	{
	public:

		friend class WorkspaceBaseComponent;

		typedef boost::shared_ptr<NOWA::CameraComponent> CameraCompPtr;
	public:
	
		CameraComponent();

		virtual ~CameraComponent();

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
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

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
			return NOWA::getIdFromName("CameraComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CameraComponent";
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
			return "Usage: Creates a camera for camera switching or viewport tiling or vr etc.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @brief	Sets this camera component as active one.
		 * @param[in] activated Whether ativate this camera component or not
		 */
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		void applySplitScreen(bool useSplitScreen, int eyeId);

		/**
		 * @brief	Just sets internally the actived flag
		 * @param[in] activated Whether the activate flag is set to true or false
		 */
		void setActivatedFlag(bool activated);

		void setCameraPosition(const Ogre::Vector3& position);

		Ogre::Vector3 getCameraPosition(void) const;

		void setCameraDegreeOrientation(const Ogre::Vector3& orientation);

		void setCameraOrientation(const Ogre::Quaternion& orientation);

		Ogre::Vector3 getCameraDegreeOrientation(void) const;

		void setNearClipDistance(Ogre::Real nearClipDistance);

		Ogre::Real getNearClipDistance(void) const;

		void setFarClipDistance(Ogre::Real farClipDistance);

		Ogre::Real getFarClipDistance(void) const;

		void setOrthographic(bool orthographic);

		bool getIsOrthographic(void) const;

		void setOrthoWindowSize(const Ogre::Vector2& orthoWindowSize);

		Ogre::Vector2 getOrthoWindowSize(void) const;

		void setFovy(const Ogre::Degree& fovy);

		Ogre::Degree getFovy(void) const;

		void setFixedYawAxis(bool fixedYawAxis);

		bool getFixedYawAxis(void) const;

		void setShowDummyEntity(bool showDummyEntity);

		bool getShowDummyEntity(void) const;

		void setExcludeRenderCategories(const Ogre::String& excludeRenderCategories);

		void setAspectRatio(Ogre::Real aspectRatio);

		Ogre::String getExcludeRenderCategories(void) const;

		Ogre::Camera* getCamera(void) const;

		Ogre::uint8 getEyeId(void) const;
	public:
		static const Ogre::String AttrActive(void){ return "Active"; }
		static const Ogre::String AttrPosition(void) { return "Position"; }
		static const Ogre::String AttrOrientation(void) { return "Orientation"; }
		static const Ogre::String AttrNearClipDistance(void) { return "Near Clip Distance"; }
		static const Ogre::String AttrFarClipDistance(void) { return "Far Clip Distance"; }
		static const Ogre::String AttrOrthographic(void) { return "Orthographic"; }
		static const Ogre::String AttrOrthoWindowSize(void) { return "Ortho Window Size"; }
		static const Ogre::String AttrFovy(void) { return "Fovy"; }
		static const Ogre::String AttrFixedYawAxis(void) { return "Fixed Yaw Axis"; }
		static const Ogre::String AttrShowDummyEntity(void) { return "Show Dummy Entity"; }
		static const Ogre::String AttrExcludeRenderCategories(void) { return "Exclude Render Categories"; }

		/**
		* @brief This is required when a camera is created via the editor, it must be placed where placenode has been when the user clicked the mouse button.
		*		 But when a camera is loaded from scene, it must not have an orientation, else there are ugly side effects
		*/
		static void setJustCreated(bool justCreated);
	private:
		void createCamera(void);
		void handleSwitchCamera(NOWA::EventDataPtr eventData);
		void handleRemoveCamera(NOWA::EventDataPtr eventData);
		void handleRemoveCameraBehavior(NOWA::EventDataPtr eventData);
	private:
		static bool justCreated;
	private:
		Ogre::Camera* camera;
		Ogre::v1::Entity* dummyEntity;
		BaseCamera* baseCamera;
		Variant* active;
		Variant* position;
		Variant* orientation;
		Variant* nearClipDistance;
		Variant* farClipDistance;
		Variant* orthographic;
		Variant* orthoWindowSize;
		Variant* fovy;
		Variant* fixedYawAxis;
		Variant* showDummyEntity;
		Variant* excludeRenderCategories;
		
		Ogre::Real timeSinceLastUpdate;
		WorkspaceBaseComponent* workspaceBaseComponent;
		int eyeId;
	};

}; //namespace end

#endif