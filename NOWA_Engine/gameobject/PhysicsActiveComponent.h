#ifndef PHYSICS_ACTIVE_COMPONENT_H
#define PHYSICS_ACTIVE_COMPONENT_H

#include "PhysicsComponent.h"
#include "Vehicle.h"
#include "main/Events.h"
#include <unordered_map>

namespace NOWA
{
	class IPicker;
	class LuaScript;

	class EXPORTED PhysicsActiveComponent : public PhysicsComponent
	{
	public:
		typedef boost::shared_ptr<PhysicsActiveComponent> PhysicsActiveCompPtr;
	public:
	
		/**
		* @class ContactData
		* @brief This class holds the contact data after a physics ray has been shot
		*/
		class EXPORTED ContactData
		{
		public:
			ContactData()
				: hitGameObject(nullptr),
				height(500.0f), // 500 is set to be invalid
				normal(Ogre::Vector3::UNIT_SCALE * 100.0f) // 100, 100, 100 is set to be invalid
			{

			}
			
			ContactData(GameObject* hitGameObject, Ogre::Real height, const Ogre::Vector3& normal, Ogre::Real slope)
				: hitGameObject(hitGameObject),
				height(height),
				normal(normal),
				slope(slope)
			{

			}

			~ContactData()
			{
			}
			
			GameObject* getHitGameObject(void) const
			{
				return this->hitGameObject;
			}
			
			Ogre::Real getHeight(void) const
			{
				return this->height;
			}
			
			Ogre::Vector3 getNormal(void) const
			{
				return this->normal;
			}

			Ogre::Real getSlope(void) const
			{
				return this->slope;
			}
		private:
			GameObject* hitGameObject;
			Ogre::Real height;
			Ogre::Vector3 normal;
			Ogre::Real slope;
		};

		/**
		* @class IForceObserver
		* @brief This interface can be implemented to react when an external object wants to add its force in the force and torque callback of this component
		*/
		class EXPORTED IForceObserver
		{
		public:
			IForceObserver(IPicker* picker)
				: picker(picker)
			{

			}

			virtual ~IForceObserver()
			{
			}

			void setName(const Ogre::String& name)
			{
				this->name = name;
			}

			const Ogre::String& getName(void)
			{
				return this->name;
			}

			/**
			* @brief		Called in the force and torque callback of the physics active component
			* @param[in]	body		The body to add the force
			* @param[in]	timeStep	The time since last force calculation
			* @param[in]	threadIndex	The thread id
			*/
			virtual void onForceAdd(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex) = 0;
		protected:
			IPicker* picker;
		private:
			Ogre::String name;
		};

		PhysicsActiveComponent();

		virtual ~PhysicsActiveComponent();

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
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool isMovable(void) const override
		{
			return true;
		}

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsActiveComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsActiveComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see  GameObjectComponent::canStaticAddComponent
		 */
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: This Component is used for physics motion. Forces to act on this component. Hence velocity of any kind must only be used for initial settings!" 
				"Not during simulation! If a component with just velocity with no forces, that can act on that component, is required, the PhysicsActiveKinematicComponent is the correct one!"; ;
		}

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::showDebugData
		*/
		virtual void showDebugData(void) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		void setLinearDamping(Ogre::Real linearDamping);

		Ogre::Real getLinearDamping(void) const;

		void setAngularDamping(const Ogre::Vector3& angularDamping);

		const Ogre::Vector3 getAngularDamping(void) const;

		virtual void setSpeed(Ogre::Real speed);

		Ogre::Real getSpeed(void) const;

		void setMaxSpeed(Ogre::Real maxSpeed);

		Ogre::Real getMaxSpeed(void) const;

		void setMinSpeed(Ogre::Real minSpeed);

		Ogre::Real getMinSpeed(void) const;

