#include "NOWAPrecompiled.h"
#include "PhysicsRagDollComponent.h"
#include "JointComponents.h"
#include "TagPointComponent.h"
#include "main/AppStateManager.h"
#include "utilities/AnimationBlender.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    typedef boost::shared_ptr<JointComponent> JointCompPtr;
    typedef boost::shared_ptr<JointBallAndSocketComponent> JointBallAndSocketCompPtr;
    typedef boost::shared_ptr<JointHingeComponent> JointHingeCompPtr;
    typedef boost::shared_ptr<JointHingeActuatorComponent> JointHingeActuatorCompPtr;
    typedef boost::shared_ptr<JointUniversalComponent> JointUniversalCompPtr;
    typedef boost::shared_ptr<JointUniversalActuatorComponent> JointUniversalActuatorCompPtr;
    typedef boost::shared_ptr<JointKinematicComponent> JointKinematicCompPtr;

    // typedef boost::shared_ptr<RagDollMotorDofComponent> RagDollMotorDofCompPtr;

    PhysicsRagDollComponent::PhysicsRagDollComponent() :
        PhysicsActiveComponent(),
        rdState(PhysicsRagDollComponent::INACTIVE),
        rdOldState(PhysicsRagDollComponent::INACTIVE),
        skeleton(nullptr),
        animationEnabled(true),
        ragdollPositionOffset(Ogre::Vector3::ZERO),
        ragdollOrientationOffset(Ogre::Quaternion::IDENTITY),
        activated(new Variant(PhysicsRagDollComponent::AttrActivated(), true, this->attributes)),
        boneConfigFile(new Variant(PhysicsRagDollComponent::AttrBonesConfigFile(), Ogre::String(), this->attributes)),
        state(new Variant(PhysicsRagDollComponent::AttrState(), {"Inactive", "Animation", "Ragdolling", "PartialRagdolling"}, this->attributes)),
        isSimulating(false),
        oldAnimationId(-1)
    {
        this->boneConfigFile->setDescription("Name of the XML config file, no path, e.g. FlubberRagdoll.rag");
        this->boneConfigFile->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
        // Even if the value does not change, force resetting the same value, so that the script will re-parsed
        // this->boneConfigFile->setUserData("ForceSet");

        this->gyroscopicTorque->setValue(false);
        this->gyroscopicTorque->setVisible(false);

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponent::gameObjectAnimationChangedDelegate), EventDataAnimationChanged::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponent::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());
    }

    PhysicsRagDollComponent::~PhysicsRagDollComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] Destructor physics rag doll component for game object: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponent::gameObjectAnimationChangedDelegate), EventDataAnimationChanged::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponent::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());

        while (this->ragDataList.size() > 0)
        {
            RagBone* ragBone = this->ragDataList.back().ragBone;
            delete ragBone;
            this->ragDataList.pop_back();
        }
    }

    bool PhysicsRagDollComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        PhysicsActiveComponent::parseCommonProperties(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BoneConfigFile")
        {
            this->boneConfigFile->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (this->boneConfigFile->getString().length() == 0)
        {
            return false;
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "State")
        {
            Ogre::String stateCaption = XMLConverter::getAttrib(propertyElement, "data", "Inactive");
            this->state->setListSelectedValue(stateCaption);
            if (stateCaption == "Inactive")
            {
                this->rdState = PhysicsRagDollComponent::INACTIVE;
            }
            else if (stateCaption == "Animation")
            {
                this->rdState = PhysicsRagDollComponent::ANIMATION;
            }
            else if (stateCaption == "Ragdolling")
            {
                this->rdState = PhysicsRagDollComponent::RAGDOLLING;
                this->animationEnabled = false;
            }
            else if (stateCaption == "PartialRagdolling")
            {
                this->rdState = PhysicsRagDollComponent::PARTIAL_RAGDOLLING;
            }
            propertyElement = propertyElement->next_sibling("property");
        }

        // PhysicsActiveComponent::parseCompoundGameObjectProperties(propertyElement, filename);

        return true;
    }

    GameObjectCompPtr PhysicsRagDollComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PhysicsRagDollCompPtr clonedCompPtr(boost::make_shared<PhysicsRagDollComponent>());

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
        // clonedCompPtr->setCollidable(this->collidable->getBool());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setBoneConfigFile(this->boneConfigFile->getString());
        clonedCompPtr->setState(this->state->getListSelectedValue());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool PhysicsRagDollComponent::postInit(void)
    {
        bool success = PhysicsComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] Post init physics rag doll component for game object: " + this->gameObjectPtr->getName());

        // Physics active component must be dynamic, else a mess occurs
        this->gameObjectPtr->setDynamic(true);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

        return success;
    }

    void PhysicsRagDollComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        this->rdState = PhysicsRagDollComponent::INACTIVE;
        boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        this->activated->setValue(false);
    }

    bool PhysicsRagDollComponent::createRagDoll(const Ogre::String& boneName)
    {
        if (this->rdState == PhysicsRagDollComponent::INACTIVE)
        {
            this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
            this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
            this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
        }

        if (true == this->boneConfigFile->getString().empty())
        {
            return true;
        }

        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->boneConfigFile->getString());
        if (stream.isNull())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Could not open bone configuration file: " + this->boneConfigFile->getString() + " for game object: " + this->gameObjectPtr->getName());
            return false;
        }

        rapidxml::xml_document<> doc;
        // Get the file contents
        Ogre::String data = stream->getAsString();
        // RapidXML needs mutable, null-terminated buffer
        std::vector<char> xmlBuffer(data.begin(), data.end());
        xmlBuffer.push_back('\0');
        try
        {
            doc.parse<0>(&xmlBuffer[0]);
        }
        catch (const rapidxml::parse_error&)
        {
            stream->close();
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Could not parse one configuration file: " + this->boneConfigFile->getString() + " for game object: " + this->gameObjectPtr->getName());
            return false;
        }
        stream->close();
        rapidxml::xml_node<>* root = doc.first_node();

        if (!root)
        {
            // error!
            Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponent] Error: cannot find 'root' in xml file: " + this->boneConfigFile->getString());
            return false;
        }

        // Check if the ragdoll config has a default pose
        Ogre::String defaultPose;
        if (auto* poseAttr = root->first_attribute("Pose"))
        {
            defaultPose = poseAttr->value();
        }

        if (auto* posOffsetAttr = root->first_attribute("PositionOffset"))
        {
            this->ragdollPositionOffset = Ogre::StringConverter::parseVector3(posOffsetAttr->value());
        }

        if (auto* orientOffsetAttr = root->first_attribute("OrientationOffset"))
        {
            Ogre::Vector3 dirOffset = Ogre::StringConverter::parseVector3(orientOffsetAttr->value());
            if (dirOffset != Ogre::Vector3::ZERO)
            {
                Ogre::Matrix3 rot;
                rot.FromEulerAnglesXYZ(Ogre::Degree(dirOffset.x), Ogre::Degree(dirOffset.y), Ogre::Degree(dirOffset.z));
                this->ragdollOrientationOffset.FromRotationMatrix(rot);
            }
        }

        // Get the skeleton.
        Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
        if (nullptr == entity)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error cannot create rag doll body, because the game object has no entity with mesh for game object: " + this->gameObjectPtr->getName());
            return false;
        }

        this->skeleton = entity->getSkeleton();
        // entity->setAlwaysUpdateMainSkeleton(true);
        // entity->setUpdateBoundingBoxFromSkeleton(true);

        // Get the mesh.
        this->mesh = entity->getMesh();
        bool success = false;

        Ogre::v1::AnimationState* animState = nullptr;
        if (defaultPose.length() > 0)
        {
            // Get the default pose animation state (ideally the T-Pose)
            animState = entity->getAnimationState(defaultPose);
            animState->setEnabled(true);
            animState->setLoop(false);
            animState->setTimePosition(100.0f);
            entity->_updateAnimation(); // critical! read this functions comments!
        }

        if (nullptr != animState)
        {
            animState->setEnabled(false);
        }

        // First delete all prior created ragbones
        while (this->ragDataList.size() > 0)
        {
            RagBone* ragBone = this->ragDataList.back().ragBone;
            delete ragBone;
            this->ragDataList.pop_back();
        }

        if (nullptr != this->rootJointCompPtr)
        {
            this->rootJointCompPtr.reset();
        }

        success = this->createRagDollFromXML(root, boneName);

        if (false == success)
        {
            return false;
        }

        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
        this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
        this->physicsBody->setMaterialGroupID(materialId);

        return true;
    }

    /*
    bool OgreRagdollOX3D::SetRagdollManualBoneAnim(String boneName, bool active)
{
    Ogre::Bone *bone;

    if (rSkeleton->hasBone(boneName))
    {
        bone = rSkeleton->getBone(boneName);
        bone->setManuallyControlled(active);
        rRagdollBoneControl = active;

        Ogre::AnimationStateIterator animations = rRagdollMesh->GetEntity()->getAllAnimationStates()->getAnimationStateIterator();

        while (animations.hasMoreElements())
        {
            Ogre::AnimationState *a = animations.getNext();
            if (active)
            {
                a->createBlendMask(rSkeleton->getNumBones(), 1);
                a->setBlendMaskEntry(bone->getHandle(), !active);
            }
            else {
                a->destroyBlendMask();
            }
        }
        return true;
    }
    else return false;
}
    */

    bool PhysicsRagDollComponent::connect(void)
    {
        bool success = PhysicsActiveComponent::connect();

        this->isSimulating = true;

        this->internalApplyState();

        return success;
    }

    bool PhysicsRagDollComponent::disconnect(void)
    {
        bool success = PhysicsActiveComponent::disconnect();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->isSimulating = false;
        // Must be set manually because, only at connect, all data for debug is available (bodies, joints)
        this->internalShowDebugData(false);

        if (this->rdState == PhysicsRagDollComponent::RAGDOLLING || this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            this->endRagdolling();
        }

        this->rdOldState = PhysicsRagDollComponent::INACTIVE;

        return success;
    }

    Ogre::String PhysicsRagDollComponent::getClassName(void) const
    {
        return "PhysicsRagDollComponent";
    }

    Ogre::String PhysicsRagDollComponent::getParentClassName(void) const
    {
        return "PhysicsActiveComponent";
    }

    Ogre::String PhysicsRagDollComponent::getParentParentClassName(void) const
    {
        return "PhysicsComponent";
    }

    void PhysicsRagDollComponent::gameObjectAnimationChangedDelegate(EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataAnimationChanged> castEventData = boost::static_pointer_cast<NOWA::EventDataAnimationChanged>(eventData);
        // if this game object has an physics rag doll component and its in ragdolling state, no animation must be processed, else the skeleton will throw apart!
        unsigned long id = castEventData->getGameObjectId();

        if (this->gameObjectPtr->getId() == id)
        {
            if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING && AnimationBlender::ANIM_JUMP_START == castEventData->getAnimationId() && this->oldAnimationId != castEventData->getAnimationId())
            {
                // this->applyModelStateToRagdoll();

                // this->endRagdolling();
                // this->internalApplyState();

                /*for (size_t j = 0; j < this->ragDataList.size(); j++)
                {
                    this->ragDataList[j].ragBone->detachFromNode();
                }

                this->startRagdolling();*/
            }

            this->oldAnimationId = castEventData->getAnimationId();
        }
    }

    void PhysicsRagDollComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (this->rdState == PhysicsRagDollComponent::RAGDOLLING || this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
            {
                if (true == activated)
                {
                    (*it).ragBone->getBody()->unFreeze();
                }
                else
                {
                    (*it).ragBone->getBody()->freeze();
                }
            }
        }
    }

    void PhysicsRagDollComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == this->activated->getBool() && false == notSimulating)
        {
            if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING || this->rdState == PhysicsRagDollComponent::RAGDOLLING)
            {
                // WOW! Must be called, because by calling setSkipAnimationStateUpdate, no animation will work, but bodies do match bones transforms. Hence calling this each frame will also let
                // animations working and bones match bodies transforms!!!!
                if (nullptr != this->skeleton)
                {
                    auto closureFunction = [this](Ogre::Real renderDt)
                    {
                        this->skeleton->setAnimationState(*this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->getAllAnimationStates());
                        // this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->_updateAnimation();
                        // Note: Order here is important!
                        this->applyRagdollStateToModel();
                    };
                    Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
                    NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
                }
            }

            if (this->rdState == PhysicsRagDollComponent::RAGDOLLING || this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
            {
                for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
                {
                    (*it).ragBone->update(dt, notSimulating);
                }
            }
        }
    }

    void PhysicsRagDollComponent::actualizeValue(Variant* attribute)
    {
        PhysicsActiveComponent::actualizeCommonValue(attribute);

        // PhysicsActiveComponent::actualizeCompoundGameObjectValue(attribute);

        if (PhysicsRagDollComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (PhysicsRagDollComponent::AttrBonesConfigFile() == attribute->getName())
        {
            this->setBoneConfigFile(attribute->getString());
        }
        else if (PhysicsRagDollComponent::AttrState() == attribute->getName())
        {
            this->setState(attribute->getListSelectedValue());
        }
    }

    void PhysicsRagDollComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int
        // 6 = real
        // 7 = string
        // 8 = vector2
        // 9 = vector3
        // 10 = vector4 -> also quaternion
        // 12 = bool
        PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "BoneConfigFile"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->boneConfigFile->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "State"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->state->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);
    }

    void PhysicsRagDollComponent::showDebugData(void)
    {
        GameObjectComponent::showDebugData();
    }

    void PhysicsRagDollComponent::internalShowDebugData(bool activate)
    {
        // Complex cases: Since everything is done manually, when user pressed e.g. debug button, debug data can only be shown, when go has been connected
        // On disconnect, the debug data must be hidden in any case, because almost everything will be destroyed
        if (false == this->ragDataList.empty())
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, activate]()
            {
                for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
                {
                    if (true == activate)
                    {
                        (*it).ragBone->getBody()->showDebugCollision(false, this->bShowDebugData && activate);
                        if (nullptr != (*it).ragBone->getJointComponent())
                        {
                            (*it).ragBone->getJointComponent()->showDebugData();
                            (*it).ragBone->getJointComponent()->forceShowDebugData(this->bShowDebugData && activate);
                        }
                    }
                    else
                    {
                        (*it).ragBone->getBody()->showDebugCollision(false, false);
                        if (nullptr != (*it).ragBone->getJointComponent())
                        {
                            (*it).ragBone->getJointComponent()->showDebugData();
                            (*it).ragBone->getJointComponent()->forceShowDebugData(false);
                        }
                    }
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponent::internalShowDebugData");
        }
        else
        {
            if (nullptr != this->physicsBody)
            {
                NOWA::GraphicsModule::RenderCommand renderCommand = [this, activate]()
                {
                    if (true == activate)
                    {
                        this->physicsBody->showDebugCollision(false, this->bShowDebugData && activate);
                    }
                    else
                    {
                        this->physicsBody->showDebugCollision(false, false);
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponent::internalShowDebugData2");
            }
        }
    }

    void PhysicsRagDollComponent::internalApplyState(void)
    {
        if (true == this->isSimulating)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                // First deactivate debug data if set
                this->internalShowDebugData(false);

                if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
                {
                    boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), true));
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGameObjectIsInRagDollingState);

                    // if (this->rdOldState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
                    {
                        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
                        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
                        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
                    }

                    this->partialRagdollBoneName = "";
                    this->releaseConstraintDirection();
                    this->releaseConstraintAxis();
                    // Must be placed, else after one time disconnect, a crash occurs
                    this->destroyBody();
                    this->createRagDoll(this->partialRagdollBoneName);
                    this->startRagdolling();
                }
                else if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
                {
                    boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGameObjectIsInRagDollingState);

                    // Must be called, else when re-activating this state, the bones will become a mess!
                    this->endRagdolling();

                    this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
                    if (this->rdOldState == PhysicsRagDollComponent::RAGDOLLING)
                    {
                        // Set a bit higher, so that the collision hull may be created above the ground, if changed from ragdoll to inactive
                        this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
                    }
                    this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
                    // Only use initial orientation, if the old state was not ragdolling, because else, the player is rotated in wrong manner and constraint axis, up vector will be incorrect
                    if (this->rdOldState != PhysicsRagDollComponent::RAGDOLLING)
                    {
                        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
                    }

                    this->releaseConstraintDirection();
                    this->releaseConstraintAxis();
                    // Must be placed, else after one time disconnect, a crash occurs
                    this->destroyBody();
                    this->createRagDoll(this->partialRagdollBoneName);

                    this->startRagdolling();
                }
                else if (this->rdState == PhysicsRagDollComponent::INACTIVE)
                {
                    createInactiveRagdoll();
                }
                else if (this->rdState == PhysicsRagDollComponent::ANIMATION)
                {
                    boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGameObjectIsInRagDollingState);

                    // Must be called, else when re-activating this state, the bones will become a mess!
                    this->endRagdolling();

                    this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
                    if (this->rdOldState == PhysicsRagDollComponent::RAGDOLLING)
                    {
                        // Set a bit higher, so that the collision hull may be created above the ground, if changed from ragdoll to inactive
                        this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
                    }
                    this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
                    // Only use initial orientation, if the old state was not ragdolling, because else, the player is rotated in wrong manner and constraint axis, up vector will be incorrect
                    if (this->rdOldState != PhysicsRagDollComponent::RAGDOLLING)
                    {
                        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
                    }

                    this->releaseConstraintDirection();
                    this->releaseConstraintAxis();
                    // Must be placed, else after one time disconnect, a crash occurs
                    this->destroyBody();
                    this->createDynamicBody();
                }

                // Must be set manually because, only at connect, all data for debug is available (bodies, joints)
                this->internalShowDebugData(true);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponent::internalApplyState");
        }
    }

    void PhysicsRagDollComponent::createInactiveRagdoll()
    {
        boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGameObjectIsInRagDollingState);

        this->partialRagdollBoneName = "";

        this->destroyBody();

        this->endRagdolling();
        this->releaseConstraintDirection();
        this->releaseConstraintAxis();

        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        if (this->rdOldState == PhysicsRagDollComponent::RAGDOLLING)
        {
            // Set a bit higher, so that the collision hull may be created above the ground, if changed from ragdoll to inactive
            this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
        }
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        // Only use initial orientation, if the old state was not ragdolling, because else, the player is rotated in wrong manner and constraint axis, up vector will be incorrect
        if (this->rdOldState != PhysicsRagDollComponent::RAGDOLLING)
        {
            this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
        }

        this->createDynamicBody();

        this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->setSkipAnimationStateUpdate(false);
    }

    void PhysicsRagDollComponent::setBoneConfigFile(const Ogre::String& boneConfigFile)
    {
        this->boneConfigFile->setValue(boneConfigFile);
    }

    Ogre::String PhysicsRagDollComponent::getBoneConfigFile(void) const
    {
        return this->boneConfigFile->getString();
    }

    void PhysicsRagDollComponent::setVelocity(const Ogre::Vector3& velocity)
    {
        if (true == this->isSimulating)
        {
            /*if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
            {
                for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
                {
                    OgreNewt::Body* body = (*it).ragBone->getBody();
                    body->setVelocity(velocity);
                }
            }
            else if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
            {
                for (auto& it = this->ragDataList.begin() + 1; it != this->ragDataList.end(); ++it)
                {
                    OgreNewt::Body* body = (*it).ragBone->getBody();
                    body->setVelocity(velocity);
                }
            }
            else*/
            {
                this->physicsBody->setVelocity(velocity);
            }
        }
    }

    Ogre::Vector3 PhysicsRagDollComponent::getVelocity(void) const
    {
        if (true == this->isSimulating)
        {
            if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
            {
                return this->ragDataList[0].ragBone->getBody()->getVelocity();
            }
            else
            {
                if (nullptr != this->physicsBody)
                {
                    return this->physicsBody->getVelocity();
                }
            }
        }
        return Ogre::Vector3::ZERO;
    }

    void PhysicsRagDollComponent::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
    {
        if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                // this->gameObjectPtr->setAttributePosition(Ogre::Vector3(x, y, z));
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->gameObjectPtr->getSceneNode(), Ogre::Vector3(x, y, z));
            }
            else
            {
                PhysicsComponent::setPosition(x, y, z);
            }
        }
        else
        {
            PhysicsComponent::setPosition(x, y, z);
        }
    }

    void PhysicsRagDollComponent::setPosition(const Ogre::Vector3& position)
    {
        if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                // this->gameObjectPtr->setAttributePosition(position);
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->gameObjectPtr->getSceneNode(), position);
            }
            else
            {
                PhysicsComponent::setPosition(position);
            }
        }
        else
        {
            PhysicsComponent::setPosition(position);
        }
    }

    void PhysicsRagDollComponent::translate(const Ogre::Vector3& relativePosition)
    {
        // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] pos1: " + Ogre::StringConverter::toString(relativePosition));
        if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                // this->gameObjectPtr->getSceneNode()->translate(relativePosition);
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->gameObjectPtr->getSceneNode(), this->gameObjectPtr->getSceneNode()->getPosition() + relativePosition);
            }
        }
        else
        {
            PhysicsComponent::translate(relativePosition);
        }
    }

    Ogre::Vector3 PhysicsRagDollComponent::getPosition(void) const
    {
        if (false == this->isSimulating || nullptr == this->physicsBody)
        {
            return this->gameObjectPtr->getPosition();
        }
        else
        {
            return this->physicsBody->getPosition();
        }
    }

    void PhysicsRagDollComponent::setOrientation(const Ogre::Quaternion& orientation)
    {
        if (this->rdState == PhysicsRagDollComponent::RAGDOLLING || this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                // this->gameObjectPtr->getSceneNode()->setOrientation(orientation);
                NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->gameObjectPtr->getSceneNode(), orientation);
            }
            else
            {
                size_t offset = 0;
                // When partial ragdolling is active, the first bone is the main body and should not apply to rag doll!
                /*if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
                {
                    offset = 1;
                }
                for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
                {
                    it->ragBone->setOrientation(orientation);
                }*/
                PhysicsComponent::setOrientation(orientation);
            }
        }
        else
        {
            PhysicsComponent::setOrientation(orientation);
        }
    }

    void PhysicsRagDollComponent::rotate(const Ogre::Quaternion& relativeRotation)
    {
        if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                // this->gameObjectPtr->getSceneNode()->rotate(relativeRotation);
                NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->gameObjectPtr->getSceneNode(), this->gameObjectPtr->getSceneNode()->getOrientation() * relativeRotation);
            }
        }
        else
        {
            PhysicsComponent::rotate(relativeRotation);
        }
    }

    Ogre::Quaternion PhysicsRagDollComponent::getOrientation(void) const
    {
        if (false == this->isSimulating || nullptr == this->physicsBody)
        {
            return this->gameObjectPtr->getOrientation();
        }
        else
        {
            return this->physicsBody->getOrientation();
        }
    }

    void PhysicsRagDollComponent::setBoneRotation(const Ogre::String& boneName, const Ogre::Vector3& axis, Ogre::Real degree)
    {
        this->oldPartialRagdollBoneName = boneName;
        this->partialRagdollBoneName = boneName;
        this->rdOldState = this->rdState;
        /*this->rdState = PhysicsRagDollComponent::PARTIAL_RAGDOLLING;

        RagBone* ragBone = this->getRagBone(boneName);
        ragBone->getBody()->setTorque(axis * (degree * Ogre::Math::PI / 180.0f));*/
        // Here via joint
        /*JointUniversalCompPtr jointCompPtr = boost::dynamic_pointer_cast<JointUniversalComponent>(ragBone->getJointComponent());
        jointCompPtr->setMotorEnabled(true);
        jointCompPtr->setMotorSpeed(10.0f);*/
    }

    void PhysicsRagDollComponent::setInitialState(void)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            size_t offset = 0;
            // When partial ragdolling is active, the first bone is the main body and should not apply to rag doll!
            if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
            {
                offset = 1;
            }

            for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
            {
                it->ragBone->setInitialState();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponent::setInitialState");
    }

    void PhysicsRagDollComponent::setAnimationEnabled(bool animationEnabled)
    {
        this->animationEnabled = animationEnabled;

        size_t offset = 0;
        // When partial ragdolling is active, the first bone is the main body and should not apply to rag doll!
        if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            offset = 1;
        }

        for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
        {
            it->ragBone->getOgreBone()->setManuallyControlled(!animationEnabled);
            it->ragBone->applyPose(Ogre::Vector3::ZERO);
        }
    }

    bool PhysicsRagDollComponent::isAnimationEnabled(void) const
    {
        return this->animationEnabled;
    }

    bool PhysicsRagDollComponent::createRagDollFromXML(rapidxml::xml_node<>* rootXmlElement, const Ogre::String& boneName)
    {
        // add all children of this bone.
        RagBone* parentRagBone = nullptr;

        rapidxml::xml_node<>* bonesXmlElement = rootXmlElement->first_node("Bones");
        if (nullptr == bonesXmlElement)
        {
            Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponent] Error: cannot find 'Bones' XML element in xml file: " + this->boneConfigFile->getString());
            return false;
        }

        bool partial = false;
        bool first = true;
        if (PhysicsRagDollComponent::PARTIAL_RAGDOLLING == this->rdState)
        {
            partial = true;
            this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->setUpdateBoundingBoxFromSkeleton(true);

            // WOW! this is it!!!! This causes in partial ragdoll, that the entity's bones have a bit offset between the bodies. But after this call, partial ragdoll still will
            // not work because no animation is working. Hence in update, this->skeleton->setAnimationState(*this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->getAllAnimationStates());
            // must be called
            // https://forums.ogre3d.org/viewtopic.php?t=79731
            this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->setSkipAnimationStateUpdate(true);
        }

        rapidxml::xml_node<>* boneXmlElement = bonesXmlElement->first_node("Bone");
        while (nullptr != boneXmlElement)
        {
            // Check if there is a source, that points to a target for position correction
            if (auto* sourceAttr = boneXmlElement->first_attribute("Source"))
            {
                Ogre::String sourceBoneName = sourceAttr->value();
                if (false == boneName.empty())
                {
                    // Found bone name at which all other targets should be parsed, so parse til the bone name is found and proceed as usual after that
                    // So that a partial ragdoll is build
                    if (sourceBoneName != boneName)
                    {
                        boneXmlElement = boneXmlElement->next_sibling("Bone");
                        continue;
                    }
                }

                auto* targetAttr = boneXmlElement->first_attribute("Target");
                Ogre::String targetBoneName = targetAttr ? targetAttr->value() : "";

                auto* offsetAttr = boneXmlElement->first_attribute("Offset");
                Ogre::Vector3 offset = Ogre::Vector3::ZERO;
                if (offsetAttr)
                {
                    offset = Ogre::StringConverter::parseVector3(offsetAttr->value());
                }

                Ogre::v1::OldBone* sourceBone = this->skeleton->getBone(sourceBoneName);
                Ogre::v1::OldBone* targetBone = this->skeleton->getBone(targetBoneName);
                this->boneCorrectionMap.emplace(sourceBone, std::make_pair(targetBone, offset));

                boneXmlElement = boneXmlElement->next_sibling("Bone");
                continue;
            }

            // get the information for the bone represented by this element.
            auto* sizeAttr = boneXmlElement->first_attribute("Size");
            if (nullptr == sizeAttr)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Size could not be loaded from XML for: " + this->gameObjectPtr->getName());
                return false;
            }
            Ogre::Vector3 size = Ogre::StringConverter::parseVector3(sizeAttr->value());

            auto* skeletonBoneAttr = boneXmlElement->first_attribute("SkeletonBone");
            Ogre::String skeletonBone = skeletonBoneAttr ? skeletonBoneAttr->value() : "";

            // Partial ragdolling root bone can be left off, so that position and orientation is taken from whole mesh (position, orientation)
            // This is useful, because when attaching to a root bone, it may be that the bone is somewhat feeble and so would be the whole character!
            if (skeletonBone.empty() && (false == first || false == partial))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Skeleton bone name could not be loaded from XML for: " + this->gameObjectPtr->getName());
                return false;
            }

            Ogre::v1::OldBone* ogreBone = nullptr;
            if (false == skeletonBone.empty())
            {
                try
                {
                    ogreBone = this->skeleton->getBone(skeletonBone);
                }
                catch (const Ogre::Exception&)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Skeleton bone name: " + skeletonBone + " for game object: " + this->gameObjectPtr->getName());
                    return false;
                }
            }

            Ogre::String boneNameFromFile;
            auto* nameAttr = boneXmlElement->first_attribute("Name");
            Ogre::String strName = nameAttr ? nameAttr->value() : "";
            if (!strName.empty())
            {
                boneNameFromFile = strName;
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Representation name could not be loaded from XML for: " + this->gameObjectPtr->getName());
                return false;
            }

            Ogre::String strCategory;
            if (auto* catAttr = boneXmlElement->first_attribute("Category"))
            {
                strCategory = catAttr->value();
            }

            Ogre::Vector3 offset = Ogre::Vector3::ZERO;
            if (auto* offsetAttr2 = boneXmlElement->first_attribute("Offset"))
            {
                offset = Ogre::StringConverter::parseVector3(offsetAttr2->value());
            }

            auto* shapeAttr = boneXmlElement->first_attribute("Shape");
            Ogre::String strShape = shapeAttr ? shapeAttr->value() : "";
            PhysicsRagDollComponent::RagBone::BoneShape shape = PhysicsRagDollComponent::RagBone::BS_BOX;

            if (strShape == "Hull")
            {
                shape = PhysicsRagDollComponent::RagBone::BS_CONVEXHULL;
            }
            else if (strShape == "Box")
            {
                shape = PhysicsRagDollComponent::RagBone::BS_BOX;
            }
            else if (strShape == "Capsule")
            {
                shape = PhysicsRagDollComponent::RagBone::BS_CAPSULE;
            }
            else if (strShape == "Cylinder")
            {
                shape = PhysicsRagDollComponent::RagBone::BS_CYLINDER;
            }
            else if (strShape == "Cone")
            {
                shape = PhysicsRagDollComponent::RagBone::BS_CONE;
            }
            else if (strShape == "Ellipsoid")
            {
                shape = PhysicsRagDollComponent::RagBone::BS_ELLIPSOID;
            }

            auto* massAttr = boneXmlElement->first_attribute("Mass");
            if (nullptr == massAttr)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Mass could not be loaded from XML for: " + this->gameObjectPtr->getName());
                return false;
            }
            Ogre::Real mass = Ogre::StringConverter::parseReal(massAttr->value());
            // this->mass += mass;

            ///////////////////////////////////////////////////////////////////////////////

            RagBone* currentRagBone = new RagBone(boneNameFromFile, this, parentRagBone, ogreBone, this->mesh, constraintDirection->getVector3(), shape, size, mass, partial, offset, strCategory);

            // add a name from file, if it exists, because it could be, that someone that rigged the mesh, did not name it properly, so this can be done in the *.xml file

            PhysicsRagDollComponent::RagData ragData;
            ragData.ragBoneName = boneNameFromFile;
            ragData.ragBone = currentRagBone;
            this->ragDataList.emplace_back(ragData);

            boneXmlElement = boneXmlElement->next_sibling("Bone");
            // store parent rag bone pointer
            parentRagBone = currentRagBone;

            first = false;
        }

        rapidxml::xml_node<>* jointsXmlElement = rootXmlElement->first_node("Joints");
        if (nullptr == jointsXmlElement)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: cannot find 'Joints' XML element in xml file: " + this->boneConfigFile->getString());
            return false;
        }

        // Go through all joints
        rapidxml::xml_node<>* jointXmlElement = jointsXmlElement->first_node("Joint");
        while (nullptr != jointXmlElement)
        {
            auto* childAttr = jointXmlElement->first_attribute("Child");
            Ogre::String ragBoneChildName = childAttr ? childAttr->value() : "";

            Ogre::String ragBoneParentName;
            if (auto* parentAttr = jointXmlElement->first_attribute("Parent"))
            {
                ragBoneParentName = parentAttr->value();
            }

            RagBone* childRagBone = this->getRagBone(ragBoneChildName);
            RagBone* parentRagBone = this->getRagBone(ragBoneParentName);

            if (nullptr == childRagBone)
            {
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Warning: There is no child rag bone name: " + ragBoneChildName + " specified for game object " + this->gameObjectPtr->getName());
            }

            Ogre::Real friction = -1.0f;
            if (auto* frictionAttr = jointXmlElement->first_attribute("Friction"))
            {
                friction = Ogre::StringConverter::parseReal(frictionAttr->value());
            }

            auto* typeAttr = jointXmlElement->first_attribute("Type");
            Ogre::String strJointType = typeAttr ? typeAttr->value() : "";
            PhysicsRagDollComponent::JointType jointType = PhysicsRagDollComponent::JT_BASE;
            Ogre::Vector3 pin = Ogre::Vector3::ZERO;

            if (strJointType == JointComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponent::JT_BASE;
            }
            else if (strJointType == JointBallAndSocketComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponent::JT_BALLSOCKET;
            }
            else if (strJointType == JointHingeComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponent::JT_HINGE;
                auto* pinAttr = jointXmlElement->first_attribute("Pin");
                if (pinAttr)
                {
                    pin = Ogre::StringConverter::parseVector3(pinAttr->value());
                }
                else
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                        "[PhysicsRagDollComponent] Error: Pin for hinge joint is missing in " + this->boneConfigFile->getString() + " for bone: '" + childRagBone->getName() + "' for game object " + this->gameObjectPtr->getName());
                    return false;
                }
            }
            else if (strJointType == JointUniversalComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponent::JT_DOUBLE_HINGE;
                if (auto* pinAttr = jointXmlElement->first_attribute("Pin"))
                {
                    pin = Ogre::StringConverter::parseVector3(pinAttr->value());
                }
            }
            else if (strJointType == JointHingeActuatorComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponent::JT_HINGE_ACTUATOR;
                auto* pinAttr = jointXmlElement->first_attribute("Pin");
                if (pinAttr)
                {
                    pin = Ogre::StringConverter::parseVector3(pinAttr->value());
                }
                else
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                        "[PhysicsRagDollComponent] Error: Pin for hinge actuator joint is missing in " + this->boneConfigFile->getString() + " for bone: '" + childRagBone->getName() + "' for game object " + this->gameObjectPtr->getName());
                    return false;
                }
            }
            else if (strJointType == JointUniversalActuatorComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponent::JT_DOUBLE_HINGE_ACTUATOR;
                auto* pinAttr = jointXmlElement->first_attribute("Pin");
                if (pinAttr)
                {
                    pin = Ogre::StringConverter::parseVector3(pinAttr->value());
                }
                else
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                        "[PhysicsRagDollComponent] Error: Pin for universal actuator joint is missing in " + this->boneConfigFile->getString() + " for bone: '" + childRagBone->getName() + "' for game object " + this->gameObjectPtr->getName());
                    return false;
                }
            }
            else if (strJointType == JointKinematicComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponent::JT_KINEMATIC;
            }

            Ogre::Degree minTwistAngle = Ogre::Degree(0.0f);
            Ogre::Degree maxTwistAngle = Ogre::Degree(0.0f);

            if (auto* minTwistAttr = jointXmlElement->first_attribute("MinTwistAngle"))
            {
                minTwistAngle = Ogre::Degree(Ogre::StringConverter::parseReal(minTwistAttr->value()));
            }
            if (auto* maxTwistAttr = jointXmlElement->first_attribute("MaxTwistAngle"))
            {
                maxTwistAngle = Ogre::Degree(Ogre::StringConverter::parseReal(maxTwistAttr->value()));
            }

            Ogre::Degree minTwistAngle2 = Ogre::Degree(0.0f);
            Ogre::Degree maxTwistAngle2 = Ogre::Degree(0.0f);

            Ogre::Degree maxConeAngle = Ogre::Degree(0.0f);

            if (auto* maxConeAttr = jointXmlElement->first_attribute("MaxConeAngle"))
            {
                maxConeAngle = Ogre::Degree(Ogre::StringConverter::parseReal(maxConeAttr->value()));
            }
            if (auto* minTwist2Attr = jointXmlElement->first_attribute("MinTwistAngle2"))
            {
                minTwistAngle2 = Ogre::Degree(Ogre::StringConverter::parseReal(minTwist2Attr->value()));
            }
            if (auto* maxTwist2Attr = jointXmlElement->first_attribute("MaxTwistAngle2"))
            {
                maxTwistAngle2 = Ogre::Degree(Ogre::StringConverter::parseReal(maxTwist2Attr->value()));
            }

            if (!(jointXmlElement->first_attribute("Child")) || !*(jointXmlElement->first_attribute("Child")->value()))
            {
                Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponent] Error: Cannot find 'Child' XML element in xml file: " + this->boneConfigFile->getString());
                return false;
            }

            bool useSpring = false;
            if (auto* springAttr = jointXmlElement->first_attribute("Spring"))
            {
                useSpring = Ogre::StringConverter::parseBool(springAttr->value());
            }

            Ogre::Vector3 offset = Ogre::Vector3::ZERO;
            if (auto* offsetAttr3 = jointXmlElement->first_attribute("Offset"))
            {
                offset = Ogre::StringConverter::parseVector3(offsetAttr3->value());
            }

            if (nullptr != childRagBone)
            {
                this->joinBones(jointType, childRagBone, parentRagBone, pin, minTwistAngle, maxTwistAngle, maxConeAngle, minTwistAngle2, maxTwistAngle2, friction, useSpring, offset);
            }
            jointXmlElement = jointXmlElement->next_sibling("Joint");
        }

        return true;
    }

    void PhysicsRagDollComponent::joinBones(PhysicsRagDollComponent::JointType type, RagBone* childRagBone, RagBone* parentRagBone, const Ogre::Vector3& pin, const Ogre::Degree& minTwistAngle, const Ogre::Degree& maxTwistAngle,
        const Ogre::Degree& maxConeAngle, const Ogre::Degree& minTwistAngle2, const Ogre::Degree& maxTwistAngle2, Ogre::Real friction, bool useSpring, const Ogre::Vector3& offset)
    {
        // http://newtondynamics.com/wiki/index.php5?title=Joints

        JointCompPtr parentJointCompPtr;
        JointCompPtr childJointCompPtr;

        // if the parent bone has no joint handler, create a default one (this one of course does not join anything, its just a placeholder to connect joint handlers together,
        // because a joint connection is just required for 2 bones)

        if (nullptr == parentRagBone)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent]: Error: Cannot joint bones, because the corresponding rag bone is null for game object: " + this->gameObjectPtr->getName());
            return;
        }

        if (type != PhysicsRagDollComponent::JT_KINEMATIC)
        {
            if (nullptr == parentRagBone->getJointComponent())
            {
                parentJointCompPtr = boost::dynamic_pointer_cast<JointComponent>(parentRagBone->createJointComponent(JointComponent::getStaticClassName()));
                parentJointCompPtr->setBody(parentRagBone->getBody());
                AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(parentJointCompPtr);
            }
            else
            {
                parentJointCompPtr = parentRagBone->getJointComponent();
            }
        }

        // Note this->getOrientation() etc. not required since when hinge is create orientation is also taken into account!

        switch (type)
        {
        case PhysicsRagDollComponent::JT_BASE:
        {
            JointCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointComponent>(childRagBone->createJointComponent(JointBallAndSocketComponent::getStaticClassName()));

            // tempChildCompPtr->setDofCount(2);
            // tempChildCompPtr->setMotorOn(true);
            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setStiffness(0.1f);
            // tempChildCompPtr->setDryFriction(0.5f);
            // Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            // Add also to GOC, so that other joints can be connected from the outside, but also create joint here, so that the body position has not changed
            // Because GOC will create later and ragdoll has been applied and body pos changed!
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponent::JT_BALLSOCKET:
        {
            JointBallAndSocketCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointBallAndSocketComponent>(childRagBone->createJointComponent(JointBallAndSocketComponent::getStaticClassName()));

            // tempChildCompPtr->setDofCount(2);
            // tempChildCompPtr->setMotorOn(true);
            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
            // tempChildCompPtr->setDryFriction(0.5f);
            // Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            tempChildCompPtr->setConeLimitsEnabled(true);
            tempChildCompPtr->setTwistLimitsEnabled(true);
            tempChildCompPtr->setMinMaxConeAngleLimit(minTwistAngle, maxTwistAngle, maxConeAngle);

            tempChildCompPtr->createJoint();

            if (-1.0f != friction)
            {
                tempChildCompPtr->setConeFriction(friction);
                tempChildCompPtr->setTwistFriction(friction);
            }

            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            // Add also to GOC, so that other joints can be connected from the outside, but also create joint here, so that the body position has not changed
            // Because GOC will create later and ragdoll has been applied and body pos changed!
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponent::JT_HINGE:
        {
            JointHingeCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointHingeComponent>(childRagBone->createJointComponent(JointHingeComponent::getStaticClassName()));

            // tempChildCompPtr->setDofCount(1);
            // tempChildCompPtr->setMotorOn(true);
            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
            // Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            tempChildCompPtr->setPin(pin);

            tempChildCompPtr->setLimitsEnabled(true);
            tempChildCompPtr->setMinMaxAngleLimit(minTwistAngle, maxTwistAngle);
            tempChildCompPtr->setStiffness(0.1f);
            // tempChildCompPtr->setSpring(useSpring);
            // tempChildCompPtr->setMinMaxConeAngleLimit(minTwistAngle, maxTwistAngle, maxConeAngle);
            tempChildCompPtr->createJoint();

            if (-1.0f != friction)
            {
                tempChildCompPtr->setFriction(friction);
            }

            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponent::JT_HINGE_ACTUATOR:
        {
            JointHingeActuatorCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointHingeActuatorComponent>(childRagBone->createJointComponent(JointHingeActuatorComponent::getStaticClassName()));

            // tempChildCompPtr->setDofCount(1);
            // tempChildCompPtr->setMotorOn(true);
            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
            // Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            tempChildCompPtr->setPin(pin);

            tempChildCompPtr->setMinAngleLimit(minTwistAngle);
            tempChildCompPtr->setMaxAngleLimit(maxTwistAngle);
            // tempChildCompPtr->setSpring(useSpring);

            tempChildCompPtr->createJoint();

            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponent::JT_DOUBLE_HINGE:
        {
            JointUniversalCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointUniversalComponent>(childRagBone->createJointComponent(JointUniversalComponent::getStaticClassName()));

            // tempChildCompPtr->setDofCount(1);
            // tempChildCompPtr->setMotorOn(true);
            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
            // Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            tempChildCompPtr->setPin(pin);

            tempChildCompPtr->setLimits0Enabled(true);
            tempChildCompPtr->setMinMaxAngleLimit1(minTwistAngle, maxTwistAngle);
            tempChildCompPtr->setLimits1Enabled(true);
            tempChildCompPtr->setMinMaxAngleLimit0(minTwistAngle2, maxTwistAngle2);
            tempChildCompPtr->setSpring0(useSpring, 1024.0f, 512.0f, 0.9f);
            tempChildCompPtr->setSpring1(useSpring, 1024.0f, 512.0f, 0.9f);
            tempChildCompPtr->createJoint();
            // tempChildCompPtr->setMinMaxConeAngleLimit(minTwistAngle, maxTwistAngle, maxConeAngle);
            if (-1.0f != friction)
            {
                tempChildCompPtr->setFriction0(friction);
                tempChildCompPtr->setFriction1(friction);
            }

            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponent::JT_DOUBLE_HINGE_ACTUATOR:
        {
            JointUniversalActuatorCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointUniversalActuatorComponent>(childRagBone->createJointComponent(JointUniversalActuatorComponent::getStaticClassName()));

            // tempChildCompPtr->setDofCount(1);
            // tempChildCompPtr->setMotorOn(true);
            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
            // Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            // tempChildCompPtr->setPin(pin);

            tempChildCompPtr->setMinAngleLimit0(minTwistAngle);
            tempChildCompPtr->setMaxAngleLimit0(maxTwistAngle);
            tempChildCompPtr->setMinAngleLimit0(minTwistAngle2);
            tempChildCompPtr->setMaxAngleLimit0(maxTwistAngle2);
            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponent::JT_KINEMATIC:
        {
            JointKinematicCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointKinematicComponent>(childRagBone->createJointComponent(JointKinematicComponent::getStaticClassName()));

            tempChildCompPtr->setMaxLinearAngleFriction(10.0f, 1000.0f);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        }

        if (nullptr != childJointCompPtr)
        {
            if (nullptr != parentRagBone)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                    "[PhysicsRagDollComponent]: Name: " + this->gameObjectPtr->getName() + " child, joint pos: " + Ogre::StringConverter::toString(childJointCompPtr->getBody()->getPosition()) +
                        " child, joint ori: " + Ogre::StringConverter::toString(childJointCompPtr->getBody()->getOrientation())

                        + " between child: " + childRagBone->getName() + " and parent: " + parentRagBone->getName() + ", joint pos: " + Ogre::StringConverter::toString(parentJointCompPtr->getBody()->getPosition()) +
                        ", joint ori: " + Ogre::StringConverter::toString(parentJointCompPtr->getBody()->getOrientation()) + ", joint pin: " + Ogre::StringConverter::toString(pin));
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent]: Name: " + this->gameObjectPtr->getName() + " child joint pos: " + Ogre::StringConverter::toString(childJointCompPtr->getJointPosition()) +
                                                                                       " between child: " + childRagBone->getName() + ", joint pin: " + Ogre::StringConverter::toString(pin));
            }
        }
    }

    void PhysicsRagDollComponent::inheritVelOmega(Ogre::Vector3 vel, Ogre::Vector3 omega)
    {
        // find main position.
        Ogre::Vector3 mainpos = this->gameObjectPtr->getSceneNode()->_getDerivedPosition();

        size_t offset = 0;
        // When partial ragdolling is active, the first bone is the main body and should not apply to rag doll!
        if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            offset = 1;
        }

        for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
        {
            Ogre::Vector3 pos;
            Ogre::Quaternion orient;

            it->ragBone->getBody()->getPositionOrientation(pos, orient);
            it->ragBone->getBody()->setVelocity(vel + omega.crossProduct(pos - mainpos));
        }
    }

    void PhysicsRagDollComponent::setState(const Ogre::String& state)
    {
        this->state->setListSelectedValue(state);

        this->rdOldState = this->rdState;

        if (state == "Inactive")
        {
            if (this->rdState == PhysicsRagDollComponent::INACTIVE)
            {
                return;
            }

            this->rdState = PhysicsRagDollComponent::INACTIVE;

            boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        }
        else if (state == "Animation")
        {
            if (this->rdState == PhysicsRagDollComponent::ANIMATION)
            {
                return;
            }
            this->rdState = PhysicsRagDollComponent::ANIMATION;
            boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        }
        else if (state == "Ragdolling")
        {
            if (nullptr == this->gameObjectPtr)
            {
                return;
            }

            if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
            {
                return;
            }
            this->rdState = PhysicsRagDollComponent::RAGDOLLING;
            this->animationEnabled = false;
            boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), true));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        }
        else if (state == "PartialRagdolling")
        {
            // Check if rotation bone name has changed, if this is the case partial ragdoll must be re-created
            if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING && this->oldPartialRagdollBoneName == this->partialRagdollBoneName)
            {
                return;
            }
            this->rdState = PhysicsRagDollComponent::PARTIAL_RAGDOLLING;

            boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        }

        if (true == this->isSimulating)
        {
            this->internalApplyState();
        }
    }

    Ogre::String PhysicsRagDollComponent::getState(void) const
    {
        return this->state->getListSelectedValue();
    }

    OgreNewt::Body* PhysicsRagDollComponent::getBody(void) const
    {
        return this->physicsBody;
    }

    const std::vector<PhysicsRagDollComponent::RagData>& PhysicsRagDollComponent::getRagDataList(void)
    {
        return this->ragDataList;
    }

    PhysicsRagDollComponent::RagBone* PhysicsRagDollComponent::getRagBone(const Ogre::String& name)
    {
        if (true == name.empty())
        {
            return nullptr;
        }
        for (auto& it = this->ragDataList.cbegin(); it != this->ragDataList.cend(); ++it)
        {
            if (name == it->ragBoneName)
            {
                return it->ragBone;
            }
        }
        return nullptr;
    }

    void PhysicsRagDollComponent::applyModelStateToRagdoll(void)
    {
        const auto node = this->gameObjectPtr->getSceneNode();
        Ogre::Vector3 nodePos = node->_getDerivedPosition();
        Ogre::Quaternion nodeOri = node->_getDerivedOrientation();
        Ogre::Vector3 nodeScale = node->_getDerivedScale();

        size_t i = 0;

        if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            i = 1;
        }

        for (; i < this->ragDataList.size(); i++)
        {
            auto& ragBone = this->ragDataList[i].ragBone;
            if (nullptr == ragBone || nullptr == ragBone->getBody() || nullptr == ragBone->getOgreBone())
            {
                continue;
            }

            // Compute bone's current world position/orientation from the skeleton animation.
            // _getDerivedPosition/Orientation gives skeleton-space; transform to world via entity node.
            Ogre::Vector3 boneSkelPos = ragBone->getOgreBone()->_getDerivedPosition();
            Ogre::Quaternion boneSkelOri = ragBone->getOgreBone()->_getDerivedOrientation();

            Ogre::Vector3 boneWorldPos = nodePos + nodeOri * (nodeScale * boneSkelPos);
            Ogre::Quaternion boneWorldOri = nodeOri * boneSkelOri;

            ragBone->getBody()->setPositionOrientation(boneWorldPos, boneWorldOri);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] applyModelStateToRagdoll BoneName: " + ragBone->getName() + " boneWorldPos: " + Ogre::StringConverter::toString(boneWorldPos) + " boneWorldOrientation: " + Ogre::StringConverter::toString(boneWorldOri));
        }

        // Set constraint axis for root body, after the final position of the bodies has been set
        this->setConstraintAxis(this->constraintAxis->getVector3());

        if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
        {
            // Release constraint axis pin, so that ragdoll can be rotated on any axis, but maybe is still clipped to plane
            this->releaseConstraintAxisPin();
        }
        else if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            // Pin the object stand in pose and not fall down
            this->setConstraintDirection(this->constraintDirection->getVector3());
        }
    }

    void PhysicsRagDollComponent::applyRagdollStateToModel(void)
    {
        if (true == this->ragDataList.empty())
        {
            return;
        }

        size_t i = 0;
        if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            i = 1;
        }

        const auto node = this->gameObjectPtr->getSceneNode();
        Ogre::Quaternion nodeOri = node->_getDerivedOrientation();
        Ogre::Vector3 nodePos = node->_getDerivedPosition();
        Ogre::Vector3 nodeScale = node->_getDerivedScale();
        Ogre::Quaternion invNodeOri = nodeOri.Inverse();

        // IMPORTANT: Process root-to-leaf order (ragDataList is in that order from XML parsing).
        // Each bone must call _update() so its children see the updated derived transform.
        for (; i < this->ragDataList.size(); i++)
        {
            auto& ragBone = this->ragDataList[i].ragBone;
            if (nullptr == ragBone || nullptr == ragBone->getBody() || nullptr == ragBone->getOgreBone())
            {
                continue;
            }

            Ogre::v1::OldBone* bone = ragBone->getOgreBone();

            // Get physics body's world transform
            Ogre::Vector3 bodyWorldPos = ragBone->getBody()->getPosition();
            Ogre::Quaternion bodyWorldOri = ragBone->getBody()->getOrientation();

            // Convert world -> skeleton space (entity-local)
            Ogre::Vector3 skelPos = invNodeOri * (bodyWorldPos - nodePos);
            skelPos /= nodeScale;
            Ogre::Quaternion skelOri = invNodeOri * bodyWorldOri;

            // With setInheritOrientation(false):
            //   _getDerivedOrientation() = bone->getOrientation() (directly what we set)
            //   So setting orientation to skeleton-space orientation is correct.
            //
            // For POSITION however, even with setInheritOrientation(false), Ogre still computes:
            //   _getDerivedPosition() = parent->_getDerivedOrientation() * (parent->_getDerivedScale() * bone->getPosition()) + parent->_getDerivedPosition()
            // So we must convert skeleton-space position to parent-bone-local space.

            Ogre::v1::OldBone* parentBone = static_cast<Ogre::v1::OldBone*>(bone->getParent());
            Ogre::Vector3 boneLocalPos;
            if (parentBone)
            {
                Ogre::Quaternion parentDerivedOri = parentBone->_getDerivedOrientation();
                Ogre::Vector3 parentDerivedPos = parentBone->_getDerivedPosition();
                Ogre::Vector3 parentDerivedScale = parentBone->_getDerivedScale();

                Ogre::Quaternion invParentOri = parentDerivedOri.Inverse();
                boneLocalPos = invParentOri * (skelPos - parentDerivedPos);
                boneLocalPos /= parentDerivedScale;
            }
            else
            {
                // Root bone - skeleton space IS local space
                boneLocalPos = skelPos;
            }

            // Update BOTH position and orientation
            // Note: orientation is skeleton-space because setInheritOrientation(false) makes derived = local
            // This runs on the render thread (inside tracked closure), so direct bone access is safe
            bone->setPosition(boneLocalPos);
            bone->setOrientation(skelOri);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[PhysicsRagDollComponent] applyRagdollStateToModel BoneName: " + ragBone->getName() + " boneLocalPos: " + Ogre::StringConverter::toString(boneLocalPos) + " skelOri: " + Ogre::StringConverter::toString(skelOri));

            // Force Ogre to recalculate derived transforms for this bone and children
            // This is CRITICAL - without this, child bones will use stale parent derived transforms
            bone->_update(true, false);
        }

        // Bone corrections remain the same
        for (auto& boneCorrectionData : this->boneCorrectionMap)
        {
            boneCorrectionData.first->setPosition(boneCorrectionData.second.first->_getDerivedPosition() + boneCorrectionData.second.second);
            boneCorrectionData.first->setOrientation(boneCorrectionData.second.first->_getDerivedOrientation());
        }
    }

    void PhysicsRagDollComponent::startRagdolling(void)
    {
        // this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->setUpdateBoundingBoxFromSkeleton(true);

        // ARRRGHHH this is it!!!! This causes in partial ragdoll, that the entity's bones have a bit offset between the bodies. But after this call, partial ragdoll still will
        // not work because no animation is working :(
        // https://forums.ogre3d.org/viewtopic.php?t=79731
        // this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->setSkipAnimationStateUpdate(true);

        // Reset all animations. Note this must be done, because a game object may be in the middle of an animation and this would mess up with the ragdolling
        Ogre::v1::AnimationStateSet* set = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->getAllAnimationStates();
        Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();

        while (it.hasMoreElements())
        {
            Ogre::v1::AnimationState* anim = it.getNext();
            anim->setEnabled(false);
            anim->setWeight(0);
            anim->setTimePosition(0);
        }

        this->applyModelStateToRagdoll();

        if (true == this->ragDataList.empty())
        {
            return;
        }

        if (nullptr == this->skeleton)
        {
            return;
        }

        Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();

        if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
        {
            if (false == this->ragDataList.empty())
            {
                this->ragDataList.front().ragBone->attachToNode();
            }

            // Set bones to manually controlled and get their initial orientations too
            for (size_t j = 1; j < this->ragDataList.size(); j++)
            {
                for (unsigned short i = 0; i < this->skeleton->getNumBones(); i++)
                {
                    Ogre::v1::OldBone* bone = this->skeleton->getBone(i);

                    // Only apply when a bone is part of the partial rag doll
                    if (bone == this->ragDataList[j].ragBone->getOgreBone())
                    {
                        // Get the absolute world orientation
                        Ogre::Quaternion absoluteWorldOrientation = bone->_getDerivedOrientation();

                        // Set inherit orientation to false
                        bone->setManuallyControlled(true);
                        bone->setInheritOrientation(false);
                        // Set the absolute world orientation
                        bone->setOrientation(absoluteWorldOrientation);

                        this->ragDataList[j].ragBone->attachToNode();
                        break;
                    }
                }
            }
        }
        else
        {
            // Full ragdoll: Set ALL bones in the skeleton to manually controlled
            // This prevents animation from overwriting our physics-driven transforms
            for (unsigned short idx = 0; idx < this->skeleton->getNumBones(); idx++)
            {
                Ogre::v1::OldBone* bone = this->skeleton->getBone(idx);
                Ogre::Quaternion absoluteWorldOrientation = bone->_getDerivedOrientation();
                bone->setManuallyControlled(true);
                bone->setInheritOrientation(false);
                bone->setOrientation(absoluteWorldOrientation);
            }

            // Attach ragdoll bones to their scene nodes
            for (size_t j = 0; j < this->ragDataList.size(); j++)
            {
                this->ragDataList[j].ragBone->attachToNode();
            }
        }

        // http://wiki.ogre3d.org/Ragdolls?highlight=ragdoll

        // Attention: https://forums.ogre3d.org/viewtopic.php?t=71135
    }

    void PhysicsRagDollComponent::endRagdolling(void)
    {
        if (nullptr == this->skeleton)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            while (this->ragDataList.size() > 0)
            {
                RagBone* ragBone = this->ragDataList.back().ragBone;
                delete ragBone;
                this->ragDataList.pop_back();
            }
            this->ragDataList.clear();

            if (nullptr != this->rootJointCompPtr)
            {
                this->rootJointCompPtr.reset();
            }

            boost::shared_ptr<TagPointComponent> tagPointCompPtr = nullptr;
            size_t i = 0;
            do
            {
                tagPointCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<TagPointComponent>(i++));
                if (nullptr != tagPointCompPtr)
                {
                    tagPointCompPtr->disconnect();
                }
            } while (nullptr != tagPointCompPtr);

            this->skeleton->unload();
            // this->skeleton->reload();
            this->skeleton->load();

            Ogre::v1::AnimationStateSet* set = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->getAllAnimationStates();
            Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();

            while (it.hasMoreElements())
            {
                Ogre::v1::AnimationState* anim = it.getNext();
                anim->setEnabled(false);
                anim->setWeight(0);
                anim->setTimePosition(0);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponent::endRagdolling");
    }

    void PhysicsRagDollComponent::deleteJointDelegate(EventDataPtr eventData)
    {
        // boost::shared_ptr<EventDataDeleteJoint> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteJoint>(eventData);

        //// Sometimes: If escape is pressed to often, a crash occurs because yet the body does exist, but this does also crash
        // for (size_t i = 0; i < this->ragDataList.size(); i++)
        //{
        //	if (this->ragDataList[i].ragBone->getJointId() == castEventData->getJointId())
        //	{
        //		this->ragDataList[i].ragBone->resetBody();
        //		break;
        //	}
        // }
    }

    /*********************************Inner bone class*****************************************************************/

    PhysicsRagDollComponent::RagBone::RagBone(const Ogre::String& name, PhysicsRagDollComponent* physicsRagDollComponent, RagBone* parentRagBone, Ogre::v1::OldBone* ogreBone, Ogre::v1::MeshPtr mesh, const Ogre::Vector3& pose,
        RagBone::BoneShape shape, Ogre::Vector3& size, Ogre::Real mass, bool partial, const Ogre::Vector3& offset, const Ogre::String& category) :
        name(name),
        physicsRagDollComponent(physicsRagDollComponent),
        parentRagBone(parentRagBone),
        ogreBone(ogreBone),
        body(nullptr),
        upVector(nullptr),
        pose(pose),
        bodySize(size),
        partial(partial),
        offset(offset),
        limit1(0.0f),
        limit2(0.0f),
        sceneNode(nullptr),
        initialBoneOrientation(Ogre::Quaternion::IDENTITY),
        initialBonePosition(Ogre::Vector3::ZERO)
    {
        this->requiredVelocityForForceCommand.vectorValue = Ogre::Vector3::ZERO;
        this->requiredVelocityForForceCommand.pending.store(false);
        this->requiredVelocityForForceCommand.inProgress.store(false);

        this->omegaForceCommand.vectorValue = Ogre::Vector3::ZERO;
        this->omegaForceCommand.pending.store(false);
        this->omegaForceCommand.inProgress.store(false);

        OgreNewt::CollisionPtr collisionPtr;
        Ogre::Vector3 inertia;
        Ogre::Vector3 massOrigin;

        if (true == partial && nullptr == parentRagBone)
        {
            this->initialBoneOrientation = this->physicsRagDollComponent->initialOrientation;
            // this->ogreBone->_setDerivedOrientation(this->initialBoneOrientation);
            this->initialBonePosition = this->physicsRagDollComponent->initialPosition;
        }
        else
        {
            // Tried several ways, but this remains the only way, rag doll bones are transformed correctly to physics collision hulls
            Ogre::Vector3 globalPos = ogreBone->_getDerivedPosition();
            Ogre::Quaternion globalOrient = ogreBone->_getDerivedOrientation();

            // Scaling must be applied, else if a huge scaled down character is used, a bone could be at position 4 60 -19
            globalPos *= this->physicsRagDollComponent->initialScale;
            globalPos = this->physicsRagDollComponent->initialOrientation * globalPos;

            globalOrient = this->physicsRagDollComponent->initialOrientation * globalOrient;
            // Set bone local position and add to world position of the character
            globalPos += this->physicsRagDollComponent->initialPosition;

            this->initialBoneOrientation = globalOrient;
            this->initialBonePosition = globalPos;

            // Compensate body position for ragdollPositionOffset.
            // The offset shifts the collision hull geometry INSIDE the body (e.g., UP by offset amount).
            // To keep the hull aligned with the actual mesh vertices in world space,
            // we shift the body in the OPPOSITE direction (DOWN by offset amount).
            // This also correctly positions the joint pivot at the hull meeting point
            // rather than at the bone's derived position (which may be at the wrong end
            // for bones with unusual orientations, like Boneup with 180° X flip).
            if (this->physicsRagDollComponent->ragdollPositionOffset != Ogre::Vector3::ZERO)
            {
                Ogre::Vector3 worldOffset = this->physicsRagDollComponent->initialOrientation
                    * (this->physicsRagDollComponent->initialScale * this->physicsRagDollComponent->ragdollPositionOffset);
                this->initialBonePosition -= worldOffset;
            }
        }

        // Bones can have really weird nearby zero values, so trunc those ugly values
        if (Ogre::Math::RealEqual(this->initialBonePosition.x, 0.0f, 0.001f))
        {
            this->initialBonePosition.x = 0.0f;
        }
        if (Ogre::Math::RealEqual(this->initialBonePosition.y, 0.0f, 0.001f))
        {
            this->initialBonePosition.y = 0.0f;
        }
        if (Ogre::Math::RealEqual(this->initialBonePosition.z, 0.0f, 0.001f))
        {
            this->initialBonePosition.z = 0.0f;
        }

        if (Ogre::Math::RealEqual(this->initialBoneOrientation.x, 0.0f, 0.001f))
        {
            this->initialBoneOrientation.x = 0.0f;
        }
        if (Ogre::Math::RealEqual(this->initialBoneOrientation.y, 0.0f, 0.001f))
        {
            this->initialBoneOrientation.y = 0.0f;
        }
        if (Ogre::Math::RealEqual(this->initialBoneOrientation.z, 0.0f, 0.001f))
        {
            this->initialBoneOrientation.z = 0.0f;
        }
        if (Ogre::Math::RealEqual(this->initialBoneOrientation.w, 0.0f, 0.001f))
        {
            this->initialBoneOrientation.w = 0.0f;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[PhysicsRagDollComponent] RagBone::RagBone Name: " + name + " initialBonePosition: " + Ogre::StringConverter::toString(this->initialBonePosition) + " initialBoneOrientation: " + Ogre::StringConverter::toString(this->initialBoneOrientation));

        // Set also from configured rag file position, orientation offset corrections for the collision hulls
        Ogre::Vector3 collisionPosition = this->physicsRagDollComponent->ragdollPositionOffset;
        Ogre::Quaternion collisionOrientation = this->physicsRagDollComponent->ragdollOrientationOffset;

        // Create the corresponding collision shape
        switch (shape)
        {
        case PhysicsRagDollComponent::RagBone::BS_BOX:
        {
            OgreNewt::CollisionPrimitives::Box* col = new OgreNewt::CollisionPrimitives::Box(this->physicsRagDollComponent->ogreNewt, size, this->physicsRagDollComponent->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);
            col->calculateInertialMatrix(inertia, massOrigin);

            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponent::RagBone::BS_CAPSULE:
        {
            OgreNewt::CollisionPrimitives::Capsule* col =
                new OgreNewt::CollisionPrimitives::Capsule(this->physicsRagDollComponent->ogreNewt, size.y, size.x, this->physicsRagDollComponent->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);

            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponent::RagBone::BS_CONE:
        {
            OgreNewt::CollisionPrimitives::Cone* col =
                new OgreNewt::CollisionPrimitives::Cone(this->physicsRagDollComponent->ogreNewt, size.y, size.x, this->physicsRagDollComponent->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);

            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponent::RagBone::BS_CYLINDER:
        {
            OgreNewt::CollisionPrimitives::Cylinder* col =
                new OgreNewt::CollisionPrimitives::Cylinder(this->physicsRagDollComponent->ogreNewt, size.y, size.x, this->physicsRagDollComponent->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);

            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponent::RagBone::BS_ELLIPSOID:
        {
            OgreNewt::CollisionPrimitives::Ellipsoid* col =
                new OgreNewt::CollisionPrimitives::Ellipsoid(this->physicsRagDollComponent->ogreNewt, size, this->physicsRagDollComponent->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);
            col->calculateInertialMatrix(inertia, massOrigin);

            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponent::RagBone::BS_CONVEXHULL:
        {
            if (nullptr != this->ogreBone)
            {
                collisionPtr = this->physicsRagDollComponent->getWeightedBoneConvexHull(this->ogreBone, mesh, size.x, inertia, massOrigin, this->physicsRagDollComponent->gameObjectPtr->getCategoryId(), collisionPosition, collisionOrientation,
                    this->physicsRagDollComponent->initialScale);
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[PhysicsRagDollComponent] Error: Cannot create a convex hull for partial ragdoll with no bone for game object: " + this->physicsRagDollComponent->getOwner()->getName());
                throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsRagDollComponent] Error: Cannot create a convex hull for partial ragdoll with no bone for game object: " + this->physicsRagDollComponent->getOwner()->getName() + "\n",
                    "NOWA");
            }
            break;
        }
        }

        this->body = new OgreNewt::Body(this->physicsRagDollComponent->ogreNewt, this->physicsRagDollComponent->gameObjectPtr->getSceneManager(), collisionPtr);
        NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->body);

        this->body->setGravity(this->physicsRagDollComponent->gravity->getVector3());

        // Set the body position and orientation to bone + an offset orientation specified in the XML file
        this->body->setPositionOrientation(this->initialBonePosition, this->initialBoneOrientation);

        /* if (nullptr != this->ogreBone)
         {
             Ogre::String ragBoneName = this->ogreBone->getName();
             Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] Creating rag bone: "
                + ragBoneName + " for game object: " + this->physicsRagDollComponent->getOwner()->getName() + " position: " + Ogre::StringConverter::toString(this->initialBonePosition));
         }
         else
         {
             Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] Creating rag bone root: "
                +  this->physicsRagDollComponent->getOwner()->getName()
                 + " for game object: " + this->physicsRagDollComponent->getOwner()->getName() + " position: " + Ogre::StringConverter::toString(this->initialBonePosition));
         }*/

        if (this->physicsRagDollComponent->linearDamping->getReal() != 0.0f)
        {
            this->body->setLinearDamping(this->physicsRagDollComponent->linearDamping->getReal());
        }

        if (this->physicsRagDollComponent->angularDamping->getVector3() != Ogre::Vector3::ZERO)
        {
            this->body->setAngularDamping(this->physicsRagDollComponent->angularDamping->getVector3());
        }

        this->body->setType(this->physicsRagDollComponent->gameObjectPtr->getCategoryId());

        inertia *= mass;

        if (Ogre::Math::RealEqual(massOrigin.x, 0.0f))
        {
            massOrigin.x = 0.0f;
        }
        if (Ogre::Math::RealEqual(massOrigin.y, 0.0f))
        {
            massOrigin.y = 0.0f;
        }
        if (Ogre::Math::RealEqual(massOrigin.z, 0.0f))
        {
            massOrigin.z = 0.0f;
        }

        this->body->setMassMatrix(mass, inertia);
        this->body->setCenterOfMass(massOrigin);

        // Important: Set the type and material group id for each piece for material pair functionality and collision callbacks
        unsigned int categoryId = this->physicsRagDollComponent->getOwner()->getCategoryId();
        if (false == category.empty())
        {
            categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->registerCategory(category);

            const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialIDFromCategory(category, this->physicsRagDollComponent->ogreNewt);
            this->body->setMaterialGroupID(materialId);
        }
        else
        {
            const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->physicsRagDollComponent->getOwner().get(), this->physicsRagDollComponent->ogreNewt);
            this->body->setMaterialGroupID(materialId);
        }

        this->body->setType(categoryId);
        this->body->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this->physicsRagDollComponent)));

        if (nullptr != this->parentRagBone)
        // if (false == this->partial)
        {
            this->body->setCustomForceAndTorqueCallback<PhysicsRagDollComponent::RagBone>(&PhysicsRagDollComponent::RagBone::moveCallback, this);
        }

        // pin the object stand in pose and not fall down
        /*if (this->physicsRagDollComponent->pose != Ogre::Vector3::ZERO) {
            this->physicsRagDollComponent->applyPose(this->physicsRagDollComponent->pose);
            }*/

        /*if (this->pose != Ogre::Vector3::ZERO) {
            this->physicsRagDollComponent->applyPose(this->pose);
            }*/

        if (false == this->physicsRagDollComponent->activated->getBool())
        {
            this->body->freeze();
        }
        else
        {
            this->body->unFreeze();
        }

        this->body->setCollidable(this->physicsRagDollComponent->collidable->getBool());

        // if this is the parent just assign the root node
        if (nullptr == this->parentRagBone)
        {
            this->sceneNode = this->physicsRagDollComponent->gameObjectPtr->getSceneNode();
            this->physicsRagDollComponent->physicsBody = this->body;

            this->physicsRagDollComponent->setPosition(this->initialBonePosition);
            this->physicsRagDollComponent->setOrientation(this->initialBoneOrientation);
        }
        else
        {
            // else create a child scene node
            this->sceneNode = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->createChildSceneNode();
            this->sceneNode->setName(name);
            this->sceneNode->getUserObjectBindings().setUserAny(Ogre::Any(this->physicsRagDollComponent->gameObjectPtr.get()));
        }

        if (nullptr == this->parentRagBone /* && true == this->partial*/)
        {
            this->physicsRagDollComponent->setGyroscopicTorqueEnabled(this->physicsRagDollComponent->gyroscopicTorque->getBool());

            this->body->setCustomForceAndTorqueCallback<PhysicsRagDollComponent>(&PhysicsActiveComponent::moveCallback, this->physicsRagDollComponent);
        }
    }

    PhysicsRagDollComponent::RagBone::~RagBone()
    {
        this->deleteRagBone();
    }

    void PhysicsRagDollComponent::RagBone::applyOmegaForceRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength)
    {
        if (nullptr == this->body)
        {
            return;
        }

        Ogre::Quaternion diffOrientation = this->body->getOrientation().Inverse() * resultOrientation;
        Ogre::Vector3 resultVector = Ogre::Vector3::ZERO;

        if (axes.x == 1.0f)
        {
            resultVector.x = diffOrientation.getPitch().valueRadians() * strength;
        }
        if (axes.y == 1.0f)
        {
            resultVector.y = diffOrientation.getYaw().valueRadians() * strength;
        }
        if (axes.z == 1.0f)
        {
            resultVector.z = diffOrientation.getRoll().valueRadians() * strength;
        }

        this->applyOmegaForce(resultVector);
    }

    void PhysicsRagDollComponent::RagBone::applyOmegaForceRotateToDirection(const Ogre::Vector3& resultDirection, Ogre::Real strength)
    {
    }

    unsigned long PhysicsRagDollComponent::RagBone::getJointId(void)
    {
        if (nullptr != this->jointCompPtr)
        {
            return this->jointCompPtr->getId();
        }
        return 0;
    }

    void PhysicsRagDollComponent::RagBone::moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
    {
        Ogre::Real nearestPlanetDistance = std::numeric_limits<Ogre::Real>::max();
        GameObjectPtr nearestGravitySourceObject;

        Ogre::Vector3 wholeForce = body->getGravity();
        Ogre::Real mass = 0.0f;
        Ogre::Vector3 inertia = Ogre::Vector3::ZERO;

        // calculate gravity
        body->getMassMatrix(mass, inertia);
        wholeForce *= mass;

        if (false == this->physicsRagDollComponent->gravitySourceCategory->getString().empty())
        {
            // If there is a gravity source GO, calculate gravity in that direction of the source, but only work with the nearest object, else the force will mess up
            auto gravitySourceGameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(this->physicsRagDollComponent->gravitySourceCategory->getString());
            for (size_t i = 0; i < gravitySourceGameObjects.size(); i++)
            {
                wholeForce = this->getPosition() - gravitySourceGameObjects[i]->getPosition();
                Ogre::Real squaredDistanceToGravitySource = wholeForce.squaredLength();
                if (squaredDistanceToGravitySource < nearestPlanetDistance)
                {
                    nearestPlanetDistance = squaredDistanceToGravitySource;
                    nearestGravitySourceObject = gravitySourceGameObjects[i];
                }
            }

            // Only do calculation for the nearest planet
            if (nullptr != nearestGravitySourceObject)
            {
                auto& gravitySourcePhysicsComponentPtr = NOWA::makeStrongPtr(nearestGravitySourceObject->getComponent<PhysicsComponent>());
                if (nullptr != gravitySourcePhysicsComponentPtr)
                {
                    Ogre::Vector3 directionToPlanet = this->getPosition() - gravitySourcePhysicsComponentPtr->getPosition();
                    directionToPlanet.normalise();

                    // Ensures constant acceleration of e.g. -19.8 m/s²
                    Ogre::Real gravityAcceleration = -this->physicsRagDollComponent->gravity->getVector3().length(); // Should be e.g. 19.8

                    wholeForce = directionToPlanet * (mass * gravityAcceleration); // F = m * a

                    // More realistic planetary scenario: Depending on planet size, gravity is much harder or slower like on the moon:
                    // Ogre::Real squaredDistanceToGravitySource = wholeForce.squaredLength();
                    // Ogre::Real strength = (gravityAcceleration * gravitySourcePhysicsComponentPtr->getMass()) / squaredDistanceToGravitySource;
                    // wholeForce = directionToPlanet * (mass * strength);
                }
            }
        }

        body->addForce(wholeForce);

        // Checks if a required force for velocity command is pending
        if (this->requiredVelocityForForceCommand.pending.load())
        {
            // Try to claim this command
            bool expected = false;
            if (this->requiredVelocityForForceCommand.inProgress.compare_exchange_strong(expected, true))
            {
                // We've claimed the command

                // Get the velocity to apply
                Ogre::Vector3 velocityToApply = this->requiredVelocityForForceCommand.vectorValue;

                // Calculate and apply the force
                Ogre::Vector3 moveForce = (velocityToApply - body->getVelocity()) * mass / timeStep;
                body->addForce(moveForce);

                // Mark command as no longer pending
                this->requiredVelocityForForceCommand.pending.store(false);

                // Release the command
                this->requiredVelocityForForceCommand.inProgress.store(false);
            }
        }

        // Checks if an omage force command is pending
        if (this->omegaForceCommand.pending.load())
        {
            // Try to claim this command
            bool expected = false;
            if (this->omegaForceCommand.inProgress.compare_exchange_strong(expected, true))
            {
                // Get the omega force to apply
                Ogre::Vector3 omegaForceToApply = this->omegaForceCommand.vectorValue;

                body->setBodyAngularVelocity(omegaForceToApply, timeStep);

                // Mark command as no longer pending
                this->omegaForceCommand.pending.store(false);

                // Release the command
                this->omegaForceCommand.inProgress.store(false);
            }
        }
    }

    void PhysicsRagDollComponent::RagBone::deleteRagBone(void)
    {
        // Dangerous: It does mess up position, orientation of bone when stopping/restarting simulation! Hence un-commented!
        // this->ogreBone->setInheritOrientation(false);
        // this->ogreBone->setManuallyControlled(false);

        if (nullptr != this->ogreBone)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent::RagBone] Deleting ragbone: " + this->ogreBone->getName());

            NOWA::GraphicsModule::getInstance()->removeTrackedOldBone(this->ogreBone);
        }
        if (nullptr != this->body)
        {
            if (this->jointCompPtr)
            {
                AppStateManager::getSingletonPtr()->getGameObjectController()->removeJointComponent(this->jointCompPtr->getId());
                jointCompPtr->releaseJoint(false);
                jointCompPtr.reset();
            }

            // Do not delete the root body, because it will be deleted automatically when this component will be deleted
            if (this->body != this->physicsRagDollComponent->physicsBody && nullptr != this->parentRagBone)
            {
                this->body->detachNode();
                this->body->removeForceAndTorqueCallback();
                this->body->removeNodeUpdateNotify();
                this->body->removeDestructorCallback();
                delete this->body;
                this->body = nullptr;
            }
            else
            {
                // if its the physics body of the component, just set this one to null
                this->body = nullptr;
            }
        }
        if (nullptr != this->sceneNode)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->sceneNode);
            if (nullptr != this->parentRagBone)
            {
                auto sceneNode = this->physicsRagDollComponent->gameObjectPtr->getSceneNode();
                auto thisSceneNode = this->sceneNode;
                sceneNode->removeAndDestroyChild(thisSceneNode);
            }
            else
            {
                this->sceneNode = nullptr;
            }
        }
    }

    void PhysicsRagDollComponent::RagBone::resetBody(void)
    {
        this->body = nullptr;
    }

    void PhysicsRagDollComponent::RagBone::update(Ogre::Real dt, bool notSimulating)
    {
        if (nullptr != this->jointCompPtr && false == notSimulating)
        {
            this->jointCompPtr->update(dt);
        }

        //// if animation is not enabled, control the bones depending on the physics hulls
        // if (!this->physicsRagDollComponent->animationEnabled)
        //{

        //	Ogre::Quaternion localOrient;
        //	Ogre::Vector3 localPos;
        //	this->body->getPositionOrientation(localPos, localOrient);
        //	// For example: wrist->forearm->arm_l->neck, that means ordering is important, parent of wrist is forearm_l, parent of rorearm_l is arm_l
        //	// if this is the root bone, get the offset offset position and orientation to the model nodes position and orientation that had been determined at line 360
        //	if (!this->getParentRagBone())
        //	{

        //		Ogre::Quaternion finalorient = (localOrient * this->offsetOrient);
        //		Ogre::Vector3 finalpos = localPos + (localOrient * this->offsetPos);
        //		// the whole ragdoll will be physically translated and rotated
        //		this->body->setPositionOrientation(finalpos, finalorient);

        //	}
        //	else
        //	{ // if this is the root orientate the node
        //		// standard bone, calculate the local orientation between it and it's parent.
        //		Ogre::Quaternion parentOrientation;
        //		Ogre::Vector3 parentPosition;

        //		this->getParentRagBone()->getBody()->getPositionOrientation(parentPosition, parentOrientation);

        //		Ogre::Quaternion localorient = parentOrientation.Inverse() * localOrient;
        //		// only the bone will be physicaly rotated
        //		this->ogreBone->setOrientation(localorient);
        //	}
        //	// do the opposite, controll the physics hulls depending on the bones
        //}
        // else
        //{
        //	/*Ogre::Quaternion localOrient = this->ogreBone->_getDerivedOrientation();
        //	Ogre::Vector3 localPos = this->ogreBone->_getDerivedPosition();

        //	if (this->parentRagBone != NULL) {

        //	this->body->setPositionOrientation(localPos, localOrient);

        //	} else { // if this is the root orientate the node

        //	Ogre::Quaternion finalOrient = (localOrient * this->offsetOrient);
        //	Ogre::Vector3 finalPos = localPos + (localOrient * this->offsetPos);

        //	this->physicsRagDollComponent->physicsBody->setPositionOrientation(finalPos, finalOrient);
        //	}*/

        //	Ogre::Quaternion worldOrient;
        //	Ogre::Vector3 worldPos;
        //	this->body->getPositionOrientation(worldPos, worldOrient);
        //	Ogre::Quaternion localOrient = this->ogreBone->getOrientation();
        //	Ogre::Vector3 localPos = this->ogreBone->getPosition();
        //	// Ogre::LogManager::getSingletonPtr()->logMessage("localpos: " + Ogre::StringConverter::toString(localPos) + " for bone: " + this->ogreBone->getName());

        //	Ogre::Vector3 center = this->physicsRagDollComponent->bones.cbegin()->bone->getBody()->getPosition();
        //	Ogre::Vector3 offsetPos = (worldPos - center);

        //	if (this->parentRagBone != nullptr)
        //	{

        //		this->body->setPositionOrientation(this->ogreBone->_getDerivedPosition(), this->ogreBone->_getDerivedOrientation());

        //	}
        //	else
        //	{ // if this is the root orientate the node

        //		Ogre::Quaternion finalOrient = (localOrient * this->offsetOrient);
        //		Ogre::Vector3 finalPos = localPos + (localOrient * this->offsetPos);

        //		this->physicsRagDollComponent->physicsBody->setPositionOrientation(worldPos + localPos, localOrient);
        //	}

        //}
    }

    const Ogre::String& PhysicsRagDollComponent::RagBone::getName(void) const
    {
        return this->name;
    }

    void PhysicsRagDollComponent::RagBone::setOrientation(const Ogre::Quaternion& orientation)
    {
        // Ogre::Vector3 center = this->physicsRagDollComponent->gameObjectPtr->getEntity()->getWorldBoundingBox().getCenter();
        Ogre::Vector3 center = this->physicsRagDollComponent->ragDataList.cbegin()->ragBone->getBody()->getPosition();

        // Very important for orientation: When the bone gets created and the collision hull, calculate once the
        // initial relative position from the current bone to the root bone (e.g. PELVIS) and set the initial orientation
        // to the orientation of the bone, when the ragdoll is fresh created. This values never changes
        // after that apply the orientation always from the initial relative position and intial orientation
        this->body->setPositionOrientation(center + orientation * this->initialBonePosition, orientation * this->initialBoneOrientation);
    }

    void PhysicsRagDollComponent::RagBone::rotate(const Ogre::Quaternion& rotation)
    {
        // hier orientieren D:\Ogre\GameEngineDevelopment\external\MeshSplitterDemos\OgrePhysX_04\source\OgrePhysXRagdoll.cpp
        Ogre::Quaternion localOrient;
        Ogre::Vector3 localPos;
        this->body->getPositionOrientation(localPos, localOrient);

        Ogre::Vector3 center = this->physicsRagDollComponent->ragDataList.cbegin()->ragBone->getBody()->getPosition();
        // for relative rotation always calculate the offset pos to the center, depending from the current local pos and orientation
        Ogre::Vector3 offsetPos = (localPos - center);

        // rotate around the center, take the local orientation into account
        this->body->setPositionOrientation(center + rotation * offsetPos, rotation * localOrient);
    }

    void PhysicsRagDollComponent::RagBone::setInitialState(void)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            Ogre::Vector3 center = this->physicsRagDollComponent->ragDataList.cbegin()->ragBone->getBody()->getPosition();
            if (nullptr != this->body)
            {
                this->body->setPositionOrientation(center + this->initialBonePosition, this->initialBoneOrientation);
            }

            this->ogreBone->setPosition(this->initialBonePosition);
            this->ogreBone->setOrientation(this->initialBoneOrientation);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponent::RagBone::setInitialState");
    }

    OgreNewt::Body* PhysicsRagDollComponent::RagBone::getBody(void)
    {
        return this->body;
    }

    Ogre::v1::OldBone* PhysicsRagDollComponent::RagBone::getOgreBone(void) const
    {
        return this->ogreBone;
    }

    PhysicsRagDollComponent::RagBone* PhysicsRagDollComponent::RagBone::getParentRagBone(void) const
    {
        return this->parentRagBone;
    }

    Ogre::Quaternion PhysicsRagDollComponent::RagBone::getInitialBoneOrientation(void)
    {
        return this->initialBoneOrientation;
    }

    Ogre::Vector3 PhysicsRagDollComponent::RagBone::getInitialBonePosition(void)
    {
        return this->initialBonePosition;
    }

    Ogre::Vector3 PhysicsRagDollComponent::RagBone::getWorldPosition(void)
    {
        // Ogre::Vector3 worldPosition = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->convertLocalToWorldPosition(this->ogreBone->_getDerivedPosition());
        // return worldPosition;
        return this->ogreBone->_getDerivedPosition();
    }

    Ogre::Quaternion PhysicsRagDollComponent::RagBone::getWorldOrientation(void)
    {
        // Ogre::Quaternion worldOrientation = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->convertLocalToWorldOrientation(this->ogreBone->_getDerivedOrientation());
        // return worldOrientation;
        return this->ogreBone->_getDerivedOrientation();
    }

    Ogre::Vector3 PhysicsRagDollComponent::RagBone::getPosition(void) const
    {
        if (nullptr != this->body)
        {
            return this->body->getPosition();
        }
        else
        {
            return Ogre::Vector3::ZERO;
        }
    }

    Ogre::Quaternion PhysicsRagDollComponent::RagBone::getOrientation(void) const
    {
        if (nullptr != this->body)
        {
            return this->body->getOrientation();
        }
        else
        {
            return Ogre::Quaternion::IDENTITY;
        }
    }

    Ogre::Vector3 PhysicsRagDollComponent::RagBone::getOffset(void) const
    {
        return this->offset;
    }

    Ogre::SceneNode* PhysicsRagDollComponent::RagBone::getSceneNode(void) const
    {
        return this->sceneNode;
    }

    void PhysicsRagDollComponent::RagBone::attachToNode(void)
    {
        this->body->attachNode(this->sceneNode);
    }

    void PhysicsRagDollComponent::RagBone::detachFromNode(void)
    {
        this->body->detachNode();
    }

    PhysicsRagDollComponent* PhysicsRagDollComponent::RagBone::getPhysicsRagDollComponent(void) const
    {
        return this->physicsRagDollComponent;
    }

    const Ogre::Vector3 PhysicsRagDollComponent::RagBone::getRagPose(void)
    {
        return this->pose;
    }

    void PhysicsRagDollComponent::RagBone::applyPose(const Ogre::Vector3& pose)
    {
        this->pose = pose;
        /*if (this->pose != Ogre::Vector3::ZERO) {
            this->physicsRagDollComponent->applyPose(this->pose);
            }*/

        if (this->pose != Ogre::Vector3::ZERO)
        {
            if (!this->upVector)
            {
                this->upVector = new OgreNewt::UpVector(this->body, pose);
            }
            else
            {
                this->upVector->setPin(this->body, pose);
            }
        }
        else
        {
            if (this->upVector)
            {
                delete this->upVector;
                this->upVector = nullptr;
            }
        }
    }

    JointCompPtr PhysicsRagDollComponent::RagBone::createJointComponent(const Ogre::String& type)
    {
        if (JointHingeComponent::getStaticClassName() == type)
        {
            this->jointCompPtr = boost::make_shared<JointHingeComponent>();
        }
        else if (JointBallAndSocketComponent::getStaticClassName() == type)
        {
            this->jointCompPtr = boost::make_shared<JointBallAndSocketComponent>();
        }
        else if (JointUniversalComponent::getStaticClassName() == type)
        {
            this->jointCompPtr = boost::make_shared<JointUniversalComponent>();
        }
        else if (JointHingeActuatorComponent::getStaticClassName() == type)
        {
            this->jointCompPtr = boost::make_shared<JointHingeActuatorComponent>();
        }
        else if (JointUniversalActuatorComponent::getStaticClassName() == type)
        {
            this->jointCompPtr = boost::make_shared<JointUniversalActuatorComponent>();
        }
        else if (JointKinematicComponent::getStaticClassName() == type)
        {
            this->jointCompPtr2 = boost::make_shared<JointKinematicComponent>();
            this->jointCompPtr2->setType(type);
            // Set scene manager for debug data
            this->jointCompPtr2->setSceneManager(this->physicsRagDollComponent->getOwner()->getSceneManager());
            this->jointCompPtr2->setOwner(this->physicsRagDollComponent->getOwner());
            return this->jointCompPtr2;
        }
        else
        {
            this->jointCompPtr = boost::make_shared<JointComponent>();
        }
        this->jointCompPtr->setType(type);
        // Set scene manager for debug data
        this->jointCompPtr->setSceneManager(this->physicsRagDollComponent->getOwner()->getSceneManager());
        // Attention: Is this correct?
        this->jointCompPtr->setOwner(this->physicsRagDollComponent->getOwner());
        return this->jointCompPtr;
    }

    JointCompPtr PhysicsRagDollComponent::RagBone::getJointComponent(void) const
    {
        return this->jointCompPtr;
    }

    JointCompPtr PhysicsRagDollComponent::RagBone::getSecondJointComponent(void) const
    {
        return this->jointCompPtr2;
    }

    Ogre::Vector3 PhysicsRagDollComponent::RagBone::getBodySize(void) const
    {
        return this->bodySize;
    }

    void PhysicsRagDollComponent::RagBone::applyRequiredForceForVelocity(const Ogre::Vector3& velocity)
    {
        if (velocity != Ogre::Vector3::ZERO)
        {
            // Set the command values
            this->requiredVelocityForForceCommand.vectorValue = velocity;

            // Mark as pending - this will be seen by the physics thread
            this->requiredVelocityForForceCommand.pending.store(true);
        }
    }

    void PhysicsRagDollComponent::RagBone::applyOmegaForce(const Ogre::Vector3& omegaForce)
    {
        if (omegaForce != Ogre::Vector3::ZERO)
        {
            // Set the command values
            this->omegaForceCommand.vectorValue = omegaForce;

            // Mark as pending - this will be seen by the physics thread
            this->omegaForceCommand.pending.store(true);
        }
    }

}; // namespace end