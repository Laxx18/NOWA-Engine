#ifndef PLAYER_CONTROLLER_COMPONENTS_H
#define PLAYER_CONTROLLER_COMPONENTS_H

#include "GameObjectComponent.h"
#include "camera/CameraManager.h"
#include "ki/MovingBehavior.h"
#include "ki/StateMachine.h"
#include "modules/LuaScript.h"
#include "modules/OgreALModule.h"
#include "utilities/AnimationBlenderV2.h"

#include <map>
#include <set>

namespace NOWA
{
    class PhysicsActiveComponent;
    class PhysicsRagDollComponentV2;
    class CameraBehaviorComponent;
    class InputDeviceComponent;
    class OgreRecastModule;
    class LuaPlayerState;

    class EXPORTED PlayerControllerComponent : public GameObjectComponent
    {
    public:
        friend class PathFollowState3D;
        friend class WalkingStateJumpNRun;
        typedef boost::shared_ptr<PlayerControllerComponent> PlayerControllerCompPtr;

    public:
        class EXPORTED AnimationBlenderObserver : public AnimationBlenderV2::IAnimationBlenderObserver
        {
        public:
            AnimationBlenderObserver(luabind::object closureFunction, bool oneTime);

            virtual ~AnimationBlenderObserver();

            virtual void onAnimationFinished(void) override;

            virtual bool shouldReactOneTime(void) const override;

            void setNewFunctionName(luabind::object closureFunction, bool oneTime);

        private:
            luabind::object closureFunction;
            bool oneTime;
        };

        PlayerControllerComponent();

        virtual ~PlayerControllerComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

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

        /**
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void) override;

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PlayerControllerComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PlayerControllerComponent";
        }

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
            return "Requirements: A kind of physics component must exist.";
        }

        /**
         * @see		GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see		GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        virtual bool isActivated(void) const override;

        GameObject* getGameObject(void) const
        {
            return this->gameObjectPtr.get();
        }

        AnimationBlenderV2* getAnimationBlender(void) const;

        PhysicsActiveComponent* getPhysicsComponent(void) const;

        PhysicsRagDollComponentV2* getPhysicsRagDollComponent(void) const;

        void setRotationSpeed(Ogre::Real rotationSpeed);

        Ogre::Real getRotationSpeed(void) const;

        void setGoalRadius(Ogre::Real goalRadius);

        Ogre::Real getGoalRadius(void) const;

        void lockMovement(const Ogre::String& ownerName, bool lock);

        void setMoveWeight(Ogre::Real moveWeight);

        Ogre::Real getMoveWeight(void) const;

        void setJumpWeight(Ogre::Real jumpWeight);

        Ogre::Real getJumpWeight(void) const;

        void setIdle(bool idle);

        bool isIdle(void) const;

        void setAnimationSpeed(Ogre::Real animationSpeed);

        Ogre::Real getAnimationSpeed(void) const;

        void setAcceleration(Ogre::Real acceleration);

        Ogre::Real getAcceleration(void) const;

        void setCategories(const Ogre::String& categories);

        Ogre::String getCategories(void) const;

        void setUseStandUp(bool useStandUp);

        bool getUseStandUp(void) const;

        void setAnimationName(const Ogre::String& name, unsigned int index);

        Ogre::String getAnimationName(unsigned int index);

        CameraBehaviorComponent* getCameraBehaviorComponent(void) const;

        InputDeviceComponent* getInputDeviceComponent(void) const;

        Ogre::Real getHeight(void) const;

        Ogre::Vector3 getNormal(void) const;

        Ogre::Real getSlope(void) const;

        GameObject* getHitGameObjectBelow(void) const;

        GameObject* getHitGameObjectFront(void) const;

        GameObject* getHitGameObjectUp(void) const;

        Ogre::Vector3 getFrontNormal(void) const;

        bool getIsFallen(void) const;

        void standUp(void);

        void setUseWallSeparationMode(bool useWallSeparationMode);

        bool getUseWallSeparationMode(void) const;

        void reactOnAnimationFinished(luabind::object closureFunction, bool oneTime);

        /**
         * @brief Gets the wall normal currently blocking the player, or ZERO when nothing is
         *        in the way. Fed by the physics contact callback, not by rays.
         */
        Ogre::Vector3 getBlockedWallNormal(void) const;

