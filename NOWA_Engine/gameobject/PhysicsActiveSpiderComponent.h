#ifndef PHYSICS_ACTIVE_SPIDER_COMPONENT_H
#define PHYSICS_ACTIVE_SPIDER_COMPONENT_H

#include "OgreNewt_SpiderBody.h"
#include "PhysicsActiveComponent.h"

namespace NOWA
{
    /**
     * @brief Procedural IK spider component (Newton Dynamics 4).
     *
     * Instead of assigning 12 individual GameObjectIds for leg segments,
     * the component owns its visual nodes/items internally:
     *   - 3 mesh names: ThighMeshName, CalfMeshName, HeelMeshName
     *   - 4 hip socket positions  (LegHipPos_0..3,      torso-local Vector3)
     *   - 4 foot rest positions   (LegFootRestPos_0..3, torso-local Vector3)
     * The component creates 4 SceneNodes+Items per mesh name (12 total) on the
     * render thread when the simulation starts, and destroys them when it stops.
     * Bone lengths are derived automatically from each mesh's AABB.
     *
     * SETUP IN NOWA-DESIGN:
     *   1. Add to TORSO GameObject. CollisionType=ConvexHull, Mass=~30 kg,
     *      MassOrigin=(0,-0.1,0), DefaultDirection=forward axis of torso (+X).
     *
     *   2. Set ThighMeshName / CalfMeshName / HeelMeshName (file-open dialog).
     *      Each mesh origin must sit at its joint (hip / knee / ankle).
     *      Mesh must extend along LegBoneAxis (default Y).
     *      Bone length is derived from the mesh AABB – no manual entry needed.
     *
     *   3. Set LegHipPos_0..3 (torso-local Vector3):
     *      Default (spider facing +X):
     *        Leg 0 FL  ( 0.3, 0,-0.3)   Leg 1 FR  ( 0.3, 0, 0.3)
     *        Leg 2 RL  (-0.3, 0,-0.3)   Leg 3 RR  (-0.3, 0, 0.3)
     *
     *   4. Set LegFootRestPos_0..3 (torso-local Vector3).
     *      Y must be negative. Default:
     *        Leg 0 ( 0.5,-0.4,-0.6)  Leg 1 ( 0.5,-0.4, 0.6)
     *        Leg 2 (-0.5,-0.4,-0.6)  Leg 3 (-0.5,-0.4, 0.6)
     *
     *   5. Gait tuning: WalkCycleDuration, StepHeight, GaitSequence, LegBoneAxis.
     *
     *   6. LuaScriptComponent:
     *      Spider_0["onMovementChanged"] = function(manip, dt)
     *          if Input:isKeyDown(KEY_W) then manip:setStride(0.3) end
     *          if Input:isKeyDown(KEY_A) then manip:setOmega(-0.25)
     *          elseif Input:isKeyDown(KEY_D) then manip:setOmega(0.25) end
     *      end
     */
    class EXPORTED PhysicsActiveSpiderComponent : public PhysicsActiveComponent
    {
    public:
        typedef boost::shared_ptr<PhysicsActiveSpiderComponent> PhysicsActiveSpiderComponentCompPtr;

        static constexpr int MAX_LEGS = 4;

    public:
        // ── Lua callback bridge ───────────────────────────────────────────────
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

