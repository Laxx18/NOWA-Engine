#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_ContactJoint.h"

//#include "OgreItem.h"
//#include "OgreMesh.h"
//#include "OgreMeshManager.h"
//#include "OgreMesh2.h"
//#include "OgreMeshManager2.h"
//#include "OgreSubMesh2.h"
//#include "OgreMesh2Serializer.h"
//
//#include "OgreRoot.h"
//#include "Vao/OgreVaoManager.h"
//#include "Vao/OgreVertexArrayObject.h"
//#include "OgreRenderWindow.h"

namespace OgreNewt
{
	Body::Body(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& collisionPtr, Ogre::SceneMemoryMgrTypes memoryType)
		: m_world(world),
		m_sceneManager(sceneManager),
		m_sceneMemoryType(memoryType),
		m_categoryType(0),
		m_node(nullptr),
		m_matid(nullptr),
		m_userdata(0),
		m_userdata2(0),
		m_nodePosit(Ogre::Vector3::ZERO),
		m_curPosit(Ogre::Vector3::ZERO),
		m_prevPosit(Ogre::Vector3::ZERO),
		m_curRotation(Ogre::Quaternion::IDENTITY),
		m_prevRotation(Ogre::Quaternion::IDENTITY),
		m_nodeRotation(Ogre::Quaternion::IDENTITY),
		m_updateRotation(false),
		m_validToUpdateStatic(false),
		m_memoryType(memoryType),
		m_debugCollisionLines(nullptr),
		m_debugCollisionItem(nullptr),
		m_gravity(Ogre::Vector3(0.0f, -12.9f, 0.0f)),
		m_isOwner(true),
		m_forcecallback(nullptr),
		m_buoyancycallback(nullptr),
		m_nodeupdatenotifycallback(nullptr),
		m_contactCallback(nullptr)
	{
		float matrix[16] = { 1,0,0,0,
							0,1,0,0,
							0,0,1,0,
							0,0,0,1 };
		// OgreNewt::Converters::QuatPosToMatrix(m_curRotation, m_curPosit, matrix);
		// m_body = NewtonCreateDynamicBody(m_world->getNewtonWorld(), collisionPtr->getNewtonCollision(), matrix);

		dFloat m[16];
		OgreNewt::Converters::QuatPosToMatrix(m_curRotation, m_curPosit, m);

		m_body = NewtonCreateDynamicBody(m_world->getNewtonWorld(), collisionPtr->getNewtonCollision(), &m[0]);

		// What is this?
		// NewtonCreateAsymetricDynamicBody when calculate volume, compound etc.
		// http://newtondynamics.com/forum/viewtopic.php?f=9&t=9336&p=63362&hilit=NewtonCreateAsymetricDynamicBody#p63362

		NewtonBodySetUserData(m_body, this);
		NewtonBodySetDestructorCallback(m_body, newtonDestructor);
		NewtonBodySetTransformCallback(m_body, newtonTransformCallback);

		setLinearDamping(m_world->getDefaultLinearDamping() * (60.0f / m_world->getUpdateFPS()));
		setAngularDamping(m_world->getDefaultAngularDamping() * (60.0f / m_world->getUpdateFPS()));

		NewtonDestroyCollision(collisionPtr->getNewtonCollision());

		// setStandardForceCallback();
	}

	Body::Body(World* world, Ogre::SceneManager* sceneManager, NewtonBody* body, Ogre::SceneMemoryMgrTypes memoryType)
		: m_world(world),
		m_sceneManager(sceneManager),
		m_body(body),
		m_sceneMemoryType(memoryType),
		m_categoryType(0),
		m_node(nullptr),
		m_matid(nullptr),
		m_userdata(0),
		m_userdata2(0),
		m_nodePosit(Ogre::Vector3::ZERO),
		m_curPosit(Ogre::Vector3::ZERO),
		m_prevPosit(Ogre::Vector3::ZERO),
		m_curRotation(Ogre::Quaternion::IDENTITY),
		m_prevRotation(Ogre::Quaternion::IDENTITY),
		m_nodeRotation(Ogre::Quaternion::IDENTITY),
		m_updateRotation(false),
		m_validToUpdateStatic(false),
		m_memoryType(memoryType),
		m_debugCollisionLines(nullptr),
		m_debugCollisionItem(nullptr),
		m_gravity(Ogre::Vector3(0.0f, -12.9f, 0.0f)),
		m_isOwner(false),
		m_forcecallback(nullptr),
		m_buoyancycallback(nullptr),
		m_nodeupdatenotifycallback(nullptr),
		m_contactCallback(nullptr)
	{
		if (nullptr != m_body)
		{
			NewtonBodySetUserData(m_body, this);
			NewtonBodySetTransformCallback(m_body, newtonTransformCallback);
			NewtonBodySetDestructorCallback(m_body, newtonDestructor);
			setLinearDamping(m_world->getDefaultLinearDamping() * (60.0f / m_world->getUpdateFPS()));
			setAngularDamping(m_world->getDefaultAngularDamping() * (60.0f / m_world->getUpdateFPS()));
			// setStandardForceCallback();
		}
	}