        /**
         * @brief Sets the lua closure called whenever the player touches a wall.
         *
         * The closure receives the game object that was hit and the horizontal wall normal.
         * It is called regardless of 'Use Wall Separation Mode': that flag only decides
         * whether the BUILT-IN reaction runs, which cancels the movement input pointing into
         * the wall. Switch it off to handle the contact entirely in lua - for a metroid
         * style ledge grab, say - and leave it on for the plain "cannot walk into walls"
         * behavior.
         *
         * @param[in] closureFunction The closure to set. Calling this again REPLACES the
         *        previous one, so it is safe to call from a per frame script function.
         */
        void reactOnWallContact(luabind::object closureFunction);

        /**
         * @brief Sets which input action counts as the "action" key, e.g. NOWA_A_ATTACK_1.
         *
         * There is deliberately no default: the action id is handed in from lua, so the key
         * can be remapped per game without touching the engine. A negative value switches
         * the whole detection off, which is also the initial state.
         *
         * @param[in] actionId The action id, taken from the NOWA_A_... constants.
         */
        void setActionKey(int actionId);

        int getActionKey(void) const;

        /**
         * @brief Sets the lua closure called on the RISING EDGE of the action key.
         *
         * The closure receives the game object currently in front of the player, or nil when
         * there is nothing. Deciding WHAT that object is - a rope, a lever, a boulder - is
         * left to the script: the engine only reports what the front rays found.
         *
         * @param[in] closureFunction The closure to set. Calling this again REPLACES the
         *        previous one.
         */
        void reactOnActionPressed(luabind::object closureFunction);

        /**
         * @brief Remembers the game object the player is currently interacting with.
         *
         * A state entered from lua - climbing a rope, pushing a boulder - needs to know WHICH
         * object it is working on, and a state has no arguments. The id is stored rather than
         * the pointer, so a deleted game object reports itself as nil instead of dangling.
         *
         * @param[in] gameObject The game object, or nullptr to clear it.
         */
        void setInteractionGameObject(GameObject* gameObject);

        GameObject* getInteractionGameObject(void) const;

    protected:
        /**
         * @brief Fires the action key closure on the rising edge. Called from update().
         */
        void internalHandleActionKey(void);

    protected:
        virtual void internalShowDebugData(void);

    private:
        void deleteDebugData(void);

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrRotationSpeed(void)
        {
            return "Rotation Speed";
        }
        static const Ogre::String AttrGoalRadius(void)
        {
            return "Goal Radius";
        }
        static const Ogre::String AttrAnimationSpeed(void)
        {
            return "Anim Speed";
        }
        static const Ogre::String AttrAcceleration(void)
        {
            return "Acceleration";
        }
        static const Ogre::String AttrCategories(void)
        {
            return "Categories";
        }
        static const Ogre::String AttrUseStandUp(void)
        {
            return "Use Standup";
        }
        static const Ogre::String AttrWallSeparationMode(void)
        {
            return "Use Wall Separation Mode";
        }

    protected:
        Variant* activated;
        Variant* rotationSpeed;
        Variant* goalRadius;
        Variant* animationSpeed;
        Variant* acceleration;
        Variant* categories;
        Variant* useStandUp;
        Variant* useWallSeparationMode;
        std::vector<Variant*> animations;

        PhysicsActiveComponent* physicsActiveComponent;
        CameraBehaviorComponent* cameraBehaviorComponent;
        InputDeviceComponent* inputDeviceComponent;
        AnimationBlenderV2* animationBlender;
        Ogre::Real moveWeight;
        Ogre::String moveLockOwner;
        Ogre::Real jumpWeight;
        Ogre::String jumpWeightOwner;
        Ogre::Real height;
        Ogre::Real priorValidHeight;
        Ogre::Vector3 normal;
        Ogre::Vector3 priorValidNormal;
        Ogre::Real slope;
        unsigned int categoriesId;

