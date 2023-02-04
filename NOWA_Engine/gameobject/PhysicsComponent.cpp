#include "NOWAPrecompiled.h"
#include "PhysicsComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/LuaScriptApi.h"
#include "JointComponents.h"
#include "main/AppStateManager.h"

namespace
{
	Ogre::String getDirectoryNameFromFilePathName(const Ogre::String& filePathName)
	{
		size_t pos = filePathName.find_last_of("\\/");
		return (std::string::npos == pos) ? "" : filePathName.substr(0, pos);
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsComponent::PhysicsComponent()
		: GameObjectComponent(),
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
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectPtr->getComponents()->size()); i++)
		{
			auto& priorPhysicsComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsComponent>(i));
			if (nullptr != priorPhysicsComponent && priorPhysicsComponent.get() != this)
			{
				Ogre::String message = "[PhysicsComponent] Cannot add physics component, because the game object: "
					+ this->gameObjectPtr->getName() + " does already have a kind of physics component.";
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

				boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
				return false;
			}
		}
		return true;
	}

	PhysicsComponent::~PhysicsComponent()
	{
		//delete the body
		this->destroyBody();
		/*this->taggedList->clear();
		delete this->taggedList;
		this->taggedList = static_cast<std::list<GameObjectPtr>*>(0);*/
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] Destructor physics component for game object: " + this->gameObjectPtr->getName());
	}

	bool PhysicsComponent::connect(void)
	{
		return true;
	}

	bool PhysicsComponent::disconnect(void)
	{
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->setVelocity(Ogre::Vector3::ZERO);
			this->physicsBody->setOmega(Ogre::Vector3::ZERO);
			this->physicsBody->setForce(Ogre::Vector3::ZERO);
			this->physicsBody->setTorque(Ogre::Vector3::ZERO);
		}
		return true;
	}

	OgreNewt::CollisionPtr PhysicsComponent::createDynamicCollision(Ogre::Vector3& inertia, const Ogre::Vector3& collisionSize, const Ogre::Vector3& collisionPosition,
		const Ogre::Quaternion& collisionOrientation, Ogre::Vector3& massOrigin, unsigned int categoryId)
	{
		Ogre::Vector3 tempCollisionSize = collisionSize;
		if (Ogre::Vector3::ZERO == collisionSize)
			tempCollisionSize = this->gameObjectPtr->getSize();

		OgreNewt::CollisionPtr collisionPtr;
		if (this->collisionType->getListSelectedValue() == "ConvexHull")
		{
			Ogre::v1::Entity* entity = nullptr;
			Ogre::Item* item = nullptr;
			OgreNewt::CollisionPrimitives::ConvexHull* col = nullptr;

			if (GameObject::ENTITY == this->gameObjectPtr->getType())
			{
				entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
				col = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt,
					entity, categoryId, collisionOrientation, collisionPosition, 0.001f/*, this->gameObjectPtr->getSceneNode()->getScale()*/);
			}
			else if (GameObject::ITEM == this->gameObjectPtr->getType())
			{
				item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
				col = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt,
					item, categoryId, collisionOrientation, collisionPosition, 0.001f/*, this->gameObjectPtr->getSceneNode()->getScale()*/);
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
			OgreNewt::CollisionPrimitives::ConcaveHull* col = new OgreNewt::CollisionPrimitives::ConcaveHull(
				this->ogreNewt, this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>(), categoryId, 0.001, this->gameObjectPtr->getSceneNode()->getScale());

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
			// strangly the position must have an offset of size.y / 2
			OgreNewt::CollisionPrimitives::Capsule* col = new OgreNewt::CollisionPrimitives::Capsule(
				this->ogreNewt, tempCollisionSize.x / 2.0f, tempCollisionSize.y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "ChamferCylinder")
		{
			OgreNewt::CollisionPrimitives::ChamferCylinder *col = new OgreNewt::CollisionPrimitives::ChamferCylinder(
				this->ogreNewt, tempCollisionSize.x / 2.0f, tempCollisionSize.y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Cone")
		{
			OgreNewt::CollisionPrimitives::Cone* col = new OgreNewt::CollisionPrimitives::Cone(
				this->ogreNewt, tempCollisionSize.x / 2.0f, tempCollisionSize.y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Cylinder")
		{
			OgreNewt::CollisionPrimitives::Cylinder* col = new OgreNewt::CollisionPrimitives::Cylinder(
				this->ogreNewt, tempCollisionSize.x / 2.0f, tempCollisionSize.y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Ellipsoid") // can also be a sphere
		{
			OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(
				this->ogreNewt, tempCollisionSize / 2.0f, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Pyramid")
		{
			OgreNewt::CollisionPrimitives::Pyramid* col = new OgreNewt::CollisionPrimitives::Pyramid(
				this->ogreNewt, tempCollisionSize, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else
		{
			OgreNewt::CollisionPrimitives::Null* col = new OgreNewt::CollisionPrimitives::Null(this->ogreNewt);
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] calculated volume: "
			+ Ogre::StringConverter::toString(this->volume) + " for: " + this->gameObjectPtr->getName());
		return collisionPtr;
	}

	OgreNewt::CollisionPtr PhysicsComponent::createDynamicCollision(Ogre::Vector3& inertia, const Ogre::Vector3& collisionPosition,
		const Ogre::Quaternion& collisionOrientation, Ogre::Vector3& massOrigin, unsigned int categoryId)
	{
		OgreNewt::CollisionPtr collisionPtr;
		if (this->collisionType->getListSelectedValue() == "ConvexHull")
		{
			Ogre::v1::Entity* entity = nullptr;
			Ogre::Item* item = nullptr;
			OgreNewt::CollisionPrimitives::ConvexHull* col = nullptr;

			if (GameObject::ENTITY == this->gameObjectPtr->getType())
			{
				entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
				col = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt,
					entity, categoryId, collisionOrientation, collisionPosition, 0.001f/*, this->gameObjectPtr->getSceneNode()->getScale()*/);
			}
			else if (GameObject::ITEM == this->gameObjectPtr->getType())
			{
				item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
				col = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt,
					item, categoryId, collisionOrientation, collisionPosition, 0.001f/*, this->gameObjectPtr->getSceneNode()->getScale()*/);
			}

			if (nullptr == col)
			{
				return OgreNewt::CollisionPtr();
			}

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			// calculate interia and mass center of the body
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "ConcaveHull")
		{
			OgreNewt::CollisionPrimitives::ConcaveHull* col = new OgreNewt::CollisionPrimitives::ConcaveHull(
				this->ogreNewt, this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>(), categoryId, 0.001, this->gameObjectPtr->getSceneNode()->getScale());
			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			// calculate interia and mass center of the body
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Box")
		{
			OgreNewt::CollisionPrimitives::Box* col = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, this->gameObjectPtr->getSize(), categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Capsule")
		{
			// strangly the position must have an offset of size.y / 2
			OgreNewt::CollisionPrimitives::Capsule* col = new OgreNewt::CollisionPrimitives::Capsule(
				this->ogreNewt, this->gameObjectPtr->getSize().x / 2.0f, this->gameObjectPtr->getSize().y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "ChamferCylinder")
		{
			OgreNewt::CollisionPrimitives::ChamferCylinder *col = new OgreNewt::CollisionPrimitives::ChamferCylinder(
				this->ogreNewt, this->gameObjectPtr->getSize().x / 2.0f, this->gameObjectPtr->getSize().y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Cone")
		{
			OgreNewt::CollisionPrimitives::Cone* col = new OgreNewt::CollisionPrimitives::Cone(
				this->ogreNewt, this->gameObjectPtr->getSize().x / 2.0f, this->gameObjectPtr->getSize().y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Cylinder")
		{
			OgreNewt::CollisionPrimitives::Cylinder* col = new OgreNewt::CollisionPrimitives::Cylinder(
				this->ogreNewt, this->gameObjectPtr->getSize().x / 2.0f, this->gameObjectPtr->getSize().y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Ellipsoid") // can also be a sphere
		{
			OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(
				this->ogreNewt, this->gameObjectPtr->getSize() / 2.0f, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else if (this->collisionType->getListSelectedValue() == "Pyramid")
		{
			OgreNewt::CollisionPrimitives::Pyramid* col = new OgreNewt::CollisionPrimitives::Pyramid(
				this->ogreNewt, this->gameObjectPtr->getSize(), categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
			col->calculateInertialMatrix(inertia, massOrigin);
		}
		else
		{
			OgreNewt::CollisionPrimitives::Null* col = new OgreNewt::CollisionPrimitives::Null(this->ogreNewt);
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] calculated volume: "
			+ Ogre::StringConverter::toString(this->volume) + " for: " + this->gameObjectPtr->getName());
		return collisionPtr;
	}

	OgreNewt::CollisionPtr PhysicsComponent::createCollisionPrimitive(const Ogre::String& _collisionType, const Ogre::Vector3& collisionPosition,
		const Ogre::Quaternion collisionOrientation, const Ogre::Vector3& collisionSize, Ogre::Vector3& inertia, Ogre::Vector3& massOrigin, unsigned categoryId)
	{
		// convex hull is not possible, because an entity would be required
		if (_collisionType == "ConvexHull")
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] Could not create convex hull collision, because this type is no primitive type!");
			return OgreNewt::CollisionPtr();
		}
		else if (_collisionType == "ConcaveHull")
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] Could not create concave hull collision, because this type is no primitive type!");
			return OgreNewt::CollisionPtr();
		}
		
		OgreNewt::CollisionPtr collisionPtr;

		if (_collisionType == "Box")
		{
			OgreNewt::CollisionPrimitives::Box* col = new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, collisionSize, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			col->calculateInertialMatrix(inertia, massOrigin);
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		else if (_collisionType == "Capsule")
		{
			// strangly the position must have an offset of size.y / 2
			OgreNewt::CollisionPrimitives::Capsule* col = new OgreNewt::CollisionPrimitives::Capsule(
				this->ogreNewt, collisionSize.x, collisionSize.y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			col->calculateInertialMatrix(inertia, massOrigin);
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		else if (_collisionType == "ChamferCylinder")
		{
			OgreNewt::CollisionPrimitives::ChamferCylinder* col = new OgreNewt::CollisionPrimitives::ChamferCylinder(
				this->ogreNewt, collisionSize.x, collisionSize.y, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		else if (_collisionType == "Cone")
		{
			OgreNewt::CollisionPrimitives::Cone* col = new OgreNewt::CollisionPrimitives::Cone(
				this->ogreNewt, collisionSize.x, collisionSize.y, categoryId, collisionOrientation, collisionPosition);

			col->calculateInertialMatrix(inertia, massOrigin);
			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		else if (_collisionType == "Cylinder")
		{
			OgreNewt::CollisionPrimitives::Cylinder* col = new OgreNewt::CollisionPrimitives::Cylinder(
				this->ogreNewt, collisionSize.x, collisionSize.y, categoryId, collisionOrientation, collisionPosition);

			col->calculateInertialMatrix(inertia, massOrigin);
			this->volume = col->calculateVolume();
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		else if (_collisionType == "Ellipsoid") // can also be a sphere
		{
			OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(
				this->ogreNewt, collisionSize, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			col->calculateInertialMatrix(inertia, massOrigin);
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		else if (_collisionType == "Pyramid")
		{
			OgreNewt::CollisionPrimitives::Pyramid* col = new OgreNewt::CollisionPrimitives::Pyramid(
				this->ogreNewt, collisionSize, categoryId, collisionOrientation, collisionPosition);

			this->volume = col->calculateVolume();
			col->calculateInertialMatrix(inertia, massOrigin);
			collisionPtr = OgreNewt::CollisionPtr(col);
		}
		/*else
		{
			OgreNewt::CollisionPrimitives::Null* col = new OgreNewt::CollisionPrimitives::Null(this->ogreNewt);
			collisionPtr = OgreNewt::CollisionPtr(col);
		}*/

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsComponent] calculated volume: "
			+ Ogre::StringConverter::toString(this->volume) + " for: " + this->gameObjectPtr->getName());
		return collisionPtr;
	}

	OgreNewt::CollisionPtr PhysicsComponent::createDeformableCollision(OgreNewt::CollisionPtr collisionPtr)
	{
		OgreNewt::CollisionPrimitives::TetrahedraDeformableCollision* deformableCollision = new OgreNewt::CollisionPrimitives::TetrahedraDeformableCollision(
			this->ogreNewt, collisionPtr->getNewtonCollision(), 0);

		collisionPtr = OgreNewt::CollisionPtr(deformableCollision);
		return collisionPtr;
	}

	OgreNewt::CollisionPtr PhysicsComponent::getWeightedBoneConvexHull(Ogre::v1::OldBone* bone, Ogre::v1::MeshPtr mesh, Ogre::Real minWeight,
		Ogre::Vector3& inertia, Ogre::Vector3& massOrigin, unsigned int categoryId, const Ogre::Vector3& offsetPosition, const Ogre::Quaternion& offsetOrientation,
		const Ogre::Vector3& scale)
	{
		std::vector<Ogre::Vector3> vertexVector;

		// for this bone, gather all of the vertices linked to it, and make an individual convex hull.
		std::string boneName = bone->getName();
		unsigned int boneIndex = bone->getHandle();

		Ogre::Vector3 bonePosition = (-bone->_getBindingPoseInversePosition()) - offsetPosition;
		Ogre::Vector3 bonescale = Ogre::Vector3::UNIT_SCALE / (bone->_getBindingPoseInverseScale() * scale);
		Ogre::Quaternion boneOrientation = offsetOrientation * bone->_getBindingPoseInverseOrientation().Inverse();

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
			/*Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsComponent] Error: Can not generate convex hull from bone weight for bone: '"
				+ bone->getName() + "' and game object: '" + this->gameObjectPtr->getName() + "', because there are no bone assigned vertices.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsComponent] Error: Can not generate convex hull from bone weight for bone: '"
				+ bone->getName() + "' and game object: '" + this->gameObjectPtr->getName() + "', because there are no bone assigned vertices.\n", "NOWA");*/

			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsComponent] Can not generate convex hull from bone weight for bone: '"
				+ bone->getName() + "' and game object: '" + this->gameObjectPtr->getName() + "', because there are no bone assigned vertices, hence creating a box hull.");
			OgreNewt::CollisionPrimitives::Box* tempCol = new OgreNewt::CollisionPrimitives::Box(
				this->ogreNewt, Ogre::Vector3(0.1f, 0.1f, 0.1f) * bonescale, categoryId, boneOrientation, bonePosition);
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

		//////////////////////////////////////////////////////////////////////////////////
		OgreNewt::CollisionPrimitives::ConvexHull* tempCol = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, verts, numVerts, categoryId);
		tempCol->calculateInertialMatrix(inertia, massOrigin);
		OgreNewt::CollisionPtr col = OgreNewt::CollisionPtr(tempCol);

		delete[] verts;

		return col;
	}

	// maybe move this to physicsArtifactComp
	OgreNewt::CollisionPtr PhysicsComponent::serializeTreeCollision(const Ogre::String& worldPath, unsigned int categoryId)
	{
		OgreNewt::CollisionPtr col;
		Ogre::String serializeCollisionPath = getDirectoryNameFromFilePathName(worldPath);

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
		//"../media/TestWorld/Meshname.col"
		serializeCollisionPath += ".col";

		// Check if serialized collision file does exist
		FILE* file;
		file = fopen(serializeCollisionPath.c_str(), "rb");
		
		if (nullptr == file)
		{
			OgreNewt::CollisionSerializer saveWorldCollision;
			Ogre::v1::Entity* entity = nullptr;
			Ogre::Item* item = nullptr;
			
			if (GameObject::ENTITY == this->gameObjectPtr->getType())
			{
				entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
				col = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, entity, true, categoryId));
			}
			else if (GameObject::ITEM == this->gameObjectPtr->getType())
			{
				item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
				col = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, item, true, categoryId));
			}

			if (nullptr == col)
			{
				return OgreNewt::CollisionPtr();
			}
			
			Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Writing collision for tree in file:" + serializeCollisionPath);
			// Export collision file for faster loading
			saveWorldCollision.exportCollision(col, serializeCollisionPath);
		}

		file = fopen(serializeCollisionPath.c_str(), "rb");
		if (nullptr == file)
		{
			Ogre::LogManager::getSingleton().logMessage("[PhysicsComponent] Could not open the object tree collision file!");
			
			Ogre::v1::Entity* entity = nullptr;
			Ogre::Item* item = nullptr;

			if (GameObject::ENTITY == this->gameObjectPtr->getType())
			{
				entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
				return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt,
					entity, true, categoryId));
			}
			else if (GameObject::ITEM == this->gameObjectPtr->getType())
			{
				item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
				return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt,
					item, true, categoryId));
			}
		}

		Ogre::FileHandleDataStream streamFile(file, Ogre::DataStream::READ);
		OgreNewt::CollisionSerializer loadWorldCollision;
		// Import collision from file for faster loading
		col = loadWorldCollision.importCollision(streamFile, ogreNewt);

		streamFile.close();
		fclose(file);

		return col;
	}

	void PhysicsComponent::destroyCollision(void)
	{
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->setCollision(OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Null(this->ogreNewt)));
			// this->physicsBody->setCollision(nullptr);
		}
	}

	void PhysicsComponent::destroyBody(void)
	{
		if (nullptr != this->physicsBody)
		{
			boost::shared_ptr<EventDataDeleteBody> deleteBodyEvent(boost::make_shared<EventDataDeleteBody>(this->physicsBody));
			AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(deleteBodyEvent);

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

			this->physicsBody->removeForceAndTorqueCallback();
			this->physicsBody->removeNodeUpdateNotify();
			this->physicsBody->detachNode();
			this->physicsBody->removeDestructorCallback();
			delete this->physicsBody;
			this->physicsBody = nullptr;
		}
	}

	void PhysicsComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
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
			this->physicsBody->setPositionOrientation(position, this->getOrientation());
	}

	void PhysicsComponent::translate(const Ogre::Vector3& relativePosition)
	{
		//TODO: set attribute translate, or get attribute position + position
		if (nullptr != this->physicsBody)
			this->physicsBody->setPositionOrientation(this->physicsBody->getPosition() + relativePosition, this->getOrientation());
	}

	void PhysicsComponent::rotate(const Ogre::Quaternion& relativeRotation)
	{
		this->gameObjectPtr->setAttributeOrientation(this->getOrientation() * relativeRotation);
		if (nullptr != this->physicsBody)
			this->physicsBody->setPositionOrientation(this->physicsBody->getPosition(), this->getOrientation() * relativeRotation);
	}

	Ogre::Vector3 PhysicsComponent::getPosition(void) const
	{
		if (nullptr != this->physicsBody)
			return this->physicsBody->getPosition();
		else
			return Ogre::Vector3::ZERO;
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
			this->gameObjectPtr->getSceneNode()->setScale(scale);
			if (nullptr != this->physicsBody)
			{
				this->reCreateCollision();
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
			this->physicsBody->setPositionOrientation(this->physicsBody->getPosition(), orientation);
	}

	Ogre::Quaternion PhysicsComponent::getOrientation(void) const
	{
		if (nullptr != this->physicsBody)
			return this->physicsBody->getOrientation();
		else
			return Ogre::Quaternion::IDENTITY;
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
			return;

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
			return;

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
			return;
	
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

}; // namespace end