#include "OgreNewt_Stdafx.h"
#include "OgreNewt_ContactJoint.h"
#include "OgreNewt_World.h"
#include "ndBodyKinematic.h"
#include "ndContact.h"
#include "ndContactOptions.h"

namespace OgreNewt
{
	ContactJoint::ContactJoint(ndContact* contact)
		: m_contact(contact)
	{
	}

	ContactJoint::~ContactJoint()
	{
	}

	Contact ContactJoint::getFirstContact()
	{
		ndContactPointList::ndNode* contactPointsNode = m_contact->GetContactPoints().GetFirst();
		Contact contact(contactPointsNode, this);
	}

	int ContactJoint::getContactCount() const
	{
		if (m_contact)
		{
			return m_contact->GetContactPoints().GetCount();
		}
		return 0;
	}

	OgreNewt::Body* ContactJoint::getBody0()
	{
		if (m_contact)
		{
			ndBodyKinematic* body = m_contact->GetBody0();
			if (body)
			{
				return static_cast<OgreNewt::Body*>(body->GetNotifyCallback()->GetUserData());
			}
		}
		return nullptr;
	}

	OgreNewt::Body* ContactJoint::getBody1()
	{
		if (m_contact)
		{
			ndBodyKinematic* body = m_contact->GetBody1();
			if (body)
			{
				return static_cast<OgreNewt::Body*>(body->GetNotifyCallback()->GetUserData());
			}
		}
		return nullptr;
	}

	ndContact* ContactJoint::_getNewtonContact()
	{
		return m_contact;
	}

