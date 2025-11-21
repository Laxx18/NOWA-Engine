#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_CollisionDebugNotify.h"

#include "OgreNewt_ContactJoint.h"
#include "OgreNewt_ContactNotify.h"


namespace OgreNewt
{
	Body::Body(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& collisionPtr, Ogre::SceneMemoryMgrTypes memoryType)
		: m_world(world),
		m_sceneManager(sceneManager),
		m_sceneMemoryType(memoryType),
		m_categoryType(0),
		m_node(nullptr),
		m_matid(nullptr),
		m_bodyNotify(nullptr), // Initialize notification object
#ifndef OGRENEWT_NO_OGRE_ANY
		m_userdata(),
		m_userdata2(),
#else
		m_userdata(nullptr),
#endif
		m_nodePosit(Ogre::Vector3::ZERO),
		m_curPosit(Ogre::Vector3::ZERO),
		m_prevPosit(Ogre::Vector3::ZERO),
		m_lastPosit(Ogre::Vector3::ZERO),
		m_curRotation(Ogre::Quaternion::IDENTITY),
		m_prevRotation(Ogre::Quaternion::IDENTITY),
		m_nodeRotation(Ogre::Quaternion::IDENTITY),
		m_lastOrientation(Ogre::Quaternion::IDENTITY),
		m_updateRotation(false),
		m_validToUpdateStatic(false),
		m_memoryType(memoryType),
		m_debugCollisionLines(nullptr),
		m_debugCollisionItem(nullptr),
		m_gravity(Ogre::Vector3(0.0f, -9.8f, 0.0f)),
		m_isOwner(true),
		m_forcecallback(nullptr),
		m_nodeupdatenotifycallback(nullptr),
		m_contactCallback(nullptr),
		m_renderUpdateCallback(nullptr)
	{
		ndMatrix matrix(ndGetIdentityMatrix());
		OgreNewt::Converters::QuatPosToMatrix(m_curRotation, m_curPosit, matrix);

		m_bodyNotify = new BodyNotify(this);
		m_body = new ndBodyDynamic();
		m_body->SetMatrix(matrix);
		m_body->SetCollisionShape(collisionPtr->getNewtonCollision());
		m_body->SetNotifyCallback(m_bodyNotify);

		m_world->getNewtonWorld()->AddBody(m_body);

		setLinearDamping(m_world->getDefaultLinearDamping() * (60.0f / m_world->getUpdateFPS()));
		setAngularDamping(m_world->getDefaultAngularDamping() * (60.0f / m_world->getUpdateFPS()));
	}

	Body::Body(World* world, Ogre::SceneManager* sceneManager, ndBodyKinematic* body, Ogre::SceneMemoryMgrTypes memoryType)
		: m_world(world),
		m_sceneManager(sceneManager),
		m_body(body),
		m_sceneMemoryType(memoryType),
		m_categoryType(0),
		m_node(nullptr),
		m_matid(nullptr),
		m_bodyNotify(nullptr), // Initialize notification object
#ifndef OGRENEWT_NO_OGRE_ANY
		m_userdata(),
		m_userdata2(),
#else
		m_userdata(nullptr),
#endif
		m_nodePosit(Ogre::Vector3::ZERO),
		m_curPosit(Ogre::Vector3::ZERO),
		m_prevPosit(Ogre::Vector3::ZERO),
		m_lastPosit(Ogre::Vector3::ZERO),
		m_curRotation(Ogre::Quaternion::IDENTITY),
		m_prevRotation(Ogre::Quaternion::IDENTITY),
		m_nodeRotation(Ogre::Quaternion::IDENTITY),
		m_lastOrientation(Ogre::Quaternion::IDENTITY),
		m_updateRotation(false),
		m_validToUpdateStatic(false),
		m_memoryType(memoryType),
		m_debugCollisionLines(nullptr),
		m_debugCollisionItem(nullptr),
		m_gravity(Ogre::Vector3(0.0f, -9.8f, 0.0f)),
		m_isOwner(false),
		m_forcecallback(nullptr),
		m_nodeupdatenotifycallback(nullptr),
		m_contactCallback(nullptr),
		m_renderUpdateCallback(nullptr)
	{
		if (nullptr != m_body)
		{
			m_bodyNotify = new BodyNotify(this);
			m_body->SetNotifyCallback(m_bodyNotify);

			m_world->getNewtonWorld()->AddBody(m_body);

			setLinearDamping(m_world->getDefaultLinearDamping() * (60.0f / m_world->getUpdateFPS()));
			setAngularDamping(m_world->getDefaultAngularDamping() * (60.0f / m_world->getUpdateFPS()));
		}
	}

