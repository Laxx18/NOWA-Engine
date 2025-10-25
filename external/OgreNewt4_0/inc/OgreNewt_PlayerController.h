/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

by melven

*/
#ifndef _INCLUDE_OGRENEWT_PLAYERCONTROLLER
#define _INCLUDE_OGRENEWT_PLAYERCONTROLLER

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"

#include <OgreQuaternion.h>
#include <OgreVector3.h>

// ND4
#include "ndBodyPlayerCapsule.h"
#include "ndWorld.h"
#include "ndSharedPtr.h"

namespace OgreNewt
{
    // ----------------------------------------------------------------
    // Same callback interface you already use in your engine
    // ----------------------------------------------------------------
    class _OgreNewtExport PlayerCallback
    {
    public:
        PlayerCallback() {}
        virtual ~PlayerCallback() {}

        // Return friction scale (like before). 2.0f ~ high friction.
        virtual Ogre::Real onContactFriction(const OgreNewt::PlayerControllerBody* visitor,
            const Ogre::Vector3& position,
            const Ogre::Vector3& normal,
            int contactId,
            const OgreNewt::Body* other)
        {
            return 2.0f;
        }

        // Optional contact notification
        virtual void onContact(const OgreNewt::PlayerControllerBody* visitor,
            const Ogre::Vector3& position,
            const Ogre::Vector3& normal,
            Ogre::Real penetration,
            int contactId,
            const OgreNewt::Body* other)
        {
        }
    };

    // ----------------------------------------------------------------
    // Internal ND4 capsule with ApplyInputs + friction callback
    // ----------------------------------------------------------------
    class OgreNewtPlayerCapsule : public ndBodyPlayerCapsule
    {
    public:
        class Owner
        {
        public:
            virtual ~Owner() = default;
            virtual Ogre::Vector3   getGravity() const = 0;
            virtual Ogre::Real      getWalkSpeed() const = 0;
            virtual Ogre::Real      getJumpSpeed() const = 0;
            virtual bool            consumeJumpFlag() = 0; // returns true once when jump requested
            virtual Ogre::Real      forwardCmd() const = 0;
            virtual Ogre::Real      strafeCmd() const = 0;
            virtual Ogre::Radian    headingCmd() const = 0;
            virtual PlayerCallback* getCallback() const = 0;
        };

        OgreNewtPlayerCapsule(Owner* owner,
            const ndMatrix& localAxis,
            ndFloat32 mass,
            ndFloat32 radius,
            ndFloat32 height,
            ndFloat32 stepHeight)
            : ndBodyPlayerCapsule(localAxis, mass, radius, height, stepHeight)
            , m_owner(owner)
        {
        }

        // Called by ND4 each physics step
        void ApplyInputs(ndFloat32 timestep) override
        {
            if (!m_owner) return;

            // gravity as impulse
            const Ogre::Vector3 g = m_owner->getGravity();
            const ndVector gravity(g.x, g.y, g.z, ndFloat32(0.0f));
            m_impulse += gravity.Scale(m_mass * timestep);

            // jump (one-shot)
            if (m_owner->consumeJumpFlag() && IsOnFloor())
            {
                m_impulse += ndVector(0.0f, ndFloat32(m_owner->getJumpSpeed() * m_mass), 0.0f, 0.0f);
            }

            // map desired movement
            ndFloat32 fwd = 0.0f;
            ndFloat32 str = 0.0f;

            if (m_owner->forwardCmd() > 0.0f)      fwd = ndFloat32(m_owner->getWalkSpeed());
            else if (m_owner->forwardCmd() < 0.0f) fwd = -ndFloat32(m_owner->getWalkSpeed());

            if (m_owner->strafeCmd() > 0.0f)       str = ndFloat32(m_owner->getWalkSpeed());
            else if (m_owner->strafeCmd() < 0.0f)  str = -ndFloat32(m_owner->getWalkSpeed());

            // normalize diagonal speed to walkSpeed
            if (fwd && str)
            {
                const ndFloat32 mag = ndSqrt(fwd * fwd + str * str);
                const ndFloat32 invMag = ndFloat32(m_owner->getWalkSpeed()) / mag;
                fwd *= invMag;
                str *= invMag;
            }

            SetForwardSpeed(fwd);
            SetLateralSpeed(str);
            SetHeadingAngle(ndFloat32(m_owner->headingCmd().valueRadians()));
        }

        // Friction hook (like your old onContactFriction). Called per contact.
        ndFloat32 ContactFrictionCallback(const ndVector& position,
            const ndVector& normal,
            ndInt32 contactId,
            const ndBodyKinematic* const otherBody) const override
        {
            PlayerCallback* cb = (m_owner ? m_owner->getCallback() : nullptr);
            if (!cb) return ndFloat32(2.0f);

            const Ogre::Vector3 pos(position.m_x, position.m_y, position.m_z);
            const Ogre::Vector3 nrm(normal.m_x, normal.m_y, normal.m_z);

            // If you want the other OgreNewt::Body here, you’d need a mapping from ndBodyKinematic to OgreNewt::Body.
            // We return nullptr safely for now or wire it via BodyNotify if you keep that on all dynamic bodies.
            const OgreNewt::Body* other = nullptr;
            return ndFloat32(cb->onContactFriction(nullptr, pos, nrm, contactId, other));
        }

    private:
        Owner* m_owner; // not owned
    };

}   // end NAMESPACE OgreNewt

#endif  // _INCLUDE_OGRENEWT_PLAYERCONTROLLER

