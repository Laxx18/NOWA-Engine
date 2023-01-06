#include "OgreNewt_Stdafx.h"
#include "OgreNewt_TriggerBody.h"
#include "OgreNewt_Tools.h"

namespace OgreNewt
{

	TriggerBody::TriggerBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, TriggerCallback* triggerCallback, Ogre::SceneMemoryMgrTypes memoryType)
		: Body(world, sceneManager, memoryType),
		m_triggerCallback(triggerCallback),
		m_triggerController(nullptr)
	{
		this->reCreateTrigger(col);
	}

	TriggerBody::~TriggerBody()
	{
		if (nullptr == m_body)
			return;
		
		if (nullptr != m_triggerCallback)
		{
			delete m_triggerCallback;
			m_triggerCallback = nullptr;
		}
		if (nullptr != m_triggerController)
		{
			CustomTriggerManager* triggerManager = m_world->getTriggerManager();
			triggerManager->DestroyTrigger(m_triggerController);
			m_triggerController = nullptr;
		}
	}

	void TriggerBody::reCreateTrigger(const OgreNewt::CollisionPtr& col)
	{
		CustomTriggerManager* triggerManager = m_world->getTriggerManager();

		if (nullptr != m_triggerController)
		{
			CustomTriggerManager* triggerManager = m_world->getTriggerManager();
			triggerManager->DestroyTrigger(m_triggerController);
			m_triggerController = nullptr;
		}

		dFloat matrix[16];
		OgreNewt::Converters::QuatPosToMatrix(Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, matrix);

		m_triggerController = triggerManager->CreateTrigger(&matrix[0], col->getNewtonCollision(), m_triggerCallback);
		// m_body is the trigger body
		m_body = m_triggerController->GetBody();
		
		// Check this
		// NewtonBodySetDestructorCallback(m_body, newtonDestructor);
// Attention: Is this required?
		NewtonBodySetTransformCallback(m_body, newtonTransformCallback);

		// setLinearDamping(m_world->getDefaultLinearDamping() * (60.0f / m_world->getUpdateFPS()));
		// setAngularDamping(m_world->getDefaultAngularDamping() * (60.0f / m_world->getUpdateFPS()));

		NewtonBodySetUserData(m_body, this);
		
		NewtonDestroyCollision(col->getNewtonCollision());

		// m_triggerController->SetUserData(m_triggerCallback);
	}

	TriggerCallback* TriggerBody::getTriggerCallback(void) const
	{
		return m_triggerCallback;
	}

	void TriggerBody::integrateVelocity(Ogre::Real dt)
	{
		NewtonBodyIntegrateVelocity(m_body, dt);
	}
}
