#ifndef _INCLUDE_OGRENEWT_JOINT
#define _INCLUDE_OGRENEWT_JOINT

#include "OgreNewt_Prerequisites.h"

#include "ndJointBilateralConstraint.h"
#include "ndBodyKinematic.h"
#include <vector>

namespace OgreNewt
{
    class Body;

    class _OgreNewtExport Joint
    {
    public:
        Joint();
        virtual ~Joint();

        Body* getBody0() const;
        Body* getBody1() const;

        // Explicit destroy (optional for callers); safe with ND4 shared ownership.
        void destroyJoint(OgreNewt::World* world);

        virtual void submitConstraint(Ogre::Real timeStep, int threadIndex)
        {

        }

        int getCollisionState() const;
        void setCollisionState(int state) const;
        Ogre::Real getStiffness() const;
        void setStiffness(Ogre::Real stiffness) const;
        void setJointForceCalculation(bool calculate);
        void setRowMinimumFriction(Ogre::Real friction) const;
        void setRowMaximumFriction(Ogre::Real friction) const;
        void setBodyMassScale(Ogre::Real scaleBody0, Ogre::Real scaleBody1);
        Ogre::Vector3 getForce0(void) const;
        Ogre::Vector3 getForce1(void) const;
        Ogre::Vector3 getTorque0(void) const;
        Ogre::Vector3 getTorque1(void) const;

    protected:
        // Back-compat helpers your old custom joints call during submitConstraint()
        void addLinearRow(const Ogre::Vector3& pt0, const Ogre::Vector3& pt1, const Ogre::Vector3& dir) const;
        void addAngularRow(Ogre::Radian relativeAngleError, const Ogre::Vector3& dir) const;
        void addGeneralRow(const Ogre::Vector3& linear0, const Ogre::Vector3& angular0,
            const Ogre::Vector3& linear1, const Ogre::Vector3& angular1) const;

        void setRowStiffness(Ogre::Real stiffness) const;
        void setRowAcceleration(Ogre::Real accel) const;
        void setRowSpringDamper(Ogre::Real stiffness, Ogre::Real springK, Ogre::Real springD) const;

        void setJointSolverModel(int model);

        // NEW: take ownership of the native joint and auto-register in ND world
        void SetSupportJoint(ndJointBilateralConstraint* supportJoint);
        ndJointBilateralConstraint* GetSupportJoint() const { return m_joint; }

        // Called from native joint’s JacobianDerivative if you use the pending-row bridge
        void applyPendingRows(ndConstraintDescritor& desc);

        static void _CDECL destructorCallback(const ndJointBilateralConstraint* me);

    protected:
        mutable std::vector<uint8_t> m_placeholder;

        struct PendingRow
        {
            enum Type { LINEAR, ANGULAR, GENERAL } type;
            ndVector p0, p1, dir, linear0, angular0, linear1, angular1;
            ndFloat32 relAngle;
            ndFloat32 stiffness, accel, springK, springD;
            PendingRow() : relAngle(0.0f), stiffness(-1.0f), accel(FLT_MAX), springK(0.0f), springD(0.0f) {}
        };
        mutable std::vector<PendingRow> m_pendingRows;

        // NEW: shared ownership + cached raw
        ndSharedPtr<ndJointBilateralConstraint> m_jointPtr{};
        ndJointBilateralConstraint* m_joint{ nullptr };
        mutable Ogre::Real m_stiffness;
    };

    // small helper ND4 joint subclass that calls back to the wrapper
    // OgreNewtUserJoint.h  (or inside the same compilation unit)
    class OgreNewtUserJoint : public ndJointBilateralConstraint
    {
    public:
        // ctor that uses single global matrix
        OgreNewtUserJoint(ndInt32 maxDof,
            const ndMatrix& globalMatrix,
            ndBodyKinematic* body0,
            ndBodyKinematic* body1,
            Joint* owner)
            : ndJointBilateralConstraint(maxDof, body0, body1, globalMatrix)
            , m_owner(owner)
        {

        }

        // ctor that uses separate body matrices
        OgreNewtUserJoint(ndInt32 maxDof,
            const ndMatrix& globalMatrixBody0,
            const ndMatrix& globalMatrixBody1,
            ndBodyKinematic* body0,
            ndBodyKinematic* body1,
            Joint* owner)
            : ndJointBilateralConstraint(maxDof, body0, body1, globalMatrixBody0, globalMatrixBody1)
            , m_owner(owner)
        {

        }

        // override to allow wrapper to emit rows
        virtual void JacobianDerivative(ndConstraintDescritor& desc) override
        {
            if (!m_owner)
                return;

            // If your wrapper stores "pending rows" that must be applied to the native desc,
            // call that first (you mentioned applyPendingRows in earlier messages). If you
            // don't have such a function, omit it.
            //
            // Example (uncomment if you implement applyPendingRows in your Joint wrapper):
            // m_owner->applyPendingRows(desc);

            // Call the wrapper's submitConstraint as the old API expected. The old code passed
            // dummy values for timestep/threadIndex; keep that for backward compatibility.
            // submitConstraint should use addLinearRow/addAngularRow helpers on the wrapper,
            // which must be implemented to translate into native Add*Jacobian calls (see note).
            m_owner->submitConstraint(0.0f, 0);
        }

    private:
        Joint* m_owner;
    };

    // CustomJoint remains the same API; implementation will create OgreNewtUserJoint
    class _OgreNewtExport CustomJoint : public Joint
    {
    public:
        CustomJoint(unsigned int maxDOF, const OgreNewt::Body* child, const OgreNewt::Body* parent);
        virtual ~CustomJoint();

        void pinAndDirToLocal(const Ogre::Vector3& pinpt, const Ogre::Vector3& pindir,
            Ogre::Quaternion& localOrient0, Ogre::Vector3& localPos0,
            Ogre::Quaternion& localOrient1, Ogre::Vector3& localPos1) const;

        void localToGlobal(const Ogre::Quaternion& localOrient, const Ogre::Vector3& localPos,
            Ogre::Quaternion& globalOrient, Ogre::Vector3& globalPos, int bodyIndex) const;

        void localToGlobalVisual(const Ogre::Quaternion& localOrient, const Ogre::Vector3& localPos,
            Ogre::Quaternion& globalOrient, Ogre::Vector3& globalPos, int bodyIndex) const;

        void globalToLocal(const Ogre::Quaternion& globalOrient, const Ogre::Vector3& globalPos,
            Ogre::Quaternion& localOrient, Ogre::Vector3& localPos, int bodyIndex) const;

        Ogre::Quaternion grammSchmidt(const Ogre::Vector3& pin) const;

    protected:
        unsigned int m_maxDOF;
        const OgreNewt::Body* m_body0;
        const OgreNewt::Body* m_body1;
    };

} // end namespace OgreNewt

#endif // _INCLUDE_OGRENEWT_JOINT
