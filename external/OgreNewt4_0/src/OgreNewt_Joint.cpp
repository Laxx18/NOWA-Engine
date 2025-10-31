#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Joint.h"
#include <float.h>

using namespace OgreNewt;

namespace
{
    // Find the ndWorld* that owns our joint by walking via BodyNotify → OgreNewt::Body → World
    ndWorld* getNdWorldFromJoint(ndJointBilateralConstraint* jnt)
    {
        if (!jnt) return nullptr;

        auto tryBody = [](ndBodyKinematic* nbody) -> ndWorld*
            {
                if (!nbody) return nullptr;
                if (auto* notify = nbody->GetNotifyCallback())
                {
                    // BodyNotify is your wrapper's notify class
                    if (auto* ogreNotify = dynamic_cast<OgreNewt::BodyNotify*>(notify))
                    {
                        if (auto* ogreBody = ogreNotify->GetOgreNewtBody())
                        {
                            if (auto* ogreWorld = ogreBody->getWorld())
                                return ogreWorld->getNewtonWorld();
                        }
                    }
                }
                return nullptr;
            };

        if (auto* w = tryBody(jnt->GetBody0())) return w;
        if (auto* w = tryBody(jnt->GetBody1())) return w;
        return nullptr;
    }
}

Joint::Joint()
    : m_jointPtr(),
    m_joint(nullptr),
    m_stiffness(1.0f)
{
}

Joint::~Joint()
{
    if (!m_jointPtr)
        return;

    if (ndWorld* ndWorldPtr = getNdWorldFromJoint(m_joint))
    {
        // Use the overload your ND4 build provides; this variant works with raw*
        ndWorldPtr->RemoveJoint(m_joint);
    }

    // Clear our reference (no reset/release API; assign default)
    m_jointPtr = ndSharedPtr<ndJointBilateralConstraint>();
    m_joint = nullptr;
}

void Joint::destroyJoint(OgreNewt::World* /*world*/)
{
    if (!m_jointPtr)
        return;

    if (ndWorld* ndWorldPtr = getNdWorldFromJoint(m_joint))
    {
        // Use the overload your ND4 build provides; this variant works with raw*
        ndWorldPtr->RemoveJoint(m_joint);
    }

    // Clear our reference (no reset/release API; assign default)
    m_jointPtr = ndSharedPtr<ndJointBilateralConstraint>();
    m_joint = nullptr;
}

void Joint::SetSupportJoint(ndJointBilateralConstraint* supportJoint)
{
    // Wrap raw -> shared
    m_jointPtr = ndSharedPtr<ndJointBilateralConstraint>(supportJoint);
    m_joint = m_jointPtr.operator->();
    if (!m_joint)
        return;

    // Resolve ndWorld* via BodyNotify → OgreNewt::Body → World
    if (ndWorld* ndWorldPtr = getNdWorldFromJoint(m_joint))
    {
        // ND4 expects a shared-ptr here
        ndWorldPtr->AddJoint(m_jointPtr);
    }

    // Optional: set user data / destructor callback if you use it
    // m_joint->SetUserData(this);
    // m_joint->SetUserDestructorCallback(&Joint::destructorCallback);
}

// ----------------- Pending row API (public wrapper methods) -----------------

void Joint::addLinearRow(const Ogre::Vector3& pt0, const Ogre::Vector3& pt1, const Ogre::Vector3& dir) const
{
    PendingRow row;
    row.type = PendingRow::LINEAR;
    row.p0 = ndVector(pt0.x, pt0.y, pt0.z, ndFloat32(1.0f));
    row.p1 = ndVector(pt1.x, pt1.y, pt1.z, ndFloat32(1.0f));
    row.dir = ndVector(dir.x, dir.y, dir.z, ndFloat32(0.0f));
    // default params: stiffness==-1 means "not set"
    m_pendingRows.push_back(row);
}

