#include "OgreNewt_Stdafx.h"
#include "OgreNewt_RayCast.h"
#include "OgreNewt_World.h"

namespace OgreNewt
{
    //============================= Raycast =============================

    Raycast::Raycast()
        : mLastBody(nullptr)
        , mBodyAdded(false)
        , mStart(Ogre::Vector3::ZERO)
        , mEnd(Ogre::Vector3::ZERO)
        , mCallback(this)
    {
    }

    Raycast::~Raycast()
    {
    }

    bool Raycast::userPreFilterCallback(OgreNewt::Body* body)
    {
        return true;
    }

    Raycast::RayCastCallback::RayCastCallback(Raycast* owner)
        : mOwner(owner)
    {
        m_param = ndFloat32(1.0f);
        // m_contact initialized by base
    }

    Raycast::RayCastCallback::~RayCastCallback()
    {
    }

    ndFloat32 Raycast::RayCastCallback::OnRayCastAction(const ndContactPoint& contact, ndFloat32 intersetParam)
    {
        // keep the closest hit only (closest-hit behavior like ND3's filter returning min t)
        if (intersetParam < m_param)
        {
            m_contact = contact;
            m_param = intersetParam;

            // Map Newton body -> OgreNewt::Body via your BodyNotify bridge
            const ndBodyKinematic* const kBody = contact.m_body0;
            if (kBody != nullptr)
            {
                if (ndBodyNotify* const notify = kBody->GetNotifyCallback())
                {
                    if (BodyNotify* const bodyNotify = dynamic_cast<BodyNotify*>(notify))
                    {
                        mOwner->mLastBody = bodyNotify->GetOgreNewtBody();
                    }
                }
            }
        }
        return intersetParam;
    }

    void Raycast::go(const World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, int /*threadIndex*/)
    {
        mLastBody = nullptr;
        mBodyAdded = false;
        mStart = startpt;
        mEnd = endpt;

        ndWorld* const ndworld = world->getNewtonWorld();

        const ndVector p0(startpt.x, startpt.y, startpt.z, ndFloat32(1.0f));
        const ndVector p1(endpt.x, endpt.y, endpt.z, ndFloat32(1.0f));

        // ND4 ray cast using closest-hit callback
        ndworld->RayCast(mCallback, p0, p1);
    }

    //========================== BasicRaycast ===========================

    BasicRaycast::BasicRaycastInfo::BasicRaycastInfo()
        : mDistance(-1.0f)
        , mBody(nullptr)
        , mCollisionID(-1)
        , mNormal(Ogre::Vector3::ZERO)
    {
    }

    Ogre::Real BasicRaycast::BasicRaycastInfo::getDistance()
    {
        return mDistance;
    }

    Body* BasicRaycast::BasicRaycastInfo::getBody()
    {
        return mBody;
    }

    long BasicRaycast::BasicRaycastInfo::getCollisionId()
    {
        return mCollisionID;
    }

    Ogre::Vector3 BasicRaycast::BasicRaycastInfo::getNormal()
    {
        return mNormal;
    }

    Ogre::Quaternion BasicRaycast::BasicRaycastInfo::getNormalOrientation()
    {
        // same helper used in ND3 (provides a frame aligned to normal)
        return Converters::grammSchmidt(mNormal);
    }

    bool BasicRaycast::BasicRaycastInfo::operator<(const BasicRaycastInfo& rhs) const
    {
        return mDistance < rhs.mDistance;
    }

    BasicRaycast::BasicRaycast()
    {
    }

    BasicRaycast::BasicRaycast(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted)
    {
        go(world, startpt, endpt, sorted);
    }

    BasicRaycast::~BasicRaycast()
    {
    }

    void BasicRaycast::go(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted)
    {
        mRayList.clear();

        // store start/end to preserve ND3 semantics in downstream code (start + (end - start) * t)
        mStart = startpt;
        mEnd = endpt;

        Raycast::go(world, startpt, endpt);

        // Convert closest hit (if any) into ND3-style record
        if (mLastBody != nullptr)
        {
            BasicRaycastInfo info;
            info.mBody = mLastBody;
            info.mDistance = static_cast<Ogre::Real>(mCallback.m_param); // normalized t
            info.mNormal = Ogre::Vector3(mCallback.m_contact.m_normal.m_x,
                mCallback.m_contact.m_normal.m_y,
                mCallback.m_contact.m_normal.m_z);
            info.mCollisionID = static_cast<long>(mCallback.m_contact.m_shapeId0);
            mRayList.push_back(info);
        }

        if (sorted)
        {
            std::sort(mRayList.begin(), mRayList.end());
        }
    }

    int BasicRaycast::getHitCount() const
    {
        return static_cast<int>(mRayList.size());
    }

    BasicRaycast::BasicRaycastInfo BasicRaycast::getFirstHit() const
    {
        BasicRaycastInfo ret;
        Ogre::Real minDist = Ogre::Math::POS_INFINITY;

        for (size_t i = 0; i < mRayList.size(); ++i)
        {
            if (mRayList[i].mDistance < minDist)
            {
                minDist = mRayList[i].mDistance;
                ret = mRayList[i];
            }
        }
        return ret;
    }

    BasicRaycast::BasicRaycastInfo BasicRaycast::getInfoAt(unsigned int hitnum) const
    {
        if (hitnum < mRayList.size())
        {
            return mRayList.at(hitnum);
        }
        else
        {
            BasicRaycastInfo emptyInfo;
            return emptyInfo;
        }
    }

    bool BasicRaycast::userCallback(Body* body, CollisionPtr /*col*/, Ogre::Real distance, const Ogre::Vector3& normal, long collisionID)
    {
        // This is here to keep compatibility if you ever switch to multi-hit accumulation again
        BasicRaycastInfo info;
        info.mBody = body;
        info.mDistance = distance;
        info.mNormal = normal;
        info.mCollisionID = collisionID;
        mRayList.push_back(info);
        return false;
    }

} // namespace OgreNewt