        bool idle;
        bool canMove;
        bool canJump;
        GameObject* hitGameObjectBelow;
        GameObject* hitGameObjectFront;
        GameObject* hitGameObjectUp;
        Ogre::Vector3 frontNormal;

        // Wall normal reported by the physics CONTACT callback, so no extra rays are needed
        // to know that a wall is in the way. The walking state uses it to drop the part of
        // the movement input pointing into that wall: holding the right arrow key against a
        // right hand wall then produces no force at all, instead of pressing into the wall
        // and being fought with an opposing separation force afterwards.
        Ogre::Vector3 blockedWallNormal;
        // Fired on every wall contact, no matter what useWallSeparationMode is set to, so a
        // script can implement its own reaction - a ledge grab that keeps the player hanging
        // and lets him push off again, for instance.
        luabind::object wallContactClosureFunction;
        // Contacts arrive deferred on the logic thread, so the normal is kept alive for a
        // short moment rather than only for the exact frame it came in.
        Ogre::Real blockedWallTimer;

        // Action key handling. 'actionId' is negative while no key has been assigned from
        // lua, which switches the detection off entirely.
        int actionId;
        bool actionKeyWasDown;
        luabind::object actionPressedClosureFunction;
        // Stored as an ID on purpose: a game object can be deleted while a state is still
        // running, and a raw pointer would dangle until the state notices.
        unsigned long interactionGameObjectId;

        Ogre::Real timeFallen;
        bool isFallen;
        Ogre::Real fallThreshold;
        Ogre::Real recoveryTime;

        Ogre::SceneNode* debugWaypointNode;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED PlayerControllerJumpNRunComponent : public PlayerControllerComponent
    {
    public:
        typedef boost::shared_ptr<PlayerControllerJumpNRunComponent> PlayerControllerJumpNRunCompPtr;

    public:
        PlayerControllerJumpNRunComponent();

        virtual ~PlayerControllerJumpNRunComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

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

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PlayerControllerJumpNRunComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PlayerControllerJumpNRunComponent";
        }

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
            return "Usage: A player controller helper for Jump 'n' Run player movement. "
                   "Requirements: A kind of physics component must exist.";
        }

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see		GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        KI::StateMachine<GameObject>* getStateMaschine(void) const;

        void setJumpForce(Ogre::Real jumpForce);

        Ogre::Real getJumpForce(void) const;

        void setDoubleJump(bool doubleJump);

        bool getDoubleJump(void) const;

        void setRunAfterWalkTime(Ogre::Real runAfterWalkTime);

        Ogre::Real getRunAfterWalkTime(void) const;

        void setFor2D(bool for2D);

        bool getIsFor2D(void) const;

        /**
         * @brief		Sets whether the player may jump an unlimited number of times while
         *				in the air (metroid style). Overrides 'Double Jump' when enabled.
         * @param[in]	xJump	true to allow unlimited air jumps
         */
        void setXJump(bool xJump);

        bool getXJump(void) const;

        /**
         * @brief		Sets whether the player accelerates from the physics component's
         *				getSpeed() up to its getMaxSpeed() while running without interruption.
         * @param[in]	useAcceleration	true to enable the speed ramp
         */
        void setUseAcceleration(bool useAcceleration);

        bool getUseAcceleration(void) const;

        /**
         * @brief		Sets how many seconds of uninterrupted running it takes to reach
         *				getMaxSpeed(). The ramp is reset by a direction change or by hitting
         *				something in front, but deliberately NOT by jumping.
         * @param[in]	accelerationDuration	The duration in seconds
         */
        void setAccelerationDuration(Ogre::Real accelerationDuration);

        Ogre::Real getAccelerationDuration(void) const;

        /**
         * @brief		Lua closure called when the player reverses direction in 2D mode.
         *				Receives the old and the new direction as numbers (see the Direction enum).
         */
        void reactOnDirectionChanged(luabind::object closureFunction);

        /**
         * @brief		Lua closure called on every jump. Receives the current jump count,
         *				1 for the jump off the ground, 2 and above for air jumps.
         */
        void reactOnJump(luabind::object closureFunction);