    private:
        // ── Internally owned visual nodes/items (12 total, 3 per leg) ─────────
        // Created on the render thread in connect(), destroyed in disconnect().
        // Nodes are children of the torso scene node; the IK system sets their
        // world transforms each frame via GraphicsModule::updateNodeTransform.
        struct LegVisual
        {
            Ogre::SceneNode* thighNode = nullptr;
            Ogre::SceneNode* calfNode = nullptr;
            Ogre::SceneNode* heelNode = nullptr;
            Ogre::Item* thighItem = nullptr;
            Ogre::Item* calfItem = nullptr;
            Ogre::Item* heelItem = nullptr;
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
            return "PhysicsActiveSpiderComponent - procedural IK spider (Newton Dynamics 4).\n\n"
                   "SETUP:\n"
                   "1. Add to TORSO. CollisionType=ConvexHull, Mass=~30 kg, "
                   "MassOrigin=(0,-0.1,0), DefaultDirection=+X.\n\n"
                   "2. Set THREE mesh names (file dialog 'Models'):\n"
                   "   ThighMeshName - upper leg. Mesh origin = HIP joint. Extends along LegBoneAxis.\n"
                   "   CalfMeshName  - lower leg. Mesh origin = KNEE joint.\n"
                   "   HeelMeshName  - foot.      Mesh origin = ANKLE joint.\n"
                   "   Bone lengths are computed automatically from the mesh AABB.\n\n"
                   "3. LegHipPos_0..3 (torso-local Vector3) - hip socket positions:\n"
                   "   Leg 0 FL (0.3,0,-0.3)  Leg 1 FR (0.3,0,0.3)\n"
                   "   Leg 2 RL (-0.3,0,-0.3) Leg 3 RR (-0.3,0,0.3)\n\n"
                   "4. LegFootRestPos_0..3 (torso-local Vector3) - neutral ground contacts:\n"
                   "   Y must be negative. "
                   "   Leg 0 (0.5,-0.4,-0.6) Leg 1 (0.5,-0.4,0.6) "
                   "   Leg 2 (-0.5,-0.4,-0.6) Leg 3 (-0.5,-0.4,0.6)\n\n"
                   "5. Gait: WalkCycleDuration (default 2.0 s), StepHeight (0.2 m), "
                   "GaitSequence ('3,1,2,0'), LegBoneAxis (Y/X/Z).\n\n"
                   "6. Lua: onMovementChanged(manip,dt) - "
                   "manip:setStride(0..0.5), manip:setOmega(+-0.25).\n\n"
                   "LEG ORDER (top view, facing +X): 0=FL 1=FR 2=RL 3=RR";
        }

        // ── Mesh name setters / getters ───────────────────────────────────────
        void setThighMeshName(const Ogre::String& meshName);
        Ogre::String getThighMeshName(void) const;

        void setCalfMeshName(const Ogre::String& meshName);
        Ogre::String getCalfMeshName(void) const;

        void setHeelMeshName(const Ogre::String& meshName);
        Ogre::String getHeelMeshName(void) const;

        // ── Per-leg placement setters / getters ───────────────────────────────
        void setLegHipPos(int index, const Ogre::Vector3& pos);
        Ogre::Vector3 getLegHipPos(int index) const;

        void setLegFootRestPos(int index, const Ogre::Vector3& pos);
        Ogre::Vector3 getLegFootRestPos(int index) const;

        // ── Gait setters / getters ────────────────────────────────────────────
        void setOnMovementChangedFunctionName(const Ogre::String& name);
        Ogre::String getOnMovementChangedFunctionName(void) const;

        void setWalkCycleDuration(Ogre::Real duration);
        Ogre::Real getWalkCycleDuration(void) const;

        void setStepHeight(Ogre::Real height);
        Ogre::Real getStepHeight(void) const;

        void setGaitSequence(const Ogre::String& sequence);
        Ogre::String getGaitSequence(void) const;

        void setLegBoneAxis(const Ogre::String& axis);
        Ogre::String getLegBoneAxis(void) const;

        void       setLegMeshScale(Ogre::Real scale);
        Ogre::Real getLegMeshScale(void) const;

        void       setThighBoneLength(Ogre::Real length);
        Ogre::Real getThighBoneLength(void) const;

        void       setCalfBoneLength(Ogre::Real length);
        Ogre::Real getCalfBoneLength(void) const;

        void setCanMove(bool canMove);
        bool isOnGround(void) const;

        OgreNewt::SpiderBody* getSpiderBody(void) const;

        // ── Attribute name constants ──────────────────────────────────────────
        static Ogre::String AttrThighMeshName(void) { return "ThighMeshName"; }
        static Ogre::String AttrCalfMeshName(void)  { return "CalfMeshName";  }
        static Ogre::String AttrHeelMeshName(void)  { return "HeelMeshName";  }

