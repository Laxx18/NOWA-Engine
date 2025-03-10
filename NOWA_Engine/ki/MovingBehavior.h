#ifndef MOVING_BEHAVIOR_H
#define MOVING_BEHAVIOR_H

#include "defines.h"
#include "Path.h"
#include "utilities/LuaObserver.h"

namespace NOWA
{
	class GameObject;
	class PhysicsActiveComponent;
	class IAnimationBlender;
	class CrowdComponent;

	typedef boost::shared_ptr<GameObject> GameObjectPtr;

	namespace KI
	{
		class EXPORTED MovingBehavior
		{
		public:

			enum BehaviorType
			{
				STOP = 1 << 0, 
				NONE = 1 << 1, // If none is on, add no force, so that other behaviors can still move the agent! Only stop adds force, even if its null
				MOVE = 1 << 2,
				MOVE_RANDOMLY = 1 << 3,
				SEEK = 1 << 4,
				FLEE = 1 << 5,
				ARRIVE = 1 << 6,
				WANDER = 1 << 7,
				PATH_FINDING_WANDER = 1 << 8,
				PURSUIT = 1 << 9,
				OFFSET_PURSUIT = 1 << 10,
				EVADE = 1 << 11,
				HIDE = 1 << 12,
				FOLLOW_PATH = 1 << 13,
				OBSTACLE_AVOIDANCE = 1 << 14,
				INTERPOSE = 1 << 15,
				FLOCKING_COHESION = 1 << 16,
				FLOCKING_SEPARATION = 1 << 17,
				FLOCKING_SPREAD = 1 << 18,
				FLOCKING_FORMATION_V_SHAPE = 1 << 19,
				FLOCKING_ALIGNMENT = 1 << 20,
				FLOCKING_OBSTACLE_AVOIDANCE = 1 << 21,
				FLOCKING_FLEE = 1 << 22,
				FLOCKING_SEEK = 1 << 23,
				FLOCKING = 1 << 24,
				SEEK_2D = 1 << 25,
				FLEE_2D = 1 << 26,
				ARRIVE_2D = 1 << 27,
				PATROL_2D = 1 << 28,
				WANDER_2D = 1 << 29,
				FOLLOW_PATH_2D = 1 << 30,
				PURSUIT_2D = 1 << 31,
				FOLLOW_TRACE_2D = 1 << 32
			};

			//Arrive makes use of these to determine how quickly a GameObject
			//should decelerate to its target
			enum eDeceleration
			{
				SLOW = 3,
				NORMAL = 2,
				FAST = 1
			};
		public:
			MovingBehavior(unsigned long agentId);

			virtual ~MovingBehavior();

			void setRotationSpeed(Ogre::Real rotationSpeed);

			//this function tests if a specific bit of mask is set
			bool isSwitchOn(BehaviorType behaviorType);
			// For pursuit etc.
			void setTargetAgentId(unsigned long targetAgentId);
			// For interpose
			void setTargetAgentId2(unsigned long targetAgentId2);

			void setAgentId(unsigned long agentId);

			void setDeceleration(eDeceleration deceleration);

			GameObject* getTargetAgent(void) const;

			GameObject* getTargetAgent2(void) const;
			// does not work somehow
			//eDeceleration getDeceleration(void) const;

			void setWanderJitter(Ogre::Real wanderJitter);
			void setWanderRadius(Ogre::Real wanderRadius);
			void setWanderDistance(Ogre::Real wanderDistance);

			void setGoalRadius(Ogre::Real goalRadius);

			Ogre::Real getGoalRadius(void) const;

			void setDeceleration(Ogre::Real deceleration);

			void setFlyMode(bool flyMode);

			bool isInFlyMode(void) const;

			void setObstacleHideData(const Ogre::String& obstaclesHideCategories, Ogre::Real obstacleRangeRadius);

			void setObstacleAvoidanceData(const Ogre::String& obstaclesAvoidanceCategories, Ogre::Real obstacleRangeRadius);

			void setNeighborDistance(Ogre::Real neighborDistance);

			Ogre::Real getNeighborDistance(void) const;

			void setFlockingAgents(const std::vector<unsigned long>& flockingAgentids);

			/**
			 * @brief Sets and actives the given behavior
			 * @param behavior The behavior to set
			 * @note If any behavior does exist already, it will be reset prior.
			 */
			void setBehavior(BehaviorType behaviorType);

			/**
			 * @brief Adds and actived the given behavior additionally to prior set behaviors
			 * @param behavior The behavior to add
			 * @note If any behavior does exist already, this behavior combines prior set behaviors.
			 */
			void addBehavior(BehaviorType behaviorType);

