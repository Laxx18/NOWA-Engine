#include "OgreNewt_Stdafx.h"
#include "OgreNewt_KinematicBody.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_BodyNotify.h"

using namespace OgreNewt;

KinematicBody::KinematicBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, Ogre::SceneMemoryMgrTypes memoryType)
    : Body(world, sceneManager, memoryType),
    m_kinematicContactCallback(nullptr)
{
    // Create an ndMatrix from Ogre Quaternion + Position
    ndMatrix matrix;
    Converters::QuatPosToMatrix(m_curRotation, m_curPosit, matrix);

    // Create a kinematic body
    m_body = new ndBodyKinematic();
    m_body->SetMatrix(matrix);
    m_body->SetCollisionShape(col->getNewtonCollision());

    // Attach to Newton world
    m_world->getNewtonWorld()->AddBody(m_body);

    // Attach the Body (this) as notify callback (Body inherits ndBodyNotify)
    m_body->SetNotifyCallback(m_bodyNotify);

    // Apply default damping settings
    setLinearDamping(m_world->getDefaultLinearDamping() * (60.0f / m_world->getUpdateFPS()));
    setAngularDamping(m_world->getDefaultAngularDamping() * (60.0f / m_world->getUpdateFPS()));
}

KinematicBody::~KinematicBody()
{
}

void KinematicBody::integrateVelocity(Ogre::Real dt)
{
    if (!m_body)
        return;

    // Integrate kinematic body velocity
    m_body->IntegrateVelocity(dt);

    // Handle contact callbacks if defined
    if (m_kinematicContactCallback)
    {
        ndBodyKinematic::ndContactMap& contacts = m_body->GetContactMap();
        ndBodyKinematic::ndContactMap::Iterator it(contacts);

        for (it.Begin(); it; it++)
        {
            ndContact* const contact = *it;
            if (contact->IsActive())
            {
                ndBodyKinematic* const body0 = contact->GetBody0();
                ndBodyKinematic* const body1 = contact->GetBody1();
                ndBodyKinematic* const other = (body0 == m_body) ? body1 : body0;

                if (other)
                {
                    // Retrieve wrapper class
                    Body* const otherWrapper = dynamic_cast<Body*>(other->GetNotifyCallback());

                    if (otherWrapper)
                        m_kinematicContactCallback(otherWrapper);
                }
            }
        }
    }
}