void Joint::addAngularRow(Ogre::Radian relativeAngleError, const Ogre::Vector3& dir) const
{
    PendingRow row;
    row.type = PendingRow::ANGULAR;
    row.dir = ndVector(dir.x, dir.y, dir.z, ndFloat32(0.0f));
    row.relAngle = ndFloat32(relativeAngleError.valueRadians());
    m_pendingRows.push_back(row);
}

void Joint::addGeneralRow(const Ogre::Vector3& linear0, const Ogre::Vector3& angular0, const Ogre::Vector3& linear1, const Ogre::Vector3& angular1) const
{
    PendingRow row;
    row.type = PendingRow::GENERAL;
    row.linear0 = ndVector(linear0.x, linear0.y, linear0.z, ndFloat32(0.0f));
    row.angular0 = ndVector(angular0.x, angular0.y, angular0.z, ndFloat32(0.0f));
    row.linear1 = ndVector(linear1.x, linear1.y, linear1.z, ndFloat32(0.0f));
    row.angular1 = ndVector(angular1.x, angular1.y, angular1.z, ndFloat32(0.0f));
    m_pendingRows.push_back(row);
}

void Joint::setRowStiffness(Ogre::Real stiffness) const
{
    if (!m_pendingRows.empty())
    {
        m_pendingRows.back().stiffness = ndFloat32(stiffness);
    }
}

void Joint::setRowAcceleration(Ogre::Real accel) const
{
    if (!m_pendingRows.empty())
    {
        m_pendingRows.back().accel = ndFloat32(accel);
    }
}

void Joint::setRowSpringDamper(Ogre::Real stiffness, Ogre::Real springK, Ogre::Real springD) const
{
    if (!m_pendingRows.empty())
    {
        m_pendingRows.back().stiffness = ndFloat32(stiffness);
        m_pendingRows.back().springK = ndFloat32(springK);
        m_pendingRows.back().springD = ndFloat32(springD);
    }
}

// ----------------- applyPendingRows: called from ND joint's JacobianDerivative -----------------
void Joint::applyPendingRows(ndConstraintDescritor& desc)
{
    if (!m_joint)
        return;

    // iterate over pending rows and add them into the ND descriptor using ND4 helper methods
    for (const PendingRow& row : m_pendingRows)
    {
        switch (row.type)
        {
        case PendingRow::LINEAR:
        {
            m_joint->AddLinearRowJacobian(desc, row.p0, row.p1, row.dir);
            m_joint->SetLowerFriction(desc, row.springK);
            m_joint->SetHighFriction(desc, row.springD);
            break;
        }
        case PendingRow::ANGULAR:
        {
            // call ND4 helper to add angular row:
            m_joint->AddAngularRowJacobian(desc, row.dir, row.relAngle);

            // set per-row params similarly (see comment above)
            break;
        }

        case PendingRow::GENERAL:
        {
            // ND4 may expose a general jacobian adder; if not, create three linear+three angular rows manually:
            // But we try a direct jacobian adder here if exists:
            // If ND4 does not have an AddGeneralRowJacobian, fall back to adding rows separately:

            // fallback: add linear row with direction = linear0? It's complicated — better to add explicit helper if ND4 provides it.
            // As a safe fallback we add three linear rows using linear0..linear1 vectors as jacobian.
            // NOTE: this fallback is simplistic. Adjust per your needs.
            m_joint->AddLinearRowJacobian(desc, row.linear0, row.linear1, row.linear0); // placeholder usage
            // and angular rows:
            m_joint->AddAngularRowJacobian(desc, row.angular0, ndFloat32(0.0f));
            break;
        }
        }
    }

    m_joint->SetDiagonalRegularizer(desc, static_cast<ndFloat32>(m_stiffness));

    // done with pending rows for this Jacobian pass
    m_pendingRows.clear();
}

// ----------------- sample destruct callback (no-op placeholder) -----------------
void Joint::destructorCallback(const ndJointBilateralConstraint* /*me*/)
{
    // If ND4 supports user-data -> wrapper mapping, use that to cleanup wrapper pointers.
    // Left intentionally blank for now.
}

