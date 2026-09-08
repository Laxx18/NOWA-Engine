#ifndef PHYSICS_ACTIVE_COMPLEX_VEHICLE_COMPONENT_H
#define PHYSICS_ACTIVE_COMPLEX_VEHICLE_COMPONENT_H

#include "OgreNewt_ComplexVehicle.h"
#include "OgreNewt_World.h"
#include "PhysicsActiveComponent.h"

#include <atomic>

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
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

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
        static const Ogre::String AttrOnSteerAngleChangedFunctionName(void)
        {
            return "On Steering Angle Function Name";
        }
        static const Ogre::String AttrOnMotorForceChangedFunctionName(void)
        {
            return "On Motor Force Function Name";
        }
        static const Ogre::String AttrOnHandBrakeChangedFunctionName(void)
        {
            return "On Hand Brake Function Name";
        }
        static const Ogre::String AttrOnBrakeChangedFunctionName(void)
        {
            return "On Brake Function Name";
        }
        static const Ogre::String AttrOnTireContactFunctionName(void)
        {
            return "On Tire Function Name";
        }

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
            PhysicsComplexVehicleCallback(PhysicsActiveComplexVehicleComponent* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onSteerAngleChangedFunctionName, const Ogre::String& onMotorForceChangedFunctionName,
                const Ogre::String& onHandBrakeChangedFunctionName, const Ogre::String& onBrakeChangedFunctionName, const Ogre::String& onTireContactFunctionName);

            virtual ~PhysicsComplexVehicleCallback();

            virtual Ogre::Real onSteerAngleChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep) override;

            virtual Ogre::Real onMotorForceChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep) override;

            virtual Ogre::Real onHandBrakeChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep) override;

            virtual Ogre::Real onBrakeChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep) override;

            virtual void onTireContact(const OgreNewt::ComplexVehicleTire* tire, const Ogre::String& tireName, OgreNewt::Body* hitBody, const Ogre::Vector3& contactPosition, const Ogre::Vector3& contactNormal, Ogre::Real penetration) override;

        private:
            /**
             * @brief Runs one lua driving callback WITHOUT blocking the physics thread.
             *
             * lua_State is shared by every script in the process - the per vehicle lua
             * MODULE does not change that, because callTableFunction() pushes onto that one
             * shared stack. Calling it straight from a newton worker, as the old
             * "Is safe run in newton thread" comment claimed, means two vehicles in split
             * screen can push onto the same stack simultaneously and corrupt it.
             *
             * The decoupling works like the render command ring buffer: the physics side
             * only ever touches atomics, the logic side is the sole owner of lua. This
             * callback returns the value lua produced for the PREVIOUS frame and queues the
             * new call - at 60 Hz a one frame lag on steering or throttle is invisible,
             * whereas a blocking enqueueAndWait() would stall a physics worker per tire per
             * substep and can deadlock against the logic thread.
             *
             * @param[in] functionName      The lua table function to call
             * @param[in] cachedResult      The atomic holding the last result lua produced
             * @param[in] inputValue        Value written into the manipulation before the call
             * @param[in] timestep          Physics timestep passed on to lua
             * @param[in] resultGetter      Reads the value lua wrote back
             * @return The cached result, i.e. the value from the previous frame
             */
            Ogre::Real callDrivingFunction(const Ogre::String& functionName, std::atomic<Ogre::Real>& cachedResult, Ogre::Real timestep, Ogre::Real (ComplexVehicleDrivingManipulation::*resultGetter)(void) const);

            PhysicsActiveComplexVehicleComponent* owner;
            LuaScript* luaScript;
            OgreNewt::World* ogreNewt;

            // Written by the LOGIC thread after each lua call, read by the PHYSICS thread.
            // Atomic, so no lock is needed on either side.
            std::atomic<Ogre::Real> cachedSteerAngle;
            std::atomic<Ogre::Real> cachedMotorForce;
            std::atomic<Ogre::Real> cachedHandBrake;
            std::atomic<Ogre::Real> cachedBrake;
            Ogre::String onSteerAngleChangedFunctionName;
            Ogre::String onMotorForceChangedFunctionName;
            Ogre::String onHandBrakeChangedFunctionName;
            Ogre::String onBrakeChangedFunctionName;
            Ogre::String onTireContactFunctionName;
            // The ComplexVehicleDrivingManipulation member is gone on purpose. It used to be
            // owned here and its pointer captured by the deferred logic commands, so a
            // command still sitting in the queue when this callback got destroyed - e.g. on
            // a mid game vehicle swap - dereferenced freed memory. Each command now builds
            // its own local instance, which leaves nothing of this object for a queued
            // command to reach into.
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