			/**
			* @brief Removes and deactivates the given behavior
			* @param behavior The behavior to remove
			* @note If there are more than one behavior active, this one will be deactivated, but the other ones will remain active.
			*/
			void removeBehavior(BehaviorType behaviorType);
			
			/**
			 * @brief Sets and actives the given behavior
			 * @param behavior The behavior to set
			 * @note If any behavior does exist already, it will be reset prior.
			 */
			void setBehavior(const Ogre::String& behavior);

			/**
			 * @brief Adds and actived the given behavior additionally to prior set behaviors
			 * @param behavior The behavior to add
			 * @note If any behavior does exist already, this behavior combines prior set behaviors.
			 */
			void addBehavior(const Ogre::String& behavior);

			/**
			 * @brief Removes and deactivates the given behavior
			 * @param behavior The behavior to remove
			 * @note If there are more than one behavior active, this one will be deactivated, but the other ones will remain active.
			 */
			void removeBehavior(const Ogre::String& behavior);

			/**
			 * @brief Gets the behavior id from the given behavior string
			 * @param behavior The behavior to set
			 * @return behavior The behavior id to get, none will be delivered if not found
			 */
			BehaviorType mapBehavior(const Ogre::String& behavior);

			/**
			 * @brief Gets the behavior string from the given behavior id
			 * @param behaviorType The behavior id to set
			 * @return behaviorString The behavior string to get, empty string will be delivered if not found.
			 */
			Ogre::String behaviorToString(BehaviorType behaviorType);

			/**
			 * @brief Sets the mask directly as abbreviation
			 * @param mask The behavior mask to set
			 */
			void setBehaviorMask(unsigned int mask);

			/**
			 * @brief		Sets the path goal observer to react when an agent reached a path goal.
			 * @note		This class takes care of the ownership of the observer and will delete it, automatically.
			 * @param[in]	pathGoalObserver		The path goal observer to set
			 */
			void setPathGoalObserver(IPathGoalObserver* pathGoalObserver);

			/**
			 * @brief		Sets the path goal closure observer to react when an agent reached a path goal.
			 * @note		This class takes care of the ownership of the observer and will delete it, automatically.
			 * @param[in]	pathGoalClosureObserver		The path goal closure observer to set
			 */
			void setPathGoalClosureObserver(IPathGoalObserver* pathGoalClosureObserver);

			/**
			 * @brief		Sets the agent stuck observer to react when an agent got stuck.
			 * @note		This class takes care of the ownership of the observer and will delete it, automatically.
			 * @param[in]	agentStuckObserver		The agent stuck observer to set
			 */
			void setAgentStuckObserver(IAgentStuckObserver* agentStuckObserver);

			
			/**
			 * @brief Updates the moving behavior and internally calculates the resulting force and moves the agent
			 * @param dt The time between to frame to set in seconds (0.016) when using 60FPS
			 */
			void update(Ogre::Real dt);
			
			/**
			 * @brief Calculates the resulting velocity from AI behaviors
			 * @param dt The time between to frame to set in seconds (0.016) when using 60FPS
			 */
			Ogre::Vector3 calculate(Ogre::Real dt);
			//  tags any GameObjects contained in a std container that are within the
			//  offset of the single GameObjects parameter
			// INFO: call this function if want to check if another objects are tagged (e.g. for the flee function just to flee, until the other gameobject is without range)
			// not used!
			// void tagNeighbors(Ogre::Real radius = 30.0f);
			// void tagNeighbor(PhysicsActiveComponent* otherPhysicsComponent, Ogre::Real radius = 0.0f);
			
			void createRandomPath(unsigned int numWaypoints, Ogre::Real minX, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxZ) const;

			Path* getPath(void) const;

			void setActualizePathDelaySec(Ogre::Real actualizePathDelay);

			Ogre::Real getActualizePathDelaySec(void) const;

			void setStuckCheckTime(Ogre::Real stuckCheckTime);

			Ogre::Real getStuckCheckTime(void) const;

			Ogre::Real getMotionDistanceChange(void) const;

			void setPathFindData(int pathSlot, int targetSlot, bool drawPath);

			void setPathSlot(int pathSlot);

			int getPathSlot(void) const;

			void setPathTargetSlot(int targetSlot);

			int getPathTargetSlot(void) const;

			void setDrawPath(bool drawPath);

			void findPath(void);

			Ogre::String getCurrentBehavior(void) const;

			void reset(void);

