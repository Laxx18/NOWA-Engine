#include "OgreNewt_MaterialPair.h"
#include "ndContactCallback.h"

namespace OgreNewt
{
    MaterialPair::MaterialPair(World* world, const MaterialID* mat1, const MaterialID* mat2)
        : m_world(world),
        id0(mat1),
        id1(mat2),
        m_contactcallback(nullptr),
        m_defaultSoftness(0.1f),
        m_defaultElasticity(0.4f),
        m_defaultStaticFriction(0.9f),
        m_defaultKineticFriction(0.5f),
        m_defaultCollidable(1),
        m_defaultSurfaceThickness(0.0f)
    {
        // Register this pair in the world for lookup
        if (m_world)
        {
            m_world->registerMaterialPair(this);

            this->registerWithWorld();
        }
    }

    MaterialPair::~MaterialPair()
    {
        if (m_world)
        {
            m_world->unregisterMaterialPair(this);
        }

        m_world = nullptr;
        id0 = nullptr;
        id1 = nullptr;
        m_contactcallback = nullptr;
    }

    void MaterialPair::registerWithWorld()
    {
        if (!m_world)
            return;

        ndContactCallback* callback = (ndContactCallback*)m_world->getNewtonWorld()->GetContactNotify();
        if (!callback)
            return;

        ndApplicationMaterial material;
        material.m_softness = static_cast<ndFloat32>(m_defaultSoftness);
        material.m_restitution = static_cast<ndFloat32>(m_defaultElasticity);
        material.m_staticFriction0 = static_cast<ndFloat32>(m_defaultStaticFriction);
        material.m_staticFriction1 = static_cast<ndFloat32>(m_defaultStaticFriction);
        material.m_dynamicFriction0 = static_cast<ndFloat32>(m_defaultKineticFriction);
        material.m_dynamicFriction1 = static_cast<ndFloat32>(m_defaultKineticFriction);
        material.m_skinMargin = static_cast<ndFloat32>(m_defaultSurfaceThickness);

        // Register material for the pair
        int idA = id0->getID();
        int idB = id1->getID();
        callback->RegisterMaterial(material, idA, idB);
    }

}
