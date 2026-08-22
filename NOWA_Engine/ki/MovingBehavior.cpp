#include "NOWAPrecompiled.h"
#include "MovingBehavior.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/PhysicsPlayerControllerComponent.h"
#include "gameobject/PhysicsActiveKinematicComponent.h"
#include "gameobject/AnimationComponentV2.h"
#include "gameobject/PlayerControllerComponents.h"
#include "gameobject/JointComponents.h"
#include "gameObject/CrowdComponent.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	namespace KI
	{
		MovingBehavior::MovingBehavior(unsigned long agentId)
			: agentId(agentId),
			mask(0),
			agent(nullptr),
			crowdComponent(nullptr),
			targetAgent(nullptr),
			targetAgent2(nullptr),
			pathGoalObserver(nullptr),
			agentStuckObserver(nullptr),
            animationComponentV2(nullptr),
			autoOrientation(true),
			autoAnimation(false),
			animationBlender(nullptr),
			oldAnimationSpeed(1.0f),
			oldGravity(Ogre::Vector3::ZERO),
			rotationSpeed(1.0f),
			deceleration(FAST),
			wanderJitter(0.1f),
			wanderRadius(2.0f),
			wanderDistance(8.0f),
			wanderAngle(0.0f),
			goalRadius(0.2f),
			decelerationTweaker(0.3f),
			flyMode(false),
			pPath(new Path()),
			pathSlot(-1),
			targetSlot(-1),
			drawPath(false),
			actualizePathDelay(-1.0f), // off
			timeSinceLastPathActualization(1.0f),
			oldTargetPosition(Ogre::Vector3::ZERO),
			oldDistance(0.0f),
			obstacleHideRangeRadius(10.0f),
			obstacleAvoidanceRangeRadius(10.0f),
			obstaclesAvoidanceCategoryIds(0),
			weightWander(1.0f),
			weightObstacleAvoidance(10.0f),
			weightSeek(2.5f),
			weightFlee(1.5f),
			weightArrive(1.0f),
			weightPursuit(1.0f),
			weightOffsetPursuit(1.0f),
			weightEvade(0.01f),
			weightHide(1.0f),
			weightFollowPath(1.0f),
			weightInterpose(1.0f),
			weightSeparation(1.5f),
			weightCohesion(1.0f),
			weightAlignment(1.0f),
			timeUntilNextRandomTurn(3.0f),
			neighborDistance(0.0f),
			stuckCount(0),
			stuckCheckTime(0.0f),
			timeSinceLastStuckCheck(0.0f),
			motionDistanceChange(0.0f),
			lastMotionDistanceChange(0.0f),
			oldAgentPositionForStuck(Ogre::Vector3::ZERO),
			jumpAtObstacle(true),
			isStuck(false),
			offsetPosition(Ogre::Vector3::ZERO),
            velocityAccumulationTweaker(2.0f),
            currentBankAngle(0.0f)
		{
			GameObjectPtr agentGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(agentId);
			if (nullptr != agentGameObjectPtr)
			{
				auto physicsActiveComponent = NOWA::makeStrongPtr(agentGameObjectPtr->getComponent<PhysicsActiveComponent>());
				if (nullptr != physicsActiveComponent)
				{
					this->agent = physicsActiveComponent.get();
					this->oldGravity = this->agent->getGravity();
					this->oldAgentPositionForStuck = this->agent->getPosition();
				}

				auto crowdComponent = NOWA::makeStrongPtr(agentGameObjectPtr->getComponent<CrowdComponent>());
				if (nullptr != crowdComponent)
				{
					this->crowdComponent = crowdComponent.get();
					this->crowdComponent->setGoalRadius(this->goalRadius);
				}
			}
			
			// Create a vector to a target position on the wander circle
			Ogre::Real theta = Ogre::Math::RangeRandom(0.0f, 1.0f) * 2.0f * Ogre::Math::PI;
			this->wanderTarget = Ogre::Vector3(this->wanderRadius * Ogre::Math::Cos(theta), 0.0f, this->wanderRadius * Ogre::Math::Sin(theta));
		}

		MovingBehavior::~MovingBehavior()
		{
			if (nullptr != this->pPath)
			{
				delete this->pPath;
				this->pPath = nullptr;
			}

			if (nullptr != this->pathGoalObserver)
			{
				delete this->pathGoalObserver;
				this->pathGoalObserver = nullptr;
			}

			if (nullptr != this->agentStuckObserver)
			{
				delete this->agentStuckObserver;
				this->agentStuckObserver = nullptr;
			}
		}

		void MovingBehavior::setRotationSpeed(Ogre::Real rotationSpeed)
		{
			this->rotationSpeed = rotationSpeed * 60.0f;
		}

		bool MovingBehavior::isSwitchOn(BehaviorType behaviorType)
		{
			return (this->mask & behaviorType) == behaviorType;
		}

				void MovingBehavior::setTargetAgentId(unsigned long targetAgentId)
        {
            if (0 == targetAgentId)
            {
                this->targetAgent = nullptr;
                return;
            }

            GameObjectPtr targetAgentGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(targetAgentId);
            if (nullptr != targetAgentGameObjectPtr)
            {
                auto physicsActiveComponent = NOWA::makeStrongPtr(targetAgentGameObjectPtr->getComponent<PhysicsActiveComponent>());
                if (nullptr != physicsActiveComponent)
                {
                    this->targetAgent = physicsActiveComponent.get();
                }
                else
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Could not get target agent from id: " + Ogre::StringConverter::toString(targetAgentId) +
                                                                                            " for game object: " + this->agent->getOwner()->getName() + ", so several behaviors will not work correctly!");
                }

                // Attention: The crowd component of the TARGET must never be assigned here. this->crowdComponent
                // belongs to the agent itself and is set in the constructor resp. in setAgentId. Overwriting it
                // makes this agent steer the crowd agent of its target.

                if (nullptr != this->targetAgent)
                {
                    // Store the current position of the target for path actualization check
                    this->oldTargetPosition = this->targetAgent->getPosition();
                }
            }
        }

		void MovingBehavior::setTargetAgentId2(unsigned long targetAgentId2)
		{
			if (0 == targetAgentId2)
			{
				this->targetAgent2 = nullptr;
				return;
			}

			GameObjectPtr targetAgentGameObject2Ptr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(targetAgentId2);
			if (nullptr != targetAgentGameObject2Ptr)
			{
				auto physicsActiveComponent2 = NOWA::makeStrongPtr(targetAgentGameObject2Ptr->getComponent<PhysicsActiveComponent>());
				if (nullptr != physicsActiveComponent2)
				{
					this->targetAgent2 = physicsActiveComponent2.get();
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Could not get target 2 agent from id: " + Ogre::StringConverter::toString(targetAgentId)
						+ " for game object: " + this->agent->getOwner()->getName() + ", so interpose behavior will not work correctly!");
				}
			}
		}

		void MovingBehavior::setAgentId(unsigned long agentId)
		{
			if (0 == agentId)
			{
				if (nullptr != this->agent)
				{
					this->agent->setGravity(this->oldGravity);
				}
				this->agent = nullptr;
				return;
			}

			GameObjectPtr agentGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(agentId);
			if (nullptr != agentGameObjectPtr)
			{
				auto physicsActiveComponent = NOWA::makeStrongPtr(agentGameObjectPtr->getComponent<PhysicsActiveComponent>());
				if (nullptr != physicsActiveComponent)
				{
					this->agent = physicsActiveComponent.get();
					this->oldGravity = this->agent->getGravity();
					this->oldAgentPositionForStuck = this->agent->getPosition();
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Could not get agent from id: " + Ogre::StringConverter::toString(agentId)
					+ " for game object: " + this->agent->getOwner()->getName() + ", so several behaviors will not work correctly!");
				}
			}
		}

		void MovingBehavior::setDeceleration(eDeceleration deceleration)
		{
			this->deceleration = deceleration;
		}

		GameObject* MovingBehavior::getTargetAgent(void) const
		{
			if (this->targetAgent != nullptr)
				return this->targetAgent->getOwner().get();

			return nullptr;
		}

		GameObject* MovingBehavior::getTargetAgent2(void) const
		{
			if (this->targetAgent2 != nullptr)
				return this->targetAgent2->getOwner().get();

			return nullptr;
		}

		void MovingBehavior::setWanderJitter(Ogre::Real wanderJitter)
		{
			this->wanderJitter = wanderJitter;
		}

		void MovingBehavior::setWanderRadius(Ogre::Real wanderRadius)
		{
			this->wanderRadius = wanderRadius;
			//create a vector to a target position on the wander circle
			Ogre::Real theta = Ogre::Math::RangeRandom(0.0f, 1.0f) * 2.0f * Ogre::Math::PI;
			this->wanderTarget = Ogre::Vector3(this->wanderRadius * Ogre::Math::Cos(theta), 0.0f, this->wanderRadius * Ogre::Math::Sin(theta));
		}

		void MovingBehavior::setWanderDistance(Ogre::Real wanderDistance)
		{
			this->wanderDistance = wanderDistance;
		}

		void MovingBehavior::setGoalRadius(Ogre::Real goalRadius)
		{
			this->goalRadius = goalRadius;
			if (nullptr != this->crowdComponent)
			{
				this->crowdComponent->setGoalRadius(this->goalRadius);
			}
		}

		Ogre::Real MovingBehavior::getGoalRadius(void) const
		{
			return this->goalRadius;
		}

		void MovingBehavior::setDeceleration(Ogre::Real deceleration)
		{
			this->decelerationTweaker = deceleration;
		}

		void MovingBehavior::setFlyMode(bool flyMode)
        {
            this->flyMode = flyMode;

            // Attention: The wander target must start CENTERED (straight ahead), not on the rim of the
            // wander circle. Seeding it with a random point on the circle means a maximum lateral offset
            // in the very first frame, which makes the agent start off in a tight circle.
            this->wanderTarget = Ogre::Vector3(0.0f, 0.0f, this->wanderRadius);
            this->wanderAngle = 0.0f;

            // The gravity is NOT touched here. A flying agent must have its gravity set to zero in the
            // scene resp. via PhysicsActiveComponent, else the force equation fights the steering.
        }

		bool MovingBehavior::isInFlyMode(void) const
		{
			return this->flyMode;
		}

		void MovingBehavior::createRandomPath(unsigned int numWaypoints, Ogre::Real minX, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxZ) const
		{
			this->pPath->createRandomPath(numWaypoints, minX, minZ, maxX, maxZ);
		}

		Path* MovingBehavior::getPath(void) const
		{
			return this->pPath;
		}

		void MovingBehavior::setActualizePathDelaySec(Ogre::Real actualizePathDelay)
		{
			if (true == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
			{
				this->actualizePathDelay = actualizePathDelay;
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not set actualize path delay data for game object: "
					+ this->agent->getOwner()->getName() + " because there are no navigation mesh elements (obstacles) for ogre recast.");
			}
			// this->timeSinceLastPathActualization = actualizePathDelay;
		}

		Ogre::Real MovingBehavior::getActualizePathDelaySec(void) const
		{
			return this->actualizePathDelay;
		}

		void MovingBehavior::setStuckCheckTime(Ogre::Real stuckCheckTime)
		{
			this->stuckCheckTime = stuckCheckTime;
			this->timeSinceLastStuckCheck = stuckCheckTime;
		}

		Ogre::Real MovingBehavior::getStuckCheckTime(void) const
		{
			return this->stuckCheckTime;
		}

		Ogre::Real MovingBehavior::getMotionDistanceChange(void) const
		{
			return this->motionDistanceChange;
		}

		void MovingBehavior::setPathFindData(int pathSlot, int targetSlot, bool drawPath)
		{
			if (true == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
			{
				this->pathSlot = pathSlot;
				this->targetSlot = targetSlot;
				this->drawPath = drawPath;
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not set find path data for game object: "
					+ this->agent->getOwner()->getName() + " because there are no navigation mesh elements (obstacles) for ogre recast.");
			}
		}

		void MovingBehavior::setPathSlot(int pathSlot)
		{
			if (true == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
				this->pathSlot = pathSlot;
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not set path slot for game object: "
					+ this->agent->getOwner()->getName() + " because there are no navigation mesh elements (obstacles) for ogre recast.");
			}
		}

		int MovingBehavior::getPathSlot(void) const
		{
			return this->pathSlot;
		}

		void MovingBehavior::setPathTargetSlot(int targetSlot)
		{
			if (true == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
				this->targetSlot = targetSlot;
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not set target slot for game object: "
					+ this->agent->getOwner()->getName() + " because there are no navigation mesh elements (obstacles) for ogre recast.");
			}
		}

		int MovingBehavior::getPathTargetSlot(void) const
		{
			return this->targetSlot;
		}

		void MovingBehavior::setDrawPath(bool drawPath)
		{
			if (true == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
				this->drawPath = drawPath;
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not set draw path data for game object: "
					+ this->agent->getOwner()->getName() + " because there are no navigation mesh elements (obstacles) for ogre recast.");
			}
		}

		void MovingBehavior::findRandomPath(void)
		{
			if (nullptr == AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there is not ogre recast specified in the project.");
				return;
			}

			if (false == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there are no navigation mesh elements (obstacles) for ogre recast.");
				return;
			}

			if (-1 == this->pathSlot)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there is no path slot set.");
				return;
			}
			else if (this->pathSlot >= 128)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there are no more path slots available. Maximum number is 128.");
				return;
			}

			if (-1 == this->targetSlot)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there is no target slot set.");
				return;
			}

			if (nullptr != this->pPath)
			{
				this->pPath->clear();
			}

			AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(this->pathSlot).clear();
			
			// Get path to random valid position on navigation mesh
			// Ogre::Vector3 endPos = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getRandomNavMeshPoint();
			Ogre::Vector3 endPos = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getRandomNavMeshPointInCircle(this->agent->getPosition(), this->wanderRadius);

			AppStateManager::getSingletonPtr()->getOgreRecastModule()->findPath(this->agent->getPosition(), endPos, this->pathSlot, this->targetSlot, this->drawPath);
		
			if (0 == AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(this->pathSlot).size())
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage("No path found!!!");
				return;
			}

#if 0
			if (nullptr != this->crowdComponent)
			{
				this->crowdComponent->setDestination(endPos);
			}
