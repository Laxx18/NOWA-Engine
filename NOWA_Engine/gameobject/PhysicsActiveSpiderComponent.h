#ifndef PHYSICS_ACTIVE_SPIDER_COMPONENT_H
#define PHYSICS_ACTIVE_SPIDER_COMPONENT_H

#include "OgreNewt_SpiderBody.h"
#include "PhysicsActiveComponent.h"

namespace NOWA
{
    /**
     * @brief Procedural IK spider component for NOWA-Engine (Newton Dynamics 4).
     *
     * Setup in NOWA-Design:
     *   1. Add this component to the TORSO GameObject (needs a physics collision shape).
     *   2. Create up to MAX_LEGS sets of three GameObjects for each leg:
     *        ThighGO (origin = hip joint)
     *        CalfGO  (origin = knee joint)
     *        HeelGO  (origin = ankle joint)
     *      These are purely visual – no physics component on them.
     *   3. Assign their IDs:
     *        ThighId_0 … ThighId_3
     *        CalfId_0  … CalfId_3
     *        HeelId_0  … HeelId_3
     *      Leave unused slots at 0.
     *   4. Configure:
     *        WalkCycleDuration  – seconds per full cycle (default 2.0)
     *        StepHeight         – how high the foot arcs (default 0.2)
     *        GaitSequence       – comma-separated phase order, e.g. "3,1,2,0"
     *        LegBoneAxis        – X / Y / Z: axis along which each limb mesh extends (default Y)
     *   5. Add a LuaScriptComponent and implement:
     *
     *        Spider_0["onMovementChanged"] = function(manip, dt)
     *            if Input:isKeyDown(KEY_W) then
     *                manip:setStride(0.3)
     *            end
     *            if Input:isKeyDown(KEY_A) then
     *                manip:setOmega(-0.25)
     *            elseif Input:isKeyDown(KEY_D) then
     *                manip:setOmega(0.25)
     *            end
     *        end
     *
     * Leg node origin requirements:
     *   Each ThighGO/CalfGO/HeelGO SceneNode origin MUST sit at its respective
     *   joint (hip, knee, ankle) and the mesh MUST extend along the configured
     *   LegBoneAxis.  This matches the default Blender export (Y-up bones).
     *
     * Torso forward axis: set via DefaultDirection on the torso GameObject (default +X).
     */
    class EXPORTED PhysicsActiveSpiderComponent : public PhysicsActiveComponent
    {
    public:
        typedef boost::shared_ptr<PhysicsActiveSpiderComponent> PhysicsActiveSpiderComponentCompPtr;

        static constexpr int MAX_LEGS = 4; // extend to 6 or 8 for hexapods / octopods

    public:
        // ─────────────────────────────────────────────────────────────────────
        // Inner callback: bridges OgreNewt::SpiderCallback → Lua
        // ─────────────────────────────────────────────────────────────────────
        class EXPORTED PhysicsSpiderCallback : public OgreNewt::SpiderCallback
        {
        public:
            PhysicsSpiderCallback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onMovementChangedFunctionName);

            virtual ~PhysicsSpiderCallback();

            virtual void onMovementChanged(OgreNewt::SpiderMovementManipulation* manip, Ogre::Real dt) override;

        private:
            GameObject* m_owner;
            LuaScript* m_luaScript;
            OgreNewt::World* m_ogreNewt;
            Ogre::String m_onMovementChangedFunctionName;
        };

    public:
        PhysicsActiveSpiderComponent();
        virtual ~PhysicsActiveSpiderComponent();

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

        virtual bool isMovable(void) const override
        {
            return true;
        }

        virtual void setActivated(bool activated) override;

        // ── Static helpers ────────────────────────────────────────────────────

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PhysicsActiveSpiderComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PhysicsActiveSpiderComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "PhysicsActiveSpiderComponent – procedural IK spider (Newton Dynamics 4).\n\n"

                   "SETUP IN NOWA-DESIGN:\n"
                   "1. Add this component to the TORSO GameObject (the main body).\n"
                   "   - CollisionType: ConvexHull (derives correct inertia from mesh).\n"
                   "   - Mass: e.g. 30 kg for a small spider.\n"
                   "   - MassOrigin: slight negative Y (e.g. 0, -0.1, 0) to lower centre of mass.\n"
                   "   - DefaultDirection: forward axis of the torso mesh (default +X).\n\n"

                   "2. Create 4 sets of THREE purely visual GameObjects per leg (no physics component!):\n"
                   "   - ThighGO: origin at the HIP JOINT,   mesh extends along LegBoneAxis.\n"
                   "   - CalfGO:  origin at the KNEE JOINT,  mesh extends along LegBoneAxis.\n"
                   "   - HeelGO:  origin at the ANKLE JOINT, mesh extends along LegBoneAxis.\n"
                   "   Node origin placement is critical: misplaced origins produce broken IK.\n\n"

                   "3. Assign IDs in the component:\n"
                   "   ThighId_0..3 = IDs of the four thigh GameObjects.\n"
                   "   CalfId_0..3  = IDs of the four calf GameObjects.\n"
                   "   HeelId_0..3  = IDs of the four heel GameObjects.\n"
                   "   Leave unused slots at 0.\n\n"

                   "4. Set LegBoneAxis to match your mesh:\n"
                   "   Y = bone extends along +Y (Blender / GLTF default for limbs).\n"
                   "   X = bone extends along +X.\n"
                   "   Z = bone extends along +Z.\n\n"

