#include "OgreNewt_Stdafx.h"
#include "OgreNewt_ContactNotify.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_ContactCallback.h"
#include "OgreNewt_ContactJoint.h"
#include "OgreNewt_MaterialPair.h"
#include "ndBodyKinematic.h"
#include "ndContact.h"

namespace OgreNewt
{
    ContactNotify::ContactNotify(OgreNewt::World* world) : ndContactCallback(), m_ogreNewtBody(nullptr), m_userData(nullptr), m_world(world)
    {
        m_fallbackMaterial.m_restitution = 0.4f;
        m_fallbackMaterial.m_staticFriction0 = 0.9f;
        m_fallbackMaterial.m_staticFriction1 = 0.9f;
        m_fallbackMaterial.m_dynamicFriction0 = 0.5f;
        m_fallbackMaterial.m_dynamicFriction1 = 0.5f;
        m_fallbackMaterial.m_skinMargin = 0.0f;
        m_fallbackMaterial.m_softness = 0.1f;
    }

    ContactNotify::~ContactNotify()
    {
        m_localMaterials.clear();
    }

    ndApplicationMaterial& ContactNotify::RegisterMaterial(const ndApplicationMaterial& material, ndUnsigned32 id0, ndUnsigned32 id1)
    {
        // Store in OgreNewt's own local map for reference.
        // We intentionally do NOT call ndContactCallback::RegisterMaterial() here.
        //
        // Reason: ndContactCallback lives in ndModel_d.dll. Its RegisterMaterial
        // accesses m_materialGraph at a byte offset baked into that DLL at compile
        // time. If OgreNewt's alignment/padding for ndContactNotify (the base) differs
        // from what ndModel_d.dll expects, m_materialGraph lands in garbage memory
        // and the red-black tree traversal crashes.
        //
        // More importantly: ALL our virtual overrides (OnAabbOverlap, OnContactCallback,
        // OnCompoundSubShapeOverlap) use m_world->findMaterialPair() — the OgreNewt-side
        // map registered via World::registerMaterialPair(). Newton's m_materialGraph is
        // NEVER consulted by any of our overrides, so registering into it is dead code.
        auto key = std::make_pair(std::min(id0, id1), std::max(id0, id1));
        auto entry = m_localMaterials[key];
        entry = material;
        return entry;
    }

    ndMaterial* ContactNotify::GetMaterial(const ndContact* const contact, const ndShapeInstance& inst0, const ndShapeInstance& inst1) const
    {
        ndMaterial* mat = ndContactCallback::GetMaterial(contact, inst0, inst1);
        if (mat)
        {
            return mat;
        }

        // fallback if nothing found (for safety)
        return &m_fallbackMaterial;
    }

    void ContactNotify::OnContactCallback(const ndContact* const contact, ndFloat32 timestep) const
    {
        if (!contact)
        {
            return;
        }

        ndMaterial* const mat = const_cast<ndMaterial*>(contact->GetMaterial());
        if (!mat)
        {
            return;
        }

        // GetBody0/1 can be null in certain ND4 internal contact states
        const ndBodyKinematic* body0 = contact->GetBody0();
        const ndBodyKinematic* body1 = contact->GetBody1();

        if (!body0 || !body1)
        {
            const ndContactPointList& points = contact->GetContactPoints();
            if (points.GetCount() == 0)
            {
                return;
            }
            const ndContactPoint& cp = points.GetFirst()->GetInfo();
            body0 = cp.m_body0;
            body1 = cp.m_body1;
        }

        if (!body0 || !body1)
        {
            return;
        }

        const ndUnsigned32 id0 = body0->GetCollisionShape().m_shapeMaterial.m_userId;
        const ndUnsigned32 id1 = body1->GetCollisionShape().m_shapeMaterial.m_userId;

        MaterialPair* pair = m_world->findMaterialPair((int)id0, (int)id1);
        if (!pair)
        {
            return;
        }

        mat->m_restitution = static_cast<ndFloat32>(pair->getDefaultElasticity());
        mat->m_softness = static_cast<ndFloat32>(pair->getDefaultSoftness());
        mat->m_staticFriction0 = static_cast<ndFloat32>(pair->getDefaultStaticFriction());
        mat->m_staticFriction1 = static_cast<ndFloat32>(pair->getDefaultStaticFriction());
        mat->m_dynamicFriction0 = static_cast<ndFloat32>(pair->getDefaultKineticFriction());
        mat->m_dynamicFriction1 = static_cast<ndFloat32>(pair->getDefaultKineticFriction());
        mat->m_skinMargin = static_cast<ndFloat32>(pair->getDefaultSurfaceThickness());

        if (pair->getContactCallback())
        {
            pair->getContactCallback()->contactsProcess(ContactJoint(const_cast<ndContact*>(contact)), timestep, 0);
        }
    }

