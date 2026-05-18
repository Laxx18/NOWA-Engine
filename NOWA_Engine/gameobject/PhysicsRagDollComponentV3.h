#ifndef PHYSICS_RAGDOLL_COMPONENT_V3_H
#define PHYSICS_RAGDOLL_COMPONENT_V3_H

#include "OgreNewt_RagDollBody.h"
#include "PhysicsActiveComponent.h"
#include "utilities/rapidxml.hpp"

namespace NOWA
{
    /**
     * @brief PhysicsRagDollComponentV3 - Articulation-based ragdoll using OgreNewt::RagDollBody.
     *
     * Replaces V2's individual-body-per-bone approach with Newton 4's ndModelArticulation
     * (single solver pass, no self-collision, much more efficient).
     *
     * The .rag XML format is identical to V2. Parsing happens here in NOWA;
     * OgreNewt::RagDollBody receives only the filled RagConfig struct.
     *
     * States:
     *   Inactive         - behaves like PhysicsActiveComponent (single collision body)
     *   Ragdolling       - full ragdoll via ndModelArticulation
     *   PartialRagdolling - partial ragdoll (root bone animation-driven)
     *   Animation        - animation drives bones, physics bodies follow
     */
    class EXPORTED PhysicsRagDollComponentV3 : public PhysicsActiveComponent
    {
    public:
        typedef boost::shared_ptr<PhysicsRagDollComponentV3> PhysicsRagDollComponentV3Ptr;

        enum RDState
        {
            INACTIVE = 0,
            ANIMATION,
            RAGDOLLING,
            PARTIAL_RAGDOLLING
        };

        PhysicsRagDollComponentV3();
        virtual ~PhysicsRagDollComponentV3();

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual Ogre::String getParentParentClassName(void) const override;

        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual void onRemoveComponent(void) override;

        virtual bool isMovable(void) const override
        {
            return true;
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PhysicsRagDollComponentV3");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PhysicsRagDollComponentV3";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Articulation-based ragdoll for Ogre::Item (V2 skeleton). Uses Newton 4's ndModelArticulation "
                   "for efficient ragdoll simulation (single solver pass, no self-collision). "
                   "States: Inactive, Animation, Ragdolling, PartialRagdolling. "
                   "Requirements: A game object with mesh and skeleton (Ogre::Item with SkeletonInstance).";
        }

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
        virtual void showDebugData(void) override;
        virtual void setActivated(bool activated) override;

        // ── Ragdoll control ──

        void setState(const Ogre::String& state);
        Ogre::String getState(void) const;

        void setBoneConfigFile(const Ogre::String& boneConfigFile);
        Ogre::String getBoneConfigFile(void) const;

        void inheritVelOmega(Ogre::Vector3 vel, Ogre::Vector3 omega);

        void setAnimationEnabled(bool animationEnabled);
        bool isAnimationEnabled(void) const;

        void setInitialState(void);

        // ── Physics overrides ──

        virtual OgreNewt::Body* getBody(void) const override;
        virtual void setVelocity(const Ogre::Vector3& velocity) override;
        virtual Ogre::Vector3 getVelocity(void) const override;
        virtual void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z);
        virtual void setPosition(const Ogre::Vector3& position) override;
        virtual void translate(const Ogre::Vector3& relativePosition) override;
        virtual Ogre::Vector3 getPosition(void) const override;
        virtual void setOrientation(const Ogre::Quaternion& orientation) override;
        virtual void rotate(const Ogre::Quaternion& relativeRotation) override;
        virtual Ogre::Quaternion getOrientation(void) const override;

        // ── Bone access ──

        OgreNewt::RagDollBody* getRagDollBody(void) const;
        OgreNewt::Body* getBoneBody(const Ogre::String& ragBoneName) const;
        Ogre::Bone* getOgreBone(const Ogre::String& ragBoneName) const;

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrBonesConfigFile(void)
        {
            return "Config. File";
        }
        static const Ogre::String AttrState(void)
        {
            return "State";
        }

    private:
        bool parseRagConfig(OgreNewt::RagDollBody::RagConfig& outConfig);
        bool createRagDoll(void);
        void destroyRagDoll(void);

        void internalApplyState(void);
        void createInactiveBody(void);

        void startRagdolling(void);
        void endRagdolling(void);

        void internalShowDebugData(bool activate);

        void gameObjectAnimationChangedDelegate(EventDataPtr eventData);
        void deleteJointDelegate(EventDataPtr eventData);

    private:
        Variant* activated;
        Variant* boneConfigFile;
        Variant* state;

        RDState rdState;
        RDState rdOldState;

        Ogre::SkeletonInstance* skeletonInstance;

        OgreNewt::RagDollBody* ragDollBody;

        bool animationEnabled;
        bool isSimulating;
        Ogre::String partialRagdollBoneName;

        Ogre::Vector3 ragdollPositionOffset;
        Ogre::Quaternion ragdollOrientationOffset;
    };

}; // namespace end

#endif