	Body::Body(World* world, Ogre::SceneManager* sceneManager, Ogre::SceneMemoryMgrTypes memoryType)
		: m_world(world),
		m_sceneManager(sceneManager),
		m_body(nullptr),
		m_sceneMemoryType(memoryType),
		m_categoryType(0),
		m_node(nullptr),
		m_matid(nullptr),
		m_userdata(0),
		m_userdata2(0),
		m_nodePosit(Ogre::Vector3::ZERO),
		m_curPosit(Ogre::Vector3::ZERO),
		m_prevPosit(Ogre::Vector3::ZERO),
		m_curRotation(Ogre::Quaternion::IDENTITY),
		m_prevRotation(Ogre::Quaternion::IDENTITY),
		m_nodeRotation(Ogre::Quaternion::IDENTITY),
		m_updateRotation(false),
		m_validToUpdateStatic(false),
		m_memoryType(memoryType),
		m_debugCollisionLines(nullptr),
		m_debugCollisionItem(nullptr),
		m_gravity(Ogre::Vector3(0.0f, -12.9f, 0.0f)),
		m_isOwner(true), // Attention: This was false before, but player controller's userdata was corrupt when applyMove when destroyed and recreated
		m_forcecallback(nullptr),
		m_buoyancycallback(nullptr),
		m_nodeupdatenotifycallback(nullptr),
		m_contactCallback(nullptr)
	{

	}

	Body::~Body()
	{
		if (m_body)
		{
			if (nullptr != m_debugCollisionLines)
			{
				Ogre::SceneNode* node = ((Ogre::SceneNode*)m_debugCollisionLines->getParentNode());
				node->detachObject(m_debugCollisionLines);
				// m_sceneManager->destroySceneNode(node);

				delete m_debugCollisionLines;
				m_debugCollisionLines = nullptr;
			}

			/*if (nullptr != m_debugCollisionItem)
			{
				Ogre::SceneNode* node = m_debugCollisionItem->getParentSceneNode();
				node->detachObject(m_debugCollisionItem);
				m_debugCollisionItem = nullptr;
				m_debugCollisionMesh.setNull();
			}*/

			detachNode();
			m_sceneManager = nullptr;
			
			NewtonWaitForUpdateToFinish(m_world->getNewtonWorld());
			if (NewtonBodyGetUserData(m_body))
			{
				// removeForceAndTorqueCallback();
				if (true == m_isOwner)
				{
					NewtonBodySetDestructorCallback(m_body, 0);
					NewtonDestroyBody(m_body);
					NewtonBodySetUserData(m_body, 0);
				}
			}
			m_body = nullptr;
		}
	}

	// destructor callback
	void _CDECL Body::newtonDestructor(const NewtonBody* body)
	{
		//newton wants to destroy the body.. so first find it.
		OgreNewt::Body* me;

		me = (OgreNewt::Body*)NewtonBodyGetUserData(body);

		// remove destructor callback
		NewtonBodySetDestructorCallback(body, 0);
		// remove the user data
		NewtonBodySetUserData(body, 0);

		//now delete the object.
		delete me;
		me = nullptr;
	}


	// transform callback
	void _CDECL Body::newtonTransformCallback(const NewtonBody* body, const float* matrix, int threadIndex)
	{
		Ogre::Real dot;
		OgreNewt::Body* me;

		me = (OgreNewt::Body*) NewtonBodyGetUserData(body);

		me->m_prevPosit = me->m_curPosit;
		me->m_prevRotation = me->m_curRotation;

		if (!Ogre::Math::isNaN(matrix[12]))
		{
			me->m_curPosit = Ogre::Vector3(matrix[12], matrix[13], matrix[14]);
		}
		else
		{
			me->m_curPosit = me->m_prevPosit;
		}

		if (me->m_updateRotation)
		{
			// use the rotation from the body
			// NewtonBodyGetRotation(body, &me->m_curRotation.w);
			NewtonBodyGetRotation(body, &me->m_curRotation.w);
			Ogre::Real temp = me->m_curRotation[3];
			me->m_curRotation[3] = me->m_curRotation[2];
			me->m_curRotation[2] = me->m_curRotation[1];
			me->m_curRotation[1] = me->m_curRotation[0];
			me->m_curRotation[0] = temp;

			dot = me->m_prevRotation.Dot(me->m_curRotation);

			if (dot < 0.0f)
			{
				me->m_prevRotation = -1.0f * me->m_prevRotation;
			}
		}
	}

	void _CDECL Body::newtonForceTorqueCallback(const NewtonBody* const body, dFloat timeStep, int threadIndex)
	{
		OgreNewt::Body* me = (OgreNewt::Body*)NewtonBodyGetUserData(body);

		if (me->m_forcecallback)
			me->m_forcecallback(me, timeStep, threadIndex);
	}