#endif

			std::vector<Ogre::Vector3> path = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(this->pathSlot);

			for (size_t i = 0; i < path.size(); i++)
			{
				this->pPath->addWayPoint(path[i]);
			}
		}

		void MovingBehavior::findPath(void)
		{
			if (nullptr == AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there is not ogre recast specified in the project.");
				return;
			}

			if (false == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there are no navigation mesh elements (obstacles) for ogre recast.");
				return;
			}

			if (-1 == this->pathSlot)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there is no path slot set.");
				return;
			}
			else if (this->pathSlot >= 128)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there are no more path slots available. Maximum number is 128.");
				return;
			}

			if (-1 == this->targetSlot)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there is no target slot set.");
				return;
			}
			else if (this->targetSlot >= 128)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there are no more path target slots available. Maximum number is 128.");
				return;
			}
			
			if (nullptr == this->targetAgent)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Could not start recast navigation for game object: "
					+ this->agent->getOwner()->getName() + " because there not target agent id set.");
				return;
			}

			if (nullptr != this->pPath)
			{
				this->pPath->clear();
			}
			AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(this->pathSlot).clear();
			Ogre::Vector3 posOnNavMesh = Ogre::Vector3::ZERO;

			if (false == AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->findNearestPointOnNavmesh(this->targetAgent->getPosition(), posOnNavMesh))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] No point found on nav mesh for game object: "
					+ this->targetAgent->getOwner()->getName() + ".");
				return;
			}

			AppStateManager::getSingletonPtr()->getOgreRecastModule()->findPath(this->agent->getPosition(), posOnNavMesh, this->pathSlot, this->targetSlot, this->drawPath);
			/*Ogre::LogManager::getSingletonPtr()->logMessage("#############findPath size: "
			+ Ogre::StringConverter::toString(this->ogreRecastModule->getOgreRecast()->getPath(0).size())
			+ " y offset: " + Ogre::StringConverter::toString(this->ogreRecastModule->getOgreRecast()->getNavmeshOffsetFromGround()));*/

			if (0 == AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(this->pathSlot).size())
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage("No path found!!!");
				return;
			}

			/*for (int i = 0; i < AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(0).size(); i++)
			{
			Ogre::LogManager::getSingletonPtr()->logMessage("pos: "
			+ Ogre::StringConverter::toString(AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(0).at(i)));
			}*/

			/*std::vector<Ogre::Vector3> path;
			if (NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(0).size() > 2) {
			path = NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(0);
			}
			else {
			path.push_back(result.second);
			}*/
			// Attention: getPath(0) was here before that new code
			std::vector<Ogre::Vector3> path = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getPath(pathSlot);


			for (size_t i = 0; i < path.size(); i++)
			{
				Ogre::Vector3 waypoint = path[i];
				this->pPath->addWayPoint(Ogre::Vector3(path[i]));
			}
		}

		Ogre::Vector3 MovingBehavior::none(void)
		{
			return Ogre::Vector3::ZERO;
		}

		Ogre::Vector3 MovingBehavior::move(void)
		{
			return (this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection()) * this->agent->getSpeed();
		}

		Ogre::Vector3 MovingBehavior::moveRandomly(Ogre::Real dt)
        {
            if (this->timeUntilNextRandomTurn >= 0.0f)
            {
                this->timeUntilNextRandomTurn -= dt;
            }

            if (this->timeUntilNextRandomTurn <= 0.0f)
            {
                Ogre::Real turnDegrees = 0.0f;

                auto contact = this->agent->getContactAhead(10, Ogre::Vector3(0.0f, 0.4f, 0.0f), 1.5f);
                if (nullptr != contact.getHitGameObject())
                {
                    // Turns away hard on a threatening collision
                    turnDegrees = (Ogre::Math::RangeRandom(-3.0f, 3.0f) > 0.0f) ? 70.0f : -70.0f;
                }
                else
                {
                    turnDegrees = Ogre::Math::RangeRandom(-70.0f, 70.0f);
                }

                // Attention: The former version computed the quaternion but never applied it, both
                // setOrientation calls were commented out. Hence the whole random turn was dead code and
                // the function just returned the current facing direction forever.
                Ogre::Quaternion targetOrientation = this->agent->getOrientation() * Ogre::Quaternion(Ogre::Degree(turnDegrees), Ogre::Vector3::UNIT_Y);
                this->agent->applyOmegaForceRotateTo(targetOrientation, Ogre::Vector3::UNIT_Y, this->getTurnRate());

                this->timeUntilNextRandomTurn = 3.0f;
            }

            return (this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection()) * this->agent->getSpeed();
        }

		Ogre::Vector3 MovingBehavior::seek(Ogre::Vector3 targetPosition, Ogre::Real dt)
        {
            // Attention: Only the real seek behavior may drive the crowd agent. Flocking rules must use
            // seekPure, else the detour destination is overwritten with e.g. the center of mass each frame.
            if (nullptr != this->crowdComponent)
            {
                this->crowdComponent->updateDestination(targetPosition, true);
            }

            return this->seekPure(targetPosition);
        }

		Ogre::Vector3 MovingBehavior::seekPure(Ogre::Vector3 targetPosition)
        {
            Ogre::Vector3 desiredVelocity = targetPosition - this->agent->getPosition();

            if (desiredVelocity.squaredLength() < 0.0001f)
            {
                return Ogre::Vector3::ZERO;
            }

            desiredVelocity.normalise();
            desiredVelocity *= this->agent->getSpeed();

            return desiredVelocity;
        }
		
		Ogre::Vector3 MovingBehavior::seek2D(Ogre::Vector3 targetPosition, Ogre::Real dt)
		{
			Ogre::Vector3 desiredVelocity = Ogre::Vector3(targetPosition.x, targetPosition.y, this->agent->getPosition().z) - Ogre::Vector3(this->agent->getPosition().x, this->agent->getPosition().y, this->agent->getPosition().z);

			desiredVelocity.normalise();
			desiredVelocity *= this->agent->getSpeed();

			// move to the calculated direction
			return std::move(desiredVelocity);
		}

		Ogre::Vector3 MovingBehavior::flee(Ogre::Vector3 targetPosition, Ogre::Real dt)
		{
			Ogre::Vector3 desiredVelocity = this->agent->getPosition() - targetPosition;
			// desiredVelocity.y = 0.0f;
			desiredVelocity.normalise();
			desiredVelocity *= this->agent->getSpeed();

			// move to the calculated direction
			return desiredVelocity;
		}
		
		Ogre::Vector3 MovingBehavior::flee2D(Ogre::Vector3 targetPosition, Ogre::Real dt)
		{
			Ogre::Vector3 desiredVelocity = Ogre::Vector3(this->agent->getPosition().x, this->agent->getPosition().y, 0.0f) - Ogre::Vector3(targetPosition.x, targetPosition.y, 0.0f);
			// desiredVelocity.y = 0.0f;
			desiredVelocity.normalise();
			desiredVelocity *= this->agent->getSpeed();

			// move to the calculated direction
			return desiredVelocity;
		}

		Ogre::Vector3 MovingBehavior::arrive(Ogre::Vector3 targetPosition, eDeceleration deceleration, Ogre::Real dt)
		{
			Ogre::Vector3 resultDirection = targetPosition - this->agent->getPosition();
			
			// Calculates the distance to the target
			Ogre::Real distance = resultDirection.length();
			// Prevents jitter
			if (distance > 0.06f)
			{
				// because Deceleration is enumerated as an int, this value is required
				// to provide fine tweaking of the deceleration..
				// calculate the speed required to reach the target given the desired
				// deceleration
				Ogre::Real speed = distance / (static_cast<Ogre::Real>(deceleration * this->decelerationTweaker));
				// make sure the velocity does not exceed the max
				speed = std::min(speed, this->agent->getMaxSpeed());
				// from here proceed just like Seek except we don't need to normalize 
				// the ToTarget vector because we have already gone to the trouble
				// of calculating its length: dist. 
				// Info: y component must eventuelly be 0
				Ogre::Vector3 desiredVelocity = resultDirection * speed / distance;

				return desiredVelocity /*- this->agent->getVelocity()*/;
			}
		    return Ogre::Vector3::ZERO;
		}
		
		Ogre::Vector3 MovingBehavior::arrive2D(Ogre::Vector3 targetPosition, eDeceleration deceleration, Ogre::Real dt)
		{
			Ogre::Vector3 resultDirection = Ogre::Vector3(targetPosition.x, targetPosition.y, 0.0f) - Ogre::Vector3(this->agent->getPosition().x, this->agent->getPosition().y, 0.0f);

			//calculate the distance to the target
			Ogre::Real distance = resultDirection.length();

			// Prevents jitter
			if (distance > 0.06f)
			{
				// because Deceleration is enumerated as an int, this value is required
				// to provide fine tweaking of the deceleration..
				// calculate the speed required to reach the target given the desired
				// deceleration
				Ogre::Real speed = distance / (static_cast<Ogre::Real>(deceleration * this->decelerationTweaker));
				// make sure the velocity does not exceed the max
				speed = std::min(speed, this->agent->getMaxSpeed());
				// from here proceed just like Seek except we don't need to normalize 
				// the ToTarget vector because we have already gone to the trouble
				// of calculating its length: dist. 
				// Info: y component must eventuelly be 0
				Ogre::Vector3 desiredVelocity = resultDirection * speed / distance;

				return desiredVelocity * Ogre::Vector3(1.0f, 1.0f, 0.0f);
			}
			return Ogre::Vector3::ZERO;
		}

		Ogre::Vector3 MovingBehavior::pursuit(Ogre::Real dt)
		{
			if (nullptr == this->targetAgent)
			{
				return Ogre::Vector3::ZERO;
			}
			//if the evader is ahead and facing the agent then we can just seek
			//for the evader's current position.
			Ogre::Vector3 toEvader = this->targetAgent->getPosition() - this->agent->getPosition();

			// get the relative heading
			Ogre::Vector3 heading = this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection();
			Ogre::Real relativeHeading = heading.dotProduct(this->targetAgent->getOrientation() * this->targetAgent->getOwner()->getDefaultDirection());

			//acos(0.95)=18 degs 
			if ((toEvader.dotProduct(heading) > 0.0f) && (relativeHeading < -0.95f))
			{
				// Attention seek can stuck
				return this->seek(this->targetAgent->getPosition(), dt);
			}

			//Not considered ahead so we predict where the evader will be.

			//the lookahead time is propotional to the distance between the evader
			//and the pursuer; and is inversely proportional to the sum of the
			//GameObject's velocities
			Ogre::Real lookAheadTime = toEvader.length() / (this->agent->getMaxSpeed() + this->targetAgent->getSpeed());

			//now seek to the predicted future position of the evader
			// Attention seek can stuck
			return this->seek(this->targetAgent->getPosition() + this->targetAgent->getVelocity() * lookAheadTime, dt);
		}

		Ogre::Vector3 MovingBehavior::pursuit2D(Ogre::Real dt)
		{
			if (nullptr == this->targetAgent)
			{
				return Ogre::Vector3::ZERO;
			}
			//if the evader is ahead and facing the agent then we can just seek
			//for the evader's current position.
			Ogre::Vector3 targetPosition2D = Ogre::Vector3(this->targetAgent->getPosition().x, this->targetAgent->getPosition().y, this->agent->getPosition().z);

			Ogre::Vector3 toEvader = targetPosition2D - this->agent->getPosition();


			// get the relative heading
			Ogre::Vector3 heading = this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection();
			Ogre::Real relativeHeading = heading.dotProduct(this->targetAgent->getOrientation() * this->targetAgent->getOwner()->getDefaultDirection());

			//acos(0.95)=18 degs 
			if ((toEvader.dotProduct(heading) > 0.0f) && (relativeHeading < -0.95f))
			{
				// Attention seek can stuck
				return this->seek(this->targetAgent->getPosition(), dt);
			}

			//Not considered ahead so we predict where the evader will be.

			//the lookahead time is propotional to the distance between the evader
			//and the pursuer; and is inversely proportional to the sum of the
			//GameObject's velocities
			Ogre::Real lookAheadTime = toEvader.length() / (this->agent->getMaxSpeed() + this->targetAgent->getSpeed());

			Ogre::Vector3 targetVelocity2D = Ogre::Vector3(this->targetAgent->getVelocity().x, this->targetAgent->getVelocity().y, 0.0f);

			//now seek to the predicted future position of the evader
			// Attention seek can stuck
			return this->seek2D(targetPosition2D + targetVelocity2D * lookAheadTime, dt);
		}

		Ogre::Vector3 MovingBehavior::offsetPursuit(Ogre::Real dt)
		{
			if (nullptr == this->targetAgent)
				return Ogre::Vector3::ZERO;

			// Calculates the offset's position in world space
			Ogre::Vector3 worldOffsetPos = this->targetAgent->getPosition() + this->offsetPosition;

			Ogre::Vector3 toOffset = worldOffsetPos - this->agent->getPosition();

			// The lookahead time is propotional to the distance between the leader and the pursuer; and is inversely proportional to the sum of both agent's velocities
			Ogre::Real lookAheadTime = toOffset.length() / (this->agent->getMaxSpeed() + this->targetAgent->getSpeed());

			// Now arrive at the predicted future position of the offset
			return this->arrive(worldOffsetPos + this->targetAgent->getVelocity() * lookAheadTime, FAST, dt);
		}

		void MovingBehavior::setOffsetPosition(const Ogre::Vector3& offsetPosition)
		{
			this->offsetPosition = offsetPosition;
		}

		Ogre::Vector3 MovingBehavior::evade(Ogre::Real dt)
		{
			if (nullptr == this->targetAgent)
			{
				return Ogre::Vector3::ZERO;
			}
			//if the evader is ahead and facing the agent then we can just seek
			//for the evader's current position.
			Ogre::Vector3 toPursuer = this->targetAgent->getPosition() - this->agent->getPosition();

			//the lookahead time is propotional to the distance between the pursuer
			//and the pursuer; and is inversely proportional to the sum of the
			//this->flockingAgents' velocities
			Ogre::Real lookAheadTime = toPursuer.length() / (this->agent->getMaxSpeed() + this->targetAgent->getSpeed());

			return this->flee(this->targetAgent->getPosition() + this->targetAgent->getVelocity() * lookAheadTime, dt);
		}

		Ogre::Vector3 MovingBehavior::followPath(Ogre::Real dt)
		{
			// Move to next target if close enough to current target (working indistance squared space)
			if (this->pPath->getWayPoints().size() > 0)
			{
				std::pair<bool, Ogre::Vector3> currentWaypoint = this->pPath->getCurrentWaypoint();
				if (currentWaypoint.first)
				{
					Ogre::Vector3 agentOriginPos = this->agent->getPosition() - this->agent->getOwner()->getBottomOffset();
					Ogre::Vector3 agentOriginPosXZ = agentOriginPos * Ogre::Vector3(1.0f, 0.0f, 1.0f);
					Ogre::Vector3 currentWaypointXZ = currentWaypoint.second * Ogre::Vector3(1.0f, 0.0f, 1.0f);
					Ogre::Real agentOriginPosY = agentOriginPos.y;
					Ogre::Real currentWaypointY = currentWaypoint.second.y;

					Ogre::Real distSqXZ = agentOriginPosXZ.squaredDistance(currentWaypointXZ);

					if (this->pPath->getRemainingWaypoints() > 1)
					{
						if (distSqXZ <= this->goalRadius * this->goalRadius && Ogre::Math::Abs(agentOriginPosY - currentWaypointY) < this->agent->getOwner()->getSize().y)
						{
							// Goalradius should be small when not in fly mode and the y pos comparison is more eased, so that the waypoint goal can be reached
							// as nearest as possible
							this->pPath->setNextWayPoint();
							// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] setNextWayPoint");
						}
					}
					else
					{
						if (distSqXZ <= this->goalRadius * this->goalRadius)
						{
							// Goalradius should be small when not in fly mode and the y pos comparison is more eased, so that the waypoint goal can be reached
							// as nearest as possible
							this->pPath->setNextWayPoint();
							// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] setNextWayPoint");
						}
					}

					if (false == this->pPath->isFinished())
					{
						currentWaypoint = this->pPath->getCurrentWaypoint();
						
						Ogre::Vector3 positionWithDirection = currentWaypoint.second;
						return this->seek(positionWithDirection, dt);
					}
					else
					{
						// If the path is not recalculated each time, 
						if (this->actualizePathDelay == -1.0f && false == this->isSwitchOn(PATH_FINDING_WANDER))
						{
							this->removeBehavior(FOLLOW_PATH);
						}
						Ogre::Vector3 resultVelocity = this->arrive(currentWaypoint.second, this->deceleration, dt);

						this->pPath->clear();
#if 0
						if (nullptr != this->crowdComponent)
						{
							this->crowdComponent->stop();
						}
#endif

						// Check if there is an path goal observer, and call when path is reached
						if (nullptr != this->pathGoalObserver)
						{
							this->pathGoalObserver->onPathGoalReached();
						}

						return resultVelocity;
					}
				}
			}
			if (this->actualizePathDelay == -1.0f && false == this->isSwitchOn(PATH_FINDING_WANDER))
			{
				this->removeBehavior(FOLLOW_PATH);
			}
			return Ogre::Vector3::ZERO;
		}
		
		Ogre::Vector3 MovingBehavior::followPath2D(Ogre::Real dt)
		{
			// Move to next target if close enough to current target (working indistance squared space)
			if (this->pPath->getWayPoints().size() > 0)
			{
				std::pair<bool, Ogre::Vector3> currentWaypoint = this->pPath->getCurrentWaypoint();
				if (currentWaypoint.first)
				{
					Ogre::Vector3 currentWaypointX = currentWaypoint.second * Ogre::Vector3(1.0f, 0.0f, 0.0f);
					Ogre::Vector3 agentPosX = this->agent->getPosition() * Ogre::Vector3(1.0f, 0.0f, 0.0f);
					Ogre::Real distSqXZ = currentWaypointX.squaredDistance(agentPosX);
					Ogre::Real distSq = this->agent->getPosition().squaredDistance(currentWaypoint.second);
					Ogre::Real currentWaypointY = currentWaypoint.second.y;
					Ogre::Real agentPosY = this->agent->getPosition().y;
					Ogre::Real agentHeight = this->agent->getOwner()->getSize().y;

					// make a copy of maybe the last point, because if setNextWayPoint has not other points, the list gets cleared!
					// Attention here: When a character does start to orientate when reaching the goal, the distance is to low, because maybe the orientation speed was to low
					// so the character never reaches the goal!
					if (false == this->flyMode)
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] Agent Pos: " + Ogre::StringConverter::toString(this->agent->getPosition()));
						if (distSq <= this->goalRadius * this->goalRadius * 5.0f)
						{
							if (distSqXZ <= this->goalRadius * this->goalRadius)
							{
								// Goalradius should be small when not in fly mode and the y pos comparison is more eased, so that the waypoint goal can be reached
								// as nearest as possible
								if (agentPosY + agentHeight > currentWaypointY && agentPosY - agentHeight < currentWaypointY)
								{
									// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] setNextWayPoint");
									this->pPath->setNextWayPoint();
								}
							}
						}
					}
					else
					{
						if (distSq <= this->goalRadius * this->goalRadius)
						{
							this->pPath->setNextWayPoint();
						}
					}

					if (false == this->pPath->isFinished())
					{
						return this->seek2D(currentWaypoint.second, dt);
					}
					else
					{
						Ogre::Vector3 resultVelocity = this->arrive2D(currentWaypoint.second, this->deceleration, dt);

						// Check if there is an path goal observer, and call when path is reached
						if (nullptr != this->pathGoalObserver && false == this->pPath->getWayPoints().empty())
						{
							this->pathGoalObserver->onPathGoalReached();
						}

						this->pPath->clear();

						return resultVelocity;
					}
				}
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Path follow 2D does not work, because there are no waypoints specified for game object: "
					+ this->agent->getOwner()->getName() + "!");
			}
			return Ogre::Vector3::ZERO;
		}

		Ogre::Vector3 MovingBehavior::interpose(Ogre::Real dt)
		{
			if (nullptr == this->targetAgent)
			{
				return Ogre::Vector3::ZERO;
			}

			if (nullptr == this->targetAgent2)
			{
				return Ogre::Vector3::ZERO;
			}

			// Figures out where the two agents are going to be at time T in the future. This is approximated by determining the time taken to reach the mid way point at the current time at at max speed
			Ogre::Vector3 midPoint = (this->targetAgent->getPosition() + this->targetAgent2->getPosition()) / 2.0;

			Ogre::Real timeToReachMidPoint = this->agent->getPosition().squaredDistance(midPoint) / this->agent->getMaxSpeed();

			// Knowing T, and assuming agent A and agent B will continue on a straight trajectory and extrapolate to get their future positions
			Ogre::Vector3 pos1 = this->targetAgent->getPosition() + this->targetAgent->getVelocity() * timeToReachMidPoint;
			Ogre::Vector3 pos2 = this->targetAgent2->getPosition() + this->targetAgent2->getVelocity() * timeToReachMidPoint;

			//calculate the mid point of these predicted positions
			midPoint = (pos1 + pos2) / 2.0;

			// Then steer to Arrive at it
			return this->arrive(midPoint, FAST, dt);
		}

				Ogre::Vector3 MovingBehavior::wander(Ogre::Real dt)
        {
            if (nullptr != this->crowdComponent)
            {
                // Navmesh based wandering. Note: This is only sensible for ground agents,
                // because detour crowd projects everything onto the navigation mesh.
                if (true == this->crowdComponent->destinationReached())
                {
                    Ogre::Vector3 randomPosition = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getRandomNavMeshPoint();
                    this->crowdComponent->updateDestination(randomPosition);
                    return this->seek(randomPosition, dt);
                }
                return Ogre::Vector3::ZERO;
            }

            // Guards against a zero or a huge dt (breakpoint, loading spike)
            if (dt <= 0.0f)
            {
                dt = 1.0f / 60.0f;
            }
            else if (dt > 0.1f)
            {
                dt = 0.1f;
            }

            const Ogre::Real radius = (this->wanderRadius > 0.01f) ? this->wanderRadius : 0.01f;
            const Ogre::Real distance = (this->wanderDistance > 0.01f) ? this->wanderDistance : 0.01f;

            // 1) Determines the current heading. The velocity is preferred, because it is the direction the
            //    agent really moves to. If the agent stands still, the facing direction is used.
            Ogre::Vector3 heading = this->agent->getVelocity();
            if (false == this->flyMode)
            {
                heading.y = 0.0f;
            }

            if (heading.squaredLength() < 0.0001f)
            {
                heading = this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection();
                if (false == this->flyMode)
                {
                    heading.y = 0.0f;
                }
            }

            if (heading.squaredLength() < 0.0001f)
            {
                heading = Ogre::Vector3::UNIT_Z;
            }
            heading.normalise();

            // 2) Builds an orthonormal frame (side, up, heading) around the heading, so that the wander
            //    offset can be applied in agent local space. This also works if the agent flies straight up.
            Ogre::Vector3 side = heading.crossProduct(Ogre::Vector3::UNIT_Y);
            if (side.squaredLength() < 0.0001f)
            {
                side = heading.crossProduct(Ogre::Vector3::UNIT_X);
            }
            side.normalise();

            Ogre::Vector3 up = side.crossProduct(heading);
            up.normalise();

            // 3) Random walk of the wander offset.
            //
            //    Attention: Two things are essential here and both were wrong before:
            //
            //    a) The increment scales with SQRT(dt), not with dt. A random walk accumulates variance
            //       linearly in time, so a linear dt scaling makes the whole behavior frame rate dependent
            //       (the agent gets lazier the higher the frame rate).
            //
            //    b) The mean reversion term is mandatory. The offset lives in the LOCAL frame of the agent
            //       and that frame rotates with the agent. A permanent lateral offset therefore means a
            //       permanent turn rate, which is a perfect circle. Without a pull back towards straight
            //       ahead, an unbiased random walk parks on one side for many seconds and the agent just
            //       circles. This is exactly the bug that made wandering useless.
            const Ogre::Real jitterThisFrame = this->wanderJitter * radius * Ogre::Math::Sqrt(dt);
            const Ogre::Real meanReversion = 1.5f; // 1/s, roughly the time the agent holds a turn direction

            this->wanderTarget.x += Ogre::Math::RangeRandom(-1.0f, 1.0f) * jitterThisFrame;
            this->wanderTarget.x -= this->wanderTarget.x * meanReversion * dt;

            if (true == this->flyMode)
            {
                // The vertical part is deliberately calmer and reverts faster, so the agent does not
                // slowly drift away in height.
                this->wanderTarget.y += Ogre::Math::RangeRandom(-1.0f, 1.0f) * jitterThisFrame * 0.4f;
                this->wanderTarget.y -= this->wanderTarget.y * meanReversion * 2.0f * dt;
            }
            else
            {
                this->wanderTarget.y = 0.0f;
            }

            // Keeps the offset inside the wander circle
            Ogre::Real lateralSquared = (this->wanderTarget.x * this->wanderTarget.x) + (this->wanderTarget.y * this->wanderTarget.y);
            if (lateralSquared > radius * radius)
            {
                const Ogre::Real scale = radius / Ogre::Math::Sqrt(lateralSquared);
                this->wanderTarget.x *= scale;
                this->wanderTarget.y *= scale;
                lateralSquared = radius * radius;
            }

            // The forward component is derived, so the target always stays on the front half of the sphere.
            // This limits the maximum steering angle to atan(radius / (distance + z)) and the agent can
            // never be told to turn backwards in a single frame.
            this->wanderTarget.z = Ogre::Math::Sqrt(std::max(0.0f, (radius * radius) - lateralSquared));

            // 4) Circle center in front of the agent plus the local space offset
            Ogre::Vector3 wanderDirection = (heading * (distance + this->wanderTarget.z)) + (side * this->wanderTarget.x) + (up * this->wanderTarget.y);

            if (wanderDirection.squaredLength() < 0.0001f)
            {
                wanderDirection = heading;
            }
            wanderDirection.normalise();

            // 5) Clamps the pitch in fly mode, so the agent never climbs or dives vertically,
            //    which looks unnatural for a flying creature.
            if (true == this->flyMode)
            {
                const Ogre::Degree maxPitch(20.0f);
                const Ogre::Real maxPitchSin = Ogre::Math::Sin(maxPitch);

                if (Ogre::Math::Abs(wanderDirection.y) > maxPitchSin)
                {
                    Ogre::Vector3 horizontal(wanderDirection.x, 0.0f, wanderDirection.z);
                    if (horizontal.squaredLength() < 0.0001f)
                    {
                        horizontal = Ogre::Vector3(heading.x, 0.0f, heading.z);
                    }
                    if (horizontal.squaredLength() < 0.0001f)
                    {
                        horizontal = Ogre::Vector3::UNIT_Z;
                    }
                    horizontal.normalise();

                    const Ogre::Real ySign = (wanderDirection.y >= 0.0f) ? 1.0f : -1.0f;
                    wanderDirection = (horizontal * Ogre::Math::Cos(maxPitch)) + (Ogre::Vector3::UNIT_Y * (ySign * maxPitchSin));
                    wanderDirection.normalise();
                }
            }
            else
            {
                wanderDirection.y = 0.0f;
                if (wanderDirection.squaredLength() < 0.0001f)
                {
                    wanderDirection = Ogre::Vector3::UNIT_Z;
                }
                wanderDirection.normalise();
            }

            return wanderDirection * this->agent->getSpeed();
        }
		
		Ogre::Vector3 MovingBehavior::wander2D(Ogre::Real dt)
		{
			//// Calculate the circle center
			//Ogre::Vector3 circleCenter = this->agent->getVelocity();
			//circleCenter.normalise();
			//circleCenter *= this->wanderDistance * Ogre::Vector3(1.0f, 0.0f, 1.0f); // This is necessary, else the agent will fly up away immediately
			////
			//// Calculate the displacement force
			//Ogre::Vector3 displacement(this->defaultOrientation);
			//displacement *= this->wanderRadius;

			////
			//// Randomly change the vector direction
			//// by making it change its current angle
			//Ogre::Real len = 1.0f * displacement.length();
			//Ogre::Vector3 angle(Ogre::Math::Cos(this->wanderAngle) * len, 0.0f, Ogre::Math::Sin(this->wanderAngle) * len);
			//
			//displacement = angle;

			//// Change wanderAngle just a bit, so it
			//// won't have the same value in the
			//// next game frame.
			//this->wanderAngle += (Ogre::Math::RangeRandom(0.0f, 1.0f) * this->wanderJitter) - (this->wanderJitter * 0.5f);
			//// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] circleCenter: " + Ogre::StringConverter::toString(circleCenter));
			////
			//// Finally calculate and return the wander force
			//Ogre::Vector3 wanderForce = circleCenter + displacement;
			//return wanderForce;

			if (Ogre::Math::RangeRandom(0.0f, this->wanderJitter) > this->wanderJitter * 0.9f)
			{
				Ogre::Vector3 direction = Ogre::Vector3(2.0f, 0.0f, 0.0f);
				if (Ogre::Math::RangeRandom(0.0f, 1.0f) > 0.5f)
					direction = Ogre::Vector3(-2.0f, 0.0f, 0.0f);

				return this->seek2D(direction, dt);
			}
			else
			{
				return this->move();
			}
		}

		Ogre::Vector3 MovingBehavior::obstacleAvoidance(Ogre::Real dt)
		{
			Ogre::Vector3 steeringForce = Ogre::Vector3::ZERO;
			// Raycast in direction of the object
			Ogre::Vector3 direction = this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection();
			// Get the position relative to direction and an offset
			Ogre::Vector3 position = this->agent->getPosition() 
				+ (this->agent->getOrientation() * Ogre::Vector3(0.0f, this->agent->getOwner()->getSize().y / 2.0f, 0.0f));
			// Shoot the ray in that direction
			// OgreNewt::BasicRaycast ray(this->agent->getOgreNewt(), position, position + (direction * this->obstacleAvoidanceRangeRadius), true);
			OgreNewt::BasicRaycast ray(this->agent->getOgreNewt(), position, position + (direction * this->obstacleAvoidanceRangeRadius), true);

			OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
			// Get contact result
			// OgreNewt::BasicRaycast::BasicRaycastInfo contact = ray.getFirstHit();
			if (info.mBody)
			{
				unsigned int type = info.mBody->getType();
				unsigned int finalType = type & this->obstaclesAvoidanceCategoryIds;
				if (type == finalType)
				{
					// Create a force in the direction of the wall normal, with a magnitude of the overshoot
					steeringForce = info.getNormal() * info.getDistance()/* * rayShootDistance*/;

					// Ogre::Vector3 target = info.getBody()->getPosition() + info.getNormal() * 2.0f;
					// steeringForce = info.getNormal() * 20.0f;
					// steeringForce = this->agent->getPosition() - info.getBody()->getPosition();
					// steeringForce.y = 0.0f;
					// this->wanderTarget = steeringForce;
					/*desiredVelocity.normalise();
					desiredVelocity *= this->agent->getSpeed();*/
				}
			}
			return steeringForce;
		}

		Ogre::Vector3 MovingBehavior::getHidingPosition(const Ogre::Vector3& positionObject, Ogre::Real radiusObject, const Ogre::Vector3& positionHunter)
		{
			// Calculates how far away the agent is to be from the chosen obstacles bounding radius
			Ogre::Real distAway = radiusObject + this->obstacleHideRangeRadius;

			// Calculates the heading toward the object from the hunter
			Ogre::Vector3 toObject = (positionObject - positionHunter);
			toObject.normalise();

			// Scales it to size and add to the obstacles position to get the hiding spot.
			return (toObject * distAway) + positionObject;
		}

		Ogre::Vector3 MovingBehavior::hide(Ogre::Real dt)
		{
			if (nullptr == this->targetAgent)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Cannot use 'hide' behavior, because there is no target agent. Please specify a target agent id. In game object: '"
					+ this->agent->getOwner()->getName() + "'");
				return Ogre::Vector3::ZERO;
			}

			PhysicsActiveComponent* hunter = this->targetAgent;
			
			Ogre::Real distanceToClosestObject = Ogre::Math::POS_INFINITY;
			Ogre::Vector3 bestHidingSpot;

			std::vector<GameObjectPtr>::const_iterator currentObject = this->obstaclesHide.cbegin();
			std::vector<GameObjectPtr>::const_iterator closestObject;

			while (currentObject != this->obstaclesHide.cend())
			{
				//calculate the position of the hiding spot for this obstacle
				Ogre::Vector3 hidingSpot = this->getHidingPosition((*currentObject)->getPosition(), (*currentObject)->getSize().x * 0.5f, hunter->getPosition());

				// Work in distance-squared space to find the closest hiding spot to the agent
				Ogre::Real distance = hidingSpot.squaredDistance(this->agent->getPosition());

				if (distance < distanceToClosestObject)
				{
					distanceToClosestObject = distance;

					bestHidingSpot = hidingSpot;

					closestObject = currentObject;
				}

				++currentObject;
			}

			 // If no suitable obstacles found then Evade the hunter
			if (distanceToClosestObject == Ogre::Math::POS_INFINITY)
			{
				return this->evade(dt);
			}

			// Else use Arrive on the hiding spot
			return this->arrive(bestHidingSpot, FAST, dt);
		}

		//Ogre::Vector3 MovingBehavior::obstacleAvoidance(Ogre::Real dt)
		//{
		//	Ogre::Vector3 heading = physicsComponent->getOrientation() * this->defaultOrientation;
		//	heading.y = 0.0f;
		//	Ogre::Quaternion sideOrientation = physicsComponent->getOrientation() * Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::UNIT_Y);
		//	Ogre::Vector3 side = sideOrientation * this->defaultOrientation;

		//	// The detection box length is proportional to this physics component velocity
		//	// m_dDBoxLength = Prm.MinDetectionBoxLength + (m_pVehicle->Speed() / m_pVehicle->MaxSpeed()) * Prm.MinDetectionBoxLength;
		//	Ogre::Real boxLength = 40.0f + (this->agent->getSpeed() / this->agent->getMaxSpeed() * 40.0f);

		//	// This will keep track of the closest intersecting obstacle
		//	GameObject* closestIntersectingObstacle = nullptr;

		//	// This will be used to track the distance to the CIB
		//	double distToClosestIP = std::numeric_limits<double>::max();

		//	// This will record the transformed local coordinates of the CIB
		//	Ogre::Vector3 localPosOfClosestObstacle;

		//	// Iterate through all game objects checking for range
		//	for (auto it = this->obstacles.cbegin(); it != this->obstacles.cend(); ++it)
		//	{
		//		GameObject* obstacle = (*it).get();

		//		Ogre::Vector3 distance = obstacle->getPosition() - this->agent->getPosition();

		//		//the bounding radius of the other is taken into account by adding it 
		//		//to the range
		//		Ogre::Real expandedRadius = this->obstacleRangeRadius + obstacle->getEntity()->getBoundingRadius();

		//		// If the game object within range, tag for further consideration. (working in distance-squared space to avoid sqrts)
		//		if ((obstacle != this->agent->getOwner().get()) && (distance.squaredLength() < expandedRadius * expandedRadius))
		//		{
		//			Ogre::Vector3 localPosition = MathHelper::getInstance()->pointToLocalSpace(obstacle->getPosition(), heading, side, physicsComponent->getPosition());
		//			// Ogre::Vector3 localPosition = this->agent->getOwner()->getSceneNode()->convertWorldToLocalPosition(obstacle->getPosition());
		//			
		//			// If the obstacle is behind the physics component, ignore the obstacle
		//			if (localPosition.x >= 0)
		//			{
		//				// If the distance from the x axis to the object's position is less than its radius + half the width of the detection box then there is a potential intersection.

		//				if (fabs(localPosition.z) < expandedRadius)
		//				{
		//					// Now to do a line/circle intersection test. The center of the 
		//					// Circle is represented by (cX, cY). The intersection points are given by the formula x = cX +/-sqrt(r^2-cY^2) for y=0. 
		//					// We only need to look at the smallest positive value of x because that will be the closest point of intersection.
		//					Ogre::Real cx = localPosition.x;
		//					Ogre::Real cz = localPosition.z;

		//					// We only need to calculate the sqrt part of the above equation once
		//					Ogre::Real sqrtPart = Ogre::Math::Sqrt(expandedRadius * expandedRadius - cz * cz);

		//					Ogre::Real ip = cx - sqrtPart;

		//					if (ip <= 0.0)
		//					{
		//						ip = cx + sqrtPart;
		//					}

		//					// Test to see if this is the closest so far. If it is keep a record of the obstacle and its local coordinates
		//					if (ip < distToClosestIP)
		//					{
		//						distToClosestIP = ip;

		//						closestIntersectingObstacle = obstacle;

		//						localPosOfClosestObstacle = localPosition;
		//					}
		//				}
		//			}
		//		}
		//	}

		//	// If we have found an intersecting obstacle, calculate a steering force away from it
		//	Ogre::Vector3 steeringForce = Ogre::Vector3::ZERO;

		//	if (nullptr != closestIntersectingObstacle)
		//	{
		//		// The closer the agent is to an object, the stronger the steering force should be
		//		Ogre::Real multiplier = 1.0f + (boxLength - localPosOfClosestObstacle.x) / boxLength;

		//		// Calculate the lateral force
		//		steeringForce.z = (closestIntersectingObstacle->getEntity()->getBoundingRadius() - localPosOfClosestObstacle.z)  * multiplier;

		//		// Apply a braking force proportional to the obstacles distance from this physics component. 
		//		const Ogre::Real brakingWeight = 0.2f;

		//		steeringForce.x = (closestIntersectingObstacle->getEntity()->getBoundingRadius() - localPosOfClosestObstacle.x) * brakingWeight;
		//	}

		//	// Finally, convert the steering vector from local to world space
		//	Ogre::Vector3 worldPosition = MathHelper::getInstance()->vectorToWorldSpace(steeringForce, heading, side);
		//	worldPosition.y = 0.0f;
		//	return worldPosition;
		//}

				Ogre::Vector3 MovingBehavior::flocking(Ogre::Real dt)
        {
            bool valid = false;
            Ogre::Vector3 sumVector = Ogre::Vector3::ZERO;

            if (true == this->isSwitchOn(FLOCKING_FORMATION_V_SHAPE))
            {
                // Attention: The formation already returns a desired velocity, so it must not be scaled
                // by weightSeparation, which has nothing to do with a formation slot.
                auto result = this->flockingFormationVShape();
                sumVector = result.second;
                valid |= result.first;

                return sumVector;
            }

            Ogre::Vector3 v1 = Ogre::Vector3::ZERO;
            Ogre::Vector3 v2 = Ogre::Vector3::ZERO;
            Ogre::Vector3 v3 = Ogre::Vector3::ZERO;
            Ogre::Vector3 v4 = Ogre::Vector3::ZERO;
            Ogre::Vector3 v5 = Ogre::Vector3::ZERO;

            if (true == this->isSwitchOn(FLOCKING_COHESION))
            {
                auto result = this->flockingRuleCohesion();
                v1 = result.second * this->weightCohesion;
                valid |= result.first;
            }
            if (true == this->isSwitchOn(FLOCKING_SEPARATION))
            {
                auto result = this->flockingRuleSeparation();
                v2 = result.second * this->weightSeparation;
                valid |= result.first;
            }
            else if (true == this->isSwitchOn(FLOCKING_SPREAD))
            {
                auto result = this->flockingRuleSpread();
                v2 = result.second * this->weightSeparation;
                valid |= result.first;
            }
            if (true == this->isSwitchOn(FLOCKING_ALIGNMENT))
            {
                auto result = this->flockingRuleAlignment();
                v3 = result.second * this->weightAlignment;
                valid |= result.first;
            }
            if (true == this->isSwitchOn(FLOCKING_FLEE))
            {
                auto result = this->flockingRuleFlee();
                v4 = result.second * this->weightFlee;
                valid |= result.first;
            }
            if (true == this->isSwitchOn(FLOCKING_SEEK))
            {
                auto result = this->flockingRuleSeek();
                v5 = result.second * this->weightSeek;
                valid |= result.first;
            }

            sumVector = v1 + v2 + v3 + v4 + v5;

            if (false == valid)
            {
                // Attention: Do NOT call setBehavior(NONE) here. The agents may simply be spread out at
                // spawn time and would come together again as soon as one of them moves into range.
                // Disabling the behavior here makes flocking silently dead for the rest of the level.
                if (this->neighborDistance > 0.0f && nullptr != this->agentStuckObserver)
                {
                    this->agentStuckObserver->onAgentStuck();
                }
                return Ogre::Vector3::ZERO;
            }

            if (sumVector.squaredLength() < 0.0001f)
            {
                return Ogre::Vector3::ZERO;
            }

            sumVector.normalise();
            return sumVector * this->agent->getSpeed();
        }

