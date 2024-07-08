#ifndef PHYSICS_ACTIVE_VEHICLE_COMPONENT_H
#define PHYSICS_ACTIVE_VEHICLE_COMPONENT_H

#include "PhysicsActiveComponent.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Vehicle.h"

namespace NOWA
{
	class VehicleDrivingManipulation;

	class EXPORTED PhysicsActiveVehicleComponent : public PhysicsActiveComponent
	{
	public:
		typedef boost::shared_ptr<PhysicsActiveVehicleComponent> PhysicsActiveVehicleCompPtr;
	public:
		class EXPORTED PhysicsVehicleCallback : public OgreNewt::VehicleCallback
		{
		public:
			PhysicsVehicleCallback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onSteerAngleChangedFunctionName,
								   const Ogre::String& onMotorForceChangedFunctionName, const Ogre::String& onHandBrakeChangedFunctionName, 
								   const Ogre::String& onBrakeChangedFunctionName, const Ogre::String& onTireContactFunctionName);

			virtual ~PhysicsVehicleCallback();

			/**
			 * @brief Controls via device input the steer angle.
			 */
			virtual Ogre::Real onSteerAngleChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real dt) override;

			/**
			 * @brief Controls via device input the motor force.
			 */
			virtual Ogre::Real onMotorForceChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real dt) override;

			/**
			 * @brief Controls via device input the hand brake force.
			 * @note  A good value is 5.5.
			 */
			virtual Ogre::Real onHandBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real dt) override;

			/**
			 * @brief Controls via device input the brake force.
			 * @note  A good value is 7.5.
			 */
			virtual Ogre::Real onBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real dt) override;

			/**
			 * @brief Is called if the tire hits another game object below.
			 */
			virtual void onTireContact(const OgreNewt::RayCastTire* tire, const Ogre::String& tireName, OgreNewt::Body* hitBody, const Ogre::Vector3& contactPosition, const Ogre::Vector3& contactNormal, Ogre::Real penetration) override;
		private:
			GameObject* owner;
			LuaScript* luaScript;
			OgreNewt::World* ogreNewt;
			Ogre::String onSteerAngleChangedFunctionName;
			Ogre::String onMotorForceChangedFunctionName;
			Ogre::String onHandBrakeChangedFunctionName;
			Ogre::String onBrakeChangedFunctionName;
			Ogre::String onTireContactFunctionName;
			VehicleDrivingManipulation* vehicleDrivingManipulation;
		};
	public:

		PhysicsActiveVehicleComponent();

		virtual ~PhysicsActiveVehicleComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

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
			return NOWA::getIdFromName("PhysicsActiveVehicleComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsActiveVehicleComponent";
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
			return "Usage: This component can be used in conjunction with JointVehicleTire component's, builing a vehicle like a car. Note: The mesh's default model axis should be in x-direction. Else the car will not drive properly. "
				" Use the Mesh Tool to rotate the mesh's origin axis properly.";
		}

		/**
		 * @brief Sets the lua function name, to react when the steering angle for the specific tires shall change.
		 * @param[in]	onEnterFunctionName		The function name to set
		 */
		void setOnSteerAngleChangedFunctionName(const Ogre::String& onSteerAngleChangedFunctionName);

		/**
		 * @brief Gets the lua function name.
		 * @return lua function name to get
		 */
		Ogre::String getOnSteerAngleChangedFunctionName(void) const;

		/**
		 * @brief Sets the lua function name, to react when the motor force for the specific tires shall change.
		 * @param[in]	onEnterFunctionName		The function name to set
		 */
		void setOnMotorForceChangedFunctionName(const Ogre::String& onMotorForceChangedFunctionName);

		/**
		 * @brief Gets the lua function name.
		 * @return lua function name to get
		 */
		Ogre::String getOnMotorForceChangedFunctionName(void) const;

		/**
		 * @brief Sets the lua function name, to react when the hand brake force shall change.
		 * @param[in]	onEnterFunctionName		The function name to set
		 */
		void setOnHandBrakeChangedFunctionName(const Ogre::String& onHandBrakeChangedFunctionName);

		/**
		 * @brief Gets the lua function name.
		 * @return lua function name to get
		 */
		Ogre::String getOnHandBrakeChangedFunctionName(void) const;

		/**
		 * @brief Sets the lua function name, to react when the brake force shall change.
		 * @param[in]	onEnterFunctionName		The function name to set
		 */
		void setOnBrakeChangedFunctionName(const Ogre::String& onBrakeChangedFunctionName);

		/**
		 * @brief Gets the lua function name.
		 * @return lua function name to get
		 */
		Ogre::String getOnBrakeChangedFunctionName(void) const;


		/**
		 * @brief Sets the lua function name, to react when the brake force shall change.
		 * @param[in]	onEnterFunctionName		The function name to set
		 */
		void setOnTireContactFunctionName(const Ogre::String& onTireContactFunctionName);

		/**
		 * @brief Gets the lua function name.
		 * @return lua function name to get
		 */
		Ogre::String getOnTireContactFunctionName(void) const;
		
		OgreNewt::Vehicle* getVehicle(void) const;

		Ogre::Vector3 getVehicleForce(void) const;
	public:
		static const Ogre::String AttrOnSteerAngleChangedFunctionName(void) { return "On Steering Angle Function Name"; }
		static const Ogre::String AttrOnMotorForceChangedFunctionName(void) { return "On Motor Force Function Name"; }
		static const Ogre::String AttrOnHandBrakeChangedFunctionName(void) { return "On Hand Brake Function Name"; }
		static const Ogre::String AttrOnBrakeChangedFunctionName(void) { return "On Brake Function Name"; }
		static const Ogre::String AttrOnTireContactFunctionName(void) { return "On Tire Function Name"; }
	protected:
		virtual bool createDynamicBody(void);
	private:
		bool isVehicleTippedOver(void);
		bool isVehicleStuck(Ogre::Real dt);
		void correctVehicleOrientation(void);
	private:
		Variant* onSteerAngleChangedFunctionName;
		Variant* onMotorForceChangedFunctionName;
		Variant* onHandBrakeChangedFunctionName;
		Variant* onBrakeChangedFunctionName;
		Variant* onTireContactFunctionName;

		Ogre::Real stuckTime;
		Ogre::Real maxStuckTime;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED VehicleDrivingManipulation
	{
	public:
		friend class PhysicsActiveVehicleComponent::PhysicsVehicleCallback;

	public:
		VehicleDrivingManipulation();

		~VehicleDrivingManipulation();

		void setSteerAngle(Ogre::Real steerAngle);

		Ogre::Real getSteerAngle(void) const;

		void setMotorForce(Ogre::Real motorForce);

		Ogre::Real getMotorForce(void) const;

		void setHandBrake(Ogre::Real handBrake);

		Ogre::Real getHandBrake(void) const;

		void setBrake(Ogre::Real brake);

		Ogre::Real getBrake(void) const;

	private:
		Ogre::Real steerAngle;
		Ogre::Real motorForce;
		Ogre::Real handBrake;
		Ogre::Real brake;
	};

}; //namespace end

#endif