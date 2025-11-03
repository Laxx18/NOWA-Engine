#include "OgreNewt_Stdafx.h"
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

        auto* callback = static_cast<ndContactCallback*>(m_world->getNewtonWorld()->GetContactNotify());
        if (!callback)
            return;

        const ndUnsigned32 idA = id0->getID();
        const ndUnsigned32 idB = id1->getID();

        ndApplicationMaterial newMat;
        newMat.m_softness = static_cast<ndFloat32>(m_defaultSoftness);
        newMat.m_restitution = static_cast<ndFloat32>(m_defaultElasticity);
        newMat.m_staticFriction0 = static_cast<ndFloat32>(m_defaultStaticFriction);
        newMat.m_staticFriction1 = static_cast<ndFloat32>(m_defaultStaticFriction);
        newMat.m_dynamicFriction0 = static_cast<ndFloat32>(m_defaultKineticFriction);
        newMat.m_dynamicFriction1 = static_cast<ndFloat32>(m_defaultKineticFriction);
        newMat.m_skinMargin = static_cast<ndFloat32>(m_defaultSurfaceThickness);

        // Register or update in Newton's internal material graph.
        callback->RegisterMaterial(newMat, idA, idB);
        if (idA != idB)
            callback->RegisterMaterial(newMat, idB, idA);
    }

    void MaterialPair::setDefaultSoftness(Ogre::Real softness)
    {
        m_defaultSoftness = softness;
        registerWithWorld();
    }

    void MaterialPair::setDefaultElasticity(Ogre::Real elasticity)
    {
        m_defaultElasticity = elasticity;
        registerWithWorld();
    }

    void MaterialPair::setDefaultFriction(Ogre::Real stat, Ogre::Real kinetic)
    {
        m_defaultStaticFriction = stat;
        m_defaultKineticFriction = kinetic;
        registerWithWorld();
    }

    void MaterialPair::setDefaultSurfaceThickness(float thickness)
    {
        m_defaultSurfaceThickness = thickness;
        registerWithWorld();
    }

    void MaterialPair::setDefaultCollidable(int state)
    {
        m_defaultCollidable = state;
        registerWithWorld();
    }
}
