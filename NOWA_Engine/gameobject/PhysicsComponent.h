#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

#include "GameObjectComponent.h"
#include "modules/LuaScript.h"

class Terra;

namespace NOWA
{
	/**
	 *	@brief	Basic physics component class.
	 *	@info	The following collision types are possible: http://newtondynamics.com/wiki/index.php5?title=Collision_primitives
	 */

	class EXPORTED PhysicsComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<PhysicsComponent> PhysicsCompPtr;
	public:
	
		PhysicsComponent();

		virtual ~PhysicsComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) = 0;

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
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const = 0;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const = 0;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) = 0;

		virtual bool isMovable(void) const = 0;

		/**
		* @see		GameObjectComponent::showDebugData
		*/
		virtual void showDebugData(void) = 0;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsComponent";
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
			return "Usage: This Component is the base physics component.";
		}

		void setBody(OgreNewt::Body* body);

		// virtual bool isTagged(GameObjectPtr otherGameObjectPtr) { return false; }

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) = 0;

		/**
		 * @brief		Sets the orientation for this game object
		 * @param[in]	orientation	The orientation to set
		 * @note		Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!
		 */
		virtual void setOrientation(const Ogre::Quaternion& orientation);

		virtual Ogre::Quaternion getOrientation(void) const;

		/**
		 * @brief		Sets the direction for this game object (internally object will be orientated correctly from direction vector)
		 * @param[in]	direction	The dirction to set
		 * @param[in]	localDirectionVector The vector which normally describes the natural direction of the node, usually -Z
		 * @note		Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!
		 */
		virtual void setDirection(const Ogre::Vector3& direction, const Ogre::Vector3& localDirectionVector = Ogre::Vector3::NEGATIVE_UNIT_Z);

		/**
		 * @brief		Sets the position for this game object
		 * @param[in]	x	The x-value to set
		 * @param[in]	y	The y-value to set
		 * @param[in]	z	The z-value to set
		 * @note		Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!
		 */
		virtual void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z);

		/**
		 * @brief		Sets the position for this game object
		 * @param[in]	position	The position to set
		 * @note		Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!
		 */
		virtual void setPosition(const Ogre::Vector3& position);

		/**
		 * @brief		Translates the game object (relative positioning)
		 * @param[in]	relativePosition	The relative position to set
		 * @note		Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!
		 */
		virtual void translate(const Ogre::Vector3& relativePosition);

		/**
		 * @brief		Rotates the game object (relative orientation)
		 * @param[in]	relativeRotation	The relative rotation to set
		 * @note		Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!
		 */
		virtual void rotate(const Ogre::Quaternion& relativeRotation);

		virtual Ogre::Vector3 getPosition(void) const;

		virtual void setScale(const Ogre::Vector3& scale);

		Ogre::Vector3 getScale(void) const;

		virtual OgreNewt::Body* getBody(void) const;

		virtual void reCreateCollision(bool overwrite = false) = 0;

		void destroyCollision(void);

		void destroyBody(void);

		OgreNewt::World* getOgreNewt(void) const;

		virtual void setMass(Ogre::Real mass);

		Ogre::Real getMass(void) const;

		void setCollisionType(const Ogre::String& collisionType);

		const Ogre::String getCollisionType(void) const;

		// std::list<GameObjectPtr>* getTaggedList(void) const;

		const Ogre::Vector3 getInitialPosition(void) const;
		
		const Ogre::Vector3 getInitialScale(void) const;

		const Ogre::Quaternion getInitialOrientation(void) const;

		void setVolume(Ogre::Real volume);

		Ogre::Real getVolume(void) const;

		void localToGlobal(const Ogre::Quaternion& localOrient, const Ogre::Vector3& localPos, Ogre::Quaternion& globalOrient, Ogre::Vector3& globalPos);

		void globalToLocal(const Ogre::Quaternion& globalOrient, const Ogre::Vector3& globalPos, Ogre::Quaternion& localOrient, Ogre::Vector3& localPos);

		void clampAngularVelocity(Ogre::Real clampValue);

		void setCollidable(bool collidable);

		bool getCollidable(void) const;

		Ogre::Vector3 getLastPosition(void) const;

		Ogre::Quaternion getLastOrientation(void) const;
	public:
		static const Ogre::String AttrCollisionType(void) { return "Collision Type"; }
		static const Ogre::String AttrMass(void) { return "Mass"; }
		static const Ogre::String AttrCollidable(void) { return "Collidable"; }
	protected:
		OgreNewt::CollisionPtr serializeTreeCollision(const Ogre::String& scenePath, unsigned int categoryId, bool overwrite = false);

		OgreNewt::CollisionPtr serializeHeightFieldCollision(const Ogre::String& scenePath, unsigned int categoryId, Ogre::Terra* terra, bool overwrite = false);

		OgreNewt::CollisionPtr createDynamicCollision(Ogre::Vector3& inertia, const Ogre::Vector3& collisionSize, const Ogre::Vector3& collisionPosition,
			const Ogre::Quaternion& collisionOrientation, Ogre::Vector3& massOrigin, unsigned int categoryId);
		
		OgreNewt::CollisionPtr createDynamicCollision(Ogre::Vector3& inertia, const Ogre::Vector3& collisionPosition, 
			const Ogre::Quaternion& collisionOrientation, Ogre::Vector3& massOrigin, unsigned int categoryId);

		OgreNewt::CollisionPtr createCollisionPrimitive(const Ogre::String& collisionType, const Ogre::Vector3& collisionPosition,
			const Ogre::Quaternion& collisionOrientation, const Ogre::Vector3& collisionSize, Ogre::Vector3& inertia, Ogre::Vector3& massOrigin, unsigned int categoryId);

		OgreNewt::CollisionPtr createDeformableCollision(OgreNewt::CollisionPtr collisionPtr);

		OgreNewt::CollisionPtr getWeightedBoneConvexHull(Ogre::v1::OldBone* bone, Ogre::v1::MeshPtr mesh, Ogre::Real minWeight,
			Ogre::Vector3& inertia, Ogre::Vector3& massOrigin, unsigned int categoryId, const Ogre::Vector3& offsetPosition, const Ogre::Quaternion& offsetOrientation,
			const Ogre::Vector3& scale = Ogre::Vector3(1.0f, 1.0f, 1.0f));

		OgreNewt::CollisionPtr createHeightFieldCollision(Ogre::Terra* terra);
	protected:
		OgreNewt::World* ogreNewt;
		OgreNewt::Body* physicsBody;
		Variant* collisionType;
		Variant* mass;
		Variant* collidable;
		Ogre::Vector3 initialPosition;
		Ogre::Vector3 initialScale;
		Ogre::Quaternion initialOrientation;
		Ogre::Real volume;
		OgreNewt::CollisionPtr collisionPtr;
	};

}; //namespace end

#endif