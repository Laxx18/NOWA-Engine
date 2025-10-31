#ifndef _INCLUDE_OGRENEWT_RAYCAST
#define _INCLUDE_OGRENEWT_RAYCAST

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_Tools.h"

#include "ndRayCastNotify.h"
#include "ndContact.h"

#include <vector>

namespace OgreNewt
{
    //! general raycast base class
    class _OgreNewtExport Raycast
    {
    public:
        friend class BasicRaycast;

        Raycast();
        virtual ~Raycast();

        void go(const World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, int threadIndex = 0);

        virtual bool userPreFilterCallback(Body* body) { return true; }
        virtual bool userCallback(Body* body, CollisionPtr collision, Ogre::Real distance, const Ogre::Vector3& normal, long collisionID) = 0;

    protected:
        Body* mLastBody;
        bool mBodyAdded;

    private:
        class RayCastCallback : public ndRayCastClosestHitCallback
        {
        public:
            RayCastCallback(Raycast* owner) : mOwner(owner) { m_param = 1.0f; }

            ndFloat32 OnRayCastAction(const ndContactPoint& contact, ndFloat32 intersetParam) override
            {
                if (intersetParam < m_param)
                {
                    m_contact = contact;
                    m_param = intersetParam;

                    const ndBodyKinematic* b0 = contact.m_body0;
                    if (b0)
                    {
                        ndBodyNotify* notify = b0->GetNotifyCallback();
                        if (notify)
                        {
                            BodyNotify* bodyNotify = dynamic_cast<BodyNotify*>(notify);
                            if (bodyNotify)
                                mOwner->mLastBody = bodyNotify->GetOgreNewtBody();
                        }
                    }
                }
                return intersetParam;
            }

            Raycast* mOwner;
        };

        RayCastCallback mCallback;
    };

    //! Basic implementation: stores all hits
    class _OgreNewtExport BasicRaycast : public Raycast
    {
    public:
        class _OgreNewtExport BasicRaycastInfo
        {
        public:
            BasicRaycastInfo()
                : mDistance(1.0f),
                mBody(nullptr),
                mCollisionID(-1)
            {

            }

            ~BasicRaycastInfo()
            {

            }

            Ogre::Real getDistance() { return mDistance; }

            Body* getBody() { return mBody; }

            long getCollisionId() { return mCollisionID; }

            Ogre::Vector3 getNormal() { return mNormal; }

            Ogre::Quaternion getNormalOrientation() { return Converters::grammSchmidt(mNormal); }

            Ogre::Real mDistance;
            Body* mBody;
            long mCollisionID;
            Ogre::Vector3 mNormal;

            bool operator<(const BasicRaycastInfo& rhs) const { return mDistance < rhs.mDistance; }
        };

        BasicRaycast();

        BasicRaycast(const World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted = true);

        void go(const World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted = true);

        ~BasicRaycast();

        bool userCallback(Body* body, CollisionPtr col, Ogre::Real distance, const Ogre::Vector3& normal, long collisionID);

        int getHitCount() const { return (int)mRayList.size(); }

        BasicRaycastInfo getInfoAt(unsigned int hitnum) const;

        BasicRaycastInfo getFirstHit() const;

    private:
        typedef std::vector<BasicRaycastInfo> RaycastInfoList;
        RaycastInfoList mRayList;
    };
}

#endif