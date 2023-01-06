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


#include "dStdafxVehicle.h"
#include "dVehicleNode.h"

dVehicleNode::dVehicleNode(dVehicleNode* const parent)
	:dCustomAlloc()
	,m_usedData(NULL)
	,m_parent(parent)
	,m_children()
	,m_index(-1)
{
	if (m_parent) {
		m_parent->m_children.Append(this);
		//m_parent->m_children.Addtop(this);
	}
}

dVehicleNode::~dVehicleNode()
{
	while (m_children.GetCount()) {
		delete m_children.GetFirst()->GetInfo();
		m_children.Remove(m_children.GetFirst());
	}
}

const void dVehicleNode::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	for (dVehicleNodeChildrenList::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		dVehicleNode* const node = child->GetInfo();
		node->Debug(debugContext);
	}
}

int dVehicleNode::GetKinematicLoops(dVehicleLoopJoint** const jointArray)
{
	int count = 0;
	for (dVehicleNodeChildrenList::dListNode* child = m_children.GetLast(); child; child = child->GetPrev()) {
		count += child->GetInfo()->GetKinematicLoops(&jointArray[count]);
	}
	return count;
}

void dVehicleNode::ApplyExternalForce()
{
	for (dVehicleNodeChildrenList::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		dVehicleNode* const node = child->GetInfo();
		node->ApplyExternalForce();
	}
}

void dVehicleNode::CalculateAABB(const NewtonCollision* const collision, const dMatrix& matrix, dVector& minP, dVector& maxP) const
{
	for (int i = 0; i < 3; i++) {
		dVector support(0.0f);
		dVector dir(0.0f);
		dir[i] = 1.0f;

		dVector localDir(matrix.UnrotateVector(dir));
		NewtonCollisionSupportVertex(collision, &localDir[0], &support[0]);
		support = matrix.TransformVector(support);
		maxP[i] = support[i];

		localDir = localDir.Scale(-1.0f);
		NewtonCollisionSupportVertex(collision, &localDir[0], &support[0]);
		support = matrix.TransformVector(support);
		minP[i] = support[i];
	}
}

void dVehicleNode::CalculateNodeAABB(const dMatrix& matrix, dVector& minP, dVector& maxP) const
{
	minP = matrix.m_posit;
	maxP = matrix.m_posit;
}

void dVehicleNode::Integrate(dFloat timestep)
{
	for (dVehicleNodeChildrenList::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		dVehicleNode* const node = child->GetInfo();
		node->Integrate(timestep);
	}
}

void dVehicleNode::CalculateFreeDof(dFloat timestep)
{
	for (dVehicleNodeChildrenList::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		dVehicleNode* const node = child->GetInfo();
		node->CalculateFreeDof(timestep);
	}
}

