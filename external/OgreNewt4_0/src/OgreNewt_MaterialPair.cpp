#include "OgreNewt_Stdafx.h"
#include "OgreNewt_MaterialPair.h"
#include "ndContactCallback.h"

namespace OgreNewt
{
    MaterialPair::MaterialPair(World* world, const MaterialID* mat1, const MaterialID* mat2) :
        m_world(world),
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
        // Nothing to do here.
        //
        // Previously this cast to ndContactCallback* and called RegisterMaterial()
        // to populate Newton's internal m_materialGraph red-black tree. That caused
        // crashes because ndContactCallback lives in ndModel_d.dll and accesses
        // m_materialGraph at byte offsets baked into that DLL — offsets that can
        // differ from OgreNewt's view of the object layout due to alignment padding
        // in ndContactNotify.
        //
        // OgreNewt's ContactNotify overrides ALL virtual callbacks and uses
        // m_world->findMaterialPair() (the OgreNewt-side map, populated via
        // World::registerMaterialPair() called from the MaterialPair constructor).
        // Newton's m_materialGraph is never consulted, so we never need to populate it.
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