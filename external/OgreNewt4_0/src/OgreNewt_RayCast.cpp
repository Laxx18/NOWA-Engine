#include "OgreNewt_Stdafx.h"
#include "OgreNewt_RayCast.h"
#include "OgreNewt_World.h"

namespace OgreNewt
{
    // ---------------------------
    // Raycast base
    Raycast::Raycast()
        : mLastBody(nullptr),
        mBodyAdded(false),
        mCallback(this)
    {

    }

    Raycast::~Raycast()
    {
    
    }

    void Raycast::go(const World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, int threadIndex)
    {
        mLastBody = nullptr;
        mBodyAdded = false;

        ndVector p0(startpt.x, startpt.y, startpt.z, 1.0f);
        ndVector p1(endpt.x, endpt.y, endpt.z, 1.0f);

        world->getNewtonWorld()->RayCast(mCallback, p0, p1);
    }

    // ---------------------------
    // BasicRaycast
    BasicRaycast::BasicRaycast()
    {

    }

    BasicRaycast::BasicRaycast(const World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted)
    {
        go(world, startpt, endpt, sorted);
    }

    void BasicRaycast::go(const World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted)
    {
        mRayList.clear();

        Raycast::go(world, startpt, endpt);

        if (mLastBody)
        {
            BasicRaycastInfo info;
            info.mBody = mLastBody;
            info.mDistance = mCallback.m_param;
            info.mNormal = Ogre::Vector3(mCallback.m_contact.m_normal.m_x, mCallback.m_contact.m_normal.m_y, mCallback.m_contact.m_normal.m_z);
            info.mCollisionID = mCallback.m_contact.m_shapeId0;

            mRayList.push_back(info);
        }

        if (sorted)
            std::sort(mRayList.begin(), mRayList.end());
    }

    BasicRaycast::~BasicRaycast()
    {

    }

    bool BasicRaycast::userCallback(Body* body, CollisionPtr col, Ogre::Real distance, const Ogre::Vector3& normal, long collisionID)
    {
        // store all hits
        BasicRaycastInfo info;
        info.mBody = body;
        info.mDistance = distance;
        info.mNormal = normal;
        info.mCollisionID = collisionID;

        mRayList.push_back(info);
        return true;
    }

    BasicRaycast::BasicRaycastInfo BasicRaycast::getFirstHit() const
    {
        if (!mRayList.empty())
            return mRayList[0];

        return BasicRaycastInfo();
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

}
