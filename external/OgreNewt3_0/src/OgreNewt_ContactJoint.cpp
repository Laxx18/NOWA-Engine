#include "OgreNewt_Stdafx.h"
#include "OgreNewt_ContactJoint.h"
#include "OgreNewt_World.h"

namespace OgreNewt
{
	ContactJoint::ContactJoint(const NewtonJoint* contactJoint)
		: m_contactJoint(contactJoint)
	{
	
	}


	ContactJoint::~ContactJoint()
	{
	}


	Contact ContactJoint::getFirstContact()
	{
		Contact contact(NewtonContactJointGetFirstContact(m_contactJoint), this);
		return contact;
	}

	int ContactJoint::getContactCount() const
	{
		return NewtonContactJointGetContactCount(m_contactJoint);
	}

	OgreNewt::Body* ContactJoint::getBody0()
	{
		return (OgreNewt::Body*) NewtonBodyGetUserData(NewtonJointGetBody0(m_contactJoint));
	}

	OgreNewt::Body* ContactJoint::getBody1()
	{
		return (OgreNewt::Body*) NewtonBodyGetUserData(NewtonJointGetBody1(m_contactJoint));
	}

	const NewtonJoint* ContactJoint::_getNewtonContactJoint()
	{
		return m_contactJoint;
	}

