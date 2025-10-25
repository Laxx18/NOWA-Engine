/*
OgreNewt Library 4.0

Ogre implementation of Newton Game Dynamics SDK 4.0
*/

#ifndef _INCLUDE_OGRENEWT_BODYNOTIFY
#define _INCLUDE_OGRENEWT_BODYNOTIFY

#include "OgreNewt_Prerequisites.h"

namespace OgreNewt
{
	class Body;

	class _OgreNewtExport BodyNotify : public ndBodyNotify
	{
	public:
		BodyNotify(Body* ogreNewtBody);
		virtual ~BodyNotify();

		// Get the OgreNewt::Body pointer
		Body* GetOgreNewtBody() const { return m_ogreNewtBody; }

		// Newton 4.0 notification callbacks
		virtual void OnTransform(ndInt32 threadIndex, const ndMatrix& matrix) override;
		virtual void OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep) override;

	private:
		Body* m_ogreNewtBody;
	};
}

#endif
