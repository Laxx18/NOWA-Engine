#ifndef PHYSICS_ACTIVE_KINEMATIC_COMPONENT_H
#define PHYSICS_ACTIVE_KINEMATIC_COMPONENT_H

#include "PhysicsActiveComponent.h"
#include "OgreNewt_World.h"
#include "OgreNewt_KinematicBody.h"

namespace NOWA
{
	class EXPORTED PhysicsActiveKinematicComponent : public PhysicsActiveComponent
	{
	public:
		typedef boost::shared_ptr<PhysicsActiveKinematicComponent> PhysicsActiveKinematicCompPtr;
	public:

		PhysicsActiveKinematicComponent();

		virtual ~PhysicsActiveKinematicComponent();

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
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentParentClassName
		*/
		virtual Ogre::String getParentParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::isMovable
		*/
		virtual bool isMovable(void) const override
		{
			return true;
		}

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsActiveKinematicComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsActiveKinematicComponent";
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
			return "Usage: This Component acts like a physics active component but its not influenced by any forces! Hence calling methods with forces are useless. "
				"Use PhysicsActiveComponent, if forces influence is desired. Note: Besides the JointKinematicComponent, its not possible to use joints for this component. "
				"It may also not be involved in joints, because else the application would crash, because newton in version 3 does not allow this combination."
				"Note: Two game objects with kinematic bodies cannot collide on each other. The physics engine prevents this. So its only possible that a kinematic component does collide with another kind of physics active/artifact component."
				"If still desired, @PhysicsPlayerControllerComponent's can be used for collisions.";
		}

		/**
		 * @brief		Sets the omega velocity in order to rotate the game object to the given result orientation.
		 * @param[in]	resultOrientation	The result orientation to which the game object should be rotated via omega to.
		 * @param[in]	axes				The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.)
		 * @param[in]	strength			The strength at which the rotation should occur
		 */
		void setOmegaVelocityRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength = 1.0f);
	protected:
		virtual bool createDynamicBody(void);
	};

}; //namespace end

#endif