        /**
         * @brief		Lua closure called when the player touches the ground again.
         *				Receives how long the player had been falling, in seconds.
         */
        void reactOnLand(luabind::object closureFunction);

        /**
         * @brief		Lua closure called whenever the acceleration ramp changes. Receives the
         *				current speed and the maximum speed, so a script can drive dust
         *				particles, camera shake or a speed HUD.
         */
        void reactOnAccelerationChanged(luabind::object closureFunction);

        luabind::object getDirectionChangedClosure(void) const;

        luabind::object getJumpClosure(void) const;

        luabind::object getLandClosure(void) const;

        luabind::object getAccelerationChangedClosure(void) const;

        /**
         * @brief Requests a state change by name, applied at the TOP of the next update.
         *
         * Deliberately NOT immediate. A state change is usually triggered from a lua closure,
         * and those closures run from an area of interest, a contact callback or an attribute
         * change - that is, potentially from inside the very update() of the state that is
         * about to be exited. Switching right there would run exit() on a state that is still
         * in the middle of its own update. Queueing it removes that class of bug entirely.
         *
         * The name may address a C++ state registered with the state machine, or a lua state
         * registered with registerLuaState(). Both live in the SAME state machine, so there
         * is exactly one current state at any time.
         *
         * @param[in] stateName The state to switch to.
         */
        void requestState(const Ogre::String& stateName);

        /**
         * @brief Makes a lua state table addressable by name.
         *
         * The table is the same shape the lua state machine has always used: optional
         * 'enter(gameObject)', 'execute(gameObject, dt)' and 'exit(gameObject)' entries.
         * Registering does NOT switch to the state, call requestState() for that.
         * Registering a name twice just swaps the table, which is what a script reload
         * needs - the instance the state machine may currently point at stays valid.
         *
         * @param[in] stateName  The name the state is addressed by.
         * @param[in] stateTable The lua table holding enter / execute / exit.
         */
        void registerLuaState(const Ogre::String& stateName, luabind::object stateTable);

        /**
         * @brief Requests the state that was active before the current one. Does nothing
         *        when there is none yet.
         */
        void requestPreviousState(void);

        Ogre::String getCurrentStateName(void) const;

        Ogre::String getPreviousStateName(void) const;

        bool isInState(const Ogre::String& stateName) const;

        /**
         * @brief Lua closure called after every state change. Receives the old and the new
         *        state name, both as strings.
         */
        void reactOnStateChanged(luabind::object closureFunction);

        /**
         * @brief Requests a state that runs IN PARALLEL to the current one.
         *
         * The state machine updates the child state first and the current state afterwards,
         * so an attack can play while the walking state keeps driving movement, input and
         * animation. That is the difference to requestState(), which REPLACES the current
         * state and therefore stops the player dead.
         *
         * A child state should not fight the running state: leave the velocity, the facing
         * and addTime() to it and only do what is genuinely on top - the attack animation,
         * a timer, an event. Requesting a child state while one is already running ends the
         * old one properly first.
         *
         * Applied at the top of the next update, exactly like requestState().
         *
         * @param[in] stateName The state to run in parallel.
         */
        void requestChildState(const Ogre::String& stateName);

        /**
         * @brief Ends the running child state. Does nothing when there is none.
         */
        void requestEndChildState(void);

        Ogre::String getCurrentChildStateName(void) const;