        static Ogre::String AttrLegHipPos(int i)
        {
            return "LegHipPos_" + Ogre::StringConverter::toString(i);
        }
        static Ogre::String AttrLegFootRestPos(int i)
        {
            return "LegFootRestPos_" + Ogre::StringConverter::toString(i);
        }

        static Ogre::String AttrOnMovementChangedFunctionName(void)
        {
            return "On Movement Changed Function Name";
        }
        static Ogre::String AttrWalkCycleDuration(void)  { return "WalkCycleDuration"; }
        static Ogre::String AttrStepHeight(void)         { return "StepHeight"; }
        static Ogre::String AttrGaitSequence(void)       { return "GaitSequence"; }
        static Ogre::String AttrLegBoneAxis(void)        { return "LegBoneAxis"; }
        static Ogre::String AttrLegMeshScale(void)       { return "LegMeshScale"; }
        static Ogre::String AttrThighBoneLength(void)     { return "ThighBoneLength"; }
        static Ogre::String AttrCalfBoneLength(void)      { return "CalfBoneLength"; }

    protected:
        virtual bool createDynamicBody(void) override;

    private:
        void spiderMoveCallback(OgreNewt::Body* body, Ogre::Real dt, int threadIndex);

        /** Creates 12 SceneNodes+Items (3 per leg x 4 legs) on the render thread.
         *  Follows the foliage batch pattern: first item per mesh type extracts
         *  datablocks; remaining 3 items share them (enables Ogre render batching).
         *  Derives bone lengths from mesh AABBs automatically. */
        bool createLegMeshes(void);

        /** Destroys all 12 SceneNodes+Items on the render thread. */
        void destroyLegMeshes(void);

        /** Registers all 4 leg chains into SpiderBody. Called once after createLegMeshes(). */
        void registerLegChains(void);

        /** Ticks gait+IK and pushes all world transforms to GraphicsModule. */
        void updateLegNodes(Ogre::Real dt);

        static std::vector<int> parseGaitSequence(const Ogre::String& str, int legCount);
        static int boneAxisFromString(const Ogre::String& axis);

        /** Returns mesh extent along boneAxis (0=X,1=Y,2=Z) from its AABB. */
        static Ogre::Real deriveBoneLength(
            const Ogre::String& meshName,
            const Ogre::String& resourceGroup,
            int boneAxis, Ogre::Real fallback = 0.5f);

        /** Resolves resource group from PathToFolder user data on the Variant. */
        static Ogre::String resolveResourceGroup(Variant* meshNameVariant);

    private:
        // ── Mesh name Variants ────────────────────────────────────────────────
        Variant* m_thighMeshName;
        Variant* m_calfMeshName;
        Variant* m_heelMeshName;

        Ogre::String m_thighResourceGroup;
        Ogre::String m_calfResourceGroup;
        Ogre::String m_heelResourceGroup;

        // ── Per-leg Variants ──────────────────────────────────────────────────
        Variant* m_legHipPos[MAX_LEGS];
        Variant* m_legFootRestPos[MAX_LEGS];

        // ── Gait Variants ─────────────────────────────────────────────────────
        Variant* m_onMovementChangedFunctionName;
        Variant* m_walkCycleDuration;
        Variant* m_stepHeight;
        Variant* m_gaitSequence;
        Variant* m_legBoneAxis;
        Variant* m_legMeshScale; // uniform scale applied to all 12 leg segment nodes
        Variant* m_thighBoneLength; // 0 = auto-compute from hip/foot positions
        Variant* m_calfBoneLength;  // 0 = auto-compute from hip/foot positions

        // ── Internally created visuals ────────────────────────────────────────
        LegVisual m_legVisuals[MAX_LEGS];

    };

} // namespace NOWA

#endif // PHYSICS_ACTIVE_SPIDER_COMPONENT_H