#if 0 // Seperation in circle effect ^^
		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingRuleSeparation(void)
		{
			bool valid = false;
			if (this->flockingAgents.empty())
			{
				return std::make_pair(valid, Ogre::Vector3::ZERO);
			}

			Ogre::Vector3 separationForce = Ogre::Vector3::ZERO;
			unsigned int neighborCount = 0;

			for (auto it = this->flockingAgents.cbegin(); it != this->flockingAgents.cend(); ++it)
			{
				if (agent == this->agent)
				{
					continue;
				}

				Ogre::Vector3 toAgent = this->agent->getPosition() - (*it)->getPosition();
				Ogre::Real distanceSquared = toAgent.squaredLength();

				if (this->neighborDistance == 0.0f || (distanceSquared > 0.0f && distanceSquared < this->neighborDistance * this->neighborDistance))
				{
					Ogre::Real distance = Ogre::Math::Sqrt(distanceSquared);
					if (distance > 0.0f)
					{
						// The closer the agent, the stronger the repulsion force
						Ogre::Vector3 repulsion = toAgent / (distance * distance);
						separationForce += repulsion;
						neighborCount++;
					}
				}
			}

			if (neighborCount > 0 && separationForce.squaredLength() > 0.0f)
			{
				separationForce /= static_cast<Ogre::Real>(neighborCount); // Get the average repulsion force
				separationForce.normalise(); // Ensure it has a consistent direction
				valid = true;
			}

			return std::make_pair(valid, separationForce);
		}

