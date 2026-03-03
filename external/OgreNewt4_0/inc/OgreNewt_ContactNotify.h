#ifndef _INCLUDE_OGRENEWT_CONTACTNOTIFY
#define _INCLUDE_OGRENEWT_CONTACTNOTIFY

#include "OgreNewt_Prerequisites.h"
#include "ndContactCallback.h"
#include <map>

namespace OgreNewt
{
    class Body;
    class ContactCallback;
    class MaterialPair;

    // ── OgreNewtMaterial ──────────────────────────────────────────────────────
    //
    // ndApplicationMaterial subclass that carries id0/id1 and a World pointer.
    //
    // In the Newton 4 API, ndContactCallback dispatches contact events to the
    // registered ndApplicationMaterial's virtual methods (OnContactCallback,
    // OnAabbOverlap, OnCompoundSubShapeOverlap).  All per-contact logic therefore
    // lives HERE rather than on ContactNotify itself.
    //
    // IMPORTANT — GetBody0/GetBody1 are NEVER called in this file.
    // Those are inline functions whose struct-field offsets are baked at compile
    // time.  If OgreNewt.dll and ndNewton_d.dll are built from different Newton
    // headers the offsets differ → garbage pointer → crash.
    // We avoid the problem by:
    //   • reading id0/id1 stored on the material at registration time, and
    //   • receiving body pointers as direct function arguments in OnAabbOverlap.
    struct _OgreNewtExport OgreNewtMaterial : public ndApplicationMaterial
    {
        ndUnsigned32 m_id0 = 0;
        ndUnsigned32 m_id1 = 0;
        OgreNewt::World* m_world = nullptr; // set by ContactNotify::RegisterMaterial

        // Clone() must return OgreNewtMaterial so that every live contact gets
        // an instance whose vtable points to OUR overrides, not the base no-ops.
        ndApplicationMaterial* Clone() const override
        {
            return new OgreNewtMaterial(*this);
        }

        // Called by ndContactCallback's private dispatcher for every active contact.
        // Updates ndMaterial physics properties from the OgreNewt MaterialPair and
        // invokes ContactCallback::contactsProcess if one is registered.
        void OnContactCallback(const ndContact* const contact, ndFloat32 timestep) const override;

        // Called during broad-phase for NEW contact pairs.
        // Body pointers arrive as direct function arguments — never via GetBody0/1 —
        // so they are always correct regardless of ABI version.
        // Handles self-collision group suppression and the user onAABBOverlap callback.
        bool OnAabbOverlap(const ndBodyKinematic* const body0, const ndBodyKinematic* const body1) const override;

        // Called for compound sub-shape pairs.
        // Invokes ContactCallback::onCompoundSubShapeOverlap if registered.
        bool OnAabbOverlap(const ndContact* const contact, ndFloat32 timestep, const ndShapeInstance& inst0, const ndShapeInstance& inst1) const override;
    };

    // ── ContactNotify ─────────────────────────────────────────────────────────
    //
    // Thin shell over ndContactCallback. Responsibilities:
    //   1. RegisterMaterial — stores an OgreNewtMaterial (with world ptr + ids)
    //                         so ndContactCallback clones the right subclass into
    //                         every new contact.
    //   2. GetMaterial      — ensures an OgreNewtMaterial is ALWAYS returned.
    //                         ndContactCallback's default returns its internal
    //                         m_defaultMaterial (a plain ndApplicationMaterial)
    //                         for unregistered pairs, which would route callbacks
    //                         to the base class no-ops.  We substitute our own
    //                         OgreNewtMaterial fallback so virtual dispatch always
    //                         reaches our overrides.
    //
    // Do NOT add overrides for OnContactCallback / OnAabbOverlap /
    // OnCompoundSubShapeOverlap here. Those are dispatched to the material's
    // virtual methods (above) by ndContactCallback's private implementation.
    class _OgreNewtExport ContactNotify : public ndContactCallback
    {
    public:
        explicit ContactNotify(OgreNewt::World* world);
        virtual ~ContactNotify();

        // Wraps the incoming ndApplicationMaterial in an OgreNewtMaterial so that
        // id0/id1 and the world ptr are baked into every Clone() the contact system
        // creates.
        virtual ndApplicationMaterial& RegisterMaterial(const ndApplicationMaterial& material, ndUnsigned32 id0, ndUnsigned32 id1) override;

        // Returns an OgreNewtMaterial in ALL cases — either the registered one, or
        // the fallback — so that virtual dispatch always reaches our overrides.
        virtual ndMaterial* GetMaterial(const ndContact* const contact, const ndShapeInstance& inst0, const ndShapeInstance& inst1) const override;

        // Backward-compat accessors (kept for existing call sites)
        void setOgreNewtBody(OgreNewt::Body* body);
        OgreNewt::Body* getOgreNewtBody() const;
        void setUserData(void* data);
        void* getUserData() const;

    private:
        OgreNewt::World* m_world;
        OgreNewt::Body* m_ogreNewtBody;
        void* m_userData;

        // Local storage for OgreNewtMaterial instances keyed by (min_id, max_id).
        // ndContactCallback::RegisterMaterial copies via Clone(), so keeping our
        // own map ensures OgreNewtMaterial (not the base) is what gets cloned into
        // every new contact.
        mutable std::map<std::pair<ndUnsigned32, ndUnsigned32>, OgreNewtMaterial> m_localMaterials;

        // Returned by GetMaterial when no registered pair matches the contact's
        // shape-material IDs.  Must be OgreNewtMaterial so that virtual dispatch
        // reaches our overrides rather than the base class no-ops.
        mutable OgreNewtMaterial m_fallbackMaterial;
    };

} // namespace OgreNewt

#endif // _INCLUDE_OGRENEWT_CONTACTNOTIFY