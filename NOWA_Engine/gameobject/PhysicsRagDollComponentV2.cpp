#include "NOWAPrecompiled.h"
#include "PhysicsRagDollComponentV2.h"
#include "JointComponents.h"
#include "TagPointComponent.h"
#include "main/AppStateManager.h"
#include "utilities/AnimationBlender.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "Animation/OgreBone.h"
#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVertexArrayObject.h"

#include <unordered_map>

#include "modules/LuaScriptApi.h"

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

    // ============================================================================
    // Helper: extract skeleton-local position and orientation from a V2 Bone.
    // _getLocalSpaceTransform() returns the accumulated bone transform in skeleton
    // space (all parent bones applied, but NOT the parent SceneNode). This is the
    // direct equivalent of V1's OldBone::_getDerivedPosition/Orientation().
    // ============================================================================

    void PhysicsRagDollComponentV2::extractBoneDerivedTransform(Ogre::Bone* bone, Ogre::Vector3& outPos, Ogre::Quaternion& outOrient)
    {
        const Ogre::SimpleMatrixAf4x3& t = bone->_getLocalSpaceTransform();

        Ogre::Matrix4 mat4;
        t.store(&mat4);

        Ogre::Vector3 scale;
        mat4.decomposition(outPos, scale, outOrient);
    }

    // ============================================================================
    // Constructor / Destructor
    // ============================================================================

    PhysicsRagDollComponentV2::PhysicsRagDollComponentV2() :
        PhysicsActiveComponent(),
        rdState(PhysicsRagDollComponentV2::INACTIVE),
        rdOldState(PhysicsRagDollComponentV2::INACTIVE),
        skeletonInstance(nullptr),
        animationEnabled(true),
        ragdollPositionOffset(Ogre::Vector3::ZERO),
        ragdollOrientationOffset(Ogre::Quaternion::IDENTITY),
        activated(new Variant(PhysicsRagDollComponentV2::AttrActivated(), true, this->attributes)),
        boneConfigFile(new Variant(PhysicsRagDollComponentV2::AttrBonesConfigFile(), Ogre::String(), this->attributes)),
        state(new Variant(PhysicsRagDollComponentV2::AttrState(), {"Inactive", "Animation", "Ragdolling", "PartialRagdolling"}, this->attributes)),
        isSimulating(false),
        oldAnimationId(-1)
    {
        this->boneConfigFile->setDescription("Name of the XML config file, no path, e.g. FlubberRagdoll.rag");
        this->boneConfigFile->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->gyroscopicTorque->setValue(false);
        this->gyroscopicTorque->setVisible(false);

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponentV2::gameObjectAnimationChangedDelegate), EventDataAnimationChanged::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponentV2::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());
    }

    PhysicsRagDollComponentV2::~PhysicsRagDollComponentV2()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponentV2] Destructor physics rag doll component for game object: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponentV2::gameObjectAnimationChangedDelegate), EventDataAnimationChanged::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponentV2::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());

        while (this->ragDataList.size() > 0)
        {
            RagBone* ragBone = this->ragDataList.back().ragBone;
            delete ragBone;
            this->ragDataList.pop_back();
        }
    }

    // ============================================================================
    // init
    // ============================================================================

    bool PhysicsRagDollComponentV2::init(rapidxml::xml_node<>*& propertyElement)
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
                this->rdState = PhysicsRagDollComponentV2::INACTIVE;
            }
            else if (stateCaption == "Animation")
            {
                this->rdState = PhysicsRagDollComponentV2::ANIMATION;
            }
            else if (stateCaption == "Ragdolling")
            {
                this->rdState = PhysicsRagDollComponentV2::RAGDOLLING;
                this->animationEnabled = false;
            }
            else if (stateCaption == "PartialRagdolling")
            {
                this->rdState = PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING;
            }
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    // ============================================================================
    // clone
    // ============================================================================

    GameObjectCompPtr PhysicsRagDollComponentV2::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PhysicsRagDollComponentV2Ptr clonedCompPtr(boost::make_shared<PhysicsRagDollComponentV2>());

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
        clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
        clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setBoneConfigFile(this->boneConfigFile->getString());
        clonedCompPtr->setState(this->state->getListSelectedValue());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    // ============================================================================
    // postInit
    // ============================================================================

    bool PhysicsRagDollComponentV2::postInit(void)
    {
        bool success = PhysicsComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponentV2] Post init physics rag doll component for game object: " + this->gameObjectPtr->getName());

        // Physics active component must be dynamic, else a mess occurs
        this->gameObjectPtr->setDynamic(true);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

        return success;
    }

    // ============================================================================
    // onRemoveComponent
    // ============================================================================

    void PhysicsRagDollComponentV2::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        this->rdState = PhysicsRagDollComponentV2::INACTIVE;
        boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        this->activated->setValue(false);
    }

    // ============================================================================
    // createRagDoll
    // ============================================================================

    bool PhysicsRagDollComponentV2::createRagDoll(const Ogre::String& boneName)
    {
		// ALWAYS capture the current scene node transform.
		// V2 meshes typically use non-unit scale (e.g., 0.01 for cm→m conversion).
            this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
            this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
            this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        if (true == this->boneConfigFile->getString().empty())
            return true;

        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->boneConfigFile->getString());
        if (stream.isNull())
        {
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Could not open bone configuration file: "
				+ this->boneConfigFile->getString() + " for game object: " + this->gameObjectPtr->getName());
            return false;
        }

        rapidxml::xml_document<> doc;
        Ogre::String data = stream->getAsString();
        std::vector<char> xmlBuffer(data.begin(), data.end());
        xmlBuffer.push_back('\0');
        try
        {
            doc.parse<0>(&xmlBuffer[0]);
        }
        catch (const rapidxml::parse_error&)
        {
            stream->close();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Could not parse configuration file: " + this->boneConfigFile->getString()
				+ " for game object: " + this->gameObjectPtr->getName());
            return false;
        }
        stream->close();
        rapidxml::xml_node<>* root = doc.first_node();

        if (!root)
        {
            Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponentV2] Error: cannot find 'root' in xml file: " + this->boneConfigFile->getString());
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

        // Get the Item (V2)
        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr == item)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error cannot create rag doll body, because the game object has no Item with mesh for game object: " + this->gameObjectPtr->getName());
            return false;
        }

        if (!item->hasSkeleton())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error: Item has no skeleton for game object: " + this->gameObjectPtr->getName());
            return false;
        }

        this->skeletonInstance = item->getSkeletonInstance();

        // Apply default pose if specified
        if (defaultPose.length() > 0 && nullptr != this->skeletonInstance)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, defaultPose, item]()
            {
                Ogre::IdString poseIdString(defaultPose);
                if (this->skeletonInstance->hasAnimation(poseIdString))
                {
                    Ogre::SkeletonAnimation* anim = this->skeletonInstance->getAnimation(poseIdString);
                    anim->setEnabled(true);
                    anim->setLoop(false);
                    anim->setTime(100.0f);
                    this->skeletonInstance->update();
                    anim->setEnabled(false);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV2::createRagDoll_pose");
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

        bool success = false;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, &success, root, boneName]()
        {
            // Parse rag doll data from xml (must run on render thread for bone access)
            success = this->createRagDollFromXML(root, boneName);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV2::createRagDollFromXML");

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

    // ============================================================================
    // connect
    // ============================================================================

    bool PhysicsRagDollComponentV2::connect(void)
    {
        bool success = PhysicsActiveComponent::connect();

        this->isSimulating = true;

        this->internalApplyState();

        return success;
    }

    // ============================================================================
    // disconnect
    // ============================================================================

    bool PhysicsRagDollComponentV2::disconnect(void)
    {
        bool success = PhysicsActiveComponent::disconnect();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->isSimulating = false;
        this->internalShowDebugData(false);

        if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING || this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
        {
            this->endRagdolling();
        }

        this->rdOldState = PhysicsRagDollComponentV2::INACTIVE;

        return success;
    }

    // ============================================================================
    // getClassName / getParentClassName / getParentParentClassName
    // ============================================================================

    Ogre::String PhysicsRagDollComponentV2::getClassName(void) const
    {
        return "PhysicsRagDollComponentV2";
    }

    Ogre::String PhysicsRagDollComponentV2::getParentClassName(void) const
    {
        return "PhysicsActiveComponent";
    }

    Ogre::String PhysicsRagDollComponentV2::getParentParentClassName(void) const
    {
        return "PhysicsComponent";
    }

    // ============================================================================
    // gameObjectAnimationChangedDelegate
    // ============================================================================

    void PhysicsRagDollComponentV2::gameObjectAnimationChangedDelegate(EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataAnimationChanged> castEventData = boost::static_pointer_cast<NOWA::EventDataAnimationChanged>(eventData);
        unsigned long id = castEventData->getGameObjectId();

        if (this->gameObjectPtr->getId() == id)
        {
            if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING && AnimationBlender::ANIM_JUMP_START == castEventData->getAnimationId() && this->oldAnimationId != castEventData->getAnimationId())
            {
                // Could trigger state changes on specific animations
            }

            this->oldAnimationId = castEventData->getAnimationId();
        }
    }

    // ============================================================================
    // setActivated
    // ============================================================================

    void PhysicsRagDollComponentV2::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING || this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
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

    // ============================================================================
    // update
    // ============================================================================

    void PhysicsRagDollComponentV2::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == activated->getBool() && false == notSimulating)
        {
            if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING || this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
            {
                // V2: Update skeleton on render thread. Manual bones are left untouched,
                // non-manual bones get their animation transforms.
                // IMPORTANT: applyRagdollStateToModel must run INSIDE the render closure
                // (same as V1) so bone transforms can be directly set and parent derived
                // transforms are available for child bone conversion.
                if (nullptr != this->skeletonInstance)
                {
                    auto closureFunction = [this](Ogre::Real renderDt)
                    {
                        this->skeletonInstance->update();
                        // Note: Order here is important! applyRagdollStateToModel must run
                        // after skeleton update, on the render thread.
                        this->applyRagdollStateToModel();
                    };
                    Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
                    NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
                }
            }

            if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING || this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
            {
                for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
                {
                    (*it).ragBone->update(dt, notSimulating);
                }
            }
        }
    }

    // ============================================================================
    // actualizeValue
    // ============================================================================

    void PhysicsRagDollComponentV2::actualizeValue(Variant* attribute)
    {
        PhysicsActiveComponent::actualizeCommonValue(attribute);

        if (PhysicsRagDollComponentV2::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (PhysicsRagDollComponentV2::AttrBonesConfigFile() == attribute->getName())
        {
            this->setBoneConfigFile(attribute->getString());
        }
        else if (PhysicsRagDollComponentV2::AttrState() == attribute->getName())
        {
            this->setState(attribute->getListSelectedValue());
        }
    }

    // ============================================================================
    // writeXML
    // ============================================================================

    void PhysicsRagDollComponentV2::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
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

    // ============================================================================
    // showDebugData / internalShowDebugData
    // ============================================================================

    void PhysicsRagDollComponentV2::showDebugData(void)
    {
        GameObjectComponent::showDebugData();
    }

    void PhysicsRagDollComponentV2::internalShowDebugData(bool activate)
    {
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
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV2::internalShowDebugData");
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
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV2::showDebugData");
            }
        }
    }

    // ============================================================================
    // internalApplyState
    // ============================================================================

    void PhysicsRagDollComponentV2::internalApplyState(void)
    {
        if (true == this->isSimulating)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                // First deactivate debug data if set
                this->internalShowDebugData(false);

                if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
                {
                    boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), true));
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);

                    if (this->rdOldState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
                    {
                        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
                        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
                        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
                    }

                    this->partialRagdollBoneName = "";
                    this->releaseConstraintDirection();
                    this->releaseConstraintAxis();
                    this->destroyBody();
                    this->createRagDoll(this->partialRagdollBoneName);
                    this->startRagdolling();
                }
                else if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
                {
                    boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);

                    this->endRagdolling();

                    this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
                    if (this->rdOldState == PhysicsRagDollComponentV2::RAGDOLLING)
                    {
                        this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
                    }
                    this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
                    if (this->rdOldState != PhysicsRagDollComponentV2::RAGDOLLING)
                    {
                        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
                    }

                    this->releaseConstraintDirection();
                    this->releaseConstraintAxis();
                    this->destroyBody();
                    this->createRagDoll(this->partialRagdollBoneName);

                    this->startRagdolling();
                }
                else if (this->rdState == PhysicsRagDollComponentV2::INACTIVE)
                {
                    createInactiveRagdoll();
                }
                else if (this->rdState == PhysicsRagDollComponentV2::ANIMATION)
                {
                    boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);

                    this->endRagdolling();

                    this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
                    if (this->rdOldState == PhysicsRagDollComponentV2::RAGDOLLING)
                    {
                        this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
                    }
                    this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
                    if (this->rdOldState != PhysicsRagDollComponentV2::RAGDOLLING)
                    {
                        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
                    }

                    this->releaseConstraintDirection();
                    this->releaseConstraintAxis();
                    this->destroyBody();
                    this->createDynamicBody();
                }

                this->internalShowDebugData(true);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV2::internalApplyState");
        }
    }

    // ============================================================================
    // createInactiveRagdoll
    // ============================================================================

    void PhysicsRagDollComponentV2::createInactiveRagdoll()
    {
        boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);

        this->partialRagdollBoneName = "";

        this->destroyBody();

        this->endRagdolling();
        this->releaseConstraintDirection();
        this->releaseConstraintAxis();

        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        if (this->rdOldState == PhysicsRagDollComponentV2::RAGDOLLING)
        {
            this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
        }
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        if (this->rdOldState != PhysicsRagDollComponentV2::RAGDOLLING)
        {
            this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
        }

        this->createDynamicBody();

        // V2: Restore all bones to animation control
        // (endRagdolling already handles resetToPose and manual bone restoration)
    }

    // ============================================================================
    // setBoneConfigFile / getBoneConfigFile
    // ============================================================================

    void PhysicsRagDollComponentV2::setBoneConfigFile(const Ogre::String& boneConfigFile)
    {
        this->boneConfigFile->setValue(boneConfigFile);
    }

    Ogre::String PhysicsRagDollComponentV2::getBoneConfigFile(void) const
    {
        return this->boneConfigFile->getString();
    }

    // ============================================================================
    // setVelocity / getVelocity
    // ============================================================================

    void PhysicsRagDollComponentV2::setVelocity(const Ogre::Vector3& velocity)
    {
        if (true == this->isSimulating)
        {
            this->physicsBody->setVelocity(velocity);
        }
    }

    Ogre::Vector3 PhysicsRagDollComponentV2::getVelocity(void) const
    {
        if (true == this->isSimulating)
        {
            if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
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

    // ============================================================================
    // setPosition / getPosition / translate
    // ============================================================================

    void PhysicsRagDollComponentV2::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
    {
        if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->gameObjectPtr->getSceneNode(), Ogre::Vector3(x, y, z));
            }
        }
        else
        {
            PhysicsComponent::setPosition(x, y, z);
        }
    }

    void PhysicsRagDollComponentV2::setPosition(const Ogre::Vector3& position)
    {
        if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->gameObjectPtr->getSceneNode(), position);
            }
        }
        else
        {
            PhysicsComponent::setPosition(position);
        }
    }

    void PhysicsRagDollComponentV2::translate(const Ogre::Vector3& relativePosition)
    {
        if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->gameObjectPtr->getSceneNode(), this->gameObjectPtr->getSceneNode()->getPosition() + relativePosition);
            }
        }
        else
        {
            PhysicsComponent::translate(relativePosition);
        }
    }

    Ogre::Vector3 PhysicsRagDollComponentV2::getPosition(void) const
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

    // ============================================================================
    // setOrientation / getOrientation / rotate
    // ============================================================================

    void PhysicsRagDollComponentV2::setOrientation(const Ogre::Quaternion& orientation)
    {
        if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING || this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->gameObjectPtr->getSceneNode(), orientation);
            }
            else
            {
                PhysicsComponent::setOrientation(orientation);
            }
        }
        else
        {
            PhysicsComponent::setOrientation(orientation);
        }
    }

    void PhysicsRagDollComponentV2::rotate(const Ogre::Quaternion& relativeRotation)
    {
        if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
        {
            if (false == this->isSimulating)
            {
                NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->gameObjectPtr->getSceneNode(), this->gameObjectPtr->getSceneNode()->getOrientation() * relativeRotation);
            }
        }
        else
        {
            PhysicsComponent::rotate(relativeRotation);
        }
    }

    Ogre::Quaternion PhysicsRagDollComponentV2::getOrientation(void) const
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

    // ============================================================================
    // setBoneRotation
    // ============================================================================

    void PhysicsRagDollComponentV2::setBoneRotation(const Ogre::String& boneName, const Ogre::Vector3& axis, Ogre::Real degree)
    {
        this->oldPartialRagdollBoneName = boneName;
        this->partialRagdollBoneName = boneName;
        this->rdOldState = this->rdState;
    }

    // ============================================================================
    // setInitialState
    // ============================================================================

    void PhysicsRagDollComponentV2::setInitialState(void)
    {
        size_t offset = 0;
        if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
        {
            offset = 1;
        }

        for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
        {
            it->ragBone->setInitialState();
        }
    }

    // ============================================================================
    // setAnimationEnabled / isAnimationEnabled
    // ============================================================================

    void PhysicsRagDollComponentV2::setAnimationEnabled(bool animationEnabled)
    {
        this->animationEnabled = animationEnabled;

        size_t offset = 0;
        if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
        {
            offset = 1;
        }

        for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
        {
            // V2: use setManualBone instead of setManuallyControlled
            if (nullptr != this->skeletonInstance)
            {
                this->skeletonInstance->setManualBone(it->ragBone->getBone(), !animationEnabled);
            }
            it->ragBone->applyPose(Ogre::Vector3::ZERO);
        }
    }

    bool PhysicsRagDollComponentV2::isAnimationEnabled(void) const
    {
        return this->animationEnabled;
    }

    // ============================================================================
    // createRagDollFromXML - Parses the SAME .rag XML format as V1
    // ============================================================================

    bool PhysicsRagDollComponentV2::createRagDollFromXML(rapidxml::xml_node<>* rootXmlElement, const Ogre::String& boneName)
    {
        RagBone* parentRagBone = nullptr;

        rapidxml::xml_node<>* bonesXmlElement = rootXmlElement->first_node("Bones");
        if (nullptr == bonesXmlElement)
        {
            Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponentV2] Error: cannot find 'Bones' XML element in xml file: " + this->boneConfigFile->getString());
            return false;
        }

        bool partial = false;
        bool first = true;
        if (PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING == this->rdState)
        {
            partial = true;
        }

        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();

		// CRITICAL: Ensure skeleton derived transforms are up-to-date before creating rag bones.
		// V2 Bone::_getLocalSpaceTransform() assumes caches are already updated (unlike V1's
		// _getDerivedPosition() which lazily recomputes). Without this, the RagBone constructor
		// gets stale/uninitialized position values, causing the ragdoll to explode.
		if (nullptr != this->skeletonInstance)
		{
			this->skeletonInstance->update();
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

                // V2: Use IdString for bone lookup
                Ogre::Bone* sourceBone = this->skeletonInstance->getBone(Ogre::IdString(sourceBoneName));
                Ogre::Bone* targetBone = this->skeletonInstance->getBone(Ogre::IdString(targetBoneName));
                this->boneCorrectionMap.emplace(sourceBone, std::make_pair(targetBone, offset));

                boneXmlElement = boneXmlElement->next_sibling("Bone");
                continue;
            }

            // Get bone size
            auto* sizeAttr = boneXmlElement->first_attribute("Size");
            if (nullptr == sizeAttr)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error: Size could not be loaded from XML for: " + this->gameObjectPtr->getName());
                return false;
            }
            Ogre::Vector3 size = Ogre::StringConverter::parseVector3(sizeAttr->value());

            // Get skeleton bone name
            auto* skeletonBoneAttr = boneXmlElement->first_attribute("SkeletonBone");
            Ogre::String skeletonBone = skeletonBoneAttr ? skeletonBoneAttr->value() : "";

            if (skeletonBone.empty() && (false == first || false == partial))
            {
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error: Skeleton bone name could not be loaded from XML for: "
					+ this->gameObjectPtr->getName());
                return false;
            }

            // V2: Get bone via IdString
            Ogre::Bone* bone = nullptr;
            if (false == skeletonBone.empty())
            {
                Ogre::IdString boneIdString(skeletonBone);
                if (!this->skeletonInstance->hasBone(boneIdString))
                {
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error: Skeleton bone name: " + skeletonBone + " not found for game object: "
						+ this->gameObjectPtr->getName());
                    return false;
                }
                bone = this->skeletonInstance->getBone(boneIdString);
            }

            // Get display name
            Ogre::String boneNameFromFile;
            auto* nameAttr = boneXmlElement->first_attribute("Name");
            Ogre::String strName = nameAttr ? nameAttr->value() : "";
            if (!strName.empty())
            {
                boneNameFromFile = strName;
            }
            else
            {
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error: Representation name could not be loaded from XML for: "
					+ this->gameObjectPtr->getName());
                return false;
            }

            // Get category
            Ogre::String strCategory;
            if (auto* catAttr = boneXmlElement->first_attribute("Category"))
            {
                strCategory = catAttr->value();
            }

            // Get offset
            Ogre::Vector3 offset = Ogre::Vector3::ZERO;
            if (auto* offsetAttr2 = boneXmlElement->first_attribute("Offset"))
            {
                offset = Ogre::StringConverter::parseVector3(offsetAttr2->value());
            }

            // Get shape type
            auto* shapeAttr = boneXmlElement->first_attribute("Shape");
            Ogre::String strShape = shapeAttr ? shapeAttr->value() : "";
            PhysicsRagDollComponentV2::RagBone::BoneShape shape = PhysicsRagDollComponentV2::RagBone::BS_BOX;

            if (strShape == "Hull")
            {
                shape = PhysicsRagDollComponentV2::RagBone::BS_CONVEXHULL;
            }
            else if (strShape == "Box")
            {
                shape = PhysicsRagDollComponentV2::RagBone::BS_BOX;
            }
            else if (strShape == "Capsule")
            {
                shape = PhysicsRagDollComponentV2::RagBone::BS_CAPSULE;
            }
            else if (strShape == "Cylinder")
            {
                shape = PhysicsRagDollComponentV2::RagBone::BS_CYLINDER;
            }
            else if (strShape == "Cone")
            {
                shape = PhysicsRagDollComponentV2::RagBone::BS_CONE;
            }
            else if (strShape == "Ellipsoid")
            {
                shape = PhysicsRagDollComponentV2::RagBone::BS_ELLIPSOID;
            }

            // Get mass
            auto* massAttr = boneXmlElement->first_attribute("Mass");
            if (nullptr == massAttr)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error: Mass could not be loaded from XML for: " + this->gameObjectPtr->getName());
                return false;
            }
            Ogre::Real mass = Ogre::StringConverter::parseReal(massAttr->value());

            ///////////////////////////////////////////////////////////////////////////////

            RagBone* currentRagBone = new RagBone(boneNameFromFile, this, parentRagBone, bone, item, constraintDirection->getVector3(), shape, size, mass, partial, offset, strCategory);

            PhysicsRagDollComponentV2::RagData ragData;
            ragData.ragBoneName = boneNameFromFile;
            ragData.ragBone = currentRagBone;
            this->ragDataList.emplace_back(ragData);

            boneXmlElement = boneXmlElement->next_sibling("Bone");
            parentRagBone = currentRagBone;

            first = false;
        }

        // ============================================================================
        // Parse Joints section (SAME format as V1)
        // ============================================================================

        rapidxml::xml_node<>* jointsXmlElement = rootXmlElement->first_node("Joints");
        if (nullptr == jointsXmlElement)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error: cannot find 'Joints' XML element in xml file: " + this->boneConfigFile->getString());
            return false;
        }

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
            RagBone* parentRagBone2 = this->getRagBone(ragBoneParentName);

            if (nullptr == childRagBone)
            {
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Warning: There is no child rag bone name: " + ragBoneChildName + " specified for game object " + this->gameObjectPtr->getName());
            }

            Ogre::Real friction = -1.0f;
            if (auto* frictionAttr = jointXmlElement->first_attribute("Friction"))
            {
                friction = Ogre::StringConverter::parseReal(frictionAttr->value());
            }

            auto* typeAttr = jointXmlElement->first_attribute("Type");
            Ogre::String strJointType = typeAttr ? typeAttr->value() : "";
            PhysicsRagDollComponentV2::JointType jointType = PhysicsRagDollComponentV2::JT_BASE;
            Ogre::Vector3 pin = Ogre::Vector3::ZERO;

            if (strJointType == JointComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponentV2::JT_BASE;
            }
            else if (strJointType == JointBallAndSocketComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponentV2::JT_BALLSOCKET;
            }
            else if (strJointType == JointHingeComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponentV2::JT_HINGE;
                auto* pinAttr = jointXmlElement->first_attribute("Pin");
                if (pinAttr)
                {
                    pin = Ogre::StringConverter::parseVector3(pinAttr->value());
                }
                else
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                        "[PhysicsRagDollComponentV2] Error: Pin for hinge joint is missing in " + this->boneConfigFile->getString() + " for bone: '" + childRagBone->getName() + "' for game object " + this->gameObjectPtr->getName());
                    return false;
                }
            }
            else if (strJointType == JointUniversalComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponentV2::JT_DOUBLE_HINGE;
                if (auto* pinAttr = jointXmlElement->first_attribute("Pin"))
                {
                    pin = Ogre::StringConverter::parseVector3(pinAttr->value());
                }
            }
            else if (strJointType == JointHingeActuatorComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponentV2::JT_HINGE_ACTUATOR;
                auto* pinAttr = jointXmlElement->first_attribute("Pin");
                if (pinAttr)
                {
                    pin = Ogre::StringConverter::parseVector3(pinAttr->value());
                }
                else
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                        "[PhysicsRagDollComponentV2] Error: Pin for hinge actuator joint is missing in " + this->boneConfigFile->getString() + " for bone: '" + childRagBone->getName() + "' for game object " + this->gameObjectPtr->getName());
                    return false;
                }
            }
            else if (strJointType == JointUniversalActuatorComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponentV2::JT_DOUBLE_HINGE_ACTUATOR;
                auto* pinAttr = jointXmlElement->first_attribute("Pin");
                if (pinAttr)
                {
                    pin = Ogre::StringConverter::parseVector3(pinAttr->value());
                }
                else
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                        "[PhysicsRagDollComponentV2] Error: Pin for universal actuator joint is missing in " + this->boneConfigFile->getString() + " for bone: '" + childRagBone->getName() + "' for game object " + this->gameObjectPtr->getName());
                    return false;
                }
            }
            else if (strJointType == JointKinematicComponent::getStaticClassName())
            {
                jointType = PhysicsRagDollComponentV2::JT_KINEMATIC;
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
                Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponentV2] Error: Cannot find 'Child' XML element in xml file: " + this->boneConfigFile->getString());
                return false;
            }

            bool useSpring = false;
            if (auto* springAttr = jointXmlElement->first_attribute("Spring"))
            {
                useSpring = Ogre::StringConverter::parseBool(springAttr->value());
            }

            Ogre::Vector3 jointOffset = Ogre::Vector3::ZERO;
            if (auto* offsetAttr3 = jointXmlElement->first_attribute("Offset"))
            {
                jointOffset = Ogre::StringConverter::parseVector3(offsetAttr3->value());
            }

            if (nullptr != childRagBone)
            {
                this->joinBones(jointType, childRagBone, parentRagBone2, pin, minTwistAngle, maxTwistAngle, maxConeAngle, minTwistAngle2, maxTwistAngle2, friction, useSpring, jointOffset);
            }
            jointXmlElement = jointXmlElement->next_sibling("Joint");
        }

        return true;
    }

    // ============================================================================
    // joinBones - Identical logic to V1
    // ============================================================================

    void PhysicsRagDollComponentV2::joinBones(PhysicsRagDollComponentV2::JointType type, RagBone* childRagBone, RagBone* parentRagBone, const Ogre::Vector3& pin, const Ogre::Degree& minTwistAngle, const Ogre::Degree& maxTwistAngle,
        const Ogre::Degree& maxConeAngle, const Ogre::Degree& minTwistAngle2, const Ogre::Degree& maxTwistAngle2, Ogre::Real friction, bool useSpring, const Ogre::Vector3& offset)
    {
        JointCompPtr parentJointCompPtr;
        JointCompPtr childJointCompPtr;

        if (nullptr == parentRagBone)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2]: Error: Cannot joint bones, because the corresponding rag bone is null for game object: " + this->gameObjectPtr->getName());
            return;
        }

        if (type != PhysicsRagDollComponentV2::JT_KINEMATIC)
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

        switch (type)
        {
        case PhysicsRagDollComponentV2::JT_BASE:
        {
            JointCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointComponent>(childRagBone->createJointComponent(JointBallAndSocketComponent::getStaticClassName()));

            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setStiffness(0.1f);
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);
            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponentV2::JT_BALLSOCKET:
        {
            JointBallAndSocketCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointBallAndSocketComponent>(childRagBone->createJointComponent(JointBallAndSocketComponent::getStaticClassName()));

            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
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
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponentV2::JT_HINGE:
        {
            JointHingeCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointHingeComponent>(childRagBone->createJointComponent(JointHingeComponent::getStaticClassName()));

            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            tempChildCompPtr->setPin(pin);

            tempChildCompPtr->setMinMaxAngleLimit(minTwistAngle, maxTwistAngle);

            tempChildCompPtr->createJoint();

            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponentV2::JT_HINGE_ACTUATOR:
        {
            JointHingeActuatorCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointHingeActuatorComponent>(childRagBone->createJointComponent(JointHingeActuatorComponent::getStaticClassName()));

            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            tempChildCompPtr->setPin(pin);

            tempChildCompPtr->setMinAngleLimit(minTwistAngle);
            tempChildCompPtr->setMaxAngleLimit(maxTwistAngle);

            tempChildCompPtr->createJoint();

            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponentV2::JT_DOUBLE_HINGE:
        {
            JointUniversalCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointUniversalComponent>(childRagBone->createJointComponent(JointUniversalComponent::getStaticClassName()));

            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
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
        case PhysicsRagDollComponentV2::JT_DOUBLE_HINGE_ACTUATOR:
        {
            JointUniversalActuatorCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointUniversalActuatorComponent>(childRagBone->createJointComponent(JointUniversalActuatorComponent::getStaticClassName()));

            tempChildCompPtr->setBodyMassScale(Ogre::Vector2(0.1f, 1.0f));
            tempChildCompPtr->setBody(childRagBone->getBody());
            tempChildCompPtr->setAnchorPosition(offset);
            tempChildCompPtr->setStiffness(0.1f);
            tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
            tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

            tempChildCompPtr->setMinAngleLimit0(minTwistAngle);
            tempChildCompPtr->setMaxAngleLimit0(maxTwistAngle);
            tempChildCompPtr->setMinAngleLimit0(minTwistAngle2);
            tempChildCompPtr->setMaxAngleLimit0(maxTwistAngle2);
            tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

            childJointCompPtr = tempChildCompPtr;
            AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(tempChildCompPtr);
        }
        break;
        case PhysicsRagDollComponentV2::JT_KINEMATIC:
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
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponentV2]: Name: " + this->gameObjectPtr->getName() +
                                                                                       " child, joint pos: " + Ogre::StringConverter::toString(childJointCompPtr->getBody()->getPosition()) +
                                                                                       " child, joint ori: " + Ogre::StringConverter::toString(childJointCompPtr->getBody()->getOrientation()) + " between child: " + childRagBone->getName() +
                                                                                       " and parent: " + parentRagBone->getName() + ", joint pos: " + Ogre::StringConverter::toString(parentJointCompPtr->getBody()->getPosition()) +
                                                                                       ", joint ori: " + Ogre::StringConverter::toString(parentJointCompPtr->getBody()->getOrientation()) + ", joint pin: " + Ogre::StringConverter::toString(pin));
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponentV2]: Name: " + this->gameObjectPtr->getName() +
                                                                                       " child joint pos: " + Ogre::StringConverter::toString(childJointCompPtr->getJointPosition()) + " between child: " + childRagBone->getName() +
                                                                                       ", joint pin: " + Ogre::StringConverter::toString(pin));
            }
        }
    }

    // ============================================================================
    // inheritVelOmega
    // ============================================================================

    void PhysicsRagDollComponentV2::inheritVelOmega(Ogre::Vector3 vel, Ogre::Vector3 omega)
    {
        Ogre::Vector3 mainpos = this->gameObjectPtr->getSceneNode()->_getDerivedPosition();

        size_t offset = 0;
        if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
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

    // ============================================================================
    // setState / getState
    // ============================================================================

    void PhysicsRagDollComponentV2::setState(const Ogre::String& state)
    {
        this->state->setListSelectedValue(state);

        this->rdOldState = this->rdState;

        if (state == "Inactive")
        {
            if (this->rdState == PhysicsRagDollComponentV2::INACTIVE)
            {
                return;
            }
            this->rdState = PhysicsRagDollComponentV2::INACTIVE;

            boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        }
        else if (state == "Animation")
        {
            if (this->rdState == PhysicsRagDollComponentV2::ANIMATION)
            {
                return;
            }
            this->rdState = PhysicsRagDollComponentV2::ANIMATION;
            boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        }
        else if (state == "Ragdolling")
        {
            if (nullptr == this->gameObjectPtr)
            {
                return;
            }

            if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
            {
                return;
            }
            this->rdState = PhysicsRagDollComponentV2::RAGDOLLING;
            this->animationEnabled = false;
            boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), true));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        }
        else if (state == "PartialRagdolling")
        {
            if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING && this->oldPartialRagdollBoneName == this->partialRagdollBoneName)
            {
                return;
            }
            this->rdState = PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING;

            boost::shared_ptr<EventDataGameObjectIsInRagDollingState> eventDataGameObjectIsInRagDollingState(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGameObjectIsInRagDollingState);
        }

        if (true == this->isSimulating)
        {
            this->internalApplyState();
        }
    }

    Ogre::String PhysicsRagDollComponentV2::getState(void) const
    {
        return this->state->getListSelectedValue();
    }

    // ============================================================================
    // getBody
    // ============================================================================

    OgreNewt::Body* PhysicsRagDollComponentV2::getBody(void) const
    {
        return this->physicsBody;
    }

    // ============================================================================
    // getRagDataList / getRagBone
    // ============================================================================

    const std::vector<PhysicsRagDollComponentV2::RagData>& PhysicsRagDollComponentV2::getRagDataList(void)
    {
        return this->ragDataList;
    }

    PhysicsRagDollComponentV2::RagBone* PhysicsRagDollComponentV2::getRagBone(const Ogre::String& name)
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

    // ============================================================================
    // applyModelStateToRagdoll
    // ============================================================================

    void PhysicsRagDollComponentV2::applyModelStateToRagdoll(void)
    {
        const auto node = this->gameObjectPtr->getSceneNode();
        Ogre::Vector3 nodePos = node->_getDerivedPosition();
        Ogre::Quaternion nodeOri = node->_getDerivedOrientation();
        Ogre::Vector3 nodeScale = node->_getDerivedScale();

        size_t i = 0;

        if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
        {
            i = 1;
        }

        for (; i < this->ragDataList.size(); i++)
        {
            auto& ragBone = this->ragDataList[i].ragBone;
            if (nullptr == ragBone || nullptr == ragBone->getBody() || nullptr == ragBone->getBone())
            {
                continue;
            }

            // Compute bone's current world position/orientation from the skeleton animation.
            // V2: extractBoneDerivedTransform gives skeleton-space (equivalent of V1's _getDerivedPosition/Orientation)
            Ogre::Vector3 boneSkelPos;
            Ogre::Quaternion boneSkelOri;
            PhysicsRagDollComponentV2::extractBoneDerivedTransform(ragBone->getBone(), boneSkelPos, boneSkelOri);

            Ogre::Vector3 boneWorldPos = nodePos + nodeOri * (nodeScale * boneSkelPos);
            Ogre::Quaternion boneWorldOri = nodeOri * boneSkelOri;

            ragBone->getBody()->setPositionOrientation(boneWorldPos, boneWorldOri);
        }

        // Set constraint axis for root body
        this->setConstraintAxis(this->constraintAxis->getVector3());

        if (this->rdState == PhysicsRagDollComponentV2::RAGDOLLING)
        {
            this->releaseConstraintAxisPin();
        }
        else if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
        {
            this->setConstraintDirection(this->constraintDirection->getVector3());
        }
    }

    // ============================================================================
    // applyRagdollStateToModel
    // ============================================================================

    void PhysicsRagDollComponentV2::applyRagdollStateToModel(void)
    {
        if (true == this->ragDataList.empty())
        {
            return;
        }

        size_t i = 0;

        if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
        {
            i = 1;
        }

        const auto node = this->gameObjectPtr->getSceneNode();
        Ogre::Quaternion nodeOri = node->_getDerivedOrientation();
        Ogre::Vector3 nodePos = node->_getDerivedPosition();
        Ogre::Vector3 nodeScale = node->_getDerivedScale();
        Ogre::Quaternion invNodeOri = nodeOri.Inverse();

        // Track skeleton-space transforms we compute from physics bodies.
        // This is needed because after setting a bone's position/orientation,
        // its _getLocalSpaceTransform() won't update until the next skeletonInstance->update().
        // By tracking what we set, child bones can use accurate parent transforms.
        std::unordered_map<Ogre::Bone*, std::pair<Ogre::Vector3, Ogre::Quaternion>> computedSkelTransforms;

        // IMPORTANT: Process root-to-leaf order (ragDataList is in that order from XML parsing).
        for (; i < this->ragDataList.size(); i++)
        {
            auto& ragBone = this->ragDataList[i].ragBone;
            if (nullptr == ragBone || nullptr == ragBone->getBody() || nullptr == ragBone->getBone())
            {
                continue;
            }

            Ogre::Bone* bone = ragBone->getBone();

            // Get physics body's world transform
            Ogre::Vector3 bodyWorldPos = ragBone->getBody()->getPosition();
            Ogre::Quaternion bodyWorldOri = ragBone->getBody()->getOrientation();

            // Convert world -> skeleton space (entity-local)
            Ogre::Vector3 skelPos = invNodeOri * (bodyWorldPos - nodePos);
            skelPos /= nodeScale;
            Ogre::Quaternion skelOri = invNodeOri * bodyWorldOri;

            // Store for children to use
            computedSkelTransforms[bone] = std::make_pair(skelPos, skelOri);

            // With setInheritOrientation(false):
            //   The derived orientation equals what we set (local = skeleton-space).
            //   So setting orientation to skeleton-space orientation is correct.
            //
            // For POSITION however, Ogre still computes derived position through the parent chain:
            //   derivedPos = parent->derivedOri * (parent->derivedScale * bone->getPosition()) + parent->derivedPos
            // So we must convert skeleton-space position to parent-bone-local space.

            Ogre::Bone* parentBone = bone->getParent();
            Ogre::Vector3 boneLocalPos;
            if (parentBone)
            {
                Ogre::Vector3 parentDerivedPos;
                Ogre::Quaternion parentDerivedOri;

                // Check if we already computed this parent's transform in this frame
                auto parentIt = computedSkelTransforms.find(parentBone);
                if (parentIt != computedSkelTransforms.end())
                {
                    parentDerivedPos = parentIt->second.first;
                    parentDerivedOri = parentIt->second.second;
                }
                else
                {
                    // Parent is not a ragdoll bone - use skeleton's stored transform
                    extractBoneDerivedTransform(parentBone, parentDerivedPos, parentDerivedOri);
                }

                Ogre::Quaternion invParentOri = parentDerivedOri.Inverse();
                boneLocalPos = invParentOri * (skelPos - parentDerivedPos);
                // Note: bone hierarchy scale is typically unit scale for ragdolls
            }
            else
            {
                // Root bone - skeleton space IS local space
                boneLocalPos = skelPos;
            }

            // This runs on the render thread (inside tracked closure), so direct bone access is safe
            bone->setPosition(boneLocalPos);
            bone->setOrientation(skelOri);
            // NOWA::GraphicsModule::getInstance()->updateBoneTransform(bone, boneLocalPos, skelOri);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[PhysicsRagDollComponentV2] applyRagdollStateToModel BoneName: " + ragBone->getName() + " boneLocalPos: " + Ogre::StringConverter::toString(boneLocalPos) + " skelOri: " + Ogre::StringConverter::toString(skelOri));

        }

        // Apply bone corrections
        for (auto& boneCorrectionData : this->boneCorrectionMap)
        {
            // V2: Extract position and orientation from the target bone's local space transform
            Ogre::Vector3 targetPos;
            Ogre::Quaternion targetOrient;
            extractBoneDerivedTransform(boneCorrectionData.second.first, targetPos, targetOrient);

            boneCorrectionData.first->setPosition(targetPos + boneCorrectionData.second.second);
            boneCorrectionData.first->setOrientation(targetOrient);
        }
    }

    // ============================================================================
    // startRagdolling
    // ============================================================================

    void PhysicsRagDollComponentV2::startRagdolling(void)
    {
        // Reset all animations. Must be done, because a game object may be in the middle of an animation
        if (nullptr != this->skeletonInstance)
        {
            Ogre::SkeletonAnimationVec& animations = this->skeletonInstance->getAnimationsNonConst();
            for (auto& anim : animations)
            {
                anim.setEnabled(false);
                anim.mWeight = 0.0f;
                anim.setFrame(0.0f);
            }
        }

        this->applyModelStateToRagdoll();

        if (true == this->ragDataList.empty())
        {
            return;
        }

        if (nullptr == this->skeletonInstance)
        {
            return;
        }

        if (this->rdState == PhysicsRagDollComponentV2::PARTIAL_RAGDOLLING)
        {
            if (false == this->ragDataList.empty())
            {
                this->ragDataList.front().ragBone->attachToNode();
            }

            // Set bones to manually controlled
            for (size_t j = 1; j < this->ragDataList.size(); j++)
            {
                for (size_t i = 0; i < this->skeletonInstance->getNumBones(); i++)
                {
                    Ogre::Bone* bone = this->skeletonInstance->getBone(i);

                    if (bone == this->ragDataList[j].ragBone->getBone())
                    {
                        // V2: Get derived orientation from the bone's local space transform
                        Ogre::Vector3 bonePos;
                        Ogre::Quaternion absoluteWorldOrientation;
                        extractBoneDerivedTransform(bone, bonePos, absoluteWorldOrientation);

                        // V2: use setManualBone and setInheritOrientation
                        this->skeletonInstance->setManualBone(bone, true);
                        bone->setInheritOrientation(false);

                        NOWA::GraphicsModule::getInstance()->updateBoneOrientation(bone, absoluteWorldOrientation);

                        this->ragDataList[j].ragBone->attachToNode();
                        break;
                    }
                }
            }
        }
        else
        {
            // Full ragdoll: Set ALL bones in the skeleton to manually controlled.
            // This prevents animation from overwriting our physics-driven transforms.
            for (size_t idx = 0; idx < this->skeletonInstance->getNumBones(); idx++)
            {
                Ogre::Bone* bone = this->skeletonInstance->getBone(idx);

                // V2: Get derived orientation from the bone's local space transform
                Ogre::Vector3 bonePos;
                Ogre::Quaternion absoluteWorldOrientation;
                extractBoneDerivedTransform(bone, bonePos, absoluteWorldOrientation);

                this->skeletonInstance->setManualBone(bone, true);
                bone->setInheritOrientation(false);

                NOWA::GraphicsModule::getInstance()->updateBoneOrientation(bone, absoluteWorldOrientation);
            }

            // Attach ragdoll bones to their scene nodes (separate loop!)
            for (size_t j = 0; j < this->ragDataList.size(); j++)
            {
                this->ragDataList[j].ragBone->attachToNode();
            }
        }
    }

    // ============================================================================
    // endRagdolling
    // ============================================================================

    void PhysicsRagDollComponentV2::endRagdolling(void)
    {
        if (nullptr == this->skeletonInstance)
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

            // V2: Reset skeleton to binding pose (replaces V1's skeleton unload/reload)
            this->skeletonInstance->resetToPose();

            // Reset all animations
            Ogre::SkeletonAnimationVec& animations = this->skeletonInstance->getAnimationsNonConst();
            for (auto& anim : animations)
            {
                anim.setEnabled(false);
                anim.mWeight = 0.0f;
                anim.setFrame(0.0f);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV2::endRagdolling");
    }

    // ============================================================================
    // deleteJointDelegate
    // ============================================================================

    void PhysicsRagDollComponentV2::deleteJointDelegate(EventDataPtr eventData)
    {
        // Reserved for future use
    }

    // Lua registration part

    PhysicsRagDollComponentV2* getPhysicsRagDollComponentV2(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<PhysicsRagDollComponentV2>(gameObject->getComponentWithOccurrence<PhysicsRagDollComponentV2>(occurrenceIndex)).get();
    }

    PhysicsRagDollComponentV2* getPhysicsRagDollComponentV2(GameObject* gameObject)
    {
        return makeStrongPtr<PhysicsRagDollComponentV2>(gameObject->getComponent<PhysicsRagDollComponentV2>()).get();
    }

    PhysicsRagDollComponentV2* getPhysicsRagDollComponentV2FromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<PhysicsRagDollComponentV2>(gameObject->getComponentFromName<PhysicsRagDollComponentV2>(name)).get();
    }

    luabind::object getRagDataListV2(PhysicsRagDollComponentV2* instance)
    {
        luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

        for (size_t i = 0; i < instance->getRagDataList().size(); i++)
        {
            obj[i] = instance->getRagDataList()[i];
        }

        return obj;
    }

    Ogre::String getJointIdV2(PhysicsRagDollComponentV2::RagBone* instance)
    {
        return Ogre::StringConverter::toString(instance->getJointId());
    }

    void PhysicsRagDollComponentV2::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<PhysicsRagDollComponentV2, PhysicsActiveComponent>("PhysicsRagDollComponentV2")
                .def("inheritVelOmega", &PhysicsRagDollComponentV2::inheritVelOmega)
                .def("setActivated", &PhysicsRagDollComponentV2::setActivated)
                .def("setState", &PhysicsRagDollComponentV2::setState)
                .def("setVelocity", &PhysicsRagDollComponentV2::setVelocity)
                .def("getVelocity", &PhysicsRagDollComponentV2::getVelocity)
                .def("getPosition", &PhysicsRagDollComponentV2::getPosition)
                .def("setOrientation", &PhysicsRagDollComponentV2::setOrientation)
                .def("getOrientation", &PhysicsRagDollComponentV2::getOrientation)
                .def("setInitialState", &PhysicsRagDollComponentV2::setInitialState)
                .def("setAnimationEnabled", &PhysicsRagDollComponentV2::setAnimationEnabled)
                .def("isAnimationEnabled", &PhysicsRagDollComponentV2::isAnimationEnabled)
                .def("setBoneConfigFile", &PhysicsRagDollComponentV2::setBoneConfigFile)
                .def("getBoneConfigFile", &PhysicsRagDollComponentV2::getBoneConfigFile)
                .def("getRagDataList", &getRagDataListV2)
                .def("getRagBone", &PhysicsRagDollComponentV2::getRagBone)
                .def("setBoneRotation", &PhysicsRagDollComponentV2::setBoneRotation)
                .scope[class_<PhysicsRagDollComponentV2::RagBone>("RagBone")
                        .def("getName", &PhysicsRagDollComponentV2::RagBone::getName)
                        .def("getPosition", &PhysicsRagDollComponentV2::RagBone::getPosition)
                        .def("setOrientation", &PhysicsRagDollComponentV2::RagBone::setOrientation)
                        .def("getOrientation", &PhysicsRagDollComponentV2::RagBone::getOrientation)
                        .def("setInitialState", &PhysicsRagDollComponentV2::RagBone::setInitialState)
                        .def("getOgreBone", &PhysicsRagDollComponentV2::RagBone::getBone)
                        .def("getParentRagBone", &PhysicsRagDollComponentV2::RagBone::getParentRagBone)
                        .def("getInitialBonePosition", &PhysicsRagDollComponentV2::RagBone::getInitialBonePosition)
                        .def("getInitialBoneOrientation", &PhysicsRagDollComponentV2::RagBone::getInitialBoneOrientation)
                        .def("getPhysicsRagDollComponentV2", &PhysicsRagDollComponentV2::RagBone::getPhysicsRagDollComponent)
                        .def("getRagPose", &PhysicsRagDollComponentV2::RagBone::getRagPose)
                        .def("applyPose", &PhysicsRagDollComponentV2::RagBone::applyPose)
                        .def("applyRequiredForceForVelocity", &PhysicsRagDollComponentV2::RagBone::applyRequiredForceForVelocity)
                        .def("applyOmegaForce", &PhysicsRagDollComponentV2::RagBone::applyOmegaForce)
                        .def("applyOmegaForceRotateTo", &PhysicsRagDollComponentV2::RagBone::applyOmegaForceRotateTo)
                        .def("getSize", &PhysicsRagDollComponentV2::RagBone::getBodySize)
                        .def("getJointId", &getJointIdV2)
                        .def("getBody", &PhysicsRagDollComponentV2::RagBone::getBody)]];

        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "class inherits PhysicsActiveComponent", PhysicsRagDollComponentV2::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "void setVelocity(Vector3 velocity)",
            "Sets the global linear velocity on the physics body. Note: This should only be used for initzialisation. Use @applyRequiredForceForVelocity in simualtion instead. Or it may be called if its a physics active kinematic body.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "Vector3 getVelocity()", "Gets currently acting velocity on the body.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "Vector3 getPosition()", "Gets the position of the physics component.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "void setOrientation(Quaternion orientation)",
            "Sets the orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "Quaternion getOrientation()", "Gets the orientation of the physics component.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "void setInitialState()", "If in ragdoll state, resets all bones ot its initial position and orientation.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "void setAnimationEnabled(bool enabled)",
            "Enables animation for the ragdoll. That is, the bones are no more controlled manually, but transform comes from animation state.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "bool isAnimationEnabled()", "Gets whether the ragdoll is being animated.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "void setBoneConfigFile(String boneConfigFile)",
            "Sets the bone configuration file. Which describes in XML, how the ragdoll is configure. The file must be placed in the same folder as the mesh and skeleton file. Note: The file can be exchanged at runtime, if a different ragdoll "
            "configuration is desire.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "String getBoneConfigFile()", "Gets the currently applied bone config file.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "Table[RagBone] getRagDataList()", "Gets List of all configured rag bones.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "RagBone getRagBone(String ragboneName)", "Gets RagBone from the given name or nil, if it does not exist.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV2", "void setBoneRotation(String ragboneName, Vector3 axis, float degree)", "Rotates the given RagBone around the given axis by degree amount.");

        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "class", "The inner class RagBone represents one physically controlled rag bone.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "String getName()", "Gets name of this bone, that has been specified in the bone config file.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "Vector3 getPosition()", "Gets the position of this rag bone in global space.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "void setOrientation(Quaternion orientation)",
            "Sets the orientation of this rag bone in global space. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "Quaternion getOrientation()", "Gets the orientation of this rag bone in global space.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "void setInitialState()", "If in ragdoll state, resets this rag bone ot its initial position and orientation.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "Bone getOgreBone()", "Gets the Ogre v2 bone.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "RagBone getParentRagBone()", "Gets the parent rag bone or nil, if its the root.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "Vector3 getInitialBonePosition()", "Gets the initial position of this rag bone in global space.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "Quaternion getInitialBoneOrientation()", "Gets the initial orientation of this rag bone in global space.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "PhysicsRagDollComponentV2 getPhysicsRagDollComponentV2()", "Gets PhysicsRagDollComponentV2 outer class object from this rag bone.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "Vector3 getPose()",
            "Gets the pose of this rag bone. Note: This is a vector if not ZERO, that is used to constraint a specific axis, so that the rag bone can not be moved along that axis.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "void applyPose(Vector3 pose)",
            "Applies a the pose to this rag bone. Note: This is a vector if not ZERO, that is used to constraint a specific axis, so that the rag bone can not be moved along that axis.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "void applyRequiredForceForVelocity(Vector3 velocity)", "Applies internally as many force, to satisfy the given velocity and move the bone by that force.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "void applyOmegaForce(Vector3 omega)", "Applies omega force in order to rotate the rag bone.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "void applyOmegaForceRotateTo(Quaternion resultOrientation, Vector3 axes, Ogre::Real strength)",
            "Applies omega force in order to rotate the game object to the given orientation. "
            "The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.). "
            "The strength at which the rotation should occur. "
            "Note: This should be used during simulation instead of @setOmegaVelocity.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "Vector3 getSize()", "Gets the size of the bone.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "string getJointId()", "Gets the joint id of the given rag bone for direct attachment.");
        LuaScriptApi::getInstance()->addClassToCollection("RagBone", "Body getBody()", "Gets the OgreNewt physics body for direct attachment.");

        gameObjectClass.def("getPhysicsRagDollComponentV2FromName", &getPhysicsRagDollComponentV2FromName);
        gameObjectClass.def("getPhysicsRagDollComponentV2", (PhysicsRagDollComponentV2 * (*)(GameObject*)) & getPhysicsRagDollComponentV2);
        // If its desired to create several of this components for one game object
        gameObjectClass.def("getPhysicsRagDollComponentV22", (PhysicsRagDollComponentV2 * (*)(GameObject*, unsigned int)) & getPhysicsRagDollComponentV2);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsRagDollComponentV2 getPhysicsRagDollComponentV22(unsigned int occurrenceIndex)",
            "Gets the component by the given occurence index, since a game object may this component maybe several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsRagDollComponentV2 getPhysicsRagDollComponentV2()", "Gets the component. This can be used if the game object this component just once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsRagDollComponentV2 getPhysicsRagDollComponentV2FromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castPhysicsRagDollComponentV2", &GameObjectController::cast<PhysicsRagDollComponentV2>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PhysicsRagDollComponentV2 castPhysicsRagDollComponentV2(PhysicsRagDollComponentV2 other)", "Casts an incoming type from function for lua auto completion.");
    }

    /*********************************Inner bone class*****************************************************************/

    // ============================================================================
    // RagBone constructor
    // ============================================================================

    PhysicsRagDollComponentV2::RagBone::RagBone(const Ogre::String& name, PhysicsRagDollComponentV2* physicsRagDollComponentV2, RagBone* parentRagBone, Ogre::Bone* bone, Ogre::Item* item, const Ogre::Vector3& pose, RagBone::BoneShape shape,
        Ogre::Vector3& size, Ogre::Real mass, bool partial, const Ogre::Vector3& offset, const Ogre::String& category) :
        name(name),
        physicsRagDollComponentV2(physicsRagDollComponentV2),
        parentRagBone(parentRagBone),
        bone(bone),
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

        // Store the initial orientation and position
        if (true == partial && nullptr == parentRagBone)
        {
            this->initialBoneOrientation = this->physicsRagDollComponentV2->initialOrientation;
            this->initialBonePosition = this->physicsRagDollComponentV2->initialPosition;
        }
        else
        {
            // V2: Use extractBoneDerivedTransform to get skeleton-local position/orientation
            // This is the equivalent of V1's ogreBone->_getDerivedPosition/Orientation()
            Ogre::Vector3 globalPos;
            Ogre::Quaternion globalOrient;
            PhysicsRagDollComponentV2::extractBoneDerivedTransform(bone, globalPos, globalOrient);

            // Apply scale and transform to world space (same as V1)
            globalPos *= this->physicsRagDollComponentV2->initialScale;
            globalPos = this->physicsRagDollComponentV2->initialOrientation * globalPos;
            globalOrient = this->physicsRagDollComponentV2->initialOrientation * globalOrient;
            globalPos += this->physicsRagDollComponentV2->initialPosition;

            this->initialBoneOrientation = globalOrient;
            this->initialBonePosition = globalPos;

            // Compensate body position for ragdollPositionOffset.
            // The offset shifts the collision hull geometry INSIDE the body (e.g., UP by offset amount).
            // To keep the hull aligned with the actual mesh vertices in world space,
            // we shift the body in the OPPOSITE direction (DOWN by offset amount).
            // This also correctly positions the joint pivot at the hull meeting point
            // rather than at the bone's derived position (which may be at the wrong end
            // for bones with unusual orientations, like Boneup with 180° X flip).
            if (this->physicsRagDollComponentV2->ragdollPositionOffset != Ogre::Vector3::ZERO)
            {
				Ogre::Vector3 worldOffset = this->physicsRagDollComponentV2->initialOrientation
					* (this->physicsRagDollComponentV2->initialScale * this->physicsRagDollComponentV2->ragdollPositionOffset);
                this->initialBonePosition -= worldOffset;
            }
        }

        // Truncate near-zero values
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

        // Set also from configured rag file position, orientation offset corrections for the collision hulls
        Ogre::Vector3 collisionPosition = this->physicsRagDollComponentV2->ragdollPositionOffset;
        Ogre::Quaternion collisionOrientation = this->physicsRagDollComponentV2->ragdollOrientationOffset;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
			"[PhysicsRagDollComponentV2] Name: " + name + " initialBonePosition: " + Ogre::StringConverter::toString(this->initialBonePosition) + " initialBoneOrientation: " + Ogre::StringConverter::toString(this->initialBoneOrientation));

        // Create the collision shape based on the Shape attribute
        switch (shape)
        {
        case PhysicsRagDollComponentV2::RagBone::BS_BOX:
        {
            OgreNewt::CollisionPrimitives::Box* col =
                new OgreNewt::CollisionPrimitives::Box(this->physicsRagDollComponentV2->ogreNewt, size, this->physicsRagDollComponentV2->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponentV2::RagBone::BS_CAPSULE:
        {
            OgreNewt::CollisionPrimitives::Capsule* col =
                new OgreNewt::CollisionPrimitives::Capsule(this->physicsRagDollComponentV2->ogreNewt, size.y, size.x, this->physicsRagDollComponentV2->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponentV2::RagBone::BS_CONE:
        {
            OgreNewt::CollisionPrimitives::Cone* col =
                new OgreNewt::CollisionPrimitives::Cone(this->physicsRagDollComponentV2->ogreNewt, size.y, size.x, this->physicsRagDollComponentV2->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponentV2::RagBone::BS_CYLINDER:
        {
            OgreNewt::CollisionPrimitives::Cylinder* col =
                new OgreNewt::CollisionPrimitives::Cylinder(this->physicsRagDollComponentV2->ogreNewt, size.y, size.x, this->physicsRagDollComponentV2->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponentV2::RagBone::BS_ELLIPSOID:
        {
            OgreNewt::CollisionPrimitives::Ellipsoid* col =
                new OgreNewt::CollisionPrimitives::Ellipsoid(this->physicsRagDollComponentV2->ogreNewt, size, this->physicsRagDollComponentV2->gameObjectPtr->getCategoryId(), collisionOrientation, collisionPosition);
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
            break;
        }
        case PhysicsRagDollComponentV2::RagBone::BS_CONVEXHULL:
        {
            if (nullptr != this->bone)
            {
                // V2: Use getWeightedBoneConvexHullV2 with Ogre::Bone* and Ogre::Item*
                // Pass ragdoll offsets (not Vector3::ZERO!) so the hull is correctly positioned
                    collisionPtr = this->physicsRagDollComponentV2->getWeightedBoneConvexHullV2(this->bone, item, size.x, inertia, massOrigin, this->physicsRagDollComponentV2->gameObjectPtr->getCategoryId(), collisionPosition,
                        collisionOrientation, this->physicsRagDollComponentV2->initialScale);
            }
            else
            {
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV2] Error: Cannot create a convex hull for partial ragdoll with no bone for game object: "
					+ this->physicsRagDollComponentV2->getOwner()->getName());
				throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsRagDollComponentV2] Error: Cannot create a convex hull for partial ragdoll with no bone for game object: "
					+ this->physicsRagDollComponentV2->getOwner()->getName() + "\n", "NOWA");
            }
            break;
        }
        }

        this->body = new OgreNewt::Body(this->physicsRagDollComponentV2->ogreNewt, this->physicsRagDollComponentV2->gameObjectPtr->getSceneManager(), collisionPtr);
        NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->body);

        this->body->setGravity(this->physicsRagDollComponentV2->gravity->getVector3());

        // Set the body position and orientation to bone transform
        this->body->setPositionOrientation(this->initialBonePosition, this->initialBoneOrientation);

        if (this->physicsRagDollComponentV2->linearDamping->getReal() != 0.0f)
        {
            this->body->setLinearDamping(this->physicsRagDollComponentV2->linearDamping->getReal());
        }

        if (this->physicsRagDollComponentV2->angularDamping->getVector3() != Ogre::Vector3::ZERO)
        {
            this->body->setAngularDamping(this->physicsRagDollComponentV2->angularDamping->getVector3());
        }

        this->body->setType(this->physicsRagDollComponentV2->gameObjectPtr->getCategoryId());

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

        // Set material group ID (with optional custom category)
        unsigned int categoryId = this->physicsRagDollComponentV2->getOwner()->getCategoryId();
        if (false == category.empty())
        {
            categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->registerCategory(category);

            const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialIDFromCategory(category, this->physicsRagDollComponentV2->ogreNewt);
            this->body->setMaterialGroupID(materialId);
        }
        else
        {
            const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->physicsRagDollComponentV2->getOwner().get(), this->physicsRagDollComponentV2->ogreNewt);
            this->body->setMaterialGroupID(materialId);
        }

        this->body->setType(categoryId);
        this->body->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this->physicsRagDollComponentV2)));

        if (nullptr != this->parentRagBone)
        {
            this->body->setCustomForceAndTorqueCallback<PhysicsRagDollComponentV2::RagBone>(&PhysicsRagDollComponentV2::RagBone::moveCallback, this);
        }

        if (false == this->physicsRagDollComponentV2->activated->getBool())
        {
            this->body->freeze();
        }
        else
        {
            this->body->unFreeze();
        }

        this->body->setCollidable(this->physicsRagDollComponentV2->collidable->getBool());

        // Root bone: use the game object's scene node and set as physicsBody
        if (nullptr == this->parentRagBone)
        {
            this->sceneNode = this->physicsRagDollComponentV2->gameObjectPtr->getSceneNode();
            this->physicsRagDollComponentV2->physicsBody = this->body;

            this->physicsRagDollComponentV2->setPosition(this->initialBonePosition);
            this->physicsRagDollComponentV2->setOrientation(this->initialBoneOrientation);
        }
        else
        {
            // Child bones: create child scene nodes
            this->sceneNode = this->physicsRagDollComponentV2->gameObjectPtr->getSceneNode()->createChildSceneNode();
            this->sceneNode->setName(name);
            this->sceneNode->getUserObjectBindings().setUserAny(Ogre::Any(this->physicsRagDollComponentV2->gameObjectPtr.get()));
        }

        if (nullptr == this->parentRagBone)
        {
            this->physicsRagDollComponentV2->setGyroscopicTorqueEnabled(this->physicsRagDollComponentV2->gyroscopicTorque->getBool());

            this->body->setCustomForceAndTorqueCallback<PhysicsRagDollComponentV2>(&PhysicsActiveComponent::moveCallback, this->physicsRagDollComponentV2);
        }
    }

    // ============================================================================
    // RagBone destructor
    // ============================================================================

    PhysicsRagDollComponentV2::RagBone::~RagBone()
    {
        this->deleteRagBone();
    }

    // ============================================================================
    // RagBone::deleteRagBone
    // ============================================================================

    void PhysicsRagDollComponentV2::RagBone::deleteRagBone(void)
    {
        if (nullptr != this->bone)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponentV2::RagBone] Deleting ragbone: " + this->bone->getName());

            NOWA::GraphicsModule::getInstance()->removeTrackedBone(this->bone);
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
            if (this->body != this->physicsRagDollComponentV2->physicsBody && nullptr != this->parentRagBone)
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
                this->body = nullptr;
            }
        }
        if (nullptr != this->sceneNode)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->sceneNode);
            if (nullptr != this->parentRagBone)
            {
                auto sceneNode = this->physicsRagDollComponentV2->gameObjectPtr->getSceneNode();
                auto thisSceneNode = this->sceneNode;
                sceneNode->removeAndDestroyChild(thisSceneNode);
            }
            else
            {
                this->sceneNode = nullptr;
            }
        }
    }

    // ============================================================================
    // RagBone::update
    // ============================================================================

    void PhysicsRagDollComponentV2::RagBone::update(Ogre::Real dt, bool notSimulating)
    {
        if (nullptr != this->jointCompPtr && false == notSimulating)
        {
            this->jointCompPtr->update(dt);
        }
    }

    // ============================================================================
    // RagBone::moveCallback
    // ============================================================================

    void PhysicsRagDollComponentV2::RagBone::moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
    {
        Ogre::Real nearestPlanetDistance = std::numeric_limits<Ogre::Real>::max();
        GameObjectPtr nearestGravitySourceObject;

        Ogre::Vector3 wholeForce = body->getGravity();
        Ogre::Real mass = 0.0f;
        Ogre::Vector3 inertia = Ogre::Vector3::ZERO;

        body->getMassMatrix(mass, inertia);
        wholeForce *= mass;

        if (false == this->physicsRagDollComponentV2->gravitySourceCategory->getString().empty())
        {
            auto gravitySourceGameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(this->physicsRagDollComponentV2->gravitySourceCategory->getString());
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

            if (nullptr != nearestGravitySourceObject)
            {
                auto& gravitySourcePhysicsComponentPtr = NOWA::makeStrongPtr(nearestGravitySourceObject->getComponent<PhysicsComponent>());
                if (nullptr != gravitySourcePhysicsComponentPtr)
                {
                    Ogre::Vector3 directionToPlanet = this->getPosition() - gravitySourcePhysicsComponentPtr->getPosition();
                    directionToPlanet.normalise();

                    Ogre::Real gravityAcceleration = -this->physicsRagDollComponentV2->gravity->getVector3().length();

                    wholeForce = directionToPlanet * (mass * gravityAcceleration);
                }
            }
        }

        body->addForce(wholeForce);

        // Handle required velocity force command
        if (this->requiredVelocityForForceCommand.pending.load())
        {
            bool expected = false;
            if (this->requiredVelocityForForceCommand.inProgress.compare_exchange_strong(expected, true))
            {
                Ogre::Vector3 velocityToApply = this->requiredVelocityForForceCommand.vectorValue;

                Ogre::Vector3 moveForce = (velocityToApply - body->getVelocity()) * mass / timeStep;
                body->addForce(moveForce);

                this->requiredVelocityForForceCommand.pending.store(false);
                this->requiredVelocityForForceCommand.inProgress.store(false);
            }
        }

        // Handle omega force command
        if (this->omegaForceCommand.pending.load())
        {
            bool expected = false;
            if (this->omegaForceCommand.inProgress.compare_exchange_strong(expected, true))
            {
                Ogre::Vector3 omegaForceToApply = this->omegaForceCommand.vectorValue;

                body->setBodyAngularVelocity(omegaForceToApply, timeStep);

                this->omegaForceCommand.pending.store(false);
                this->omegaForceCommand.inProgress.store(false);
            }
        }
    }

    // ============================================================================
    // RagBone getters
    // ============================================================================

    const Ogre::String& PhysicsRagDollComponentV2::RagBone::getName(void) const
    {
        return this->name;
    }

    OgreNewt::Body* PhysicsRagDollComponentV2::RagBone::getBody(void)
    {
        return this->body;
    }

    Ogre::Bone* PhysicsRagDollComponentV2::RagBone::getBone(void) const
    {
        return this->bone;
    }

    PhysicsRagDollComponentV2::RagBone* PhysicsRagDollComponentV2::RagBone::getParentRagBone(void) const
    {
        return this->parentRagBone;
    }

    Ogre::Quaternion PhysicsRagDollComponentV2::RagBone::getInitialBoneOrientation(void)
    {
        return this->initialBoneOrientation;
    }

    Ogre::Vector3 PhysicsRagDollComponentV2::RagBone::getInitialBonePosition(void)
    {
        return this->initialBonePosition;
    }

    // V2: Extract skeleton-local position from bone's local space transform
    Ogre::Vector3 PhysicsRagDollComponentV2::RagBone::getWorldPosition(void)
    {
        Ogre::Vector3 pos;
        Ogre::Quaternion orient;
        PhysicsRagDollComponentV2::extractBoneDerivedTransform(this->bone, pos, orient);
        return pos;
    }

    // V2: Extract skeleton-local orientation from bone's local space transform
    Ogre::Quaternion PhysicsRagDollComponentV2::RagBone::getWorldOrientation(void)
    {
        Ogre::Vector3 pos;
        Ogre::Quaternion orient;
        PhysicsRagDollComponentV2::extractBoneDerivedTransform(this->bone, pos, orient);
        return orient;
    }

    Ogre::Vector3 PhysicsRagDollComponentV2::RagBone::getPosition(void) const
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

    Ogre::Quaternion PhysicsRagDollComponentV2::RagBone::getOrientation(void) const
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

    Ogre::Vector3 PhysicsRagDollComponentV2::RagBone::getOffset(void) const
    {
        return this->offset;
    }

    Ogre::SceneNode* PhysicsRagDollComponentV2::RagBone::getSceneNode(void) const
    {
        return this->sceneNode;
    }

    // ============================================================================
    // RagBone::attachToNode / detachFromNode
    // ============================================================================

    void PhysicsRagDollComponentV2::RagBone::attachToNode(void)
    {
        this->body->attachNode(this->sceneNode);
    }

    void PhysicsRagDollComponentV2::RagBone::detachFromNode(void)
    {
        this->body->detachNode();
    }

    // ============================================================================
    // RagBone::setOrientation / rotate / setInitialState
    // ============================================================================

    void PhysicsRagDollComponentV2::RagBone::setOrientation(const Ogre::Quaternion& orientation)
    {
        Ogre::Vector3 center = this->physicsRagDollComponentV2->ragDataList.cbegin()->ragBone->getBody()->getPosition();

        this->body->setPositionOrientation(center + orientation * this->initialBonePosition, orientation * this->initialBoneOrientation);
    }

    void PhysicsRagDollComponentV2::RagBone::rotate(const Ogre::Quaternion& rotation)
    {
        Ogre::Quaternion localOrient;
        Ogre::Vector3 localPos;
        this->body->getPositionOrientation(localPos, localOrient);

        Ogre::Vector3 center = this->physicsRagDollComponentV2->ragDataList.cbegin()->ragBone->getBody()->getPosition();
        Ogre::Vector3 offsetPos = (localPos - center);

        this->body->setPositionOrientation(center + rotation * offsetPos, rotation * localOrient);
    }

    void PhysicsRagDollComponentV2::RagBone::setInitialState(void)
    {
        Ogre::Vector3 center = this->physicsRagDollComponentV2->ragDataList.cbegin()->ragBone->getBody()->getPosition();
        if (nullptr != this->body)
        {
            this->body->setPositionOrientation(center + this->initialBonePosition, this->initialBoneOrientation);
        }

        NOWA::GraphicsModule::getInstance()->updateBoneTransform(this->bone, this->initialBonePosition, this->initialBoneOrientation);
    }

    void PhysicsRagDollComponentV2::RagBone::resetBody(void)
    {
        this->body = nullptr;
    }

    // ============================================================================
    // RagBone component / pose / joint accessors
    // ============================================================================

    PhysicsRagDollComponentV2* PhysicsRagDollComponentV2::RagBone::getPhysicsRagDollComponent(void) const
    {
        return this->physicsRagDollComponentV2;
    }

    const Ogre::Vector3 PhysicsRagDollComponentV2::RagBone::getRagPose(void)
    {
        return this->pose;
    }

    void PhysicsRagDollComponentV2::RagBone::applyPose(const Ogre::Vector3& pose)
    {
        this->pose = pose;

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

    JointCompPtr PhysicsRagDollComponentV2::RagBone::createJointComponent(const Ogre::String& type)
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
            this->jointCompPtr2->setSceneManager(this->physicsRagDollComponentV2->getOwner()->getSceneManager());
            this->jointCompPtr2->setOwner(this->physicsRagDollComponentV2->getOwner());
            return this->jointCompPtr2;
        }
        else
        {
            this->jointCompPtr = boost::make_shared<JointComponent>();
        }
        this->jointCompPtr->setType(type);
        this->jointCompPtr->setSceneManager(this->physicsRagDollComponentV2->getOwner()->getSceneManager());
        this->jointCompPtr->setOwner(this->physicsRagDollComponentV2->getOwner());
        return this->jointCompPtr;
    }

    JointCompPtr PhysicsRagDollComponentV2::RagBone::getJointComponent(void) const
    {
        return this->jointCompPtr;
    }

    JointCompPtr PhysicsRagDollComponentV2::RagBone::getSecondJointComponent(void) const
    {
        return this->jointCompPtr2;
    }

    Ogre::Vector3 PhysicsRagDollComponentV2::RagBone::getBodySize(void) const
    {
        return this->bodySize;
    }

    // ============================================================================
    // RagBone force application
    // ============================================================================

    void PhysicsRagDollComponentV2::RagBone::applyRequiredForceForVelocity(const Ogre::Vector3& velocity)
    {
        if (velocity != Ogre::Vector3::ZERO)
        {
            this->requiredVelocityForForceCommand.vectorValue = velocity;
            this->requiredVelocityForForceCommand.pending.store(true);
        }
    }

    void PhysicsRagDollComponentV2::RagBone::applyOmegaForce(const Ogre::Vector3& omegaForce)
    {
        if (omegaForce != Ogre::Vector3::ZERO)
        {
            this->omegaForceCommand.vectorValue = omegaForce;
            this->omegaForceCommand.pending.store(true);
        }
    }

    void PhysicsRagDollComponentV2::RagBone::applyOmegaForceRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength)
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

    void PhysicsRagDollComponentV2::RagBone::applyOmegaForceRotateToDirection(const Ogre::Vector3& resultDirection, Ogre::Real strength)
    {
    }

    unsigned long PhysicsRagDollComponentV2::RagBone::getJointId(void)
    {
        if (nullptr != this->jointCompPtr)
        {
            return this->jointCompPtr->getId();
        }
        return 0;
    }

}; // namespace end
