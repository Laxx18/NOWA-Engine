#ifndef AI_COMPONENTS_H
#define AI_COMPONENTS_H

#include "GameObjectComponent.h"
#include "ki/MovingBehavior.h"
#include "utilities/LuaObserver.h"

namespace NOWA
{
	using namespace KI;

	class EXPORTED AiComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<AiComponent> AiCompPtr;
	public:

		AiComponent();

		virtual ~AiComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) = 0;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AiComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::canStaticAddComponent
		 */
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist and no AiLuaComponent, because it has already a moving behavior, which can be used in lua script for all kinds of ai component behavior.";
		}

		void setRotationSpeed(Ogre::Real rotationSpeed);

		Ogre::Real getRotationSpeed(void) const;

		void setFlyMode(bool flyMode);

		bool getIsInFlyMode(void) const;

		void setStuckTime(Ogre::Real stuckTime);

		Ogre::Real getStuckeTime(void) const;

		void setAutoOrientation(bool autoOrientation);

		bool getIsAutoOrientated(void) const;

		void setAutoAnimation(bool autoAnimation);

		bool getIsAutoAnimated(void) const;

		boost::weak_ptr<NOWA::KI::MovingBehavior> getMovingBehavior(void) const;

		void reactOnPathGoalReached(luabind::object closureFunction);

		void reactOnAgentStuck(luabind::object closureFunction);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrRotationSpeed(void) { return "Rotation Speed"; }
		static const Ogre::String AttrFlyMode(void) { return "Fly Mode"; }
		static const Ogre::String AttrStuckTime(void) { return "Stuck Time"; }
		static const Ogre::String AttrAutoOrientation(void) { return "Auto Orientation"; }
		static const Ogre::String AttrAutoAnimation(void) { return "Auto Animation"; }
	protected:
		Variant* activated;
		Variant* rotationSpeed;
		Variant* flyMode; // not yet implemented checkbox whether to take y into account or not
		Variant* stuckTime;
		Variant* autoOrientation;
		Variant* autoAnimation;
		NOWA::KI::MovingBehavior::BehaviorType behaviorTypeId;
		boost::shared_ptr<NOWA::KI::MovingBehavior> movingBehaviorPtr;
		IPathGoalObserver* pathGoalObserver;
		IAgentStuckObserver* agentStuckObserver;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiMoveComponent : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiMoveComponent> AiMoveCompPtr;
	public:

		AiMoveComponent();

		virtual ~AiMoveComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

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
			return NOWA::getIdFromName("AiMoveComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiMoveComponent";
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
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Move an agent according the specified behavior. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.";
		}

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		void setBehaviorType(const Ogre::String& behaviorType);
		
		Ogre::String getBehaviorType(void) const;
 
		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;
	public:
		static const Ogre::String AttrBehaviorType(void) { return "Behavior"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
	private:
		Variant* behaviorType;
		Variant* targetId;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiMoveRandomlyComponent : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiMoveRandomlyComponent> AiMoveRandomlyCompPtr;
	public:

		AiMoveRandomlyComponent();

		virtual ~AiMoveRandomlyComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
			return NOWA::getIdFromName("AiMoveRandomlyComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiMoveRandomlyComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Move an agent randomly. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.";
		}
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiPathFollowComponent : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiPathFollowComponent> AiPathFollowCompPtr;
	public:

		AiPathFollowComponent();

		virtual ~AiPathFollowComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

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
			return NOWA::getIdFromName("AiPathFollowComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiPathFollowComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Move an agent according a specfied waypoint path. The waypoints are created through GameObjects with NodeComponents and can be linked by their GameObject Ids. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.";
		}

		void setWaypointsCount(unsigned int waypointsCount);

		unsigned int getWaypointsCount(void) const;

		void setWaypointId(unsigned int index, unsigned long id);

		void addWaypointId(unsigned long id);

		unsigned long getWaypointId(unsigned int index);

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void setDirectionChange(bool directionChange);

		bool getDirectionChange(void) const;

		void setInvertDirection(bool invertDirection);

		bool getInvertDirection(void) const;

		void setGoalRadius(Ogre::Real goalRadius);

		Ogre::Real getGoalRadius(void) const;
	public:
		static const Ogre::String AttrWaypointsCount(void) { return "Waypoints Count"; }
		static const Ogre::String AttrWaypoint(void) { return "Waypoint Id "; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrInvertDirection(void) { return "Invert Direction"; }
		static const Ogre::String AttrDirectionChange(void) { return "Direction Change"; }
		static const Ogre::String AttrGoalRadius(void) { return "Goal Radius"; }
	private:
		Variant* waypointsCount;
		std::vector<Variant*> waypoints;
		Variant* repeat;
		Variant* directionChange;
		Variant* invertDirection;
		Variant* goalRadius;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiWanderComponent : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiWanderComponent> AiWanderCompPtr;
	public:

		AiWanderComponent();

		virtual ~AiWanderComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
			return NOWA::getIdFromName("AiWanderComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiWanderComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Let an agent wander randomly. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.";
		}

		void setWanderJitter(Ogre::Real wanderJitter);

		Ogre::Real getWanderJitter(void) const;

		void setWanderRadius(Ogre::Real wanderRadius);

		Ogre::Real getWanderRadius(void) const;

		void setWanderDistance(Ogre::Real wanderDistance);

		Ogre::Real getWanderDistance(void) const;
	public:
		static const Ogre::String AttrWanderJitter(void) { return "Wander Jitter"; }
		static const Ogre::String AttrWanderRadius(void) { return "Wander Radius"; }
		static const Ogre::String AttrWanderDistance(void) { return "Wander Distance"; }
	private:
		Variant* wanderJitter;
		Variant* wanderRadius;
		Variant* wanderDistance;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiFlockingComponent : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiFlockingComponent> AiFlockingCompPtr;
	public:

		AiFlockingComponent();

		virtual ~AiFlockingComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		 * @see		GameObjectComponent::onCloned
		 */
		virtual bool onCloned(void) override;

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
			return NOWA::getIdFromName("AiFlockingComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiFlockingComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Move a group of agents according flocking behavior. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist and all the agents must belong to the same category.";
		}

		void setNeighborDistance(Ogre::Real neighborDistance);

		Ogre::Real getNeighborDistance(void) const;

		void setCohesionBehavior(bool cohesion);

		bool getCohesionBehavior(void) const;

		void setSeparationBehavior(bool separation);

		bool getSeparationBehavior(void) const;

		void setAlignmentBehavior(bool alignment);

		bool getAlignmentBehavior(void) const;

		void setBorderBehavior(bool border);

		bool getBorderBehavior(void) const;

		void setObstacleBehavior(bool obstacle);

		bool getObstacleBehavior(void) const;

		void setFleeBehavior(bool flee);

		bool getFleeBehavior(void) const;

		void setSeekBehavior(bool seek);

		bool getSeekBehavior(void) const;

		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;
	public:
		static const Ogre::String AttrNeighborDistance(void) { return "Neighbor Distance"; }
		static const Ogre::String AttrCohesion(void) { return "Cohesion"; }
		static const Ogre::String AttrSeparation(void) { return "Separation"; }
		static const Ogre::String AttrAlignment(void) { return "Alignment"; }
		static const Ogre::String AttrBorder(void) { return "Border"; }
		static const Ogre::String AttrObstacle(void) { return "Obstacle"; }
		static const Ogre::String AttrFlee(void) { return "Flee"; }
		static const Ogre::String AttrSeek(void) { return "Seek"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
	private:
		Variant* neighborDistance;
		Variant* cohesion;
		Variant* separation;
		Variant* alignment;
		Variant* border;
		Variant* obstacle;
		Variant* flee;
		Variant* seek;
		Variant* targetId;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiRecastPathNavigationComponent : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiRecastPathNavigationComponent> AiRecastPathNavigationCompPtr;
	public:

		AiRecastPathNavigationComponent();

		virtual ~AiRecastPathNavigationComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		 * @see		GameObjectComponent::onCloned
		 */
		virtual bool onCloned(void) override;

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
			return NOWA::getIdFromName("AiRecastPathNavigationComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiRecastPathNavigationComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Move an agent along an optimal path generated by navigation mesh. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist and a valid navigation mesh through OgreRecast.";
		}

		/**
		* @see		GameObjectComponent::showDebugData
		*/
		virtual void showDebugData(void) override;

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void setDirectionChange(bool directionChange);

		bool getDirectionChange(void) const;

		void setInvertDirection(bool invertDirection);

		bool getInvertDirection(void) const;

		void setGoalRadius(Ogre::Real goalRadius);

		Ogre::Real getGoalRadius(void) const;

		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;

		void setActualizePathDelaySec(Ogre::Real actualizePathDelay);

		Ogre::Real getActualizePathDelaySec(void) const;

		void setPathSlot(int pathSlot);

		int getPathSlot(void) const;

		void setPathTargetSlot(int targetSlot);

		int getPathTargetSlot(void) const;
	public:
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrInvertDirection(void) { return "Invert Direction"; }
		static const Ogre::String AttrDirectionChange(void) { return "Direction Change"; }
		static const Ogre::String AttrGoalRadius(void) { return "Goal Radius"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrActualizePathDelay(void) { return "Actualize Path Delay"; }
		static const Ogre::String AttrPathSlot(void) { return "Path Slot"; }
		static const Ogre::String AttrTargetSlot(void) { return "Target Slot"; }
	private:
		Variant* repeat;
		Variant* directionChange;
		Variant* invertDirection;
		Variant* goalRadius;
		Variant* targetId;
		Variant* actualizePathDelay;
		GameObject* targetGameObject;
		Variant* pathSlot;
		Variant* targetSlot;
		bool drawPath;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiObstacleAvoidanceComponent : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiObstacleAvoidanceComponent> AiObstacleAvoidanceCompPtr;
	public:

		AiObstacleAvoidanceComponent();

		virtual ~AiObstacleAvoidanceComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
			return NOWA::getIdFromName("AiObstacleAvoidanceComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiObstacleAvoidanceComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Move an agent and avoid obstacles. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.";
		}

		void setAvoidanceRadius(Ogre::Real avoidanceRadius);

		Ogre::Real getAvoidanceRadius(void) const;

		void setObstacleCategories(const Ogre::String& obstacleCategories);

		Ogre::String getObstacleCategories(void) const;
	public:
		static const Ogre::String AttrObstacleCategories(void) { return "Obstacle Categories"; }
		static const Ogre::String AttrAvoidanceRadius(void) { return "Avoidance Radius"; }
	private:
		Variant* avoidanceRadius;
		Variant* obstacleCategories;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiHideComponent : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiHideComponent> AiHideCompPtr;
	public:

		AiHideComponent();

		virtual ~AiHideComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

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
			return NOWA::getIdFromName("AiHideComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiHideComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Hides an agent against a target. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist and a valid target GameObject.";
		}

		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;

		void setObstacleRangeRadius(Ogre::Real obstacleRangeRadius);

		Ogre::Real getObstacleRangeRadius(void) const;

		void setObstacleCategories(const Ogre::String& obstacleCategories);

		Ogre::String getObstacleCategories(void) const;
	public:
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrObstacleCategories(void) { return "Obstacle Categories"; }
		static const Ogre::String AttrObstacleRangeRadius(void) { return "Obstacle Range Radius"; }
	private:
		Variant* targetId;
		Variant* obstacleRangeRadius;
		Variant* obstacleCategories;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiMoveComponent2D : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiMoveComponent2D> AiMoveComp2DPtr;
	public:

		AiMoveComponent2D();

		virtual ~AiMoveComponent2D();

		/**
		* @see		GameObjectComponent2D::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent2D::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent2D::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent2D::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent2D::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent2D::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent2D::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent2D::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AiMoveComponent2D");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiMoveComponent2D";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see		GameObjectComponent2D::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent2D::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent2D::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent2D::getInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Move an agent according the specified behavior. Note: Several AiComponent2Ds for an agent can be combined. Requirements: A kind of physics active component must exist.";
		}
		
		void setBehaviorType(const Ogre::String& behaviorType);
		
		Ogre::String getBehaviorType(void) const;
 
		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;
	public:
		static const Ogre::String AttrBehaviorType(void) { return "Behavior"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
	private:
		Variant* behaviorType;
		Variant* targetId;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiPathFollowComponent2D : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiPathFollowComponent2D> AiPathFollowComp2DPtr;
	public:

		AiPathFollowComponent2D();

		virtual ~AiPathFollowComponent2D();

		/**
		* @see		GameObjectComponent2D::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent2D::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent2D::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent2D::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent2D::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent2D::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent2D::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent2D::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AiPathFollowComponent2D");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiPathFollowComponent2D";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent2D::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent2D::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;
		
		/**
		 * @see	GameObjectComponent2D::getInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Move an agent according a specfied waypoint path. The waypoints are created through GameObjects with NodeComponent2Ds and can be linked by their GameObject Ids. Note: Several AiComponent2Ds for an agent can be combined. Requirements: A kind of physics active component must exist.";
		}

		void setWaypointsCount(unsigned int waypointsCount);

		unsigned int getWaypointsCount(void) const;

		void setWaypointId(unsigned int index, unsigned long id);

		void addWaypointId(unsigned long id);

		unsigned long getWaypointId(unsigned int index);

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void setDirectionChange(bool directionChange);

		bool getDirectionChange(void) const;

		void setInvertDirection(bool invertDirection);

		bool getInvertDirection(void) const;

		void setGoalRadius(Ogre::Real goalRadius);

		Ogre::Real getGoalRadius(void) const;
	public:
		static const Ogre::String AttrWaypointsCount(void) { return "Waypoints Count"; }
		static const Ogre::String AttrWaypoint(void) { return "Waypoint Id "; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrInvertDirection(void) { return "Invert Direction"; }
		static const Ogre::String AttrDirectionChange(void) { return "Direction Change"; }
		static const Ogre::String AttrGoalRadius(void) { return "Goal Radius"; }
	private:
		Variant* waypointsCount;
		std::vector<Variant*> waypoints;
		Variant* repeat;
		Variant* directionChange;
		Variant* invertDirection;
		Variant* goalRadius;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AiWanderComponent2D : public AiComponent
	{
	public:

		typedef boost::shared_ptr<AiWanderComponent2D> AiWanderComp2DPtr;
	public:

		AiWanderComponent2D();

		virtual ~AiWanderComponent2D();

		/**
		* @see		GameObjectComponent2D::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent2D::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent2D::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent2D::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent2D::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent2D::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent2D::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AiWanderComponent2D");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiWanderComponent2D";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent2D::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent2D::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent2D::getInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics active component must exist. "
				   "Usage: Let an agent wander randomly. Note: Several AiComponent2Ds for an agent can be combined. Requirements: A kind of physics active component must exist.";
		}

		void setWanderJitter(Ogre::Real wanderJitter);

		Ogre::Real getWanderJitter(void) const;

		void setWanderRadius(Ogre::Real wanderRadius);

		Ogre::Real getWanderRadius(void) const;

		void setWanderDistance(Ogre::Real wanderDistance);

		Ogre::Real getWanderDistance(void) const;
	public:
		static const Ogre::String AttrWanderJitter(void) { return "Wander Jitter"; }
		static const Ogre::String AttrWanderRadius(void) { return "Wander Radius"; }
		static const Ogre::String AttrWanderDistance(void) { return "Wander Distance"; }
	private:
		Variant* wanderJitter;
		Variant* wanderRadius;
		Variant* wanderDistance;
	};

}; //namespace end

#endif