#endif

		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingRuleAlignment(void)
		{
			if (this->flockingAgents.empty())
			{
				return { false, Ogre::Vector3::ZERO };
			}

			Ogre::Vector3 alignmentVelocity = Ogre::Vector3::ZERO;
			unsigned int neighborCount = 0;
			Ogre::Real neighborDistSquared = this->neighborDistance * this->neighborDistance;

			// If targetAgent is available, use its position for seeking
			Ogre::Vector3 targetAlignment = Ogre::Vector3::ZERO;
			if (nullptr != this->targetAgent)
			{
				targetAlignment = this->seek(this->targetAgent->getPosition(), 0.016f); // Using 0.016f as deltaTime for the seek behavior
			}

			// Compute alignment based on neighbor velocities
			for (const auto& agent : this->flockingAgents)
			{
				if (agent == this->agent)
				{
					continue; // Skip the current agent
				}

				Ogre::Vector3 toAgent = this->agent->getPosition() - agent->getPosition();
				Ogre::Real distanceSquared = toAgent.squaredLength();

				if (this->neighborDistance == 0.0f || (distanceSquared > 0.0f && distanceSquared < neighborDistSquared))
				{
					alignmentVelocity += agent->getBody()->getVelocity();
					neighborCount++;
				}
			}

			if (neighborCount == 0 || alignmentVelocity.squaredLength() == 0.0f)
			{
				return { false, targetAlignment }; // Return the target alignment if no neighbors are present
			}

			alignmentVelocity /= static_cast<Ogre::Real>(neighborCount);
			alignmentVelocity = alignmentVelocity.normalisedCopy(); // Normalize for consistent direction

			return { true, targetAlignment + alignmentVelocity };
		}

		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingRuleCohesion(void)
		{
			if (this->flockingAgents.empty())
			{
				return { false, Ogre::Vector3::ZERO };
			}

			Ogre::Vector3 centerOfMass = Ogre::Vector3::ZERO;
			unsigned int neighborCount = 0;
			Ogre::Real neighborDistSquared = this->neighborDistance * this->neighborDistance;

			for (const auto& agent : this->flockingAgents)
			{
				if (agent == this->agent)
				{
					continue; // Skip the current agent
				}

				Ogre::Vector3 toAgent = this->agent->getPosition() - agent->getPosition();
				Ogre::Real distanceSquared = toAgent.squaredLength();

				if (this->neighborDistance == 0.0f || (distanceSquared > 0.0f && distanceSquared < neighborDistSquared))
				{
					centerOfMass += agent->getPosition();
					neighborCount++;
				}
			}

			if (neighborCount == 0)
			{
				return { false, Ogre::Vector3::ZERO };
			}

			centerOfMass /= static_cast<Ogre::Real>(neighborCount);
			Ogre::Vector3 steeringForce = this->seek(centerOfMass, 0);

			return { true, steeringForce.normalisedCopy() };
		}

		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingRuleSpread(void)
        {
            // Spread: Constant outward push, independent of the distance. This produces a loose, evenly
            // distributed group, whereas separation only reacts when neighbors get really close.
            if (true == this->flockingAgents.empty())
            {
                return {false, Ogre::Vector3::ZERO};
            }

            Ogre::Vector3 spreadForce = Ogre::Vector3::ZERO;
            unsigned int neighborCount = 0;
            const Ogre::Real neighborDistSquared = this->neighborDistance * this->neighborDistance;

            for (const auto& flockingAgent : this->flockingAgents)
            {
                if (flockingAgent == this->agent)
                {
                    continue;
                }

                Ogre::Vector3 toAgent = this->agent->getPosition() - flockingAgent->getPosition();
                Ogre::Real distanceSquared = toAgent.squaredLength();

                if (this->neighborDistance == 0.0f || (distanceSquared > 0.0f && distanceSquared < neighborDistSquared))
                {
                    spreadForce += toAgent;
                    neighborCount++;
                }
            }

            if (0 == neighborCount || spreadForce.squaredLength() < 0.0001f)
            {
                return {false, Ogre::Vector3::ZERO};
            }

            spreadForce /= static_cast<Ogre::Real>(neighborCount);
            return {true, spreadForce.normalisedCopy()};
        }

		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingRuleSeparation(void)
        {
            // Separation: Steers away from neighbors. The closer a neighbor, the stronger the repulsion.
            if (true == this->flockingAgents.empty())
            {
                return {false, Ogre::Vector3::ZERO};
            }

            Ogre::Vector3 separationForce = Ogre::Vector3::ZERO;
            unsigned int neighborCount = 0;

            const Ogre::Real separationStrength = 2.5f;
            const Ogre::Real maxSeparationForce = 3.0f;
            const Ogre::Real neighborDistSquared = this->neighborDistance * this->neighborDistance;

            for (const auto& flockingAgent : this->flockingAgents)
            {
                if (flockingAgent == this->agent)
                {
                    continue;
                }

                Ogre::Vector3 toAgent = this->agent->getPosition() - flockingAgent->getPosition();
                Ogre::Real distanceSquared = toAgent.squaredLength();

                if (this->neighborDistance == 0.0f || (distanceSquared > 0.0f && distanceSquared < neighborDistSquared))
                {
                    Ogre::Real distance = Ogre::Math::Sqrt(distanceSquared);
                    if (distance > 0.01f)
                    {
                        separationForce += (toAgent / distance) * (separationStrength / distance);
                        neighborCount++;
                    }
                }
            }

            if (0 == neighborCount || separationForce.squaredLength() < 0.0001f)
            {
                return {false, Ogre::Vector3::ZERO};
            }

            if (separationForce.squaredLength() > maxSeparationForce * maxSeparationForce)
            {
                separationForce.normalise();
                separationForce *= maxSeparationForce;
            }

            return {true, separationForce};
        }

