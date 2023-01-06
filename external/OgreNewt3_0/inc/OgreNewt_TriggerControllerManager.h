/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

by melven

*/
#ifndef _INCLUDE_OGRENEWT_TRIGGER_CONTROLLER_MANAGER
#define _INCLUDE_OGRENEWT_TRIGGER_CONTROLLER_MANAGER

#include "OgreNewt_Prerequisites.h"
#include "dCustomTriggerManager.h"

namespace OgreNewt
{
	class World;

	class _OgreNewtExport TriggerCallback
	{
	public:
		TriggerCallback()
		{
		}

		virtual ~TriggerCallback()
		{
		}

		virtual void OnEnter(const OgreNewt::Body* visitor)
		{
		}

		virtual void OnInside(const OgreNewt::Body* visitor)
		{
		}

		virtual void OnExit(const OgreNewt::Body* visitor)
		{
		}
	};

	class _OgreNewtExport CustomTriggerManager : public dCustomTriggerManager
	{
	public:
		CustomTriggerManager(World* world, dFloat faceAngle = 0.0f, dFloat waveSpeed = 0.75f, dFloat wavePeriod = 4.0f, dFloat waveAmplitude = 0.25f);

		virtual void OnEnter(const dCustomTriggerController* const me, dFloat timestep, NewtonBody* const guess) const override;

		virtual void OnExit(const dCustomTriggerController* const me, dFloat timestep, NewtonBody* const guess) const override;

		virtual void WhileIn(const dCustomTriggerController* const me, dFloat timestep, NewtonBody* const guess) const override;

		void update(dFloat dt);

		dFloat getFaceAngle(void) const;

		dFloat getWaveSpeed(void) const;

		dFloat getWavePeriod(void) const;

		dFloat getWaveAmplitude(void) const;
	private:
		dFloat m_faceAngle;
		dFloat m_waveSpeed;
		dFloat m_wavePeriod;
		dFloat m_waveAmplitude;
	};
}


#endif