	Body::Body(World* world, Ogre::SceneManager* sceneManager, Ogre::SceneMemoryMgrTypes memoryType)
		: m_world(world),
		m_sceneManager(sceneManager),
		m_body(nullptr),
		m_bodyNotify(nullptr), // Initialize notification object
		m_sceneMemoryType(memoryType),
		m_categoryType(0),
		m_node(nullptr),
		m_matid(nullptr),
#ifndef OGRENEWT_NO_OGRE_ANY
		m_userdata(),
		m_userdata2(),
#else
		m_userdata(nullptr),
#endif
		m_nodePosit(Ogre::Vector3::ZERO),
		m_curPosit(Ogre::Vector3::ZERO),
		m_prevPosit(Ogre::Vector3::ZERO),
		m_lastPosit(Ogre::Vector3::ZERO),
		m_curRotation(Ogre::Quaternion::IDENTITY),
		m_prevRotation(Ogre::Quaternion::IDENTITY),
		m_nodeRotation(Ogre::Quaternion::IDENTITY),
		m_lastOrientation(Ogre::Quaternion::IDENTITY),
		m_updateRotation(false),
		m_validToUpdateStatic(false),
		m_memoryType(memoryType),
		m_debugCollisionLines(nullptr),
		m_debugCollisionItem(nullptr),
		m_gravity(Ogre::Vector3(0.0f, -9.8f, 0.0f)),
		m_isOwner(true),
		m_forcecallback(nullptr),
		m_nodeupdatenotifycallback(nullptr),
		m_contactCallback(nullptr),
		m_renderUpdateCallback(nullptr)
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
				m_sceneManager->destroyManualObject(m_debugCollisionLines);
				m_debugCollisionLines = nullptr;
			}

			detachNode();
			m_sceneManager = nullptr;

			m_world->getNewtonWorld()->Sync();

			if (true == m_isOwner)
			{
				m_world->getNewtonWorld()->RemoveBody(m_body);
				m_body = nullptr;
			}
		}
	}

	void Body::onTransformCallback(const ndMatrix& matrix)
	{
		Ogre::Real dot;

		m_prevPosit = m_curPosit;
		m_prevRotation = m_curRotation;

		const ndVector& pos = matrix.m_posit;
		if (!Ogre::Math::isNaN(pos.m_x))
		{
			m_curPosit = Ogre::Vector3(pos.m_x, pos.m_y, pos.m_z);
		}
		else
		{
			m_curPosit = m_prevPosit;
		}

		if (m_updateRotation)
		{
			ndQuaternion quat(matrix);
			m_curRotation = Ogre::Quaternion(quat.m_w, quat.m_x, quat.m_y, quat.m_z);

			dot = m_prevRotation.Dot(m_curRotation);
			if (dot < 0.0f)
			{
				m_prevRotation = -1.0f * m_prevRotation;
			}
		}
	}

	void Body::onForceAndTorqueCallback(ndFloat32 timestep, int threadIndex)
	{
		if (m_forcecallback)
			m_forcecallback(this, timestep, threadIndex);
	}

	void Body::standardForceCallback(OgreNewt::Body* me, float timestep, int threadIndex)
	{
		Ogre::Real mass;
		Ogre::Vector3 inertia;

		me->getMassMatrix(mass, inertia);
		const Ogre::Vector3 gravityAcceleration(me->getGravity());
		const Ogre::Vector3 gravityForce = gravityAcceleration * mass;

		me->addForce(gravityForce);

		for (auto& it = me->m_accumulatedGlobalForces.cbegin(); it != me->m_accumulatedGlobalForces.cend(); it++)
		{
			me->addGlobalForce(it->first, it->second);
		}

		me->m_accumulatedGlobalForces.clear();
	}

	void Body::setType(unsigned int categoryType)
	{
		m_categoryType = categoryType;
	}

	void Body::attachNode(Ogre::SceneNode* node, bool updateRotation)
	{
		m_node = node;
		m_updateRotation = updateRotation;
		updateNode(1.0f);

		m_lastPosit = m_node->getPosition();
		m_lastOrientation = m_node->getOrientation();
	}

	void Body::detachNode(void)
	{
		m_node = nullptr;
	};

	CollisionPrimitiveType Body::getCollisionPrimitiveType() const
	{
		const ndShapeInstance* col = getNewtonCollision();
		if (!col)
			return NullPrimitiveType;

		const ndShape* constShape = col->GetShape();
		if (!constShape)
			return NullPrimitiveType;

		ndShape* shape = const_cast<ndShape*>(constShape);

		if (shape->GetAsShapeBox())
			return BoxPrimitiveType;
		if (shape->GetAsShapeCone())
			return ConePrimitiveType;
		if (shape->GetAsShapeSphere())
			return EllipsoidPrimitiveType; // maps ND4 sphere to OgreNewt ellipsoid
		if (shape->GetAsShapeCapsule())
			return CapsulePrimitiveType;
		if (shape->GetAsShapeCylinder())
			return CylinderPrimitiveType;
		if (shape->GetAsShapeCompound())
			return CompoundCollisionPrimitiveType;
		if (shape->GetAsShapeConvexHull())
			return ConvexHullPrimitiveType;
		if (shape->GetAsShapeChamferCylinder())
			return ChamferCylinderPrimitiveType;
		// if (shape->GetAsShapeScene())
			// return ScenePrimitiveType;
		if (shape->GetAsShapeHeightfield())
			return HeighFieldPrimitiveType;
		if (shape->GetAsShapeStaticMesh())
			return TreeCollisionPrimitiveType;
		// if (shape->GetAsShapeDeformableMesh())
			// return DeformableType;
		// if (shape->GetAsShapeConvexHullModifier())
		// 	return ConcaveHullPrimitiveType;

		return NullPrimitiveType;
	}

	void Body::freeze()
	{
		if (!m_body)
			return;

		m_body->SetSleepState(true);
	}

	void Body::unFreeze()
	{
		if (!m_body)
			return;

		m_body->SetSleepState(false);
	}

	bool Body::isFreezed()
	{
		if (!m_body)
			return false;

		return m_body->GetSleepState();
	}

	void Body::setAutoSleep(int flag)
	{
		if (!m_body)
			return;

		// use newtondynamics4 final api
		m_body->SetAutoSleep(flag != 0);
	}

	int Body::getAutoSleep()
	{
		if (!m_body)
			return 0;

		return m_body->GetAutoSleep() ? 1 : 0;
	}

	void Body::scaleCollision(const Ogre::Vector3& scale)
	{
		ndShapeInstance& collision = m_body->GetCollisionShape();
		collision.SetScale(ndVector(scale.x, scale.y, scale.z, ndFloat32(0.0f)));
	}

	void Body::setPositionOrientation(const Ogre::Vector3& pos, const Ogre::Quaternion& orient, int threadIndex)
	{
		if (m_body)
		{
			m_curPosit = pos;
			m_prevPosit = pos;
			m_curRotation = orient;
			m_prevRotation = orient;
			m_validToUpdateStatic = true;

			ndMatrix matrix;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, matrix);
			m_body->SetMatrix(matrix);

			updateNode(1.0f);
		}
	}

	void Body::setMassMatrix(Ogre::Real mass, const Ogre::Vector3& inertia)
	{
		if (m_body)
		{
			ndMatrix inertiaMatrix(ndGetIdentityMatrix());
			inertiaMatrix[0][0] = inertia.x;
			inertiaMatrix[1][1] = inertia.y;
			inertiaMatrix[2][2] = inertia.z;
			m_body->SetMassMatrix(ndFloat32(mass), inertiaMatrix);
		}
	}

	void Body::setConvexIntertialMatrix(const Ogre::Vector3& inertia, const Ogre::Vector3& massOrigin)
	{
		if (m_body)
		{
			ndShapeInstance& collision = m_body->GetCollisionShape();
			ndVector inertiaVec(inertia.x, inertia.y, inertia.z, ndFloat32(0.0f));
			ndVector originVec(massOrigin.x, massOrigin.y, massOrigin.z, ndFloat32(0.0f));
			// collision.CalculateInertia(inertiaVec, originVec);
			// TODO: what here?
			collision.CalculateInertia();
		}
	}

	void Body::setStandardForceCallback()
	{
		setCustomForceAndTorqueCallback(standardForceCallback);
	}

	void Body::setCustomForceAndTorqueCallback(ForceCallback callback)
	{
		m_forcecallback = callback;
	}

	void Body::removeForceAndTorqueCallback()
	{ 
		m_forcecallback = nullptr;
	}

	void Body::setCollision(const OgreNewt::CollisionPtr& col)
	{
		m_body->SetCollisionShape(col->getNewtonCollision());
	}

	ndShapeInstance* Body::getNewtonCollision(void) const
	{
		return &m_body->GetCollisionShape();
	}

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
		ndVector minBox, maxBox;
		m_body->GetAABB(minBox, maxBox);
		Ogre::AxisAlignedBox box;
		box.setMinimum(minBox.m_x, minBox.m_y, minBox.m_z);
		box.setMaximum(maxBox.m_x, maxBox.m_y, maxBox.m_z);
		return box;
	}

	void Body::getMassMatrix(Ogre::Real& mass, Ogre::Vector3& inertia) const
	{
		ndVector inertiaVec = m_body->GetMassMatrix();
		mass = inertiaVec.m_w;
		inertia = Ogre::Vector3(inertiaVec.m_x, inertiaVec.m_y, inertiaVec.m_z);
	}

	Ogre::Real Body::getMass() const
	{
		return m_body->GetMassMatrix().m_w;
	}

	Ogre::Vector3 Body::getInertia() const
	{
		ndVector inertiaVec = m_body->GetMassMatrix();
		return Ogre::Vector3(inertiaVec.m_x, inertiaVec.m_y, inertiaVec.m_z);
	}

	void Body::getInvMass(Ogre::Real& mass, Ogre::Vector3& inertia) const
	{
		ndVector invMassVec = m_body->GetInvMass();
		mass = invMassVec.m_w;
		inertia = Ogre::Vector3(invMassVec.m_x, invMassVec.m_y, invMassVec.m_z);
	}

	Ogre::Vector3 Body::getOmega() const
	{
		ndVector omega = m_body->GetOmega();
		return Ogre::Vector3(omega.m_x, omega.m_y, omega.m_z);
	}

	Ogre::Vector3 Body::getVelocity() const
	{
		ndVector velocity = m_body->GetVelocity();
		return Ogre::Vector3(velocity.m_x, velocity.m_y, velocity.m_z);
	}

	Ogre::Vector3 Body::getVelocityAtPoint(const Ogre::Vector3& point) const
	{
		ndVector pointVec(point.x, point.y, point.z, ndFloat32(1.0f));
		ndVector velocityOut = m_body->GetVelocityAtPoint(pointVec);
		return Ogre::Vector3(velocityOut.m_x, velocityOut.m_y, velocityOut.m_z);
	}

	Ogre::Vector3 Body::getForce() const
	{
		ndVector force = m_body->GetForce();
		return Ogre::Vector3(force.m_x, force.m_y, force.m_z);
	}

	Ogre::Vector3 Body::getTorque() const
	{
		ndVector torque = m_body->GetTorque();
		return Ogre::Vector3(torque.m_x, torque.m_y, torque.m_z);
	}

	Ogre::Vector3 Body::getAngularDamping() const
	{
		ndVector damping = m_body->GetAngularDamping();
		return Ogre::Vector3(damping.m_x, damping.m_y, damping.m_z);
	}

	void Body::setCenterOfMass(const Ogre::Vector3& centerOfMass)
	{
		ndVector com(centerOfMass.x, centerOfMass.y, centerOfMass.z, ndFloat32(1.0f));
		m_body->SetCentreOfMass(com);
	}

	Ogre::Vector3 Body::getCenterOfMass() const
	{
		const ndVector com = m_body->GetCentreOfMass();
		return Ogre::Vector3(com.m_x, com.m_y, com.m_z);
	}

	void Body::setContinuousCollisionMode(unsigned state)
	{
		// Not found in ND4
	}

	int Body::getContinuousCollisionMode() const
	{
		// Not found in ND4
		return 0;
	}

	void Body::setJointRecursiveCollision(unsigned state)
	{
		// Not found in ND4
	}

	int Body::getJointRecursiveCollision() const
	{
		// Not found in ND4
		return 0;
	}

	void Body::enableGyroscopicTorque(bool enable)
	{
		// TODO: What here?
		// m_body->SetGyroMode(enable);
		// ND4 has much more complexity, for now leave it as is.
	}

	void Body::setMaterialGroupID(const MaterialID* materialId)
	{
		// store pointer (keep legacy API)
		m_matid = materialId ? materialId : m_world->getDefaultMaterialID();

		// propagate to BodyNotify so contact callbacks can read it
		if (m_body && m_body->GetNotifyCallback())
		{
			if (auto* ogreNotify = dynamic_cast<BodyNotify*>(m_body->GetNotifyCallback()))
			{
				const int id = m_matid ? m_matid->getID() : 0;
				ogreNotify->SetMaterialId(id);
				ndShapeInstance& shape = m_body->GetCollisionShape();
				shape.m_shapeMaterial.m_userId = static_cast<ndUnsigned32>(id);
			}
		}
	}

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


	void Body::setCollidable(bool collidable)
	{
		ndShapeInstance& collision = m_body->GetCollisionShape();
		collision.SetCollisionMode(collidable);
	}

	bool Body::getCollidable(void) const
	{
		const ndShapeInstance& collision = m_body->GetCollisionShape();
		return collision.GetCollisionMode();
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
		ndVector bodyOmega = m_body->GetOmega();
		ndVector dDesiredOmega(desiredOmega.x, desiredOmega.y, desiredOmega.z, ndFloat32(0.0f));

		ndVector omegaError = dDesiredOmega - bodyOmega;

		ndMatrix bodyInertia = m_body->CalculateInertiaMatrix();
		ndVector angularImpulse = bodyInertia.RotateVector(omegaError);

		ndVector linearImpulse(ndFloat32(0.0f));
		m_body->ApplyImpulsePair(linearImpulse, angularImpulse, timestep);
	}

	void Body::setForce(const Ogre::Vector3& force)
	{
		if (!m_body)
			return;

		if (auto* dyn = m_body->GetAsBodyDynamic())
		{
			ndVector f((ndFloat32)force.x, (ndFloat32)force.y, (ndFloat32)force.z, 0.0f);
			dyn->SetForce(f);
		}
	}

	void Body::setTorque(const Ogre::Vector3& torque)
	{
		if (!m_body)
			return;

		if (auto* dyn = m_body->GetAsBodyDynamic())
		{
			ndVector t((ndFloat32)torque.x, (ndFloat32)torque.y, (ndFloat32)torque.z, 0.0f);
			dyn->SetTorque(t);
		}
	}

	void Body::addForce(const Ogre::Vector3& force)
	{
		if (!m_body)
			return;

		if (auto* dyn = m_body->GetAsBodyDynamic())
		{
			// accumulate previous + new
			ndVector prev = dyn->GetForce();
			ndVector f((ndFloat32)force.x, (ndFloat32)force.y, (ndFloat32)force.z, 0.0f);
			dyn->SetForce(prev + f);
		}
	}

	void Body::addTorque(const Ogre::Vector3& torque)
	{
		if (!m_body)
			return;

		if (auto* dyn = m_body->GetAsBodyDynamic())
		{
			ndVector prev = dyn->GetTorque();
			ndVector t((ndFloat32)torque.x, (ndFloat32)torque.y, (ndFloat32)torque.z, 0.0f);
			dyn->SetTorque(prev + t);
		}
	}

	void Body::addImpulse(const Ogre::Vector3& deltav, const Ogre::Vector3& posit, Ogre::Real timeStep)
	{
		if (!m_body)
			return;

		if (auto* dyn = m_body->GetAsBodyDynamic())
		{
			ndVector v((ndFloat32)deltav.x, (ndFloat32)deltav.y, (ndFloat32)deltav.z, 0.0f);
			ndVector p((ndFloat32)posit.x, (ndFloat32)posit.y, (ndFloat32)posit.z, 1.0f);
			dyn->AddImpulse(v, p, (ndFloat32)timeStep);
		}
	}

	void Body::setLinearDamping(Ogre::Real damp)
	{
		if (!m_body)
			return;

		if (auto* dyn = m_body->GetAsBodyDynamic())
		{
			dyn->SetLinearDamping(static_cast<ndFloat32>(damp));
		}
	}

	void Body::setAngularDamping(const Ogre::Vector3& damp)
	{
		if (!m_body)
			return;

		if (auto* dyn = m_body->GetAsBodyDynamic())
		{
			ndVector d(static_cast<ndFloat32>(damp.x), static_cast<ndFloat32>(damp.y), static_cast<ndFloat32>(damp.z), 0.0f);
			dyn->SetAngularDamping(d);
		}
	}

	void Body::setOmega(const Ogre::Vector3& omega)
	{
		if (!m_body)
			return;

		ndVector w(static_cast<ndFloat32>(omega.x), static_cast<ndFloat32>(omega.y), static_cast<ndFloat32>(omega.z), 0.0f);

		m_body->SetOmega(w);
	}

	void Body::setVelocity(const Ogre::Vector3& vel)
	{
		if (!m_body)
			return;

		ndVector v(static_cast<ndFloat32>(vel.x), static_cast<ndFloat32>(vel.y), static_cast<ndFloat32>(vel.z), 0.0f);

		m_body->SetVelocity(v);
	}


	void Body::showDebugCollision(bool isStatic, bool show, const Ogre::ColourValue& color)
	{
		if (!m_node)
			return;

		ndBodyKinematic* nbody = getNewtonBody();
		if (!nbody)
			return;

		// hide
		if (!show)
		{
			if (m_debugCollisionLines)
			{
				Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(m_debugCollisionLines->getParentNode());
				node->detachObject(m_debugCollisionLines);
				m_sceneManager->destroyManualObject(m_debugCollisionLines);
				m_debugCollisionLines = nullptr;
			}
			return;
		}

		// create or clear manual object
		if (!m_debugCollisionLines)
		{
			m_debugCollisionLines = m_sceneManager->createManualObject(isStatic ? Ogre::SCENE_STATIC : Ogre::SCENE_DYNAMIC);
			m_debugCollisionLines->setRenderQueueGroup(200);
			m_debugCollisionLines->setQueryFlags(0);
			m_debugCollisionLines->setCastShadows(false);
			static_cast<Ogre::SceneNode*>(m_node)->attachObject(m_debugCollisionLines);
		}
		else
		{
			m_debugCollisionLines->clear();
		}

		Ogre::Vector3 pos, scale;
		Ogre::Quaternion ori;
		getPositionOrientation(pos, ori);
		scale = m_node->_getDerivedScaleUpdated();

		if (m_debugCollisionLines && m_debugCollisionLines->getNumSections() > 0)
		{
			m_debugCollisionLines->beginUpdate(0);
		}
		else
		{
			m_debugCollisionLines->begin("GreenNoLighting", Ogre::OperationType::OT_LINE_LIST);
			// m_debugCollisionLines->begin("BaseWhiteLine", Ogre::OperationType::OT_LINE_LIST);
		}

		CollisionDebugNotify collisionDebugNotify(m_debugCollisionLines, scale);

		// get collision shape
		ndShapeInstance& shapeInstance = nbody->GetCollisionShape();
#if 1
		shapeInstance.DebugShape(ndGetIdentityMatrix(), collisionDebugNotify);
#else
		ndMatrix bodyMatrix = nbody->GetMatrix();
		shapeInstance.DebugShape(bodyMatrix, dbgCallback);
#endif

		m_debugCollisionLines->end();
	}

	Body* Body::getNext() const
	{
		const auto& bodyList = m_world->getNewtonWorld()->GetBodyList();
		ndBodyList::ndNode* currentNode = nullptr;

		for (ndBodyList::ndNode* node = bodyList.GetFirst(); node; node = node->GetNext())
		{
			// TODO: Debug this!
			if (node->GetInfo()->GetAsBody() == m_body)
			{
				currentNode = node;
				break;
			}
		}

		if (currentNode && currentNode->GetNext())
		{
			// TODO: Debug this!
			auto nextBody = currentNode->GetNext()->GetInfo();
			if (nextBody)
			{
				ndBodyNotify* notify = nextBody->GetNotifyCallback();
				if (notify)
				{
					BodyNotify* ogreNotify = static_cast<BodyNotify*>(notify);
					return ogreNotify->GetOgreNewtBody();
				}
			}
		}
		return nullptr;
	}

	void Body::setNodeUpdateNotify(NodeUpdateNotifyCallback callback)
	{
		m_nodeupdatenotifycallback = callback;
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
		if (!m_node)
			return;

		m_lastPosit = m_node->getPosition();
		m_lastOrientation = m_node->getOrientation();

		const Ogre::Vector3 velocity = m_curPosit - m_prevPosit;
		m_nodePosit = m_prevPosit + velocity * interpolatParam;

		if (m_updateRotation)
		{
			m_nodeRotation = Ogre::Quaternion::Slerp(interpolatParam, m_prevRotation, m_curRotation);
		}

		if (m_nodePosit.positionEquals(m_curPosit) && m_nodeRotation.equals(m_curRotation, Ogre::Radian(0.001f)))
		{
			return;
		}

		// m_world->m_ogreMutex.lock();

		Ogre::Vector3 nodePosit = m_nodePosit;
		Ogre::Quaternion nodeRot = m_nodeRotation;
		bool updateRot = m_updateRotation;
		bool updateStatic = m_validToUpdateStatic;
		Ogre::SceneNode* node = m_node;
		Ogre::Node* parent = node->getParent();

		if (!parent)
			return;

		if (!node->isStatic())
		{
			if (nullptr != m_renderUpdateCallback)
			{
				m_renderUpdateCallback(m_node, m_nodePosit, m_nodeRotation, m_updateRotation, m_validToUpdateStatic);
			}
			else
			{
				node->setPosition((parent->_getDerivedOrientation().Inverse() * (nodePosit - parent->_getDerivedPosition())) / parent->_getDerivedScale());

				if (updateRot)
				{
					node->setOrientation(parent->_getDerivedOrientation().Inverse() * nodeRot);
				}
			}
		}
		else if (updateStatic)
		{
			if (nullptr != m_renderUpdateCallback)
			{
				m_renderUpdateCallback(m_node, m_nodePosit, m_nodeRotation, m_updateRotation, m_validToUpdateStatic);
			}
			else
			{
				node->setPosition((parent->_getDerivedOrientationUpdated().Inverse() * (nodePosit - parent->_getDerivedPositionUpdated())) / parent->_getDerivedScaleUpdated());
				node->setOrientation(parent->_getDerivedOrientationUpdated().Inverse() * nodeRot);
			}
		}

		m_validToUpdateStatic = false;

		if (m_nodeupdatenotifycallback)
		{
			m_nodeupdatenotifycallback(this);
		}

		if (m_contactCallback)
		{
			// Iterate all active contacts for this body
			ndBodyKinematic* const me = m_body;
			if (me)
			{
				ndBodyKinematic::ndContactMap& cmap = me->GetContactMap();
				for (ndBodyKinematic::ndContactMap::Iterator it(cmap); it; it++)
				{
					ndContact* const contact = *it;
					if (!contact || !contact->IsActive())
					{
						continue;
					}

					// Determine the other Newton body
					ndBodyKinematic* const body0 = contact->GetBody0();
					ndBodyKinematic* const body1 = contact->GetBody1();
					ndBodyKinematic* const otherNd = (body0 == me) ? body1 : body0;

					if (!otherNd)
					{
						continue;
					}

					// Map Newton body -> OgreNewt::Body via your ContactNotify bridge
					OgreNewt::Body* other = nullptr;
					if (ndBodyNotify* notify = otherNd->GetNotifyCallback())
					{
						if (auto* bridge = dynamic_cast<OgreNewt::ContactNotify*>(notify))
						{
							other = bridge->getOgreNewtBody();
						}
					}

					if (!other)
					{
						continue;
					}

					// Wrap the joint and iterate contact points
					OgreNewt::ContactJoint contactJoint(contact);

					const ndContactPointList& pts = contact->GetContactPoints();
					for (ndContactPointList::ndNode* node = pts.GetFirst(); node; node = node->GetNext())
					{
						OgreNewt::Contact ogreContact(node, &contactJoint);

						// This mirrors:
						m_contactCallback(other, &ogreContact);
					}
				}
			}
		}

		/*if (m_isSoftBody)
		{
			this->updateDeformableCollision();
		}*/

		// m_world->m_ogreMutex.unlock();
	}

	Ogre::SceneMemoryMgrTypes Body::getSceneMemoryType(void) const
	{
		return m_sceneMemoryType;
	}

	void Body::clampAngularVelocity(Ogre::Real clampValue)
	{
		ndVector omega = m_body->GetOmega();
		ndFloat32 mag2 = omega.DotProduct(omega).GetScalar();
		if (mag2 > (clampValue * clampValue))
		{
			omega = omega.Normalize().Scale(clampValue);
			m_body->SetOmega(omega);
		}
	}

	Ogre::Vector3 Body::getLastPosition(void) const
	{
		return m_lastPosit;
	}

	Ogre::Quaternion Body::getLastOrientation(void) const
	{
		return m_lastOrientation;
	}

	/*void Body::setIsSoftBody(bool isSoftBody)
	{
		m_isSoftBody = isSoftBody;
	}

	bool Body::getIsSoftBody(void) const
	{
		return m_isSoftBody;
	}*/

	void Body::setRenderUpdateCallback(RenderUpdateCallback renderUpdateCallback)
	{
		m_renderUpdateCallback = std::move(renderUpdateCallback);
	}

	bool Body::hasRenderUpdateCallback(void) const
	{
		return m_renderUpdateCallback != nullptr;
	}

	//void Body::updateDeformableCollision(void)
	//{
	//	// This is a placeholder - Newton 4.0 soft body implementation differs
	//	// Would need to use Newton 4.0's soft body/deformable API
	//}
}