#if 0
		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingFormationVShape()
		{
			if (this->flockingAgents.empty())
			{
				return { false, Ogre::Vector3::ZERO };
			}

			// Determine the leader
			PhysicsActiveComponent* leader = this->targetAgent ? this->targetAgent : this->flockingAgents.front();

			// The leader does NOT follow the V-formation
			if (this->agent == leader)
			{
				return { false, Ogre::Vector3::ZERO };
			}

			// Get leader's position & velocity direction
			Ogre::Vector3 leaderPos = leader->getPosition();
			Ogre::Vector3 leaderVel = leader->getBody()->getVelocity().normalisedCopy();

			// Prevent division by zero
			if (leaderVel.squaredLength() < 0.01f)
			{
				return { false, Ogre::Vector3::ZERO };
			}

			// **Revised side direction**: Compute perpendicular to the leader's velocity in the horizontal plane
			Ogre::Vector3 sideDir = leaderVel.perpendicular().normalisedCopy();

			// Find agent's index in the flock
			auto it = std::find(this->flockingAgents.begin(), this->flockingAgents.end(), this->agent);
			if (it == this->flockingAgents.end())
			{
				return { false, Ogre::Vector3::ZERO };
			}

			size_t index = std::distance(this->flockingAgents.begin(), it) - 1; // -1 to exclude leader
			if (index >= this->flockingAgents.size())
			{
				return { false, Ogre::Vector3::ZERO };
			}

			// Compute position in the V-formation
			int row = (index / 2) + 1;
			Ogre::Real depthOffset = row * this->neighborDistance * 2.5f;
			Ogre::Real sideOffset = row * this->neighborDistance * (index % 2 == 0 ? -1.0f : 1.0f);

			// Prevent vertical stacking: Maintain same altitude as leader
			Ogre::Vector3 targetPos = leaderPos + (-leaderVel * depthOffset) + (sideDir * sideOffset);
			targetPos.y = leaderPos.y;

			// Move toward target position
			Ogre::Vector3 toTarget = targetPos - this->agent->getPosition();
			Ogre::Real distance = toTarget.length();

			// Apply velocity change
			Ogre::Vector3 desiredVelocity = toTarget.normalisedCopy() * this->agent->getSpeed();

			// Clamp the velocity to the agent's maximum speed
			if (desiredVelocity.squaredLength() > this->agent->getMaxSpeed() * this->agent->getMaxSpeed())
			{
				desiredVelocity.normalise();
				desiredVelocity *= this->agent->getMaxSpeed();
			}

			// Calculate the velocity change required
			Ogre::Vector3 velocityChange = desiredVelocity - this->agent->getBody()->getVelocity();

			// Smooth out the velocity change (optional)
			Ogre::Real smoothingFactor = 0.1f; // You can tweak this value
			velocityChange *= smoothingFactor;

			return { true, velocityChange };
		}
#endif

#if 0

		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingFormationVShape(void)
		{
			if (this->flockingAgents.empty())
			{
				return { false, Ogre::Vector3::ZERO };
			}

			// Determine the leader (targetAgent if available, otherwise first agent)
			PhysicsActiveComponent* leader = this->targetAgent ? this->targetAgent : this->flockingAgents.front();

			// Get the leader's position and velocity
			Ogre::Vector3 leaderPos = leader->getPosition();
			Ogre::Vector3 leaderVel = leader->getBody()->getVelocity().normalisedCopy();

			// Formation parameters
			Ogre::Real baseSpacing = this->neighborDistance * 4.5f; // Further apart
			Ogre::Real rowSpacing = baseSpacing * 2.5f; // Increase row spacing
			Ogre::Vector3 vDirection = leaderVel.perpendicular();
			bool leftSide = true; // Alternate sides

			// Step 1: Calculate the desired target position for each agent
			std::vector<Ogre::Vector3> targetPositions;
			unsigned int index = 0;

			for (const auto& agent : this->flockingAgents)
			{
				if (agent == leader)
				{
					targetPositions.push_back(leaderPos); // Leader stays at its position
					continue;
				}

				// Determine row and offset in V-formation
				Ogre::Real row = (index / 2) + 1; // Every two agents, go to the next row
				Ogre::Real sideOffset = row * baseSpacing * 2.0f; // Wider spacing for wings
				Ogre::Real depthOffset = row * rowSpacing; // More depth

				Ogre::Vector3 offset = leaderVel * (-depthOffset) + vDirection * (leftSide ? -sideOffset : sideOffset);
				Ogre::Vector3 targetPos = leaderPos + offset;

				targetPositions.push_back(targetPos);

				// Switch sides for the next agent
				leftSide = !leftSide;
				index++;
			}

			// Step 2: Calculate forces for each agent to move towards the target position
			Ogre::Vector3 totalForce = Ogre::Vector3::ZERO;

			index = 0;
			for (const auto& agent : this->flockingAgents)
			{
				if (agent == leader)
				{
					continue; // Skip the leader
				}

				// Get the target position for this agent
				Ogre::Vector3 targetPos = targetPositions[index];

				// Local seek force per agent
				Ogre::Vector3 desiredVelocity = targetPos - agent->getPosition();
				Ogre::Real distance = desiredVelocity.length();

				// Normalize only if distance is not zero
				if (distance > 0.0f)
				{
					desiredVelocity /= distance; // Normalize
					desiredVelocity *= agent->getSpeed();

					// If agents are still melting together, apply a separation force
					if (distance < baseSpacing * 0.8f) // If too close, spread out
					{
						desiredVelocity *= 1.5f; // Push them apart more
					}
					else if (distance > baseSpacing * 2.0f) // If too far, slow them down
					{
						desiredVelocity *= 0.7f;
					}
				}

				// Accumulate forces
				totalForce += (desiredVelocity - agent->getBody()->getVelocity());

				index++;
			}

			// Normalize total force for consistency
			totalForce /= static_cast<Ogre::Real>(this->flockingAgents.size()) * 2.0f;

			return { true, totalForce };
		}
#endif

#if 1

				std::pair<bool, Ogre::Vector3> MovingBehavior::flockingFormationVShape(void)
        {
            if (true == this->flockingAgents.empty())
            {
                return {false, Ogre::Vector3::ZERO};
            }

            PhysicsActiveComponent* leader = (nullptr != this->targetAgent) ? this->targetAgent : this->flockingAgents.front();
            if (this->agent == leader)
            {
                return {false, Ogre::Vector3::ZERO}; // The leader does not follow the formation
            }

            Ogre::Vector3 leaderPosition = leader->getPosition();

            Ogre::Vector3 leaderDirection = leader->getVelocity();
            if (false == this->flyMode)
            {
                leaderDirection.y = 0.0f;
            }

            if (leaderDirection.squaredLength() < 0.01f)
            {
                // Attention: If the leader stands still, its velocity is useless. Fall back to its facing
                // direction, else the whole formation collapses onto the leader.
                leaderDirection = leader->getOrientation() * leader->getOwner()->getDefaultDirection();
            }

            if (leaderDirection.squaredLength() < 0.0001f)
            {
                return {false, Ogre::Vector3::ZERO};
            }
            leaderDirection.normalise();

            Ogre::Vector3 sideDirection = leaderDirection.crossProduct(Ogre::Vector3::UNIT_Y);
            if (sideDirection.squaredLength() < 0.0001f)
            {
                sideDirection = leaderDirection.crossProduct(Ogre::Vector3::UNIT_X);
            }
            sideDirection.normalise();

            // Determines the slot index. Attention: Only the followers BEFORE this agent are counted and the
            // leader is skipped explicitly. The former 'distance(begin, it) - 1' underflowed the size_t when
            // the leader was the target agent and therefore not part of the flocking agents at all.
            size_t slot = 0;
            bool found = false;
            for (const auto& flockingAgent : this->flockingAgents)
            {
                if (flockingAgent == this->agent)
                {
                    found = true;
                    break;
                }
                if (flockingAgent == leader)
                {
                    continue;
                }
                slot++;
            }

            if (false == found)
            {
                return {false, Ogre::Vector3::ZERO};
            }

            Ogre::Real spacing = this->neighborDistance;
            if (spacing <= 0.0f)
            {
                spacing = this->agent->getOwner()->getSize().length();
            }
            if (spacing <= 0.0f)
            {
                spacing = 2.0f;
            }

            const int row = static_cast<int>(slot / 2) + 1;
            const Ogre::Real depthOffset = static_cast<Ogre::Real>(row) * spacing * 1.5f;
            const Ogre::Real sideOffset = static_cast<Ogre::Real>(row) * spacing * ((0 == slot % 2) ? -1.0f : 1.0f);

            Ogre::Vector3 targetPosition = leaderPosition - (leaderDirection * depthOffset) + (sideDirection * sideOffset);

            if (false == this->flyMode)
            {
                targetPosition.y = this->agent->getPosition().y;
            }

            Ogre::Vector3 toTarget = targetPosition - this->agent->getPosition();
            Ogre::Real distance = toTarget.length();

            if (distance < 0.0001f)
            {
                return {true, Ogre::Vector3::ZERO};
            }

            // Arrive like damping, so the agent does not oscillate around its slot
            Ogre::Real speedFactor = distance / (spacing * 0.5f);
            if (speedFactor > 1.0f)
            {
                speedFactor = 1.0f;
            }

            // Attention: No applyForce / applyOmegaForceRotateTo here! This function must be free of side
            // effects, because update() applies the returned velocity exactly once. Doing both doubles it.
            return {true, (toTarget / distance) * (speedFactor * this->agent->getSpeed())};
        }


