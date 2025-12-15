#include "OgreNewt_Stdafx.h"
#include "OgreNewt_TriggerBody.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_BodyNotify.h"   // to recover OgreNewt::Body from ndBodyNotify
#include "OgreNewt_Collision.h"

#include "ndWorld.h"
#include "ndBodyKinematic.h"
#include "ndBodyTriggerVolume.h"
#include "ndShapeInstance.h"
#include "ndSharedPtr.h"

namespace OgreNewt
{
    // Small adapter that forwards ND4 trigger callbacks to your TriggerCallback
    class OgreNewtTriggerVolume : public ndBodyTriggerVolume
    {
    public:
        OgreNewtTriggerVolume(TriggerCallback* cb)
            : ndBodyTriggerVolume(), m_cb(cb)
        {
        }

        void OnTriggerEnter(ndBodyKinematic* const body, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb) return;
            if (OgreNewt::Body* visitor = bodyToOgre(body))
            {
                m_cb->OnEnter(visitor);
            }
        }

        void OnTrigger(const ndContact* const contact, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb || !contact)
                return;

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
                return;

            if (OgreNewt::Body* visitor = bodyToOgre(otherKinematic))
            {
                // This is your "stay / inside" callback
                m_cb->OnInside(visitor);
            }
        }

        void OnTriggerExit(ndBodyKinematic* const body, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb) return;
            if (OgreNewt::Body* visitor = bodyToOgre(body))
            {
                m_cb->OnExit(visitor);
            }
        }

    private:
        static OgreNewt::Body* bodyToOgre(ndBodyKinematic* const body)
        {
            if (!body) return nullptr;
            if (ndBodyNotify* notify = body->GetNotifyCallback())
            {
                // Your BodyNotify stores a back-pointer to OgreNewt::Body
                if (auto* ogreNotify = dynamic_cast<BodyNotify*>(notify)) {
                    return ogreNotify->GetOgreNewtBody();
                }
            }
            return nullptr;
        }

        TriggerCallback* m_cb; // not owned; TriggerBody owns it (same as ND3 pattern)
    };

    // -------- TriggerBody --------

    TriggerBody::TriggerBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, TriggerCallback* triggerCallback, Ogre::SceneMemoryMgrTypes memoryType)
        : Body(world, sceneManager, memoryType)
        , m_triggerVolume(nullptr)
        , m_triggerCallback(triggerCallback)
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
            return;

        ndShapeInstance* srcInst = col->getShapeInstance();
        if (!srcInst && !col->getNewtonCollision())
            return;

        // Build shape instance on caller thread (safe)
        ndShapeInstance shapeInstance = srcInst
            ? ndShapeInstance(*srcInst)
            : ndShapeInstance(col->getNewtonCollision());

        // Allocate the new trigger on caller thread
        OgreNewtTriggerVolume* newTrigger = new OgreNewtTriggerVolume(m_triggerCallback);

        // Build initial matrix (identity for now)
        ndMatrix matrix(ndGetIdentityMatrix());
        newTrigger->SetMatrix(matrix);
        newTrigger->SetCollisionShape(shapeInstance);

        if (!m_bodyNotify)
            m_bodyNotify = new BodyNotify(this);

        // Cache old body pointers (if we are recreating)
        ndBodyKinematic* oldBody = m_body;
        OgreNewtTriggerVolume* oldTrigger = m_triggerVolume;

        // Do all world mutations on world thread / safe point
        m_world->enqueuePhysicsAndWait([this, newTrigger, oldBody, oldTrigger](World& w) mutable
            {
                // If there was an old trigger/body, remove it first
                if (oldBody)
                {
                    oldBody->SetNotifyCallback(nullptr);
                    w.destroyBody(oldBody);

                    // If we own it, delete it (most trigger volumes are owned here)
                    // Adjust this if your ownership differs.
                    delete oldBody;
                }

                // Install new trigger as our active body
                m_triggerVolume = newTrigger;
                m_body = newTrigger;

                // Attach notify + add to world
                m_body->SetNotifyCallback(m_bodyNotify);
                w.addBody(m_body);
            });

        // Note: triggers usually don't need damping/mass (fine)
    }

    TriggerCallback* TriggerBody::getTriggerCallback(void) const
    {
        return m_triggerCallback;
    }

    void TriggerBody::integrateVelocity(Ogre::Real dt)
    {
        // ND3: NewtonBodyIntegrateVelocity(m_body, dt);
        // ND4: integrate on the kinematic body
        if (m_body)
        {
            m_body->IntegrateVelocity(ndFloat32(dt));
        }
    }

} // namespace OgreNewt
