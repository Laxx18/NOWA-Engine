#ifndef PHYSICS_ACTIVE_VEHICLE_COMPONENT_V2_H
#define PHYSICS_ACTIVE_VEHICLE_COMPONENT_V2_H

#include "OgreNewt_VehicleV2.h"
#include "PhysicsActiveComponent.h"

namespace NOWA
{
    /**
     * @brief Impulse-based vehicle component for NOWA-Engine (Newton Dynamics 4).
     *
     * Setup in NOWA-Design:
     *   1. Add this component to the chassis GameObject.
     *   2. Place up to 8 tire GameObjects anywhere in the scene (no physics
     *      component needed on tires – they are purely visual).
     *   3. Set TireId_0 … TireId_7 to the IDs of those GameObjects.
     *   4. Add a LuaScriptComponent and implement the four control callbacks
     *      with the same signatures as the old PhysicsActiveVehicleComponent:
     *
     *   Scene1_MyCar_0["onSteerAngleChanged"] = function(manip, dt) ... end
     *   Scene1_MyCar_0["onMotorForceChanged"] = function(manip, dt) ... end
     *   Scene1_MyCar_0["onHandBrakeChanged"]  = function(manip, dt) ... end
     *   Scene1_MyCar_0["onBrakeChanged"]      = function(manip, dt) ... end
     *
     * The manip object supports:
     *   manip:setSteerAngle(deg)   manip:getSteerAngle()
     *   manip:setMotorForce(f)     manip:getMotorForce()
     *   manip:setHandBrake(f)      manip:getHandBrake()
     *   manip:setBrake(f)          manip:getBrake()
     *
     * Vehicle forward axis is +X.  Use the Mesh Tool to align the chassis
     * mesh if needed.
     */
    class EXPORTED PhysicsActiveVehicleComponentV2 : public PhysicsActiveComponent
    {
    public:
        typedef boost::shared_ptr<PhysicsActiveVehicleComponentV2> PhysicsActiveVehicleComponentV2CompPtr;

        static constexpr int MAX_TIRES = 8;

    public:
        // ─────────────────────────────────────────────────────────────────────
        // Inner callback: bridges OgreNewt::VehicleV2Callback -> Lua
        // Four separate Lua functions match the old vehicle's API exactly.
        // ─────────────────────────────────────────────────────────────────────
        class EXPORTED PhysicsVehicleV2Callback : public OgreNewt::VehicleV2Callback
        {
        public:
            PhysicsVehicleV2Callback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onSteerAngleChangedFunctionName, const Ogre::String& onMotorForceChangedFunctionName,
                const Ogre::String& onHandBrakeChangedFunctionName, const Ogre::String& onBrakeChangedFunctionName);

            virtual ~PhysicsVehicleV2Callback();

            virtual Ogre::Real onSteerAngleChanged(OgreNewt::VehicleDrivingManipulationV2* manip, Ogre::Real dt) override;

            virtual Ogre::Real onMotorForceChanged(OgreNewt::VehicleDrivingManipulationV2* manip, Ogre::Real dt) override;

            virtual Ogre::Real onHandBrakeChanged(OgreNewt::VehicleDrivingManipulationV2* manip, Ogre::Real dt) override;

            virtual Ogre::Real onBrakeChanged(OgreNewt::VehicleDrivingManipulationV2* manip, Ogre::Real dt) override;

        private:
            GameObject* m_owner;
            LuaScript* m_luaScript;
            OgreNewt::World* m_ogreNewt;
            Ogre::String m_onSteerAngleChangedFunctionName;
            Ogre::String m_onMotorForceChangedFunctionName;
            Ogre::String m_onHandBrakeChangedFunctionName;
            Ogre::String m_onBrakeChangedFunctionName;
        };

    public:
        PhysicsActiveVehicleComponentV2();
        virtual ~PhysicsActiveVehicleComponentV2();

        // ── GameObjectComponent interface ─────────────────────────────────────

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        virtual bool postInit(void) override;

        virtual void onRemoveComponent(void) override;

        virtual bool connect(void) override;

        virtual bool disconnect(void) override;

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        virtual void actualizeValue(Variant* attribute) override;

        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual Ogre::String getClassName(void) const override;

        virtual Ogre::String getParentClassName(void) const override;

        virtual Ogre::String getParentParentClassName(void) const override;

        virtual void setActivated(bool activated) override;

        virtual void reCreateDynamicBodyForItem(Ogre::Item* item) override;