#endif

		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingRuleFlee(void)
        {
            if (true == this->flockingAgents.empty() || nullptr == this->targetAgent)
            {
                return {false, Ogre::Vector3::ZERO};
            }

            Ogre::Vector3 awayFromTarget = this->agent->getPosition() - this->targetAgent->getPosition();
            Ogre::Real distance = awayFromTarget.length();

            if (distance < 0.0001f)
            {
                return {false, Ogre::Vector3::ZERO};
            }

            if (distance < 0.5f)
            {
                distance = 0.5f;
            }

            // The panic is strongest when the hunter is close and fades out with the distance
            return {true, (awayFromTarget / distance) * (1.0f / distance)};
        }

				std::pair<bool, Ogre::Vector3> MovingBehavior::flockingRuleSeek(void)
        {
            if (true == this->flockingAgents.empty() || nullptr == this->targetAgent)
            {
                return {false, Ogre::Vector3::ZERO};
            }

            // Attention: The former version returned '(1/distance) * targetPosition', which is an absolute
            // WORLD POSITION scaled down. The resulting force therefore depended on how far the level is
            // away from the world origin. It must be a direction from the agent to the target.
            Ogre::Vector3 toTarget = this->targetAgent->getPosition() - this->agent->getPosition();

            if (toTarget.squaredLength() < 0.0001f)
            {
                return {false, Ogre::Vector3::ZERO};
            }

            return {true, toTarget.normalisedCopy()};
        }

		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingRuleBorder(void)
		{
			// Agenten sollen sich innerhalb der Weltgrenzen bewegen
			Ogre::Vector3 fleeVec = Ogre::Vector3::ZERO;
			/*OgreNewt::BasicRaycast::BasicRaycastInfo contact = currentSheep->getContactAhead(3.0);
			Ogre::Quaternion quat = Ogre::Quaternion::IDENTITY;
			if (contact.mBody)
			{
			if ((contact.mBody->getType() != Utilities::TERRAIN) || (contact.mBody->getType() != Utilities::SHEEP))
			{
			//In negative Richtung abhngig von der Position des Objektes bewegen
			//je grer die Distanz zum Objekt, desto wenig wird sich in die entgegende Richtung bewegt
			Ogre::Vector3 fencePos = Ogre::Vector3::ZERO;
			Ogre::Quaternion fenceOrient = Ogre::Quaternion::IDENTITY;
			contact.mBody->getPositionOrientation(fencePos, fenceOrient);
			Ogre::Real distance = (currentSheep->getPosition() - fencePos).length();
			fleeVec = -(1.0f / distance) * fencePos;
			}
			}*/
			
			auto contact = this->agent->getContactAhead(11, Ogre::Vector3(0.0f, 0.4f, 0.0f), 4.0f);
			if (contact.getHitGameObject())
			{
				// if (info.mDistance > 4.0f)
				{
					//Ogre::Real distance = (currentSheep->getPosition() - itr->movable->getParentSceneNode()->getPosition()).length();
					//fleeVec = -(1.0f / distance) * itr->movable->getParentSceneNode()->getPosition();
					fleeVec = this->agent->getPosition() - contact.getHitGameObject()->getPosition();
					/*Ogre::Vector3 vec = Ogre::Vector3::UNIT_X;
					Ogre::Vector3 normal = ray.getDirection();

					//if (normal != Ogre::Vector3::ZERO)
					Ogre::Real deg = Ogre::Math::ACos(vec.dotProduct(normal) / (vec.length() * normal.length())).valueDegrees();
					if (deg >= 90)
					currentSheep->setAngularVelocity(2.0f);
					else
					currentSheep->setAngularVelocity(-2.0f);*/
				}
			}
			return std::make_pair(true, fleeVec);
		}

		std::pair<bool, Ogre::Vector3> MovingBehavior::flockingObstacleAvoidance(void)
		{
			auto contact = this->agent->getContactAhead(12, Ogre::Vector3(0.0f, 0.4f, 0.0f), 2.0f);
			if (contact.getHitGameObject())
			{
				// if (info.distance <= 3.0f)
				{
					Ogre::Vector3 vec = Ogre::Vector3::UNIT_X;
					Ogre::Vector3 normal = contact.getNormal();

					Ogre::Real deg = Ogre::Math::ACos(vec.dotProduct(normal) / (vec.length() * normal.length())).valueDegrees();
					if (deg >= 90.0f)
						this->agent->applyOmegaForce(Ogre::Vector3(0.0f, 2.0f, 0.0f));
					else
						this->agent->applyOmegaForce(Ogre::Vector3(0.0f, -2.0f, 0.0f));
					return std::make_pair(true, Ogre::Vector3::ZERO);
				}
			}
			this->agent->applyOmegaForce(Ogre::Vector3::ZERO);

			return std::make_pair(true, Ogre::Vector3::ZERO);
		}

		MovingBehavior::BehaviorType MovingBehavior::mapBehavior(const Ogre::String& behavior)
		{
			if (behavior == "MoveRandomly")
			{
				return MovingBehavior::MOVE_RANDOMLY;
			}
			else if (behavior == "Move")
			{
				return MovingBehavior::MOVE;
			}
			else if (behavior == "None")
			{
				return MovingBehavior::NONE;
			}
			else if (behavior == "Stop")
			{
				return MovingBehavior::STOP;
			}
			else if (behavior == "Flee")
			{
				return MovingBehavior::FLEE;
			}
			else if (behavior == "Seek")
			{
				return MovingBehavior::SEEK;
			}
			else if (behavior == "Arrive")
			{
				return MovingBehavior::ARRIVE;
			}
			else if (behavior == "FollowPath")
			{
				return MovingBehavior::FOLLOW_PATH;
			}
			else if (behavior == "Interpose")
			{
				return MovingBehavior::INTERPOSE;
			}
			else if (behavior == "Wander")
			{
				return MovingBehavior::WANDER;
			}
			else if (behavior == "PathFindingWander")
			{
				return MovingBehavior::PATH_FINDING_WANDER;
			}
			else if (behavior == "Pursuit")
			{
				return MovingBehavior::PURSUIT;
			}
			else if (behavior == "OffsetPursuit")
			{
				return MovingBehavior::OFFSET_PURSUIT;
			}
			else if (behavior == "Evade")
			{
				return MovingBehavior::EVADE;
			}
			else if (behavior == "ObstacleAvoidance")
			{
				return MovingBehavior::OBSTACLE_AVOIDANCE;
			}
			else if (behavior == "Hide")
			{
				return MovingBehavior::HIDE;
			}
			else if (behavior == "FlockingCohesion")
			{
				return MovingBehavior::FLOCKING_COHESION;
			}
			else if (behavior == "FlockingSeparation")
			{
				return MovingBehavior::FLOCKING_SEPARATION;
			}
			else if (behavior == "FlockingSpread")
			{
				return MovingBehavior::FLOCKING_SPREAD;
			}
			else if (behavior == "flockingFormationVShape")
			{
				return MovingBehavior::FLOCKING_FORMATION_V_SHAPE;
			}
			else if (behavior == "FlockingAlignment")
			{
				return MovingBehavior::FLOCKING_ALIGNMENT;
			}
			else if (behavior == "Flocking")
			{
				return MovingBehavior::FLOCKING;
			}
			else if (behavior == "Flee2D")
			{
				return MovingBehavior::FLEE_2D;
			}
			else if (behavior == "Seek2D")
			{
				return MovingBehavior::SEEK_2D;
			}
			else if (behavior == "Arrive2D")
			{
				return MovingBehavior::ARRIVE_2D;
			}
			else if (behavior == "FollowPath2D")
			{
				return MovingBehavior::FOLLOW_PATH_2D;
			}
			else if (behavior == "Wander2D")
			{
				return MovingBehavior::WANDER_2D;
			}
			else if (behavior == "Pursuit2D")
			{
				return MovingBehavior::PURSUIT_2D;
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Cannot map behavior, because the: '" + behavior + "' does not exist. In game object: '"
					+ this->agent->getOwner()->getName() + "'");
				return MovingBehavior::NONE;
			}
		}

		Ogre::String MovingBehavior::behaviorToString(MovingBehavior::BehaviorType behaviorType)
		{
			switch (behaviorType)
			{
			case MovingBehavior::MOVE_RANDOMLY:
				return "MoveRandomly";
			case MovingBehavior::MOVE:
				return "Move";
			case MovingBehavior::NONE:
				return "None";
			case MovingBehavior::STOP:
				return "Stop";
			case MovingBehavior::FLEE:
				return "Flee";
			case MovingBehavior::SEEK:
				return "Seek";
			case MovingBehavior::ARRIVE:
				return "Arrive";
			case MovingBehavior::FOLLOW_PATH:
				return "FollowPath";
			case MovingBehavior::INTERPOSE:
				return "Interpose";
			case MovingBehavior::WANDER:
				return "Wander";
			case MovingBehavior::PATH_FINDING_WANDER:
				return "PathFindingWander";
			case MovingBehavior::PURSUIT:
				return "Pursuit";
			case MovingBehavior::OFFSET_PURSUIT:
				return "OffsetPursuit";
			case MovingBehavior::EVADE:
				return "Evade";
			case MovingBehavior::OBSTACLE_AVOIDANCE:
				return "ObstacleAvoidance";
			case MovingBehavior::HIDE:
				return "Hide";
			case MovingBehavior::FLOCKING_COHESION:
				return "FlockingCohesion";
			case MovingBehavior::FLOCKING_SEPARATION:
				return "FlockingSeparation";
			case MovingBehavior::FLOCKING_SPREAD:
				return "FlockingSpread";
			case MovingBehavior::FLOCKING_FORMATION_V_SHAPE:
				return "FlockingFormationVShape";
			case MovingBehavior::FLOCKING_ALIGNMENT:
				return "FlockingAlignment";
			case MovingBehavior::FLOCKING:
				return "Flocking";
			case MovingBehavior::FLEE_2D:
				return "Flee2D";
			case MovingBehavior::SEEK_2D:
				return "Seek2D";
			case MovingBehavior::ARRIVE_2D:
				return "Arrive2D";
			case MovingBehavior::FOLLOW_PATH_2D:
				return "FollowPath2D";
			case MovingBehavior::WANDER_2D:
				return "Wander2D";
			case MovingBehavior::PURSUIT_2D:
				return "Pursuit2D";
			}
			return "";
		}

		void MovingBehavior::setObstacleHideData(const Ogre::String& obstaclesHideCategories, Ogre::Real obstacleRangeRadius)
		{
			this->obstaclesHide = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(obstaclesHideCategories);
			if (true == this->obstaclesHide.empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Could find category id for name: " + obstaclesHideCategories
					+ " in 'setObstacleHideData'. Hence hide behavior will not work for game object: " + this->agent->getOwner()->getName());
			}
			this->obstacleHideRangeRadius = obstacleRangeRadius;
		}

		void MovingBehavior::setObstacleAvoidanceData(const Ogre::String& obstaclesAvoidanceCategories, Ogre::Real obstacleRangeRadius)
		{
			this->obstaclesAvoidanceCategoryIds = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(obstaclesAvoidanceCategories);
			if (0 == this->obstaclesAvoidanceCategoryIds)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Could find category id for name: " + obstaclesAvoidanceCategories
					+ " in 'setObstacleAvoidanceData'. Hence obstacle avoidance behavior will not work for game object: " + this->agent->getOwner()->getName());
			}
			this->obstacleAvoidanceRangeRadius = obstacleRangeRadius;
		}

		void MovingBehavior::setNeighborDistance(Ogre::Real neighborDistance)
		{
			this->neighborDistance = neighborDistance;
		}

		Ogre::Real MovingBehavior::getNeighborDistance(void) const
		{
			return this->neighborDistance;
		}

		void MovingBehavior::setFlockingAgents(const std::vector<unsigned long>& flockingAgentIds)
		{
			this->flockingAgents.clear();
			for (size_t i = 0; i < flockingAgentIds.size(); i++)
			{
				GameObjectPtr flockingAgentGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(flockingAgentIds[i]);
				if (nullptr != flockingAgentGameObjectPtr)
				{
					auto physicsActiveComponent = NOWA::makeStrongPtr(flockingAgentGameObjectPtr->getComponent<PhysicsActiveComponent>());
					if (nullptr != physicsActiveComponent)
					{
						this->flockingAgents.emplace_back(physicsActiveComponent.get());
					}
				}
			}
		}

		void MovingBehavior::setBehavior(MovingBehavior::BehaviorType behaviorType)
		{
            if (NONE == behaviorType || STOP == behaviorType)
			{
                if (nullptr != this->agent)
                {
                    this->agent->clearLatchedVelocity();
                }
			}

			this->mask = 0;
			this->mask |= behaviorType;

			this->pathSlot = -1;
			this->targetSlot = -1;
			this->drawPath = false;
			this->actualizePathDelay = -1.0f;
			this->isStuck = false;
			this->currentBehavior = this->behaviorToString(behaviorType);
		}

		void MovingBehavior::addBehavior(BehaviorType behaviorType)
		{
			/*if ((FLEE == behaviorType || SEEK == behaviorType || ARRIVE == behaviorType || PURSUIT == behaviorType || EVADE == behaviorType)
				&& nullptr == this->targetAgent)
			{
				throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND,
					"[MovingBehavior]: Cannot switch behavior type: '" + Ogre::StringConverter::toString(behaviorType) + "' on, since there is no target PhysicsActiveComponent specified. Call setTargetAgent(...) first.", "MovingBehavior::setBehavior");
			}*/

			/*if (BehaviorType::FOLLOW_PATH == behaviorType || BehaviorType::FOLLOW_PATH_2D == behaviorType)
			{
				if (nullptr != this->pPath)
				{
					this->pPath->clear();
				}
			}*/
			// Stop and None must always be removed prior because they are contrary against all other behaviors
			this->mask &= ~NONE;
			this->mask &= ~STOP;

			this->mask |= behaviorType;
		}

		void MovingBehavior::removeBehavior(BehaviorType behaviorType)
		{
			if (this->isSwitchOn(behaviorType))
			{
				this->mask ^= behaviorType;
			}
			// Do not set to none, because else, its not possible to react, when resultVelocity is zero, to e.g. stop a physics player controller etc.
#if 0
			if (0 == this->mask)
			{
				this->mask = NONE;
			}
#endif
		}

		void MovingBehavior::setBehavior(const Ogre::String& behavior)
		{
			BehaviorType behaviorType = mapBehavior(behavior);

			this->pathSlot = -1;
			this->targetSlot = -1;
			this->drawPath = false;
			this->actualizePathDelay = -1.0f;
			this->isStuck = false;

			this->mask = 0;
			this->mask |= behaviorType;
		}

		void MovingBehavior::addBehavior(const Ogre::String& behavior)
		{
			BehaviorType behaviorType = mapBehavior(behavior);
			// Stop and None must always be removed prior because they are contrary against all other behaviors
			this->mask &= ~NONE;
			this->mask &= ~STOP;

			this->mask |= behaviorType;
		}

		void MovingBehavior::removeBehavior(const Ogre::String& behavior)
		{
			BehaviorType behaviorType = mapBehavior(behavior);
			if (this->isSwitchOn(behaviorType))
			{
				this->mask &= ~behaviorType;
			}
		}

		void MovingBehavior::setBehaviorMask(unsigned int mask)
		{
			this->mask = mask;
		}

		void MovingBehavior::setPathGoalObserver(IPathGoalObserver* pathGoalObserver)
		{
			this->pathGoalObserver = pathGoalObserver;
		}

		void MovingBehavior::setAgentStuckObserver(IAgentStuckObserver* agentStuckObserver)
		{
			this->agentStuckObserver = agentStuckObserver;
		}

		Ogre::Real MovingBehavior::getTurnRate(void) const
        {
            // setRotationSpeed stores the value multiplied by 60, because the player controller branch
            // works in degrees per frame. The dynamic body branch needs the raw user value back.
            Ogre::Real turnRate = this->rotationSpeed / 60.0f;

            if (turnRate < 0.1f)
            {
                turnRate = 0.1f;
            }

            return turnRate;
        }

        void MovingBehavior::updateOrientation(const Ogre::Vector3& forward, const Ogre::Vector3& gravityDir, Ogre::Real dt)
        {
            if (nullptr == this->agent)
            {
                return;
            }

            if (false == this->autoOrientation)
            {
                this->agent->applyOmegaForceRotateTo(this->agent->getOrientation(), gravityDir);
                return;
            }

            Ogre::Vector3 worldUp = -gravityDir;
            if (worldUp.squaredLength() < 0.0001f)
            {
                worldUp = Ogre::Vector3::UNIT_Y;
            }
            worldUp.normalise();

            if (false == this->flyMode)
            {
                // Ground agent: Keep upright, only correct pitch and roll, never the yaw
                Ogre::Vector3 currentAgentUp = this->agent->getOrientation() * Ogre::Vector3::UNIT_Y;
                Ogre::Real angleDeviation = Ogre::Math::ACos(Ogre::Math::Clamp(currentAgentUp.dotProduct(worldUp), -1.0f, 1.0f)).valueDegrees();

                const Ogre::Real tiltThresholdAngle = 20.0f;

                if (angleDeviation > tiltThresholdAngle)
                {
                    Ogre::Quaternion uprightRotation = Ogre::Vector3::UNIT_Y.getRotationTo(worldUp);
                    Ogre::Vector3 correctionAxes(1.0f, 0.0f, 1.0f);
                    this->agent->applyOmegaForceRotateTo(uprightRotation, correctionAxes, this->getTurnRate());
                }
                else
                {
                    this->agent->applyOmegaForceRotateToDirection(forward, this->getTurnRate());
                }
                return;
            }

            // Fly mode.
            //
            // Attention: applyOmegaForceRotateToDirection alone is NOT enough here. It only aligns the
            // forward axis and leaves the rotation AROUND that axis completely free, so the creature
            // slowly tumbles around its own roll axis. A full target orientation must be built from the
            // forward direction plus an explicitly chosen up vector.
            Ogre::Vector3 currentForward = this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection();

            // Signed horizontal turn amount, used to bank into the curve
            Ogre::Vector3 currentFlat = currentForward - worldUp * currentForward.dotProduct(worldUp);
            Ogre::Vector3 targetFlat = forward - worldUp * forward.dotProduct(worldUp);

            Ogre::Real targetBank = 0.0f;
            if (currentFlat.squaredLength() > 0.0001f && targetFlat.squaredLength() > 0.0001f)
            {
                currentFlat.normalise();
                targetFlat.normalise();

                // Positive when the agent has to turn to its left
                const Ogre::Real sinTurn = worldUp.dotProduct(currentFlat.crossProduct(targetFlat));

                const Ogre::Radian maxBank(Ogre::Degree(15.0f));
                targetBank = -sinTurn * maxBank.valueRadians() * 3.0f;

                targetBank = Ogre::Math::Clamp(targetBank, -maxBank.valueRadians(), maxBank.valueRadians());
            }

            // Smooths the bank, so the creature rolls into and out of the curve instead of snapping
            const Ogre::Real bankSmoothing = Ogre::Math::Clamp(4.0f * dt, 0.0f, 1.0f);
            this->currentBankAngle += (targetBank - this->currentBankAngle) * bankSmoothing;

            Ogre::Vector3 desiredUp = Ogre::Quaternion(Ogre::Radian(this->currentBankAngle), forward) * worldUp;

            Ogre::Vector3 zAxis = forward;
            Ogre::Vector3 xAxis = desiredUp.crossProduct(zAxis);
            if (xAxis.squaredLength() < 0.0001f)
            {
                // Flying almost straight up or down: any perpendicular axis will do
                xAxis = Ogre::Vector3::UNIT_X.crossProduct(zAxis);
            }
            if (xAxis.squaredLength() < 0.0001f)
            {
                xAxis = Ogre::Vector3::UNIT_Z.crossProduct(zAxis);
            }
            xAxis.normalise();

            Ogre::Vector3 yAxis = zAxis.crossProduct(xAxis);
            yAxis.normalise();

            Ogre::Quaternion targetOrientation(xAxis, yAxis, zAxis);

            // Compensates the default direction of the mesh. The axis frame above assumes the mesh looks
            // along +Z in local space. Attention: If applyOmegaForceRotateTo already accounts for the
            // default direction internally, remove this correction, else the agent is rotated twice.
            const Ogre::Vector3 defaultDirection = this->agent->getOwner()->getDefaultDirection();
            if (false == defaultDirection.positionEquals(Ogre::Vector3::UNIT_Z, 0.001f))
            {
                Ogre::Quaternion defaultCorrection = Ogre::Vector3::UNIT_Z.getRotationTo(defaultDirection);
                targetOrientation = targetOrientation * defaultCorrection.Inverse();
            }

            this->agent->applyOmegaForceRotateTo(targetOrientation, Ogre::Vector3::UNIT_SCALE, this->getTurnRate());
        }

		void MovingBehavior::update(Ogre::Real dt)
        {
            // If none is on, add no force, so that other behaviors can still move the agent! Only stop adds force, even if its null
            if (true == this->isSwitchOn(NONE))
            {
                return;
            }

            if (nullptr == this->agent)
            {
                return;
            }

            // Apply the physics velocity according to the resulting behavior
            Ogre::Vector3 resultVelocity = this->calculate(dt);
            Ogre::Vector3 gravityDir = this->agent->getGravityDirection();

            if (nullptr != this->crowdComponent)
            {
                if (Ogre::Vector3::ZERO != resultVelocity)
                {
                    // TODO: gravityDir for planets movement
                    this->crowdComponent->setVelocity(resultVelocity);
                    resultVelocity = this->crowdComponent->beginUpdateVelocity();
                }
            }

            // Determines the direction the agent shall face.
            // In fly mode the full 3d velocity is used, so that the agent may pitch up and down,
            // else the velocity is projected onto the plane perpendicular to the gravity direction.
            Ogre::Vector3 forward = Ogre::Vector3::ZERO;
            if (true == this->flyMode)
            {
                forward = resultVelocity;
            }
            else
            {
                forward = resultVelocity - gravityDir * resultVelocity.dotProduct(gravityDir);
            }

            if (forward.squaredLength() > 0.0001f)
            {
                forward.normalise();
            }
            else
            {
                // Attention: Ogre does not normalise a zero length vector, so it would stay zero and the agent
                // would be rotated to an undefined direction. Hence keep the current facing direction.
                forward = this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection();
            }

            Ogre::Real velocityLength = this->agent->getVelocity().length();
            if (true == Ogre::Math::RealEqual(velocityLength, 0.0f))
            {
                this->motionDistanceChange = 0.1f;
            }
            else
            {
                this->motionDistanceChange = velocityLength / this->agent->getSpeed();
                if (this->motionDistanceChange < 0.1f)
                {
                    this->motionDistanceChange = 0.1f;
                }

                this->motionDistanceChange = NOWA::MathHelper::getInstance()->lowPassFilter(this->motionDistanceChange, this->lastMotionDistanceChange, 0.5f);
            }

            if (this->motionDistanceChange > 1.0f)
            {
                this->motionDistanceChange = 1.0f;
            }

            // Apply animation speed
            if (nullptr != this->animationBlender)
            {
                this->animationBlender->addTime(dt * this->oldAnimationSpeed * this->motionDistanceChange, "MovingBehavior");
            }

            this->lastMotionDistanceChange = this->motionDistanceChange;

            this->detectAgentMotionChange(dt);

            // Velocity should not be used for dynamic bodies, player controller is a kinematic body.
            // Kinematic bodies are rigid bodies that do not part of the dynamic resolution, they are only part of the collision.
            // Kinematic bodies should be controlled via velocity!
            PhysicsPlayerControllerComponent* physicsPlayerControllerComponent = dynamic_cast<PhysicsPlayerControllerComponent*>(this->agent);

            if (nullptr != physicsPlayerControllerComponent)
            {
                if (false == this->flyMode)
                {
                    resultVelocity.y = 0.0f;
                }

                Ogre::Radian heading = this->agent->getOrientation().getYaw();

                // If there is a ai movement going on, set the new orientation
                if (Ogre::Vector3::ZERO != resultVelocity)
                {
                    if (true == this->autoOrientation)
                    {
                        this->updateOrientation(forward, gravityDir, dt);
                    }
                }

                // TODO: gravityDir for planet movement
                physicsPlayerControllerComponent->move(this->agent->getSpeed() * resultVelocity.length(), 0.0f, heading);
            }
            else
            {
                // Kinematic
                PhysicsActiveKinematicComponent* physicsActiveKinematicComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(this->agent);
                if (nullptr != physicsActiveKinematicComponent)
                {
                    Ogre::Vector3 targetVelocity = resultVelocity;

                    physicsActiveKinematicComponent->setVelocity(targetVelocity);

                    if (false == resultVelocity.isZeroLength())
                    {
                        // Attention: newOrientation must be initialized, else the else branch would rotate to identity
                        Ogre::Quaternion newOrientation = this->agent->getOrientation();
                        if (true == this->autoOrientation)
                        {
                            newOrientation = (this->agent->getOrientation() * this->agent->getOwner()->getDefaultDirection()).getRotationTo(resultVelocity);
                            this->agent->setOmegaVelocity(Ogre::Vector3(0.0f, newOrientation.getYaw().valueDegrees() * 0.1f, 0.0f));
                        }
                        else
                        {
                            this->agent->setOmegaVelocityRotateTo(newOrientation, Ogre::Vector3(0.0f, 1.0f, 0.0f));
                        }
                    }
                }
                // Usual force
                else
                {
                    Ogre::Vector3 forceForVelocity = Ogre::Vector3::ZERO;

                    if (false == this->flyMode)
                    {
                        // Compute vertical velocity along the gravity direction
                        Ogre::Vector3 verticalVelocity = gravityDir * this->agent->getVelocity().dotProduct(gravityDir);

                        // Compute movement direction (excluding vertical component)
                        Ogre::Vector3 directionMove = resultVelocity - (resultVelocity.dotProduct(gravityDir) * gravityDir);

                        // Combine vertical velocity with movement direction
                        forceForVelocity = verticalVelocity + directionMove;
                    }
                    else
                    {
                        // In fly mode, use resultVelocity directly
                        forceForVelocity = resultVelocity;
                    }

                    // Apply the calculated force
                    this->agent->applyRequiredForceForVelocity(forceForVelocity);

                    if (true == this->autoOrientation)
                    {
                        if (true == this->flyMode)
                        {
                            // Attention: No upright correction in fly mode! It would immediately fight against
                            // any pitch of the agent, so the flying creature could never look up or down.
                            this->agent->applyOmegaForceRotateToDirection(forward, this->getTurnRate());
                        }
                        else
                        {
                            // Get the player's current up vector based on the current orientation
                            Ogre::Vector3 currentPlayerUp = this->agent->getOrientation() * Ogre::Vector3::UNIT_Y;

                            // Compute angle deviation between player's up and gravity up
                            Ogre::Real angleDeviation = Ogre::Math::ACos(currentPlayerUp.dotProduct(-gravityDir)).valueDegrees();

                            // Prevents fall over
                            const Ogre::Real tiltThresholdAngle = Ogre::Degree(20.0f).valueDegrees();

                            if (angleDeviation > tiltThresholdAngle)
                            {
                                // Apply corrective force to upright the player
                                Ogre::Quaternion uprightRotation = Ogre::Vector3::UNIT_Y.getRotationTo(-gravityDir);

                                // Only correct pitch & roll, not yaw
                                Ogre::Vector3 correctionAxes(1.0f, 0.0f, 1.0f);
                                this->agent->applyOmegaForceRotateTo(uprightRotation, correctionAxes, this->getTurnRate());
                            }
                            else
                            {
                                this->agent->applyOmegaForceRotateToDirection(forward, this->getTurnRate());
                            }
                        }
                    }
                    else
                    {
                        this->agent->applyOmegaForceRotateTo(this->agent->getOrientation(), gravityDir);
                    }
                }
            }

            if (this->actualizePathDelay != -1.0f)
            {
                // Update path finding, if enabled
                if (this->timeSinceLastPathActualization > 0.0f)
                {
                    this->timeSinceLastPathActualization -= dt;
                }
                else
                {
                    this->timeSinceLastPathActualization = this->actualizePathDelay;

                    if (this->isSwitchOn(FOLLOW_PATH) && nullptr != this->targetAgent)
                    {
                        if (false == this->oldTargetPosition.positionEquals(this->targetAgent->getPosition(), 0.2f))
                        {
                            this->findPath();
                        }

                        this->oldTargetPosition = this->targetAgent->getPosition();
                    }
                }
            }
            else if (this->isSwitchOn(PATH_FINDING_WANDER) && this->pPath->isFinished())
            {
                this->findRandomPath();
            }

            if (nullptr != this->crowdComponent)
            {
                this->crowdComponent->endUpdateVelocity();
            }
        }

		void MovingBehavior::detectAgentMotionChange(Ogre::Real dt)
		{
			if (0 == this->mask || NONE == this->mask || STOP == this->mask)
			{
				return;
			}

			if (this->stuckCheckTime != 0.0f)
			{
				Ogre::Real distanceSQ = (this->agent->getPosition() - this->oldAgentPositionForStuck).squaredLength();

				if (this->timeSinceLastStuckCheck > 0.0f)
				{
					this->timeSinceLastStuckCheck -= dt;
				}
				else
				{
					this->timeSinceLastStuckCheck = this->stuckCheckTime;
					
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] moved distance: " + Ogre::StringConverter::toString(distanceSQ));
					if (distanceSQ < 0.3f * 0.3f)
					{
						this->stuckCount += 1;
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] Start stuck: " + Ogre::StringConverter::toString(this->stuckCount) + " is stuck: "
						//  + Ogre::StringConverter::toString(this->isStuck));

						if (this->stuckCount >= 3 && false == this->isStuck)
						{
							this->isStuck = true;
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] Stuck!");
							
							this->setBehavior(NONE);
							this->stuckCount = 0;

							if (nullptr != this->agentStuckObserver)
							{
								this->agentStuckObserver->onAgentStuck();
							}
						}
					}
					else
					{
						this->stuckCount = 0;
						this->isStuck = false;
					}
					this->oldAgentPositionForStuck = this->agent->getPosition();
				}
			}
		}

		void MovingBehavior::setAutoOrientation(bool autoOrientation)
		{
			this->autoOrientation = autoOrientation;
		}

		bool MovingBehavior::getIsAutoOrientated(void) const
		{
			return this->autoOrientation;
		}

		void MovingBehavior::setAutoAnimation(bool autoAnimation)
        {
            if (nullptr == this->agent && true == autoAnimation)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Warning: Cannot use auto animation, because the agent is null.");
                return;
            }

            if (true == autoAnimation)
            {
                bool hasAnimation = false;
                IAnimationBlender* animationBlender;
                auto animationCompPtrV2 = NOWA::makeStrongPtr(this->agent->getOwner()->getComponent<AnimationComponentV2>());
                if (nullptr != animationCompPtrV2)
                {
                    animationBlender = animationCompPtrV2->getAnimationBlender();
                    this->autoAnimation = autoAnimation;
                    this->animationBlender = animationBlender;
                    this->oldAnimationSpeed = animationCompPtrV2->getSpeed();
                    hasAnimation = true;

                    // This behavior now drives addTime() itself with movement-derived
                    // speed. Tell AnimationComponentV2 to stop self-driving with its
                    // default constant speed while this behavior is active, and cache
                    // the component so the false-branch below can release the claim.
                    animationCompPtrV2->setExternallyDriven(true);
                    this->animationComponentV2 = animationCompPtrV2.get();
                }

                if (false == hasAnimation)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[MovingBehavior] Warning: Cannot use auto animation, because the agent has no component which includes animation (AnimationComponentV2, PlayerControllerComponent) for game object: " + this->agent->getOwner()->getName());
                }
            }
            else
            {
                if (nullptr != this->animationBlender)
                {
                    this->animationBlender = nullptr;
                }

                if (nullptr != this->animationComponentV2)
                {
                    this->animationComponentV2->setExternallyDriven(false);
                    this->animationComponentV2 = nullptr;
                }
            }
        }

		bool MovingBehavior::getIsAutoAnimated(void) const
		{
			return this->autoAnimation;
		}

		Ogre::Vector3 MovingBehavior::calculate(Ogre::Real dt)
		{
			Ogre::Vector3 resultVelocity = Ogre::Vector3::ZERO;
			// later other strategies to calculate results
			// calculateWeightedSum
			// calculateDithered

			resultVelocity = this->calculatePrioritized(dt);

			return resultVelocity;
		}

		Ogre::Vector3 MovingBehavior::calculatePrioritized(Ogre::Real dt)
		{
			//---------------------- calculatePrioritized ----------------------------
			//
			//  This method calls each active steering behavior in order of priority
			//  and acumulates their forces until the max steering velocity magnitude
			//  is reached, at which time the function returns the steering velocity 
			//  accumulated to that point
			//------------------------------------------------------------------------
			Ogre::Vector3 totalVelocity = Ogre::Vector3::ZERO;
			Ogre::Vector3 velocity = Ogre::Vector3::ZERO;
			this->currentBehavior.clear();

			// Order is important, so that imporatant behaviors have more priority and only if there is enough velocity left, the other behaviors come into account
			if (true == this->isSwitchOn(OBSTACLE_AVOIDANCE))
			{
				this->currentBehavior += "ObstacleAvoidance ";
				velocity = this->obstacleAvoidance(dt) * this->weightObstacleAvoidance;

				if (!this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}

			if (true == this->isSwitchOn(NONE))
			{
				this->currentBehavior += "None ";
				velocity = this->none();
			}
			if (true == this->isSwitchOn(STOP))
			{
				this->currentBehavior += "Stop ";
				velocity = this->none();
			}
			if (true == this->isSwitchOn(MOVE))
			{
				this->currentBehavior += "Move ";
				velocity = this->move();
				if (!this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (true == this->isSwitchOn(MOVE_RANDOMLY))
			{
				this->currentBehavior += "MoveRandomly ";
				velocity = this->moveRandomly(dt) * this->weightWander;
				if (!this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (true == this->isSwitchOn(SEEK))
			{
				this->currentBehavior += "Seek ";
				if (nullptr == this->targetAgent)
					return Ogre::Vector3::ZERO;

				// Attention: seek can stuck
				velocity = this->seek(this->targetAgent->getPosition(), dt) * this->weightSeek;
				if (!this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			else if (true == this->isSwitchOn(SEEK_2D))
			{
				this->currentBehavior += "Seek2D";
				if (nullptr == this->targetAgent)
					return Ogre::Vector3::ZERO;

				// Attention: seek can stuck
				velocity = this->seek2D(this->targetAgent->getPosition(), dt) * this->weightSeek;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (true == this->isSwitchOn(FLEE))
			{
				this->currentBehavior += "Flee ";
				if (nullptr == this->targetAgent)
					return Ogre::Vector3::ZERO;

				velocity = this->flee(this->targetAgent->getPosition(), dt) * this->weightFlee;
				if (!this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			else if (true == this->isSwitchOn(FLEE_2D))
			{
				this->currentBehavior += "Flee2D ";
				if (nullptr == this->targetAgent)
					return Ogre::Vector3::ZERO;

				velocity = this->flee2D(this->targetAgent->getPosition(), dt) * this->weightFlee;
				if (!this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (this->isSwitchOn(ARRIVE))
			{
				if (nullptr == this->targetAgent)
					return Ogre::Vector3::ZERO;

				this->currentBehavior += "Arrive ";
				velocity = this->arrive(this->targetAgent->getPosition(), this->deceleration, dt) * this->weightArrive;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			else if (this->isSwitchOn(ARRIVE_2D))
			{
				if (nullptr == this->targetAgent)
					return Ogre::Vector3::ZERO;

				this->currentBehavior += "Arrive2D ";
				velocity = this->arrive2D(this->targetAgent->getPosition(), this->deceleration, dt) * this->weightArrive;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (this->isSwitchOn(WANDER))
			{
				this->currentBehavior += "Wander ";
				velocity = this->wander(dt) * this->weightWander;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			else if (this->isSwitchOn(WANDER_2D))
			{
				this->currentBehavior += "Wander2D ";
				velocity = this->wander2D(dt) * this->weightWander;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}

			if (this->isSwitchOn(PURSUIT))
			{
				this->currentBehavior += "Pursuit ";
				velocity = this->pursuit(dt) * this->weightPursuit;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (this->isSwitchOn(OFFSET_PURSUIT))
			{
				this->currentBehavior += "OffsetPursuit ";
				velocity = this->offsetPursuit(dt) * this->weightPursuit;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (this->isSwitchOn(EVADE))
			{
				this->currentBehavior += "Evade ";
				velocity = this->evade(dt) * this->weightEvade;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (this->isSwitchOn(HIDE))
			{
				this->currentBehavior += "Hide ";
				velocity = this->hide(dt) * this->weightHide;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			// Either follow path or wander with follow path from recast
			if (this->isSwitchOn(FOLLOW_PATH))
			{
				this->currentBehavior += "FollowPath ";
				velocity = this->followPath(dt) * this->weightFollowPath;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			if (this->isSwitchOn(INTERPOSE))
			{
				this->currentBehavior += "Interpose ";
				velocity = this->interpose(dt) * this->weightInterpose;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			else if (this->isSwitchOn(FOLLOW_PATH_2D))
			{
				this->currentBehavior += "FollowPath2D ";
				velocity = this->followPath2D(dt) * this->weightFollowPath;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			else if (this->isSwitchOn(PURSUIT_2D))
			{
				this->currentBehavior += "Pursuit2D ";
				velocity = this->pursuit2D(dt) * this->weightPursuit;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}
			else if (this->isSwitchOn(PATH_FINDING_WANDER))
			{
				this->currentBehavior += "PathFindingWander ";
				velocity = this->followPath(dt) * this->weightFollowPath;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
			}

			if (this->isSwitchOn(FLOCKING))
			{
				this->currentBehavior += "Flocking ";
				velocity = this->flocking(dt);
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return totalVelocity;
				}
				else
				{
					return this->limitFlockingVelocity(totalVelocity);
				}
			}
#if 0
			if (this->isSwitchOn(FLOCKING_SEPARATION))
			{
				this->currentBehavior += "FlockingSeparation ";
				velocity = this->flockingRuleSeparation().second * this->weightSeparation;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return this->limitFlockingVelocity(totalVelocity);
				}
			}
			if (this->isSwitchOn(FLOCKING_SPREAD))
			{
				this->currentBehavior += "FlockingSpread ";
				velocity = this->flockingRuleSpread().second * this->weightSeparation;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return this->limitFlockingVelocity(totalVelocity);
				}
			}
			if (this->isSwitchOn(FLOCKING_FORMATION_V_SHAPE))
			{
				this->currentBehavior += "flockingFormationVShape ";
				velocity = this->flockingFormationVShape().second * this->weightSeparation;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return this->limitFlockingVelocity(totalVelocity);
				}
			}
			if (this->isSwitchOn(FLOCKING_ALIGNMENT))
			{
				this->currentBehavior += "FlockingAlignment ";
				velocity = this->flockingRuleAlignment().second * this->weightAlignment;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return this->limitFlockingVelocity(totalVelocity);
				}
			}
			if (this->isSwitchOn(FLOCKING_COHESION))
			{
				this->currentBehavior += "FlockingAlignment ";
				velocity = this->flockingRuleCohesion().second * this->weightCohesion;
				if (false == this->accumulateVelocity(totalVelocity, velocity))
				{
					return this->limitFlockingVelocity(totalVelocity);
				}
			}
#endif
			if (0 != this->mask)
			{
				return this->limitVelocity(totalVelocity);
			}
			else
			{
				return Ogre::Vector3::ZERO;
			}
		}

		bool MovingBehavior::accumulateVelocity(Ogre::Vector3& runningTotal, Ogre::Vector3 velocityToAdd)
        {
            if (nullptr == this->agent)
            {
                return false;
            }

            // Attention: The former hardcoded budget of '2 * 20' was completely decoupled from the agent.
            // With a max speed of 5 no behavior could ever saturate it, so the priority truncation never
            // triggered and calculatePrioritized silently degraded to a plain sum of all behaviors.
            const Ogre::Real magnitudeBudget = this->agent->getMaxSpeed() * this->velocityAccumulationTweaker;

            const Ogre::Real magnitudeSoFar = runningTotal.length();
            const Ogre::Real magnitudeRemaining = magnitudeBudget - magnitudeSoFar;

            if (magnitudeRemaining <= 0.0f)
            {
                return false;
            }

            const Ogre::Real magnitudeToAdd = velocityToAdd.length();

            if (magnitudeToAdd < magnitudeRemaining)
            {
                runningTotal += velocityToAdd;
            }
            else
            {
                runningTotal += (velocityToAdd.normalisedCopy() * magnitudeRemaining);
            }

            return true;
        }

		Ogre::Vector3 MovingBehavior::limitVelocity(const Ogre::Vector3& totalVelocity)
        {
            // https://physics.stackexchange.com/questions/17049/how-does-force-relate-to-velocity
            if (nullptr == this->agent)
            {
                return Ogre::Vector3::ZERO;
            }

            Ogre::Vector3 velocity = totalVelocity;

            // In walk mode the vertical part is handled by gravity resp. by the player controller,
            // hence it must not be part of the steering velocity at all.
            if (false == this->flyMode)
            {
                velocity.y = 0.0f;
            }

            const Ogre::Real maxSpeed = this->agent->getMaxSpeed();
            const Ogre::Real minSpeed = this->agent->getMinSpeed();
            const Ogre::Real squaredLength = velocity.squaredLength();

            if (squaredLength < 0.0001f)
            {
                // No steering at all: Do not push the agent to min speed in an undefined direction
                return Ogre::Vector3::ZERO;
            }

            if (squaredLength > maxSpeed * maxSpeed)
            {
                velocity = velocity.normalisedCopy() * maxSpeed;
            }
            else if (squaredLength < minSpeed * minSpeed)
            {
                velocity = velocity.normalisedCopy() * minSpeed;
            }

            // Attention: The y component must be clamped together with x and z, else in fly mode the agent
            // would accelerate vertically up to the accumulation budget while x/z stay limited to max speed.
            return velocity;
        }

				Ogre::Vector3 MovingBehavior::limitFlockingVelocity(const Ogre::Vector3& totalVelocity)
        {
            if (nullptr == this->agent)
            {
                return Ogre::Vector3::ZERO;
            }

            Ogre::Vector3 velocity = totalVelocity;

            if (false == this->flyMode)
            {
                velocity.y = 0.0f;
            }

            const Ogre::Real squaredLength = velocity.squaredLength();

            // Attention: Never force min speed onto a zero vector. normalisedCopy of a zero length vector
            // stays zero in Ogre, but the intent is wrong anyway: A resting agent would be pushed into an
            // arbitrary direction as soon as numerical noise appears.
            if (squaredLength < 0.0001f)
            {
                return Ogre::Vector3::ZERO;
            }

            const Ogre::Real maxSpeed = this->agent->getMaxSpeed();
            const Ogre::Real minSpeed = this->agent->getMinSpeed();

            if (squaredLength > maxSpeed * maxSpeed)
            {
                velocity = velocity.normalisedCopy() * maxSpeed;
            }
            else if (squaredLength < minSpeed * minSpeed)
            {
                velocity = velocity.normalisedCopy() * minSpeed;
            }

            return velocity;
        }

		Ogre::String MovingBehavior::getCurrentBehavior(void) const
		{
			return this->currentBehavior;
		}

		void MovingBehavior::reset(void)
		{
			this->setBehavior(NONE);
			this->flockingAgents.clear();
			if (nullptr != this->agent)
			{
				this->agent->resetForce();
				this->oldAgentPositionForStuck = this->agent->getPosition();
			}
			if (nullptr != this->pPath)
			{
				this->pPath->clear();
				this->pPath->setRepeat(false);
				this->pPath->setInvertDirection(false);
				this->pPath->setDirectionChange(false);
			}
			if (nullptr != AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
			{
				AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->CreateRecastPathLine(0, false);
			}
			this->timeSinceLastStuckCheck = 0.0f;
			this->stuckCount = 0;
			this->obstaclesHide.clear();
			this->obstacleHideRangeRadius = 10.0f;
			this->obstacleAvoidanceRangeRadius = 10.0f;
			this->obstaclesAvoidanceCategoryIds = 0;
			this->motionDistanceChange = 0.0f;
			this->animationBlender = nullptr;
			this->oldAnimationSpeed = 1.0f;
			// this->setAgentId(0);
		}

		unsigned long MovingBehavior::getAgentId(void) const
		{
			return this->agentId;
		}

		void MovingBehavior::setWeightSeparation(Ogre::Real weightSeparation)
		{
			this->weightSeparation = weightSeparation;
		}

		Ogre::Real MovingBehavior::getWeightSeparation(void) const
		{
			return this->weightSeparation;
		}

		void MovingBehavior::setWeightCohesion(Ogre::Real weightCohesion)
		{
			this->weightCohesion = weightCohesion;
		}

		Ogre::Real MovingBehavior::getWeightCohesion(void) const
		{
			return this->weightCohesion;
		}

		void MovingBehavior::setWeightAlignment(Ogre::Real weightAlignment)
		{
			this->weightAlignment = weightAlignment;
		}

		Ogre::Real MovingBehavior::getWeightAlignment(void) const
		{
			return this->weightAlignment;
		}

		void MovingBehavior::setWeightWander(Ogre::Real weightWander)
		{
			this->weightWander = weightWander;
		}

		Ogre::Real MovingBehavior::getWeightWander(void) const
		{
			return this->weightWander;
		}

		void MovingBehavior::setWeightObstacleAvoidance(Ogre::Real weightObstacleAvoidance)
		{
			this->weightObstacleAvoidance = weightObstacleAvoidance;
		}

		Ogre::Real MovingBehavior::getWeightObstacleAvoidance(void) const
		{
			return this->weightObstacleAvoidance;
		}

		void MovingBehavior::setWeightSeek(Ogre::Real weightSeek)
		{
			this->weightSeek = weightSeek;
		}

		Ogre::Real MovingBehavior::getWeightSeek(void) const
		{
			return this->weightSeek;
		}

		void MovingBehavior::setWeightFlee(Ogre::Real weightFlee)
		{
			this->weightFlee = weightFlee;
		}

		Ogre::Real MovingBehavior::getWeightFlee(void) const
		{
			return this->weightFlee;
		}

		void MovingBehavior::setWeightArrive(Ogre::Real weightArrive)
		{
			this->weightArrive = weightArrive;
		}

		Ogre::Real MovingBehavior::getWeightArrive(void) const
		{
			return this->weightArrive;
		}

		void MovingBehavior::setWeightPursuit(Ogre::Real weightPursuit)
		{
			this->weightPursuit = weightPursuit;
		}

		Ogre::Real MovingBehavior::getWeightPursuit(void) const
		{
			return this->weightPursuit;
		}

		void MovingBehavior::setWeightOffsetPursuit(Ogre::Real weightOffsetPursuit)
		{
			this->weightOffsetPursuit = weightOffsetPursuit;
		}

		Ogre::Real MovingBehavior::getWeightOffsetPursuit(void) const
		{
			return this->weightOffsetPursuit;
		}

		void MovingBehavior::setWeightHide(Ogre::Real weightHide)
		{
			this->weightHide = weightHide;
		}

		Ogre::Real MovingBehavior::getWeightHide(void) const
		{
			return this->weightHide;
		}

		void MovingBehavior::setWeightEvade(Ogre::Real weightEvade)
		{
			this->weightEvade = weightEvade;
		}

		Ogre::Real MovingBehavior::getWeightEvade(void) const
		{
			return this->weightEvade;
		}

		void MovingBehavior::setWeightFollowPath(Ogre::Real weightFollowPath)
		{
			this->weightFollowPath = weightFollowPath;
		}

		Ogre::Real MovingBehavior::getWeightFollowPath(void) const
		{
			return this->weightFollowPath;
		}

		void MovingBehavior::setWeightInterpose(Ogre::Real weightInterpose)
		{
			this->weightInterpose = weightInterpose;
		}

		Ogre::Real MovingBehavior::getWeightInterpose(void) const
		{
			return this->weightInterpose;
		}

		bool MovingBehavior::getIsStuck(void) const
		{
			return this->isStuck;
		}

	}; //end namespace KI

}; //end namespace NOWA