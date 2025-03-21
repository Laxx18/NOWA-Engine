#ifndef PLAYER_CONTROLLER_COMPONENTS_H
#define PLAYER_CONTROLLER_COMPONENTS_H

#include "GameObjectComponent.h"
#include "ki/StateMachine.h"
#include "ki/MovingBehavior.h"
#include "utilities/IAnimationBlender.h"
#include "camera/CameraManager.h"
#include "modules/OgreALModule.h"
#include "modules/LuaScript.h"

namespace NOWA
{
	class PhysicsActiveComponent;
	class PhysicsRagDollComponent;
	class CameraBehaviorComponent;
	class InputDeviceComponent;
	class OgreRecastModule;

	class EXPORTED PlayerControllerComponent : public GameObjectComponent
	{
	public:
		friend class PathFollowState3D;
		typedef boost::shared_ptr<PlayerControllerComponent> PlayerControllerCompPtr;
	public:

		class EXPORTED AnimationBlenderObserver : public IAnimationBlender::IAnimationBlenderObserver
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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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

		IAnimationBlender* getAnimationBlender(void) const;

		PhysicsActiveComponent* getPhysicsComponent(void) const;

		PhysicsRagDollComponent* getPhysicsRagDollComponent(void) const;

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

		bool getIsFallen(void) const;

		void standUp(void);

		void reactOnAnimationFinished(luabind::object closureFunction, bool oneTime);
	protected:
		virtual void internalShowDebugData(void);
	private:
		void deleteDebugData(void);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrRotationSpeed(void) { return "Rotation Speed"; }
		static const Ogre::String AttrGoalRadius(void) { return "Goal Radius"; }
		static const Ogre::String AttrAnimationSpeed(void) { return "Anim Speed"; }
		static const Ogre::String AttrAcceleration(void) { return "Acceleration"; }
		static const Ogre::String AttrCategories(void) { return "Categories"; }
		static const Ogre::String AttrUseStandUp(void) { return "Use Standup"; }
	protected:
		Variant* activated;
		Variant* rotationSpeed;
		Variant* goalRadius;
		Variant* animationSpeed;
		Variant* acceleration;
		Variant* categories;
		Variant* useStandUp;
		std::vector<Variant*> animations;

		PhysicsActiveComponent* physicsActiveComponent;
		CameraBehaviorComponent* cameraBehaviorComponent;
		InputDeviceComponent* inputDeviceComponent;
		IAnimationBlender* animationBlender;
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

		Ogre::Real timeFallen;
		bool isFallen;
		Ogre::Real fallThreshold;
		Ogre::Real recoveryTime;

		AnimationBlenderObserver* animationBlenderObserver;
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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
	public:
		static const Ogre::String AttrJumpForce(void) { return "Jump Force"; }
		static const Ogre::String AttrDoubleJump(void) { return "Double Jump"; }
		static const Ogre::String AttrRunAfterWalkTime(void) { return "Run After Walk Time Sec"; }
		static const Ogre::String AttrFor2D(void) { return "For 2D"; }
		static const Ogre::String AttrAnimIdle1(void) { return "Anim Idle1"; }
		static const Ogre::String AttrAnimIdle2(void) { return "Anim Idle2"; }
		static const Ogre::String AttrAnimIdle3(void) { return "Anim Idle3"; }
		static const Ogre::String AttrAnimWalkNorth(void) { return "Anim Walk North"; }
		static const Ogre::String AttrAnimWalkSouth(void) { return "Anim Walk South"; }
		static const Ogre::String AttrAnimWalkWest(void) { return "Anim Walk West"; }
		static const Ogre::String AttrAnimWalkEast(void) { return "Anim Walk East"; }
		static const Ogre::String AttrAnimJumpStart(void) { return "Anim Jump Start"; }
		static const Ogre::String AttrAnimJumpWalk(void) { return "Anim Jump Walk"; }
		static const Ogre::String AttrAnimHighJumpEnd(void) { return "Anim High Jump End"; }
		static const Ogre::String AttrAnimJumpEnd(void) { return "Anim Jump End"; }
		static const Ogre::String AttrAnimRun(void) { return "Anim Run"; }
		static const Ogre::String AttrAnimSneak(void) { return "Anim Sneak"; }
		static const Ogre::String AttrAnimDuck(void) { return "Anim Duck"; }
	private:
		Variant* jumpForce;
		Variant* doubleJump;
		Variant* runAfterWalkTime;
		Variant* for2D;
		const unsigned short animationsCount = 14;
		KI::StateMachine<GameObject>* stateMachine;
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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		static const Ogre::String AttrStartStateName(void) { return "Start State Name"; }
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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		static const Ogre::String AttrCategories(void) { return "Categories"; }
		static const Ogre::String AttrRange(void) { return "Range"; }
		static const Ogre::String AttrPathSlot(void) { return "Path Slot"; }
		static const Ogre::String AttrAnimIdle1(void) { return "Anim Idle1"; }
		static const Ogre::String AttrAnimIdle2(void) { return "Anim Idle2"; }
		static const Ogre::String AttrAnimIdle3(void) { return "Anim Idle3"; }
		static const Ogre::String AttrAnimWalkNorth(void) { return "Anim Walk North"; }
		static const Ogre::String AttrAnimRun(void) { return "Anim Run"; }
		static const Ogre::String AttrAutoClick(void) { return "Auto Click"; }
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
	};

}; //namespace end

#endif