#include "OgreNewt_Stdafx.h"
#include "OgreNewt_ContactNotify.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_ContactCallback.h"
#include "OgreNewt_ContactJoint.h"
#include "OgreNewt_MaterialPair.h"
#include "ndBodyKinematic.h"
#include "ndContact.h"

namespace OgreNewt
{
	ContactNotify::ContactNotify(OgreNewt::World* ogreNewt)
		: ndContactNotify(ogreNewt->getNewtonWorld()->GetScene())
		, ogreNewt(ogreNewt)
		, m_ogreNewtBody(nullptr)
		, m_userData(nullptr)
	{
	}

	ContactNotify::~ContactNotify()
	{
		m_materialCallbacks.clear();
	}

	void ContactNotify::OnContactCallback(const ndContact* const contact, ndFloat32 timestep) const
	{
		if (!contact) return;

		ContactJoint contactJoint(const_cast<ndContact*>(contact));

		// Get the two bodies
		OgreNewt::Body* ogreBody0 = nullptr;
		OgreNewt::Body* ogreBody1 = nullptr;

		ndBodyKinematic* newtonBody0 = contact->GetBody0();
		ndBodyKinematic* newtonBody1 = contact->GetBody1();

		if (newtonBody0 && newtonBody1)
		{
			ContactNotify* notify0 = dynamic_cast<ContactNotify*>(newtonBody0->GetNotifyCallback());
			ContactNotify* notify1 = dynamic_cast<ContactNotify*>(newtonBody1->GetNotifyCallback());

			if (notify0) ogreBody0 = notify0->getOgreNewtBody();
			if (notify1) ogreBody1 = notify1->getOgreNewtBody();
		}

		if (!ogreBody0 || !ogreBody1) return;

		const MaterialID* mat0 = ogreBody0->getMaterialGroupID();
		const MaterialID* mat1 = ogreBody1->getMaterialGroupID();
		if (!mat0 || !mat1) return;

		// Lookup the material pair stored in the world
		MaterialPair* pair = ogreNewt->findMaterialPair(mat0->getID(), mat1->getID());
		if (!pair) return;

		// Apply defaults only if no per-contact overrides exist
		const ndContactPointList& points = contact->GetContactPoints();
		for (ndContactPointList::ndNode* node = points.GetFirst(); node; node = node->GetNext())
		{
			auto& contactPoint = node->GetInfo();

			// Only set if the engine hasn't overridden
			if (contactPoint.m_material.m_softness < 0.0f)  // use negative as "unset"
				contactPoint.m_material.m_softness = pair->getDefaultSoftness();
			if (contactPoint.m_material.m_restitution < 0.0f)
				contactPoint.m_material.m_restitution = pair->getDefaultElasticity();
			if (contactPoint.m_material.m_staticFriction0 < 0.0f)
			{
				contactPoint.m_material.m_staticFriction0 = pair->getDefaultStaticFriction();
				contactPoint.m_material.m_staticFriction1 = pair->getDefaultStaticFriction();
				contactPoint.m_material.m_dynamicFriction0 = pair->getDefaultKineticFriction();
				contactPoint.m_material.m_dynamicFriction1 = pair->getDefaultKineticFriction();
			}
		}

		// Call user callback if any
		if (pair->getContactCallback())
		{
			pair->getContactCallback()->contactsProcess(contactJoint, timestep, 0);
		}
	}

	bool ContactNotify::OnAabbOverlap(const ndContact* const contact, ndFloat32 timestep) const
	{
		if (!contact)
			return true; // Allow collision by default

		// Get the two Newton bodies
		ndBodyKinematic* newtonBody0 = contact->GetBody0();
		ndBodyKinematic* newtonBody1 = contact->GetBody1();
		if (!newtonBody0 || !newtonBody1)
			return true;

		// Get OgreNewt bodies
		ContactNotify* notify0 = dynamic_cast<ContactNotify*>(newtonBody0->GetNotifyCallback());
		ContactNotify* notify1 = dynamic_cast<ContactNotify*>(newtonBody1->GetNotifyCallback());
		if (!notify0 || !notify1)
			return true;

		OgreNewt::Body* ogreBody0 = notify0->getOgreNewtBody();
		OgreNewt::Body* ogreBody1 = notify1->getOgreNewtBody();
		if (!ogreBody0 || !ogreBody1)
			return true;

		// Get Material IDs
		const MaterialID* mat0 = ogreBody0->getMaterialGroupID();
		const MaterialID* mat1 = ogreBody1->getMaterialGroupID();
		if (!mat0 || !mat1)
			return true;

		// Lookup the MaterialPair stored in the world
		MaterialPair* pair = ogreNewt->findMaterialPair(mat0->getID(), mat1->getID());
		if (!pair)
			return true;

		// Call the AABB overlap callback if it exists
		OgreNewt::ContactCallback* callback = pair->getContactCallback();
		if (callback)
		{
			int result = callback->onAABBOverlap(ogreBody0, ogreBody1, 0); // threadIndex = 0
			return result != 0;
		}

		return true; // Allow collision by default
	}

