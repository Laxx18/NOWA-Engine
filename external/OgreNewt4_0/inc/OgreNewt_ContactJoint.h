/*
	OgreNewt Library

	Ogre implementation of Newton Game Dynamics SDK

	OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

		by melven
		some changes by melven
		adapted for Newton 4.0

*/
#ifndef _INCLUDE_OGRENEWT_CONTACTJOINT
#define _INCLUDE_OGRENEWT_CONTACTJOINT

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "ndContact.h"
#include "ndContactNotify.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{
	// Plain data snapshot — safe to copy across thread boundaries
	struct ContactSnapshot
	{
		Ogre::Vector3 position;
		Ogre::Vector3 normal;
		Ogre::Vector3 tangentDir0;
		Ogre::Vector3 tangentDir1;
		Ogre::Real normalSpeed = 0.0f;
		Ogre::Real normalImpact = 0.0f;
		Ogre::Real tangentSpeed0 = 0.0f;
		Ogre::Real tangentSpeed1 = 0.0f;
		Ogre::Real penetration = 0.0f;
		Ogre::Real closestDistance = 0.0f;
		int        faceAttribute = 0;
	};

	//! with this class you can iterate through all contacts
	/*!
		this class must not be inherited or instantiated directly in any way,
		it is passed as argument in the contactsProcess-function of the ContactCallback!
		for iterating through all contacts you can do something like this:
		for(Contact contact = contactJoint.getFirstContact(); contact; contact = contact.getNext())
			...
	*/
	class _OgreNewtExport ContactJoint
	{
	public:
		//! constructor
		ContactJoint(ndContact* contact);

		//! destructor
		~ContactJoint();

		//! return the number of contacts
		int getContactCount() const;

		//! get the first contact
		Contact getFirstContact();

		//! get the first body
		OgreNewt::Body* getBody0();

		//! get the second body
		OgreNewt::Body* getBody1();

		//! get the newton Contact pointer
		ndContact* _getNewtonContact();

		//! get the MaterialPair (deprecated in Newton 4.0, returns nullptr)
		/*!
			Newton 4.0 doesn't use material pairs anymore
		*/
		MaterialPair* getMaterialPair();

	protected:
		ndContact* m_contact;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//! with the methods from this class you can set the behavior of each contact-point
	/*!
		this class is for creating custom behavior between materials.
		this class must not be inherited or instantiated directly in any way,
		you can obtain an object of this class in the contact-callback
		from the contactJoint and iterate through all contacts using the
		getNext-function
	*/
	class _OgreNewtExport Contact
	{
	public:

		//! constructor
		Contact(ndContactPointList::ndNode* contactNode, ContactJoint* parent);

		//! return true, if this is a valid contact
		operator bool() const;

		//! get the next contact from the parent contact-joint
		Contact getNext() const;

		//! get whether this contact is empty (last one)
		bool isEmpty() const;

		// basic contact control commands... they can only be used, if this is a valid contact ( !contact == false )

		//! get the first body
		OgreNewt::Body* getBody0() const;

		//! get the second body
		OgreNewt::Body* getBody1() const;

		//! get the face ID of a TreeCollision object
		unsigned getFaceAttribute() const;

		//! speed of the collision (normal speed)
		Ogre::Real getNormalSpeed() const;

		//! force of the collision
		/*!
			only valid for objects in a stable state (sitting on top of each other, etc)
		*/
		Ogre::Vector3 getForce() const;

		//! get position and normal of the collision
		void getPositionAndNormal(Ogre::Vector3& pos, Ogre::Vector3& norm, OgreNewt::Body* body) const;

		//! get tangent speed of the collision
		Ogre::Real getTangentSpeed(int index) const;

		//! set softness of the current contact
		void setSoftness(Ogre::Real softness);

		//! set elasticity of the current contact
		void setElasticity(Ogre::Real elasticity);

		//! Enables or disables friction calculation for this contact.
		void setFrictionState(int state, int index);

		//! set static friction for current contact
		void setFrictionCoef(Ogre::Real stat, Ogre::Real kinetic, int index);

		//! set tangent acceleration for contact
		void setTangentAcceleration(Ogre::Real accel, int index);

		//! manually set the normal for the collision.
		void setNormalDirection(const Ogre::Vector3& dir);

		//! manually set the acceleration along the collision normal.
		void setNormalAcceleration(Ogre::Real accel);

		//! Rotate the tangent direction of the contacts
		void setRotateTangentDirections(const Ogre::Vector3& dir);

		//! Get the contact force
		Ogre::Real getContactForce(OgreNewt::Body* body) const;

		//! Get the maximum normal impact
		Ogre::Real getContactMaxNormalImpact(void) const;

		//! Set tangent friction
		void setContactTangentFriction(Ogre::Real friction, int index) const;

		//! Get the maximum tangent impact
		Ogre::Real getContactMaxTangentImpact(int index) const;

		//! Get contact penetration depth
		Ogre::Real getContactPenetration(void) const;

		//! Get the contact tangent vectors to the contact point.
		void getContactTangentDirections(OgreNewt::Body* body, Ogre::Vector3& dir1, Ogre::Vector3& dir2) const;

		//! Get closest distance between bodies
		Ogre::Real getClosestDistance(void) const;

		void setSkinMargin(float margin);

		void setCollidable(bool enable);

		void setFriction0Enabled(bool enable);

		void setFriction1Enabled(bool enable);

		//! removes the contact from the parent contact-joint
		void remove();

		//! get the ndContactPoint from this contact
		ndContactPoint* _getNewtonContactPoint();

		//! Print contact information for debugging
		Ogre::String print(void);

		//! Snapshot all contact data into a plain struct — safe to use after Newton recycles this contact
		ContactSnapshot createSnapshot() const;

	protected:
		ndContactPointList::ndNode* m_contactNode;

		OgreNewt::ContactJoint* m_parent;
		int m_index;
	};

}   // end NAMESPACE OgreNewt

#endif
// _INCLUDE_OGRENEWT_CONTACTJOINT