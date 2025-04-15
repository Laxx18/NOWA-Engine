#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_TriggerControllerManager.h"

namespace OgreNewt
{
	CustomTriggerManager::CustomTriggerManager(World* world, dFloat faceAngle, dFloat waveSpeed, dFloat wavePeriod, dFloat waveAmplitude)
		: dCustomTriggerManager(world->getNewtonWorld()),
		m_faceAngle(faceAngle),
		m_waveSpeed(waveSpeed),
		m_wavePeriod(wavePeriod),
		m_waveAmplitude(waveAmplitude)
	{

	}

	void CustomTriggerManager::OnEnter(const dCustomTriggerController* const me, dFloat timestep, NewtonBody* const guess) const
	{
		TriggerCallback* const callback = (TriggerCallback*)me->GetUserData();
		const OgreNewt::Body* visitor = (const OgreNewt::Body*) NewtonBodyGetUserData(guess);
		if (nullptr != visitor)
		{
			callback->OnEnter(visitor);
		}
		// m_world->criticalSectionLock();
	}

	void CustomTriggerManager::OnExit(const dCustomTriggerController* const me, dFloat timestep, NewtonBody* const guess) const
	{
		TriggerCallback* const callback = (TriggerCallback*)me->GetUserData();
		const OgreNewt::Body* visitor = (const OgreNewt::Body*) NewtonBodyGetUserData(guess);
		if (nullptr != visitor)
		{
			callback->OnExit(visitor);
		}
	}

	void CustomTriggerManager::WhileIn(const dCustomTriggerController* const me, dFloat timestep, NewtonBody* const guess) const
	{
		TriggerCallback* const callback = (TriggerCallback*)me->GetUserData();
		const OgreNewt::Body* visitor = (const OgreNewt::Body*) NewtonBodyGetUserData(guess);
		if (nullptr != visitor)
		{
			callback->OnInside(visitor);
		}
	}

	void CustomTriggerManager::update(dFloat dt)
	{
		// update stationary swimming pool water surface
		m_faceAngle = dMod(m_faceAngle + m_waveSpeed * dt, dFloat(2.0f * dPi));
	}

	dFloat CustomTriggerManager::getFaceAngle(void) const
	{
		return m_faceAngle;
	}

	dFloat CustomTriggerManager::getWaveSpeed(void) const
	{
		return m_waveSpeed;
	}

	dFloat CustomTriggerManager::getWavePeriod(void) const
	{
		return m_wavePeriod;
	}

	dFloat CustomTriggerManager::getWaveAmplitude(void) const
	{
		return m_waveAmplitude;
	}

}

