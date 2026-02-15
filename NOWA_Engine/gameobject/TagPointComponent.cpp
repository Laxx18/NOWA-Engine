#include "NOWAPrecompiled.h"
#include "TagPointComponent.h"
#include "AnimationComponent.h"
#include "JointComponents.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsActiveKinematicComponent.h"
#include "PhysicsRagDollComponent.h"
#include "main/AppStateManager.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

// V2 includes
#include "Animation/OgreBone.h"
#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // http://forums.ogre3d.org/viewtopic.php?f=4&t=84650#p522055
    // V1 approach - Slow but automatic
    class TagPointListener : public Ogre::v1::OldNode::Listener
    {
    public:
        TagPointListener(Ogre::Node* realNode, const Ogre::Vector3& offsetPosition, const Ogre::Quaternion& offsetOrientation, PhysicsActiveComponent* sourcePhysicsActiveComponent, GameObject* gameObject) :
            realNode(realNode),
            offsetPosition(offsetPosition),
            offsetOrientation(offsetOrientation),
            sourcePhysicsActiveComponent(sourcePhysicsActiveComponent),
            gameObject(gameObject),
            jointKinematicComponent(nullptr)
        {
            if (nullptr != sourcePhysicsActiveComponent)
            {
                auto sourcePhysicsActiveKinematicComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(sourcePhysicsActiveComponent);
                if (nullptr == sourcePhysicsActiveKinematicComponent)
                {
                    auto& existingJointKinematicCompPtr = NOWA::makeStrongPtr(this->sourcePhysicsActiveComponent->getOwner()->getComponent<JointKinematicComponent>());
                    if (nullptr == existingJointKinematicCompPtr)
                    {
                        boost::shared_ptr<JointKinematicComponent> jointKinematicCompPtr(boost::make_shared<JointKinematicComponent>());
                        this->sourcePhysicsActiveComponent->getOwner()->addDelayedComponent(jointKinematicCompPtr, true);
                        jointKinematicCompPtr->setOwner(this->sourcePhysicsActiveComponent->getOwner());
                        jointKinematicCompPtr->setBody(this->sourcePhysicsActiveComponent->getBody());
                        jointKinematicCompPtr->setPickingMode(4);
                        jointKinematicCompPtr->setMaxLinearAngleFriction(10000.0f, 10000.0f);
                        jointKinematicCompPtr->createJoint();
                        this->jointKinematicComponent = jointKinematicCompPtr.get();
                    }
                    else
                    {
                        this->jointKinematicComponent = existingJointKinematicCompPtr.get();
                        auto& jointCompPtr = NOWA::makeStrongPtr(this->gameObject->getComponent<JointComponent>());
                        if (nullptr != jointCompPtr)
                        {
                            this->jointKinematicComponent->connectPredecessorId(jointCompPtr->getId());
                            this->jointKinematicComponent->createJoint();
                        }
                    }
                }
            }
        }

        virtual TagPointListener::~TagPointListener()
        {
        }

        Ogre::Vector3 findVelocity(const Ogre::Vector3& aPos, const Ogre::Vector3& bPos, Ogre::Real speed)
        {
            Ogre::Vector3 disp = bPos - aPos;
            float distance = Ogre::Math::Sqrt(disp.x * disp.x + disp.y * disp.y + disp.z * disp.z);
            return disp * (speed / distance);
        }

        virtual void OldNodeUpdated(const Ogre::v1::OldNode* updatedNode) override
        {
            Ogre::Quaternion newOrientation = updatedNode->_getDerivedOrientation() * this->offsetOrientation;
            Ogre::Vector3 newPosition = (updatedNode->_getDerivedPosition() + (newOrientation * this->offsetPosition)) * this->realNode->getScale();

            NOWA::GraphicsModule::getInstance()->updateNodeTransform(this->realNode, newPosition, newOrientation);

            if (nullptr != this->sourcePhysicsActiveComponent)
            {
                auto sourcePhysicsActiveKinematicComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(sourcePhysicsActiveComponent);
                if (nullptr == sourcePhysicsActiveKinematicComponent)
                {
                    this->jointKinematicComponent->setTargetPositionRotation(this->realNode->_getDerivedPositionUpdated(), this->realNode->_getDerivedOrientationUpdated());
                }
                else
                {
                    sourcePhysicsActiveKinematicComponent->setOrientation(this->realNode->_getDerivedOrientationUpdated());
                    sourcePhysicsActiveKinematicComponent->setPosition(this->realNode->_getDerivedPositionUpdated());
                }
            }
        }

        virtual void OldNodeDestroyed(const Ogre::v1::OldNode* updatedNode) override
        {
            delete this;
        }

    private:
        Ogre::Node* realNode;
        Ogre::Vector3 offsetPosition;
        Ogre::Quaternion offsetOrientation;
        PhysicsActiveComponent* sourcePhysicsActiveComponent;
        JointKinematicComponent* jointKinematicComponent;
        GameObject* gameObject;
    };

    TagPointComponent::TagPointComponent() :
        GameObjectComponent(),
        tagPoint(nullptr),
        tagPointNode(nullptr),
        sourcePhysicsActiveComponent(nullptr),
        alreadyConnected(false),
        debugGeometryArrowNode(nullptr),
        debugGeometrySphereNode(nullptr),
        debugGeometryArrowItem(nullptr),
        debugGeometrySphereItem(nullptr),
        skeletonInstance(nullptr),
        attachedBone(nullptr),
        updateClosureId(""),
        tagPoints(new Variant(TagPointComponent::AttrTagPointName(), std::vector<Ogre::String>(), this->attributes)),
        sourceId(new Variant(TagPointComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes, true)),
        offsetPosition(new Variant(TagPointComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
        offsetOrientation(new Variant(TagPointComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes))
    {
        this->tagPoints->setDescription("If this game object uses several TagPoint-Components data may not be tagged to the same bone name");
        this->offsetOrientation->setDescription("Orientation is set in the form: (degreeX, degreeY, degreeZ)");
    }

    TagPointComponent::~TagPointComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Destructor tag point component for game object: " + this->gameObjectPtr->getName());

        this->tagPoint = nullptr;
        this->attachedBone = nullptr;
        this->skeletonInstance = nullptr;

        GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
        if (nullptr != sourceGameObjectPtr)
        {
            sourceGameObjectPtr->setConnectedGameObject(boost::weak_ptr<GameObject>());
        }
        this->destroyDebugData();
    }

    bool TagPointComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TagPointName")
        {
            this->tagPoints->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SourceId")
        {
            this->sourceId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
        {
            this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
        {
            this->offsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    GameObjectCompPtr TagPointComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        TagPointCompPtr clonedCompPtr(boost::make_shared<TagPointComponent>());

        clonedCompPtr->setTagPointName(this->tagPoints->getListSelectedValue());
        clonedCompPtr->setSourceId(this->sourceId->getULong());
        clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
        clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool TagPointComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Init tag point component for game object: " + this->gameObjectPtr->getName());

        this->gameObjectPtr->setDynamic(true);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

        // Try v1::Entity first
        Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
        if (nullptr != entity)
        {
            this->initializeV1Entity(entity);
        }
        else
        {
            // Try v2::Item
            Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
            if (nullptr != item)
            {
                this->initializeV2Item(item);
            }
        }

        return true;
    }

    void TagPointComponent::initializeV1Entity(Ogre::v1::Entity* entity)
    {
        Ogre::v1::Skeleton* skeleton = entity->getSkeleton();
        if (nullptr != skeleton)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] List all bones (v1) for mesh '" + entity->getMesh()->getName() + "':");

            std::vector<Ogre::String> tagPointNames;
            unsigned short numBones = entity->getSkeleton()->getNumBones();

            for (unsigned short iBone = 0; iBone < numBones; iBone++)
            {
                Ogre::v1::OldBone* bone = entity->getSkeleton()->getBone(iBone);
                if (nullptr == bone)
                {
                    continue;
                }

                unsigned short numChildren = bone->numChildren();
                if (numChildren == 0)
                {
                    bool unique = true;
                    for (size_t i = 0; i < tagPointNames.size(); i++)
                    {
                        if (tagPointNames[i] == bone->getName())
                        {
                            unique = false;
                            break;
                        }
                    }
                    if (true == unique)
                    {
                        tagPointNames.emplace_back(bone->getName());
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Bone name: " + bone->getName());
                    }
                }
                else
                {
                    bool unique = true;
                    for (size_t i = 0; i < tagPointNames.size(); i++)
                    {
                        if (tagPointNames[i] == bone->getName())
                        {
                            unique = false;
                            break;
                        }
                    }
                    if (true == unique)
                    {
                        tagPointNames.emplace_back(bone->getName());
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Bone name: " + bone->getName());
                    }
                }
            }

            this->tagPoints->setValue(tagPointNames);
        }

        this->setTagPointName(this->tagPoints->getListSelectedValue());
    }

    void TagPointComponent::initializeV2Item(Ogre::Item* item)
    {
        this->skeletonInstance = item->getSkeletonInstance();
        if (nullptr != this->skeletonInstance)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] List all bones (v2) for mesh '" + item->getMesh()->getName() + "':");

            std::vector<Ogre::String> tagPointNames;
            unsigned short numBones = this->skeletonInstance->getNumBones();

            for (unsigned short iBone = 0; iBone < numBones; iBone++)
            {
                Ogre::Bone* bone = this->skeletonInstance->getBone(iBone);
                if (nullptr == bone)
                {
                    continue;
                }

                // In v2, check if bone has children by checking child bones
                bool isLeaf = true;
                for (unsigned short j = 0; j < numBones; j++)
                {
                    if (iBone != j) // Fixed: was 'i', should be 'iBone'
                    {
                        Ogre::Bone* potentialChild = this->skeletonInstance->getBone(j);
                        if (potentialChild->getParent() == bone)
                        {
                            isLeaf = false;
                            break;
                        }
                    }
                }

                // Add all bones (both leaf and non-leaf) to the list
                Ogre::String boneName = bone->getName();
                bool unique = true;
                for (size_t i = 0; i < tagPointNames.size(); i++)
                {
                    if (tagPointNames[i] == boneName)
                    {
                        unique = false;
                        break;
                    }
                }
                if (true == unique)
                {
                    tagPointNames.emplace_back(boneName);
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Bone name: " + boneName);
                }
            }

            this->tagPoints->setValue(tagPointNames);
        }

        this->setTagPointName(this->tagPoints->getListSelectedValue());
    }

    bool TagPointComponent::connect(void)
    {
        if (false == alreadyConnected)
        {
            // Try v1::Entity first
            Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
            if (nullptr != entity)
            {
                this->connectV1Entity(entity);
            }
            else
            {
                // Try v2::Item
                Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
                if (nullptr != item)
                {
                    this->connectV2Item(item);
                }
            }
        }
        return true;
    }

    void TagPointComponent::connectV1Entity(Ogre::v1::Entity* entity)
    {
        ENQUEUE_RENDER_COMMAND_MULTI("TagPointComponent::connectV1Entity", _1(entity), {
            Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
            if (nullptr != oldSkeletonInstance)
            {
                GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
                if (nullptr == sourceGameObjectPtr)
                {
                    return;
                }

                sourceGameObjectPtr->setConnectedGameObject(this->gameObjectPtr);

                auto& physicsActiveCompPtr = NOWA::makeStrongPtr(sourceGameObjectPtr->getComponent<PhysicsActiveComponent>());
                if (nullptr != physicsActiveCompPtr)
                {
                    this->sourcePhysicsActiveComponent = physicsActiveCompPtr.get();
                }

                this->tagPointNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
                this->tagPointNode->setScale(this->gameObjectPtr->getSceneNode()->getScale());
                this->tagPointNode->setPosition(this->tagPointNode->getPosition() * this->tagPointNode->getScale());

                std::vector<Ogre::MovableObject*> movableObjects;
                Ogre::SceneNode* sourceSceneNode = sourceGameObjectPtr->getSceneNode();
                auto& it = sourceSceneNode->getAttachedObjectIterator();

                while (it.hasMoreElements())
                {
                    Ogre::MovableObject* movableObject = it.getNext();
                    movableObjects.emplace_back(movableObject);
                }

                for (size_t i = 0; i < movableObjects.size(); i++)
                {
                    movableObjects[i]->detachFromParent();
                    this->tagPointNode->attachObject(movableObjects[i]);
                }

                if (nullptr == this->tagPoint)
                {
                    this->tagPoint = oldSkeletonInstance->createTagPointOnBone(oldSkeletonInstance->getBone(this->tagPoints->getListSelectedValue()));
                    this->tagPoint->setScale(this->gameObjectPtr->getSceneNode()->getScale());
                }

                this->tagPoint->setListener(new TagPointListener(this->tagPointNode, this->offsetPosition->getVector3(), MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()), this->sourcePhysicsActiveComponent,
                    this->gameObjectPtr.get()));

                this->alreadyConnected = true;
            }
        });
    }

    void TagPointComponent::connectV2Item(Ogre::Item* item)
    {
        ENQUEUE_RENDER_COMMAND_MULTI("TagPointComponent::connectV2Item", _1(item), {
            if (nullptr != this->skeletonInstance)
            {
                GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
                if (nullptr == sourceGameObjectPtr)
                {
                    return;
                }

                sourceGameObjectPtr->setConnectedGameObject(this->gameObjectPtr);

                auto& physicsActiveCompPtr = NOWA::makeStrongPtr(sourceGameObjectPtr->getComponent<PhysicsActiveComponent>());
                if (nullptr != physicsActiveCompPtr)
                {
                    this->sourcePhysicsActiveComponent = physicsActiveCompPtr.get();
                }

                this->tagPointNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
                this->tagPointNode->setScale(this->gameObjectPtr->getSceneNode()->getScale());
                this->tagPointNode->setPosition(this->tagPointNode->getPosition() * this->tagPointNode->getScale());

                std::vector<Ogre::MovableObject*> movableObjects;
                Ogre::SceneNode* sourceSceneNode = sourceGameObjectPtr->getSceneNode();
                auto& it = sourceSceneNode->getAttachedObjectIterator();

                while (it.hasMoreElements())
                {
                    Ogre::MovableObject* movableObject = it.getNext();
                    movableObjects.emplace_back(movableObject);
                }

                for (size_t i = 0; i < movableObjects.size(); i++)
                {
                    movableObjects[i]->detachFromParent();
                    this->tagPointNode->attachObject(movableObjects[i]);
                }

                // Get the bone by name
                Ogre::IdString boneIdString(this->tagPoints->getListSelectedValue());
                this->attachedBone = this->skeletonInstance->getBone(boneIdString);

                if (nullptr != this->attachedBone)
                {
                    // Create tracked closure to update tagpoint node position/orientation each frame
                    this->updateClosureId = this->gameObjectPtr->getName() + this->getClassName() + "::updateTagPoint" + Ogre::StringConverter::toString(this->index);

                    auto closureFunction = [this](Ogre::Real renderDt)
                    {
                        this->updateV2TagPointTransform();
                    };

                    NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->updateClosureId, closureFunction, false);
                }

                this->alreadyConnected = true;
            }
        });
    }

    void TagPointComponent::updateV2TagPointTransform(void)
    {
        if (nullptr == this->attachedBone || nullptr == this->tagPointNode || nullptr == this->skeletonInstance)
        {
            return;
        }

        // Extract bone transform in skeleton-local space
        Ogre::Vector3 boneSkeletonLocalPos;
        Ogre::Quaternion boneSkeletonLocalOrient;
        this->extractBoneDerivedTransform(this->attachedBone, boneSkeletonLocalPos, boneSkeletonLocalOrient);

        // Apply offset
        Ogre::Quaternion offsetQuat = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());
        Ogre::Quaternion finalOrient = boneSkeletonLocalOrient * offsetQuat;
        Ogre::Vector3 finalPos = boneSkeletonLocalPos + (finalOrient * this->offsetPosition->getVector3());

        // Transform from skeleton-local to world space using parent Item's scene node
        Ogre::SceneNode* parentSceneNode = this->gameObjectPtr->getSceneNode();
        Ogre::Vector3 worldPos = parentSceneNode->_getDerivedPosition() + (parentSceneNode->_getDerivedOrientation() * (finalPos * parentSceneNode->_getDerivedScale()));
        Ogre::Quaternion worldOrient = parentSceneNode->_getDerivedOrientation() * finalOrient;

        // Update the tagpoint node
        NOWA::GraphicsModule::getInstance()->updateNodeTransform(this->tagPointNode, worldPos, worldOrient);

        // Update physics if needed
        if (nullptr != this->sourcePhysicsActiveComponent)
        {
            auto sourcePhysicsActiveKinematicComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(sourcePhysicsActiveComponent);
            if (nullptr == sourcePhysicsActiveKinematicComponent)
            {
                // Use joint kinematic approach (if jointKinematicComponent exists)
                // This would need to be set up similar to v1 approach if needed
            }
            else
            {
                sourcePhysicsActiveKinematicComponent->setOrientation(this->tagPointNode->_getDerivedOrientationUpdated());
                sourcePhysicsActiveKinematicComponent->setPosition(this->tagPointNode->_getDerivedPositionUpdated());
            }
        }
    }

    void TagPointComponent::extractBoneDerivedTransform(Ogre::Bone* bone, Ogre::Vector3& outPos, Ogre::Quaternion& outOrient)
    {
        const Ogre::SimpleMatrixAf4x3& t = bone->_getLocalSpaceTransform();
        Ogre::Matrix4 mat4;
        t.store(&mat4);
        Ogre::Vector3 scale;
        mat4.decomposition(outPos, scale, outOrient);
    }

    bool TagPointComponent::disconnect(void)
    {
        this->resetTagPoint();
        return true;
    }

    void TagPointComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();
        this->resetTagPoint();
    }

    void TagPointComponent::onOtherComponentRemoved(unsigned int index)
    {
        auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
        if (nullptr != gameObjectCompPtr)
        {
            auto& physicsRagdollCompPtr = boost::dynamic_pointer_cast<PhysicsRagDollComponent>(gameObjectCompPtr);
            auto& animationCompPtr = boost::dynamic_pointer_cast<AnimationComponent>(gameObjectCompPtr);
            if (nullptr != physicsRagdollCompPtr || nullptr != animationCompPtr)
            {
                this->resetTagPoint();
            }
        }
    }

    bool TagPointComponent::onCloned(void)
    {
        GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->sourceId->getULong());
        if (nullptr != sourceGameObjectPtr)
        {
            this->setSourceId(sourceGameObjectPtr->getId());
        }
        else
        {
            this->setSourceId(0);
        }
        return true;
    }

    void TagPointComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating)
        {
            if (true == this->bShowDebugData && nullptr != this->debugGeometryArrowNode)
            {
                Ogre::Vector3 worldPosition;
                Ogre::Quaternion worldOrientation;

                // Try v1 approach first
                if (nullptr != this->tagPoint)
                {
                    worldPosition = this->tagPoint->getParent()->_getDerivedPosition();
                    worldOrientation = this->tagPoint->getParent()->_getDerivedOrientation();

                    Ogre::Node* parentNode = this->gameObjectPtr->getMovableObject()->getParentNode();
                    Ogre::SceneNode* sceneNode = this->gameObjectPtr->getMovableObject()->getParentSceneNode();
                    while (parentNode != nullptr)
                    {
                        if (parentNode != sceneNode)
                        {
                            worldPosition = ((Ogre::v1::TagPoint*)parentNode)->_getFullLocalTransform() * worldPosition;
                            parentNode = ((Ogre::v1::TagPoint*)parentNode)->getParentEntity()->getParentNode();
                            worldOrientation = ((Ogre::v1::TagPoint*)parentNode)->_getDerivedOrientation() * worldOrientation;
                        }
                        else
                        {
                            worldPosition = parentNode->_getFullTransform() * worldPosition;
                            worldOrientation = parentNode->_getDerivedOrientation() * worldOrientation;
                            break;
                        }
                    }
                }
                else if (nullptr != this->tagPointNode)
                {
                    // V2 approach - just use the tagpoint node's derived transform
                    worldPosition = this->tagPointNode->_getDerivedPosition();
                    worldOrientation = this->tagPointNode->_getDerivedOrientation();
                }

                NOWA::GraphicsModule::getInstance()->updateNodeTransform(this->debugGeometryArrowNode, worldPosition, worldOrientation);
            }
        }
    }

    void TagPointComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (TagPointComponent::AttrTagPointName() == attribute->getName())
        {
            this->setTagPointName(attribute->getListSelectedValue());
        }
        else if (TagPointComponent::AttrSourceId() == attribute->getName())
        {
            this->setSourceId(attribute->getULong());
        }
        else if (TagPointComponent::AttrOffsetPosition() == attribute->getName())
        {
            this->setOffsetPosition(attribute->getVector3());
        }
        else if (TagPointComponent::AttrOffsetOrientation() == attribute->getName())
        {
            this->setOffsetOrientation(attribute->getVector3());
        }
    }

    void TagPointComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TagPointName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tagPoints->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SourceId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sourceId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetPosition->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetOrientation->getVector3())));
        propertiesXML->append_node(propertyXML);
    }

    void TagPointComponent::showDebugData(void)
    {
        GameObjectComponent::showDebugData();
        if (true == this->bShowDebugData)
        {
            this->destroyDebugData();
            this->generateDebugData();
        }
        else
        {
            this->destroyDebugData();
        }
    }

    Ogre::String TagPointComponent::getClassName(void) const
    {
        return "TagPointComponent";
    }

    Ogre::String TagPointComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void TagPointComponent::setActivated(bool activated)
    {
    }

    void TagPointComponent::setTagPointName(const Ogre::String& tagPointName)
    {
        this->tagPoints->setListSelectedValue(tagPointName);

        if (nullptr == this->gameObjectPtr)
        {
            return;
        }

        // Try v1::Entity first
        Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
        if (nullptr != entity)
        {
            this->setTagPointNameV1(entity, tagPointName);
        }
        else
        {
            // Try v2::Item
            Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
            if (nullptr != item)
            {
                this->setTagPointNameV2(item, tagPointName);
            }
        }

        if (true == this->bShowDebugData)
        {
            this->destroyDebugData();
            this->generateDebugData();
        }
    }

    void TagPointComponent::setTagPointNameV1(Ogre::v1::Entity* entity, const Ogre::String& tagPointName)
    {
        ENQUEUE_RENDER_COMMAND_MULTI("TagPointComponent::setTagPointNameV1", _2(entity, tagPointName), {
            Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
            if (nullptr != oldSkeletonInstance)
            {
                if (nullptr != this->tagPoint)
                {
                    oldSkeletonInstance->freeTagPoint(this->tagPoint);
                    this->tagPoint = nullptr;
                }

                this->tagPoint = oldSkeletonInstance->createTagPointOnBone(oldSkeletonInstance->getBone(tagPointName));
                this->tagPoint->setScale(this->gameObjectPtr->getSceneNode()->getScale());
            }
        });
    }

    void TagPointComponent::setTagPointNameV2(Ogre::Item* item, const Ogre::String& tagPointName)
    {
        if (nullptr != this->skeletonInstance)
        {
            Ogre::IdString boneIdString(tagPointName);
            this->attachedBone = this->skeletonInstance->getBone(boneIdString);
        }
    }

    Ogre::String TagPointComponent::getTagPointName(void) const
    {
        return this->tagPoints->getListSelectedValue();
    }

    void TagPointComponent::setSourceId(unsigned long sourceId)
    {
        this->sourceId->setValue(sourceId);
    }

    unsigned long TagPointComponent::getSourceId(void) const
    {
        return this->sourceId->getULong();
    }

    void TagPointComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
    {
        this->offsetPosition->setValue(offsetPosition);
    }

    Ogre::Vector3 TagPointComponent::getOffsetPosition(void) const
    {
        return this->offsetPosition->getVector3();
    }

    void TagPointComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
    {
        this->offsetOrientation->setValue(offsetOrientation);
    }

    Ogre::Vector3 TagPointComponent::getOffsetOrientation(void) const
    {
        return this->offsetOrientation->getVector3();
    }

    Ogre::v1::TagPoint* TagPointComponent::getTagPoint(void) const
    {
        return this->tagPoint;
    }

    Ogre::SceneNode* TagPointComponent::getTagPointNode(void) const
    {
        return this->tagPointNode;
    }

    void TagPointComponent::generateDebugData(void)
    {
        // Generate debug visualization for both v1 and v2 paths
        bool canGenerateDebug = (nullptr != this->tagPoint) || (nullptr != this->attachedBone && nullptr != this->tagPointNode);

        if (nullptr == this->debugGeometryArrowNode && canGenerateDebug)
        {
            ENQUEUE_RENDER_COMMAND("TagPointComponent::generateDebugData", {
                this->debugGeometryArrowNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
                this->debugGeometryArrowNode->setName("tagPointComponentArrowNode");
                this->debugGeometrySphereNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
                this->debugGeometrySphereNode->setName("tagPointComponentSphereNode");

                Ogre::Vector3 worldPosition;
                Ogre::Quaternion worldOrientation;

                // Get position based on v1 or v2
                if (nullptr != this->tagPoint)
                {
                    // V1 path
                    worldPosition = this->tagPoint->getParent()->_getDerivedPosition();
                    worldOrientation = this->tagPoint->getParent()->_getDerivedOrientation();

                    Ogre::Node* parentNode = this->gameObjectPtr->getMovableObject()->getParentNode();
                    Ogre::SceneNode* sceneNode = this->gameObjectPtr->getMovableObject()->getParentSceneNode();
                    while (parentNode != nullptr)
                    {
                        if (parentNode != sceneNode)
                        {
                            worldPosition = ((Ogre::v1::TagPoint*)parentNode)->_getFullLocalTransform() * worldPosition;
                            parentNode = ((Ogre::v1::TagPoint*)parentNode)->getParentEntity()->getParentNode();
                            worldOrientation = ((Ogre::v1::TagPoint*)parentNode)->_getDerivedOrientation() * worldOrientation;
                        }
                        else
                        {
                            worldPosition = parentNode->_getFullTransform() * worldPosition;
                            worldOrientation = parentNode->_getDerivedOrientation() * worldOrientation;
                            break;
                        }
                    }
                }
                else if (nullptr != this->tagPointNode)
                {
                    // V2 path
                    worldPosition = this->tagPointNode->_getDerivedPosition();
                    worldOrientation = this->tagPointNode->_getDerivedOrientation();
                }

                this->debugGeometryArrowNode->setPosition(worldPosition);
                this->debugGeometryArrowNode->setOrientation(worldOrientation);
                this->debugGeometryArrowNode->setScale(0.05f, 0.05f, 0.025f);
                this->debugGeometryArrowItem = this->gameObjectPtr->getSceneManager()->createItem("Arrow.mesh");
                this->debugGeometryArrowItem->setName("tagPointComponentArrowEntity");
                this->debugGeometryArrowItem->setDatablock("BaseYellowLine");
                this->debugGeometryArrowItem->setQueryFlags(0 << 0);
                this->debugGeometryArrowItem->setCastShadows(false);
                this->debugGeometryArrowNode->attachObject(this->debugGeometryArrowItem);

                this->debugGeometrySphereNode->setPosition(worldPosition);
                this->debugGeometrySphereNode->setScale(0.05f, 0.05f, 0.05f);
                this->debugGeometrySphereItem = this->gameObjectPtr->getSceneManager()->createItem("gizmosphere.mesh");
                this->debugGeometrySphereItem->setName("tagPointComponentSphereEntity");
                this->debugGeometrySphereItem->setDatablock("BaseYellowLine");
                this->debugGeometrySphereItem->setQueryFlags(0 << 0);
                this->debugGeometrySphereItem->setCastShadows(false);
                this->debugGeometrySphereNode->attachObject(this->debugGeometrySphereItem);
            });
        }
    }

    void TagPointComponent::destroyDebugData(void)
    {
        if (nullptr != this->debugGeometryArrowNode)
        {
            GraphicsModule::getInstance()->removeTrackedNode(this->debugGeometryArrowNode);
            GraphicsModule::getInstance()->removeTrackedNode(this->debugGeometrySphereNode);

            ENQUEUE_RENDER_COMMAND("TagPointComponent::destroyDebugData", {
                this->debugGeometryArrowNode->detachAllObjects();
                this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryArrowNode);
                this->debugGeometryArrowNode = nullptr;
                this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryArrowItem);
                this->debugGeometryArrowItem = nullptr;

                this->debugGeometrySphereNode->detachAllObjects();
                this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometrySphereNode);
                this->debugGeometrySphereNode = nullptr;
                this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometrySphereItem);
                this->debugGeometrySphereItem = nullptr;
            });
        }
    }

    void TagPointComponent::resetTagPoint(void)
    {
        // Try v1 first
        Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
        if (nullptr != entity && nullptr != this->tagPoint)
        {
            this->resetTagPointV1(entity);
        }
        else
        {
            // Try v2
            Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
            if (nullptr != item && nullptr != this->attachedBone)
            {
                this->resetTagPointV2(item);
            }
        }
    }

    void TagPointComponent::resetTagPointV1(Ogre::v1::Entity* entity)
    {
        ENQUEUE_RENDER_COMMAND_MULTI("TagPointComponent::resetTagPointV1", _1(entity), {
            Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
            if (nullptr != oldSkeletonInstance)
            {
                if (nullptr != this->tagPoint)
                {
                    oldSkeletonInstance->freeTagPoint(this->tagPoint);
                    Ogre::v1::OldNode::Listener* nodeListener = this->tagPoint->getListener();
                    this->tagPoint->setListener(nullptr);
                    delete nodeListener;
                    this->tagPoint = nullptr;
                }
            }

            GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
            if (nullptr != sourceGameObjectPtr)
            {
                sourceGameObjectPtr->setConnectedGameObject(boost::weak_ptr<GameObject>());

                if (nullptr != this->tagPointNode)
                {
                    std::vector<Ogre::MovableObject*> movableObjects;
                    auto& it = this->tagPointNode->getAttachedObjectIterator();
                    while (it.hasMoreElements())
                    {
                        Ogre::MovableObject* movableObject = it.getNext();
                        movableObjects.emplace_back(movableObject);
                    }
                    for (size_t i = 0; i < movableObjects.size(); i++)
                    {
                        movableObjects[i]->detachFromParent();
                        sourceGameObjectPtr->getSceneNode()->attachObject(movableObjects[i]);
                    }
                }
            }

            if (nullptr != this->tagPointNode)
            {
                GraphicsModule::getInstance()->removeTrackedNode(this->tagPointNode);
                this->tagPointNode->removeAndDestroyAllChildren();
                this->gameObjectPtr->getSceneManager()->destroySceneNode(this->tagPointNode);
                this->tagPointNode = nullptr;
            }

            this->alreadyConnected = false;
        });
    }

    void TagPointComponent::resetTagPointV2(Ogre::Item* item)
    {
        ENQUEUE_RENDER_COMMAND_MULTI("TagPointComponent::resetTagPointV2", _1(item), {
            // Remove the tracked closure
            if (this->updateClosureId.length() > 0)
            {
                NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->updateClosureId);
                this->updateClosureId = "";
            }

            this->attachedBone = nullptr;

            GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
            if (nullptr != sourceGameObjectPtr)
            {
                sourceGameObjectPtr->setConnectedGameObject(boost::weak_ptr<GameObject>());

                if (nullptr != this->tagPointNode)
                {
                    std::vector<Ogre::MovableObject*> movableObjects;
                    auto& it = this->tagPointNode->getAttachedObjectIterator();
                    while (it.hasMoreElements())
                    {
                        Ogre::MovableObject* movableObject = it.getNext();
                        movableObjects.emplace_back(movableObject);
                    }
                    for (size_t i = 0; i < movableObjects.size(); i++)
                    {
                        movableObjects[i]->detachFromParent();
                        sourceGameObjectPtr->getSceneNode()->attachObject(movableObjects[i]);
                    }
                }
            }

            if (nullptr != this->tagPointNode)
            {
                GraphicsModule::getInstance()->removeTrackedNode(this->tagPointNode);
                this->tagPointNode->removeAndDestroyAllChildren();
                this->gameObjectPtr->getSceneManager()->destroySceneNode(this->tagPointNode);
                this->tagPointNode = nullptr;
            }

            this->alreadyConnected = false;
        });
    }

}; // namespace end