	MaterialPair* ContactJoint::getMaterialPair()
	{
		Body* body0 = getBody0();
		Body* body1 = getBody1();
		if (nullptr == body0 || nullptr == body1)
			return nullptr;

		const World* world = body0->getWorld();

		return (MaterialPair*)NewtonMaterialGetUserData(world->getNewtonWorld(), body0->getMaterialGroupID()->getID(), body1->getMaterialGroupID()->getID());
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Contact::Contact(void* contact, ContactJoint* parent)
		: m_parent(parent),
		m_contact(contact),
		m_material(nullptr)
	{
		if (nullptr != contact)
		{
			m_material = NewtonContactGetMaterial(contact);
		}
	}

	Contact Contact::getNext() const
	{
		Contact contact(NewtonContactJointGetNextContact(m_parent->_getNewtonContactJoint(), m_contact), m_parent);
		return contact;
	}

	bool Contact::isEmpty() const
	{
		return m_material == nullptr;
	}

	Ogre::Vector3 Contact::getForce() const
	{
		Ogre::Vector3 force;
		NewtonMaterialGetContactForce(m_material, getBody0()->getNewtonBody(), &force.x);
		return force;
	}

	Contact::operator bool() const
	{
		return m_contact != nullptr;
	}

	OgreNewt::Body* Contact::getBody0() const
	{
		return static_cast<OgreNewt::Body*>(NewtonBodyGetUserData(NewtonJointGetBody0(m_parent->_getNewtonContactJoint())));
	}

	OgreNewt::Body* Contact::getBody1() const
	{
		return static_cast<OgreNewt::Body*>(NewtonBodyGetUserData(NewtonJointGetBody1(m_parent->_getNewtonContactJoint())));
	}

	unsigned Contact::getFaceAttribute() const
	{
		return NewtonMaterialGetContactFaceAttribute(m_material);
	}

//	unsigned Contact::getBodyCollisionID(OgreNewt::Body* body) const
//	{
//		// FIXME: NewtonMaterialGetBodyCollisionID is no more valid in newton version 2.36!!!
////         return NewtonMaterialGetBodyCollisionID( m_material, body->getNewtonBody() );
//		return 0;
//	}

	Ogre::Real Contact::getNormalSpeed() const
	{
		return static_cast<Ogre::Real>(NewtonMaterialGetContactNormalSpeed(m_material));
	}

	void Contact::getPositionAndNormal(Ogre::Vector3& pos, Ogre::Vector3& norm, OgreNewt::Body*body) const
	{
		NewtonMaterialGetContactPositionAndNormal(m_material, body->getNewtonBody(), &pos.x, &norm.x);
	}

	Ogre::Real Contact::getTangentSpeed(int index) const
	{
		return static_cast<Ogre::Real>(NewtonMaterialGetContactTangentSpeed(m_material, index));
	}

	void Contact::setSoftness(Ogre::Real softness)
	{
		NewtonMaterialSetContactSoftness(m_material, softness);
	}

	void Contact::setElasticity(Ogre::Real elasticity)
	{
		NewtonMaterialSetContactElasticity(m_material, elasticity);
	}

	void Contact::setFrictionState(int state, int index)
	{
		NewtonMaterialSetContactFrictionState(m_material, state, index);
	}

	void Contact::setFrictionCoef(Ogre::Real stat, Ogre::Real kinetic, int index)
	{
		NewtonMaterialSetContactFrictionCoef(m_material, stat, kinetic, index);
	}

	void Contact::setTangentAcceleration(Ogre::Real accel, int index)
	{
		NewtonMaterialSetContactTangentAcceleration(m_material, accel, index);
	}

	void Contact::setNormalDirection(const Ogre::Vector3& dir)
	{
		NewtonMaterialSetContactNormalDirection(m_material, &dir.x);
	}

	void Contact::setNormalAcceleration(Ogre::Real accel)
	{
		NewtonMaterialSetContactNormalAcceleration(m_material, accel);
	}

	void Contact::setRotateTangentDirections(const Ogre::Vector3& dir)
	{
		NewtonMaterialContactRotateTangentDirections(m_material, &dir.x);
	}

	Ogre::Real Contact::getContactForce(OgreNewt::Body* body) const
	{
		Ogre::Real resultForce = 0.0f;
		NewtonMaterialGetContactForce(m_material, body->getNewtonBody(), &resultForce);
		return resultForce;
	}

	Ogre::Real Contact::getContactMaxNormalImpact(void) const
	{
		return NewtonMaterialGetContactMaxNormalImpact(m_material);
	}

	void Contact::setContactTangentFriction(Ogre::Real friction, int index) const
	{
		NewtonMaterialSetContactTangentFriction(m_material, friction, index);
	}

	Ogre::Real Contact::getContactMaxTangentImpact(int index) const
	{
		return NewtonMaterialGetContactMaxTangentImpact(m_material, index);
	}

	void Contact::setContactPruningTolerance(OgreNewt::Body* body0, OgreNewt::Body* body1, Ogre::Real tolerance) const
	{
		return NewtonMaterialSetContactPruningTolerance(m_parent->_getNewtonContactJoint(), tolerance);
	}

	Ogre::Real Contact::getContactPruningTolerance(OgreNewt::Body* body0, OgreNewt::Body* body1) const
	{
		return NewtonMaterialGetContactPruningTolerance(m_parent->_getNewtonContactJoint());
	}

	Ogre::Real Contact::getContactPenetration(void) const
	{
		return NewtonMaterialGetContactPenetration(m_material);
	}

	void Contact::getContactTangentDirections(OgreNewt::Body* body, Ogre::Vector3& dir1, Ogre::Vector3& dir2) const
	{
		NewtonMaterialGetContactTangentDirections(m_material, body->getNewtonBody(), &dir1.x, &dir2.x);
	}

	Ogre::Real Contact::getClosestDistance(void) const
	{
		return NewtonContactJointGetClosestDistance(m_parent->_getNewtonContactJoint());
	}

	void Contact::remove()
	{
		NewtonContactJointRemoveContact(m_parent->_getNewtonContactJoint(), m_contact);
		m_parent = nullptr;
		m_material = nullptr;
		m_contact = nullptr;
	}

	NewtonMaterial* Contact::_getNewtonMaterial()
	{
		return m_material;
	}

	Ogre::String Contact::print(void)
	{
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
		message += "getTangentSpeed: " + Ogre::StringConverter::toString(this->getTangentSpeed(0)) + " \n";
		// message += "getContactForce: " + Ogre::StringConverter::toString(this->getContactForce(this->getBody0())) + " \n";
		message += "getContactMaxNormalImpact: " + Ogre::StringConverter::toString(this->getContactMaxNormalImpact()) + " \n";
		message += "getContactMaxTangentImpact: " + Ogre::StringConverter::toString(this->getContactMaxTangentImpact(0)) + " \n";
		message += "getContactPruningTolerance: " + Ogre::StringConverter::toString(this->getContactPruningTolerance(this->getBody0(), this->getBody1())) + " \n";
		message += "getContactPenetration: " + Ogre::StringConverter::toString(this->getContactPenetration()) + " \n";
		message += "getClosestDistance: " + Ogre::StringConverter::toString(this->getClosestDistance()) + " \n";
		message += "getFaceAttribute: " + Ogre::StringConverter::toString(this->getFaceAttribute()) + " \n";

		return message;
	}
}