        // ── Static helpers ────────────────────────────────────────────────────

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PhysicsActiveVehicleComponentV2");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PhysicsActiveVehicleComponentV2";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Impulse-based vehicle component (Newton Dynamics 4). "
                   "No joint components required. "
                   "Add up to 8 tire GameObjects (no physics component on tires) "
                   "and assign their IDs: "
                   "TireId_0=Front-Left, TireId_1=Front-Right, "
                   "TireId_2=Rear-Left,  TireId_3=Rear-Right "
                   "(TireId_4..7 for extra axles, set to 0 if unused). "
                   "Front tires (slots 0-1) rotate visually when steering. "
                   "Implement the four Lua callbacks with the same signatures as "
                   "PhysicsActiveVehicleComponent. "
                   "Vehicle forward axis is controlled by DefaultDirection "
                   "(default +X; use Mesh Tool to align if needed).";
        }

        // ── Attribute setters / getters ───────────────────────────────────────

        /** Tire ID slots 0..MAX_TIRES-1 */
        void setTireId(int index, unsigned long tireId);
        unsigned long getTireId(int index) const;

        void setOnSteerAngleChangedFunctionName(const Ogre::String& name);
        Ogre::String getOnSteerAngleChangedFunctionName(void) const;

        void setOnMotorForceChangedFunctionName(const Ogre::String& name);
        Ogre::String getOnMotorForceChangedFunctionName(void) const;

        void setOnHandBrakeChangedFunctionName(const Ogre::String& name);
        Ogre::String getOnHandBrakeChangedFunctionName(void) const;

        void setOnBrakeChangedFunctionName(const Ogre::String& name);
        Ogre::String getOnBrakeChangedFunctionName(void) const;

        void setTireDirectionSwap(bool swap);

        bool getTireDirectionSwap() const;

        void setSteeringStrength(Ogre::Real strength);

        Ogre::Real getSteeringStrength() const;

        void setTireSpinAxis(const Ogre::String& axis);

        Ogre::String getTireSpinAxis() const;

        /** Drive / freeze without destroying the body. */
        void setCanDrive(bool canDrive);

        /** true when no active contacts exist on the chassis body. */
        bool isAirborne(void) const;

        /** Last longitudinal force (for HUD / camera feedback). */
        Ogre::Vector3 getVehicleForce(void) const;

        /** Apply a wheely stunt (lift front). Same signature as old component. */
        void applyWheelie(Ogre::Real strength);

        /** Apply a drift stunt. Same signature as old component. */
        void applyDrift(bool left, Ogre::Real strength, Ogre::Real steeringStrength);

        /** Apply pitch angular impulse. Same signature as old component. */
        void applyPitch(Ogre::Real strength, Ogre::Real dt);

        OgreNewt::VehicleV2* getVehicleV2(void) const;

        // ── Attribute name constants ──────────────────────────────────────────

        static const Ogre::String AttrTireId(int i)
        {
            return "TireId_" + Ogre::StringConverter::toString(i);
        }
        static const Ogre::String AttrOnSteerAngleChangedFunctionName(void)
        {
            return "On Steer Angle Function Name";
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
        static Ogre::String AttrTireDirectionSwap()
        {
            return "TireDirectionSwap";
        }
        static Ogre::String AttrSteeringStrength()
        {
            return "SteeringStrength";
        }
        static Ogre::String AttrTireSpinAxis()
        {
            return "TireSpinAxis";
        }

    protected:
        virtual bool createDynamicBody(void) override;

    private:
        /** Newton force callback: gravity (base) + vehicle impulses. */
        void vehicleMoveCallback(OgreNewt::Body* body, Ogre::Real dt, int threadIndex);

        /** Scan TireId slots, resolve GameObjects, register in VehicleV2. */
        void registerTireNodes(void);

        /** Called from update(): tick VehicleV2::computeTireTransforms, then
         *  push world transforms into GraphicsModule. */
        void updateTireNodes(Ogre::Real dt);

        void applySpinAxisToVehicle(OgreNewt::VehicleV2* vehicle);

    private:
        Variant* m_tireIds[MAX_TIRES];
        Variant* m_onSteerAngleChangedFunctionName;
        Variant* m_onMotorForceChangedFunctionName;
        Variant* m_onHandBrakeChangedFunctionName;
        Variant* m_onBrakeChangedFunctionName;
        Variant* m_tireDirectionSwap;
        Variant* m_steeringStrength;
        Variant* m_tireSpinAxis;
    };

} // namespace NOWA

#endif // PHYSICS_ACTIVE_VEHICLE_COMPONENT_V2_H
