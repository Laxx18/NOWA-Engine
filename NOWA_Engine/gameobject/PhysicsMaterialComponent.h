#ifndef PHYSICS_MATERIAL_COMPONENT_H
#define PHYSICS_MATERIAL_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
    class ConveyorContactCallback;
    class WallSlideContactCallback;
    class GenericContactCallback;
    class OneWayContactCallback;
    class LuaScript;

    class EXPORTED PhysicsMaterialComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::PhysicsMaterialComponent> PhysicsMaterialCompPtr;

    public:
        PhysicsMaterialComponent();

        virtual ~PhysicsMaterialComponent();

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        virtual bool postInit(void) override;

        virtual bool connect(void) override;

        virtual Ogre::String getClassName(void) const override;

        virtual Ogre::String getParentClassName(void) const override;

        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PhysicsMaterialComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PhysicsMaterialComponent";
        }

        /**
         * @see		GameObjectComponent::canStaticAddComponent
         */
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @see  GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: With this component physics effects like friction, possibility on reacting when two physically active game objects have collided etc. It glues two game object categories together. "
                   "Note: Several of this components can be created.\n"
                   "Attention: Physics material collision do only work for the following constellations:\n"
                   "Category1						Category2							Result\n"
                   "PhysicsActiveComponent			PhysicsActiveComponent				Working\n"
                   "PhysicsArtifactComponent		PhysicsActiveComponent				Working\n"
                   "PhysicsActiveComponent			PhysicsKinematicComponent			Working\n"
                   "PhysicsArtifactComponent		PhysicsKinematicComponent			Not Working\n"
                   "PhysicsKinematicComponent		PhysicsKinematicComponent			Not Working\n"
                   "The two last constellations do not work, because no force is taking place, use PhysicsTriggerComponent, in order to detect collisions of this ones!";
        }

        virtual void update(Ogre::Real dt, bool notSimulating = false) override {};

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        void setCategory1(const Ogre::String& category1);

        Ogre::String getCategory1(void) const;

        void setCategory2(const Ogre::String& category2);

        Ogre::String getCategory2(void) const;

        void setFriction(const Ogre::Vector2& friction);

        Ogre::Vector2 getFriction(void) const;

        void setSoftness(Ogre::Real softness);

        Ogre::Real getSoftness(void) const;

        void setElasticity(Ogre::Real elasticity);

        Ogre::Real getElasticity(void) const;

        void setSurfaceThickness(Ogre::Real surfaceThickness);

        Ogre::Real getSurfaceThickness(void) const;

        void setCollideable(bool collideable);

        bool getCollideable(void) const;

        void setContactBehavior(const Ogre::String& contactBehavior);

        Ogre::String getContactBehavior(void) const;

        void setContactSpeed(Ogre::Real contactSpeed);

        Ogre::Real getContactSpeed(void) const;

        void setContactDirection(const Ogre::Vector3 contactDirection);

        Ogre::Vector3 getContactDirection(void) const;

        /**
         * @brief Sets the local axis (of category1's body, rotated by its current orientation) the
         *        one-way filter operates along. Only used when Contact Behavior is 'OneWay'.
         * @param[in] oneWayAxis 'Y' for the classic platform you can jump through from below and
         *        land on from above, 'X' for a one-way wall/corridor.
         */
        void setOneWayAxis(const Ogre::String& oneWayAxis);

        Ogre::String getOneWayAxis(void) const;

        /**
         * @brief Sets which direction along One Way Axis the moving object may pass through
         *        without colliding - the opposite direction always collides normally. Only used
         *        when Contact Behavior is 'OneWay'.
         * @param[in] oneWayAllowedDirection 'Positive' or 'Negative'
         */
        void setOneWayAllowedDirection(const Ogre::String& oneWayAllowedDirection);

        Ogre::String getOneWayAllowedDirection(void) const;

        /**
         * @brief Sets how steep a surface has to be before its tangent friction is removed.
         *
         * Applies to EVERY contact behavior of this material pair, not just one of them.
         * A character pressed against a wall is otherwise held up by the vertical tangent
         * friction of that contact and hangs in mid air instead of sliding down.
         *
         * @param[in] wallSlideAngle Minimum angle in degrees against the horizontal plane.
         *        0 switches the feature off entirely. 90 is a perfectly vertical wall, 68
         *        also covers typical platform edges, and walkable ramps stay below it and
         *        keep their friction.
         */
        void setWallSlideAngle(Ogre::Real wallSlideAngle);

        Ogre::Real getWallSlideAngle(void) const;

        void setOverlapFunctionName(const Ogre::String& overlapFunctionName);

        void setContactFunctionName(const Ogre::String& contactFunctionName);

        void setContactOnceFunctionName(const Ogre::String& contactOnceFunctionName);

        void setContactScratchFunctionName(const Ogre::String& contactScratchFunctionName);

        /*
         * @brief Sets the physics contact callback script file to react in script at the moment two game objects with physics components collide.
         * @param[in]	contactScriptFilePathName	The physics contact callback script file
         * @note			The file must have the following functions implemented and the module name must be called like the lua file without the lua extension. E.g. 'Contact.lua':
         *	module("Contact", package.seeall);
         *	function onAABBOverlap(gameObject0, gameObject1)
         *	end
         *
         *	function onContact(gameObject0, gameObject1, contact)
         *	end
         *
         *	function onContactOnce(gameObject0, gameObject1, contact)
         *	end
         */
    public:
        static const Ogre::String AttrCategory1(void)
        {
            return "Category 1";
        }
        static const Ogre::String AttrCategory2(void)
        {
            return "Category 2";
        }
        static const Ogre::String AttrFriction(void)
        {
            return "Friction";
        }
        static const Ogre::String AttrSoftness(void)
        {
            return "Softness";
        }
        static const Ogre::String AttrElasticity(void)
        {
            return "Elasticity";
        }
        static const Ogre::String AttrSurfaceThickness(void)
        {
            return "Surface Thickness";
        }
        static const Ogre::String AttrCollideable(void)
        {
            return "Colideable";
        }
        static const Ogre::String AttrContactBehavior(void)
        {
            return "Contact Behavior";
        }
        static const Ogre::String AttrContactSpeed(void)
        {
            return "Contact Speed";
        }
        static const Ogre::String AttrContactDirection(void)
        {
            return "Contact Direction";
        }
        static const Ogre::String AttrOneWayAxis(void)
        {
            return "One Way Axis";
        }
        static const Ogre::String AttrOneWayAllowedDirection(void)
        {
            return "One Way Allowed Direction";
        }
        static const Ogre::String AttrWallSlideAngle(void)
        {
            return "Wall Slide Angle";
        }
        static const Ogre::String AttrOverlapFunctionName(void)
        {
            return "Overlap Function Name";
        }
        static const Ogre::String AttrContactFunctionName(void)
        {
            return "Contact Function Name";
        }
        static const Ogre::String AttrContactOnceFunctionName(void)
        {
            return "Contact Once Function Name";
        }
        static const Ogre::String AttrContactScratchFunctionName(void)
        {
            return "Contact Scratch Function Name";
        }

    private:
        void createMaterialPair(void);

    private:
        Variant* category1;
        Variant* category2;
        Variant* friction;
        Variant* softness;
        Variant* elasticity;
        Variant* surfaceThickness;
        Variant* collideable;
        Variant* contactBehavior;
        Variant* contactSpeed;
        Variant* contactDirection;
        Variant* oneWayAxis;
        Variant* oneWayAllowedDirection;
        Variant* wallSlideAngle;
        Variant* overlapFunctionName;
        Variant* contactFunctionName;
        Variant* contactOnceFunctionName;
        Variant* contactScratchFunctionName;

        OgreNewt::World* ogreNewt;
        OgreNewt::MaterialPair* materialPair;
        ConveyorContactCallback* conveyorContactCallback;
        GenericContactCallback* genericContactCallback;
        OneWayContactCallback* oneWayContactCallback;
        WallSlideContactCallback* wallSlideContactCallback;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Disables the tangent friction of every contact steep enough to count as a wall.
     *
     * ND4 gives every contact point two tangent friction directions, m_dir0 and m_dir1,
     * spanning the plane perpendicular to the contact normal. For a wall normal of
     * (-1, 0, 0) that plane is Y/Z, so one of those directions points straight UP - a
     * character pressed against a wall is therefore held up by vertical friction and hangs
     * in mid air instead of sliding down.
     *
     * Floors and walkable ramps stay below the angle threshold and keep their friction, so
     * walking and standing are unaffected. Ceilings are skipped as well, so bumping your
     * head still stops you.
     *
     * @param[in] contactJoint    The contact joint to process.
     * @param[in] maxUpComponent  Cosine of the wall slide angle. A value of 1 or more means
     *                            the feature is switched off and nothing is touched.
     */
    void EXPORTED disableWallFriction(const OgreNewt::ContactJoint& contactJoint, Ogre::Real maxUpComponent);

    /**
     * @brief Converts a wall slide angle in degrees into the cosine the contact test uses.
     *        An angle of 0 switches the feature off by returning a value no normal can
     *        exceed the test for.
     */
    Ogre::Real EXPORTED wallSlideAngleToMaxUpComponent(Ogre::Real wallSlideAngle);

    /**
     * @class WallSlideContactCallback
     * @brief Does nothing but remove the tangent friction of wall-like contacts.
     *
     * Installed when the material pair has no other contact behavior selected. Without it
     * the friction removal would only ever run for conveyor, one way or lua driven pairs -
     * and a plain "Player vs Platform" pair, which is exactly the one that needs it, would
     * get no callback at all and the character would keep sticking to walls and ledges.
     */
    class WallSlideContactCallback : public OgreNewt::ContactCallback
    {
    public:
        WallSlideContactCallback(Ogre::Real wallSlideAngle) : OgreNewt::ContactCallback()
        {
            this->setWallSlideAngle(wallSlideAngle);
        }

        int onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex) override
        {
            return 1;
        }

        void contactsProcess(const OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex) override
        {
            disableWallFriction(contactJoint, this->maxUpComponent);
        }

        /**
         * @brief Sets the wall slide angle in degrees, 0 to switch the friction removal off.
         */
        void setWallSlideAngle(Ogre::Real wallSlideAngle)
        {
            this->maxUpComponent = wallSlideAngleToMaxUpComponent(wallSlideAngle);
        }

    private:
        // Cosine of the wall slide angle, so the per contact test is a plain dot product.
        Ogre::Real maxUpComponent = 2.0f;
    };

    class ConveyorContactCallback : public OgreNewt::ContactCallback
    {
    public:
        ConveyorContactCallback(Ogre::Real speed, const Ogre::Vector3& direction, int conveyorCategoryId, bool forPlayer);
        ~ConveyorContactCallback();

        int onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex);

        void contactsProcess(const OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex);

        void setContactSpeed(Ogre::Real contactSpeed);

        void setContactDirection(const Ogre::Vector3& contactDirection);

    public:
        /**
         * @brief Sets the wall slide angle in degrees, 0 to switch the friction removal off.
         */
        void setWallSlideAngle(Ogre::Real wallSlideAngle)
        {
            this->maxUpComponent = wallSlideAngleToMaxUpComponent(wallSlideAngle);
        }

    private:
        // Cosine of the wall slide angle, so the per contact test is a plain dot product.
        // Initialised above 1 so no normal can pass the test - the feature is off by default.
        Ogre::Real maxUpComponent = 2.0f;
        int conveyorCategoryId;
        Ogre::Real speed;
        Ogre::Vector3 direction;
        bool forPlayer;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @class OneWayContactCallback
     * @brief One-directional collision along a configurable local axis - e.g. a platform you can
     *        jump through from below and land on from above, or a one-way wall/corridor.
     *        Re-evaluated per contact, per frame, from the moving object's velocity at the contact
     *        point - see contactsProcess() for the full reasoning.
     */
    class OneWayContactCallback : public OgreNewt::ContactCallback
    {
    public:
        /**
         * @param[in] localAxis The axis, in the ONE-WAY BODY's own local space, this filter
         *            operates along. Ogre::Vector3::UNIT_Y for a classic jump-through-from-below
         *            platform, Ogre::Vector3::UNIT_X for a one-way wall/corridor.
         * @param[in] allowPositiveDirection If true, the moving object may pass through while its
         *            velocity at the contact point has a POSITIVE component along the platform's
         *            world-space axis (e.g. moving up, or moving right); moving the opposite way
         *            still collides normally. If false, the allowed direction is negative instead.
         * @param[in] oneWayCategoryId The category/type id of the body this filtering is anchored
         *            to - its OWN orientation is what localAxis gets rotated by, same as
         *            ConveyorContactCallback's conveyorCategoryId identifies the belt body.
         */
        OneWayContactCallback(const Ogre::Vector3& localAxis, bool allowPositiveDirection, int oneWayCategoryId);
        ~OneWayContactCallback();

        int onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex);

        void contactsProcess(const OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex);

        void setLocalAxis(const Ogre::Vector3& localAxis);

        void setAllowPositiveDirection(bool allowPositiveDirection);

    public:
        /**
         * @brief Sets the wall slide angle in degrees, 0 to switch the friction removal off.
         */
        void setWallSlideAngle(Ogre::Real wallSlideAngle)
        {
            this->maxUpComponent = wallSlideAngleToMaxUpComponent(wallSlideAngle);
        }

    private:
        // Cosine of the wall slide angle, so the per contact test is a plain dot product.
        // Initialised above 1 so no normal can pass the test - the feature is off by default.
        Ogre::Real maxUpComponent = 2.0f;
        Ogre::Vector3 localAxis;
        bool allowPositiveDirection;
        int oneWayCategoryId;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    class GenericContactCallback : public OgreNewt::ContactCallback
    {
    public:
        GenericContactCallback(LuaScript* luaScript, int firstObjectId, const Ogre::String& overlapFunctionName, const Ogre::String& contactFunctionName, const Ogre::String& contactOnceFunctionName, const Ogre::String& contactOnceScratchName);

        ~GenericContactCallback();

        int onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex) override;

        void contactsProcess(const OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex) override;

    public:
        /**
         * @brief Sets the wall slide angle in degrees, 0 to switch the friction removal off.
         */
        void setWallSlideAngle(Ogre::Real wallSlideAngle)
        {
            this->maxUpComponent = wallSlideAngleToMaxUpComponent(wallSlideAngle);
        }

    private:
        // Cosine of the wall slide angle, so the per contact test is a plain dot product.
        // Initialised above 1 so no normal can pass the test - the feature is off by default.
        Ogre::Real maxUpComponent = 2.0f;
        // Resolves the owning game object of a body, or nullptr if the body has none.
        // A body that cannot be cast to a physics component simply has no game object -
        // the normal case for e.g. ragdoll bones, which are plain bodies. The collision
        // itself is still valid.
        static GameObject* resolveGameObject(OgreNewt::Body* body);

        // Orders a body pair so that index 0 is always the one matching firstObjectId.
        void orderBodies(OgreNewt::Body* bodyA, OgreNewt::Body* bodyB, OgreNewt::Body*& outBody0, OgreNewt::Body*& outBody1) const;

    private:
        // All members below are IMMUTABLE after construction. ND4 spawns one worker thread
        // per CPU core (see World's SetThreadCount) and drives contact processing from all of
        // them, so anything written here at runtime would be a data race - the previous
        // gameObject0/1, body0/1 and lastNormalSpeed members were exactly that. They were only
        // ever used to smuggle values from onAABBOverlap() into contactsProcess(); the latter
        // now resolves everything it needs from the ContactJoint it is handed, which makes the
        // shared state unnecessary rather than merely synchronised. No locks required.
        const int firstObjectId;
        LuaScript* const luaScript;
        const Ogre::String overlapFunctionName;
        const Ogre::String contactFunctionName;
        const Ogre::String contactOnceFunctionName;
        const Ogre::String contactScratchFunctionName;
    };

}; // namespace end

#endif