                   "5. Gait tuning:\n"
                   "   WalkCycleDuration: seconds per full step cycle (default 2.0; lower = faster).\n"
                   "   StepHeight: foot arc height during swing (default 0.2 m).\n"
                   "   GaitSequence: comma-separated phase order, e.g. '3,1,2,0' (diagonal walk).\n"
                   "     2 legs: '1,0'   4 legs: '3,1,2,0'   6 legs: '5,2,4,1,3,0'\n\n"

                   "6. Add a LuaScriptComponent and implement:\n"
                   "   Spider_0['onMovementChanged'] = function(manip, dt)\n"
                   "       if Input:isKeyDown(KEY_W) then manip:setStride(0.3) end\n"
                   "       if Input:isKeyDown(KEY_A) then manip:setOmega(-0.25)\n"
                   "       elseif Input:isKeyDown(KEY_D) then manip:setOmega(0.25) end\n"
                   "   end\n\n"

                   "IMPORTANT THREAD NOTE:\n"
                   "   onMovementChanged runs on the MAIN THREAD (safe to read input, call game logic).\n"
                   "   Internally, stride/omega are cached and consumed by the Newton physics thread.\n"
                   "   Never call Ogre scene graph functions from inside Newton force callbacks.\n\n"

                   "LEG ORDERING CONVENTION (viewed from above, spider facing +X):\n"
                   "   Leg 0 = Front-Left   Leg 1 = Front-Right\n"
                   "   Leg 2 = Rear-Left    Leg 3 = Rear-Right";
        }

        // ── Attribute setters / getters ───────────────────────────────────────

        void setThighId(int index, unsigned long id);
        unsigned long getThighId(int index) const;

        void setCalfId(int index, unsigned long id);
        unsigned long getCalfId(int index) const;

        void setHeelId(int index, unsigned long id);
        unsigned long getHeelId(int index) const;

        void setOnMovementChangedFunctionName(const Ogre::String& name);
        Ogre::String getOnMovementChangedFunctionName(void) const;

        void setWalkCycleDuration(Ogre::Real duration);
        Ogre::Real getWalkCycleDuration(void) const;

        void setStepHeight(Ogre::Real height);
        Ogre::Real getStepHeight(void) const;

        /** Comma-separated gait sequence, e.g. "3,1,2,0".
         *  Length must match the number of active legs. */
        void setGaitSequence(const Ogre::String& sequence);
        Ogre::String getGaitSequence(void) const;

        /** Axis along which each limb mesh extends: "X", "Y" or "Z". */
        void setLegBoneAxis(const Ogre::String& axis);
        Ogre::String getLegBoneAxis(void) const;

        void setCanMove(bool canMove);
        bool isOnGround(void) const;

        OgreNewt::SpiderBody* getSpiderBody(void) const;

        // ── Attribute name constants ──────────────────────────────────────────

        static Ogre::String AttrThighId(int i)
        {
            return "ThighId_" + Ogre::StringConverter::toString(i);
        }
        static Ogre::String AttrCalfId(int i)
        {
            return "CalfId_" + Ogre::StringConverter::toString(i);
        }
        static Ogre::String AttrHeelId(int i)
        {
            return "HeelId_" + Ogre::StringConverter::toString(i);
        }
        static Ogre::String AttrOnMovementChangedFunctionName(void)
        {
            return "On Movement Changed Function Name";
        }
        static Ogre::String AttrWalkCycleDuration(void)
        {
            return "WalkCycleDuration";
        }
        static Ogre::String AttrStepHeight(void)
        {
            return "StepHeight";
        }
        static Ogre::String AttrGaitSequence(void)
        {
            return "GaitSequence";
        }
        static Ogre::String AttrLegBoneAxis(void)
        {
            return "LegBoneAxis";
        }

    protected:
        virtual bool createDynamicBody(void) override;

    private:
        /** Newton force callback: gravity (base) + spider locomotion impulses. */
        void spiderMoveCallback(OgreNewt::Body* body, Ogre::Real dt, int threadIndex);

        /** Resolve GameObjects from IDs, compute bind-pose geometry, register
         *  all leg chains in SpiderBody.  Called once from connect(). */
        void registerLegChains(void);

        /** Called from update(): tick computeLegTransforms, then push world
         *  transforms for all segments into GraphicsModule. */
        void updateLegNodes(Ogre::Real dt);

        /** Parse GaitSequence string "3,1,2,0" → vector<int>.
         *  Falls back to a sensible default based on active leg count. */
        static std::vector<int> parseGaitSequence(const Ogre::String& str, int legCount);

        /** Convert LegBoneAxis attribute string ("X"/"Y"/"Z") to int (0/1/2). */
        static int boneAxisFromString(const Ogre::String& axis);

    private:
        // 3 ID variants per leg
        Variant* m_thighIds[MAX_LEGS];
        Variant* m_calfIds[MAX_LEGS];
        Variant* m_heelIds[MAX_LEGS];

        Variant* m_onMovementChangedFunctionName;
        Variant* m_walkCycleDuration;
        Variant* m_stepHeight;
        Variant* m_gaitSequence;
        Variant* m_legBoneAxis;
    };

} // namespace NOWA

#endif // PHYSICS_ACTIVE_SPIDER_COMPONENT_H
