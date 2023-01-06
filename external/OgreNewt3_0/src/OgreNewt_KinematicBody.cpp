#include "OgreNewt_Stdafx.h"
#include "OgreNewt_KinematicBody.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"

namespace OgreNewt
{
	
	KinematicBody::KinematicBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, 
		Ogre::SceneMemoryMgrTypes memoryType)
		: Body(world, sceneManager, memoryType)
	{
		dFloat m[16];
		OgreNewt::Converters::QuatPosToMatrix(m_curRotation, m_curPosit, m);
		m_body = NewtonCreateKinematicBody(m_world->getNewtonWorld(), col->getNewtonCollision(), &m[0]);

		NewtonBodySetUserData(m_body, this);
		NewtonBodySetDestructorCallback(m_body, newtonDestructor);
		NewtonBodySetTransformCallback(m_body, newtonTransformCallback);

		setLinearDamping(m_world->getDefaultLinearDamping() * (60.0f / m_world->getUpdateFPS()));
		setAngularDamping(m_world->getDefaultAngularDamping() * (60.0f / m_world->getUpdateFPS()));
	}

	KinematicBody::~KinematicBody()
	{

	}

	void KinematicBody::integrateVelocity(Ogre::Real dt)
	{
		NewtonBodyIntegrateVelocity(m_body, dt);
	}
}