			unsigned long getAgentId(void) const;

			void setWeightSeparation(Ogre::Real weightSeparation);

			Ogre::Real getWeightSeparation(void) const;

			void setWeightCohesion(Ogre::Real weightCohesion);

			Ogre::Real getWeightCohesion(void) const;

			void setWeightAlignment(Ogre::Real weightAlignment);

			Ogre::Real getWeightAlignment(void) const;

			void setWeightWander(Ogre::Real weightWander);

			Ogre::Real getWeightWander(void) const;

			void setWeightObstacleAvoidance(Ogre::Real weightObstacleAvoidance);

			Ogre::Real getWeightObstacleAvoidance(void) const;

			void setWeightSeek(Ogre::Real weightSeek);

			Ogre::Real getWeightSeek(void) const;

			void setWeightFlee(Ogre::Real weightFlee);

			Ogre::Real getWeightFlee(void) const;

			void setWeightArrive(Ogre::Real weightArrive);

			Ogre::Real getWeightArrive(void) const;

			void setWeightPursuit(Ogre::Real weightPursuit);

			Ogre::Real getWeightPursuit(void) const;

			void setWeightOffsetPursuit(Ogre::Real weightOffsetPursuit);

			Ogre::Real getWeightOffsetPursuit(void) const;

			void setWeightHide(Ogre::Real weightHide);

			Ogre::Real getWeightHide(void) const;

			void setWeightEvade(Ogre::Real weightEvade);

			Ogre::Real getWeightEvade(void) const;

			void setWeightFollowPath(Ogre::Real weightFollowPath);

			Ogre::Real getWeightFollowPath(void) const;

			void setWeightInterpose(Ogre::Real weightInterpose);

			Ogre::Real getWeightInterpose(void) const;
			
			void setJumpAtObstacle(bool jumpAtObstacle);
			
			bool getJumpAtObstacle(void) const;

			bool getIsStuck(void) const;

			void setAutoOrientation(bool autoOrientation);

			bool getIsAutoOrientated(void) const;

			void setAutoAnimation(bool autoAnimation);

			bool getIsAutoAnimated(void) const;

			void setOffsetPosition(const Ogre::Vector3& offsetPosition);
		private:
			// Stand idle
			Ogre::Vector3 none(void);

			// This behavior returns a vector that move the GameObject according to an outside specified direction/orientation
			Ogre::Vector3 move(void);

			Ogre::Vector3 moveRandomly(Ogre::Real dt);

			// This behavior moves the GameObject towards a target position and delivers stuck, if position is unreachable
			Ogre::Vector3 seek(Ogre::Vector3 targetPosition, Ogre::Real dt);
			
			Ogre::Vector3 seek2D(Ogre::Vector3 targetPosition, Ogre::Real dt);

			// This behavior returns a vector that moves the GameObject away
			// from a target position
			Ogre::Vector3 flee(Ogre::Vector3 targetPosition, Ogre::Real dt);
			
			Ogre::Vector3 flee2D(Ogre::Vector3 targetPosition, Ogre::Real dt);

			// This behavior is similar to seek but it attempts to arrive 
			// at the target position with a zero velocity
			Ogre::Vector3 arrive(Ogre::Vector3 targetPosition, eDeceleration deceleration, Ogre::Real dt);
			
			Ogre::Vector3 arrive2D(Ogre::Vector3 targetPosition, eDeceleration deceleration, Ogre::Real dt);

			// This behavior makes the agent wander about randomly
			Ogre::Vector3 wander(Ogre::Real dt);
			
			Ogre::Vector3 wander2D(Ogre::Real dt);
			// This behavior predicts where an target will be in time T and seeks towards that point to intercept it
			Ogre::Vector3 pursuit(Ogre::Real dt);

			Ogre::Vector3 pursuit2D(Ogre::Real dt);

			// This behavior produces a steering force that keeps an agent at a specified offset from a target
			Ogre::Vector3 offsetPursuit(Ogre::Real dt);

			Ogre::Vector3 evade(Ogre::Real dt);

			//  Given a series of Vector3Ds, this method produces a force that will
			//  move the GameObject along the waypoints in order. The GameObject uses the
			// 'seek' behavior to move to the next waypoint - unless it is the last
			//  waypoint, in which case it 'arrives'
			Ogre::Vector3 followPath(Ogre::Real dt);
			
			Ogre::Vector3 followPath2D(Ogre::Real dt);

			Ogre::Vector3 followTrace2D(Ogre::Real dt);

			Ogre::Vector3 interpose(Ogre::Real dt);

