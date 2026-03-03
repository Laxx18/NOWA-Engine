#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_CollisionDebugNotify.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_World.h"

#include "OgreNewt_ContactJoint.h"
#include "OgreNewt_ContactNotify.h"

#include "OgreNewt_ComplexVehicle.h"
#include "OgreNewt_Vehicle.h"
#include "OgreNewt_VehicleNotify.h"

namespace OgreNewt
{
    // ─────────────────────────────────────────────────────────────────────────────
    // Constructor 1 — creates its own ndBodyDynamic (the normal path)
    // ─────────────────────────────────────────────────────────────────────────────
    Body::Body(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& collisionPtr, Ogre::SceneMemoryMgrTypes memoryType, NotifyKind notifyKind) :
        m_world(world),
        m_sceneManager(sceneManager),
        m_sceneMemoryType(memoryType),
        m_categoryType(0),
        m_node(nullptr),
        m_matid(nullptr),
        m_bodyPtr(new ndBodyDynamic()),
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
        // Snap fields mirror live fields. captureTransformSnapshot() keeps them in
        // sync once physics starts (called from World::PostUpdate on Newton's thread).
        m_snapCurPosit(Ogre::Vector3::ZERO),
        m_snapPrevPosit(Ogre::Vector3::ZERO),
        m_snapCurRotation(Ogre::Quaternion::IDENTITY),
        m_snapPrevRotation(Ogre::Quaternion::IDENTITY),
        m_snapUpdateRotation(false),
        m_memoryType(memoryType),
        m_debugCollisionLines(nullptr),
        m_debugCollisionItem(nullptr),
        m_gravity(Ogre::Vector3(0.0f, -9.8f, 0.0f)),
        m_isOwner(true),
        m_forcecallback(nullptr),
        m_nodeupdatenotifycallback(nullptr),
        m_contactCallback(nullptr),
        m_renderUpdateCallback(nullptr),
        m_selfCollisionGroup(0)
    {
        switch (notifyKind)
        {
        case NotifyKind::Vehicle:
            m_bodyNotifyPtr = new VehicleNotify(static_cast<Vehicle*>(this));
            break;
        case NotifyKind::ComplexVehicle:
            m_bodyNotifyPtr = new ComplexVehicleNotify(static_cast<ComplexVehicle*>(this));
            break;
        default:
            m_bodyNotifyPtr = new BodyNotify(this);
            break;
        }

        ndMatrix matrix(ndGetIdentityMatrix());
        OgreNewt::Converters::QuatPosToMatrix(m_curRotation, m_curPosit, matrix);
        getNewtonBody()->SetMatrix(matrix);

        ndShapeInstance* srcInst = collisionPtr->getShapeInstance();
        if (srcInst)
        {
            getNewtonBody()->SetCollisionShape(ndShapeInstance(*srcInst));
        }
        else
        {
            getNewtonBody()->SetCollisionShape(ndShapeInstance(collisionPtr->getNewtonCollision()));
        }

        m_world->enqueuePhysicsAndWait(
            [this](World& w)
            {
                getNewtonBody()->SetNotifyCallback(m_bodyNotifyPtr); // pass SharedPtr directly
                w.addBody(m_bodyPtr);                                // pass m_bodyPtr, not getNewtonBody()
                setLinearDamping(w.getDefaultLinearDamping() * (60.0f / w.getUpdateFPS()));
                setAngularDamping(w.getDefaultAngularDamping() * (60.0f / w.getUpdateFPS()));
            });
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Constructor 2 — wraps an externally-created ndBodyKinematic raw pointer
    // ─────────────────────────────────────────────────────────────────────────────
    Body::Body(World* world, Ogre::SceneManager* sceneManager, ndSharedPtr<ndBody> bodyPtr, Ogre::SceneMemoryMgrTypes memoryType, NotifyKind notifyKind) :
        m_world(world),
        m_sceneManager(sceneManager),
        m_bodyPtr(bodyPtr),
        m_sceneMemoryType(memoryType),
        m_categoryType(0),
        m_node(nullptr),
        m_matid(nullptr),
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
        m_snapCurPosit(Ogre::Vector3::ZERO),
        m_snapPrevPosit(Ogre::Vector3::ZERO),
        m_snapCurRotation(Ogre::Quaternion::IDENTITY),
        m_snapPrevRotation(Ogre::Quaternion::IDENTITY),
        m_snapUpdateRotation(false),
        m_memoryType(memoryType),
        m_debugCollisionLines(nullptr),
        m_debugCollisionItem(nullptr),
        m_gravity(Ogre::Vector3(0.0f, -9.8f, 0.0f)),
        m_isOwner(false),
        m_forcecallback(nullptr),
        m_nodeupdatenotifycallback(nullptr),
        m_contactCallback(nullptr),
        m_renderUpdateCallback(nullptr),
        m_selfCollisionGroup(0)
    {

        switch (notifyKind)
        {
        case NotifyKind::Vehicle:
            m_bodyNotifyPtr = new VehicleNotify(static_cast<Vehicle*>(this));
            break;
        case NotifyKind::ComplexVehicle:
            m_bodyNotifyPtr = new ComplexVehicleNotify(static_cast<ComplexVehicle*>(this));
            break;
        default:
            m_bodyNotifyPtr = new BodyNotify(this);
            break;
        }

        m_world->enqueuePhysicsAndWait(
            [this](World& w)
            {
                getNewtonBody()->SetNotifyCallback(m_bodyNotifyPtr); // pass SharedPtr directly
                w.addBody(m_bodyPtr);                                // pass m_bodyPtr, not getNewtonBody()
                setLinearDamping(w.getDefaultLinearDamping() * (60.0f / w.getUpdateFPS()));
                setAngularDamping(w.getDefaultAngularDamping() * (60.0f / w.getUpdateFPS()));
            });
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Constructor 3 — deferred: subclass sets getNewtonBody() / getNewtonBody() later
    // ─────────────────────────────────────────────────────────────────────────────
    Body::Body(World* world, Ogre::SceneManager* sceneManager, Ogre::SceneMemoryMgrTypes memoryType) :
        m_world(world),
        m_sceneManager(sceneManager),
        m_bodyPtr(),
        m_bodyNotifyPtr(),
        m_sceneMemoryType(memoryType),
        m_categoryType(0),
        m_node(nullptr),
        m_matid(nullptr),
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
        m_snapCurPosit(Ogre::Vector3::ZERO),
        m_snapPrevPosit(Ogre::Vector3::ZERO),
        m_snapCurRotation(Ogre::Quaternion::IDENTITY),
        m_snapPrevRotation(Ogre::Quaternion::IDENTITY),
        m_snapUpdateRotation(false),
        m_memoryType(memoryType),
        m_debugCollisionLines(nullptr),
        m_debugCollisionItem(nullptr),
        m_gravity(Ogre::Vector3(0.0f, -9.8f, 0.0f)),
        m_isOwner(true),
        m_forcecallback(nullptr),
        m_nodeupdatenotifycallback(nullptr),
        m_contactCallback(nullptr),
        m_renderUpdateCallback(nullptr),
        m_selfCollisionGroup(0)
    {
        // getNewtonBody() intentionally empty — subclass initialises them.
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Destructor
    // ─────────────────────────────────────────────────────────────────────────────
    Body::~Body()
    {
        if (m_debugCollisionLines)
        {
            auto* node = static_cast<Ogre::SceneNode*>(m_debugCollisionLines->getParentNode());
            node->detachObject(m_debugCollisionLines);
            m_sceneManager->destroyManualObject(m_debugCollisionLines);
            m_debugCollisionLines = nullptr;
        }
        detachNode();
        m_sceneManager = nullptr;

        if (!m_world || !m_bodyPtr)
        {
            return;
        }

        // Sever Newton -> OgreNewt back-pointer
        auto& np = (*m_bodyPtr)->GetNotifyCallback();
        if (np)
        {
            if (auto* bn = dynamic_cast<BodyNotify*>(*np))
            {
                bn->SetOgreNewtBody(nullptr);
            }
        }

        // Capture ONLY m_bodyPtr — that's the one thing PostUpdate needs to RemoveBody.
        // m_bodyNotifyPtr is owned by Newton via SetNotifyCallback; do NOT move it here.
        ndSharedPtr<ndBody> bodyPtr = std::move(m_bodyPtr);
        World* world = m_world;
        m_world = nullptr;
        // m_bodyNotifyPtr left intact — Newton drops it when body is removed

        world->destroyBody(std::move(bodyPtr)); // ONE call, queue owns last ref
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
        {
            m_forcecallback(this, timestep, threadIndex);
        }
    }

    // Called from BodyNotify::CaptureTransform(), which is called from World::PostUpdate().
    // PostUpdate() runs on Newton's own thread after ALL substep worker threads have
    // finished. It is therefore safe to read the live cur/prev fields here and copy them
    // into the snap fields that the main thread will read from Body::updateNode().
    //
    // Threading guarantee:
    //   - Workers write  m_curPosit / m_prevPosit / m_curRotation / m_prevRotation
    //     during the step (OnTransform callbacks).
    //   - PostUpdate()   writes m_snap* fields here (workers already done).
    //   - Main thread    reads  m_snap* fields in updateNode() / interalPostUpdate().
    //     By the time main thread runs interalPostUpdate(), the next frame's Update()
    //     has already Sync()'d the step that wrote these snaps → no race.
    void Body::captureTransformSnapshot()
    {
        m_snapCurPosit = m_curPosit;
        m_snapPrevPosit = m_prevPosit;
        m_snapCurRotation = m_curRotation;
        m_snapPrevRotation = m_prevRotation;
        m_snapUpdateRotation = m_updateRotation;
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
        // Sync snap fields so the first updateNode() call (below) has correct data.
        // Physics hasn't started yet so cur/prev are the spawn position set by the caller.
        captureTransformSnapshot();
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
        {
            return NullPrimitiveType;
        }

        const ndShape* constShape = col->GetShape();
        if (!constShape)
        {
            return NullPrimitiveType;
        }

        ndShape* shape = const_cast<ndShape*>(constShape);

        if (shape->GetAsShapeBox())
        {
            return BoxPrimitiveType;
        }
        if (shape->GetAsShapeCone())
        {
            return ConePrimitiveType;
        }
        if (shape->GetAsShapeSphere())
        {
            return EllipsoidPrimitiveType; // maps ND4 sphere to OgreNewt ellipsoid
        }
        if (shape->GetAsShapeCapsule())
        {
            return CapsulePrimitiveType;
        }
        if (shape->GetAsShapeCylinder())
        {
            return CylinderPrimitiveType;
        }
        if (shape->GetAsShapeCompound())
        {
            return CompoundCollisionPrimitiveType;
        }
        if (shape->GetAsShapeConvexHull())
        {
            return ConvexHullPrimitiveType;
        }
        if (shape->GetAsShapeChamferCylinder())
        {
            return ChamferCylinderPrimitiveType;
        }
        // if (shape->GetAsShapeScene())
        // return ScenePrimitiveType;
        if (shape->GetAsShapeHeightfield())
        {
            return HeighFieldPrimitiveType;
        }
        if (shape->GetAsShapeStaticMesh())
        {
            return TreeCollisionPrimitiveType;
        }
        // if (shape->GetAsShapeDeformableMesh())
        // return DeformableType;
        // if (shape->GetAsShapeConvexHullModifier())
        // 	return ConcaveHullPrimitiveType;

        return NullPrimitiveType;
    }

    void Body::freeze()
    {
        if (!getNewtonBody())
        {
            return;
        }

        getNewtonBody()->SetSleepState(true);
    }

    void Body::unFreeze()
    {
        if (!getNewtonBody())
        {
            return;
        }

        getNewtonBody()->SetSleepState(false);
    }

    bool Body::isFreezed()
    {
        if (!getNewtonBody())
        {
            return false;
        }

        return getNewtonBody()->GetSleepState();
    }

    void Body::setAutoSleep(int flag)
    {
        if (!getNewtonBody())
        {
            return;
        }

        // use newtondynamics4 final api
        getNewtonBody()->SetAutoSleep(flag != 0);
    }

    int Body::getAutoSleep()
    {
        if (!getNewtonBody())
        {
            return 0;
        }

        return getNewtonBody()->GetAutoSleep() ? 1 : 0;
    }

    void Body::scaleCollision(const Ogre::Vector3& scale)
    {
        ndShapeInstance& collision = getNewtonBody()->GetCollisionShape();
        collision.SetScale(ndVector(scale.x, scale.y, scale.z, ndFloat32(0.0f)));
    }

    void Body::setPositionOrientation(const Ogre::Vector3& pos, const Ogre::Quaternion& orient, int threadIndex)
    {
        if (getNewtonBody())
        {
            m_curPosit = pos;
            m_prevPosit = pos;
            m_curRotation = orient;
            m_prevRotation = orient;
            m_validToUpdateStatic = true;

            // Keep snap fields in sync: main-thread code (updateNode) reads snaps.
            // This is a direct main-thread write so no race — physics is either not
            // running yet, or this is a teleport that overwrites both live and snap.
            m_snapCurPosit = pos;
            m_snapPrevPosit = pos;
            m_snapCurRotation = orient;
            m_snapPrevRotation = orient;
            m_snapUpdateRotation = m_updateRotation;

            ndMatrix matrix;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, matrix);
            getNewtonBody()->SetMatrix(matrix);

            updateNode(1.0f);
        }
    }

    void Body::setMassMatrix(Ogre::Real mass, const Ogre::Vector3& inertia)
    {
        if (getNewtonBody())
        {
            ndMatrix inertiaMatrix(ndGetIdentityMatrix());
            inertiaMatrix[0][0] = inertia.x;
            inertiaMatrix[1][1] = inertia.y;
            inertiaMatrix[2][2] = inertia.z;
            getNewtonBody()->SetMassMatrix(ndFloat32(mass), inertiaMatrix);
        }
    }

    void Body::setConvexIntertialMatrix(const Ogre::Vector3& inertia, const Ogre::Vector3& massOrigin)
    {
        if (getNewtonBody())
        {
            ndShapeInstance& collision = getNewtonBody()->GetCollisionShape();
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
        if (!getNewtonBody())
        {
            return;
        }

        // The Collision always has a valid ndShapeInstance now (no old-model fallback).
        ndShapeInstance* srcInst = col->getShapeInstance();
        if (srcInst)
        {
            // Deep-copy the instance so the body owns its own independent copy.
            ndShapeInstance shapeInst(*srcInst);
            getNewtonBody()->SetCollisionShape(shapeInst);
        }
    }

    ndShapeInstance* Body::getNewtonCollision(void) const
    {
        return &getNewtonBody()->GetCollisionShape();
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
        getNewtonBody()->GetAABB(minBox, maxBox);
        Ogre::AxisAlignedBox box;
        box.setMinimum(minBox.m_x, minBox.m_y, minBox.m_z);
        box.setMaximum(maxBox.m_x, maxBox.m_y, maxBox.m_z);
        return box;
    }

    void Body::getMassMatrix(Ogre::Real& mass, Ogre::Vector3& inertia) const
    {
        ndVector inertiaVec = getNewtonBody()->GetMassMatrix();
        mass = inertiaVec.m_w;
        inertia = Ogre::Vector3(inertiaVec.m_x, inertiaVec.m_y, inertiaVec.m_z);
    }

    Ogre::Real Body::getMass() const
    {
        return getNewtonBody()->GetMassMatrix().m_w;
    }

    Ogre::Vector3 Body::getInertia() const
    {
        ndVector inertiaVec = getNewtonBody()->GetMassMatrix();
        return Ogre::Vector3(inertiaVec.m_x, inertiaVec.m_y, inertiaVec.m_z);
    }

    void Body::getInvMass(Ogre::Real& mass, Ogre::Vector3& inertia) const
    {
        ndVector invMassVec = getNewtonBody()->GetInvMass();
        mass = invMassVec.m_w;
        inertia = Ogre::Vector3(invMassVec.m_x, invMassVec.m_y, invMassVec.m_z);
    }

    Ogre::Vector3 Body::getOmega() const
    {
        ndVector omega = getNewtonBody()->GetOmega();
        return Ogre::Vector3(omega.m_x, omega.m_y, omega.m_z);
    }

    Ogre::Vector3 Body::getVelocity() const
    {
        ndVector velocity = getNewtonBody()->GetVelocity();
        return Ogre::Vector3(velocity.m_x, velocity.m_y, velocity.m_z);
    }

    Ogre::Vector3 Body::getVelocityAtPoint(const Ogre::Vector3& point) const
    {
        ndVector pointVec(point.x, point.y, point.z, ndFloat32(1.0f));
        ndVector velocityOut = getNewtonBody()->GetVelocityAtPoint(pointVec);
        return Ogre::Vector3(velocityOut.m_x, velocityOut.m_y, velocityOut.m_z);
    }

    Ogre::Vector3 Body::getForce() const
    {
        ndVector force = getNewtonBody()->GetForce();
        return Ogre::Vector3(force.m_x, force.m_y, force.m_z);
    }

    Ogre::Vector3 Body::getTorque() const
    {
        ndVector torque = getNewtonBody()->GetTorque();
        return Ogre::Vector3(torque.m_x, torque.m_y, torque.m_z);
    }

    Ogre::Real Body::getLinearDamping() const
    {
        return getNewtonBody()->GetLinearDamping();
    }

    Ogre::Vector3 Body::getAngularDamping() const
    {
        ndVector damping = getNewtonBody()->GetAngularDamping();
        return Ogre::Vector3(damping.m_x, damping.m_y, damping.m_z);
    }

    Ogre::Vector3 Body::calculateInverseDynamicsForce(Ogre::Real timestep, Ogre::Vector3 desiredVelocity)
    {
        // TODO: What is the function for that?
        return Ogre::Vector3();
    }

    void Body::setCenterOfMass(const Ogre::Vector3& centerOfMass)
    {
        ndVector com(centerOfMass.x, centerOfMass.y, centerOfMass.z, ndFloat32(1.0f));
        getNewtonBody()->SetCentreOfMass(com);
    }

    Ogre::Vector3 Body::getCenterOfMass() const
    {
        const ndVector com = getNewtonBody()->GetCentreOfMass();
        return Ogre::Vector3(com.m_x, com.m_y, com.m_z);
    }

    void Body::setJointRecursiveCollision(unsigned state)
    {
        if (!m_world)
        {
            return;
        }

        m_world->setJointRecursiveCollision(this, state ? true : false);
    }

    int Body::getJointRecursiveCollision() const
    {
        // Not found in ND4
        return m_selfCollisionGroup > 0;
    }

    void Body::enableGyroscopicTorque(bool enable)
    {
        // TODO: What here?
        // getNewtonBody()->SetGyroMode(enable);
        // ND4 has much more complexity, for now leave it as is.
    }

    void Body::setMaterialGroupID(const MaterialID* materialId)
    {
        m_matid = materialId ? materialId : m_world->getDefaultMaterialID();
        const int id = m_matid ? m_matid->getID() : 0;

        ndSharedPtr<ndBodyNotify>& notifyPtr = getNewtonBody()->GetNotifyCallback();
        if (notifyPtr)
        {
            if (auto* ogreNotify = dynamic_cast<BodyNotify*>(*notifyPtr))
            {
                ogreNotify->SetMaterialId(id);
            }
        }

        m_world->enqueuePhysicsAndWait(
            [this, id](World& w)
            {
                ndShapeInstance& shape = getNewtonBody()->GetCollisionShape();
                shape.m_shapeMaterial.m_userId = static_cast<ndUnsigned32>(id);
            });
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
        ndShapeInstance& collision = getNewtonBody()->GetCollisionShape();
        collision.SetCollisionMode(collidable);
    }

    bool Body::getCollidable(void) const
    {
        const ndShapeInstance& collision = getNewtonBody()->GetCollisionShape();
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
        ndVector bodyOmega = getNewtonBody()->GetOmega();
        ndVector dDesiredOmega(desiredOmega.x, desiredOmega.y, desiredOmega.z, ndFloat32(0.0f));

        ndVector omegaError = dDesiredOmega - bodyOmega;

        ndMatrix bodyInertia = getNewtonBody()->CalculateInertiaMatrix();
        ndVector angularImpulse = bodyInertia.RotateVector(omegaError);

        ndVector linearImpulse(ndFloat32(0.0f));
        getNewtonBody()->ApplyImpulsePair(linearImpulse, angularImpulse, timestep);
    }

    void Body::setForce(const Ogre::Vector3& force)
    {
        if (!getNewtonBody())
        {
            return;
        }

        if (auto* dyn = getNewtonBody()->GetAsBodyDynamic())
        {
            ndVector f((ndFloat32)force.x, (ndFloat32)force.y, (ndFloat32)force.z, 0.0f);
            dyn->SetForce(f);
        }
    }

    void Body::setTorque(const Ogre::Vector3& torque)
    {
        if (!getNewtonBody())
        {
            return;
        }

        if (auto* dyn = getNewtonBody()->GetAsBodyDynamic())
        {
            ndVector t((ndFloat32)torque.x, (ndFloat32)torque.y, (ndFloat32)torque.z, 0.0f);
            dyn->SetTorque(t);
        }
    }

    void Body::addForce(const Ogre::Vector3& force)
    {
        if (!getNewtonBody())
        {
            return;
        }

        if (auto* dyn = getNewtonBody()->GetAsBodyDynamic())
        {
            // accumulate previous + new
            ndVector prev = dyn->GetForce();
            ndVector f((ndFloat32)force.x, (ndFloat32)force.y, (ndFloat32)force.z, 0.0f);
            dyn->SetForce(prev + f);
        }
    }

    void Body::addTorque(const Ogre::Vector3& torque)
    {
        if (!getNewtonBody())
        {
            return;
        }

        if (auto* dyn = getNewtonBody()->GetAsBodyDynamic())
        {
            ndVector prev = dyn->GetTorque();
            ndVector t((ndFloat32)torque.x, (ndFloat32)torque.y, (ndFloat32)torque.z, 0.0f);
            dyn->SetTorque(prev + t);
        }
    }

    void Body::addImpulse(const Ogre::Vector3& deltav, const Ogre::Vector3& posit, Ogre::Real timeStep)
    {
        if (!getNewtonBody())
        {
            return;
        }

        if (auto* dyn = getNewtonBody()->GetAsBodyDynamic())
        {
            ndVector v((ndFloat32)deltav.x, (ndFloat32)deltav.y, (ndFloat32)deltav.z, 0.0f);
            ndVector p((ndFloat32)posit.x, (ndFloat32)posit.y, (ndFloat32)posit.z, 1.0f);
            dyn->AddImpulse(v, p, (ndFloat32)timeStep);
        }
    }

    void Body::setLinearDamping(Ogre::Real damp)
    {
        if (!getNewtonBody())
        {
            return;
        }

        if (auto* dyn = getNewtonBody()->GetAsBodyDynamic())
        {
            dyn->SetLinearDamping(static_cast<ndFloat32>(damp));
        }
    }

    void Body::setAngularDamping(const Ogre::Vector3& damp)
    {
        if (!getNewtonBody())
        {
            return;
        }

        if (auto* dyn = getNewtonBody()->GetAsBodyDynamic())
        {
            ndVector d(static_cast<ndFloat32>(damp.x), static_cast<ndFloat32>(damp.y), static_cast<ndFloat32>(damp.z), 0.0f);
            dyn->SetAngularDamping(d);
        }
    }

    void Body::setOmega(const Ogre::Vector3& omega)
    {
        if (!getNewtonBody())
        {
            return;
        }

        ndVector w(static_cast<ndFloat32>(omega.x), static_cast<ndFloat32>(omega.y), static_cast<ndFloat32>(omega.z), 0.0f);

        getNewtonBody()->SetOmega(w);
    }

    void Body::setVelocity(const Ogre::Vector3& vel)
    {
        if (!getNewtonBody())
        {
            return;
        }

        ndVector v(static_cast<ndFloat32>(vel.x), static_cast<ndFloat32>(vel.y), static_cast<ndFloat32>(vel.z), 0.0f);

        getNewtonBody()->SetVelocity(v);
    }

    void Body::showDebugCollision(bool isStatic, bool show, const Ogre::ColourValue& color)
    {
        if (!m_node)
        {
            return;
        }

        ndBodyKinematic* nbody = getNewtonBody();
        if (!nbody)
        {
            return;
        }

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
        if (!m_world || !getNewtonBody())
        {
            return nullptr;
        }

        // m_world->assertMainThread();

        const auto& bodyList = m_world->getNewtonWorld()->GetBodyList();

        ndBodyList::ndNode* currentNode = nullptr;

        for (ndBodyList::ndNode* node = bodyList.GetFirst(); node; node = node->GetNext())
        {
            // IMPORTANT: reference, not copy (GetInfo() returns T&)
            auto& bodySp = node->GetInfo();                          // ndSharedPtr<ndBody>& (most likely)
            ndBodyKinematic* const b = bodySp->GetAsBodyKinematic(); // or bodySp->GetAsBody()

            if (b == getNewtonBody())
            {
                currentNode = node;
                break;
            }
        }

        if (currentNode)
        {
            ndBodyList::ndNode* nextNode = currentNode->GetNext();
            if (nextNode)
            {
                auto& nextSp = nextNode->GetInfo(); // ref, not copy
                ndBodyKinematic* const nextBody = nextSp->GetAsBodyKinematic();
                if (nextBody)
                {
                    ndSharedPtr<ndBodyNotify>& notifyPtr = nextBody->GetNotifyCallback();
                    if (notifyPtr)
                    {
                        // if you’re 100% sure it's always BodyNotify, keep static_cast.
                        // Safer during debugging:
                        // if (auto* ogreNotify = dynamic_cast<BodyNotify*>(notify)) ...
                        BodyNotify* ogreNotify = dynamic_cast<BodyNotify*>(*notifyPtr);
                        return ogreNotify ? ogreNotify->GetOgreNewtBody() : nullptr;
                    }
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
        {
            return;
        }

        m_lastPosit = m_node->getPosition();
        m_lastOrientation = m_node->getOrientation();

        // Read from snap fields (written by PostUpdate on Newton's thread, NOT from the
        // live m_curPosit / m_prevPosit which worker threads may still be writing).
        const Ogre::Vector3 velocity = m_snapCurPosit - m_snapPrevPosit;
        m_nodePosit = m_snapPrevPosit + velocity * interpolatParam;

        if (m_snapUpdateRotation)
        {
            m_nodeRotation = Ogre::Quaternion::Slerp(interpolatParam, m_snapPrevRotation, m_snapCurRotation);
        }

        Ogre::Vector3 nodePosit = m_nodePosit;
        Ogre::Quaternion nodeRot = m_nodeRotation;
        bool updateRot = m_snapUpdateRotation;
        bool updateStatic = m_validToUpdateStatic;
        Ogre::SceneNode* node = m_node;
        Ogre::Node* parent = node->getParent();

        if (!parent)
        {
            return;
        }

        if (!node->isStatic())
        {
            if (nullptr != m_renderUpdateCallback)
            {
                m_renderUpdateCallback(m_node, m_nodePosit, m_nodeRotation, m_snapUpdateRotation, m_validToUpdateStatic);
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
                m_renderUpdateCallback(m_node, m_nodePosit, m_nodeRotation, m_snapUpdateRotation, m_validToUpdateStatic);
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
    }

    Ogre::SceneMemoryMgrTypes Body::getSceneMemoryType(void) const
    {
        return m_sceneMemoryType;
    }

    void Body::clampAngularVelocity(Ogre::Real clampValue)
    {
        ndVector omega = getNewtonBody()->GetOmega();
        ndFloat32 mag2 = omega.DotProduct(omega).GetScalar();
        if (mag2 > (clampValue * clampValue))
        {
            omega = omega.Normalize().Scale(clampValue);
            getNewtonBody()->SetOmega(omega);
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

    // void Body::updateDeformableCollision(void)
    //{
    //	// This is a placeholder - Newton 4.0 soft body implementation differs
    //	// Would need to use Newton 4.0's soft body/deformable API
    // }

    void Body::setSelfCollisionGroup(unsigned int selfCollisionGroup)
    {
        m_selfCollisionGroup = selfCollisionGroup;
    }

    unsigned int Body::getSelfCollisionGroup() const
    {
        return m_selfCollisionGroup;
    }

    void Body::setBodyNotify(ndSharedPtr<ndBodyNotify> bodyNotifyPtr)
    {
        if (!m_world || !m_bodyPtr)
        {
            return;
        }

        m_bodyNotifyPtr = std::move(bodyNotifyPtr); // move, not copy

        m_world->enqueuePhysicsAndWait(
            [this](World& w)
            {
                getNewtonBody()->SetNotifyCallback(m_bodyNotifyPtr);
            });
    }

    void Body::dispatchContacts()
    {
        if (!m_contactCallback)
        {
            return;
        }

        ndBodyKinematic* const me = getNewtonBody();
        if (!me)
        {
            return;
        }

        ndBodyKinematic::ndContactMap& cmap = me->GetContactMap();
        for (ndBodyKinematic::ndContactMap::Iterator it(cmap); it; it++)
        {
            ndContact* const contact = *it;
            if (!contact || !contact->IsActive())
            {
                continue;
            }

            ndBodyKinematic* const body0 = contact->GetBody0();
            ndBodyKinematic* const body1 = contact->GetBody1();
            ndBodyKinematic* const otherNd = (body0 == me) ? body1 : body0;
            if (!otherNd)
            {
                continue;
            }

            OgreNewt::Body* other = nullptr;
            ndSharedPtr<ndBodyNotify>& notifyPtr = otherNd->GetNotifyCallback();
            if (notifyPtr)
            {
                if (auto* ogreNotify = dynamic_cast<BodyNotify*>(*notifyPtr))
                {
                    other = ogreNotify->GetOgreNewtBody();
                }
            }

            if (!other)
            {
                continue;
            }

            OgreNewt::ContactJoint contactJoint(contact);
            const ndContactPointList& pts = contact->GetContactPoints();
            for (ndContactPointList::ndNode* node = pts.GetFirst(); node; node = node->GetNext())
            {
                OgreNewt::Contact ogreContact(node, &contactJoint);
                m_contactCallback(other, &ogreContact);
            }
        }
    }
}