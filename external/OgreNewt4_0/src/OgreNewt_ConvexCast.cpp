#include "OgreNewt_Stdafx.h"
#include "OgreNewt_ConvexCast.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"

namespace OgreNewt
{
    Convexcast::Convexcast()
        : mFirstContactDistance(-1)
    {
    }

    Convexcast::~Convexcast()
    {
    }

    void Convexcast::go(const OgreNewt::World* world, const ndShapeInstance& shape, const Ogre::Vector3& startpt,
        const Ogre::Quaternion& orientation, const Ogre::Vector3& endpt, int /*maxcontactscount*/, int /*threadIndex*/)
    {
        mContacts.clear();
        mFirstContactDistance = -1;

        // Convert start point + orientation to ndMatrix
        ndMatrix matrixStart;
        OgreNewt::Converters::QuatPosToMatrix(orientation, startpt, matrixStart);

        // Convert end point to ndVector (world-space destination of shape origin)
        ndVector vecEnd(endpt.x, endpt.y, endpt.z, 1.0f);

        // Perform convex cast against the world
        ConvexCastNotify notify(this);
        ndWorld* const ndworld = world->getNewtonWorld();
        ndworld->ConvexCast(notify, shape, matrixStart, vecEnd);

        // Copy out contacts from notify's ndConvexCastNotify::m_contacts
        const ndInt32 count = notify.GetContactCount();
        for (ndInt32 i = 0; i < count; ++i)
        {
            const ndContactPoint& c = notify.GetContact(i);

            // body0 may be null for loose-shape casts; world body is usually body1
            const ndBodyKinematic* hitKBody = c.m_body0 ? c.m_body0 : c.m_body1;
            if (!hitKBody)
            {
                continue;
            }

            OgreNewt::Body* ogreBody = nullptr;

            if (ndBodyNotify* const ndNotify = hitKBody->GetNotifyCallback())
            {
                if (BodyNotify* const bodyNotify = dynamic_cast<BodyNotify*>(ndNotify))
                {
                    ogreBody = bodyNotify->GetOgreNewtBody();
                }
            }

            if (!ogreBody)
            {
                continue;
            }

            ContactInfo ci;
            ci.m_body = ogreBody;
            ci.m_point = Ogre::Vector3(c.m_point.m_x, c.m_point.m_y, c.m_point.m_z);
            ci.m_normal = Ogre::Vector3(c.m_normal.m_x, c.m_normal.m_y, c.m_normal.m_z);
            ci.m_penetration = static_cast<Ogre::Real>(c.m_penetration);

            mContacts.push_back(ci);
        }

        if (count > 0 && !mContacts.empty())
        {
            // closest hit param in [0,1]
            mFirstContactDistance = static_cast<Ogre::Real>(notify.GetFirstParam());
        }
        else
        {
            mFirstContactDistance = -1;
        }
    }

    BasicConvexcast::ConvexcastContactInfo BasicConvexcast::getInfoAt(int idx) const
    {
        ConvexcastContactInfo info;
        if (idx < 0 || idx >= static_cast<int>(mContacts.size()))
            return info;

        const ContactInfo& ci = mContacts[idx];

        info.mBody = ci.m_body;
        info.mContactPoint = ci.m_point;
        info.mContactNormal = ci.m_normal;
        info.mContactPenetration = ci.m_penetration;

        return info;
    }
}
