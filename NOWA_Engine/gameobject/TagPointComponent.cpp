#include "NOWAPrecompiled.h"
#include "TagPointComponent.h"
#include "AnimationComponentV2.h"
#include "JointComponents.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsActiveKinematicComponent.h"
#include "PhysicsRagDollComponentV2.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

// V2 includes
#include "Animation/OgreBone.h"
#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"
#include "Animation/OgreTagPoint2.h"

namespace
{
    // Extracts a bone's LOCAL-space position/orientation (i.e. relative to
    // the skeleton root, ignoring the owning entity's own scene node
    // transform). Scale is intentionally discarded here, mirroring
    // PhysicsRagDollComponentV2::extractBoneDerivedTransform, since bones
    // in this engine are not expected to carry their own non-uniform scale.
    void extractBoneLocalTransform(Ogre::Bone* bone, Ogre::Vector3& outPos, Ogre::Quaternion& outOrient)
    {
        // Make sure the bone's cached transform is not stale before we
        // read it (Ogre::Bone does not have Node-style _getDerivedXyzUpdated()
        // methods, this is the equivalent for bones).
        bone->_getFullTransformUpdated();

        const Ogre::SimpleMatrixAf4x3& t = bone->_getLocalSpaceTransform();

        Ogre::Matrix4 mat4;
        t.store(&mat4);

        Ogre::Vector3 scale;
        mat4.decomposition(outPos, scale, outOrient);
    }
}

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
                    auto existingJointKinematicCompPtr = NOWA::makeStrongPtr(this->sourcePhysicsActiveComponent->getOwner()->getComponent<JointKinematicComponent>());
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
                        auto jointCompPtr = NOWA::makeStrongPtr(this->gameObject->getComponent<JointComponent>());
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
                    // Same reason as in updateV2PhysicsFromTagPoint(): a KinematicBody is only
                    // moved by setKinematicPositionOrientation(), and both values must go in
                    // with one call.
                    sourcePhysicsActiveKinematicComponent->setKinematicPositionOrientation(this->realNode->_getDerivedPositionUpdated(), this->realNode->_getDerivedOrientationUpdated());
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
        tagPointNode(nullptr),
        sourcePhysicsActiveComponent(nullptr),
        alreadyConnected(false),
        debugGeometryArrowNode(nullptr),
        debugGeometrySphereNode(nullptr),
        debugGeometryArrowItem(nullptr),
        debugGeometrySphereItem(nullptr),
        skeletonInstance(nullptr),
        attachedBone(nullptr),
        tagPointV2(nullptr),
        updateClosureId(""),
        tagPoints(new Variant(TagPointComponent::AttrTagPointName(), std::vector<Ogre::String>(), this->attributes)),
        sourceId(new Variant(TagPointComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes, true)),
        offsetPosition(new Variant(TagPointComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
        offsetOrientation(new Variant(TagPointComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
        useBakedOffset(new Variant(TagPointComponent::AttrUseBakedOffset(), true, this->attributes)),
        bakeOffsetAction(new Variant(TagPointComponent::AttrBakeOffsetAction(), "Bake Offset Now", this->attributes))
    {
        // Attention: the default is ON, but only for a component that is NEWLY ADDED in the
        // editor. init() switches it off before parsing, so a scene that was saved before this
        // attribute existed keeps the old behaviour and nothing in it moves.
        this->bakeOffsetAction->addUserData(GameObject::AttrActionExec());
        this->bakeOffsetAction->addUserData(GameObject::AttrActionExecId(), "TagPointComponent.BakeOffset");
        this->bakeOffsetAction->setDescription("Freezes where the source currently hangs into 'Offset Position' and 'Offset Orientation' and switches 'Use Baked Offset' on. "
                                               "Press this after moving the source, then save the scene.");

        this->tagPoints->setDescription("If this game object uses several TagPoint-Components data may not be tagged to the same bone name");
        this->offsetOrientation->setDescription("Orientation is set in the form: (degreeX, degreeY, degreeZ)");
        this->useBakedOffset->setDescription("If on, the offset IS the attachment transform in the bone's local space and nothing is derived from world transforms at connect time. "
                                             "Use bakeOffset() once to fill the offset in, then save the scene. If off, the offset is added on top of wherever the source happens to "
                                             "sit relative to the bone when this component connects - which breaks as soon as the character is repositioned before that happens.");
    }

    TagPointComponent::~TagPointComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Destructor tag point component for game object: " + this->gameObjectPtr->getName());

        this->tagPointV2 = nullptr;
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

        // Attention: switched off BEFORE parsing, on purpose.
        //
        // The constructor defaults it to ON so that a component added in the editor is robust
        // from the start. A scene saved before this attribute existed has no 'UseBakedOffset'
        // property at all, so the block further down never fires - and without this line such a
        // scene would silently come up with baking enabled and an offset of all zeroes, which
        // would drop the source right onto the bone. Scenes saved WITH the property overwrite
        // this again a few lines later.
        this->useBakedOffset->setValue(false);

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
        // Attention: read with a default of false, so every scene that was saved before this
        // attribute existed keeps the old derive-at-connect behaviour and nothing moves.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseBakedOffset")
        {
            this->useBakedOffset->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
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
        clonedCompPtr->setUseBakedOffset(this->useBakedOffset->getBool());

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

        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr != item)
        {
            this->initializeV2Item(item);
        }

        return true;
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
        GameObjectComponent::connect();

        if (false == alreadyConnected)
        {
            Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
            if (nullptr != item)
            {
                this->connectV2Item(item);
            }
        }
        return true;
    }

    void TagPointComponent::connectV2Item(Ogre::Item* item)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, item]()
        {
            // TEMPORARY DIAGNOSTICS - remove once the kinematic contact is understood.
            // Every branch below used to fail silently.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPoint-DIAG] connectV2Item RUNNING for: " + this->gameObjectPtr->getName() + " skeletonInstance: " + Ogre::String(nullptr != this->skeletonInstance ? "ok" : "NULL") +
                                                                                    " sourceId: " + Ogre::StringConverter::toString(this->sourceId->getULong()) + " boneName: '" + this->tagPoints->getListSelectedValue() + "'");

            if (nullptr != this->skeletonInstance)
            {
                GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
                if (nullptr == sourceGameObjectPtr)
                {
                    // TEMPORARY DIAGNOSTICS
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPoint-DIAG] ABORTED: no source game object for id: " + Ogre::StringConverter::toString(this->sourceId->getULong()));
                    return;
                }

                sourceGameObjectPtr->setConnectedGameObject(this->gameObjectPtr);

                auto physicsActiveCompPtr = NOWA::makeStrongPtr(sourceGameObjectPtr->getComponent<PhysicsActiveComponent>());
                if (nullptr != physicsActiveCompPtr)
                {
                    this->sourcePhysicsActiveComponent = physicsActiveCompPtr.get();
                }

                // TEMPORARY DIAGNOSTICS - without a source physics component the update
                // closure further down is never registered, and then nothing ever drives the
                // body. The dynamic_cast result decides which of the two drive paths is taken.
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPoint-DIAG] source: " + sourceGameObjectPtr->getName() +
                                                                                        " physicsActiveComponent: " + Ogre::String(nullptr != this->sourcePhysicsActiveComponent ? "FOUND" : "NULL") +
                                                                                        " isKinematic: " + Ogre::String(nullptr != dynamic_cast<PhysicsActiveKinematicComponent*>(this->sourcePhysicsActiveComponent) ? "YES" : "no"));

                // Resolve the bone by name
                Ogre::IdString boneIdString(this->tagPoints->getListSelectedValue());
                this->attachedBone = this->skeletonInstance->getBone(boneIdString);

                // TEMPORARY DIAGNOSTICS
                if (nullptr == this->attachedBone)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPoint-DIAG] ABORTED: bone '" + this->tagPoints->getListSelectedValue() + "' NOT FOUND on the skeleton.");
                }

                if (nullptr != this->attachedBone)
                {
                    Ogre::SceneNode* characterSceneNode = this->gameObjectPtr->getSceneNode();
                    Ogre::SceneNode* sourceSceneNode = sourceGameObjectPtr->getSceneNode();

                    // Capture the shield's CURRENT world transform before it gets
                    // detached from its own scene node. This is exactly the pose
                    // we want to preserve once it hangs from the bone.
                    Ogre::Vector3 sourceWorldPosition = sourceSceneNode->_getDerivedPositionUpdated();
                    Ogre::Quaternion sourceWorldOrientation = sourceSceneNode->_getDerivedOrientationUpdated();
                    Ogre::Vector3 sourceWorldScale = sourceSceneNode->_getDerivedScale();

                    // Character's (the skeleton owner's) current world transform.
                    Ogre::Vector3 characterWorldPosition = characterSceneNode->_getDerivedPositionUpdated();
                    Ogre::Quaternion characterWorldOrientation = characterSceneNode->_getDerivedOrientationUpdated();
                    Ogre::Vector3 characterWorldScale = characterSceneNode->_getDerivedScale();

                    // Ogre::Bone is NOT a generic Ogre::Node, so it has no
                    // _getDerivedPositionUpdated()/_getDerivedOrientationUpdated()/
                    // _getDerivedScale(). We get its LOCAL-space transform (relative
                    // to the skeleton root) instead, and combine it manually with
                    // the character's world transform to get the bone's world transform.
                    Ogre::Vector3 boneLocalPosition;
                    Ogre::Quaternion boneLocalOrientation;
                    extractBoneLocalTransform(this->attachedBone, boneLocalPosition, boneLocalOrientation);

                    Ogre::Vector3 boneWorldPosition = characterWorldOrientation * (boneLocalPosition * characterWorldScale) + characterWorldPosition;
                    Ogre::Quaternion boneWorldOrientation = characterWorldOrientation * boneLocalOrientation;
                    Ogre::Vector3 boneWorldScale = characterWorldScale; // bones don't carry their own scale here

                    Ogre::Quaternion boneWorldOrientationInverse = boneWorldOrientation.Inverse();

                    // Base local transform: keeps the shield exactly where it
                    // currently is/looks in the world, relative to the bone.
                    Ogre::Vector3 baseLocalPosition = boneWorldOrientationInverse * ((sourceWorldPosition - boneWorldPosition) / boneWorldScale);
                    Ogre::Quaternion baseLocalOrientation = boneWorldOrientationInverse * sourceWorldOrientation;
                    Ogre::Vector3 baseLocalScale = sourceWorldScale / boneWorldScale;

                    // User-specified additional offset, applied on top of the base transform
                    // Not const: the automatic bake further down may replace them.
                    Ogre::Vector3 offsetPos = this->offsetPosition->getVector3();
                    Ogre::Quaternion offsetQuat = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());

                    // -------------------------------------------------------
                    // Create the real v2 TagPoint and parent it to the bone.
                    // TagPoint is a SceneNode subclass so we can reuse
                    // tagPointNode for all downstream SceneNode-based code.
                    // -------------------------------------------------------
                    this->tagPointV2 = this->gameObjectPtr->getSceneManager()->createTagPoint();

                    // Keep the shield's current world position/orientation and only
                    // THEN add the user-specified offset on top of it (instead of
                    // using the offset as the sole/absolute local transform, which
                    // caused the visible twist).
                    // Automatic one time bake.
                    //
                    // 'Use Baked Offset' is on but nothing has ever been stored, which is the
                    // state of a freshly placed source: the values are still at their defaults.
                    // The transform that was just derived above is exactly what has to be frozen,
                    // and THIS is the right moment for it - the source is still sitting where it
                    // was placed, the character has not been moved anywhere yet and the skeleton
                    // is in the pose the editor showed.
                    //
                    // From the next connect onwards the stored values are used and nothing is
                    // derived any more, so it no longer matters what has been repositioned in the
                    // meantime. Saving the scene persists it; pressing 'Bake Offset Now' in the
                    // editor repeats it deliberately after the source was moved.
                    if (true == this->useBakedOffset->getBool() && true == this->offsetPosition->getVector3().positionEquals(Ogre::Vector3::ZERO, 0.0000001f) &&
                        true == this->offsetOrientation->getVector3().positionEquals(Ogre::Vector3::ZERO, 0.0000001f))
                    {
                        this->offsetPosition->setValue(baseLocalPosition);
                        this->offsetOrientation->setValue(this->internalOrientationToDegrees(baseLocalOrientation));

                        offsetPos = this->offsetPosition->getVector3();
                        offsetQuat = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());

                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPointComponent] Baked the attachment of game object: " + this->gameObjectPtr->getName() +
                                                                                                " -> offset position: " + Ogre::StringConverter::toString(this->offsetPosition->getVector3()) +
                                                                                                " offset orientation: " + Ogre::StringConverter::toString(this->offsetOrientation->getVector3()) + ". Save the scene to persist it.");
                    }

                    if (true == this->useBakedOffset->getBool())
                    {
                        // The offset IS the attachment, in the bone's local space. Nothing above
                        // is used: not where this game object stands, not where the source stands,
                        // not which animation frame the skeleton happens to show.
                        //
                        // That is the whole point. The derived variant below needs all three of
                        // those to be correct at this exact instant, and they are not: a character
                        // that is repositioned on level load before this component connects leaves
                        // the source behind, so 'sourceWorldPosition - boneWorldPosition' becomes
                        // the distance between the two places and the weapon hangs in the void.
                        this->tagPointV2->setPosition(offsetPos);
                        this->tagPointV2->setOrientation(offsetQuat);
                    }
                    else
                    {
                        this->tagPointV2->setPosition(baseLocalPosition + (baseLocalOrientation * offsetPos));
                        this->tagPointV2->setOrientation(baseLocalOrientation * offsetQuat);
                    }

                    // Re-apply the shield's own visual scale (e.g. 0.5, 0.5, 0.5).
                    // Without this the tag point defaults to scale (1,1,1), so the
                    // shield snapped back to its unscaled mesh size once it was
                    // re-parented from its own scene node to the tag point.
                    this->tagPointV2->setScale(baseLocalScale);

                    // Register with the bone � Ogre-Next will update this
                    // TagPoint every frame as part of the skeleton update pass
                    this->attachedBone->addTagPoint(this->tagPointV2);

                    // Expose as tagPointNode so the rest of the engine
                    // (debug draw, physics, etc.) can use it transparently
                    this->tagPointNode = this->tagPointV2;

                    std::vector<Ogre::MovableObject*> movableObjects;
                    auto it = sourceSceneNode->getAttachedObjectIterator();

                    while (it.hasMoreElements())
                    {
                        movableObjects.emplace_back(it.getNext());
                    }

                    for (size_t i = 0; i < movableObjects.size(); i++)
                    {
                        movableObjects[i]->detachFromParent();
                        this->tagPointV2->attachObject(movableObjects[i]);
                    }

                    // Get the bone by name
                    if (nullptr != this->sourcePhysicsActiveComponent)
                    {
                        // For non-kinematic bodies (regular PhysicsActiveComponent)
                        // we need a JointKinematicComponent to drive the body,
                        // mirroring exactly what the v1 TagPointListener does.
                        auto sourcePhysicsActiveKinematicComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(this->sourcePhysicsActiveComponent);
                        if (nullptr == sourcePhysicsActiveKinematicComponent)
                        {
                            auto existingJointKinematicCompPtr = NOWA::makeStrongPtr(this->sourcePhysicsActiveComponent->getOwner()->getComponent<JointKinematicComponent>());
                            if (nullptr == existingJointKinematicCompPtr)
                            {
                                boost::shared_ptr<JointKinematicComponent> jointKinematicCompPtr(boost::make_shared<JointKinematicComponent>());
                                this->sourcePhysicsActiveComponent->getOwner()->addDelayedComponent(jointKinematicCompPtr, true);
                                jointKinematicCompPtr->setOwner(this->sourcePhysicsActiveComponent->getOwner());
                                jointKinematicCompPtr->setBody(this->sourcePhysicsActiveComponent->getBody());
                                jointKinematicCompPtr->setPickingMode(4);
                                jointKinematicCompPtr->setMaxLinearAngleFriction(10000.0f, 10000.0f);
                                jointKinematicCompPtr->createJoint();
                            }
                            else
                            {
                                auto jointCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<JointComponent>());
                                if (nullptr != jointCompPtr)
                                {
                                    existingJointKinematicCompPtr->connectPredecessorId(jointCompPtr->getId());
                                    existingJointKinematicCompPtr->createJoint();
                                }
                            }
                        }

                        // Attention: only the ID is built here, the closure itself is NOT
                        // registered from this render command.
                        //
                        // GraphicsModule::updateTrackedClosure() is meant to be called from an
                        // update() function, once per frame - that is how every other caller in
                        // the engine uses it. When it is called FROM the render thread it takes
                        // its documented shortcut: run the closure once with dt = 0 and return,
                        // without adding it to the tracked list. And connectV2Item() runs as a
                        // RenderCommand.
                        //
                        // So this used to register the physics update exactly once. The attached
                        // object's body was driven to the bone a single time and then stayed
                        // frozen at that transform forever, while the mesh kept following the
                        // bone - body and visual drifted apart with no error anywhere. The
                        // registration now happens in update(), see there.
                        this->updateClosureId = this->gameObjectPtr->getName() + this->getClassName() + "::updateTagPointV2Physics" + Ogre::StringConverter::toString(this->index);
                    }
                }

                this->alreadyConnected = true;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "TagPointComponent::connectV2Item");
    }

    void TagPointComponent::updateV2PhysicsFromTagPoint(void)
    {
        // Runs on the render thread (tracked closure, after renderOneFrame).

        // TEMPORARY DIAGNOSTICS - logged BEFORE the early return, so "the closure never runs"
        // can be told apart from "it runs but returns immediately".
        {
            static unsigned int diagEnterCounter = 0;
            diagEnterCounter++;
            if (diagEnterCounter % 300 == 1)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPoint-DIAG] updateV2PhysicsFromTagPoint ENTERED #" + Ogre::StringConverter::toString(diagEnterCounter) + " tagPointV2: " +
                                                                                        Ogre::String(nullptr != this->tagPointV2 ? "ok" : "NULL") + " sourcePhysics: " + Ogre::String(nullptr != this->sourcePhysicsActiveComponent ? "ok" : "NULL"));
            }
        }

        // attachedBone and the character's scene node are dereferenced below now, so they are
        // part of the guard.
        if (nullptr == this->tagPointV2 || nullptr == this->sourcePhysicsActiveComponent || nullptr == this->attachedBone || nullptr == this->gameObjectPtr->getSceneNode())
        {
            return;
        }

        // Attention: the TagPoint's own derived transform is NOT usable here, in neither variant.
        //
        // An Ogre::TagPoint attached to a bone gets its world transform written by the SKELETON
        // pass, straight from the bone - not through the node parent chain. _getDerivedPositionUpdated()
        // however calls _updateFromParent(), which recomputes from mParent, and that is not the bone.
        // So the call does not resolve the transform, it OVERWRITES the correct one with garbage, and
        // every later _getDerivedPosition() then reads that clobbered cache. Measured: both variants
        // returned 0 0 0 while the mesh was visibly rendering in the character's hand, and the weapon
        // body was driven to the world origin every single frame.
        //
        // The bone's world transform is therefore computed by hand, exactly the way getBonePosition()
        // and getBoneOrientation() in this file do it - bone LOCAL space combined with the character's
        // own node world transform - and the tag point's local offset is applied on top of it. That
        // local offset is what connectV2Item() stored: the source's original pose relative to the bone
        // plus the designer's offset.
        Ogre::Vector3 boneLocalPosition;
        Ogre::Quaternion boneLocalOrientation;
        extractBoneLocalTransform(this->attachedBone, boneLocalPosition, boneLocalOrientation);

        Ogre::SceneNode* characterSceneNode = this->gameObjectPtr->getSceneNode();

        const Ogre::Vector3 characterWorldPosition = characterSceneNode->_getDerivedPositionUpdated();
        const Ogre::Quaternion characterWorldOrientation = characterSceneNode->_getDerivedOrientationUpdated();
        const Ogre::Vector3 characterWorldScale = characterSceneNode->_getDerivedScale();

        const Ogre::Quaternion boneWorldOrientation = characterWorldOrientation * boneLocalOrientation;
        const Ogre::Vector3 boneWorldPosition = characterWorldOrientation * (boneLocalPosition * characterWorldScale) + characterWorldPosition;

        // The tag point's LOCAL transform is untouched by the problem above - it is plain member data
        // that connectV2Item() wrote and nothing recomputes it.
        const Ogre::Vector3 tagPointLocalPosition = this->tagPointV2->getPosition();
        const Ogre::Quaternion tagPointLocalOrientation = this->tagPointV2->getOrientation();

        const Ogre::Quaternion tagPointWorldOrientation = boneWorldOrientation * tagPointLocalOrientation;
        const Ogre::Vector3 tagPointWorldPosition = boneWorldPosition + boneWorldOrientation * (tagPointLocalPosition * characterWorldScale);

        // Attention: the former code wrote this transform into this->gameObjectPtr->getSceneNode() via
        // GraphicsModule::updateNodeTransform(). But this->gameObjectPtr is the CHARACTER that owns the
        // skeleton (this component lives on the character), not the attached source (weapon). So every
        // render frame the tag point's world transform - 0 0 0 on the very first call - was written into
        // the character's own interpolation slot, additionally with useDerived = false, which fought
        // against the physics writes (useDerived = true) for the same node. That was the jump of the
        // player to the world origin on connect.
        //
        // No node write is needed here at all: the source's movable objects were moved onto the tag point
        // in connectV2Item(), so Ogre-Next draws them at the bone automatically. The source's own scene
        // node only follows its physics body, and that is already handled by the body's render callback
        // (OgreNewtModule::registerRenderCallbackForBody) once the body is driven below.

        auto sourcePhysicsActiveKinematicComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(this->sourcePhysicsActiveComponent);
        if (nullptr != sourcePhysicsActiveKinematicComponent)
        {
            // Attention: setKinematicPositionOrientation, NOT setPosition/setOrientation.
            //
            // PhysicsComponent::setPosition() forwards to OgreNewt::Body::setPositionOrientation(),
            // which does not move a KinematicBody at all - OgreNewt has a separate
            // setKinematicPositionOrientation() for those, which translate() and rotate() in
            // PhysicsComponent already use. With the wrong setter the weapon's collision hull
            // stayed where the body was created while the mesh followed the hand, so the body
            // never came near anything and its contact map stayed empty.
            //
            // One call for both values on purpose: set separately, the second call reads the
            // other value back out of the body and writes the stale one.
            sourcePhysicsActiveKinematicComponent->setKinematicPositionOrientation(tagPointWorldPosition, tagPointWorldOrientation);

            // TEMPORARY DIAGNOSTICS - remove once the kinematic contact is understood.
            //
            // Separates the only two possibilities for a weapon body sitting at the world
            // origin:
            //
            //   tagPoint 0 0 0               -> the TagPoint's derived transform is the
            //                                   problem. _getDerivedPositionUpdated() resolves
            //                                   a NODE chain, but a v2 TagPoint hangs off a
            //                                   BONE and is written during the skeleton pass.
            //                                   Compare against 'bone', which is computed the
            //                                   way getBonePosition() does it.
            //
            //   tagPoint correct, body 0 0 0 -> setPosition() never reaches the kinematic
            //                                   body. createDynamicBody() removed its force
            //                                   and torque callback, so a deferred transform
            //                                   command would have no consumer.
            {
                static unsigned int diagCounter = 0;
                diagCounter++;
                if (diagCounter % 30 == 1)
                {
                    // boneLocal is the decisive value: if it never changes while an animation is
                    // playing, the bone transform being read is the bind pose and not the
                    // animated one - then no amount of physics work downstream can help.
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPoint-DIAG] #" + Ogre::StringConverter::toString(diagCounter) + " boneLocal: " + Ogre::StringConverter::toString(boneLocalPosition) +
                                                                                            " charWorld: " + Ogre::StringConverter::toString(characterWorldPosition) + " bone: " + Ogre::StringConverter::toString(boneWorldPosition) +
                                                                                            " computedWorld: " + Ogre::StringConverter::toString(tagPointWorldPosition) +
                                                                                            " bodyAfterSet: " + Ogre::StringConverter::toString(sourcePhysicsActiveKinematicComponent->getPosition()));
                }
            }
        }
        else
        {
            // TEMPORARY DIAGNOSTICS - if this fires for the cudgel, the dynamic_cast to
            // PhysicsActiveKinematicComponent failed and the wrong drive path is used.
            {
                static unsigned int diagJointCounter = 0;
                diagJointCounter++;
                if (diagJointCounter % 300 == 1)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPoint-DIAG] NON-KINEMATIC drive path used for: " + this->sourcePhysicsActiveComponent->getOwner()->getName());
                }
            }

            // Non-kinematic body: the JointKinematicComponent was created in
            // connectV2Item and drives the body via a target position/rotation.
            auto jointKinematicCompPtr = NOWA::makeStrongPtr(this->sourcePhysicsActiveComponent->getOwner()->getComponent<JointKinematicComponent>());
            if (nullptr != jointKinematicCompPtr)
            {
                jointKinematicCompPtr->setTargetPositionRotation(tagPointWorldPosition, tagPointWorldOrientation);
            }
        }
    }

    bool TagPointComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

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
        auto gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
        if (nullptr != gameObjectCompPtr)
        {
            auto physicsRagdollCompPtr = boost::dynamic_pointer_cast<PhysicsRagDollComponentV2>(gameObjectCompPtr);
            auto animationCompPtr = boost::dynamic_pointer_cast<AnimationComponentV2>(gameObjectCompPtr);
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
            // The physics update closure is (re)registered from HERE, on the logic thread,
            // every frame - the way updateTrackedClosure() is meant to be used. Registering it
            // from connectV2Item() did not work, see the comment there.
            //
            // Calling this every frame is cheap and intended: addPersistentClosure() is
            // idempotent, so repeated calls from the same caller are no-ops once the closure
            // sits in the list.
            if (false == this->updateClosureId.empty() && nullptr != this->tagPointV2 && nullptr != this->sourcePhysicsActiveComponent)
            {
                NOWA::GraphicsModule::getInstance()->updateTrackedClosure(
                    this->updateClosureId,
                    [this](Ogre::Real /*renderDt*/)
                    {
                        this->updateV2PhysicsFromTagPoint();
                    },
                    false);
            }

            if (true == this->bShowDebugData && nullptr != this->debugGeometryArrowNode)
            {
                Ogre::Vector3 worldPosition;
                Ogre::Quaternion worldOrientation;

                // Try v1 approach first
                if (nullptr != this->tagPointV2)
                {
                    // V2 approach - just use the tagpoint node's derived transform
                    worldPosition = this->tagPointV2->_getDerivedPosition();
                    worldOrientation = this->tagPointV2->_getDerivedOrientation();
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
        else if (TagPointComponent::AttrUseBakedOffset() == attribute->getName())
        {
            this->setUseBakedOffset(attribute->getBool());
        }
    }

    bool TagPointComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if ("TagPointComponent.BakeOffset" == actionId)
        {
            this->bakeOffset();
            return true;
        }

        return false;
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

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UseBakedOffset"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useBakedOffset->getBool())));
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

        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr != item)
        {
            this->setTagPointNameV2(item, tagPointName);
        }

        if (true == this->bShowDebugData)
        {
            this->destroyDebugData();
            this->generateDebugData();
        }
    }

    void TagPointComponent::setTagPointNameV2(Ogre::Item* item, const Ogre::String& tagPointName)
    {
        if (nullptr == this->skeletonInstance)
        {
            return;
        }

        Ogre::IdString boneIdString(tagPointName);
        Ogre::Bone* newBone = this->skeletonInstance->getBone(boneIdString);

        if (nullptr != this->tagPointV2)
        {
            // Already connected: re-parent the existing TagPoint to the new bone
            // (must happen on render thread since it touches the skeleton graph)
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, item, tagPointName]()
            {
                if (nullptr != this->attachedBone)
                {
                    this->attachedBone->removeTagPoint(this->tagPointV2);
                }
                Ogre::IdString newBoneId(tagPointName);
                this->attachedBone = this->skeletonInstance->getBone(newBoneId);
                if (nullptr != this->attachedBone)
                {
                    this->attachedBone->addTagPoint(this->tagPointV2);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "TagPointComponent::setTagPointNameV2");
        }
        else
        {
            // Not yet connected � just track the bone; connectV2Item will use it
            this->attachedBone = newBone;
        }
    }

    Ogre::String TagPointComponent::getTagPointName(void) const
    {
        return this->tagPoints->getListSelectedValue();
    }

    void TagPointComponent::setSourceId(unsigned long sourceId)
    {
        const unsigned long currentSourceId = this->sourceId->getULong();
        if (currentSourceId == sourceId)
        {
            return;
        }

        // A connected tag point has already captured the old source node's
        // movable objects and physics driver. Queue the teardown first, then
        // queue a fresh connection against the new source id in the same
        // render-command order.
        if (true == this->bConnected)
        {
            this->sourceId->setValue(currentSourceId);
            this->resetTagPoint();
        }

        this->sourceId->setValue(sourceId);

        if (true == this->bConnected)
        {
            this->alreadyConnected = false;
            Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
            if (nullptr != item)
            {
                this->connectV2Item(item);
            }
        }
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

    Ogre::Vector3 TagPointComponent::internalOrientationToDegrees(const Ogre::Quaternion& orientation)
    {
        // The offset attribute is degrees, so the quaternion has to be decomposed. Ogre's XYZ
        // euler extraction is the inverse of building the rotation as X * Y * Z.
        Ogre::Matrix3 rotationMatrix;
        orientation.ToRotationMatrix(rotationMatrix);

        Ogre::Radian eulerX;
        Ogre::Radian eulerY;
        Ogre::Radian eulerZ;
        rotationMatrix.ToEulerAnglesXYZ(eulerX, eulerY, eulerZ);

        Ogre::Vector3 degrees(eulerX.valueDegrees(), eulerY.valueDegrees(), eulerZ.valueDegrees());

        // Verified rather than trusted: MathHelper::degreesToQuat is what connect() feeds these
        // degrees back into, so the round trip has to land on the same rotation. If the two used a
        // different euler order, the attachment would come back subtly twisted and silently so.
        Ogre::Quaternion verifyOrientation = MathHelper::getInstance()->degreesToQuat(degrees);
        const Ogre::Real orientationDot = Ogre::Math::Abs(verifyOrientation.Dot(orientation));

        if (orientationDot < 0.9999f)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPointComponent] The attachment orientation of game object: " + this->gameObjectPtr->getName() + " cannot be expressed as degrees without loss (dot: " +
                                                                                    Ogre::StringConverter::toString(orientationDot) + "). MathHelper::degreesToQuat uses a different euler order than Matrix3::ToEulerAnglesXYZ.");
        }

        return degrees;
    }

    bool TagPointComponent::internalComputeBaseLocalTransform(Ogre::Bone* bone, Ogre::Vector3& outLocalPosition, Ogre::Quaternion& outLocalOrientation)
    {
        if (nullptr == bone || nullptr == this->gameObjectPtr)
        {
            return false;
        }

        GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
        if (nullptr == sourceGameObjectPtr)
        {
            return false;
        }

        Ogre::SceneNode* characterSceneNode = this->gameObjectPtr->getSceneNode();
        Ogre::SceneNode* sourceSceneNode = sourceGameObjectPtr->getSceneNode();

        if (nullptr == characterSceneNode || nullptr == sourceSceneNode)
        {
            return false;
        }

        Ogre::Vector3 sourceWorldPosition = sourceSceneNode->_getDerivedPositionUpdated();
        Ogre::Quaternion sourceWorldOrientation = sourceSceneNode->_getDerivedOrientationUpdated();

        Ogre::Vector3 characterWorldPosition = characterSceneNode->_getDerivedPositionUpdated();
        Ogre::Quaternion characterWorldOrientation = characterSceneNode->_getDerivedOrientationUpdated();
        Ogre::Vector3 characterWorldScale = characterSceneNode->_getDerivedScale();

        Ogre::Vector3 boneLocalPosition;
        Ogre::Quaternion boneLocalOrientation;
        extractBoneLocalTransform(bone, boneLocalPosition, boneLocalOrientation);

        Ogre::Vector3 boneWorldPosition = characterWorldOrientation * (boneLocalPosition * characterWorldScale) + characterWorldPosition;
        Ogre::Quaternion boneWorldOrientation = characterWorldOrientation * boneLocalOrientation;
        Ogre::Vector3 boneWorldScale = characterWorldScale;

        Ogre::Quaternion boneWorldOrientationInverse = boneWorldOrientation.Inverse();

        outLocalPosition = boneWorldOrientationInverse * ((sourceWorldPosition - boneWorldPosition) / boneWorldScale);
        outLocalOrientation = boneWorldOrientationInverse * sourceWorldOrientation;

        return true;
    }

    void TagPointComponent::setUseBakedOffset(bool useBakedOffset)
    {
        this->useBakedOffset->setValue(useBakedOffset);
    }

    bool TagPointComponent::getUseBakedOffset(void) const
    {
        return this->useBakedOffset->getBool();
    }

    void TagPointComponent::bakeOffset(void)
    {
        Ogre::Vector3 localPosition = Ogre::Vector3::ZERO;
        Ogre::Quaternion localOrientation = Ogre::Quaternion::IDENTITY;
        bool baked = false;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, &localPosition, &localOrientation, &baked]()
        {
            if (nullptr != this->tagPointV2)
            {
                // Already attached: the tag point is a child of the bone, so its LOCAL transform
                // IS the attachment relative to that bone. Nothing has to be recomputed.
                localPosition = this->tagPointV2->getPosition();
                localOrientation = this->tagPointV2->getOrientation();
                baked = true;
                return;
            }

            // Not attached yet - which is the NORMAL case, because the natural moment to bake is
            // in the editor with the simulation switched OFF: the source sits exactly where it was
            // pushed into the hand, the character is at its authored position and the skeleton is
            // in its bind pose. There is no better pose to freeze than that one, and no tag point
            // exists at that point.
            //
            // So the very computation connect() would do is run right here instead, and its result
            // is stored rather than being redone later under conditions nobody controls.
            if (nullptr == this->skeletonInstance)
            {
                return;
            }

            const Ogre::String boneName = this->tagPoints->getListSelectedValue();
            if (true == boneName.empty() || false == this->skeletonInstance->hasBone(Ogre::IdString(boneName)))
            {
                return;
            }

            baked = this->internalComputeBaseLocalTransform(this->skeletonInstance->getBone(Ogre::IdString(boneName)), localPosition, localOrientation);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "TagPointComponent::bakeOffset");

        if (false == baked)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[TagPointComponent] bakeOffset failed for game object: " + this->gameObjectPtr->getName() + ". Check that 'Tag Point Name' names an existing bone and that 'Source Id' points at a game object that is in the scene.");
            return;
        }

        Ogre::Vector3 degrees = this->internalOrientationToDegrees(localOrientation);

        this->offsetPosition->setValue(localPosition);
        this->offsetOrientation->setValue(degrees);
        this->useBakedOffset->setValue(true);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPointComponent] bakeOffset for game object: " + this->gameObjectPtr->getName() + " -> offset position: " + Ogre::StringConverter::toString(localPosition) +
                                                                                " offset orientation: " + Ogre::StringConverter::toString(degrees) + ". Save the scene to persist it.");
    }

    void TagPointComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
    {
        this->offsetOrientation->setValue(offsetOrientation);
    }

    Ogre::Vector3 TagPointComponent::getOffsetOrientation(void) const
    {
        return this->offsetOrientation->getVector3();
    }

    Ogre::Vector3 TagPointComponent::getBonePosition(const Ogre::String& name) const
    {
        if (nullptr == this->skeletonInstance || nullptr == this->gameObjectPtr || nullptr == this->gameObjectPtr->getSceneNode())
        {
            return Ogre::Vector3::ZERO;
        }

        Ogre::Bone* bone = this->skeletonInstance->getBone(Ogre::IdString(name));
        if (nullptr == bone)
        {
            return Ogre::Vector3::ZERO;
        }

        Ogre::Vector3 localPosition;
        Ogre::Quaternion localOrientation;
        extractBoneLocalTransform(bone, localPosition, localOrientation);

        Ogre::SceneNode* characterSceneNode = this->gameObjectPtr->getSceneNode();
        return characterSceneNode->_getDerivedOrientationUpdated() * (localPosition * characterSceneNode->_getDerivedScale()) + characterSceneNode->_getDerivedPositionUpdated();
    }

    Ogre::Quaternion TagPointComponent::getBoneOrientation(const Ogre::String& name) const
    {
        if (nullptr == this->skeletonInstance || nullptr == this->gameObjectPtr || nullptr == this->gameObjectPtr->getSceneNode())
        {
            return Ogre::Quaternion::IDENTITY;
        }

        Ogre::Bone* bone = this->skeletonInstance->getBone(Ogre::IdString(name));
        if (nullptr == bone)
        {
            return Ogre::Quaternion::IDENTITY;
        }

        Ogre::Vector3 localPosition;
        Ogre::Quaternion localOrientation;
        extractBoneLocalTransform(bone, localPosition, localOrientation);
        return this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated() * localOrientation;
    }

    Ogre::TagPoint* TagPointComponent::getTagPoint(void) const
    {
        return this->tagPointV2;
    }

    Ogre::SceneNode* TagPointComponent::getTagPointNode(void) const
    {
        return this->tagPointNode;
    }

    void TagPointComponent::generateDebugData(void)
    {
        // Generate debug visualization for both v1 and v2 paths
        bool canGenerateDebug = nullptr != this->tagPointV2;

        if (nullptr == this->debugGeometryArrowNode && canGenerateDebug)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->debugGeometryArrowNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
                this->debugGeometryArrowNode->setName("tagPointComponentArrowNode");
                this->debugGeometrySphereNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
                this->debugGeometrySphereNode->setName("tagPointComponentSphereNode");

                Ogre::Vector3 worldPosition;
                Ogre::Quaternion worldOrientation;

                if (nullptr != this->tagPointV2)
                {
                    // V2 path
                    // transform is updated automatically by Ogre each frame.
                    worldPosition = this->tagPointV2->_getDerivedPosition();
                    worldOrientation = this->tagPointV2->_getDerivedOrientation();
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
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "TagPointComponent::generateDebugData");
        }
    }

    void TagPointComponent::destroyDebugData(void)
    {
        if (nullptr != this->debugGeometryArrowNode)
        {
            GraphicsModule::getInstance()->removeTrackedNode(this->debugGeometryArrowNode);
            GraphicsModule::getInstance()->removeTrackedNode(this->debugGeometrySphereNode);

            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
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
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "TagPointComponent::destroyDebugData");
        }
    }

    void TagPointComponent::resetTagPoint(void)
    {
        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr != item && nullptr != this->tagPointV2)
        {
            this->resetTagPointV2(item);
        }
    }

    void TagPointComponent::resetTagPointV2(Ogre::Item* item)
    {
        // TODO: Is this correct?
        if (true == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, item]()
        {
            // Remove the tracked closure
            if (this->updateClosureId.length() > 0)
            {
                NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->updateClosureId);
                this->updateClosureId = "";
            }

            GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
            if (nullptr != sourceGameObjectPtr)
            {
                sourceGameObjectPtr->setConnectedGameObject(boost::weak_ptr<GameObject>());

                if (nullptr != this->tagPointV2)
                {
                    std::vector<Ogre::MovableObject*> movableObjects;
                    auto it = this->tagPointV2->getAttachedObjectIterator();
                    while (it.hasMoreElements())
                    {
                        movableObjects.emplace_back(it.getNext());
                    }
                    for (size_t i = 0; i < movableObjects.size(); i++)
                    {
                        movableObjects[i]->detachFromParent();
                        sourceGameObjectPtr->getSceneNode()->attachObject(movableObjects[i]);
                    }
                }
            }

            // Detach the TagPoint from the bone, then destroy it
            if (nullptr != this->tagPointV2)
            {
                if (nullptr != this->attachedBone)
                {
                    this->attachedBone->removeTagPoint(this->tagPointV2);
                }
                // this->gameObjectPtr->getSceneManager()->destroytagpo(this->tagPointV2);
                this->tagPointV2 = nullptr;
                this->tagPointNode = nullptr;
            }

            this->attachedBone = nullptr;
            this->alreadyConnected = false;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "TagPointComponent::resetTagPointV2");
    }

    // Lua registration part

    TagPointComponent* getTagPointComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<TagPointComponent>(gameObject->getComponentWithOccurrence<TagPointComponent>(occurrenceIndex)).get();
    }

    TagPointComponent* getTagPointComponent(GameObject* gameObject)
    {
        return makeStrongPtr<TagPointComponent>(gameObject->getComponent<TagPointComponent>()).get();
    }

    TagPointComponent* getTagPointComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<TagPointComponent>(gameObject->getComponentFromName<TagPointComponent>(name)).get();
    }

    void setSourceIdForLua(TagPointComponent* instance, const Ogre::String& sourceId)
    {
        instance->setSourceId(Ogre::StringConverter::parseUnsignedLong(sourceId));
    }

    Ogre::String getSourceIdForLua(TagPointComponent* instance)
    {
        return Ogre::StringConverter::toString(instance->getSourceId());
    }

    void TagPointComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<TagPointComponent, GameObjectComponent>("TagPointComponent")
                .def("setTagPointName", &TagPointComponent::setTagPointName)
                .def("getTagPointName", &TagPointComponent::getTagPointName)
                .def("setSourceId", &setSourceIdForLua)
                .def("getSourceId", &getSourceIdForLua)
                .def("setOffsetPosition", &TagPointComponent::setOffsetPosition)
                .def("getOffsetPosition", &TagPointComponent::getOffsetPosition)
                .def("setOffsetOrientation", &TagPointComponent::setOffsetOrientation)
                .def("setUseBakedOffset", &TagPointComponent::setUseBakedOffset)
                .def("getUseBakedOffset", &TagPointComponent::getUseBakedOffset)
                .def("bakeOffset", &TagPointComponent::bakeOffset)
                .def("getOffsetOrientation", &TagPointComponent::getOffsetOrientation)
                .def("getBonePosition", &TagPointComponent::getBonePosition)
                .def("getBoneOrientation", &TagPointComponent::getBoneOrientation)];

        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "class inherits GameObjectComponent", TagPointComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "void setTagPointName(String tagName)", "Sets the tag point name the source game object should be attached to.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "String getTagPointName()", "Gets the current active tag point name.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "void setSourceId(String sourceId)", "Sets the source game object id.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "String getSourceId()", "Gets the source game object id.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets the attachment offset position.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "Vector3 getOffsetPosition()", "Gets the attachment offset position.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "void setOffsetOrientation(Vector3 offsetOrientation)", "Sets the attachment offset orientation in degrees.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "Vector3 getOffsetOrientation()", "Gets the attachment offset orientation in degrees.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "void setUseBakedOffset(bool useBakedOffset)",
            "Sets whether the offset IS the attachment transform in the bone's local space. On, nothing is derived from world transforms at connect time, so it no longer matters where "
            "the character or the source are standing or which animation frame is showing. Off (the default) keeps the old behaviour of adding the offset on top of a transform derived "
            "at connect time.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "bool getUseBakedOffset()", "Gets whether the offset is used as the final attachment transform.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "void bakeOffset()",
            "Freezes the CURRENT attachment into the offset attributes and switches 'Use Baked Offset' on. Place the source where it belongs, let the component connect once, then call "
            "this and save the scene.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "Vector3 getBonePosition(String name)", "Gets a named bone position in world space.");
        LuaScriptApi::getInstance()->addClassToCollection("TagPointComponent", "Quaternion getBoneOrientation(String name)", "Gets a named bone orientation in world space.");

        gameObjectClass.def("getTagPointComponentFromName", &getTagPointComponentFromName);
        gameObjectClass.def("getTagPointComponent", (TagPointComponent * (*)(GameObject*)) & getTagPointComponent);
        // If its desired to create several of this components for one game object
        gameObjectClass.def("getTagPointComponentFromIndex", (TagPointComponent * (*)(GameObject*, unsigned int)) & getTagPointComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TagPointComponent getTagPointComponentFromIndex(unsigned int occurrenceIndex)",
            "Gets the component by the given occurence index, since a game object may this component maybe several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TagPointComponent getTagPointComponent()", "Gets the component. This can be used if the game object this component just once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TagPointComponent getTagPointComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castTagPointComponent", &GameObjectController::cast<TagPointComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "TagPointComponent castTagPointComponent(TagPointComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end