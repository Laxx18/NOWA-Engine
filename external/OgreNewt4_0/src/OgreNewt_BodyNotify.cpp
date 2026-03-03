#include "OgreNewt_Stdafx.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Vehicle.h"

namespace OgreNewt
{
    BodyNotify::BodyNotify(Body* ogreNewtBody) : ndBodyNotify(ndVector(ogreNewtBody->getGravity().x, ogreNewtBody->getGravity().y, ogreNewtBody->getGravity().z, 1.0f)), m_ogreNewtBody(ogreNewtBody)
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
        if (m_ogreNewtBody)
        {
            m_ogreNewtBody->onForceAndTorqueCallback(timestep, threadIndex);
        }
    }

    // Called from World::PostUpdate() — Newton's own thread, all substep workers done.
    // Snapshot live transform fields into snap fields so the main thread reads stable data.
    void BodyNotify::CaptureTransform()
    {
        if (m_ogreNewtBody)
        {
            m_ogreNewtBody->captureTransformSnapshot();
        }
    }
}