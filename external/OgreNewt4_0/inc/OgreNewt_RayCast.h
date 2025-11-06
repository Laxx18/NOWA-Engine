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
    class _OgreNewtExport Raycast
    {
    public:
        Raycast();
        virtual ~Raycast();

        void go(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, int threadIndex = 0);

        virtual bool userCallback(OgreNewt::Body* body, OgreNewt::CollisionPtr collision, Ogre::Real distance, const Ogre::Vector3& normal, long collisionID) = 0;
        virtual bool userPreFilterCallback(OgreNewt::Body* body);

    protected:
        class RayCastCallback : public ndRayCastClosestHitCallback
        {
        public:
            explicit RayCastCallback(Raycast* owner);
            virtual ~RayCastCallback();

            virtual ndFloat32 OnRayCastAction(const ndContactPoint& contact, ndFloat32 intersetParam) override;

            Raycast* mOwner;
        };

        OgreNewt::Body* mLastBody;
        bool mBodyAdded;

        Ogre::Vector3 mStart;
        Ogre::Vector3 mEnd;

        RayCastCallback mCallback;
    };

    class _OgreNewtExport BasicRaycast : public Raycast
    {
    public:
        class _OgreNewtExport BasicRaycastInfo
        {
        public:
            BasicRaycastInfo();

            // ND3-compatible getters (REQUIRED BY YOUR ENGINE)
            Ogre::Real getDistance();
            Body* getBody();
            long getCollisionId();
            Ogre::Vector3 getNormal();
            Ogre::Quaternion getNormalOrientation();

            // Public data members (unchanged from ND3)
            Ogre::Real mDistance;       // normalized t in [0..1]
            Body* mBody;
            long mCollisionID;
            Ogre::Vector3 mNormal;

            bool operator<(const BasicRaycastInfo& rhs) const;
        };

        BasicRaycast();
        BasicRaycast(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted = true);
        ~BasicRaycast();

        void go(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted = true);

        int getHitCount() const;
        BasicRaycastInfo getFirstHit() const;
        BasicRaycastInfo getInfoAt(unsigned int hitnum) const;

        virtual bool userCallback(Body* body, CollisionPtr col, Ogre::Real distance, const Ogre::Vector3& normal, long collisionID);

    private:
        typedef std::vector<BasicRaycastInfo> RaycastInfoList;
        RaycastInfoList mRayList;
    };
}

#endif