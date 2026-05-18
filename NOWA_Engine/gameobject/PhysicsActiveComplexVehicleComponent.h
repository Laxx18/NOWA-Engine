#ifndef PHYSICS_ACTIVE_COMPLEX_VEHICLE_COMPONENT_H
#define PHYSICS_ACTIVE_COMPLEX_VEHICLE_COMPONENT_H

#include "PhysicsActiveComponent.h"
#include "OgreNewt_World.h"
#include "OgreNewt_ComplexVehicle.h"

namespace NOWA
{
    class ComplexVehicleDrivingManipulation;

    class EXPORTED PhysicsActiveComplexVehicleComponent : public PhysicsActiveComponent
    {
    public:
        typedef boost::shared_ptr<PhysicsActiveComplexVehicleComponent> PhysicsActiveComplexVehicleCompPtr;

    public:
        PhysicsActiveComplexVehicleComponent(void);

        virtual ~PhysicsActiveComplexVehicleComponent(void) override;

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        virtual bool postInit(void) override;

        /**
        * @see  GameObjectComponent::onRemoveComponent
        */
        virtual void onRemoveComponent(void) override;

        /**
        * @see		GameObjectComponent::connect
        */
        virtual bool connect(void) override;

        /**
        * @see		GameObjectComponent::disconnect
        */
        virtual bool disconnect(void) override;

        virtual Ogre::String getClassName(void) const override;

        virtual Ogre::String getParentClassName(void) const override;

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PhysicsActiveComplexVehicleComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PhysicsActiveComplexVehicleComponent";
        }

        /**
        * @see  GameObjectComponent::createStaticApiForLua
        */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) {}

        /**
        * @see GameObjectComponent::getStaticInfoText
        */
        static Ogre::String getStaticInfoText(void)
        {
            return "Info: Active complex physics vehicle component using ComplexVehicle model. "
                "Requirements: Dynamic GameObject with a rigid body and multiple JointComplexVehicleTireComponents as children.";
        }

        // Access the underlying ComplexVehicle (for tire joints etc.)
        OgreNewt::ComplexVehicle* getComplexVehicle(void) const;

        Ogre::Vector3 getVehicleForce(void) const;

        bool getCanDrive(void) const;

        void setCanDrive(bool canDrive);

        void applyWheelie(Ogre::Real strength);

        void applyDrift(bool left, Ogre::Real strength, Ogre::Real steeringStrength);

    public:
        // Reuse same attribute names as PhysicsActiveVehicleComponent for Lua callback function names
        static const Ogre::String AttrOnSteerAngleChangedFunctionName(void) { return "On Steering Angle Function Name"; }
        static const Ogre::String AttrOnMotorForceChangedFunctionName(void) { return "On Motor Force Function Name"; }
        static const Ogre::String AttrOnHandBrakeChangedFunctionName(void) { return "On Hand Brake Function Name"; }
        static const Ogre::String AttrOnBrakeChangedFunctionName(void) { return "On Brake Function Name"; }
        static const Ogre::String AttrOnTireContactFunctionName(void) { return "On Tire Function Name"; }

    protected:
        // Creates rigid body + complex vehicle
        virtual bool createDynamicBody(void) override;

    private:
        bool isVehicleTippedOver(void);
        bool isVehicleStuck(Ogre::Real dt);
        void correctVehicleOrientation(void);

    private:
        // Lua callback function names (same pattern as simple vehicle)
        Variant* onSteerAngleChangedFunctionName;
        Variant* onMotorForceChangedFunctionName;
        Variant* onHandBrakeChangedFunctionName;
        Variant* onBrakeChangedFunctionName;
        Variant* onTireContactFunctionName;

        OgreNewt::ComplexVehicle* complexVehicle;
        OgreNewt::ComplexVehicleCallback* vehicleCallback;

        // Cached force vector from last update, for UI / gameplay
        Ogre::Vector3 vehicleForce;

        bool canDrive;
        Ogre::Real stuckTime;
        Ogre::Real maxStuckTime;

    public:
        // A callback implementation very similar to PhysicsActiveVehicleComponent::PhysicsVehicleCallback,
        // but we keep it local to the complex vehicle
        class PhysicsComplexVehicleCallback : public OgreNewt::ComplexVehicleCallback
        {
        public:
            PhysicsComplexVehicleCallback(PhysicsActiveComplexVehicleComponent* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt,
                const Ogre::String& onSteerAngleChangedFunctionName,
                const Ogre::String& onMotorForceChangedFunctionName,
                const Ogre::String& onHandBrakeChangedFunctionName,
                const Ogre::String& onBrakeChangedFunctionName,
                const Ogre::String& onTireContactFunctionName);

            virtual ~PhysicsComplexVehicleCallback();

            virtual Ogre::Real onSteerAngleChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep) override;

            virtual Ogre::Real onMotorForceChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep) override;

            virtual Ogre::Real onHandBrakeChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep) override;

            virtual Ogre::Real onBrakeChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep) override;

            virtual void onTireContact(const OgreNewt::ComplexVehicleTire* tire, const Ogre::String& tireName, OgreNewt::Body* hitBody,
                const Ogre::Vector3& contactPosition, const Ogre::Vector3& contactNormal, Ogre::Real penetration) override;

        private:
            PhysicsActiveComplexVehicleComponent* owner;
            LuaScript* luaScript;
            OgreNewt::World* ogreNewt;
            Ogre::String onSteerAngleChangedFunctionName;
            Ogre::String onMotorForceChangedFunctionName;
            Ogre::String onHandBrakeChangedFunctionName;
            Ogre::String onBrakeChangedFunctionName;
            Ogre::String onTireContactFunctionName;
            ComplexVehicleDrivingManipulation* vehicleDrivingManipulation;
        };
    };

    ///////////////////////////////////////////////////////////////
    // ComplexVehicleDrivingManipulation
    ///////////////////////////////////////////////////////////////

    class EXPORTED ComplexVehicleDrivingManipulation
    {
    public:
        friend class PhysicsActiveComplexVehicleComponent::PhysicsComplexVehicleCallback;

    public:
        ComplexVehicleDrivingManipulation();
        ~ComplexVehicleDrivingManipulation();

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

}; // namespace NOWA

#endif
