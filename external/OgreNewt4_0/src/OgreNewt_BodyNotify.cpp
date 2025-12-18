#include "OgreNewt_Stdafx.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Vehicle.h"

namespace OgreNewt
{
	BodyNotify::BodyNotify(Body* ogreNewtBody)
		: ndBodyNotify(ndVector(0.0f, -9.8f, 0.0f, 0.0f))
		, m_ogreNewtBody(ogreNewtBody)
	{
	}

	BodyNotify::~BodyNotify()
	{
	}

	void BodyNotify::OnTransform(ndFloat32 timestep, const ndMatrix& matrix)
	{
		if (m_ogreNewtBody)
		{
			m_ogreNewtBody->onTransformCallback(matrix);
		}
	}

	void BodyNotify::OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep)
	{
		/*if (auto* v = dynamic_cast<OgreNewt::Vehicle*>(m_ogreNewtBody))
		{
			v->Update(timestep);
		}*/

		if (m_ogreNewtBody)
		{
			m_ogreNewtBody->onForceAndTorqueCallback(timestep, threadIndex);
		}
	}
}
