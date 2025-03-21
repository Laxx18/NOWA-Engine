/* Copyright (c) <2003-2019> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


// NewtonPlayerControllerManager.h: interface for the NewtonPlayerControllerManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef D_CUSTOM_PLAYER_CONTROLLER_H_
#define D_CUSTOM_PLAYER_CONTROLLER_H_

#include "dStdafxVehicle.h"
#include "dVehicle.h"

class dPlayerControllerImpulseSolver;
class dPlayerControllerContactSolver;

class dPlayerController: public dVehicle
{
	public:
	DVEHICLE_API dPlayerController(NewtonWorld* const world, const dMatrix& location, const dMatrix& localAxis, dFloat mass, dFloat radius, dFloat height, dFloat stepHeight);
	DVEHICLE_API ~dPlayerController();

	virtual dPlayerController* GetAsPlayerController() { return this; }

	DVEHICLE_API void ToggleCrouch ();

	bool IsCrouched () const {return m_isCrouched;}
	bool IsAirBorn () const {return m_isAirbone;}
	bool IsOnFloor () const {return m_isOnFloor;}
	const dFloat GetMass() const { return m_mass;}

	const dVector& GetImpulse() { return m_impulse; }
	void SetImpulse(const dVector& impulse) { m_impulse = impulse;}

	dFloat GetForwardSpeed() const { return -m_forwardSpeed; }
	void SetForwardSpeed(dFloat speed) {m_forwardSpeed = -speed; }

	dFloat GetLateralSpeed() const { return -m_lateralSpeed; }
	void SetLateralSpeed(dFloat speed) { m_lateralSpeed = -speed; }

	dFloat GetHeadingAngle() const { return m_headingAngle; }
	//void SetHeadingAngle(dFloat angle) {m_headingAngle = dClamp (angle, dFloat (-dPi), dFloat (dPi));}
	void SetHeadingAngle(dFloat angle) { m_headingAngle = angle; }

	DVEHICLE_API dVector GetVelocity() const;
	DVEHICLE_API void SetVelocity(const dVector& veloc);

	private:
	enum dCollisionState
	{
		m_colliding,
		m_freeMovement,
		m_deepPenetration,
	};

	
	void ResolveStep(dFloat timestep, dPlayerControllerContactSolver& contactSolver);
	void ResolveCollision(dPlayerControllerContactSolver& contactSolver, dFloat timestep);
	dFloat PredictTimestep(dFloat timestep, dPlayerControllerContactSolver& contactSolver);
	dCollisionState TestPredictCollision(const dPlayerControllerContactSolver& contactSolver, const dVector& veloc) const;
	void ResolveInterpenetrations(dPlayerControllerContactSolver& contactSolver, dPlayerControllerImpulseSolver& impulseSolver);

	protected:
	virtual dFloat ContactFrictionCallback(const dVector& position, const dVector& normal, int contactId, const NewtonBody* const otherbody) const 
	{ 
		return 2.0f; 
	}
	void PostUpdate(dFloat timestep)
	{
	}

	DVEHICLE_API virtual void UpdatePlayerStatus(dPlayerControllerContactSolver& contactSolver);
	DVEHICLE_API virtual const void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	DVEHICLE_API void PreUpdate(dFloat timestep);

	private:
	dVector m_impulse;
	dFloat m_mass;
	dFloat m_invMass;
	dFloat m_headingAngle;
	dFloat m_forwardSpeed;
	dFloat m_lateralSpeed;
	dFloat m_stepHeight;
	dFloat m_contactPatch;
	dFloat m_height;
	dFloat m_weistScale;
	dFloat m_crouchScale;
	//void* m_userData;
	bool m_isAirbone;
	bool m_isOnFloor;
	bool m_isCrouched;
	friend class dVehicleManager;
	friend class dPlayerControllerImpulseSolver;
};

#endif 

