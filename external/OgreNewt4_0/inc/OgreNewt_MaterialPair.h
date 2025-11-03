#ifndef _INCLUDE_OGRENEWT_MATERIALPAIR
#define _INCLUDE_OGRENEWT_MATERIALPAIR

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_World.h"
#include "OgreNewt_ContactCallback.h"
#include "OgreNewt_MaterialID.h"

namespace OgreNewt
{
    class _OgreNewtExport MaterialPair
    {
    public:
        MaterialPair(World* world, const MaterialID* mat1, const MaterialID* mat2);
        ~MaterialPair();

        void setDefaultSoftness(Ogre::Real softness);
        void setDefaultElasticity(Ogre::Real elasticity);
        void setDefaultCollidable(int state);
        void setDefaultSurfaceThickness(float thickness);
        void setDefaultFriction(Ogre::Real stat, Ogre::Real kinetic);

        void setContactCallback(OgreNewt::ContactCallback* callback) { m_contactcallback = callback; }

        const MaterialID* getMaterial0() const { return id0; }
        const MaterialID* getMaterial1() const { return id1; }

        Ogre::Real getDefaultSoftness() const { return m_defaultSoftness; }
        Ogre::Real getDefaultElasticity() const { return m_defaultElasticity; }
        Ogre::Real getDefaultStaticFriction() const { return m_defaultStaticFriction; }
        Ogre::Real getDefaultKineticFriction() const { return m_defaultKineticFriction; }
        int getDefaultCollidable() const { return m_defaultCollidable; }
        float getDefaultSurfaceThickness() const { return m_defaultSurfaceThickness; }

        OgreNewt::ContactCallback* getContactCallback() const { return m_contactcallback; }
    private:
        void registerWithWorld();

    protected:
        const MaterialID* id0;
        const MaterialID* id1;
        World* m_world;
        OgreNewt::ContactCallback* m_contactcallback;

        Ogre::Real m_defaultSoftness;
        Ogre::Real m_defaultElasticity;
        Ogre::Real m_defaultStaticFriction;
        Ogre::Real m_defaultKineticFriction;
        int m_defaultCollidable;
        float m_defaultSurfaceThickness;
    };
}

#endif
