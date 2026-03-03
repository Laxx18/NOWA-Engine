#include "OgreNewt_Stdafx.h"
#include "OgreNewt_ContactNotify.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_ContactCallback.h"
#include "OgreNewt_ContactJoint.h"
#include "OgreNewt_MaterialPair.h"
#include "OgreNewt_World.h"
#include "ndBodyKinematic.h"
#include "ndContact.h"

namespace OgreNewt
{
    // =========================================================================
    // Internal helpers
    // =========================================================================

    // Resolve an OgreNewt::Body* from an ndBodyKinematic via BodyNotify.
    // Returns nullptr for Newton-internal bodies (tree-mesh faces, sentinel, etc.)
    // that have no OgreNewt wrapper attached.
    // Body pointers must come from function arguments, NOT from GetBody0/GetBody1
    // -- see the ABI note in the header.
    static OgreNewt::Body* resolveOgreBody(const ndBodyKinematic* b)
    {
        if (!b)
        {
            return nullptr;
        }

        auto& np = const_cast<ndBodyKinematic*>(b)->GetNotifyCallback();
        if (!np)
        {
            return nullptr;
        }

        if (auto* bn = dynamic_cast<OgreNewt::BodyNotify*>(*np))
        {
            return bn->GetOgreNewtBody();
        }

        return nullptr;
    }

    // Resolve the self-collision group from an ndBodyKinematic.
    // Returns 0 for bodies without an OgreNewt wrapper.
    static unsigned int resolveCollisionGroup(const ndBodyKinematic* b)
    {
        OgreNewt::Body* ob = resolveOgreBody(b);
        return ob ? ob->getSelfCollisionGroup() : 0u;
    }

    // =========================================================================
    // OgreNewtMaterial
    // =========================================================================

    // Called by ndContactCallback's PRIVATE dispatcher for every active contact.
    // We do NOT call GetBody0/GetBody1 here (inline ABI hazard).
    // m_id0/m_id1 were baked at RegisterMaterial time so we look up the
    // MaterialPair directly.
    void OgreNewtMaterial::OnContactCallback(const ndContact* const contact, ndFloat32 timestep) const
    {
        if (!contact || !m_world)
        {
            return;
        }

        MaterialPair* pair = m_world->findMaterialPair((int)m_id0, (int)m_id1);
        if (!pair)
        {
            return;
        }

        // Update ndMaterial physics properties from the OgreNewt MaterialPair.
        // const_cast is intentional: these are mutable physics-state fields.
        OgreNewtMaterial* self = const_cast<OgreNewtMaterial*>(this);
        self->m_restitution = static_cast<ndFloat32>(pair->getDefaultElasticity());
        self->m_softness = static_cast<ndFloat32>(pair->getDefaultSoftness());
        self->m_staticFriction0 = static_cast<ndFloat32>(pair->getDefaultStaticFriction());
        self->m_staticFriction1 = static_cast<ndFloat32>(pair->getDefaultStaticFriction());
        self->m_dynamicFriction0 = static_cast<ndFloat32>(pair->getDefaultKineticFriction());
        self->m_dynamicFriction1 = static_cast<ndFloat32>(pair->getDefaultKineticFriction());
        self->m_skinMargin = static_cast<ndFloat32>(pair->getDefaultSurfaceThickness());

        if (pair->getContactCallback())
        {
            pair->getContactCallback()->contactsProcess(ContactJoint(const_cast<ndContact*>(contact)), timestep, 0);
        }
    }

    // Called by ndContactCallback's private dispatcher during broad-phase for
    // NEW contact pairs. Body pointers arrive as direct function arguments --
    // never via GetBody0/GetBody1 -- so they are always correct regardless of ABI.
    bool OgreNewtMaterial::OnAabbOverlap(const ndBodyKinematic* const body0, const ndBodyKinematic* const body1) const
    {
        if (!body0 || !body1)
        {
            return true;
        }

        // Self-collision group suppression.
        // Bodies in the same non-zero group do not collide with each other
        // (used for ragdolls, articulations, compound vehicles, etc.)
        const unsigned int g0 = resolveCollisionGroup(body0);
        const unsigned int g1 = resolveCollisionGroup(body1);
        if (g0 != 0u && g0 == g1)
        {
            return false;
        }

        if (!m_world)
        {
            return true;
        }

        MaterialPair* pair = m_world->findMaterialPair((int)m_id0, (int)m_id1);
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

        // Resolve OgreNewt::Body* for the user callback.
        // Safe: we have raw body ptrs from function arguments, not from inline getters.
        OgreNewt::Body* ob0 = resolveOgreBody(body0);
        OgreNewt::Body* ob1 = resolveOgreBody(body1);
        if (!ob0 || !ob1)
        {
            // One or both bodies are Newton-internal (no OgreNewt wrapper).
            // Allow the collision -- nothing user-side to suppress it.
            return true;
        }

        return pair->getContactCallback()->onAABBOverlap(ob0, ob1, 0) != 0;
    }

