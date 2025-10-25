#include "OgreNewt_Stdafx.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_Body.h"

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

	void BodyNotify::OnTransform(ndInt32 threadIndex, const ndMatrix& matrix)
	{
		if (m_ogreNewtBody)
		{
			m_ogreNewtBody->onTransformCallback(matrix);
		}
	}

	void BodyNotify::OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep)
	{
		if (m_ogreNewtBody)
		{
			m_ogreNewtBody->onForceAndTorqueCallback(timestep, threadIndex);
		}
	}
}