		/**
		 * @brief		Sets the omega velocity
		 * @param[in]	omegaVelocity The omega velocity vector to apply
		 * @Note		This should only be set for initialization and not during simulation, as It could break physics calculation. Use @applyAngularVelocity instead
		 *				Or it may be called if its a physics active kinematic body.
		 */
		virtual void setOmegaVelocity(const Ogre::Vector3& omegaVelocity);

		/**
		 * @brief		Applies the omega force in move callback function
		 * @param[in]	omegaForce The omega force vector to apply
		 * @Note		This should be used during simulation instead of @setOmegaVelocity.
		 */
		virtual void applyOmegaForce(const Ogre::Vector3& omegaForce);

		/**
		 * @brief		Applies the omega force in move callback function in order to rotate the game object to the given orientation.
		 * @param[in]	resultOrientation The result orientation to which the game object should be rotated via omega to.
		 * @param[in]	axes The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.)
		 * @param[in]	strength The strength at which the rotation should occur
		 * @Note		This should be used during simulation instead of @setOmegaVelocity.
		 */
		virtual void applyOmegaForceRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength = 10.0f);
		
		Ogre::Vector3 getOmegaVelocity(void) const;

		void setGyroscopicTorqueEnabled(bool enable);

		bool getGyroscopicTorqueEnabled(void) const;

		void resetForce(void);

		/**
		 * @brief	Applies force for the given body.
		 * @param[in]	force The force vector to apply
		 */
		virtual void applyForce(const Ogre::Vector3& force);

		/**
		 * @brief	Applies the required force in order move by the given velocity.
		 * @param[in]	velocity The velocity vector
		 */
		virtual void applyRequiredForceForVelocity(const Ogre::Vector3& velocity);

		virtual Ogre::Vector3 getForce(void) const;

		virtual void setGravity(const Ogre::Vector3& gravity);

		virtual const Ogre::Vector3 getGravity(void) const;

		virtual void addImpulse(const Ogre::Vector3& deltaVector);

		/**
		 * @brief	Sets the velocity.
		 * @param[in]	velocity The velocity vector to apply
		 * @Note		This should only be set for initialization and not during simulation, as It could break physics calculation. Use @applyRequiredForceForVelocity instead.
		 *				Or it may be called if its a physics active kinematic body.
		 */
		virtual void setVelocity(const Ogre::Vector3& velocity);

		/**
		 * @brief	Sets the velocity speed for the current direction.
		 * @param[in]	speed The speed for the current direction to apply.
		 * @Note		The default direction axis is applied. See: @GameObject::getDefaultDirection(). This should only be set for initialization and not during simulation, as It could break physics calculation, or it may be called if its a physics active kinematic body.
		 */
		virtual void setDirectionVelocity(Ogre::Real speed);

		/**
		 * @brief	Applies the force speed for the current direction.
		 * @param[in]	speed The speed for the current direction to apply.
		 * @Note		The default direction axis is applied. See: @GameObject::getDefaultDirection(). This should be used for any kind of physics active component, not for kinematic components."
		 */
		virtual void applyDirectionForce(Ogre::Real speed);

		virtual Ogre::Vector3 getVelocity(void) const;

		virtual void setMass(Ogre::Real mass) override;

		Ogre::Real getMass(void) const;

		void setMassOrigin(const Ogre::Vector3& massOrigin);

		const Ogre::Vector3 getMassOrigin(void) const;

		void setCollisionDirection(const Ogre::Vector3& collisionPosition);

		const Ogre::Vector3 getCollisionDirection(void) const;

		virtual void setCollisionPosition(const Ogre::Vector3& collisionPosition);

		const Ogre::Vector3 getCollisionPosition(void) const;

		void setCollisionSize(const Ogre::Vector3& collisionSize);

		const Ogre::Vector3 getCollisionSize(void) const;

		const Ogre::Vector3 getConstraintAxis(void) const;

		void releaseConstraintAxis(void);

		void releaseConstraintAxisPin(void);

	    void setConstraintDirection(const Ogre::Vector3& constraintDirection);

		void setConstraintAxis(const Ogre::Vector3& constraintAxis);

		const Ogre::Vector3 getConstraintDirection(void) const;

		void releaseConstraintDirection(void);

		void setAsSoftBody(bool asSoftBody);

		bool getIsSoftBody(void) const;

		void setGravitySourceCategory(const Ogre::String& gravitySourceCategory);

		Ogre::String getGravitySourceCategory(void) const;

		/*void setDefaultPoseName(const Ogre::String& defaultPoseName);

		const Ogre::String getDefaultPoseName(void) const;*/
		
		void setContinuousCollision(bool continuousCollision);
		
		bool getContinuousCollision(void) const;

		/**
		 * @brief	Clamps the omega of a body by the given value, in order to prevent high peaks.
		 * @param	clampValue The clamp value to set, 100 is a good value. Setting a value of 0, will disable clamping.
		 * @note	This is usefull e.g. for ragdoll bones, so that the bone will not rotate like crazy.
		 */
		void setClampOmega(Ogre::Real clampValue);

		// can be overwritten
		virtual void moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex);

		virtual void contactCallback(OgreNewt::Body* otherBody);

		void updateCallback(OgreNewt::Body* body);

		virtual void reCreateCollision(void) override;

		// Query scene what is ahead of an object
		ContactData getContactAhead(int index, const Ogre::Vector3& offset = Ogre::Vector3(0.0f, 0.4f, 0.0f), Ogre::Real length = 1.0f, 
			bool forceDrawLine = false, unsigned int categoryIds = 0xFFFFFFFF);

		// Query scene to a given direction
		ContactData getContactToDirection(int index, const Ogre::Vector3& direction, const Ogre::Vector3& offset = Ogre::Vector3(0.0f, 0.4f, 0.0f), Ogre::Real from = 0.0f, Ogre::Real to = 1.0f, 
			bool forceDrawLine = false, unsigned int categoryIds = 0xFFFFFFFF);

		GameObject* getContact(int index, const Ogre::Vector3& direction = Ogre::Vector3::NEGATIVE_UNIT_Z, const Ogre::Vector3& offset = Ogre::Vector3(0.0f, 0.4f, 0.0f),
			Ogre::Real from = 0.0f, Ogre::Real to = 1.0f, bool drawLine = false, unsigned int categoryIds = 0xFFFFFFFF);

		bool getFixedContactToDirection(int index, const Ogre::Vector3& direction, const Ogre::Vector3& offset, Ogre::Real scale, 
			unsigned int categoryIds = 0xFFFFFFFF);

		ContactData getContactBelow(int index, const Ogre::Vector3& positionOffset1, bool forceDrawLine = false, 
			unsigned int categoryIds = 0xFFFFFFFF);

		Ogre::Real determineGameObjectHeight(const Ogre::Vector3& positionOffset1, const Ogre::Vector3& positionOffset2, 
			unsigned int categoryIds = 0xFFFFFFFF);

		void setBounds(const Ogre::Vector3& minBounds, const Ogre::Vector3& maxBounds);

		void removeBounds(void);

		void createCompoundBody(const std::vector<PhysicsActiveComponent*>& physicsComponentList);

		void destroyCompoundBody(const std::vector<PhysicsActiveComponent*>& physicsComponentList);

		bool getHasAttraction(void) const;

		bool getHasSpring(void) const;

		Ogre::Vector3 getCurrentForceForVelocity(void) const;

		/**
		* @brief		Attaches a force observer to react at the moment when a game object has been picked or released.
		* @param[in]	pickObserver	The pick observer to attach
		* @note		The pick observer must be created on heap, but will be destroy either by calling @detachAndDestroyPickObserver or @detachAndDestroyAllPickObserver or will be destroyed
		*				at last automatically.
		*/
		void attachForceObserver(IForceObserver* forceObserver);

		/**
		* @brief		Detaches and destroys a pick observer
		* @param[in]	name	The pick observer name to detach and destroy
		*/
		void detachAndDestroyForceObserver(const Ogre::String& name);

		/**
		* @brief		Enables contact solving, so that in a lua script onContactSolving(PhysicsComponent otherPhysicsComponent) will be called, when two bodies are colliding.
		* @param[in]	enable	If set to true, contact solving callback will be enabled.
		*/
		void setContactSolvingEnabled(bool enable);

		/**
		* @brief		Detaches and destroys all force observer
		*/
		void detachAndDestroyAllForceObserver(void);

		void addJointSpringComponent(unsigned long id);

		void removeJointSpringComponent(unsigned long id);

		void addJointAttractorComponent(unsigned long id);

		void removeJointAttractorComponent(unsigned long id);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrForce(void) { return "Force"; }
		static const Ogre::String AttrGravity(void) { return "Gravity"; }
		static const Ogre::String AttrGravitySourceCategory(void) { return "Gravity Source Category"; }
		static const Ogre::String AttrMassOrigin(void) { return "Mass Origin"; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
		static const Ogre::String AttrMinSpeed(void) { return "Min Speed"; }
		static const Ogre::String AttrMaxSpeed(void) { return "Max Speed"; }
		static const Ogre::String AttrSleep(void) { return "Sleep"; }
		static const Ogre::String AttrContinuousCollision(void) { return "Continuous Collision"; }
		static const Ogre::String AttrLinearDamping(void) { return "Linear Damping"; }
		static const Ogre::String AttrAngularDamping(void) { return "Angular Damping"; }
		static const Ogre::String AttrCollisionSize(void) { return "Collision Size"; }
		static const Ogre::String AttrCollisionPosition(void) { return "Collision Pos"; }
		static const Ogre::String AttrCollisionDirection(void) { return "Collision Dir"; }
		static const Ogre::String AttrConstraintDirection(void) { return "Constraint Dir"; }
		static const Ogre::String AttrConstraintAxis(void) { return "Constraint Axis"; }
		static const Ogre::String AttrAsSoftBody(void) { return "As Softbody"; }
		static const Ogre::String AttrEnableGyroscopicTorque(void) { return "Enable Gyroscopic Torque"; }
		
	protected:
		void parseCommonProperties(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename);

		void writeCommonProperties(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc);

		void actualizeCommonValue(Variant* attribute);

		virtual bool createDynamicBody(void);

		void createSoftBody(void);
	private:
		void destroyLineMap(void);
	protected:
		Ogre::Real height;
		Ogre::Real rise;
		Ogre::Real clampedOmega;
		Ogre::Vector3 omegaForce;
		bool canAddOmegaForce;
		Ogre::Vector3 forceForVelocity;
		bool canAddForceForVelocity;
		bool bResetForce;
		Variant* activated;
		Variant* force;
		Variant* gravity;
		Variant* gravitySourceCategory;
		Variant* massOrigin;
		Variant* speed;
		Variant* minSpeed;
		Variant* maxSpeed;
		Variant* collisionSize;
		Variant* collisionPosition;
		Variant* collisionDirection;
		OgreNewt::UpVector* upVector;
		OgreNewt::PlaneConstraint* planeConstraint;
		Variant* constraintDirection;
		Variant* continuousCollision;
		Variant* linearDamping;
		Variant* angularDamping;
		Variant* constraintAxis;
		Variant* gyroscopicTorque;

		Variant* asSoftBody;
		// Ogre::String defaultPoseName;

		bool hasAttraction;
		std::vector<unsigned long> physicsAttractors;

		std::vector<unsigned long> springs;
		bool hasSpring;

		std::unordered_map<Ogre::String, IForceObserver*> forceObservers;
		std::unordered_map<Ogre::String, std::pair<Ogre::SceneNode*, Ogre::ManualObject*>> drawLineMap;

		double lastTime;
		Ogre::Real dt;
		Ogre::Timer timer;
		bool isInSimulation;
		bool usesBounds;
		Ogre::Vector3 minBounds;
		Ogre::Vector3 maxBounds;
	};

}; //namespace end

#endif