    // Called by ndContactCallback's private dispatcher for compound shape pairs.
    // We do NOT call GetBody0/GetBody1 -- see the ABI note.
    // Body pointers cannot be reconstructed safely here, so we pass nullptr to
    // the user callback and let them decide based on sub-shape instances alone.
    bool OgreNewtMaterial::OnAabbOverlap(const ndContact* const contact, ndFloat32 /*timestep*/, const ndShapeInstance& inst0, const ndShapeInstance& inst1) const
    {
        /*if (!contact || !m_world)
        {
            return true;
        }

        MaterialPair* pair = m_world->findMaterialPair((int)m_id0, (int)m_id1);
        if (!pair || !pair->getContactCallback())
        {
            return true;
        }

        return pair->getContactCallback()->onCompoundSubShapeOverlap(nullptr, nullptr, &inst0, &inst1);*/

        return true;
    }

    // =========================================================================
    // ContactNotify
    // =========================================================================

    ContactNotify::ContactNotify(OgreNewt::World* world) : ndContactCallback(), m_world(world), m_ogreNewtBody(nullptr), m_userData(nullptr)
    {
        // Wire up the fallback material so virtual dispatch reaches our overrides
        // even for contacts whose shape-material IDs have no registered pair.
        m_fallbackMaterial.m_world = world;
        m_fallbackMaterial.m_id0 = 0;
        m_fallbackMaterial.m_id1 = 0;

        // Sensible physics defaults for the fallback.
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

    // Wraps the incoming ndApplicationMaterial in an OgreNewtMaterial that carries
    // m_id0, m_id1, and m_world. Because ndContactCallback calls Clone() on the
    // registered material each time a new contact is created, every live contact
    // gets an OgreNewtMaterial whose callbacks can look up the MaterialPair without
    // ever calling GetBody0/GetBody1.
    ndApplicationMaterial& ContactNotify::RegisterMaterial(const ndApplicationMaterial& material, ndUnsigned32 id0, ndUnsigned32 id1)
    {
        const ndUnsigned32 lo = std::min(id0, id1);
        const ndUnsigned32 hi = std::max(id0, id1);
        auto key = std::make_pair(lo, hi);

        OgreNewtMaterial& entry = m_localMaterials[key];

        // Copy base ndApplicationMaterial properties from caller.
        static_cast<ndApplicationMaterial&>(entry) = material;

        // Bake our extra fields so Clone() produces a fully-wired OgreNewtMaterial.
        entry.m_id0 = lo;
        entry.m_id1 = hi;
        entry.m_world = m_world;

        // Delegate to ndContactCallback which stores a Clone() of 'entry' in its
        // internal material graph. Because entry is OgreNewtMaterial, Clone()
        // returns OgreNewtMaterial -- preserving id0/id1/world on every contact.
        return ndContactCallback::RegisterMaterial(entry, id0, id1);
    }

    // Newton calls this once per contact to pick the ndMaterial* stored on
    // ndContact::m_material. That pointer is cast to ndApplicationMaterial* and
    // its virtual OnContactCallback is dispatched.
    //
    // ndContactCallback::GetMaterial NEVER returns nullptr -- for unregistered
    // pairs it returns &m_defaultMaterial, which is a plain ndApplicationMaterial
    // whose virtual methods are all no-ops. We intercept that case and substitute
    // our OgreNewtMaterial fallback so virtual dispatch always reaches our overrides.
    ndMaterial* ContactNotify::GetMaterial(const ndContact* const contact, const ndShapeInstance& inst0, const ndShapeInstance& inst1) const
    {
        const ndUnsigned32 id0 = ndUnsigned32(inst0.GetMaterial().m_userId);
        const ndUnsigned32 id1 = ndUnsigned32(inst1.GetMaterial().m_userId);

        ndMaterial* mat = ndContactCallback::GetMaterial(id0, id1);

        // ndMaterial is not polymorphic. The graph always stores ndApplicationMaterial*
        // (either our OgreNewtMaterial or the base-class default), so static_cast is safe.
        // Then dynamic_cast on the polymorphic ndApplicationMaterial* to detect our subclass.
        ndApplicationMaterial* appMat = static_cast<ndApplicationMaterial*>(mat);
        if (dynamic_cast<OgreNewtMaterial*>(appMat))
        {
            return appMat;
        }

        // The base class returned its internal m_defaultMaterial (plain
        // ndApplicationMaterial -- no pair was registered). Return our
        // OgreNewtMaterial fallback so virtual dispatch reaches our overrides.
        // Update ids so OnContactCallback can look up any catch-all MaterialPair
        // the user may have registered under (0, 0).
        m_fallbackMaterial.m_id0 = std::min(id0, id1);
        m_fallbackMaterial.m_id1 = std::max(id0, id1);
        m_fallbackMaterial.m_world = m_world;
        return &m_fallbackMaterial;
    }

    // =========================================================================
    // Backward-compat accessors
    // =========================================================================

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

} // namespace OgreNewt