#include "OgreNewt_Stdafx.h"
#include "OgreNewt_ConvexCast.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"

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
		const Ogre::Quaternion& orientation, const Ogre::Vector3& endpt, int maxcontactscount, int threadIndex)
	{
		mContacts.clear();
		mFirstContactDistance = -1;

		// Convert start point + orientation to ndMatrix
		ndMatrix matrixStart;
		OgreNewt::Converters::QuatPosToMatrix(orientation, startpt, matrixStart);

		// Convert end point to ndVector
		ndVector vecEnd(endpt.x, endpt.y, endpt.z, 1.0f);

		// Perform convex cast
		ConvexCastNotify notify(this);
		world->getNewtonWorld()->ConvexCast(notify, shape, matrixStart, vecEnd);
	}

	BasicConvexcast::ConvexcastContactInfo BasicConvexcast::getInfoAt(int idx) const
	{
		ConvexcastContactInfo info;
		if (idx < 0 || idx >= (int)mContacts.size())
			return info;

		const ContactInfo& ci = mContacts[idx];
		if (ci.m_body && ci.m_body->GetNotifyCallback())
		{
			info.mBody = static_cast<OgreNewt::BodyNotify*>(ci.m_body->GetNotifyCallback())->GetOgreNewtBody();
		}

		info.mContactPoint.x = ci.m_point.m_x;
		info.mContactPoint.y = ci.m_point.m_y;
		info.mContactPoint.z = ci.m_point.m_z;

		info.mContactNormal.x = ci.m_normal.m_x;
		info.mContactNormal.y = ci.m_normal.m_y;
		info.mContactNormal.z = ci.m_normal.m_z;

		info.mContactPenetration = ci.m_penetration;

		return info;
	}
}
