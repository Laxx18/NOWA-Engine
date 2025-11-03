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

    //! Bridge class that connects Newton 4.0's contact notify system with OgreNewt callbacks
    class _OgreNewtExport ContactNotify : public ndContactCallback
    {
    public:
        ContactNotify(OgreNewt::World* ogreNewt);
        virtual ~ContactNotify();

        ndApplicationMaterial& RegisterMaterial(const ndApplicationMaterial& material, ndUnsigned32 id0, ndUnsigned32 id1) override;

        ndMaterial* GetMaterial(const ndContact* const contact, const ndShapeInstance& inst0, const ndShapeInstance& inst1) const override;

        void OnContactCallback(const ndContact* const contact, ndFloat32 timestep) const override;

        bool OnAabbOverlap(const ndContact* const contact, ndFloat32 timestep) const override;

        bool OnCompoundSubShapeOverlap(const ndContact* const contact, ndFloat32 timestep, const ndShapeInstance* const subShapeA, const ndShapeInstance* const subShapeB) const override;

        // OgreNewt helper bridge
        void setOgreNewtBody(OgreNewt::Body* body);

        OgreNewt::Body* getOgreNewtBody() const;

        void setUserData(void* data);
        void* getUserData() const;

    protected:
        OgreNewt::Body* m_ogreNewtBody;
        void* m_userData;

    private:
        OgreNewt::World* m_world;
        mutable std::map<std::pair<ndUnsigned32, ndUnsigned32>, ndApplicationMaterial> m_localMaterials;
        mutable ndApplicationMaterial m_fallbackMaterial;
    };
}

#endif
