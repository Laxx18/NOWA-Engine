#ifndef _INCLUDE_OGRENEWT_CONVEXCAST
#define _INCLUDE_OGRENEWT_CONVEXCAST

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_BodyNotify.h"

namespace OgreNewt
{
    class _OgreNewtExport Convexcast
    {
    public:
        Convexcast();

        virtual ~Convexcast();

        void go(const OgreNewt::World* world, const ndShapeInstance& shape, const Ogre::Vector3& startpt,
            const Ogre::Quaternion& orientation, const Ogre::Vector3& endpt, int maxcontactscount, int threadIndex);

        virtual bool userPreFilterCallback(OgreNewt::Body* body) { return true; }

    protected:
        struct ContactInfo
        {
            Ogre::Vector3    m_point;
            Ogre::Vector3    m_normal;
            OgreNewt::Body* m_body;
            Ogre::Real       m_penetration;

            ContactInfo()
                : m_point(Ogre::Vector3::ZERO)
                , m_normal(Ogre::Vector3::ZERO)
                , m_body(nullptr)
                , m_penetration(0.0f)
            {
            }
        };

        std::vector<ContactInfo> mContacts;
        Ogre::Real mFirstContactDistance;

    private:
        class ConvexCastNotify : public ndConvexCastNotify
        {
        public:
            ConvexCastNotify(Convexcast* owner)
                : m_owner(owner)
            {
            }

            virtual ndUnsigned32 OnRayPrecastAction(const ndBody* const body, const ndShapeInstance* const) override
            {
                OgreNewt::Body* b = nullptr;

                if (body->GetNotifyCallback())
                {
                    b = static_cast<OgreNewt::BodyNotify*>(body->GetNotifyCallback())->GetOgreNewtBody();
                }

                if (!b || m_owner->userPreFilterCallback(b))
                    return 1;
                return 0;
            }

            ndInt32 GetContactCount() const
            {
                return ndInt32(m_contacts.GetCount());
            }

            const ndContactPoint& GetContact(ndInt32 index) const
            {
                return m_contacts[index];
            }

            ndFloat32 GetFirstParam() const
            {
                return m_param;
            }

        private:
            Convexcast* m_owner;
        };
    };

    class _OgreNewtExport BasicConvexcast : public Convexcast
    {
    public:
        class ConvexcastContactInfo
        {
        public:
            OgreNewt::Body* mBody;
            Ogre::Vector3   mContactPoint;
            Ogre::Vector3   mContactNormal;
            Ogre::Real      mContactPenetration;

            ConvexcastContactInfo()
                : mBody(nullptr)
                , mContactPoint(Ogre::Vector3::ZERO)
                , mContactNormal(Ogre::Vector3::ZERO)
                , mContactPenetration(0)
            {
            }
        };

        BasicConvexcast()
            : mIgnoreBody(nullptr)
        {
        }

        BasicConvexcast(const OgreNewt::World* world, const ndShapeInstance& shape, const Ogre::Vector3& startpt,
            const Ogre::Quaternion& ori, const Ogre::Vector3& endpt, int maxcontactscount, int threadIndex, OgreNewt::Body* ignoreBody = nullptr)
            : mIgnoreBody(ignoreBody)
        {
            go(world, shape, startpt, ori, endpt, maxcontactscount, threadIndex);
        }

        bool userPreFilterCallback(OgreNewt::Body* body) override
        {
            return (!mIgnoreBody || body != mIgnoreBody);
        }

        int getContactsCount() const { return static_cast<int>(mContacts.size()); }

        ConvexcastContactInfo getInfoAt(int idx) const;

        Ogre::Real getDistanceToFirstHit() const { return mFirstContactDistance; }

        void setIgnoreBody(OgreNewt::Body* body) { mIgnoreBody = body; }

        OgreNewt::Body* getIgnoreBody() { return mIgnoreBody; }

    protected:
        OgreNewt::Body* mIgnoreBody;
    };
}

#endif
