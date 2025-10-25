#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Joint.h"
#include <float.h>

using namespace OgreNewt;

// ----------------- Joint ctor/dtor -----------------
Joint::Joint()
    : m_joint(nullptr)
{
}

Joint::~Joint()
{
    if (m_joint)
    {
        // if you allocated the ND joint, delete it or hand it back to world as your ownership scheme requires
        // Note: do not attempt to call ND API that isn't present in your build
        m_joint = nullptr;
    }
}

// set the ND joint pointer
void Joint::SetSupportJoint(ndJointBilateralConstraint* supportJoint)
{
    m_joint = supportJoint;
    if (m_joint)
    {
        // if needed, set user data/destructor via ND API; left as TODO if your ND4 exposes it
        // m_joint->SetUserData(this);
        // m_joint->SetUserDestructorCallback(destructorCallback);
    }
}

ndJointBilateralConstraint* Joint::GetSupportJoint() const
{
    return m_joint;
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
            // call the ND4 helper you referenced:
            // void AddLinearRowJacobian(ndConstraintDescritor& desc, const ndVector& pivot0, const ndVector& pivot1, const ndVector& dir)
            // Use m_joint (ndJointBilateralConstraint) to add the row
            m_joint->AddLinearRowJacobian(desc, row.p0, row.p1, row.dir);

            // if the ND API or ndConstraintDescritor allows setting row stiffness/accel after adding the row,
            // do it here. If not, you may need to store parameters elsewhere or pass them via ND helper.
            // Example (pseudocode - replace with your ND4 API):
            // if (row.stiffness >= 0.0f) desc.SetRowStiffness(row.stiffness);
            // if (row.accel != FLT_MAX) desc.SetRowAcceleration(row.accel);
            // if (row.springK != 0.0f || row.springD != 0.0f) desc.SetRowSpringDamper(row.stiffness, row.springK, row.springD);
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

    // done with pending rows for this Jacobian pass
    m_pendingRows.clear();
}

// ----------------- sample destruct callback (no-op placeholder) -----------------
void Joint::destructorCallback(const ndJointBilateralConstraint* /*me*/)
{
    // If ND4 supports user-data -> wrapper mapping, use that to cleanup wrapper pointers.
    // Left intentionally blank for now.
}


// ----------------- CustomJoint: construct ND joint and pass owner pointer -----------------
CustomJoint::CustomJoint(unsigned int maxDOF, const Body* child, const Body* parent)
    : m_maxDOF(maxDOF)
    , m_body0(child)
    , m_body1(parent)
{
    ndBodyKinematic* b0 = nullptr;
    ndBodyKinematic* b1 = nullptr;
    if (child) b0 = const_cast<ndBodyKinematic*>(child->getNewtonBody());
    if (parent) b1 = const_cast<ndBodyKinematic*>(parent->getNewtonBody());

    // build an identity frame
    ndMatrix frame(ndGetIdentityMatrix());

    // create our custom ND joint that calls back into this wrapper
    OgreNewtUserJoint* ujoint = new OgreNewtUserJoint(frame, b0, b1, this);
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