    // ── Body-pair broad phase ─────────────────────────────────────────────────
    // Called by Newton BEFORE a contact object exists (two AABBs newly overlap).
    // MUST be overridden — without it the base ndContactCallback::OnAabbOverlap
    // runs and calls GetCollisionShape() on internal Newton primitives (TreeCollision
    // face bodies) that have no valid shape instance -> null ref -> crash.
    bool ContactNotify::OnAabbOverlap(const ndBodyKinematic* const body0, const ndBodyKinematic* const body1) const
    {
        if (!body0 || !body1)
        {
            return true;
        }

        // Same-model suppression (spider, ragdoll, vehicle articulation).
        ndModel* model0 = const_cast<ndBodyKinematic*>(body0)->GetModel();
        ndModel* model1 = const_cast<ndBodyKinematic*>(body1)->GetModel();
        if (model0 && model0 == model1)
        {
            return false;
        }

        // Resolve via BodyNotify — never touch GetCollisionShape() on unknown bodies.
        auto np0 = const_cast<ndBodyKinematic*>(body0)->GetNotifyCallback();
        auto np1 = const_cast<ndBodyKinematic*>(body1)->GetNotifyCallback();
        if (!np0 || !np1)
        {
            return true;
        }

        BodyNotify* n0 = dynamic_cast<BodyNotify*>(*np0);
        BodyNotify* n1 = dynamic_cast<BodyNotify*>(*np1);
        if (!n0 || !n1)
        {
            return true;
        }

        OgreNewt::Body* ob0 = n0->GetOgreNewtBody();
        OgreNewt::Body* ob1 = n1->GetOgreNewtBody();
        if (!ob0 || !ob1)
        {
            return true;
        }

        // Self-collision group suppression.
        const unsigned int g0 = ob0->getSelfCollisionGroup();
        const unsigned int g1 = ob1->getSelfCollisionGroup();
        if (g0 != 0u && g0 == g1)
        {
            return false;
        }

        if (!m_world)
        {
            return true;
        }

        const int id0 = n0->GetMaterialId();
        const int id1 = n1->GetMaterialId();
        MaterialPair* pair = m_world->findMaterialPair(id0, id1);
        if (!pair)
        {
            return true;
        }
        if (!pair->getDefaultCollidable())
        {
            return false;
        }
        if (!pair->getContactCallback())
        {
            return true;
        }
        return pair->getContactCallback()->onAABBOverlap(ob0, ob1, 0) != 0;
    }

