#include "NOWAPrecompiled.h"
#include "PhysicsActiveKinematicComponent.h"
#include "PhysicsComponent.h"
#include "main/AppStateManager.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    PhysicsActiveKinematicComponent::PhysicsActiveKinematicComponent() : PhysicsActiveComponent(), previousPosition(Ogre::Vector3::ZERO), previousOrientation(Ogre::Quaternion::IDENTITY), hasPreviousTransform(false)
    {
        this->asSoftBody->setVisible(false);
        this->gyroscopicTorque->setValue(false);
        this->gyroscopicTorque->setVisible(false);
        this->gravity->setValue(Ogre::Vector3::ZERO);
        this->gravity->setVisible(false);
        this->gravitySourceCategory->setVisible(false);
        this->constraintDirection->setValue(Ogre::Vector3::ZERO);
        this->constraintDirection->setVisible(false);

        this->onKinematicContactFunctionName = new Variant(PhysicsActiveKinematicComponent::AttrOnKinematicContactFunctionName(), Ogre::String(""), this->attributes);

        this->onKinematicContactFunctionName->setDescription("Sets the function name to react in lua script at the moment when another game object collided with this kinematic game object. "
                                                             "It can also be used with (setCollidable(false)), so that ghost collision can be detected. E.g. onKinematicContact(otherGameObject).");
        this->onKinematicContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onKinematicContactFunctionName->getString() + "(otherGameObject)");
    }

    PhysicsActiveKinematicComponent::~PhysicsActiveKinematicComponent()
    {
    }

    bool PhysicsActiveKinematicComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        PhysicsActiveComponent::parseCommonProperties(propertyElement);

        this->constraintDirection->setValue(Ogre::Vector3::ZERO);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnKinematicContactFunctionName")
        {
            this->onKinematicContactFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr PhysicsActiveKinematicComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PhysicsActiveKinematicCompPtr clonedCompPtr(boost::make_shared<PhysicsActiveKinematicComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setMass(this->mass->getReal());
        clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
        clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
        clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
        clonedCompPtr->setGravity(this->gravity->getVector3());
        clonedCompPtr->setGravitySourceCategory(this->gravitySourceCategory->getString());
        clonedCompPtr->setConstraintDirection(this->constraintDirection->getVector3());
        clonedCompPtr->setSpeed(this->speed->getReal());
        clonedCompPtr->setMaxSpeed(this->maxSpeed->getReal());
        // clonedCompPtr->setDefaultPoseName(this->defaultPoseName);
        clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
        // do not use constraintAxis variable, because its being manipulated during physics body creation
        // clonedCompPtr->setConstraintAxis(this->initConstraintAxis);

        clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());
        // Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property
        clonedCompPtr->setCollidable(this->collidable->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        clonedCompPtr->setOnKinematicContactFunctionName(this->onKinematicContactFunctionName->getString());

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool PhysicsActiveKinematicComponent::postInit(void)
    {
        bool success = PhysicsComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveKinematicComponent] Init physics active kinematic component for game object: " + this->gameObjectPtr->getName());

        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        // Physics active component must be dynamic, else a mess occurs
        this->gameObjectPtr->setDynamic(true);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

        // this->gameObjectPtr->getAttribute(PhysicsActiveComponent::AttrMass())->setVisible(false);

        if (false == this->createDynamicBody())
        {
            return false;
        }

        return success;
    }

    void PhysicsActiveKinematicComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveKinematicComponent] RemoveComponent physics active kinematic component for game object: " + this->gameObjectPtr->getName());

        this->releaseConstraintAxis();
    }

    bool PhysicsActiveKinematicComponent::connect(void)
    {
        bool success = PhysicsActiveComponent::connect();

        this->setOnKinematicContactFunctionName(this->onKinematicContactFunctionName->getString());

        return success;
    }

    bool PhysicsActiveKinematicComponent::disconnect(void)
    {
        bool success = PhysicsActiveComponent::disconnect();

        // Dropped, so the first frame after reconnecting does not derive a velocity from a
        // transform that belongs to the previous simulation run.
        this->hasPreviousTransform = false;
        this->previousPosition = Ogre::Vector3::ZERO;
        this->previousOrientation = Ogre::Quaternion::IDENTITY;

        return success;
    }

    void PhysicsActiveKinematicComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating)
        {
            this->gravityDirection = Ogre::Vector3::NEGATIVE_UNIT_Y;

            // Calculates orientation vectors relative to planet surface
            this->up = -this->gravityDirection;
            // Gets the mesh's default direction
            Ogre::Vector3 defaultDirection = this->gameObjectPtr->getDefaultDirection();

            // Stores current entity rotation as quaternion
            Ogre::Quaternion currentRotation = this->getOrientation();

            // Calculates forward vector based on the current rotation and the default direction
            // First, gets the forward direction in the character's local space
            this->forward = currentRotation * defaultDirection;
            // Projects it onto the plane perpendicular to up vector
            this->forward = forward - up * forward.dotProduct(up);

            if (this->forward.squaredLength() < 0.001f * 0.001f) // = 0.000001f
            {
                // Fallback if forward is too small
                // Use a vector perpendicular to up that's close to our preferred direction
                Ogre::Vector3 worldForward;
                if (defaultDirection.dotProduct(Ogre::Vector3::UNIT_Z) > 0.7f)
                {
                    worldForward = Ogre::Vector3::UNIT_Z;
                }
                else if (defaultDirection.dotProduct(Ogre::Vector3::UNIT_X) > 0.7f)
                {
                    worldForward = Ogre::Vector3::UNIT_X;
                }
                else
                {
                    worldForward = Ogre::Vector3::UNIT_Y;
                }

                // Find a suitable forward vector perpendicular to up
                this->forward = worldForward - up * worldForward.dotProduct(up);
                if (this->forward.squaredLength() < 0.001f * 0.001f)
                {
                    this->forward = up.crossProduct(Ogre::Vector3(1.0f, 0.0f, 0.0f));
                    if (this->forward.squaredLength() < 0.001f * 0.001f)
                    {
                        this->forward = up.crossProduct(Ogre::Vector3(0.0f, 0.0f, 1.0f));
                    }
                }
            }
            this->forward.normalise();

            // Calculate right from forward and up (ensures orthogonality)
            this->right = this->forward.crossProduct(up);
            this->right.normalise();

            // --------------------------------------------------------------------------
            // Velocity drive
            //
            // The transform of a kinematic body is written with SetMatrix - by the tag point,
            // by a joint, by a script. For the solver that is a TELEPORT: the body is simply
            // somewhere else in the next substep and reports a velocity of zero. Contact points
            // are then only found when the hulls happen to overlap exactly at a substep, which
            // for anything fast - a weapon swing - almost never happens. That is what a log line
            // saying "contacts: 1 active: 0" means: the AABBs touch, but no contact point was
            // ever generated, so KinematicBody::integrateVelocity() never dispatches.
            //
            // So the velocity that the last transform change IMPLIES is handed to newton here.
            // The body then reads as swept rather than teleported: the contact becomes active,
            // and the body it hits receives a real impulse instead of only a notification.
            //
            // The position itself stays authoritative from whoever wrote the transform, which
            // is why it is captured before integrating and restored afterwards -
            // IntegrateVelocity() would otherwise advance it by another full delta and the body
            // would run ahead of the hand it is attached to.
            // --------------------------------------------------------------------------
            const Ogre::Vector3 authoritativePosition = this->physicsBody->getPosition();
            const Ogre::Quaternion authoritativeOrientation = this->physicsBody->getOrientation();

            if (true == this->hasPreviousTransform && dt > 0.0f)
            {
                Ogre::Vector3 linearVelocity = (authoritativePosition - this->previousPosition) / dt;

                // A teleport across the level - a scene load, a respawn, a weapon changing
                // hands - would otherwise produce an absurd velocity and fire the body through
                // half the world in the solver's eyes.
                const Ogre::Real maximumSpeed = 100.0f;
                if (linearVelocity.squaredLength() > maximumSpeed * maximumSpeed)
                {
                    linearVelocity = Ogre::Vector3::ZERO;
                }

                this->physicsBody->setVelocity(linearVelocity);

                Ogre::Quaternion deltaOrientation = authoritativeOrientation * this->previousOrientation.Inverse();
                deltaOrientation.normalise();

                Ogre::Radian angle;
                Ogre::Vector3 axis;
                deltaOrientation.ToAngleAxis(angle, axis);

                // Shortest way round. Without this a small turn one way is reported as a nearly
                // full turn the other way, and the omega explodes.
                Ogre::Real angleRadians = angle.valueRadians();
                if (angleRadians > Ogre::Math::PI)
                {
                    angleRadians -= Ogre::Math::TWO_PI;
                }

                if (axis.squaredLength() > 0.0001f)
                {
                    this->physicsBody->setOmega(axis.normalisedCopy() * (angleRadians / dt));
                }
            }

            this->previousPosition = authoritativePosition;
            this->previousOrientation = authoritativeOrientation;
            this->hasPreviousTransform = true;

            static_cast<OgreNewt::KinematicBody*>(this->physicsBody)->integrateVelocity(dt);

            // Restore what the transform owner wrote, see the comment above.
            this->physicsBody->setKinematicPositionOrientation(authoritativePosition, authoritativeOrientation);
        }
    }

    void PhysicsActiveKinematicComponent::actualizeValue(Variant* attribute)
    {
        PhysicsActiveComponent::actualizeCommonValue(attribute);

        if (PhysicsActiveKinematicComponent::AttrOnKinematicContactFunctionName() == attribute->getName())
        {
            this->setOnKinematicContactFunctionName(attribute->getString());
        }
    }

    void PhysicsActiveKinematicComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);

        // 2 = int
        // 6 = real
        // 7 = string
        // 8 = vector2
        // 9 = vector3
        // 10 = vector4 -> also quaternion
        // 12 = bool

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OnKinematicContactFunctionName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onKinematicContactFunctionName->getString())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String PhysicsActiveKinematicComponent::getClassName(void) const
    {
        return "PhysicsActiveKinematicComponent";
    }

    Ogre::String PhysicsActiveKinematicComponent::getParentClassName(void) const
    {
        return "PhysicsActiveComponent";
    }

    Ogre::String PhysicsActiveKinematicComponent::getParentParentClassName(void) const
    {
        return "PhysicsComponent";
    }

    // --------------------------------------------------------------------------------------
    // Transform setters
    //
    // All three route to setKinematicPositionOrientation(), so it no longer matters who does
    // the call - engine code, a joint, or a lua script via getPhysicsActiveKinematicComponent().
    //
    // The inherited versions call OgreNewt::Body::setPositionOrientation(), which writes
    // m_prevPosit = m_curPosit = pos. That collapses the interpolation pair, so the render
    // thread has nothing left to interpolate between and every move lands as a hard teleport -
    // visible as a stuttering weapon on anything that is repositioned once per frame, which is
    // exactly what a kinematic body is for. The kinematic variant keeps the previous transform
    // and lets updateNode(interp) do its work.
    //
    // setPosition() and setOrientation() additionally read the OTHER value straight back out of
    // the body. That is correct here - the body is the single source of truth for a kinematic
    // body - but it is also why the two must never be called one after the other to set a full
    // transform: the second call re-reads what the first one wrote. Use
    // setPositionOrientation() for that, which is now the kinematic one too.
    // --------------------------------------------------------------------------------------

    void PhysicsActiveKinematicComponent::setPosition(const Ogre::Vector3& position)
    {
        this->setKinematicPositionOrientation(position, this->getOrientation());
    }

    void PhysicsActiveKinematicComponent::setOrientation(const Ogre::Quaternion& orientation)
    {
        this->setKinematicPositionOrientation(this->getPosition(), orientation);
    }

    void PhysicsActiveKinematicComponent::setPositionOrientation(const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        this->setKinematicPositionOrientation(position, orientation);
    }

    void PhysicsActiveKinematicComponent::setOnKinematicContactFunctionName(const Ogre::String& onKinematicContactFunctionName)
    {
        this->onKinematicContactFunctionName->setValue(onKinematicContactFunctionName);
        if (false == onKinematicContactFunctionName.empty())
        {
            this->onKinematicContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onKinematicContactFunctionName + "(otherGameObject)");
            this->setKinematicContactSolvingEnabled(true);
        }
        else
        {
            this->setKinematicContactSolvingEnabled(false);
        }
    }

    bool PhysicsActiveKinematicComponent::createDynamicBody(void)
    {
        this->destroyCollision();
        this->destroyBody();

        Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

        Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY; // this->gameObjectPtr->getSceneNode()->getOrientation();
        if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
        {
            collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
        }

        Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

        OgreNewt::CollisionPtr collisionPtr;

        NOWA::GraphicsModule::RenderCommand command = [this, &inertia, &collisionPtr, collisionOrientation, &calculatedMassOrigin]()
        {
            collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId());

            if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
            {
                calculatedMassOrigin = this->massOrigin->getVector3();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(command), "PhysicsComponent::createDynamicCollision");

        this->physicsBody = new OgreNewt::KinematicBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collisionPtr);

        this->physicsBody->setGravity(Ogre::Vector3::ZERO);

        Ogre::Real weightedMass = this->mass->getReal(); /** scale.x * scale.y * scale.z;*/ // scale is not used anymore, because if big game objects are scaled down, the mass is to low!

        // set mass origin
        this->physicsBody->setCenterOfMass(calculatedMassOrigin);

        if (this->collisionType->getListSelectedValue() == "ConvexHull")
        {
            this->physicsBody->setConvexIntertialMatrix(inertia, calculatedMassOrigin);
        }

        // Apply mass and scale to inertia (the bigger the object, the more mass)
        inertia *= weightedMass;
        this->physicsBody->setMassMatrix(weightedMass, inertia);

        if (this->linearDamping->getReal() != 0.0f)
        {
            this->physicsBody->setLinearDamping(this->linearDamping->getReal());
        }
        if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
        {
            this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
        }

        this->setCollidable(this->collidable->getBool());

        // Note: Kinematic Body has not force and torque callback!
        // this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveKinematicComponent>(&PhysicsActiveComponent::moveCallback, this);
        this->physicsBody->removeForceAndTorqueCallback();

        this->setActivated(this->activated->getBool());

        // set user data for ogrenewt
        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
        this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

        // Attention: setKinematicPositionOrientation, NOT setPosition/setOrientation.
        //
        // PhysicsComponent::setPosition() forwards to OgreNewt::Body::setPositionOrientation(),
        // and that setter does not move a KinematicBody. The body therefore stayed at the
        // default transform of its constructor - the world origin - even though the scene
        // position from the XML was handed in right here. Everything downstream inherited that:
        // the collision hull sat at 0 0 0 while the mesh was rendered wherever it belonged, and
        // the body's contact map stayed empty because nothing was ever near it.
        //
        // Both values also have to go in with ONE call. Setting them separately makes the second
        // call read the first one's value back out of the body, which is the exact trap
        // PhysicsActiveComponent::init() already documents ("pos, orientation must be set at
        // once, else orientation will be the old one").
        this->setKinematicPositionOrientation(this->initialPosition, this->initialOrientation);

        this->setConstraintAxis(this->constraintAxis->getVector3());

        // pin the object stand in pose and not fall down
        this->setConstraintDirection(this->constraintDirection->getVector3());

        this->setCollidable(this->collidable->getBool());

        this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
        this->physicsBody->setMaterialGroupID(materialId);

        return true;
    }

    void PhysicsActiveKinematicComponent::kinematicContactCallback(OgreNewt::Body* otherBody)
    {
        if (nullptr == otherBody)
        {
            return;
        }

        // A body that cannot be cast to a physics component has no owning game object - the
        // normal case for e.g. ragdoll bones, which are plain bodies. GenericContactCallback
        // guards this very cast for exactly that reason; here it was dereferenced unchecked,
        // and only inside the deferred command, so the crash would surface a frame later and
        // far away from its cause.
        PhysicsComponent* otherPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(otherBody->getUserData());
        if (nullptr == otherPhysicsComponent)
        {
            // TEMPORARY DIAGNOSTICS - a body without a physics component has no owning game
            // object. Normal for ragdoll bones, suspicious for anything else.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveKinematicComponent-DIAG] Contact DROPPED: other body has no physics component.");
            return;
        }

        GameObjectPtr otherGameObjectPtr = otherPhysicsComponent->getOwner();
        if (nullptr == otherGameObjectPtr)
        {
            // TEMPORARY DIAGNOSTICS
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveKinematicComponent-DIAG] Contact DROPPED: other physics component has no owner.");
            return;
        }

        if (nullptr == this->gameObjectPtr->getLuaScript())
        {
            // TEMPORARY DIAGNOSTICS
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveKinematicComponent-DIAG] Contact DROPPED: no lua script on " + this->gameObjectPtr->getName() + ".");
            return;
        }

        // Resolved NOW rather than inside the command: this callback runs on a physics worker
        // thread while the command runs later on the logic thread, so anything read at
        // execution time may already have changed. The owner is captured as a shared pointer,
        // which additionally keeps the other game object alive until the call has happened.
        const Ogre::String capturedFunctionName = this->onKinematicContactFunctionName->getString();

        // The weak pointer covers this component being destroyed between enqueueing and
        // execution - the command's first line used to touch this->gameObjectPtr before any
        // check could run.
        boost::weak_ptr<GameObjectComponent> weakThis = this->shared_from_this();

        NOWA::AppStateManager::LogicCommand logicCommand = [this, weakThis, otherGameObjectPtr, capturedFunctionName]()
        {
            boost::shared_ptr<GameObjectComponent> strongThis = weakThis.lock();
            if (nullptr == strongThis)
            {
                return;
            }

            // Re-checked here as well: the null check above happened one or more frames ago
            // and the script may have been torn down since.
            LuaScript* luaScript = this->gameObjectPtr->getLuaScript();
            if (nullptr == luaScript)
            {
                return;
            }

            luaScript->callTableFunction(capturedFunctionName, otherGameObjectPtr);
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    void PhysicsActiveKinematicComponent::setOmegaVelocityRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength)
    {
        if (nullptr == this->physicsBody)
        {
            return;
        }

        Ogre::Quaternion diffOrientation = this->physicsBody->getOrientation().Inverse() * resultOrientation;
        Ogre::Vector3 resultVector = Ogre::Vector3::ZERO;

        if (axes.x == 1.0f)
        {
            resultVector.x = diffOrientation.getPitch().valueDegrees() * strength;
        }
        if (axes.y == 1.0f)
        {
            resultVector.y = diffOrientation.getYaw().valueDegrees() * strength;
        }
        if (axes.z == 1.0f)
        {
            resultVector.z = diffOrientation.getRoll().valueDegrees() * strength;
        }

        this->setOmegaVelocity(resultVector);
    }

    void PhysicsActiveKinematicComponent::setKinematicContactSolvingEnabled(bool enable)
    {
        // TEMPORARY DIAGNOSTICS - remove once the kinematic contact is understood.
        //
        // This guard used to return silently, and it is the most likely reason for a contact
        // function that is never called: connect() calls this, and if the lua script is not
        // attached to the game object YET at that moment, the callback is simply never
        // installed and nothing in the log says so. Every other lua driven component in the
        // engine (AiLuaComponent, PlayerControllerJumpNRunLuaComponent) waits for
        // EventDataLuaScriptConnected instead of relying on connect() ordering.
        if (nullptr == this->gameObjectPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveKinematicComponent-DIAG] setKinematicContactSolvingEnabled ABORTED: no game object.");
            return;
        }
        if (nullptr == this->gameObjectPtr->getLuaScript())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveKinematicComponent-DIAG] setKinematicContactSolvingEnabled ABORTED for game object: " + this->gameObjectPtr->getName() +
                                                                                    " because there is NO LUA SCRIPT on it (yet). Function name was: '" + this->onKinematicContactFunctionName->getString() + "'.");
            return;
        }
        if (true == this->onKinematicContactFunctionName->getString().empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[PhysicsActiveKinematicComponent-DIAG] setKinematicContactSolvingEnabled ABORTED for game object: " + this->gameObjectPtr->getName() + " because 'OnKinematicContactFunctionName' is EMPTY.");
            return;
        }
        if (nullptr == this->physicsBody)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[PhysicsActiveKinematicComponent-DIAG] setKinematicContactSolvingEnabled ABORTED for game object: " + this->gameObjectPtr->getName() + " because there is no physics body.");
            return;
        }

        if (true == enable)
        {
            static_cast<OgreNewt::KinematicBody*>(this->physicsBody)->setKinematicContactCallback<PhysicsActiveKinematicComponent>(&PhysicsActiveKinematicComponent::kinematicContactCallback, this);
        }
        else
        {
            static_cast<OgreNewt::KinematicBody*>(this->physicsBody)->removeKinematicContactCallback();
        }
    }

}; // namespace end