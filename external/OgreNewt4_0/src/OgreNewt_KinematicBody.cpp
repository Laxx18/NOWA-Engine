#include "OgreNewt_Stdafx.h"
#include "OgreNewt_KinematicBody.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_World.h"

using namespace OgreNewt;

KinematicBody::KinematicBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, Ogre::SceneMemoryMgrTypes memoryType) :
    Body(world, sceneManager, memoryType) // Constructor 3 — deferred
    ,
    m_kinematicContactCallback(nullptr)
{
    if (!m_world)
    {
        return;
    }

    // This IS the first (and only) owner
    m_bodyPtr = ndSharedPtr<ndBody>(new ndBodyKinematic());
    m_bodyNotifyPtr = ndSharedPtr<ndBodyNotify>(new BodyNotify(this));

    ndBodyKinematic* kb = getNewtonBody();

    ndMatrix matrix;
    Converters::QuatPosToMatrix(m_curRotation, m_curPosit, matrix);
    kb->SetMatrix(matrix);

    // Set collision shape ONCE
    ndShapeInstance* src = col->getShapeInstance();
    kb->SetCollisionShape(src ? ndShapeInstance(*src) : ndShapeInstance(col->getNewtonCollision()));

    m_world->enqueuePhysicsAndWait(
        [this](World& w)
        {
            getNewtonBody()->SetNotifyCallback(m_bodyNotifyPtr);
            w.addBody(m_bodyPtr);
            setLinearDamping(w.getDefaultLinearDamping() * (60.0f / w.getUpdateFPS()));
            setAngularDamping(w.getDefaultAngularDamping() * (60.0f / w.getUpdateFPS()));
        });
}

KinematicBody::~KinematicBody()
{
}

void KinematicBody::integrateVelocity(Ogre::Real dt)
{
    if (!getNewtonBody())
    {
        return;
    }

    getNewtonBody()->IntegrateVelocity(dt);

    if (m_kinematicContactCallback)
    {
        ndBodyKinematic::ndContactMap& contacts = getNewtonBody()->GetContactMap();
        ndBodyKinematic::ndContactMap::Iterator it(contacts);

        for (it.Begin(); it; it++)
        {
            ndContact* const contact = *it;
            if (contact->IsActive())
            {
                ndBodyKinematic* const body0 = contact->GetBody0();
                ndBodyKinematic* const body1 = contact->GetBody1();
                ndBodyKinematic* const other = (body0 == getNewtonBody()) ? body1 : body0;

                if (other)
                {
                    auto& notifyPtr = other->GetNotifyCallback();
                    if (!notifyPtr)
                    {
                        continue;
                    }

                    if (auto* ogreNotify = dynamic_cast<OgreNewt::BodyNotify*>(*notifyPtr))
                    {
                        if (Body* const otherWrapper = ogreNotify->GetOgreNewtBody())
                        {
                            m_kinematicContactCallback(otherWrapper);
                        }
                    }
                }
            }
        }
    }
}
