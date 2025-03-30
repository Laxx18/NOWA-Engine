#include "NOWAPrecompiled.h"
#include "FormationBehavior.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/PhysicsPlayerControllerComponent.h"
#include "gameobject/PhysicsActiveKinematicComponent.h"
#include "gameobject/AnimationComponent.h"
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
        FormationBehavior::FormationBehavior()
            : leader(nullptr),
            currentFormationType(NONE),
            spacing(3.0f),
            autoOrientation(true),
            flyMode(false)
        {
        }

        FormationBehavior::~FormationBehavior()
        {
            this->formationAgents.clear();
        }

        void FormationBehavior::setLeader(PhysicsActiveComponent* leader)
        {
            this->leader = leader;
        }

        PhysicsActiveComponent* FormationBehavior::getLeader() const
        {
            return this->leader;
        }

        void FormationBehavior::setFormationAgents(const std::vector<unsigned long>& formationAgentIds)
        {
            this->formationAgents.clear();
            for (size_t i = 0; i < formationAgentIds.size(); i++)
            {
                GameObjectPtr formationAgentGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(formationAgentIds[i]);
                if (nullptr != formationAgentGameObjectPtr)
                {
                    auto physicsActiveComponent = NOWA::makeStrongPtr(formationAgentGameObjectPtr->getComponent<PhysicsActiveComponent>());
                    if (nullptr != physicsActiveComponent)
                    {
                        this->formationAgents.emplace_back(physicsActiveComponent.get());
                    }
                }
            }
        }

        void FormationBehavior::addFormationAgent(unsigned long agentId)
        {
            GameObjectPtr formationAgentGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(agentId);
            if (nullptr != formationAgentGameObjectPtr)
            {
                auto physicsActiveComponent = NOWA::makeStrongPtr(formationAgentGameObjectPtr->getComponent<PhysicsActiveComponent>());
                if (nullptr != physicsActiveComponent)
                {
                    if (std::find(this->formationAgents.begin(), this->formationAgents.end(), physicsActiveComponent.get()) == this->formationAgents.end())
                    {
                        this->formationAgents.push_back(physicsActiveComponent.get());
                    }
                }
            }
        }

        void FormationBehavior::removeFormationAgent(unsigned long agentId)
        {
            GameObjectPtr formationAgentGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(agentId);
            if (nullptr != formationAgentGameObjectPtr)
            {
                auto physicsActiveComponent = NOWA::makeStrongPtr(formationAgentGameObjectPtr->getComponent<PhysicsActiveComponent>());
                if (nullptr != physicsActiveComponent)
                {
                    auto it = std::find(this->formationAgents.begin(), this->formationAgents.end(), physicsActiveComponent.get());
                    if (it != this->formationAgents.end())
                    {
                        this->formationAgents.erase(it);
                    }
                }
            }
        }

        void FormationBehavior::setFormationType(FormationBehavior::FormationType formationType)
        {
            this->currentFormationType = formationType;
        }

        void FormationBehavior::setFormationType(const Ogre::String& formationTypeStr)
        {
            FormationType type = mapFormationType(formationTypeStr);
            setFormationType(type);
        }

        FormationBehavior::FormationType FormationBehavior::getFormationType() const
        {
            return this->currentFormationType;
        }

        void FormationBehavior::setSpacing(Ogre::Real spacing)
        {
            this->spacing = spacing;
        }

        Ogre::Real FormationBehavior::getSpacing() const
        {
            return this->spacing;
        }

        void FormationBehavior::setAutoOrientation(bool autoOrient)
        {
            this->autoOrientation = autoOrient;
        }

        bool FormationBehavior::getAutoOrientation() const
        {
            return this->autoOrientation;
        }

        void FormationBehavior::setFlyMode(bool flyMode)
        {
            this->flyMode = flyMode;
        }

        bool FormationBehavior::getFlyMode() const
        {
            return this->flyMode;
        }

        FormationBehavior::FormationType FormationBehavior::mapFormationType(const Ogre::String& formationTypeStr)
        {
            static std::map<Ogre::String, FormationType> formationMap = {
                {"NONE", NONE},
                {"LINE", LINE},
                {"WEDGE", WEDGE},
                {"CIRCLE", CIRCLE},
                {"SQUARE", SQUARE},
                {"COLUMN", COLUMN},
                {"ECHELON_LEFT", ECHELON_LEFT},
                {"ECHELON_RIGHT", ECHELON_RIGHT}
            };

            auto it = formationMap.find(formationTypeStr);
            if (it != formationMap.end())
            {
                return it->second;
            }
            return NONE;
        }

        Ogre::String FormationBehavior::formationTypeToString(FormationBehavior::FormationType formationType)
        {
            switch (formationType)
            {
            case NONE: return "NONE";
            case LINE: return "LINE";
            case WEDGE: return "WEDGE";
            case CIRCLE: return "CIRCLE";
            case SQUARE: return "SQUARE";
            case COLUMN: return "COLUMN";
            case ECHELON_LEFT: return "ECHELON_LEFT";
            case ECHELON_RIGHT: return "ECHELON_RIGHT";
            default: return "UNKNOWN";
            }
        }

        void FormationBehavior::update(Ogre::Real dt)
        {
            if (!this->leader || this->formationAgents.empty() || this->currentFormationType == NONE)
            {
                return;
            }

            Ogre::Vector3 leaderPosition = this->leader->getPosition();
            Ogre::Quaternion leaderOrientation = this->leader->getOrientation();
            Ogre::Vector3 leaderForward = leaderOrientation * this->leader->getOwner()->getDefaultDirection();
            Ogre::Vector3 leaderVelocity = this->leader->getVelocity();

            // Calculate right and up vectors based on leader orientation
            Ogre::Vector3 leaderRight = leaderForward.crossProduct(Ogre::Vector3::UNIT_Y);
            leaderRight.normalise();
            Ogre::Vector3 leaderUp = leaderRight.crossProduct(leaderForward);
            leaderUp.normalise();

            // Process each agent in the formation
            for (size_t i = 0; i < this->formationAgents.size(); ++i)
            {
                PhysicsActiveComponent* agent = this->formationAgents[i];
                if (!agent) continue;

                // Calculate formation offset based on formation type and agent index
                Ogre::Vector3 formationOffset = this->calculateFormationOffset(i, leaderForward, leaderRight, leaderUp);

                // Calculate target position
                Ogre::Vector3 targetPosition = leaderPosition + formationOffset;

                // Calculate direction to target
                Ogre::Vector3 currentPosition = agent->getPosition();
                Ogre::Vector3 directionToTarget = targetPosition - currentPosition;
                Ogre::Real distanceToTarget = directionToTarget.length();

                // Only move if we're not already at the target
                if (distanceToTarget > 0.1f)
                {
                    directionToTarget.normalise();

                    // Scale by speed
                    Ogre::Real speed = agent->getSpeed();
                    // Slow down as we approach the target
                    if (distanceToTarget < speed * dt * 5.0f)
                    {
                        speed *= (distanceToTarget / (speed * dt * 5.0f));
                    }

                    Ogre::Vector3 resultVelocity = directionToTarget * speed;

                    // Apply the velocity/force to the physics component
                    this->applyMovement(agent, resultVelocity, dt);
                }
                else
                {
                    // At target position, match leader's orientation
                    Ogre::Quaternion targetOrientation = leaderOrientation;
                    agent->setOrientation(targetOrientation);

                    // Stop movement if we're at the target
                    this->applyMovement(agent, Ogre::Vector3::ZERO, dt);
                }
            }
        }

        Ogre::Vector3 FormationBehavior::calculateFormationOffset(size_t agentIndex, const Ogre::Vector3& forward,
            const Ogre::Vector3& right, const Ogre::Vector3& up)
        {
            Ogre::Vector3 offset = Ogre::Vector3::ZERO;
            const Ogre::Real spacing = this->spacing;

            int row, col;
            Ogre::Real angle;
            int totalAgents = this->formationAgents.size();

            switch (this->currentFormationType)
            {
                case LINE:
                {
                    // Line formation perpendicular to movement direction
                    offset = right * ((agentIndex - (totalAgents - 1) / 2.0f) * spacing);
                    break;
                }
                case WEDGE:
                {
                    // V-shaped formation, leader at the front
                    row = agentIndex / 2;
                    col = agentIndex % 2;
                    if (col == 0)
                    {
                        // Right side of wedge
                        offset = -forward * (row * spacing) + right * (row * spacing);
                    }
                    else
                    {
                        // Left side of wedge
                        offset = -forward * (row * spacing) - right * (row * spacing);
                    }
                    break;
                }
                case CIRCLE:
                {
                    // Circular formation around leader
                    angle = 2.0f * Ogre::Math::PI * agentIndex / totalAgents;
                    offset = right * (cos(angle) * spacing) +
                        -forward * (sin(angle) * spacing);
                    break;
                }
                case SQUARE:
                {
                    // Square/rectangular formation
                    int sideLength = ceil(sqrt(totalAgents));
                    row = agentIndex / sideLength;
                    col = agentIndex % sideLength;
                    offset = right * ((col - (sideLength - 1) / 2.0f) * spacing) +
                        -forward * ((row - (sideLength - 1) / 2.0f) * spacing);
                    break;
                }
                case COLUMN:
                {
                    // Column formation - agents follow in a line
                    offset = -forward * (agentIndex * spacing);
                    break;
                }
                case ECHELON_LEFT:
                {
                    // Echelon left formation
                    offset = -forward * (agentIndex * spacing * 0.5f) - right * (agentIndex * spacing);
                    break;
                }
                case ECHELON_RIGHT:
                {
                    // Echelon right formation
                    offset = -forward * (agentIndex * spacing * 0.5f) + right * (agentIndex * spacing);
                    break;
                }
                case NONE:
                default:
                {
                    // No formation, no offset
                    offset = Ogre::Vector3::ZERO;
                    break;
                }
            }

            return offset;
        }

        void FormationBehavior::applyMovement(PhysicsActiveComponent* agent, const Ogre::Vector3& resultVelocity, Ogre::Real dt)
        {
            if (nullptr == agent)
            {
                return;
            }

            Ogre::Vector3 targetVelocity = resultVelocity;

            // Handle Y-axis constraints if not in fly mode
            if (false == this->flyMode)
            {
                targetVelocity.y = 0.0f;
            }

            // Check if this is a player controller component
            PhysicsPlayerControllerComponent* physicsPlayerControllerComponent = dynamic_cast<PhysicsPlayerControllerComponent*>(agent);

            if (physicsPlayerControllerComponent)
            {
                Ogre::Radian heading = agent->getOrientation().getYaw();

                // Set orientation if moving
                if (targetVelocity != Ogre::Vector3::ZERO && this->autoOrientation)
                {
                    Ogre::Quaternion resultOrientation =
                        (agent->getOrientation() * agent->getOwner()->getDefaultDirection()).getRotationTo(targetVelocity);
                    heading = resultOrientation.getYaw();
                }

                // Apply movement to player controller
                physicsPlayerControllerComponent->move(agent->getSpeed() * targetVelocity.length(), 0.0f, heading);
            }
            // Check if this is a kinematic component
            else
            {
                PhysicsActiveKinematicComponent* physicsActiveKinematicComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(agent);

                if (nullptr != physicsActiveKinematicComponent)
                {
                    // For kinematic components
                    if (this->flyMode)
                    {
                        Ogre::Real maxYVelocity = agent->getSpeed();
                        targetVelocity.y = Ogre::Math::Clamp(targetVelocity.y, -1.0f, maxYVelocity);
                    }

                    physicsActiveKinematicComponent->setVelocity(targetVelocity);

                    // Handle orientation
                    if (targetVelocity != Ogre::Vector3::ZERO)
                    {
                        Ogre::Quaternion newOrientation = agent->getOrientation();
                        if (this->autoOrientation)
                        {
                            newOrientation = (agent->getOrientation() * agent->getOwner()->getDefaultDirection()).getRotationTo(targetVelocity);
                            agent->setOmegaVelocity(Ogre::Vector3(0.0f, newOrientation.getYaw().valueDegrees() * 0.1f, 0.0f));
                        }
                        else
                        {
                            agent->setOmegaVelocityRotateTo(newOrientation, Ogre::Vector3(0.0f, 1.0f, 0.0f));
                        }
                    }
                }
                // For regular physics components
                else
                {
                    Ogre::Vector3 forceForVelocity = Ogre::Vector3::ZERO;
                    if (false == this->flyMode)
                    {
                        targetVelocity.y = 0.0f;
                        forceForVelocity = agent->getVelocity() * Ogre::Vector3(0.0f, 1.0f, 0.0f) + targetVelocity;
                    }
                    else
                    {
                        forceForVelocity = targetVelocity;
                    }

                    agent->applyRequiredForceForVelocity(forceForVelocity);

                    // Handle orientation
                    if (targetVelocity != Ogre::Vector3::ZERO)
                    {
                        Ogre::Quaternion newOrientation = agent->getOrientation();
                        if (this->autoOrientation)
                        {
                            newOrientation = (agent->getOrientation() * agent->getOwner()->getDefaultDirection()).getRotationTo(targetVelocity);
                            agent->applyOmegaForce(Ogre::Vector3(0.0f, newOrientation.getYaw().valueDegrees() * 0.1f, 0.0f));
                        }
                        else
                        {
                            agent->applyOmegaForceRotateTo(newOrientation, Ogre::Vector3(0.0f, 1.0f, 0.0f));
                        }
                    }
                }
            }
        }

	}; //end namespace KI

}; //end namespace NOWA