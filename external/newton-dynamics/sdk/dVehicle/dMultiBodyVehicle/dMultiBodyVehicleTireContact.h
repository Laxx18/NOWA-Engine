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


#ifndef __D_VEHICLE_TIRE_CONTACT_H__
#define __D_VEHICLE_TIRE_CONTACT_H__

#include "dStdafxVehicle.h"
#include "dVehicleLoopJoint.h"

#define D_TIRE_MAX_LATERAL_SLIP				dFloat (0.20f)
#define D_TIRE_MAX_ELASTIC_DEFORMATION		dFloat (0.05f)
#define D_TIRE_PENETRATION_RECOVERING_SPEED	dFloat (10.0f)

class dMultiBodyVehicleCollisionNode;

class dMultiBodyVehicleTireContact: public dVehicleLoopJoint
{
	public:
	class dTireModel
	{
		public:
		dTireModel()
		{
			memset (this, 0, sizeof (dTireModel));
		}

		dFloat m_tireLoad;
		dFloat m_lateralForce;
		dFloat m_longitunalForce;
		dFloat m_alingMoment;
		dFloat m_lateralSlip;
		dFloat m_longitudinalSlip;
		dFloat m_gammaStiffness;
	};

	dMultiBodyVehicleTireContact();
	void ResetContact();
	void Debug(dCustomJoint::dDebugDisplay* const debugContext, dFloat scale) const;
	void SetContact(dMultiBodyVehicleCollisionNode* const node, const dVector& posit, const dVector& normal, const dVector& lateralDir, dFloat penetration, dFloat staticFriction, bool isPatch);

	private:
	int GetMaxDof() const { return 3; }
	void TireForces(dFloat longitudinalSlip, dFloat lateralSlip, dFloat frictionCoef);
	void JacobianDerivative(dComplementaritySolver::dParamInfo* const constraintParams);
	void UpdateSolverForces(const dComplementaritySolver::dJacobianPair* const jacobians) const {}
	void SpecialSolverFrictionCallback(const dFloat* const force, dFloat* const lowFriction, dFloat* const highFriction) const;

	dVector m_point;
	dVector m_normal;
	dVector m_lateralDir;
	dVector m_longitudinalDir;
	dMultiBodyVehicleCollisionNode* m_collidingNode;
	dFloat m_penetration;
	dFloat m_staticFriction;
	mutable dTireModel m_tireModel;
	bool m_isContactPatch;
	bool m_isLowSpeed;

	friend class dMultiBodyVehicleTire;
};

#endif 