			Ogre::Vector3 obstacleAvoidance(Ogre::Real dt);

			Ogre::Vector3 getHidingPosition(const Ogre::Vector3& positionObject, Ogre::Real radiusObject, const Ogre::Vector3& positionHunter);

			Ogre::Vector3 hide(Ogre::Real dt);

			Ogre::Vector3 flocking(Ogre::Real dt);

			std::pair<bool, Ogre::Vector3> flockingRuleCohesion(void);

			std::pair<bool, Ogre::Vector3> flockingRuleSeparation(void);

			std::pair<bool, Ogre::Vector3> flockingRuleSpread(void);

			std::pair<bool, Ogre::Vector3> flockingFormationVShape(void);
			
			std::pair<bool, Ogre::Vector3> flockingRuleAlignment(void);

			std::pair<bool, Ogre::Vector3> flockingRuleFlee(void);

			std::pair<bool, Ogre::Vector3> flockingRuleSeek(void);

			std::pair<bool, Ogre::Vector3> flockingRuleBorder(void);

			std::pair<bool, Ogre::Vector3> flockingObstacleAvoidance(void);

			Ogre::Vector3 calculatePrioritized(Ogre::Real dt);

			bool accumulateVelocity(Ogre::Vector3& runningTotal, Ogre::Vector3 velocityToAdd);

			Ogre::Vector3 limitVelocity(const Ogre::Vector3& totalVelocity);

			Ogre::Vector3 limitFlockingVelocity(const Ogre::Vector3& totalVelocity);

			void findRandomPath(void);

			void detectAgentMotionChange(Ogre::Real dt);

		private:
			// Owner of this instance
			PhysicsActiveComponent* agent;
			CrowdComponent* crowdComponent;
			unsigned long agentId;
			bool autoOrientation;
			bool autoAnimation;
			IAnimationBlender* animationBlender;
			Ogre::Real oldAnimationSpeed;
			Ogre::Vector3 oldGravity;
			// Target object
			PhysicsActiveComponent* targetAgent;
			PhysicsActiveComponent* targetAgent2;
			unsigned int targetAgentId;
			unsigned int targetAgentId2;
			unsigned int mask;

			Ogre::Real rotationSpeed;
			eDeceleration deceleration;
			Ogre::Real wanderJitter;
			Ogre::Real wanderRadius;
			Ogre::Real wanderDistance;
			Ogre::Real wanderAngle;
			Ogre::Vector3 wanderTarget;
			Ogre::Real decelerationTweaker;
			bool flyMode;
			//pointer to any current path
			Path* pPath;
			int pathSlot;
			int targetSlot;
			bool drawPath;
			Ogre::Real actualizePathDelay;
			Ogre::Real timeSinceLastPathActualization;
			Ogre::Vector3 oldTargetPosition;

			Ogre::Vector3 oldDistance;
			std::vector<GameObjectPtr> obstaclesHide;

			Ogre::Real obstacleHideRangeRadius;
			Ogre::Real obstacleAvoidanceRangeRadius;
			unsigned int obstaclesAvoidanceCategoryIds;

			IPathGoalObserver* pathGoalObserver;
			IAgentStuckObserver* agentStuckObserver;

			// params
			Ogre::Real weightSeparation;
			Ogre::Real weightCohesion;
			Ogre::Real weightAlignment;
			Ogre::Real weightWander;
			Ogre::Real weightObstacleAvoidance;
			// Ogre::Real weightWallAvoidance;
			Ogre::Real weightSeek;
			Ogre::Real weightFlee;
			Ogre::Real weightArrive;
			Ogre::Real weightPursuit;
			Ogre::Real weightOffsetPursuit;
			Ogre::Real weightHide;
			Ogre::Real weightEvade;
			Ogre::Real weightFollowPath;
			Ogre::Real weightInterpose;

			Ogre::String currentBehavior;

			Ogre::Real timeUntilNextRandomTurn;
			Ogre::Real goalRadius;

			std::vector<PhysicsActiveComponent*> flockingAgents;
			Ogre::Real neighborDistance;
			unsigned short stuckCount;
			Ogre::Real stuckCheckTime;
			Ogre::Real timeSinceLastStuckCheck;
			Ogre::Real motionDistanceChange;
			Ogre::Real lastMotionDistanceChange;
			Ogre::Vector3 oldAgentPositionForStuck;
			bool isStuck;
			
			bool jumpAtObstacle;

			Ogre::Vector3 offsetPosition;
		};

	}; //end namespace KI

}; //end namespace NOWA

#endif