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

		void SetMaterialId(int id) { m_materialId = id; }
		int  GetMaterialId() const { return m_materialId; }

		// Get the OgreNewt::Body pointer
		Body* GetOgreNewtBody() const { return m_ogreNewtBody; }

		// Newton 4.0 notification callbacks
		virtual void OnTransform(ndFloat32 timestep, const ndMatrix& matrix) override;
		virtual void OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep) override;

	private:
		Body* m_ogreNewtBody;
		int m_materialId { 0 };
	};
}

#endif