    public:
        static const Ogre::String AttrJumpForce(void)
        {
            return "Jump Force";
        }
        static const Ogre::String AttrDoubleJump(void)
        {
            return "Double Jump";
        }
        static const Ogre::String AttrRunAfterWalkTime(void)
        {
            return "Run After Walk Time Sec";
        }
        static const Ogre::String AttrFor2D(void)
        {
            return "For 2D";
        }
        static const Ogre::String AttrXJump(void)
        {
            return "X Jump";
        }
        static const Ogre::String AttrUseAcceleration(void)
        {
            return "Use Acceleration";
        }
        static const Ogre::String AttrAccelerationDuration(void)
        {
            return "Acceleration Duration Sec";
        }
        static const Ogre::String AttrAnimAirJump(void)
        {
            return "Anim Air Jump";
        }
        static const Ogre::String AttrAnimIdle1(void)
        {
            return "Anim Idle1";
        }
        static const Ogre::String AttrAnimIdle2(void)
        {
            return "Anim Idle2";
        }
        static const Ogre::String AttrAnimIdle3(void)
        {
            return "Anim Idle3";
        }
        static const Ogre::String AttrAnimWalkNorth(void)
        {
            return "Anim Walk North";
        }
        static const Ogre::String AttrAnimWalkSouth(void)
        {
            return "Anim Walk South";
        }
        static const Ogre::String AttrAnimWalkWest(void)
        {
            return "Anim Walk West";
        }
        static const Ogre::String AttrAnimWalkEast(void)
        {
            return "Anim Walk East";
        }
        static const Ogre::String AttrAnimJumpStart(void)
        {
            return "Anim Jump Start";
        }
        static const Ogre::String AttrAnimJumpWalk(void)
        {
            return "Anim Jump Walk";
        }
        static const Ogre::String AttrAnimHighJumpEnd(void)
        {
            return "Anim High Jump End";
        }
        static const Ogre::String AttrAnimJumpEnd(void)
        {
            return "Anim Jump End";
        }
        static const Ogre::String AttrAnimRun(void)
        {
            return "Anim Run";
        }
        static const Ogre::String AttrAnimSneak(void)
        {
            return "Anim Sneak";
        }
        static const Ogre::String AttrAnimDuck(void)
        {
            return "Anim Duck";
        }

    private:
        Variant* jumpForce;
        Variant* doubleJump;
        Variant* runAfterWalkTime;
        Variant* for2D;
        Variant* xJump;
        Variant* useAcceleration;
        Variant* accelerationDuration;

        luabind::object directionChangedClosureFunction;
        luabind::object jumpClosureFunction;
        luabind::object landClosureFunction;
        luabind::object accelerationChangedClosureFunction;

        // 15 instead of 14: ANIM_SALTO was added for the air jump, so a metroid style
        // multi jump can play a different animation than the jump off the ground.
        const unsigned short animationsCount = 15;
        KI::StateMachine<GameObject>* stateMachine;

        // Lua states are owned HERE, not by the state machine. StateMachine::registerState
        // can only default construct its instances, which a state carrying a lua table
        // cannot be - but changeState() also accepts a raw IState pointer, and that is the
        // door these go through. The machine therefore still does all the exit / enter
        // pairing, and there is only ever one state machine.
        std::map<Ogre::String, LuaPlayerState*> luaStates;
        // Names of the states registered with the state machine itself, so requestState()
        // can report an unknown name instead of tripping the assert inside changeState().
        std::set<Ogre::String> cppStateNames;

        Ogre::String currentStateName;
        Ogre::String previousStateName;
        Ogre::String requestedStateName;
        bool hasStateRequest;
        luabind::object stateChangedClosureFunction;

        // Child state: runs in parallel to the current state, see requestChildState().
        Ogre::String currentChildStateName;
        Ogre::String requestedChildStateName;
        bool hasChildStateRequest;

