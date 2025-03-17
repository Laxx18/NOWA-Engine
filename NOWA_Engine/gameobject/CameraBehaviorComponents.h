#ifndef CAMERA_BEHAVIOR_COMPONENTS_H
#define CAMERA_BEHAVIOR_COMPONENTS_H

#include "GameObjectComponent.h"
#include "camera/BaseCamera.h"
#include "main/Events.h"

namespace NOWA
{
	class PhysicsActiveComponent;

	class EXPORTED CameraBehaviorComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<CameraBehaviorComponent> CameraBehaviorCompPtr;
	public:

		CameraBehaviorComponent();

		virtual ~CameraBehaviorComponent();

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
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

		/**
		 * @see		GameObjectComponent::onCloned
		 */
		virtual bool onCloned(void) override;

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
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) = 0;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CameraBehaviorComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CameraBehaviorComponent";
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
			return "Requirements: A kind of player controller component must exist.";
		}

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

		virtual bool isActivated(void) const override;
		
		BaseCamera* getBaseCamera(void) const;

		Ogre::Camera* getCamera(void) const;

		void setCameraControlLocked(bool cameraControlLocked);

		bool getCameraControlLocked(void) const;

		/**
		 * @brief Sets the camera game object id in order if this camera bevhavior shall be used for an other camera, e.g. Splitscreen.
		 * @param[in] cameraGameObjectId The cameraGameObjectId to set.
		 * @note: If 0 (not set), the currently active camera is used.
		 */
		void setCameraGameObjectId(const unsigned long cameraGameObjectId);

		/**
		 * @brief Gets the camera game object id in order if this camera bevhavior shall be used for an other camera, e.g. Splitscreen.
		 * @return cameraGameObjectId The cameraGameObjectId to get
		 */
		unsigned long getCameraGameObjectId(void) const;
	protected:
		void acquireActiveCamera(void);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrCameraGameObjectId(void) { return "Camera GameObject Id"; }
	private:
		void handleRemoveCameraBehavior(EventDataPtr eventData);
	protected:
		Variant* activated;
		Variant* cameraGameObjectId;
		BaseCamera* baseCamera;
		Ogre::Camera* activeCamera;
		Ogre::Vector3 oldPosition;
		Ogre::Quaternion oldOrientation;
		PhysicsActiveComponent* physicsActiveComponent;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CameraBehaviorBaseComponent : public CameraBehaviorComponent
	{
	public:

		typedef boost::shared_ptr<CameraBehaviorBaseComponent> CameraBehaviorBaseCompPtr;
	public:

		CameraBehaviorBaseComponent();

		virtual ~CameraBehaviorBaseComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CameraBehaviorBaseComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CameraBehaviorBaseComponent";
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
			return "Requirements: A kind of player controller component must exist.";
		}

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
		
		void setMoveSpeed(Ogre::Real moveSpeed);

		Ogre::Real getMoveSpeed(void) const;
		
		void setRotationSpeed(Ogre::Real rotationSpeed);

		Ogre::Real getRotationSpeed(void) const;
		
		void setSmoothValue(Ogre::Real smoothValue);

		Ogre::Real getSmoothValue(void) const;
	public:
		static const Ogre::String AttrMoveSpeed(void) { return "Move Speed"; }
		static const Ogre::String AttrRotationSpeed(void) { return "Rotation Speed"; }
		static const Ogre::String AttrSmoothValue(void) { return "Smooth Value"; }
	private:
		Variant* moveSpeed;
		Variant* rotationSpeed;
		Variant* smoothValue;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CameraBehaviorFirstPersonComponent : public CameraBehaviorComponent
	{
	public:

		typedef boost::shared_ptr<CameraBehaviorFirstPersonComponent> CameraBehaviorFirstPersonCompPtr;
	public:

		CameraBehaviorFirstPersonComponent();

		virtual ~CameraBehaviorFirstPersonComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CameraBehaviorFirstPersonComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CameraBehaviorFirstPersonComponent";
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
			return "Requirements: A kind of player controller component must exist.";
		}

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
		
		void setSmoothValue(Ogre::Real smoothValue);

		Ogre::Real getSmoothValue(void) const;
		
		void setRotationSpeed(Ogre::Real rotationSpeed);

		Ogre::Real getRotationSpeed(void) const;
		
		void setOffsetPosition(const Ogre::Vector3& offsetPosition);

		Ogre::Vector3 getOffsetPosition(void) const;
	public:
		static const Ogre::String AttrSmoothValue(void) { return "Smooth Value"; }
		static const Ogre::String AttrRotationSpeed(void) { return "Rotation Speed"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position"; }
	private:
		Variant* smoothValue;
		Variant* rotationSpeed;
		Variant* offsetPosition;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CameraBehaviorThirdPersonComponent : public CameraBehaviorComponent
	{
	public:

		typedef boost::shared_ptr<CameraBehaviorThirdPersonComponent> CameraBehaviorThirdPersonCompPtr;
	public:

		CameraBehaviorThirdPersonComponent();

		virtual ~CameraBehaviorThirdPersonComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CameraBehaviorThirdPersonComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CameraBehaviorThirdPersonComponent";
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
			return "Requirements: A kind of player controller component must exist. Note: If player is not in front of camera, adjust the game object's global mesh direction.";
		}

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
		
		void setYOffset(Ogre::Real yOffset);

		Ogre::Real getYOffset(void) const;

		void setLookAtOffset(const Ogre::Vector3& lookAtOffset);

		const Ogre::Vector3 getLookAtOffset(void) const;
		
		void setSpringForce(Ogre::Real springForce);

		Ogre::Real getSpringForce(void) const;
		
		void setFriction(Ogre::Real friction);

		Ogre::Real getFriction(void) const;
		
		void setSpringLength(Ogre::Real springLength);

		Ogre::Real getSpringLength(void) const;
	public:
		static const Ogre::String AttrYOffset(void) { return "Y-Offset"; }
		static const Ogre::String AttrLookAtOffset(void) { return "Look-At Offset"; }
		static const Ogre::String AttrSpringForce(void) { return "Spring Force"; }
		static const Ogre::String AttrFriction(void) { return "Friction"; }
		static const Ogre::String AttrSpringLength(void) { return "Spring Length"; }
	private:
		Variant* yOffset;
		Variant* springForce;
		Variant* friction;
		Variant* springLength;
		Variant* lookAtOffset;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CameraBehaviorFollow2DComponent : public CameraBehaviorComponent
	{
	public:

		typedef boost::shared_ptr<CameraBehaviorFollow2DComponent> CameraBehaviorFollow2DCompPtr;
	public:

		CameraBehaviorFollow2DComponent();

		virtual ~CameraBehaviorFollow2DComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CameraBehaviorFollow2DComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CameraBehaviorFollow2DComponent";
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
			return "Requirements: A kind of player controller component must exist.";
		}

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
		
		void setSmoothValue(Ogre::Real smoothValue);

		Ogre::Real getSmoothValue(void) const;
		
		void setOffsetPosition(const Ogre::Vector3& offsetPosition);

		Ogre::Vector3 getOffsetPosition(void) const;

		void setBorderOffset(const Ogre::Vector3& borderOffset);

		Ogre::Vector3 getBorderOffset(void) const;
	public:
		static const Ogre::String AttrSmoothValue(void) { return "Smooth Value"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position"; }
		static const Ogre::String AttrBorderOffset(void) { return "Border Offset"; }
	private:
		Variant* smoothValue;
		Variant* offsetPosition;
		Variant* borderOffset;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CameraBehaviorZoomComponent : public CameraBehaviorComponent
	{
	public:

		typedef boost::shared_ptr<CameraBehaviorZoomComponent> CameraBehaviorZoomCompPtr;
	public:

		CameraBehaviorZoomComponent();

		virtual ~CameraBehaviorZoomComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CameraBehaviorZoomComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CameraBehaviorZoomComponent";
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
			return "Zoom a camera for all game objects of the given category. Note: Camera position and orientation is important! Else scene could be clipped away. A good orientation value for the camera is: -80 -60 0. Requirements: Camera must be in orthogonal mode.";
		}

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
		
		void setCategory(const Ogre::String& category);
		
		Ogre::String getCategory(void) const;
		
		void setSmoothValue(Ogre::Real smoothValue);

		Ogre::Real getSmoothValue(void) const;

		void setGrowMultiplicator(Ogre::Real growMultiplicator);

		Ogre::Real getGrowMultiplicator(void) const;
		
	public:
		static const Ogre::String AttrCategory(void) { return "Category"; }
		static const Ogre::String AttrSmoothValue(void) { return "Smooth Value"; }
		static const Ogre::String AttrGrowMultiplicator(void) { return "Grow Multiplicator"; }
	private:
		Variant* category;
		Variant* smoothValue;
		Variant* growMultiplicator;
	};

}; //namespace end

#endif