	MaterialPair* ContactJoint::getMaterialPair()
	{
		// Newton 4.0 doesn't use material pairs anymore
		// Material behavior is handled through body notify callbacks
		return nullptr;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Contact::Contact(ndContactPointList::ndNode* contactNode, ContactJoint* parent)
		: m_parent(parent),
		m_contactNode(contactNode),
		m_index(0)
	{
	}

	Contact Contact::getNext() const
	{
		if (!m_contactNode)
			return Contact(nullptr, m_parent);

		ndContactPointList::ndNode* nextNode = m_contactNode->GetNext();
		return nextNode ? Contact(nextNode, m_parent) : Contact(nullptr, m_parent);
	}

	bool Contact::isEmpty() const
	{
		return m_contactNode == nullptr;
	}

	Ogre::Vector3 Contact::getForce() const
	{
		if (!m_contactNode)
			return Ogre::Vector3::ZERO;

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		// Include all force components
		ndVector force = contactPoint.m_normal * contactPoint.m_normal_Force.m_force
			+ contactPoint.m_dir0 * contactPoint.m_dir0_Force.m_force
			+ contactPoint.m_dir1 * contactPoint.m_dir1_Force.m_force;

		return Ogre::Vector3(force.m_x, force.m_y, force.m_z);
	}


	Contact::operator bool() const
	{
		return m_contactNode != nullptr;
	}

	OgreNewt::Body* Contact::getBody0() const
	{
		if (m_parent)
		{
			return m_parent->getBody0();
		}
		return nullptr;
	}

	OgreNewt::Body* Contact::getBody1() const
	{
		if (m_parent)
		{
			return m_parent->getBody1();
		}
		return nullptr;
	}

	unsigned Contact::getFaceAttribute() const
	{
		if (!m_contactNode)
			return 0;

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		// In Newton 4.0, shape attributes are stored differently
		// TODO: Is this correct?
		return contactPoint.m_shapeId0; // or m_shapeId1 depending on which shape you need
	}

	Ogre::Real Contact::getNormalSpeed() const
	{
		if (!m_contactNode)
			return 0.0f;

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		const ndBodyKinematic* body0 = contactPoint.m_body0;
		const ndBodyKinematic* body1 = contactPoint.m_body1;

		if (!body0 || !body1)
			return 0.0f;

		// Compute velocities at contact point
		ndVector v0 = body0->GetVelocityAtPoint(contactPoint.m_point);
		ndVector v1 = body1->GetVelocityAtPoint(contactPoint.m_point);

		// Relative velocity along the normal
		ndVector vRel = v1 - v0;
		ndFloat32 speed = vRel.DotProduct(contactPoint.m_normal).GetScalar();

		return static_cast<Ogre::Real>(speed);
	}

	void Contact::getPositionAndNormal(Ogre::Vector3& pos, Ogre::Vector3& norm, OgreNewt::Body* body) const
	{
		if (!m_contactNode)
		{
			pos = Ogre::Vector3::ZERO;
			norm = Ogre::Vector3::ZERO;
			return;
		}

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		pos = Ogre::Vector3(contactPoint.m_point.m_x, contactPoint.m_point.m_y, contactPoint.m_point.m_z);

		norm = Ogre::Vector3(contactPoint.m_normal.m_x, contactPoint.m_normal.m_y, contactPoint.m_normal.m_z);
	}

	Ogre::Real Contact::getTangentSpeed(int index) const
	{
		if (!m_contactNode)
			return 0.0f;

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		if (index == 0)
			return static_cast<Ogre::Real>(contactPoint.m_dir0_Force.m_force);
		else if (index == 1)
			return static_cast<Ogre::Real>(contactPoint.m_dir1_Force.m_force);

		return 0.0f;
	}

	void Contact::setSoftness(Ogre::Real softness)
	{
		if (!m_contactNode)
			return;

		ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		contactPoint.m_material.m_softness = softness;
	}

	void Contact::setElasticity(Ogre::Real elasticity)
	{
		if (!m_contactNode)
			return;

		ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		contactPoint.m_material.m_restitution = static_cast<ndFloat32>(elasticity);
	}

	void Contact::setFrictionState(int state, int index)
	{
		if (!m_contactNode)
			return;

		ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		if (state == 0)
		{
			// Disable friction for the selected tangent index
			if (index == 0)
			{
				contactPoint.m_material.m_staticFriction0 = 0.0f;
				contactPoint.m_material.m_dynamicFriction0 = 0.0f;
			}
			else if (index == 1)
			{
				contactPoint.m_material.m_staticFriction1 = 0.0f;
				contactPoint.m_material.m_dynamicFriction1 = 0.0f;
			}
		}
		// Else leave existing friction values intact (or set defaults if you want)
	}

	void Contact::setFrictionCoef(Ogre::Real stat, Ogre::Real kinetic, int index)
	{
		if (!m_contactNode)
			return;

		ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		if (index == 0)
		{
			contactPoint.m_material.m_staticFriction0 = static_cast<ndFloat32>(stat);
			contactPoint.m_material.m_dynamicFriction0 = static_cast<ndFloat32>(kinetic);
		}
		else if (index == 1)
		{
			contactPoint.m_material.m_staticFriction1 = static_cast<ndFloat32>(stat);
			contactPoint.m_material.m_dynamicFriction1 = static_cast<ndFloat32>(kinetic);
		}
	}

	void Contact::setTangentAcceleration(Ogre::Real accel, int index)
	{
		if (!m_contactNode)
			return;

		ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		if (index == 0)
		{
			contactPoint.m_dir0_Force.m_force = static_cast<ndFloat32>(accel);
			contactPoint.OverrideFriction0Accel(static_cast<ndFloat32>(accel));
		}
		else if (index == 1)
		{
			contactPoint.m_dir1_Force.m_force = static_cast<ndFloat32>(accel);
			contactPoint.OverrideFriction1Accel(static_cast<ndFloat32>(accel));
		}
	}

	void Contact::setNormalDirection(const Ogre::Vector3& dir)
	{
		if (!m_contactNode)
			return;

		ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		Ogre::Vector3 normDir = dir;
		normDir.normalise();

		contactPoint.m_normal = ndVector(normDir.x, normDir.y, normDir.z, ndFloat32(0.0f));
	}

	void Contact::setNormalAcceleration(Ogre::Real accel)
	{
		if (!m_contactNode)
			return;

		ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		// Newton 4.0: normal acceleration can be stored as a force placeholder
		contactPoint.m_normal_Force.m_force = static_cast<ndFloat32>(accel);
	}

	void Contact::setRotateTangentDirections(const Ogre::Vector3& dir)
	{
		if (!m_contactNode)
			return;

		// Tangent directions are computed from the normal automatically
		// You could store a preferred tangent here if needed (advanced)
	}

	Ogre::Real Contact::getContactForce(OgreNewt::Body* /*body*/) const
	{
		if (!m_contactNode)
			return 0.0f;

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		return static_cast<Ogre::Real>(contactPoint.m_normal_Force.m_force);
	}

	Ogre::Real Contact::getContactMaxNormalImpact(void) const
	{
		return getContactForce(nullptr);
	}

	void Contact::setContactTangentFriction(Ogre::Real friction, int index) const
	{
		if (!m_contactNode)
			return;

		ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		if (index == 0)
		{
			contactPoint.m_material.m_staticFriction0 = static_cast<ndFloat32>(friction);
			contactPoint.m_material.m_dynamicFriction0 = static_cast<ndFloat32>(friction);
		}
		else if (index == 1)
		{
			contactPoint.m_material.m_staticFriction1 = static_cast<ndFloat32>(friction);
			contactPoint.m_material.m_dynamicFriction1 = static_cast<ndFloat32>(friction);
		}
	}

	Ogre::Real Contact::getContactMaxTangentImpact(int index) const
	{
		if (!m_contactNode)
			return 0.0f;

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		if (index == 0)
			return static_cast<Ogre::Real>(contactPoint.m_material.m_staticFriction0 * contactPoint.m_normal_Force.m_force);
		else if (index == 1)
			return static_cast<Ogre::Real>(contactPoint.m_material.m_staticFriction1 * contactPoint.m_normal_Force.m_force);

		return 0.0f;
	}

	void Contact::setContactPruningTolerance(OgreNewt::Body* /*body0*/, OgreNewt::Body* /*body1*/, Ogre::Real /*tolerance*/) const
	{
		// Newton 4.0 handles contact pruning internally
	}

	Ogre::Real Contact::getContactPruningTolerance(OgreNewt::Body* /*body0*/, OgreNewt::Body* /*body1*/) const
	{
		return 0.0f;
	}

	Ogre::Real Contact::getContactPenetration(void) const
	{
		if (!m_contactNode)
			return 0.0f;

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		return static_cast<Ogre::Real>(contactPoint.m_penetration);
	}

	void Contact::getContactTangentDirections(OgreNewt::Body* /*body*/, Ogre::Vector3& dir1, Ogre::Vector3& dir2) const
	{
		if (!m_contactNode)
		{
			dir1 = Ogre::Vector3::ZERO;
			dir2 = Ogre::Vector3::ZERO;
			return;
		}

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		ndVector normal(contactPoint.m_normal);
		normal.m_w = ndFloat32(0.0f);

		ndVector tangent0, tangent1;
		if (ndAbs(normal.m_y) > ndFloat32(0.577f))
			tangent0 = ndVector(-normal.m_z, ndFloat32(0.0f), normal.m_x, ndFloat32(0.0f));
		else
			tangent0 = ndVector(-normal.m_y, normal.m_x, ndFloat32(0.0f), ndFloat32(0.0f));

		tangent0 = tangent0.Normalize();
		tangent1 = normal.CrossProduct(tangent0);

		dir1 = Ogre::Vector3(tangent0.m_x, tangent0.m_y, tangent0.m_z);
		dir2 = Ogre::Vector3(tangent1.m_x, tangent1.m_y, tangent1.m_z);
	}

	Ogre::Real Contact::getClosestDistance() const
	{
		if (!m_contactNode)
			return 0.0f;

		const ndContactMaterial& contactPoint = m_contactNode->GetInfo();

		// TODO: is this correct? Debug it!
		Ogre::Real dist = static_cast<Ogre::Real>(-contactPoint.m_penetration);

		if (dist < 0.0f)
			dist = 0.0f;

		return dist;
	}

	void Contact::setSkinMargin(float margin)
	{
		if (!m_contactNode) return;
		ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		contactPoint.m_material.m_skinMargin = static_cast<ndFloat32>(margin);
	}

	void Contact::setCollidable(bool enable)
	{
		if (!m_contactNode) return;
		ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		if (enable)
			contactPoint.m_material.m_flags |= ndContactOptions::m_collisionEnable;
		else
			contactPoint.m_material.m_flags &= ~ndContactOptions::m_collisionEnable;
	}

	void Contact::setFriction0Enabled(bool enable)
	{
		if (!m_contactNode) return;
		ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		if (enable)
			contactPoint.m_material.m_flags |= ndContactOptions::m_friction0Enable;
		else
			contactPoint.m_material.m_flags &= ~ndContactOptions::m_friction0Enable;
	}

	void Contact::setFriction1Enabled(bool enable)
	{
		if (!m_contactNode) return;
		ndContactMaterial& contactPoint = m_contactNode->GetInfo();
		if (enable)
			contactPoint.m_material.m_flags |= ndContactOptions::m_friction1Enable;
		else
			contactPoint.m_material.m_flags &= ~ndContactOptions::m_friction1Enable;
	}

	void Contact::remove()
	{
		if (!m_contactNode || !m_parent || !m_parent->_getNewtonContact())
			return;

		ndContact* contact = m_parent->_getNewtonContact();
		ndContactPointList& points = contact->GetContactPoints();

		// Remove this contact node
		points.Remove(m_contactNode);

		m_contactNode = nullptr; // mark as removed
		m_parent = nullptr;
	}

	ndContactPoint* Contact::_getNewtonContactPoint()
	{
		if (!m_contactNode)
			return nullptr;

		return &m_contactNode->GetInfo();
	}

	Ogre::String Contact::print(void)
	{
		if (!m_contactNode)
			return "Invalid Contact";

		Ogre::Vector3 position = Ogre::Vector3::ZERO;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		this->getPositionAndNormal(position, normal, this->getBody0());

		Ogre::Vector3 direction1 = Ogre::Vector3::ZERO;
		Ogre::Vector3 direction2 = Ogre::Vector3::ZERO;

		this->getContactTangentDirections(this->getBody0(), direction1, direction2);

		Ogre::String message;
		message += "getNormalSpeed: " + Ogre::StringConverter::toString(this->getNormalSpeed()) + " \n";
		message += "getContactPosition: " + Ogre::StringConverter::toString(position) + " \n";
		message += "getContactNormal: " + Ogre::StringConverter::toString(normal) + " \n";
		message += "getTangentDirection1: " + Ogre::StringConverter::toString(direction1) + " \n";
		message += "getTangentDirection2: " + Ogre::StringConverter::toString(direction2) + " \n";
		message += "getTangentSpeed(0): " + Ogre::StringConverter::toString(this->getTangentSpeed(0)) + " \n";
		message += "getTangentSpeed(1): " + Ogre::StringConverter::toString(this->getTangentSpeed(1)) + " \n";
		message += "getContactMaxNormalImpact: " + Ogre::StringConverter::toString(this->getContactMaxNormalImpact()) + " \n";
		message += "getContactMaxTangentImpact(0): " + Ogre::StringConverter::toString(this->getContactMaxTangentImpact(0)) + " \n";
		message += "getContactPenetration: " + Ogre::StringConverter::toString(this->getContactPenetration()) + " \n";
		message += "getClosestDistance: " + Ogre::StringConverter::toString(this->getClosestDistance()) + " \n";
		message += "getFaceAttribute: " + Ogre::StringConverter::toString(this->getFaceAttribute()) + " \n";

		return message;
	}
}