	void Body::standardForceCallback(OgreNewt::Body* me, float timestep, int threadIndex)
	{
		//apply a simple gravity force.
		Ogre::Real mass;
		Ogre::Vector3 inertia;

		me->getMassMatrix(mass, inertia);
		const Ogre::Vector3 gravityAcceleration(me->getGravity());
		const Ogre::Vector3 gravityForce = gravityAcceleration * mass;

		me->addForce(gravityForce);

		const int nth = 0;

		for (auto& it = me->m_accumulatedGlobalForces.cbegin(); it != me->m_accumulatedGlobalForces.cend(); it++)
		{
			//Ogre::LogManager::getSingleton().getDefaultLog()->logMessage("# [" + Ogre::StringConverter::toString(++nth) + "] force " + Ogre::StringConverter::toString(it->first) + " at " + Ogre::StringConverter::toString(it->second) + " to " + me->getOgreNode()->getName());
			me->addGlobalForce(it->first, it->second);
		}

		me->m_accumulatedGlobalForces.clear();
	}


	int _CDECL Body::newtonBuoyancyCallback(const int collisionID, void *context, const float* globalSpaceMatrix, float* globalSpacePlane)
	{
		OgreNewt::Body* me = (OgreNewt::Body*)context;

		Ogre::Quaternion orient;
		Ogre::Vector3 pos;

		OgreNewt::Converters::MatrixToQuatPos(globalSpaceMatrix, orient, pos);

		// call our user' function to get the plane.
		Ogre::Plane theplane;

		if (me->m_buoyancycallback(collisionID, me, orient, pos, theplane))
		{
			globalSpacePlane[0] = theplane.normal.x;
			globalSpacePlane[1] = theplane.normal.y;
			globalSpacePlane[2] = theplane.normal.z;
			globalSpacePlane[3] = theplane.d;

			return 1;
		}

		return 0;
	}

	void Body::setType(unsigned int categoryType)
	{
		m_categoryType = categoryType;
		auto collision = NewtonBodyGetCollision(m_body);
		NewtonCollisionSetUserID(collision, categoryType);
	}

	// attachToNode
	void Body::attachNode(Ogre::Node* node, bool updateRotation)
	{
		m_node = node;
		m_updateRotation = updateRotation;
		updateNode(1.0f);
	}

	void Body::detachNode(void)
	{
		m_node = nullptr;
	}

