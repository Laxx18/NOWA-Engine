#include "OgreNewt_Stdafx.h"
#include "OgreNewt_TriggerBody.h"
#include "OgreNewt_BodyNotify.h" // to recover OgreNewt::Body from ndBodyNotify
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"

#include "ndBodyKinematic.h"
#include "ndBodyTriggerVolume.h"
#include "ndShapeInstance.h"
#include "ndSharedPtr.h"
#include "ndWorld.h"

namespace OgreNewt
{
    // Small adapter that forwards ND4 trigger callbacks to your TriggerCallback
    class OgreNewtTriggerVolume : public ndBodyTriggerVolume
    {
    public:
        OgreNewtTriggerVolume(TriggerCallback* cb) : ndBodyTriggerVolume(), m_cb(cb)
        {
        }

        void OnTriggerEnter(ndBodyKinematic* const body, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb)
            {
                return;
            }
            if (OgreNewt::Body* visitor = bodyToOgre(body))
            {
                m_cb->OnEnter(visitor);
            }
        }

        void OnTrigger(const ndContact* const contact, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb || !contact)
            {
                return;
            }

            // The trigger itself is this ndBodyTriggerVolume (which is an ndBodyKinematic)
            ndBodyKinematic* const self = static_cast<ndBodyKinematic*>(this);

            // Figure out which body is the "other" one in the contact
            ndBodyKinematic* otherKinematic = nullptr;

            if (contact->GetBody0() != self)
            {
                otherKinematic = contact->GetBody0()->GetAsBodyKinematic();
            }
            else
            {
                otherKinematic = contact->GetBody1()->GetAsBodyKinematic();
            }

            if (!otherKinematic)
            {
                return;
            }

            if (OgreNewt::Body* visitor = bodyToOgre(otherKinematic))
            {
                // This is your "stay / inside" callback
                m_cb->OnInside(visitor);
            }
        }

        void OnTriggerExit(ndBodyKinematic* const body, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb)
            {
                return;
            }
            if (OgreNewt::Body* visitor = bodyToOgre(body))
            {
                m_cb->OnExit(visitor);
            }
        }

    private:
        static OgreNewt::Body* bodyToOgre(ndBodyKinematic* const body)
        {
            if (!body)
            {
                return nullptr;
            }

            ndSharedPtr<ndBodyNotify>& notifyPtr = body->GetNotifyCallback();
            if (notifyPtr)
            {
                if (auto* ogreNotify = dynamic_cast<BodyNotify*>(*notifyPtr))
                {
                    return ogreNotify->GetOgreNewtBody();
                }
            }
            return nullptr;
        }

        TriggerCallback* m_cb; // not owned; TriggerBody owns it (same as ND3 pattern)
    };

    // -------- TriggerBody --------

    TriggerBody::TriggerBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, TriggerCallback* triggerCallback, Ogre::SceneMemoryMgrTypes memoryType) :
        Body(world, sceneManager, memoryType),
        m_triggerVolume(nullptr),
        m_triggerCallback(triggerCallback)
    {
        reCreateTrigger(col);
    }

    TriggerBody::~TriggerBody()
    {
        // We do NOT delete m_triggerVolume here because it is owned by the ND4 world (shared_ptr).
        // Just null our raw pointer.
        m_triggerVolume = nullptr;

        if (m_triggerCallback)
        {
            delete m_triggerCallback;
            m_triggerCallback = nullptr;
        }
    }

    void TriggerBody::reCreateTrigger(const OgreNewt::CollisionPtr& col)
    {
        if (!m_world || !col)
        {
            return;
        }

        ndShapeInstance* srcInst = col->getShapeInstance();
        if (!srcInst && !col->getNewtonCollision())
        {
            return;
        }

        ndShapeInstance shapeInstance = srcInst ? ndShapeInstance(*srcInst) : ndShapeInstance(col->getNewtonCollision());

        OgreNewtTriggerVolume* newTrigger = new OgreNewtTriggerVolume(m_triggerCallback);
        newTrigger->SetMatrix(ndGetIdentityMatrix());
        newTrigger->SetCollisionShape(shapeInstance);

        if (!m_bodyNotifyPtr)
        {
            m_bodyNotifyPtr = ndSharedPtr<ndBodyNotify>(new BodyNotify(this));
        }

        // Capture OLD SharedPtr by value — keeps old body alive until lambda exits
        ndSharedPtr<ndBody> oldBodyPtr = m_bodyPtr;

        // NEW SharedPtr takes ownership
        m_bodyPtr = ndSharedPtr<ndBody>(newTrigger);
        m_triggerVolume = newTrigger;

        m_world->enqueuePhysicsAndWait(
            [this, newTrigger, oldBodyPtr](World& w) mutable
            {
                if (oldBodyPtr)
                {
                    // Sever old notify before removal
                    (*oldBodyPtr)->GetNotifyCallback() = ndSharedPtr<ndBodyNotify>();
                    w.destroyBody(std::move(oldBodyPtr));
                }
                newTrigger->SetNotifyCallback(m_bodyNotifyPtr);
                w.addBody(m_bodyPtr);
            });
    }

    TriggerCallback* TriggerBody::getTriggerCallback(void) const
    {
        return m_triggerCallback;
    }

    void TriggerBody::integrateVelocity(Ogre::Real dt)
    {
        // ND3: NewtonBodyIntegrateVelocity(m_body, dt);
        // ND4: integrate on the kinematic body
        if (getNewtonBody())
        {
            getNewtonBody()->IntegrateVelocity(ndFloat32(dt));
        }
    }

} // namespace OgreNewt