void Joint::setCollisionState(int state) const
{
	m_joint->SetCollidable(state != 0);
}

int Joint::getCollisionState() const
{
	return m_joint->IsCollidable() ? 1 : 0;
}

Ogre::Real Joint::getStiffness() const
{
    return m_stiffness;
}

void Joint::setStiffness(Ogre::Real stiffness) const
{
    m_stiffness = stiffness;
}

void Joint::setBodyMassScale(Ogre::Real, Ogre::Real)
{
    // no-op in ND4; left for API compatibility
}

void Joint::setJointForceCalculation(bool)
{
    // ND4 always computes forces; keep for backward compatibility
}

OgreNewt::Body* Joint::getBody0() const
{
    if (!m_joint)
        return nullptr;

    if (auto* notify = m_joint->GetBody0()->GetNotifyCallback())
    {
        if (auto* bodyNotify = dynamic_cast<OgreNewt::BodyNotify*>(notify))
        {
            return bodyNotify->GetOgreNewtBody();
        }
    }
    return nullptr;
}

OgreNewt::Body* Joint::getBody1() const
{
    if (!m_joint)
        return nullptr;

    if (auto* notify = m_joint->GetBody1()->GetNotifyCallback())
    {
        if (auto* bodyNotify = dynamic_cast<OgreNewt::BodyNotify*>(notify))
        {
            return bodyNotify->GetOgreNewtBody();
        }
    }
    return nullptr;
}

void Joint::setRowMinimumFriction(Ogre::Real friction) const
{
    if (!m_pendingRows.empty())
        m_pendingRows.back().springK = static_cast<ndFloat32>(friction); // reuse param for lower friction
}

void Joint::setRowMaximumFriction(Ogre::Real friction) const
{
    if (!m_pendingRows.empty())
        m_pendingRows.back().springD = static_cast<ndFloat32>(friction); // reuse param for upper friction
}

Ogre::Vector3 Joint::getForce0() const
{
    if (!m_joint)
        return Ogre::Vector3::ZERO;

    const ndVector f = m_joint->GetForceBody0();
    return Ogre::Vector3(f.m_x, f.m_y, f.m_z);
}

Ogre::Vector3 Joint::getTorque0() const
{
    if (!m_joint)
        return Ogre::Vector3::ZERO;

    const ndVector t = m_joint->GetTorqueBody0();
    return Ogre::Vector3(t.m_x, t.m_y, t.m_z);
}

Ogre::Vector3 Joint::getForce1() const
{
    if (!m_joint)
        return Ogre::Vector3::ZERO;

    const ndVector f = m_joint->GetForceBody1();
    return Ogre::Vector3(f.m_x, f.m_y, f.m_z);
}

Ogre::Vector3 Joint::getTorque1() const
{
    if (!m_joint)
        return Ogre::Vector3::ZERO;

    const ndVector t = m_joint->GetTorqueBody1();
    return Ogre::Vector3(t.m_x, t.m_y, t.m_z);
}

// ----------------- CustomJoint: construct ND joint and pass owner pointer -----------------
CustomJoint::CustomJoint(unsigned int maxDOF, const Body* child, const Body* parent)
    : m_maxDOF(maxDOF)
    , m_body0(child)
    , m_body1(parent)
{
    ndBodyKinematic* b0 = nullptr;
    ndBodyKinematic* b1 = nullptr;

    if (child)
        b0 = const_cast<ndBodyKinematic*>(child->getNewtonBody());
    if (parent)
        b1 = const_cast<ndBodyKinematic*>(parent->getNewtonBody());

    // build an identity frame
    const ndMatrix frame(ndGetIdentityMatrix());

    // create our custom ND joint that calls back into this wrapper
    OgreNewtUserJoint* ujoint = new OgreNewtUserJoint(m_maxDOF, frame, b0, b1, this);
    SetSupportJoint(ujoint);

    // You must add this joint to the ND world if your world expects to own it,
    // e.g. world->AddJoint(ndSharedPtr<ndJointBilateralConstraint>(ujoint));
    // How you handle lifetime depends on your World integration.
}