	void Body::setPositionOrientation(const Ogre::Vector3& pos, const Ogre::Quaternion& orient, int threadIndex)
	{
		if (m_body)
		{
			float matrix[16];

			// reset the previews Position and rotation for this body
			m_curPosit = pos;
			m_prevPosit = pos;
			m_curRotation = orient;
			m_prevRotation = orient;
			// Its valid to update the nodes position and orientation one time, this is for static scene objects, so that they have a chance to change their position orientation manually
			m_validToUpdateStatic = true;

			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);
			NewtonBodySetMatrix(m_body, &matrix[0]);

			updateNode(1.0f);
		}
	}

	// set mass matrix
	void Body::setMassMatrix(Ogre::Real mass, const Ogre::Vector3& inertia)
	{
		if (m_body)
		{
			NewtonBodySetMassMatrix(m_body, (float)mass, (float)inertia.x, (float)inertia.y, (float)inertia.z);
		}
	}

	void Body::setConvexIntertialMatrix(const Ogre::Vector3& inertia, const Ogre::Vector3& massOrigin)
	{
		if (m_body)
		{
			NewtonConvexCollisionCalculateInertialMatrix(getNewtonCollision(), &(float)inertia.x, &(float)massOrigin.x);
		}
	}

	// basic gravity callback
	void Body::setStandardForceCallback()
	{
		setCustomForceAndTorqueCallback(standardForceCallback);
	}

	// custom user force callback
	void Body::setCustomForceAndTorqueCallback(ForceCallback callback)
	{
		if (!m_forcecallback)
		{
			m_forcecallback = callback;
			NewtonBodySetForceAndTorqueCallback(m_body, newtonForceTorqueCallback);
		}
		else
		{
			m_forcecallback = callback;
		}
	}

	//set collision
	void Body::setCollision(const OgreNewt::CollisionPtr& col)
	{
		NewtonBodySetCollision(m_body, col->getNewtonCollision());

		// m_collision = col;
	}

	NewtonCollision* Body::getNewtonCollision(void)
	{
		return NewtonBodyGetCollision(m_body);
	}

	//get material group ID
	const OgreNewt::MaterialID* Body::getMaterialGroupID() const
	{
		if (m_matid)
		{
			return m_matid;
		}
		else
		{
			return m_world->getDefaultMaterialID();
		}
	}


	// get position and orientation
	void Body::getPositionOrientation(Ogre::Vector3& pos, Ogre::Quaternion& orient) const
	{
		pos = m_curPosit;
		orient = m_curRotation;
	}

	Ogre::Vector3 Body::getPosition() const
	{
		return m_curPosit;
	}


	Ogre::Quaternion Body::getOrientation() const
	{
		return m_curRotation;
	}

	//! get the node position and orientation in form of an Ogre::Vector(position) and Ogre::Quaternion(orientation)
	void Body::getVisualPositionOrientation(Ogre::Vector3& pos, Ogre::Quaternion& orient) const
	{
		pos = m_nodePosit;
		orient = m_nodeRotation;
	}

	Ogre::Vector3 Body::getVisualPosition() const
	{
		return m_nodePosit;
	}


	Ogre::Quaternion Body::getVisualOrientation() const
	{
		return m_nodeRotation;
	}

	Ogre::AxisAlignedBox Body::getAABB() const
	{
		Ogre::Real min[3], max[3];
		NewtonBodyGetAABB(m_body, min, max);
		Ogre::AxisAlignedBox box;
		box.setMinimum(min[0], min[1], min[2]);
		box.setMaximum(max[0], max[1], max[2]);
		return box;
	}

	void Body::getMassMatrix(Ogre::Real& mass, Ogre::Vector3& inertia) const
	{
		NewtonBodyGetMass(m_body, &mass, &inertia.x, &inertia.y, &inertia.z);
	}

	Ogre::Real Body::getMass() const
	{
		Ogre::Real mass;
		Ogre::Vector3 inertia;

		NewtonBodyGetMass(m_body, &mass, &inertia.x, &inertia.y, &inertia.z);

		return mass;
	}

	Ogre::Vector3 Body::getInertia() const
	{
		Ogre::Real mass;
		Ogre::Vector3 inertia;

		NewtonBodyGetMass(m_body, &mass, &inertia.x, &inertia.y, &inertia.z);

		return inertia;
	}

	void Body::getInvMass(Ogre::Real& mass, Ogre::Vector3& inertia) const
	{
		NewtonBodyGetInvMass(m_body, &mass, &inertia.x, &inertia.y, &inertia.z);
	}

	Ogre::Vector3 Body::getOmega() const
	{
		Ogre::Vector3 ret;
		NewtonBodyGetOmega(m_body, &ret.x);
		return ret;
	}

	Ogre::Vector3 Body::getVelocity() const
	{
		Ogre::Vector3 ret;
		NewtonBodyGetVelocity(m_body, &ret.x);
		return ret;
	}

	Ogre::Vector3 Body::getVelocityAtPoint(const Ogre::Vector3& point) const
	{
		Ogre::Vector3 velocityOut;

		NewtonBodyGetPointVelocity(m_body, &m_curPosit.x, &velocityOut.x);

		return velocityOut;
	}

	Ogre::Vector3 Body::getForce() const
	{
		Ogre::Vector3 ret;
		NewtonBodyGetForce(m_body, &ret.x);
		return ret;
	}

	Ogre::Vector3 Body::getTorque() const
	{
		Ogre::Vector3 ret;
		NewtonBodyGetTorque(m_body, &ret.x);
		return ret;
	}

	Ogre::Vector3 Body::getAngularDamping() const
	{
		Ogre::Vector3 ret;
		NewtonBodyGetAngularDamping(m_body, &ret.x);
		return ret;
	}

	Ogre::Vector3 Body::getCenterOfMass() const
	{
		Ogre::Vector3 ret;
		NewtonBodyGetCentreOfMass(m_body, &ret.x);
		return ret;
	}

	void Body::enableGyroscopicTorque(bool enable)
	{
		NewtonBodySetGyroscopicTorque(m_body, enable ? 1 : 0);
	}

	void Body::setCollidable(bool collidable)
	{
		// Does not work
		// NewtonBodySetCollidable(m_body, collidable ? 1 : 0);
		const NewtonCollision* const collision = NewtonBodyGetCollision(m_body);
		NewtonCollisionSetMode(collision, collidable ? 1 : 0);
	}

	bool Body::getCollidable(void) const
	{
		const NewtonCollision* const collision = NewtonBodyGetCollision(m_body);
		return NewtonCollisionGetMode(collision) == 0 ? false : true;
	}

	void Body::addGlobalForce(const Ogre::Vector3& force, const Ogre::Vector3& pos)
	{
		Ogre::Vector3 bodypos;
		Ogre::Quaternion bodyorient;
		getPositionOrientation(bodypos, bodyorient);

		const Ogre::Vector3 localMassCenter = getCenterOfMass();
		const Ogre::Vector3 globalMassCenter = bodypos + bodyorient * localMassCenter;

		const Ogre::Vector3 topoint = pos - globalMassCenter;
		const Ogre::Vector3 torque = topoint.crossProduct(force);

		addForce(force);
		addTorque(torque);
	}

	void Body::addGlobalForceAccumulate(const Ogre::Vector3& force, const Ogre::Vector3& pos)
	{
		m_accumulatedGlobalForces.push_back(std::pair<Ogre::Vector3, Ogre::Vector3>(force, pos));
	}

	void Body::addLocalForce(const Ogre::Vector3& force, const Ogre::Vector3& pos)
	{
		Ogre::Vector3 bodypos;
		Ogre::Quaternion bodyorient;

		getPositionOrientation(bodypos, bodyorient);

		const Ogre::Vector3 globalforce = bodyorient * force;
		const Ogre::Vector3 globalpoint = (bodyorient * pos) + bodypos;

		addGlobalForce(globalforce, globalpoint);
	}

	void Body::setBodyAngularVelocity(const Ogre::Vector3& desiredOmega, Ogre::Real timestep)
	{
		dMatrix bodyInertia;
		dVector bodyOmega(0.0f);
		const dVector dDesiredOmega(desiredOmega.x, desiredOmega.y, desiredOmega.z);

		// get body internal data
		NewtonBodyGetOmega(m_body, &bodyOmega[0]);
		NewtonBodyGetInertiaMatrix(m_body, &bodyInertia[0][0]);

		// calculate angular velocity error 
		const dVector omegaError(dDesiredOmega - bodyOmega);

		// calculate impulse
		dVector angularImpulse(bodyInertia.RotateVector(omegaError));

		// apply impulse to achieve desired omega
		dVector linearImpulse(0.0f);
		NewtonBodyApplyImpulsePair(m_body, &linearImpulse[0], &angularImpulse[0], timestep);
	}

	void Body::showDebugCollision(bool isStatic, bool show)
	{
		if (nullptr == m_node)
			return;

		if (true == show)
		{
			const NewtonMesh* debugCollisionMesh = NewtonMeshCreateFromCollision(this->getNewtonCollision());
			NewtonMeshTriangulate(debugCollisionMesh);

			// Static/Dynamic may not be mixed, so recreate if state changed
			bool mustRecreate = false;
			if (nullptr != m_debugCollisionLines)
			{
				mustRecreate = m_debugCollisionLines->isStatic() != isStatic;
			}
			if (true == mustRecreate)
			{
				Ogre::SceneNode* node = ((Ogre::SceneNode*)m_debugCollisionLines->getParentNode());
				node->detachObject(m_debugCollisionLines);
				// m_sceneManager->destroySceneNode(node);

				// delete m_debugCollisionLines;
				m_sceneManager->destroyManualObject(m_debugCollisionLines);
				m_debugCollisionLines = nullptr;
			}

			if (nullptr == m_debugCollisionLines)
			{
				m_debugCollisionLines = m_sceneManager->createManualObject(isStatic ? Ogre::SCENE_STATIC : Ogre::SCENE_DYNAMIC);
				m_debugCollisionLines->clear();
				m_debugCollisionLines->setRenderQueueGroup(200); // V2
				m_debugCollisionLines->setQueryFlags(0 << 0);
				m_debugCollisionLines->setCastShadows(false);
				// Use the scene node that is updated from newtons physics! Else the collision debug hull position, orientation is wrong
				((Ogre::SceneNode*)m_node)->attachObject(m_debugCollisionLines);
			}

			Ogre::Vector3 pos;
			Ogre::Quaternion ori;
			this->getPositionOrientation(pos, ori);

			const int vertexCount = NewtonMeshGetPointCount(debugCollisionMesh);
			m_debugCollisionLines->estimateVertexCount(vertexCount);
			dFloat* vertexArray = new dFloat[3 * vertexCount];
			memset(vertexArray, 0, 3 * vertexCount * sizeof(dFloat));

			NewtonMeshGetVertexChannel(debugCollisionMesh, 3 * sizeof(dFloat), (dFloat*)vertexArray);

			/*dFloat tempMatrix[16];
			Ogre::Quaternion quat = (Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::NEGATIVE_UNIT_Z) * Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::NEGATIVE_UNIT_Y));

			Converters::QuatPosToMatrix(quat, Ogre::Vector3::ZERO, &tempMatrix[0]);

			dMatrix matrix(tempMatrix);
			matrix.TransformVector(dVector(m_node->getScale().x, m_node->getScale().y, m_node->getScale().z));
			matrix.TransformTriplex(vertexArray, 3 * sizeof(dFloat), vertexArray, 3 * sizeof(dFloat), vertexCount);*/

			// matrix = (matrix.Inverse4x4()).Transpose();
			// matrix.TransformTriplex(m_normal, 3 * sizeof(dFloat), m_normal, 3 * sizeof(dFloat), m_vertexCount);*/

			m_debugCollisionLines->clear();
			if (m_debugCollisionLines && m_debugCollisionLines->getNumSections() > 0)
			{
				m_debugCollisionLines->beginUpdate(0);
			}
			else
			{
				m_debugCollisionLines->begin("GreenNoLighting", Ogre::OperationType::OT_LINE_LIST);
				// m_debugCollisionLines->begin("BaseWhiteLine", Ogre::OperationType::OT_LINE_LIST);
			}

			// Note: scale must be derived updated, because it can by especially when using rag doll, that a node has parents, so the scale must be absolute and not relative!
			Ogre::Vector3 scale = this->m_node->_getDerivedScaleUpdated();

			// if (nullptr != this->m_node->getParent())
			// 	scale = this->m_node->getParent()->getScale();

			unsigned int* indexArray = nullptr;

			Ogre::Vector3 aabbMin = Ogre::Vector3::ZERO;
			Ogre::Vector3 aabbMax = Ogre::Vector3::ZERO;

			// extract the materials index array for mesh
			void* const geometryHandle = NewtonMeshBeginHandle(debugCollisionMesh);
			for (int handle = NewtonMeshFirstMaterial(debugCollisionMesh, geometryHandle); handle != -1; handle = NewtonMeshNextMaterial(debugCollisionMesh, geometryHandle, handle))
			{
				int material = NewtonMeshMaterialGetMaterial(debugCollisionMesh, geometryHandle, handle);
				int indexCount = NewtonMeshMaterialGetIndexCount(debugCollisionMesh, geometryHandle, handle);
				m_debugCollisionLines->estimateIndexCount(indexCount);
				indexArray = new unsigned[indexCount];

				NewtonMeshMaterialGetIndexStream(debugCollisionMesh, geometryHandle, handle, (int*)indexArray);


				/*
				for (int vert = 0; vert < vertexCount; vert++)
				{
					Ogre::Real x1 = (vertexArray[(vert * 3) + 0] / scale.x);
					Ogre::Real y1 = (vertexArray[(vert * 3) + 1] / scale.y);
					Ogre::Real z1 = (vertexArray[(vert * 3) + 2] / scale.z);

					if (x1 < aabbMin.x) aabbMin.x = x;
					if (y1 < aabbMin.y) aabbMin.y = y;
					if (z1 < aabbMin.z) aabbMin.z = z;
					if (x1 > aabbMax.x) aabbMax.x = x;
					if (y1 > aabbMax.y) aabbMax.y = y;
					if (z1 > aabbMax.z) aabbMax.z = z;
					
					m_debugCollisionLines->position(x1, y1, z1);
				}
				*/
				
				int a = 0;

				for (int vert = 0; vert < vertexCount; vert++)
				{
					int face = (vert * 3 + 2) % vertexCount;

					Ogre::Vector3 p0(vertexArray[(face * 3) + 0] / scale.x, vertexArray[(face * 3) + 1] / scale.y, vertexArray[(face * 3) + 2] / scale.z);

					for (face = vert; face < (vert + 2) % vertexCount; face++)
					{
						// if (p0 < aabbMin) aabbMin = p0;
						// if (p0 > aabbMax) aabbMax = p0;

						Ogre::Vector3 p1(vertexArray[(face * 3) + 0] / scale.x, vertexArray[(face * 3) + 1] / scale.y, vertexArray[(face * 3) + 2] / scale.z);

						// if (p1 < aabbMin) aabbMin = p1;
						// if (p1 > aabbMax) aabbMax = p1;

						m_debugCollisionLines->position(p0);
						m_debugCollisionLines->index(a++);
						// m_debugCollisionLines->colour(Ogre::ColourValue::Blue);
						m_debugCollisionLines->position(p1);
						m_debugCollisionLines->index(a++);
						// m_debugCollisionLines->colour(Ogre::ColourValue::Blue);

						p0 = p1;
					}
				}

				/*for (int i = 0; i < static_cast<int>(vertexCount); i++)
				{
					int iIndex = indexArray[i];
					int vIndex0 = (iIndex * 3) + 0;
					int vIndex1 = (iIndex * 3) + 1;
					int vIndex2 = (iIndex * 3) + 2;

					Ogre::Real x = vertexArray[vIndex0] / scale.x;
					Ogre::Real y = vertexArray[vIndex1] / scale.y;
					Ogre::Real z = vertexArray[vIndex2] / scale.z;

					if (x < aabbMin.x) aabbMin.x = x;
					if (y < aabbMin.y) aabbMin.y = y;
					if (z < aabbMin.z) aabbMin.z = z;
					if (x > aabbMax.x) aabbMax.x = x;
					if (y > aabbMax.y) aabbMax.y = y;
					if (z > aabbMax.z) aabbMax.z = z;

					m_debugCollisionLines->position(x, y, z);
				}*/

				//for (int index = 0; index < indexCount - 3; /*index++*/)
				//{
				//	//could use object->line here but all that does is call index twice
				//	m_debugCollisionLines->index(indexArray[index++ + 0]);
				//	m_debugCollisionLines->index(indexArray[index++ + 2]);
				//	m_debugCollisionLines->index(indexArray[index++ + 1]);
				//}
				//for (int index = 0; index < indexCount; index++)
				//{
				//	//could use object->line here but all that does is call index twice
				//	m_debugCollisionLines->index(indexArray[index]);
				//}
				// Go only one time in this loop, is that correct?
				break;
			}

			
			m_debugCollisionLines->end();
			// m_debugCollisionLines->setLocalAabb(Ogre::Aabb::newFromExtents(aabbMin, aabbMax));

			NewtonMeshEndHandle(debugCollisionMesh, geometryHandle);
			NewtonMeshDestroy(debugCollisionMesh);

			delete[] indexArray;
			delete[] vertexArray;
		}
		else
		{
			if (nullptr != m_debugCollisionLines)
			{
				Ogre::SceneNode* node = ((Ogre::SceneNode*)m_debugCollisionLines->getParentNode());
				node->detachObject(m_debugCollisionLines);
				// m_sceneManager->destroySceneNode(node);
				
				// delete m_debugCollisionLines;
				m_sceneManager->destroyManualObject(m_debugCollisionLines);
				m_debugCollisionLines = nullptr;
			}
		}
	}

	Body* Body::getNext() const
	{
		NewtonBody* body = NewtonWorldGetNextBody(m_world->getNewtonWorld(), m_body);
		if (body)
		{
			return (Body*)NewtonBodyGetUserData(body);
		}
		return nullptr;
	}

	void Body::setNodeUpdateNotify(NodeUpdateNotifyCallback callback)
	{
		m_nodeupdatenotifycallback = callback;
	}

	void Body::removeTransformCallback(void)
	{
		NewtonBodySetTransformCallback(m_body, nullptr);
	}

	void Body::setContactCallback(ContactCallback callback)
	{
		m_contactCallback = callback;
	}

	void Body::setGravity(const Ogre::Vector3& gravity)
	{
		m_gravity = gravity;
	}

	Ogre::Vector3 Body::getGravity(void) const
	{
		return m_gravity;
	}

	void Body::updateNode(Ogre::Real interpolatParam)
	{
		if (m_node)
		{
			const Ogre::Vector3 velocity = m_curPosit - m_prevPosit;

			m_nodePosit = m_prevPosit + velocity * interpolatParam;
			if (m_updateRotation)
			{
				m_nodeRotation = Ogre::Quaternion::Slerp(interpolatParam, m_prevRotation, m_curRotation);
			}

			m_world->m_ogreMutex.lock();

			// Set position in global space
			Ogre::Node* m_nodeParent = m_node->getParent();
			// If its a dynamic body, the _getDerived function can be used, because the interal Ogre cache is updated
			if (false == m_node->isStatic())
			{
				m_node->setPosition((m_nodeParent->_getDerivedOrientation().Inverse() * (m_nodePosit - m_nodeParent->_getDerivedPosition()) / m_nodeParent->_getDerivedScale()));
				if (m_updateRotation)
				{
					m_node->setOrientation((m_nodeParent->_getDerivedOrientation().Inverse() * m_nodeRotation));
				}
			}
			else
			{
				// Else the cache must be updated manually, but this is expensive if there are a lot of nodes, so only update if position/orientation changed
				// but only if position orientation function has been called, to replace the object manually
				if (true == m_validToUpdateStatic)
				{
					m_node->setPosition((m_nodeParent->_getDerivedOrientationUpdated().Inverse() * (m_nodePosit - m_nodeParent->_getDerivedPositionUpdated())
						/ m_nodeParent->_getDerivedScaleUpdated()));
					// m_node->_getFullTransformUpdated();
					m_node->setOrientation((m_nodeParent->_getDerivedOrientationUpdated().Inverse() * m_nodeRotation));
				}
				m_validToUpdateStatic = false;
			}

			if (m_nodeupdatenotifycallback)
			{
				m_nodeupdatenotifycallback(this);
			}

			if (m_contactCallback)
			{
				int count = 0;

				for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(m_body); joint; joint = NewtonBodyGetNextContactJoint(m_body, joint))
				{
					if (NewtonJointIsActive(joint))
					{
						NewtonBody* const newtonBody0 = NewtonJointGetBody0(joint);
						NewtonBody* const newtonBody1 = NewtonJointGetBody1(joint);

						const NewtonBody* const otherBody = (newtonBody0 == m_body) ? newtonBody1 : newtonBody0;
						OgreNewt::Body* body = (OgreNewt::Body*)NewtonBodyGetUserData(otherBody);

						// Does only work if collisionmode is set on! If required without collision, checkout KinematicBody!
						for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact))
						{
							OgreNewt::ContactJoint contactJoint(joint);
							OgreNewt::Contact ogreNewtContact(contact, &contactJoint);
							count++;

							m_contactCallback(body, &ogreNewtContact);
						}
					}
				}
			}

			this->updateDeformableCollision();

			m_world->m_ogreMutex.unlock();
		}
	}

	Ogre::SceneMemoryMgrTypes Body::getSceneMemoryType(void) const
	{
		return m_sceneMemoryType;
	}

	void Body::clampAngularVelocity(Ogre::Real clampValue)
	{
		dVector omega;
		NewtonBodyGetOmega(m_body, &omega[0]);
		omega.m_w = 0.0f;
		const dFloat mag2 = omega.DotProduct3(omega);
		if (mag2 > (clampValue * clampValue))
		{
			omega = omega.Normalize().Scale(100.0f);
			NewtonBodySetOmega(m_body, &omega[0]);
		}
	}

	void Body::updateDeformableCollision(void)
	{
		const NewtonCollision* deformableCollision = NewtonBodyGetCollision(m_body);
		if (NewtonCollisionGetType(deformableCollision) == SERIALIZE_ID_DEFORMABLE_SOLID)
		{
			const dFloat* const particles = NewtonDeformableMeshGetParticleArray(deformableCollision);
			const int stride = NewtonDeformableMeshGetParticleStrideInBytes(deformableCollision) / sizeof(dFloat);

			/*float mass = 10.0f;
			float lxx = 1.0f;
			float lyy = 1.0f;
			float lzz = 1.0f;
			NewtonBodyGetMass(m_body, &mass, &lxx, &lyy, &lzz);

			NewtonBodySetMassProperties(m_body, mass, deformableCollision);*/

			Ogre::SceneNode* sceneNode = static_cast<Ogre::SceneNode*>(m_node);

			Ogre::v1::MeshPtr mesh = static_cast<Ogre::v1::Entity*>(sceneNode->getAttachedObject(0))->getMesh();

			//find number of submeshes
			const unsigned short sub = mesh->getNumSubMeshes();

			for (unsigned short i = 0; i < sub; i++)
			{
				Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);
				Ogre::v1::VertexData* vertexData;
				if (subMesh->useSharedVertices)
				{
					vertexData = mesh->sharedVertexData[0];
				}
				else
				{
					vertexData = subMesh->vertexData[0];
				}
				Ogre::v1::IndexData* indexData = subMesh->indexData[0];

				const Ogre::v1::VertexElement* posElem = vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(posElem->getSource());
				Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;

				float* vertices = static_cast<float *>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_WRITE_ONLY));
				float* indices = static_cast<float *>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_WRITE_ONLY));

				for (int i = 0; i < vertexData->vertexCount; i++)
				{
					int index = indices[i] * stride;
					vertices[i * 3 + 0] = particles[index + 0];
					vertices[i * 3 + 1] = particles[index + 1];
					vertices[i * 3 + 2] = particles[index + 2];
				}
				vbuf->unlock();
				ibuf->unlock();
			}
		}
	}



	//void Body::updateDeformableCollision(void)
	//{
	//	NewtonCollision* deformableCollision = NewtonBodyGetCollision(m_body);
	//	if (NewtonCollisionGetType(deformableCollision) == SERIALIZE_ID_DEFORMABLE_SOLID)
	//	{
	//		const dFloat* const particles = NewtonDeformableMeshGetParticleArray(deformableCollision);
	//		int stride = NewtonDeformableMeshGetParticleStrideInBytes(deformableCollision) / sizeof(dFloat);

	//		float mass = 10.0f;
	//		float lxx = 1.0f;
	//		float lyy = 1.0f;
	//		float lzz = 1.0f;
	//		NewtonBodyGetMass(m_body, &mass, &lxx, &lyy, &lzz);

	//		NewtonBodySetMassProperties(m_body, mass, deformableCollision);

	//		Ogre::SceneNode* sceneNode = static_cast<Ogre::SceneNode*>(m_node);

	//		Ogre::v1::MeshPtr mesh = static_cast<Ogre::v1::Entity*>(sceneNode->getAttachedObject(0))->getMesh();

	//		//find number of submeshes
	//		unsigned short sub = mesh->getNumSubMeshes();

	//		for (unsigned short i = 0; i < sub; i++)
	//		{
	//			Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);
	//			Ogre::v1::VertexData* vertexData;
	//			if (subMesh->useSharedVertices)
	//			{
	//				vertexData = mesh->sharedVertexData[0];
	//			}
	//			else
	//			{
	//				vertexData = subMesh->vertexData[0];
	//			}

	//			Ogre::v1::IndexData* indexData = subMesh->indexData[0];

	//			const Ogre::v1::VertexElement* posElem = vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
	//			Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(posElem->getSource());
	//			Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
	//			unsigned char* v_ptr = static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_WRITE_ONLY));

	//			bool uses32bit = (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);
	//			unsigned long* i_Longptr;
	//			unsigned short* i_Shortptr;


	//			if (uses32bit)
	//			{
	//				i_Longptr = static_cast<unsigned long*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_WRITE_ONLY));

	//			}
	//			else
	//			{
	//				i_Shortptr = static_cast<unsigned short*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_WRITE_ONLY));
	//			}

	//			// float* vertices = static_cast<float *>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_NORMAL));
	//			// float* indices = static_cast<float *>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_NORMAL));

	//			if (uses32bit)
	//			{
	//				for (int i = 0; i < vertexData->vertexCount; i++)
	//				{
	//					unsigned char* v_offset;
	//					float* v_Posptr;
	//					int index = i_Longptr[i] * stride;
	//					v_offset = v_ptr + (index * vbuf->getVertexSize());
	//					posElem->baseVertexPointerToElement(v_offset, &v_Posptr);
	//					
	//					int index = i_Longptr[i] * stride;
	//					vertices[i * 3 + 0] = particles[index + 0];
	//					vertices[i * 3 + 1] = particles[index + 1];
	//					vertices[i * 3 + 2] = particles[index + 2];
	//				}
	//			}
	//			else
	//			{

	//			}
	//			vbuf->unlock();
	//			ibuf->unlock();
	//		}
	//	}
	//}
}