	bool ContactNotify::OnCompoundSubShapeOverlap(const ndContact* const contact, ndFloat32 timestep, const ndShapeInstance* const subShapeA, const ndShapeInstance* const subShapeB) const
	{
		if (!contact)
			return false;

		// Get the two Newton bodies
		ndBodyKinematic* newtonBody0 = contact->GetBody0();
		ndBodyKinematic* newtonBody1 = contact->GetBody1();
		if (!newtonBody0 || !newtonBody1)
			return false;

		// Get OgreNewt bodies
		ContactNotify* notify0 = dynamic_cast<ContactNotify*>(newtonBody0->GetNotifyCallback());
		ContactNotify* notify1 = dynamic_cast<ContactNotify*>(newtonBody1->GetNotifyCallback());
		if (!notify0 || !notify1)
			return false;

		OgreNewt::Body* ogreBody0 = notify0->getOgreNewtBody();
		OgreNewt::Body* ogreBody1 = notify1->getOgreNewtBody();
		if (!ogreBody0 || !ogreBody1)
			return false;

		// Get material IDs
		const MaterialID* mat0 = ogreBody0->getMaterialGroupID();
		const MaterialID* mat1 = ogreBody1->getMaterialGroupID();
		if (!mat0 || !mat1)
			return false;

		// Lookup MaterialPair in world
		MaterialPair* pair = ogreNewt->findMaterialPair(mat0->getID(), mat1->getID());
		if (!pair)
			return false;

		// Call callback if exists
		ContactCallback* callback = pair->getContactCallback();
		if (callback)
		{
			return callback->onCompoundSubShapeOverlap(ogreBody0, ogreBody1, subShapeA, subShapeB);
		}

		return false;
	}

	void ContactNotify::setOgreNewtBody(OgreNewt::Body* body)
	{
		m_ogreNewtBody = body;
	}

	OgreNewt::Body* ContactNotify::getOgreNewtBody() const
	{
		return m_ogreNewtBody;
	}

	void ContactNotify::setUserData(void* data)
	{
		m_userData = data;
	}

	void* ContactNotify::getUserData() const
	{
		return m_userData;
	}

	void ContactNotify::registerMaterialPairCallback(int materialID0, int materialID1, OgreNewt::ContactCallback* callback)
	{
		int key = getMaterialPairKey(materialID0, materialID1);
		m_materialCallbacks[key] = callback;
	}

	void ContactNotify::unregisterMaterialPairCallback(int materialID0, int materialID1)
	{
		int key = getMaterialPairKey(materialID0, materialID1);
		m_materialCallbacks.erase(key);
	}

	OgreNewt::ContactCallback* ContactNotify::findCallback(OgreNewt::Body* body0, OgreNewt::Body* body1) const
	{
		if (!body0 || !body1)
			return nullptr;

		// Get material IDs from bodies
		// Note: You'll need to add getMaterialGroupID() to the Body class
		const MaterialID* mat0 = body0->getMaterialGroupID();
		const MaterialID* mat1 = body1->getMaterialGroupID();

		if (!mat0 || !mat1)
			return nullptr;

		// Try to find callback for this material pair
		int key = getMaterialPairKey(mat0->getID(), mat1->getID());
		auto it = m_materialCallbacks.find(key);

		if (it != m_materialCallbacks.end())
		{
			return it->second;
		}

		// Try reverse order
		key = getMaterialPairKey(mat1->getID(), mat0->getID());
		it = m_materialCallbacks.find(key);

		if (it != m_materialCallbacks.end())
		{
			return it->second;
		}

		return nullptr;
	}

	int ContactNotify::getMaterialPairKey(int mat0, int mat1) const
	{
		// Ensure consistent ordering: smaller ID first
		if (mat0 > mat1)
		{
			int temp = mat0;
			mat0 = mat1;
			mat1 = temp;
		}

		// Combine into single key (assuming material IDs are < 65536)
		return (mat0 << 16) | mat1;
	}

}