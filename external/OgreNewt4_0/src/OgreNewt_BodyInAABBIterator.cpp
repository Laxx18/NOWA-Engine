#include "OgreNewt_Stdafx.h"
#include "OgreNewt_BodyInAABBIterator.h"

namespace OgreNewt
{
	BodyInAABBIterator::BodyInAABBIterator(const OgreNewt::World* world)
		: m_world(world),
		m_callback(nullptr)
	{

	}

    void BodyInAABBIterator::go(const Ogre::AxisAlignedBox& aabb, IteratorCallback callback, void* userdata) const
    {
        m_callback = callback;

        ndVector boxMin(aabb.getMinimum().x, aabb.getMinimum().y, aabb.getMinimum().z, 0.0f);
        ndVector boxMax(aabb.getMaximum().x, aabb.getMaximum().y, aabb.getMaximum().z, 0.0f);

        ndBodiesInAabbNotify notify;
        m_world->getNewtonWorld()->BodiesInAabb(notify, boxMin, boxMax);

        const int count = notify.m_bodyArray.GetCount();
        for (int i = 0; i < count; ++i)
        {
            const ndBody* nbody = notify.m_bodyArray[i];
            OgreNewt::Body* body = static_cast<OgreNewt::BodyNotify*>(nbody->GetNotifyCallback())->GetOgreNewtBody();
            m_callback(body, userdata);
        }
    }
}