    // ── Contact broad phase ───────────────────────────────────────────────────
    bool ContactNotify::OnAabbOverlap(const ndContact* const contact, ndFloat32) const
    {
        if (!contact)
        {
            return true;
        }

        ndBodyKinematic* b0 = contact->GetBody0();
        ndBodyKinematic* b1 = contact->GetBody1();
        if (!b0 || !b1)
        {
            return true;
        }

        // ── Suppress intra-articulation collisions ────────────────────────────────
        // Both bodies belonging to the same ndModelArticulation means they are part
        // of the same spider (or any articulated model). No self-collision.
        ndModel* model0 = b0->GetModel();
        ndModel* model1 = b1->GetModel();
        if (model0 && model0 == model1)
        {
            return false;
        }

        // use Newton's real shape user IDs to find the pair
        const ndInt64 id0 = b0->GetCollisionShape().m_shapeMaterial.m_userId;
        const ndInt64 id1 = b1->GetCollisionShape().m_shapeMaterial.m_userId;
        MaterialPair* pair = m_world->findMaterialPair((int)id0, (int)id1);
        if (!pair || !pair->getContactCallback())
        {
            return true;
        }

        if (!pair->getDefaultCollidable())
        {
            return false;
        }

        auto notifyPtr0 = b0->GetNotifyCallback();
        auto notifyPtr1 = b1->GetNotifyCallback();

        if (!notifyPtr0 || !notifyPtr1)
        {
            return true;
        }

        BodyNotify* n0 = dynamic_cast<BodyNotify*>(*notifyPtr0);
        BodyNotify* n1 = dynamic_cast<BodyNotify*>(*notifyPtr1);

        if (!n0 || !n1)
        {
            return true;
        }

        OgreNewt::Body* ob0 = n0->GetOgreNewtBody();
        OgreNewt::Body* ob1 = n1->GetOgreNewtBody();

        if (!ob0 || !ob1)
        {
            return true;
        }

        const unsigned int g0 = ob0->getSelfCollisionGroup();
        const unsigned int g1 = ob1->getSelfCollisionGroup();
        if (g0 != 0 && g0 == g1)
        {
            // Same articulation group => no self-collision
            return false;
        }

        // delegate to your engine callback
        return pair->getContactCallback()->onAABBOverlap(ob0, ob1, 0) != 0;
    }

    bool ContactNotify::OnCompoundSubShapeOverlap(const ndContact* const contact, ndFloat32, const ndShapeInstance* const subA, const ndShapeInstance* const subB) const
    {
        if (!contact)
        {
            return true; // allow by default
        }

        ndBodyKinematic* b0 = contact->GetBody0();
        ndBodyKinematic* b1 = contact->GetBody1();
        if (!b0 || !b1)
        {
            return true;
        }

        // use Newton's real shape user IDs to find the pair
        const ndUnsigned32 id0 = b0->GetCollisionShape().m_shapeMaterial.m_userId;
        const ndUnsigned32 id1 = b1->GetCollisionShape().m_shapeMaterial.m_userId;
        MaterialPair* pair = m_world->findMaterialPair((int)id0, (int)id1);
        if (!pair || !pair->getContactCallback())
        {
            return true;
        }

        auto notifyPtr0 = b0->GetNotifyCallback();
        auto notifyPtr1 = b1->GetNotifyCallback();

        if (!notifyPtr0 || !notifyPtr1)
        {
            return true;
        }

        BodyNotify* n0 = dynamic_cast<BodyNotify*>(*notifyPtr0);
        BodyNotify* n1 = dynamic_cast<BodyNotify*>(*notifyPtr1);

        if (!n0 || !n1)
        {
            return true;
        }

        OgreNewt::Body* ob0 = n0->GetOgreNewtBody();
        OgreNewt::Body* ob1 = n1->GetOgreNewtBody();
        if (!ob0 || !ob1)
        {
            return true;
        }

        // delegate to your engine callback (return whether to keep this sub-shape contact)
        return pair->getContactCallback()->onCompoundSubShapeOverlap(ob0, ob1, subA, subB);
    }

    void ContactNotify::setOgreNewtBody(OgreNewt::Body* body)
    {
        m_ogreNewtBody = body;
    }

    OgreNewt::Body* ContactNotify::getOgreNewtBody() const
    {
        return m_ogreNewtBody;
    }

    void ContactNotify::setUserData(void* data)
    {
        m_userData = data;
    }

    void* ContactNotify::getUserData() const
    {
        return m_userData;
    }
}