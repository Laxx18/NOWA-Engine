#ifndef _OGRENEWT_BUOYANCYBODY_H_
#define _OGRENEWT_BUOYANCYBODY_H_

#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Collision.h"

#include <OgrePlane.h>
#include <OgreVector3.h>

class ndBodyTriggerVolume;

namespace OgreNewt
{
    class World;

    /// High-level callback used by PhysicsBuoyancyComponent for Lua/event handling.
    /// It NO LONGER applies any forces – pure event layer + per-body density setup.
    class _OgreNewtExport BuoyancyForceTriggerCallback
    {
    public:
        /// @param waterToSolidVolumeRatio  Density factor for bodies entering this volume (<1 = floats, >1 = sinks).
        /// @param viscosity                 Additional damping factor (0..1).
        BuoyancyForceTriggerCallback(
            Ogre::Real waterToSolidVolumeRatio,
            Ogre::Real viscosity);

        virtual ~BuoyancyForceTriggerCallback() = default;

        /// Trigger events (called from trigger volume)
        virtual void OnEnter(const Body* visitor);
        virtual void OnInside(const Body* visitor);
        virtual void OnExit(const Body* visitor);

        /// Per-frame update hook (currently no ND4 trigger-manager, so usually noop)
        virtual void update(Ogre::Real dt);

        // Parameters
        void setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio);
        Ogre::Real getWaterToSolidVolumeRatio() const;

        void setViscosity(Ogre::Real viscosity);
        Ogre::Real getViscosity() const;

    private:
        Ogre::Real m_waterToSolidVolumeRatio;
        Ogre::Real m_viscosity;
    };

    /// Buoyancy trigger "body" used in the world (ND4 trigger volume with Archimedes physics).
    /// All real buoyancy forces are computed in the internal ND4 trigger body.
    class _OgreNewtExport BuoyancyBody : public Body
    {
    public:
        BuoyancyBody(
            World* world,
            Ogre::SceneManager* sceneManager,
            const CollisionPtr& col,
            BuoyancyForceTriggerCallback* buoyancyForceTriggerCallback);

        virtual ~BuoyancyBody();

        /// Recreate the trigger volume if the collision changes.
        void reCreateTrigger(const CollisionPtr& col);

        // Parameters (mirrored into trigger volume)
        void setFluidPlane(const Ogre::Plane& fluidPlane);
        Ogre::Plane getFluidPlane() const;

        void setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio);
        Ogre::Real getWaterToSolidVolumeRatio() const;

        void setViscosity(Ogre::Real viscosity);
        Ogre::Real getViscosity() const;

        void setGravity(const Ogre::Vector3& gravity);
        Ogre::Vector3 getGravity() const;

        /// Wave parameters (for gentle perpetual bobbing)
        void setWaveAmplitude(Ogre::Real waveAmplitude);
        Ogre::Real getWaveAmplitude() const;

        void setWaveFrequency(Ogre::Real waveFrequency);
        Ogre::Real getWaveFrequency() const;

        /// Access to callback for scripting / component.
        BuoyancyForceTriggerCallback* getTriggerCallback() const;

        /// Per-frame update hook.
        void update(Ogre::Real dt);

        void setUseRaycastPlane(bool useRaycast);
        bool getUseRaycastPlane() const;

    private:
        Ogre::Plane   m_fluidPlane;
        Ogre::Real    m_waterToSolidVolumeRatio;
        Ogre::Real    m_viscosity;
        Ogre::Vector3 m_gravity;
        bool          m_useRaycastPlane;

        Ogre::Real    m_waveAmplitude;
        Ogre::Real    m_waveFrequency;

        ndBodyTriggerVolume* m_triggerBody;
        BuoyancyForceTriggerCallback* m_buoyancyForceTriggerCallback;
    };

} // namespace OgreNewt

#endif
