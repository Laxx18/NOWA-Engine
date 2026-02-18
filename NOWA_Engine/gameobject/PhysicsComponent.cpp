#include "NOWAPrecompiled.h"
#include "PhysicsComponent.h"
#include "JointComponents.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "Terra.h"

#include "Animation/OgreBone.h"
#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"
#include "OgreBitwise.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVertexArrayObject.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    PhysicsComponent::PhysicsComponent() :
        GameObjectComponent(),
        ogreNewt(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()), // really important line of code, ogrenewt must be not null, when using it
        physicsBody(nullptr),
        initialPosition(Ogre::Vector3::ZERO),
        initialScale(Ogre::Vector3::UNIT_SCALE),
        initialOrientation(Ogre::Quaternion::IDENTITY),
        volume(0.0f),
        collisionType(new Variant(PhysicsComponent::AttrCollisionType(), this->attributes)),
        mass(new Variant(PhysicsComponent::AttrMass(), Ogre::Real(10.0f), this->attributes)),
        collidable(new Variant(PhysicsComponent::AttrCollidable(), true, this->attributes))
    {
    }

    bool PhysicsComponent::postInit(void)
    {
        // If this game object has already an kind of physics component, do not add another one! Else crash, e.g. PhysicsArtifactComponent may not have an additional PhysicsActiveComponent

        // Deactivated: Case: Terra with TerrainComponent and foliage with artifactcomponent
        /*for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectPtr->getComponents()->size()); i++)
        {
            auto& priorPhysicsComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsComponent>(i));
            if (nullptr != priorPhysicsComponent && priorPhysicsComponent.get() != this)
            {
                Ogre::String message = "[PhysicsComponent] Cannot add physics component, because the game object: " + this->gameObjectPtr->getName() + " does already have a kind of physics component.";
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

                boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
                return false;
            }
        }*/
        return true;
    }

    PhysicsComponent::~PhysicsComponent()
    {
        // delete the body
        this->destroyBody();
        /*this->taggedList->clear();
        delete this->taggedList;
        this->taggedList = static_cast<std::list<GameObjectPtr>*>(0);*/
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] Destructor physics component for game object: " + this->gameObjectPtr->getName());
    }

    bool PhysicsComponent::connect(void)
    {
        GameObjectComponent::connect();
        return true;
    }

    bool PhysicsComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        if (nullptr != this->physicsBody)
        {
            this->physicsBody->setVelocity(Ogre::Vector3::ZERO);
            this->physicsBody->setOmega(Ogre::Vector3::ZERO);
            this->physicsBody->setForce(Ogre::Vector3::ZERO);
            this->physicsBody->setTorque(Ogre::Vector3::ZERO);
        }
        return true;
    }

    void PhysicsComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();
    }

    OgreNewt::CollisionPtr PhysicsComponent::createDynamicCollision(Ogre::Vector3& inertia, const Ogre::Vector3& collisionSize, const Ogre::Vector3& collisionPosition, const Ogre::Quaternion& collisionOrientation, Ogre::Vector3& massOrigin,
        unsigned int categoryId)
    {
        Ogre::Vector3 tempCollisionSize = collisionSize;
        if (Ogre::Vector3::ZERO == collisionSize)
        {
            tempCollisionSize = this->gameObjectPtr->getSize();
        }

        OgreNewt::CollisionPtr collisionPtr;

        if (this->collisionType->getListSelectedValue() == "ConvexHull")
        {
            Ogre::v1::Entity* entity = nullptr;
            Ogre::Item* item = nullptr;
            OgreNewt::CollisionPrimitives::ConvexHull* col = nullptr;

            if (GameObject::ENTITY == this->gameObjectPtr->getType())
            {
                entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
                col = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, entity, categoryId, collisionOrientation, collisionPosition, 0.001f /*, this->gameObjectPtr->getSceneNode()->getScale()*/);
            }
            else if (GameObject::ITEM == this->gameObjectPtr->getType())
            {
                item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
                col = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, item, categoryId, collisionOrientation, collisionPosition, 0.001f /*, this->gameObjectPtr->getSceneNode()->getScale()*/);
            }

            if (nullptr == col)
            {
                return OgreNewt::CollisionPtr();
            }

            if (Ogre::Vector3::ZERO != collisionSize)
            {
                col->scaleCollision(collisionSize);
            }

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            // calculate interia and mass center of the body
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "ConcaveHull")
        {
            OgreNewt::CollisionPrimitives::ConcaveHull* col =
                new OgreNewt::CollisionPrimitives::ConcaveHull(this->ogreNewt, this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>(), categoryId, 0.001, this->gameObjectPtr->getSceneNode()->getScale());

            if (Ogre::Vector3::ZERO != collisionSize)
            {
                col->scaleCollision(collisionSize);
            }

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            // calculate interia and mass center of the body
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Box")
        {
            OgreNewt::CollisionPrimitives::Box* col = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, tempCollisionSize, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Capsule")
        {
            if (Ogre::Vector3::ZERO == collisionSize)
            {
                tempCollisionSize.y *= 0.5f;
            }

            OgreNewt::CollisionPrimitives::Capsule* col;
            // Check if all vector components are used. In this case the developer may construct a really custom capsule
            if (tempCollisionSize.z != 0.0f)
            {
                col = new OgreNewt::CollisionPrimitives::Capsule(this->ogreNewt, tempCollisionSize, categoryId, collisionOrientation, collisionPosition);
            }
            else
            {
                // strangly the position must have an offset of size.y / 2
                col = new OgreNewt::CollisionPrimitives::Capsule(this->ogreNewt, tempCollisionSize.x, tempCollisionSize.y, categoryId, collisionOrientation, collisionPosition);
            }

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "ChamferCylinder")
        {
            if (Ogre::Vector3::ZERO == collisionSize)
            {
                tempCollisionSize.y *= 0.5f;
            }

            OgreNewt::CollisionPrimitives::ChamferCylinder* col = new OgreNewt::CollisionPrimitives::ChamferCylinder(this->ogreNewt, tempCollisionSize.x, tempCollisionSize.y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Cone")
        {
            if (Ogre::Vector3::ZERO == collisionSize)
            {
                tempCollisionSize.y *= 0.5f;
            }

            OgreNewt::CollisionPrimitives::Cone* col = new OgreNewt::CollisionPrimitives::Cone(this->ogreNewt, tempCollisionSize.x, tempCollisionSize.y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Cylinder")
        {
            if (Ogre::Vector3::ZERO == collisionSize)
            {
                tempCollisionSize.y *= 0.5f;
            }

            OgreNewt::CollisionPrimitives::Cylinder* col = new OgreNewt::CollisionPrimitives::Cylinder(this->ogreNewt, tempCollisionSize.x, tempCollisionSize.y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Ellipsoid") // can also be a sphere
        {
            OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(this->ogreNewt, tempCollisionSize, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Pyramid")
        {
            OgreNewt::CollisionPrimitives::Pyramid* col = new OgreNewt::CollisionPrimitives::Pyramid(this->ogreNewt, tempCollisionSize, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else
        {
            OgreNewt::CollisionPrimitives::Null* col = new OgreNewt::CollisionPrimitives::Null(this->ogreNewt);
            collisionPtr = OgreNewt::CollisionPtr(col);
        }
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] calculated volume: " + Ogre::StringConverter::toString(this->volume) + " for: " + this->gameObjectPtr->getName());

        return collisionPtr;
    }

    OgreNewt::CollisionPtr PhysicsComponent::createDynamicCollision(Ogre::Vector3& inertia, const Ogre::Vector3& collisionPosition, const Ogre::Quaternion& collisionOrientation, Ogre::Vector3& massOrigin, unsigned int categoryId)
    {
        if (this->collisionType->getListSelectedValue() == "ConvexHull")
        {
            Ogre::v1::Entity* entity = nullptr;
            Ogre::Item* item = nullptr;
            OgreNewt::CollisionPrimitives::ConvexHull* col = nullptr;

            if (GameObject::ENTITY == this->gameObjectPtr->getType())
            {
                entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
                col = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, entity, categoryId, collisionOrientation, collisionPosition, 0.001f /*, this->gameObjectPtr->getSceneNode()->getScale()*/);
            }
            else if (GameObject::ITEM == this->gameObjectPtr->getType())
            {
                item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
                col = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, item, categoryId, collisionOrientation, collisionPosition, 0.001f /*, this->gameObjectPtr->getSceneNode()->getScale()*/);
            }

            if (nullptr == col)
            {
                return OgreNewt::CollisionPtr();
            }

            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            // calculate interia and mass center of the body
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "ConcaveHull")
        {
            OgreNewt::CollisionPrimitives::ConcaveHull* col =
                new OgreNewt::CollisionPrimitives::ConcaveHull(this->ogreNewt, this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>(), categoryId, 0.001, this->gameObjectPtr->getSceneNode()->getScale());
            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            // calculate interia and mass center of the body
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Box")
        {
            OgreNewt::CollisionPrimitives::Box* col = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, this->gameObjectPtr->getSize(), categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Capsule")
        {
            // strangly the position must have an offset of size.y / 2
            OgreNewt::CollisionPrimitives::Capsule* col = new OgreNewt::CollisionPrimitives::Capsule(this->ogreNewt, this->gameObjectPtr->getSize().x / 2.0f, this->gameObjectPtr->getSize().y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "ChamferCylinder")
        {
            OgreNewt::CollisionPrimitives::ChamferCylinder* col =
                new OgreNewt::CollisionPrimitives::ChamferCylinder(this->ogreNewt, this->gameObjectPtr->getSize().x / 2.0f, this->gameObjectPtr->getSize().y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Cone")
        {
            OgreNewt::CollisionPrimitives::Cone* col = new OgreNewt::CollisionPrimitives::Cone(this->ogreNewt, this->gameObjectPtr->getSize().x / 2.0f, this->gameObjectPtr->getSize().y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Cylinder")
        {
            OgreNewt::CollisionPrimitives::Cylinder* col = new OgreNewt::CollisionPrimitives::Cylinder(this->ogreNewt, this->gameObjectPtr->getSize().x / 2.0f, this->gameObjectPtr->getSize().y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Ellipsoid") // can also be a sphere
        {
            OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(this->ogreNewt, this->gameObjectPtr->getSize() / 2.0f, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else if (this->collisionType->getListSelectedValue() == "Pyramid")
        {
            OgreNewt::CollisionPrimitives::Pyramid* col = new OgreNewt::CollisionPrimitives::Pyramid(this->ogreNewt, this->gameObjectPtr->getSize(), categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            this->collisionPtr = OgreNewt::CollisionPtr(col);
            col->calculateInertialMatrix(inertia, massOrigin);
        }
        else
        {
            OgreNewt::CollisionPrimitives::Null* col = new OgreNewt::CollisionPrimitives::Null(this->ogreNewt);
            this->collisionPtr = OgreNewt::CollisionPtr(col);
        }
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] calculated volume: " + Ogre::StringConverter::toString(this->volume) + " for: " + this->gameObjectPtr->getName());

        return this->collisionPtr;
    }

    OgreNewt::CollisionPtr PhysicsComponent::createCollisionPrimitive(const Ogre::String& collisionType, const Ogre::Vector3& collisionPosition, const Ogre::Quaternion& collisionOrientation, const Ogre::Vector3& collisionSize, Ogre::Vector3& inertia,
        Ogre::Vector3& massOrigin, unsigned int categoryId)
    {
        // convex hull is not possible, because an entity would be required
        if (collisionType == "ConvexHull")
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] Could not create convex hull collision, because this type is no primitive type!");
            return OgreNewt::CollisionPtr();
        }
        else if (collisionType == "ConcaveHull")
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] Could not create concave hull collision, because this type is no primitive type!");
            return OgreNewt::CollisionPtr();
        }

        OgreNewt::CollisionPtr collisionPtr;

        if (collisionType == "Box")
        {
            OgreNewt::CollisionPrimitives::Box* col = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, collisionSize, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
        }
        else if (collisionType == "Capsule")
        {
            // strangly the position must have an offset of size.y / 2
            OgreNewt::CollisionPrimitives::Capsule* col = new OgreNewt::CollisionPrimitives::Capsule(this->ogreNewt, collisionSize.x, collisionSize.y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
        }
        else if (collisionType == "ChamferCylinder")
        {
            OgreNewt::CollisionPrimitives::ChamferCylinder* col = new OgreNewt::CollisionPrimitives::ChamferCylinder(this->ogreNewt, collisionSize.x, collisionSize.y, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
        }
        else if (collisionType == "Cone")
        {
            OgreNewt::CollisionPrimitives::Cone* col = new OgreNewt::CollisionPrimitives::Cone(this->ogreNewt, collisionSize.x, collisionSize.y, categoryId, collisionOrientation, collisionPosition);

            col->calculateInertialMatrix(inertia, massOrigin);
            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
        }
        else if (collisionType == "Cylinder")
        {
            OgreNewt::CollisionPrimitives::Cylinder* col = new OgreNewt::CollisionPrimitives::Cylinder(this->ogreNewt, collisionSize.x, collisionSize.y, categoryId, collisionOrientation, collisionPosition);

            col->calculateInertialMatrix(inertia, massOrigin);
            this->volume = col->calculateVolume();
            collisionPtr = OgreNewt::CollisionPtr(col);
        }
        else if (collisionType == "Ellipsoid") // can also be a sphere
        {
            OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(this->ogreNewt, collisionSize, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
        }
        else if (collisionType == "Pyramid")
        {
            OgreNewt::CollisionPrimitives::Pyramid* col = new OgreNewt::CollisionPrimitives::Pyramid(this->ogreNewt, collisionSize, categoryId, collisionOrientation, collisionPosition);

            this->volume = col->calculateVolume();
            col->calculateInertialMatrix(inertia, massOrigin);
            collisionPtr = OgreNewt::CollisionPtr(col);
        }
        /*else
        {
            OgreNewt::CollisionPrimitives::Null* col = new OgreNewt::CollisionPrimitives::Null(this->ogreNewt);
            collisionPtr = OgreNewt::CollisionPtr(col);
        }*/

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] calculated volume: " + Ogre::StringConverter::toString(this->volume) + " for: " + this->gameObjectPtr->getName());

        return collisionPtr;
    }

    OgreNewt::CollisionPtr PhysicsComponent::createDeformableCollision(OgreNewt::CollisionPtr collisionPtr)
    {
        /*OgreNewt::CollisionPrimitives::TetrahedraDeformableCollision* deformableCollision = new OgreNewt::CollisionPrimitives::TetrahedraDeformableCollision(
            this->ogreNewt, collisionPtr->getNewtonCollision(), 0);

        collisionPtr = OgreNewt::CollisionPtr(deformableCollision);
        return collisionPtr;*/

        return OgreNewt::CollisionPtr();
    }

    OgreNewt::CollisionPtr PhysicsComponent::getWeightedBoneConvexHull(Ogre::v1::OldBone* bone, Ogre::v1::MeshPtr mesh, Ogre::Real minWeight, Ogre::Vector3& inertia, Ogre::Vector3& massOrigin, unsigned int categoryId,
        const Ogre::Vector3& offsetPosition, const Ogre::Quaternion& offsetOrientation, const Ogre::Vector3& scale)
    {
        std::vector<Ogre::Vector3> vertexVector;

        // for this bone, gather all of the vertices linked to it, and make an individual convex hull.
        std::string boneName = bone->getName();
        unsigned int boneIndex = bone->getHandle();

        Ogre::Vector3 bonePosition = (-bone->_getBindingPoseInversePosition()) /*- offsetPosition*/;
        Ogre::Quaternion boneOrientation = bone->_getBindingPoseInverseOrientation().Inverse() /** offsetOrientation*/;

        Ogre::Vector3 bonescale = Ogre::Vector3::UNIT_SCALE / (bone->_getBindingPoseInverseScale() * scale);

        Ogre::Matrix4 invMatrix;
        invMatrix.makeInverseTransform(bonePosition, bonescale, boneOrientation);

        unsigned int num_sub = mesh->getNumSubMeshes();

        for (unsigned int i = 0; i < num_sub; i++)
        {
            Ogre::v1::SubMesh* submesh = mesh->getSubMesh(i);
            Ogre::v1::SubMesh::BoneAssignmentIterator bai = submesh->getBoneAssignmentIterator();

            Ogre::v1::VertexDeclaration* decl;
            const Ogre::v1::VertexElement* elem;
            float* posPtr;
            // size_t count;
            Ogre::v1::VertexData* data = nullptr;

            if (submesh->useSharedVertices)
            {
                data = mesh->sharedVertexData[0];
                // count = data->vertexCount;
                decl = data->vertexDeclaration;
                elem = decl->findElementBySemantic(Ogre::VES_POSITION);
            }
            else
            {
                data = submesh->vertexData[0];
                // count = data->vertexCount;
                decl = data->vertexDeclaration;
                elem = decl->findElementBySemantic(Ogre::VES_POSITION);
            }

            // size_t start = data->vertexStart;
            // pointer
            Ogre::v1::HardwareVertexBufferSharedPtr sptr = data->vertexBufferBinding->getBuffer(elem->getSource());
            unsigned char* ptr = static_cast<unsigned char*>(sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
            unsigned char* offsetVertex;

            while (bai.hasMoreElements())
            {
                Ogre::VertexBoneAssignment vba = bai.getNext();
                if (vba.boneIndex == boneIndex)
                {
                    // found a vertex that is attached to this bone.
                    if (vba.weight >= minWeight)
                    {
                        // get offset to Position data!
                        offsetVertex = ptr + (vba.vertexIndex * sptr->getVertexSize());
                        elem->baseVertexPointerToElement(offsetVertex, &posPtr);

                        Ogre::Vector3 vert;
                        vert.x = *posPtr;
                        posPtr++;
                        vert.y = *posPtr;
                        posPtr++;
                        vert.z = *posPtr;
                        // apply transformation in to local space.
                        vert = invMatrix * vert;

                        vertexVector.push_back(vert);
                        // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RagDoll] vertex found! id:" + Ogre::StringConverter::toString(vba.vertexIndex));
                    }
                }
            }
            sptr->unlock();
        }

        // okay, we have gathered all verts for this bone, make a convex hull!
        unsigned int numVerts = static_cast<unsigned int>(vertexVector.size());
        if (0 == numVerts)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsComponent] Can not generate convex hull from bone weight for bone: '" + bone->getName() + "' and game object: '" + this->gameObjectPtr->getName() +
                                                                                "', because there are no bone assigned vertices, hence creating a box hull.");
            OgreNewt::CollisionPrimitives::Box* tempCol = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, Ogre::Vector3(0.1f, 0.1f, 0.1f) * bonescale, categoryId, boneOrientation, bonePosition);
            tempCol->calculateInertialMatrix(inertia, massOrigin);
            OgreNewt::CollisionPtr col = OgreNewt::CollisionPtr(tempCol);
            return col;
        }
        Ogre::Vector3* verts = new Ogre::Vector3[numVerts];
        unsigned int j = 0;
        while (!vertexVector.empty())
        {
            verts[j] = vertexVector.back();
            vertexVector.pop_back();
            j++;
        }

        OgreNewt::CollisionPrimitives::ConvexHull* tempCol = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, verts, numVerts, categoryId, offsetOrientation, offsetPosition);
        tempCol->calculateInertialMatrix(inertia, massOrigin);
        OgreNewt::CollisionPtr col = OgreNewt::CollisionPtr(tempCol);

        delete[] verts;

        return col;
    }

    // Helper function to find bone index in skeleton
    size_t PhysicsComponent::findBoneIndex(Ogre::SkeletonInstance* skelInstance, Ogre::Bone* bone)
    {
        size_t numBones = skelInstance->getNumBones();
        for (size_t i = 0; i < numBones; ++i)
        {
            if (skelInstance->getBone(i) == bone)
            {
                return i;
            }
        }
        return size_t(-1);
    }

    // Helper function to convert SimpleMatrixAf4x3 to Matrix4
    Ogre::Matrix4 PhysicsComponent::convertSimpleMatrixToMatrix4(const Ogre::SimpleMatrixAf4x3& simpleMatrix)
    {
        Ogre::Matrix4 mat;
        simpleMatrix.store(&mat);
        return mat;
    }

    // Extract vertices from a VAO that are weighted to a specific bone
    void PhysicsComponent::extractVerticesFromVAO(const Ogre::VertexArrayObject* vao, size_t targetBoneIndex, Ogre::Real minWeight, const Ogre::Matrix4& invMatrix, std::vector<Ogre::Vector3>& vertexVector)
    {
        if (!vao)
        {
            return;
        }

        // Cast away const — readRequests/mapAsyncTickets are non-const but only create
        // temporary read tickets, so this is safe.
        Ogre::VertexArrayObject* mutableVao = const_cast<Ogre::VertexArrayObject*>(vao);

        // Setup read requests for the three semantics we need.
        // readRequests will automatically find the correct buffer for each semantic.
        Ogre::VertexArrayObject::ReadRequestsVec requests;
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_BLEND_WEIGHTS));
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_BLEND_INDICES));

        try
        {
            mutableVao->readRequests(requests);
            mutableVao->mapAsyncTickets(requests);
        }
        catch (Ogre::Exception&)
        {
            // This submesh doesn't have skinning data (no blend weights/indices)
            return;
        }

        size_t numVertices = requests[0].vertexBuffer->getNumElements();

        // Pre-allocate space in vector to avoid reallocations
        vertexVector.reserve(vertexVector.size() + numVertices / 10);

        for (size_t vertIdx = 0; vertIdx < numVertices; ++vertIdx)
        {
            // ========== POSITION ==========
            Ogre::Vector3 worldPos;
            if (requests[0].type == Ogre::VET_HALF4)
            {
                const Ogre::uint16* posData = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
                worldPos.x = Ogre::Bitwise::halfToFloat(posData[0]);
                worldPos.y = Ogre::Bitwise::halfToFloat(posData[1]);
                worldPos.z = Ogre::Bitwise::halfToFloat(posData[2]);
            }
            else // VET_FLOAT3
            {
                const float* posData = reinterpret_cast<const float*>(requests[0].data);
                worldPos.x = posData[0];
                worldPos.y = posData[1];
                worldPos.z = posData[2];
            }

            // ========== BLEND WEIGHTS ==========
            float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};

            if (requests[1].type == Ogre::VET_FLOAT4)
            {
                const float* weightData = reinterpret_cast<const float*>(requests[1].data);
                weights[0] = weightData[0];
                weights[1] = weightData[1];
                weights[2] = weightData[2];
                weights[3] = weightData[3];
            }
            else if (requests[1].type == Ogre::VET_UBYTE4_NORM)
            {
                // 4 normalized bytes: value / 255.0f
                const Ogre::uint8* weightData = reinterpret_cast<const Ogre::uint8*>(requests[1].data);
                weights[0] = static_cast<float>(weightData[0]) / 255.0f;
                weights[1] = static_cast<float>(weightData[1]) / 255.0f;
                weights[2] = static_cast<float>(weightData[2]) / 255.0f;
                weights[3] = static_cast<float>(weightData[3]) / 255.0f;
            }
            else if (requests[1].type == Ogre::VET_USHORT4_NORM)
            {
                // 4 normalized ushorts: value / 65535.0f
                const Ogre::uint16* weightData = reinterpret_cast<const Ogre::uint16*>(requests[1].data);
                weights[0] = static_cast<float>(weightData[0]) / 65535.0f;
                weights[1] = static_cast<float>(weightData[1]) / 65535.0f;
                weights[2] = static_cast<float>(weightData[2]) / 65535.0f;
                weights[3] = static_cast<float>(weightData[3]) / 65535.0f;
            }
            else if (requests[1].type == Ogre::VET_HALF4)
            {
                const Ogre::uint16* weightData = reinterpret_cast<const Ogre::uint16*>(requests[1].data);
                weights[0] = Ogre::Bitwise::halfToFloat(weightData[0]);
                weights[1] = Ogre::Bitwise::halfToFloat(weightData[1]);
                weights[2] = Ogre::Bitwise::halfToFloat(weightData[2]);
                weights[3] = Ogre::Bitwise::halfToFloat(weightData[3]);
            }
            else if (requests[1].type == Ogre::VET_HALF2)
            {
                const Ogre::uint16* weightData = reinterpret_cast<const Ogre::uint16*>(requests[1].data);
                weights[0] = Ogre::Bitwise::halfToFloat(weightData[0]);
                weights[1] = Ogre::Bitwise::halfToFloat(weightData[1]);
            }
            else if (requests[1].type == Ogre::VET_FLOAT2)
            {
                const float* weightData = reinterpret_cast<const float*>(requests[1].data);
                weights[0] = weightData[0];
                weights[1] = weightData[1];
            }
            else if (requests[1].type == Ogre::VET_FLOAT1)
            {
                const float* weightData = reinterpret_cast<const float*>(requests[1].data);
                weights[0] = weightData[0];
            }

            // ========== BLEND INDICES ==========
            Ogre::uint16 boneIndices[4] = {0, 0, 0, 0};

            if (requests[2].type == Ogre::VET_UBYTE4)
            {
                // Most common: 4 x uint8 bone indices
                const Ogre::uint8* indexData = reinterpret_cast<const Ogre::uint8*>(requests[2].data);
                boneIndices[0] = static_cast<Ogre::uint16>(indexData[0]);
                boneIndices[1] = static_cast<Ogre::uint16>(indexData[1]);
                boneIndices[2] = static_cast<Ogre::uint16>(indexData[2]);
                boneIndices[3] = static_cast<Ogre::uint16>(indexData[3]);
            }
            else if (requests[2].type == Ogre::VET_USHORT4)
            {
                // For skeletons with > 255 bones
                const Ogre::uint16* indexData = reinterpret_cast<const Ogre::uint16*>(requests[2].data);
                boneIndices[0] = indexData[0];
                boneIndices[1] = indexData[1];
                boneIndices[2] = indexData[2];
                boneIndices[3] = indexData[3];
            }
            else if (requests[2].type == Ogre::VET_SHORT4)
            {
                const Ogre::int16* indexData = reinterpret_cast<const Ogre::int16*>(requests[2].data);
                boneIndices[0] = static_cast<Ogre::uint16>(indexData[0]);
                boneIndices[1] = static_cast<Ogre::uint16>(indexData[1]);
                boneIndices[2] = static_cast<Ogre::uint16>(indexData[2]);
                boneIndices[3] = static_cast<Ogre::uint16>(indexData[3]);
            }

            // Check all bone influences (up to 4 per vertex)
            bool vertexAdded = false;
            for (int influenceIdx = 0; influenceIdx < 4 && !vertexAdded; ++influenceIdx)
            {
                if (boneIndices[influenceIdx] == static_cast<Ogre::uint16>(targetBoneIndex) && weights[influenceIdx] >= minWeight)
                {
                    Ogre::Vector3 localPos = invMatrix * worldPos;
                    vertexVector.push_back(localPos);
                    vertexAdded = true;
                }
            }

            // Advance data pointers by stride (same pattern as MeshModifyComponent)
            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
            requests[1].data += requests[1].vertexBuffer->getBytesPerElement();
            requests[2].data += requests[2].vertexBuffer->getBytesPerElement();
        }

        // Unmap async tickets
        mutableVao->unmapAsyncTickets(requests);
    }

    // Main function to get weighted bone convex hull for V2
    OgreNewt::CollisionPtr PhysicsComponent::getWeightedBoneConvexHullV2(Ogre::Bone* bone, Ogre::Item* item, Ogre::Real minWeight, Ogre::Vector3& inertia, Ogre::Vector3& massOrigin, unsigned int categoryId, const Ogre::Vector3& offsetPosition,
        const Ogre::Quaternion& offsetOrientation, const Ogre::Vector3& scale)
    {
        std::vector<Ogre::Vector3> vertexVector;

        if (nullptr == bone || nullptr == item)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsComponent] Invalid bone or item provided");

            OgreNewt::CollisionPrimitives::Box* tempCol = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, Ogre::Vector3(0.1f, 0.1f, 0.1f) * scale, categoryId, offsetOrientation, offsetPosition);
            tempCol->calculateInertialMatrix(inertia, massOrigin);
            return OgreNewt::CollisionPtr(tempCol);
        }

        Ogre::SkeletonInstance* skelInstance = item->getSkeletonInstance();
        const Ogre::MeshPtr& mesh = item->getMesh();

        if (nullptr == skelInstance || mesh.isNull())
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsComponent] Item has no skeleton or mesh");

            OgreNewt::CollisionPrimitives::Box* tempCol = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, Ogre::Vector3(0.1f, 0.1f, 0.1f) * scale, categoryId, offsetOrientation, offsetPosition);
            tempCol->calculateInertialMatrix(inertia, massOrigin);
            return OgreNewt::CollisionPtr(tempCol);
        }

        size_t boneIndex = this->findBoneIndex(skelInstance, bone);
        if (size_t(-1) == boneIndex)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsComponent] Bone not found in skeleton");

            OgreNewt::CollisionPrimitives::Box* tempCol = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, Ogre::Vector3(0.1f, 0.1f, 0.1f) * scale, categoryId, offsetOrientation, offsetPosition);
            tempCol->calculateInertialMatrix(inertia, massOrigin);
            return OgreNewt::CollisionPtr(tempCol);
        }

        Ogre::String boneName = bone->getName();

        // Bind pose
        skelInstance->resetToPose();
        skelInstance->update();

        // Full bone transform at bind pose (affine 3x4)
        Ogre::Matrix4 fullBindPoseMat4;
        bone->_getFullTransform().store(&fullBindPoseMat4);

        // Inverse bind pose matrix
        Ogre::Matrix4 invBindPoseMat4 = fullBindPoseMat4.inverse();

        // Decompose inverse bind pose (matches v1's *_getBindingPoseInverseX)
        Ogre::Vector3 invPos;
        Ogre::Vector3 invScale;
        Ogre::Quaternion invOrient;
        invBindPoseMat4.decomposition(invPos, invScale, invOrient);

        // Replicate v1 math exactly
        Ogre::Vector3 bonePosition = (-invPos) - offsetPosition;
        Ogre::Vector3 boneScale = Ogre::Vector3::UNIT_SCALE / (invScale * scale);
        Ogre::Quaternion boneOrientation = offsetOrientation * invOrient.Inverse();

        Ogre::Matrix4 invMatrix;
        invMatrix.makeInverseTransform(bonePosition, boneScale, boneOrientation);

        // Extract vertices
        unsigned int numSubMeshes = mesh->getNumSubMeshes();
        for (unsigned int subMeshIdx = 0; subMeshIdx < numSubMeshes; ++subMeshIdx)
        {
            const Ogre::SubMesh* subMesh = mesh->getSubMesh(subMeshIdx);
            const Ogre::VertexArrayObjectArray& vaos = subMesh->mVao[Ogre::VpNormal];

            for (const Ogre::VertexArrayObject* vao : vaos)
            {
                this->extractVerticesFromVAO(vao, boneIndex, minWeight, invMatrix, vertexVector);
            }
        }

        unsigned int numVerts = static_cast<unsigned int>(vertexVector.size());
        if (0 == numVerts)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                "[PhysicsComponent] Can not generate convex hull from bone weight for bone: '" + boneName + "' and game object: '" + this->gameObjectPtr->getName() + "', because there are no bone assigned vertices, hence creating a box hull.");

            OgreNewt::CollisionPrimitives::Box* tempCol = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, Ogre::Vector3(0.1f, 0.1f, 0.1f) * boneScale, categoryId, boneOrientation, bonePosition);
            tempCol->calculateInertialMatrix(inertia, massOrigin);
            return OgreNewt::CollisionPtr(tempCol);
        }

        Ogre::Vector3* verts = new Ogre::Vector3[numVerts];
        for (unsigned int i = 0; i < numVerts; ++i)
        {
            verts[i] = vertexVector[i];
        }

        OgreNewt::CollisionPrimitives::ConvexHull* tempCol = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, verts, numVerts, categoryId);
        tempCol->calculateInertialMatrix(inertia, massOrigin);
        OgreNewt::CollisionPtr col = OgreNewt::CollisionPtr(tempCol);

        delete[] verts;

        return col;
    }

    // maybe move this to physicsArtifactComp
    OgreNewt::CollisionPtr PhysicsComponent::serializeTreeCollision(const Ogre::String& scenePath, unsigned int categoryId, bool overwrite)
    {
        if (true == scenePath.empty())
        {
            return OgreNewt::CollisionPtr();
        }

        Ogre::String serializeCollisionPath = scenePath;

        Ogre::String meshName;
        if (GameObject::ENTITY == this->gameObjectPtr->getType())
        {
            meshName = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>()->getMesh()->getName();
        }
        else
        {
            meshName = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>()->getMesh()->getName();
        }

        serializeCollisionPath += "/";
        serializeCollisionPath += meshName;
        //"../media/TestWorld/Meshname.ply"
        serializeCollisionPath += ".ply";

        // Check if serialized collision file does exist
        FILE* file;
        file = fopen(serializeCollisionPath.c_str(), "rb");

        if (nullptr == file || true == overwrite)
        {
            OgreNewt::CollisionSerializer saveWorldCollision;
            Ogre::v1::Entity* entity = nullptr;
            Ogre::Item* item = nullptr;

            if (GameObject::ENTITY == this->gameObjectPtr->getType())
            {
                entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
                this->collisionPtr = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, entity, true, categoryId));
            }
            else if (GameObject::ITEM == this->gameObjectPtr->getType())
            {
                item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
                this->collisionPtr = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, item, true, categoryId));
            }

            if (nullptr == this->collisionPtr)
            {
                return OgreNewt::CollisionPtr();
            }

            Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Writing collision for tree in file:" + serializeCollisionPath);
            // Export collision file for faster loading
            saveWorldCollision.exportCollision(this->collisionPtr, serializeCollisionPath);

            boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
        }

        if (false == overwrite)
        {
            file = fopen(serializeCollisionPath.c_str(), "rb");
            if (nullptr == file)
            {
                Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Could not open the object tree collision file!");

                Ogre::v1::Entity* entity = nullptr;
                Ogre::Item* item = nullptr;

                if (GameObject::ENTITY == this->gameObjectPtr->getType())
                {
                    entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
                    return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, entity, true, categoryId));
                }
                else if (GameObject::ITEM == this->gameObjectPtr->getType())
                {
                    item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
                    return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, item, true, categoryId));
                }
            }
            else
            {
                Ogre::FileHandleDataStream streamFile(file, Ogre::DataStream::READ);
                if (streamFile.size() <= 0)
                {
                    // File is thrash, must be recreated
                    OgreNewt::CollisionSerializer saveWorldCollision;
                    Ogre::v1::Entity* entity = nullptr;
                    Ogre::Item* item = nullptr;

                    if (GameObject::ENTITY == this->gameObjectPtr->getType())
                    {
                        entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
                        this->collisionPtr = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, entity, true, categoryId));
                    }
                    else if (GameObject::ITEM == this->gameObjectPtr->getType())
                    {
                        item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
                        this->collisionPtr = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, item, true, categoryId));
                    }

                    if (nullptr == this->collisionPtr)
                    {
                        return OgreNewt::CollisionPtr();
                    }

                    Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Writing collision for tree in file:" + serializeCollisionPath);
                    // Export collision file for faster loading
                    saveWorldCollision.exportCollision(this->collisionPtr, serializeCollisionPath);

                    boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
                }
                else
                {
                    OgreNewt::CollisionSerializer loadWorldCollision;
                    // Import collision from file for faster loading
                    this->collisionPtr = loadWorldCollision.importCollision(streamFile, ogreNewt);
                }

                streamFile.close();
                fclose(file);
            }
        }

        return this->collisionPtr;
    }

    OgreNewt::CollisionPtr PhysicsComponent::serializeCompoundCollision(const Ogre::String& scenePath, const std::vector<OgreNewt::CollisionPtr>& childCollisions, const Ogre::String& collisionName, unsigned int categoryId, bool overwrite)
    {
        if (scenePath.empty())
        {
            Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Empty scene path for compound serialization", Ogre::LML_CRITICAL);
            return OgreNewt::CollisionPtr();
        }

        if (childCollisions.empty())
        {
            Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] No child collisions provided for compound", Ogre::LML_CRITICAL);
            return OgreNewt::CollisionPtr();
        }

        Ogre::String serializeCollisionPath = scenePath + "/" + collisionName + ".ply";

        // Check if serialized collision file exists
        FILE* file = fopen(serializeCollisionPath.c_str(), "rb");

        if (nullptr == file || true == overwrite)
        {
            if (file)
            {
                fclose(file);
            }

            auto start = std::chrono::high_resolution_clock::now();

            // Create compound collision
            this->collisionPtr = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, childCollisions, categoryId));

            if (nullptr == this->collisionPtr)
            {
                Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Failed to create compound collision", Ogre::LML_CRITICAL);
                return OgreNewt::CollisionPtr();
            }

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = finish - start;

            Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Created compound with " + Ogre::StringConverter::toString(childCollisions.size()) + " children in " + Ogre::StringConverter::toString(elapsed.count() * 0.001) + "s",
                Ogre::LML_CRITICAL);

            // Serialize to .ply file
            OgreNewt::CollisionSerializer saveCompoundCollision;
            Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Writing compound collision to file: " + serializeCollisionPath);

            saveCompoundCollision.exportCollision(this->collisionPtr, serializeCollisionPath);

            boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
        }

        if (false == overwrite)
        {
            file = fopen(serializeCollisionPath.c_str(), "rb");
            if (nullptr == file)
            {
                Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Could not open compound collision file: " + serializeCollisionPath, Ogre::LML_CRITICAL);

                // Fallback: create compound directly
                return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, childCollisions, categoryId));
            }
            else
            {
                Ogre::FileHandleDataStream streamFile(file, Ogre::DataStream::READ);

                if (streamFile.size() <= 0)
                {
                    // File is trash, must be recreated
                    auto start = std::chrono::high_resolution_clock::now();

                    this->collisionPtr = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, childCollisions, categoryId));

                    if (nullptr == this->collisionPtr)
                    {
                        streamFile.close();
                        fclose(file);
                        return OgreNewt::CollisionPtr();
                    }

                    auto finish = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> elapsed = finish - start;

                    Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Recreated compound (corrupted file) in " + Ogre::StringConverter::toString(elapsed.count() * 0.001) + "s");

                    OgreNewt::CollisionSerializer saveCompoundCollision;
                    saveCompoundCollision.exportCollision(this->collisionPtr, serializeCollisionPath);

                    boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
                }
                else
                {
                    // Import from file
                    Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Loading compound collision from: " + serializeCollisionPath);

                    OgreNewt::CollisionSerializer loadCompoundCollision;
                    this->collisionPtr = loadCompoundCollision.importCollision(streamFile, this->ogreNewt);
                }

                streamFile.close();
                fclose(file);
            }
        }

        return this->collisionPtr;
    }

    OgreNewt::CollisionPtr PhysicsComponent::serializeHeightFieldCollision(const Ogre::String& scenePath, unsigned int categoryId, Ogre::Terra* terra, bool overwrite)
    {
        if (true == scenePath.empty())
        {
            return OgreNewt::CollisionPtr();
        }

        Ogre::String serializeCollisionPath = scenePath;
        serializeCollisionPath += "/";
        serializeCollisionPath += this->gameObjectPtr->getName();
        //"../media/TestWorld/gameObjectName.ply"
        serializeCollisionPath += ".ply";

        // Check if serialized collision file does exist
        FILE* file;
        file = fopen(serializeCollisionPath.c_str(), "rb");

        if (nullptr == file || true == overwrite)
        {
            this->collisionPtr = this->createHeightFieldCollision(terra);

            if (nullptr == this->collisionPtr)
            {
                return OgreNewt::CollisionPtr();
            }

            Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Writing collision for tree in file:" + serializeCollisionPath);
            // Export collision file for faster loading

            OgreNewt::CollisionSerializer saveWorldCollision;
            saveWorldCollision.exportCollision(this->collisionPtr, serializeCollisionPath);

            boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);

            return this->collisionPtr;
        }

        if (false == overwrite)
        {
            file = fopen(serializeCollisionPath.c_str(), "rb");
            if (nullptr == file)
            {
                this->collisionPtr = this->createHeightFieldCollision(terra);
            }
            else
            {
                Ogre::FileHandleDataStream streamFile(file, Ogre::DataStream::READ);
                // File to small (thrash)
                if (streamFile.size() <= 0)
                {
                    this->collisionPtr = this->createHeightFieldCollision(terra);

                    if (nullptr == this->collisionPtr)
                    {
                        return OgreNewt::CollisionPtr();
                    }

                    Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Writing collision for tree in file:" + serializeCollisionPath);
                    // Export collision file for faster loading

                    OgreNewt::CollisionSerializer saveWorldCollision;
                    saveWorldCollision.exportCollision(this->collisionPtr, serializeCollisionPath);

                    boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
                }
                else
                {
                    OgreNewt::CollisionSerializer loadWorldCollision;
                    // Import collision from file for faster loading
                    this->collisionPtr = loadWorldCollision.importCollision(streamFile, ogreNewt);
                }

                streamFile.close();
                fclose(file);
            }
        }

        return this->collisionPtr;
    }

    OgreNewt::CollisionPtr PhysicsComponent::createHeightFieldCollision(Ogre::Terra* terra)
    {
        int sizeX = (int)terra->getXZDimensions().x;
        int sizeZ = (int)terra->getXZDimensions().y;
        auto* elevation = new Ogre::Real[sizeX * sizeZ];
        std::fill(elevation, elevation + sizeX * sizeZ, 0.0f);

        const Ogre::Vector3 origin = terra->getTerrainOrigin();

        const Ogre::Real cellSizeX = 1.0f;
        const Ogre::Real cellSizeZ = 1.0f;

        for (int z = 0; z < sizeZ; ++z)
        {
            for (int x = 0; x < sizeX; ++x)
            {
                Ogre::Vector3 p(origin.x + x * cellSizeX, 0.0f, origin.z + z * cellSizeZ);
                bool ok = terra->getHeightAt(p); // p.y gets filled with WORLD height

                // FIX: Subtract origin.y to get height relative to terrain's local origin
                // because getHeightAt returns world height (includes m_terrainOrigin.y)
                const Ogre::Real h = ok ? (p.y - origin.y) : 0.0f;
                elevation[z * sizeX + x] = h;
            }
        }

        Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
        // Position should be the terrain origin (including Y)
        Ogre::Vector3 position = origin;

        auto col = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::HeightField(this->ogreNewt, sizeX, sizeZ, elevation,
            /*verticalScale*/ 1.0f,
            /*horizontalScaleX*/ cellSizeX,
            /*horizontalScaleZ*/ cellSizeZ, position, orientation, this->gameObjectPtr->getCategoryId()));

        delete[] elevation;
        return col;
    }

    void PhysicsComponent::destroyCollision(void)
    {
        if (nullptr != this->physicsBody)
        {
            // this->physicsBody->setCollision(OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Null(this->ogreNewt)));
            if (nullptr != this->collisionPtr)
            {
                // Newton does it already, if body is destroyed, so its prohibited to do it!
                // this->collisionPtr->getNewtonCollision()->Release();
                this->collisionPtr.reset();
            }
        }
    }

    void PhysicsComponent::destroyBody(void)
    {
        if (nullptr != this->physicsBody)
        {
            Ogre::Node* node = this->gameObjectPtr ? this->gameObjectPtr->getSceneNode() : nullptr;

            boost::shared_ptr<EventDataDeleteBody> deleteBodyEvent(boost::make_shared<EventDataDeleteBody>(this->physicsBody));
            AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(deleteBodyEvent);

            // Dangerous: If there is a joint and the body is destroyed, all constraints are destroyed by newton automatically!
            // And joint pointer will become invalid! Hence release the joints
            unsigned int i = 0;
            boost::shared_ptr<JointComponent> jointCompPtr = nullptr;
            do
            {
                jointCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentWithOccurrence<JointComponent>(i));
                if (nullptr != jointCompPtr)
                {
                    jointCompPtr->releaseJoint();
                    i++;
                }
            } while (nullptr != jointCompPtr);

            this->physicsBody->setRenderUpdateCallback(nullptr);
            this->physicsBody->removeForceAndTorqueCallback();
            this->physicsBody->removeNodeUpdateNotify();
            this->physicsBody->detachNode();
            this->physicsBody->removeDestructorCallback();
            delete this->physicsBody;
            this->physicsBody = nullptr;

            // Really important! Remove the tracked node, else there is a transform mess next time!
            if (nullptr != node)
            {
                GraphicsModule::getInstance()->removeTrackedNode(node);
            }
        }
    }

    void PhysicsComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    void PhysicsComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (GameObject::AttrPosition() == attribute->getName())
        {
            this->setPosition(attribute->getVector3());
        }
        else if (GameObject::AttrScale() == attribute->getName())
        {
            this->setScale(attribute->getVector3());
        }
        else if (GameObject::AttrOrientation() == attribute->getName())
        {
            this->setOrientation(MathHelper::getInstance()->degreesToQuat(attribute->getVector3()));
        }
    }

    void PhysicsComponent::setBody(OgreNewt::Body* body)
    {
        this->physicsBody = body;
    }

    OgreNewt::Body* PhysicsComponent::getBody(void) const
    {
        return this->physicsBody;
    }

    void PhysicsComponent::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
    {
        this->setPosition(Ogre::Vector3(x, y, z));
    }

    void PhysicsComponent::setPosition(const Ogre::Vector3& position)
    {
        this->gameObjectPtr->setAttributePosition(position);
        if (nullptr != this->physicsBody)
        {
            this->physicsBody->setPositionOrientation(position, this->getOrientation());
        }
    }

    void PhysicsComponent::translate(const Ogre::Vector3& relativePosition)
    {
        this->gameObjectPtr->setAttributePosition(this->gameObjectPtr->getSceneNode()->_getDerivedPosition() + relativePosition);
        if (nullptr != this->physicsBody)
        {
            this->physicsBody->setPositionOrientation(this->gameObjectPtr->getSceneNode()->_getDerivedPosition() + relativePosition, this->getOrientation());
        }
    }

    void PhysicsComponent::rotate(const Ogre::Quaternion& relativeRotation)
    {
        this->gameObjectPtr->setAttributeOrientation(this->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * relativeRotation);
        if (nullptr != this->physicsBody)
        {
            this->physicsBody->setPositionOrientation(this->gameObjectPtr->getSceneNode()->_getDerivedPosition(), this->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * relativeRotation);
        }
    }

    Ogre::Vector3 PhysicsComponent::getPosition(void) const
    {
        if (nullptr != this->physicsBody)
        {
            return this->physicsBody->getPosition();
        }
        else
        {
            return Ogre::Vector3::ZERO;
        }
    }

    void PhysicsComponent::setScale(const Ogre::Vector3& scale)
    {
        // Check if the scale really changed, because reCreateCollision is an heavy operation
        // No: Else if an object is huge, e.g. a scale of 0.003 is not possible
        // if (false == this->gameObjectPtr->getSceneNode()->getScale().positionEquals(scale, 0.01f))
        {
            if (true == MathHelper::getInstance()->vector3Equals(this->gameObjectPtr->getSceneNode()->getScale(), scale))
            {
                return;
            }

            this->gameObjectPtr->setAttributeScale(scale);
            if (nullptr != this->physicsBody)
            {
                this->reCreateCollision(true);
            }
        }
    }

    Ogre::Vector3 PhysicsComponent::getScale(void) const
    {
        return this->gameObjectPtr->getSceneNode()->getScale();
    }

    void PhysicsComponent::setOrientation(const Ogre::Quaternion& orientation)
    {
        this->gameObjectPtr->setAttributeOrientation(orientation);
        if (nullptr != this->physicsBody)
        {
            this->physicsBody->setPositionOrientation(this->physicsBody->getPosition(), orientation);
        }
    }

    Ogre::Quaternion PhysicsComponent::getOrientation(void) const
    {
        if (nullptr != this->physicsBody)
        {
            return this->physicsBody->getOrientation();
        }
        else
        {
            return Ogre::Quaternion::IDENTITY;
        }
    }

    void PhysicsComponent::setDirection(const Ogre::Vector3& direction, const Ogre::Vector3& localDirectionVector)
    {
        if (nullptr != this->physicsBody)
        {
            Ogre::Vector3 targetDir = direction.normalisedCopy();
            Ogre::Quaternion currentOrient = this->physicsBody->getOrientation();

            // Get current local direction relative to world space
            Ogre::Vector3 currentDir = currentOrient * localDirectionVector;
            Ogre::Quaternion targetOrientation;
            if ((currentDir + targetDir).squaredLength() < 0.00005f)
            {
                // Oops, a 180 degree turn (infinite possible rotation axes)
                // Default to yaw i.e. use current UP
                targetOrientation = Ogre::Quaternion(-currentOrient.y, -currentOrient.z, currentOrient.w, currentOrient.x);
            }
            else
            {
                // Derive shortest arc to new direction
                Ogre::Quaternion rotQuat = currentDir.getRotationTo(targetDir);
                targetOrientation = rotQuat * currentOrient;
            }
            this->setOrientation(targetOrientation);
        }
    }

    const Ogre::Vector3 PhysicsComponent::getInitialPosition(void) const
    {
        return this->initialPosition;
    }

    const Ogre::Vector3 PhysicsComponent::getInitialScale(void) const
    {
        return this->initialScale;
    }

    const Ogre::Quaternion PhysicsComponent::getInitialOrientation(void) const
    {
        return this->initialOrientation;
    }

    void PhysicsComponent::setVolume(Ogre::Real volume)
    {
        this->volume = volume;
    }

    Ogre::Real PhysicsComponent::getVolume(void) const
    {
        return this->volume;
    }

    OgreNewt::World* PhysicsComponent::getOgreNewt(void) const
    {
        return this->ogreNewt;
    }

    void PhysicsComponent::setCollisionType(const Ogre::String& collisionType)
    {
        this->collisionType->setListSelectedValue(collisionType);
    }

    const Ogre::String PhysicsComponent::getCollisionType(void) const
    {
        return this->collisionType->getListSelectedValue();
    }

    void PhysicsComponent::setMass(Ogre::Real mass)
    {
        this->mass->setValue(mass);
    }

    Ogre::Real PhysicsComponent::getMass(void) const
    {
        return this->mass->getReal();
    }

    /*std::list<GameObjectPtr>* PhysicsComponent::getTaggedList(void) const
    {
        return this->taggedList;
    }*/

    void PhysicsComponent::localToGlobal(const Ogre::Quaternion& localOrient, const Ogre::Vector3& localPos, Ogre::Quaternion& globalOrient, Ogre::Vector3& globalPos)
    {
        globalOrient = Ogre::Quaternion::IDENTITY;
        globalPos = Ogre::Vector3::ZERO;

        if (nullptr == this->physicsBody)
        {
            return;
        }

        Ogre::Quaternion bodyOrient = Ogre::Quaternion::IDENTITY;
        Ogre::Vector3 bodyPos = Ogre::Vector3::ZERO;

        this->physicsBody->getPositionOrientation(bodyPos, bodyOrient);

        globalPos = (bodyOrient * localPos) + bodyPos;
        globalOrient = bodyOrient * localOrient;
    }

    void PhysicsComponent::globalToLocal(const Ogre::Quaternion& globalOrient, const Ogre::Vector3& globalPos, Ogre::Quaternion& localOrient, Ogre::Vector3& localPos)
    {
        localOrient = Ogre::Quaternion::IDENTITY;
        localPos = Ogre::Vector3::ZERO;

        if (nullptr == this->physicsBody)
        {
            return;
        }

        Ogre::Quaternion bodyOrient = Ogre::Quaternion::IDENTITY;
        Ogre::Vector3 bodyPos = Ogre::Vector3::ZERO;

        this->physicsBody->getPositionOrientation(bodyPos, bodyOrient);

        Ogre::Quaternion bodyOrientInv = bodyOrient.Inverse();

        localOrient = bodyOrientInv * globalOrient;
        localPos = bodyOrientInv * (globalPos - bodyPos);
    }

    void PhysicsComponent::clampAngularVelocity(Ogre::Real clampValue)
    {
        if (nullptr == this->physicsBody)
        {
            return;
        }

        this->physicsBody->clampAngularVelocity(clampValue);
    }

    void PhysicsComponent::setCollidable(bool collidable)
    {
        this->collidable->setValue(collidable);
        if (nullptr != this->physicsBody)
        {
            static_cast<OgreNewt::Body*>(this->physicsBody)->setCollidable(collidable);
        }
    }

    bool PhysicsComponent::getCollidable(void) const
    {
        return this->collidable->getBool();
    }

    Ogre::Vector3 PhysicsComponent::getLastPosition(void) const
    {
        if (nullptr == this->physicsBody)
        {
            return Ogre::Vector3::ZERO;
        }

        return this->physicsBody->getLastPosition();
    }

    Ogre::Quaternion PhysicsComponent::getLastOrientation(void) const
    {
        if (nullptr == this->physicsBody)
        {
            return Ogre::Quaternion::IDENTITY;
        }

        return this->physicsBody->getLastOrientation();
    }

}; // namespace end