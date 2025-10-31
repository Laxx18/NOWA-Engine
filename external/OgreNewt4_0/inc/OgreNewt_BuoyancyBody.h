#pragma once

#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_TriggerControllerManager.h" // for TriggerCallback base

#include <OgreVector3.h>
#include <OgrePlane.h>

namespace OgreNewt
{
    // ----------------------------------------------------------------
    // ndArchimedesBuoyancyVolume (ND4 trigger-based buoyancy volume)
    // ----------------------------------------------------------------
    class ndArchimedesBuoyancyVolume : public ndBodyTriggerVolume
    {
    public:
        D_CLASS_REFLECTION(ndArchimedesBuoyancyVolume, ndBodyTriggerVolume)

            ndArchimedesBuoyancyVolume();

        void CalculatePlane(ndBodyKinematic* const body);
        void OnTriggerEnter(ndBodyKinematic* const body, ndFloat32 timestep) override;
        void OnTrigger(ndBodyKinematic* const body, ndFloat32 timestep) override;
        void OnTriggerExit(ndBodyKinematic* const body, ndFloat32 timestep) override;

        void SetGravity(const ndVector& g) { m_gravity = g; }
        void SetDensity(ndFloat32 density) { m_density = density; }
        void SetViscosity(ndFloat32 viscosity) { m_viscosity = viscosity; }

    protected:
        ndVector   m_gravity;
        ndFloat32  m_density;
        ndFloat32  m_viscosity;
        ndPlane    m_plane;
        bool       m_hasPlane;
    };

    class Body;
    class _OgreNewtExport BuoyancyForceTriggerCallback : public TriggerCallback
    {
    public:
        BuoyancyForceTriggerCallback(
            const Ogre::Plane& fluidPlane,
            Ogre::Real waterToSolidVolumeRatio,
            Ogre::Real viscosity,
            const Ogre::Vector3& gravity,
            Ogre::Real waterSurfaceRestHeight = 0.0f);

        void OnEnter(const Body* visitor) override;
        void OnInside(const Body* visitor) override;
        void OnExit(const Body* visitor) override;
        void update(Ogre::Real dt);

        void setFluidPlane(const Ogre::Plane& fluidPlane);
        Ogre::Plane getFluidPlane() const;

        void setWaterToSolidVolumeRatio(Ogre::Real v);
        Ogre::Real getWaterToSolidVolumeRatio() const;

        void setViscosity(Ogre::Real v);
        Ogre::Real getViscosity() const;

        void setGravity(const Ogre::Vector3& g);
        Ogre::Vector3 getGravity() const;

        void setWaterSurfaceRestHeight(Ogre::Real h);
        Ogre::Real getWaterSurfaceRestHeight() const;

    protected:
        Ogre::Plane     m_fluidPlane;
        Ogre::Real      m_waterToSolidVolumeRatio;
        Ogre::Real      m_viscosity;
        Ogre::Vector3   m_gravity;
        Ogre::Real      m_waterSurfaceRestHeight;
    };

    // ----------------------------------------------------------------
    // BuoyancyBody (same API as before, ND4-compatible)
    // ----------------------------------------------------------------
    class _OgreNewtExport BuoyancyBody : public Body
    {
    public:
        BuoyancyBody(World* world, Ogre::SceneManager* sceneManager, const CollisionPtr& col, BuoyancyForceTriggerCallback* buoyancyForceTriggerCallback);

        virtual ~BuoyancyBody();

        void reCreateTrigger(const CollisionPtr& col);

        void setFluidPlane(const Ogre::Plane& fluidPlane);
        Ogre::Plane getFluidPlane() const;

        void setWaterToSolidVolumeRatio(Ogre::Real v);
        Ogre::Real getWaterToSolidVolumeRatio() const;

        void setViscosity(Ogre::Real v);
        Ogre::Real getViscosity() const;

        void setGravity(const Ogre::Vector3& g);
        Ogre::Vector3 getGravity() const;

        BuoyancyForceTriggerCallback* getTriggerCallback() const;
        void update(Ogre::Real dt);

    protected:
        Ogre::Plane                      m_fluidPlane;
        Ogre::Real                       m_waterToSolidVolumeRatio;
        Ogre::Real                       m_viscosity;
        Ogre::Vector3                    m_gravity;

        ndArchimedesBuoyancyVolume* m_triggerBody;
        BuoyancyForceTriggerCallback* m_buoyancyForceTriggerCallback;
    };

} // namespace OgreNewt