CustomJoint::~CustomJoint()
{
    // cleanup is handled by world or owner depending on your integration.
}

void CustomJoint::pinAndDirToLocal(const Ogre::Vector3& pinpt, const Ogre::Vector3& pindir,
    Ogre::Quaternion& localOrient0, Ogre::Vector3& localPos0,
    Ogre::Quaternion& localOrient1, Ogre::Vector3& localPos1) const
{
    if (!m_body0 || !m_body1)
        return;

    Ogre::Vector3 pos0, pos1;
    Ogre::Quaternion ori0, ori1;
    m_body0->getPositionOrientation(pos0, ori0);
    m_body1->getPositionOrientation(pos1, ori1);

    // Global pin frame
    Ogre::Quaternion globalOrient = grammSchmidt(pindir);
    Ogre::Vector3 globalPos = pinpt;

    // Convert to local
    localOrient0 = ori0.Inverse() * globalOrient;
    localPos0 = ori0.Inverse() * (globalPos - pos0);

    localOrient1 = ori1.Inverse() * globalOrient;
    localPos1 = ori1.Inverse() * (globalPos - pos1);
}

void CustomJoint::localToGlobal(const Ogre::Quaternion& localOrient, const Ogre::Vector3& localPos,
    Ogre::Quaternion& globalOrient, Ogre::Vector3& globalPos, int bodyIndex) const
{
    const OgreNewt::Body* body = (bodyIndex == 0) ? m_body0 : m_body1;
    if (!body)
        return;

    Ogre::Vector3 pos;
    Ogre::Quaternion ori;
    body->getPositionOrientation(pos, ori);

    globalOrient = ori * localOrient;
    globalPos = pos + ori * localPos;
}

void CustomJoint::localToGlobalVisual(const Ogre::Quaternion& localOrient, const Ogre::Vector3& localPos,
    Ogre::Quaternion& globalOrient, Ogre::Vector3& globalPos, int bodyIndex) const
{
    // Identical to localToGlobal, but could include visual offset or scale correction if needed
    localToGlobal(localOrient, localPos, globalOrient, globalPos, bodyIndex);
}

void CustomJoint::globalToLocal(const Ogre::Quaternion& globalOrient, const Ogre::Vector3& globalPos,
    Ogre::Quaternion& localOrient, Ogre::Vector3& localPos, int bodyIndex) const
{
    const OgreNewt::Body* body = (bodyIndex == 0) ? m_body0 : m_body1;
    if (!body)
        return;

    Ogre::Vector3 pos;
    Ogre::Quaternion ori;
    body->getPositionOrientation(pos, ori);

    localOrient = ori.Inverse() * globalOrient;
    localPos = ori.Inverse() * (globalPos - pos);
}

Ogre::Quaternion CustomJoint::grammSchmidt(const Ogre::Vector3& pin) const
{
    // front is the primary axis
    Ogre::Vector3 front = pin;
    front.normalise();

    // pick a helper vector that is not parallel to front
    Ogre::Vector3 helper = Ogre::Vector3::UNIT_Y;
    if (fabs(front.dotProduct(helper)) > 0.99f) // nearly parallel -> use X instead
    {
        helper = Ogre::Vector3::UNIT_X;
    }

    // compute right and up to form an orthonormal basis
    Ogre::Vector3 right = front.crossProduct(helper);
    right.normalise();

    Ogre::Vector3 up = right.crossProduct(front);
    up.normalise();

    // Build matrix from axes and convert to quaternion
    Ogre::Matrix3 mat;
    mat.FromAxes(front, up, right); // same order used in original code
    Ogre::Quaternion quat;
    quat.FromRotationMatrix(mat);
    return quat;
}