        void internalApplyStateRequest(void);
        void internalApplyChildStateRequest(void);
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED PlayerControllerJumpNRunLuaComponent : public PlayerControllerComponent
    {
    public:
        typedef boost::shared_ptr<PlayerControllerJumpNRunLuaComponent> PlayerControllerJumpNRunLuaCompPtr;

    public:
        PlayerControllerJumpNRunLuaComponent();

        virtual ~PlayerControllerJumpNRunLuaComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

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

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PlayerControllerJumpNRunLuaComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PlayerControllerJumpNRunLuaComponent";
        }

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
            return "Usage: A player controller helper for Jump 'n' Run player movement in a lua script. "
                   "Requirements: A kind of physics component must exist.";
        }

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see		GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        void setStartStateName(const Ogre::String& startStateName);

        Ogre::String getStartStateName(void) const;

        NOWA::KI::LuaStateMachine<GameObject>* getStateMachine(void) const;

    public:
        static const Ogre::String AttrStartStateName(void)
        {
            return "Start State Name";
        }

    private:
        void handleLuaScriptConnected(NOWA::EventDataPtr eventData);

    private:
        NOWA::KI::LuaStateMachine<GameObject>* luaStateMachine;
        Variant* startStateName;
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED PlayerControllerClickToPointComponent : public PlayerControllerComponent
    {
    public:
        typedef boost::shared_ptr<PlayerControllerClickToPointComponent> PlayerControllerPointToClickCompPtr;

    public:
        PlayerControllerClickToPointComponent();

        virtual ~PlayerControllerClickToPointComponent();

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

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
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PlayerControllerClickToPointComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PlayerControllerClickToPointComponent";
        }

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
            return "Usage: A player controller helper for click to point player movement. "
                   "Requirements: A kind of physics component must exist.";
        }

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see		GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        KI::StateMachine<GameObject>* getStateMachine(void) const;

        NOWA::KI::MovingBehavior* getMovingBehavior(void) const;

        void setCategories(const Ogre::String& categories);

        Ogre::String getCategories(void) const;

        unsigned int getCategoriesId(void) const;

        void setRange(Ogre::Real range);

        Ogre::Real getRange(void) const;

        void setPathSlot(int pathSlot);

        int getPathSlot(void) const;

        void setAutoClick(bool autoClick);

        bool getAutoClick(void) const;

        bool getDrawPath(void) const;

        Ogre::RaySceneQuery* getRaySceneQuery(void) const;

    public:
        static const Ogre::String AttrCategories(void)
        {
            return "Categories";
        }
        static const Ogre::String AttrRange(void)
        {
            return "Range";
        }
        static const Ogre::String AttrPathSlot(void)
        {
            return "Path Slot";
        }
        static const Ogre::String AttrAnimIdle1(void)
        {
            return "Anim Idle1";
        }
        static const Ogre::String AttrAnimIdle2(void)
        {
            return "Anim Idle2";
        }
        static const Ogre::String AttrAnimIdle3(void)
        {
            return "Anim Idle3";
        }
        static const Ogre::String AttrAnimWalkNorth(void)
        {
            return "Anim Walk North";
        }
        static const Ogre::String AttrAnimRun(void)
        {
            return "Anim Run";
        }
        static const Ogre::String AttrAutoClick(void)
        {
            return "Auto Click";
        }

    protected:
        virtual void internalShowDebugData(void) override;

    private:
        const unsigned short animationsCount = 5;
        Variant* range;
        Variant* pathSlot;
        Variant* autoClick;
        unsigned int categoriesId;
        KI::StateMachine<GameObject>* stateMachine;
        boost::shared_ptr<NOWA::KI::MovingBehavior> movingBehaviorPtr;
        bool drawPath;
        Ogre::RaySceneQuery* raySceneQuery;
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class PlayerControllerJumpNRunComponent;

    // Realy important: Since this is a bidirectional association between PlayerControllerJumpNRunComponent and the States, the PlayerControllerJumpNRunComponent usage like PlayerControllerJumpNRunComponent->method
    // must be hidden in the source cpp file in order to prevent compiling errors

    enum class Direction
    {
        NONE = 0,
        RIGHT = 1,
        LEFT = 2,
        UP = 3,
        DOWN = 4
    };

    //---------------------------LuaPlayerState-------------------

    /**
     * @brief A state of the player's state machine whose behavior lives in a lua table.
     *
     * This is the bridge that lets hand written C++ states (the locomotion, which has to run
     * at logic rate and touches the velocity servo, the slope projection and the animation
     * blender gate) and script authored states (attacking, a portal, a cutscene, a ragdoll
     * timeout) sit in ONE state machine instead of two competing ones.
     *
     * The table has the same shape the lua state machine has always used:
     * 'enter(gameObject)', 'execute(gameObject, dt)' and 'exit(gameObject)'. All three are
     * optional - a missing entry is simply skipped.
     */
    class EXPORTED LuaPlayerState : public NOWA::KI::IState<GameObject>
    {
    public:
        LuaPlayerState();

        virtual ~LuaPlayerState();

    public:
        static Ogre::String getName(void)
        {
            return "LuaPlayerState";
        }

        void setStateName(const Ogre::String& stateName);

        Ogre::String getStateName(void) const;

        /**
         * @brief Sets (or replaces) the lua table backing this state.
         *
         * An invalid object switches the state off without destroying it, which is what
         * disconnect() does: the lua environment is gone by then, but the state machine may
         * still hold this pointer.
         */
        void setStateTable(luabind::object stateTable);

        bool hasStateTable(void) const;

        virtual void enter(GameObject* player) override;

        virtual void update(GameObject* player, Ogre::Real dt) override;

        virtual void exit(GameObject* player) override;

    private:
        void callStateFunction(const Ogre::String& functionName, GameObject* player);

        luabind::object stateTable;
        Ogre::String stateName;
    };

    //---------------------------WalkingStateJumpNRun-------------------

    class EXPORTED WalkingStateJumpNRun : public NOWA::KI::IState<GameObject>
    {
    public:
        WalkingStateJumpNRun();

        virtual ~WalkingStateJumpNRun();

    public:
        static Ogre::String getName(void)
        {
            return "WalkingStateJumpNRun";
        }

        virtual void enter(GameObject* player) override;

        virtual void update(GameObject* player, Ogre::Real dt) override;

        virtual void exit(GameObject* player) override;

    private:
        PlayerControllerJumpNRunComponent* playerController;
        Direction direction;
        bool directionChanged;
        bool isJumping;
        bool isAttacking;
        bool isOnRope;
        Direction oldDirection;
        Ogre::Real jumpForce;
        Ogre::Vector3 keyDirection;
        Ogre::Real boringTimer;
        Ogre::Real noMoveTimer;
        bool inAir;
        bool tryJump;
        bool highFalling;
        bool jumpKeyPressed;
        unsigned int jumpCount;
        bool canDoubleJump;
        Ogre::Real walkCount;
        bool hasPhysicsPlayerControllerComponent;
        Ogre::Real acceleration;
        // Seconds of uninterrupted running, drives the getSpeed() -> getMaxSpeed() ramp.
        // Reset on a direction change and on hitting something in front, but NOT on jumping.
        Ogre::Real accelerationTimer;
        Ogre::Real lastReportedSpeed;
        // How long the player has been falling, handed to the land closure on touchdown.
        Ogre::Real fallTimer;
        bool groundedOnce;
        bool duckedOnce;
        OgreAL::Sound* walkSound;
        OgreAL::Sound* jumpSound;
        bool hasInputDevice;
        Ogre::SceneManager* sceneManager;
    };

    //---------------------------PathFollowState3D-------------------

    class PlayerControllerClickToPointComponent;

    class EXPORTED PathFollowState3D : public NOWA::KI::IState<GameObject>
    {
    public:
        PathFollowState3D();

        virtual ~PathFollowState3D();

    public:
        static Ogre::String getName(void)
        {
            return "PathFollowState3D";
        }

        virtual void enter(GameObject* player) override;

        virtual void update(GameObject* player, Ogre::Real dt) override;

        virtual void exit(GameObject* player) override;

    private:
        PlayerControllerClickToPointComponent* playerController;
        Ogre::Real animationSpeed;
        Ogre::Real boringTimer;
        NOWA::KI::MovingBehavior* movingBehavior;
        bool hasGoal;
        Ogre::RaySceneQuery* raySceneQuery;
        OgreRecastModule* ogreRecastModule;
        bool canClick;
        int mouseX;
        int mouseY;
        Ogre::Real maxHeightDifference;
        bool middleButtonWasDown;
        int lastPathMouseX;
        int lastPathMouseY;
        bool pendingPath;
        Ogre::Vector3 pendingPosOnNavMesh;
        Ogre::Real raycastThrottleTimer;
        Ogre::Real raycastThrottleInterval;
        Ogre::Vector3 lastKnownClickPosition;
        bool hasLastKnownClickPosition;
    };

}; // namespace end

#endif