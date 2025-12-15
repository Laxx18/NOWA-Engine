#include "OgreNewt_Stdafx.h"
#include "OgreNewt_KinematicBody.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"

using namespace OgreNewt;

KinematicBody::KinematicBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, Ogre::SceneMemoryMgrTypes memoryType)
    : Body(world, sceneManager, memoryType)
    , m_kinematicContactCallback(nullptr)
{
    if (!m_world)
        return;

    // Create initial matrix from cached transform
    ndMatrix matrix;
    Converters::QuatPosToMatrix(m_curRotation, m_curPosit, matrix);

    // Create body (caller thread is fine for allocation/setup)
    m_body = new ndBodyKinematic();
    m_body->SetMatrix(matrix);

    // Build shape instance (caller thread is fine)
    ndShapeInstance shapeInst =
        col->getShapeInstance()
        ? ndShapeInstance(*col->getShapeInstance())
        : ndShapeInstance(col->getNewtonCollision());

    m_body->SetCollisionShape(shapeInst);

    // Ensure notify exists (matches your other constructors)
    if (!m_bodyNotify)
        m_bodyNotify = new BodyNotify(this);

    // World mutations must be queued
    m_world->enqueuePhysicsAndWait([this](World& w)
        {
            m_body->SetNotifyCallback(m_bodyNotify);
            w.addBody(m_body);

            // Damping: only if this is actually a dynamic body
            // For kinematic, ND may ignore damping; keep it for API consistency but guard it.
            setLinearDamping(w.getDefaultLinearDamping() * (60.0f / w.getUpdateFPS()));
            setAngularDamping(w.getDefaultAngularDamping() * (60.0f / w.getUpdateFPS()));
        });
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
