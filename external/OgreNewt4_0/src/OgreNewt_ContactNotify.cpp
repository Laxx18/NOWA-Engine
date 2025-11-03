#include "OgreNewt_Stdafx.h"
#include "OgreNewt_ContactNotify.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_MaterialPair.h"
#include "OgreNewt_ContactJoint.h"
#include "OgreNewt_ContactCallback.h"
#include "ndBodyKinematic.h"
#include "ndContact.h"

namespace OgreNewt
{
    ContactNotify::ContactNotify(OgreNewt::World* world)
        : ndContactCallback(), m_ogreNewtBody(nullptr), m_userData(nullptr), m_world(world)
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
        auto key = std::make_pair(std::min(id0, id1), std::max(id0, id1));
        auto& entry = m_localMaterials[key];
        entry = material;

        // Call base class method to actually add to Newton’s internal graph
        ndApplicationMaterial& matRef = ndContactCallback::RegisterMaterial(entry, id0, id1);

        // Also register reverse order, because Newton sometimes hashes differently
        // if (id0 != id1)
          //   ndContactCallback::RegisterMaterial(entry, id1, id0);

        return matRef;
    }

    ndMaterial* ContactNotify::GetMaterial(const ndContact* const contact, const ndShapeInstance& inst0, const ndShapeInstance& inst1) const
    {
        ndMaterial* mat = ndContactCallback::GetMaterial(contact, inst0, inst1);
        if (mat)
            return mat;

        // fallback if nothing found (for safety)
        return &m_fallbackMaterial;
    }

    void ContactNotify::OnContactCallback(const ndContact* const contact, ndFloat32 timestep) const
    {
        if (!contact)
            return;

        // Get actual Newton material
        ndMaterial* const mat = const_cast<ndMaterial*>(contact->GetMaterial());
        if (!mat)
            return;

        // --- use Newton's real shape IDs ---
        const ndUnsigned32 id0 = contact->GetBody0()->GetCollisionShape().m_shapeMaterial.m_userId;
        const ndUnsigned32 id1 = contact->GetBody1()->GetCollisionShape().m_shapeMaterial.m_userId;

        // look up in your engine map
        MaterialPair* pair = m_world->findMaterialPair((int)id0, (int)id1);
        if (!pair)
            return;

        mat->m_restitution = static_cast<ndFloat32>(pair->getDefaultElasticity());
        mat->m_softness = static_cast<ndFloat32>(pair->getDefaultSoftness());
        mat->m_staticFriction0 = static_cast<ndFloat32>(pair->getDefaultStaticFriction());
        mat->m_staticFriction1 = static_cast<ndFloat32>(pair->getDefaultStaticFriction());
        mat->m_dynamicFriction0 = static_cast<ndFloat32>(pair->getDefaultKineticFriction());
        mat->m_dynamicFriction1 = static_cast<ndFloat32>(pair->getDefaultKineticFriction());
        mat->m_skinMargin = static_cast<ndFloat32>(pair->getDefaultSurfaceThickness());

        if (pair->getContactCallback())
            pair->getContactCallback()->contactsProcess(ContactJoint(const_cast<ndContact*>(contact)), timestep, 0);
    }

    bool ContactNotify::OnAabbOverlap(const ndContact* const contact, ndFloat32) const
    {
        if (!contact)
            return true;

        ndBodyKinematic* b0 = contact->GetBody0();
        ndBodyKinematic* b1 = contact->GetBody1();
        if (!b0 || !b1)
            return true;

        auto* n0 = dynamic_cast<ContactNotify*>(b0->GetNotifyCallback());
        auto* n1 = dynamic_cast<ContactNotify*>(b1->GetNotifyCallback());
        if (!n0 || !n1)
            return true;

        OgreNewt::Body* ob0 = n0->getOgreNewtBody();
        OgreNewt::Body* ob1 = n1->getOgreNewtBody();
        if (!ob0 || !ob1)
            return true;

        const MaterialID* mat0 = ob0->getMaterialGroupID();
        const MaterialID* mat1 = ob1->getMaterialGroupID();
        if (!mat0 || !mat1)
            return true;

        MaterialPair* pair = m_world->findMaterialPair(mat0->getID(), mat1->getID());
        if (pair && pair->getContactCallback())
        {
            return pair->getContactCallback()->onAABBOverlap(ob0, ob1, 0) != 0;
        }

        return true;
    }

    bool ContactNotify::OnCompoundSubShapeOverlap(
        const ndContact* const contact, ndFloat32, const ndShapeInstance* const subA,
        const ndShapeInstance* const subB) const
    {
        if (!contact)
            return false;

        ndBodyKinematic* b0 = contact->GetBody0();
        ndBodyKinematic* b1 = contact->GetBody1();
        if (!b0 || !b1)
            return false;

        auto* n0 = dynamic_cast<ContactNotify*>(b0->GetNotifyCallback());
        auto* n1 = dynamic_cast<ContactNotify*>(b1->GetNotifyCallback());
        if (!n0 || !n1)
            return false;

        OgreNewt::Body* ob0 = n0->getOgreNewtBody();
        OgreNewt::Body* ob1 = n1->getOgreNewtBody();
        if (!ob0 || !ob1)
            return false;

        const MaterialID* mat0 = ob0->getMaterialGroupID();
        const MaterialID* mat1 = ob1->getMaterialGroupID();
        if (!mat0 || !mat1)
            return false;

        MaterialPair* pair = m_world->findMaterialPair(mat0->getID(), mat1->getID());
        if (pair && pair->getContactCallback())
            return pair->getContactCallback()->onCompoundSubShapeOverlap(ob0, ob1, subA, subB);

        return false;
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
