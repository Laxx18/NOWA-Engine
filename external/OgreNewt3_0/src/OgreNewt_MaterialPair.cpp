#include "OgreNewt_Stdafx.h"
#include "OgreNewt_MaterialPair.h"
#include "OgreNewt_ContactJoint.h"

namespace OgreNewt
{


	MaterialPair::MaterialPair(const World* world, const MaterialID* mat1, const MaterialID* mat2)
		: m_world(world),
		id0(mat1),
		id1(mat2),
		m_contactcallback(nullptr)
	{
		NewtonMaterialSetCallbackUserData(m_world->getNewtonWorld(), id0->getID(), id1->getID(), this);
	}

	MaterialPair::~MaterialPair()
	{
		// setContactCallback(nullptr);
		m_world = nullptr;
		id0 = nullptr;
		id1 = nullptr;
		/*if (m_contactcallback)
		{
			delete m_contactcallback;
			m_contactcallback = nullptr;
		}*/
	}


	void MaterialPair::setContactCallback(OgreNewt::ContactCallback* callback)
	{
		m_contactcallback = callback;
		if (callback)
		{
			NewtonMaterialSetCollisionCallback(m_world->getNewtonWorld(), id0->getID(), id1->getID(), collisionCallback_onAABBOverlap, collisionCallback_contactsProcess);
		}
		else
		{
			if (nullptr != m_world)
			{
				NewtonMaterialSetCollisionCallback(m_world->getNewtonWorld(), id0->getID(), id1->getID(), nullptr, nullptr);
			}
		}
	}
	
	// int _CDECL MaterialPair::collisionCallback_onAABBOverlap(const NewtonMaterial* const material, const NewtonBody* const newtonBody0, const NewtonBody* const newtonBody1, int threadIndex)
	
	int _CDECL MaterialPair::collisionCallback_onAABBOverlap(const NewtonJoint* const contact, dFloat timestep, int threadIndex)
	{
		ContactJoint contactJoint(contact);

		MaterialPair* me = contactJoint.getMaterialPair();
		if (nullptr == me)
			return 0;

		return me->m_contactcallback->onAABBOverlap(contactJoint.getBody0(), contactJoint.getBody1(), threadIndex);
	}

	void _CDECL MaterialPair::collisionCallback_contactsProcess(const NewtonJoint* const contact, float timestep, int threadIndex)
	{
		ContactJoint contactJoint(contact);

		MaterialPair* me = contactJoint.getMaterialPair();

		if (me != nullptr)
		{
			(me->m_contactcallback->contactsProcess)(contactJoint, timestep, threadIndex);
		}
	}
}

