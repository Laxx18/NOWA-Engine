/*
	OgreNewt Library

	Ogre implementation of Newton Game Dynamics SDK

	OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

		by melven
		some changes by melven

*/
#ifndef _INCLUDE_OGRENEWT_CONTACTJOINT
#define _INCLUDE_OGRENEWT_CONTACTJOINT


#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{


	//! with this class you can iterate through all contacts
	/*!
		this class must not be inherited or instantiated directly in any way,
		it is passed as argument in the contacsProcess-function of the ContactCallback!
		for iterating through all contacts you can do something like this:
		for(Contact contact = contactJoint.getFirstContact(); contact; contact = contact.getNext())
			...
	*/
	class _OgreNewtExport ContactJoint
	{
	public:
		//! constructor
		ContactJoint(const NewtonJoint* contactJoint);

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

		//! get the newton ContactJoint
		const NewtonJoint* _getNewtonContactJoint();

		//! get the MaterialPair with the basis material of this contact-joint
		/*!
			you shouldn't need to call this function, it's only used internally
		*/
		MaterialPair* getMaterialPair();

	protected:
		const NewtonJoint* m_contactJoint;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//! with the methods from this class you can set the behavior of each contac-point
	/*!
		this class is for creating custom behavior between material GroupIDs.
		this class must not be inherited or instantiated directly in any way,
		you can obtain an object of this class in the contact-callback
		from the contactJoint and iterate through all contacts using the
		getNext-function
	*/
	class _OgreNewtExport Contact
	{
	public:

		//! constructor
		Contact(void *contact, ContactJoint* parent);

		//! return true, if this is not a valid contact (end of the contact-list!)
		operator bool() const;

		//! get the next contact from the parent contact-joint
		Contact getNext() const;

		//! get whether this contact is empty (last one) and has not material
		bool isEmpty() const;

		// basic contact control commands... they can only be used, if this is a valid contact ( !contact == false )

		//! get the first body
		OgreNewt::Body* getBody0() const;

		OgreNewt::Body* getBody1() const;

		//! get the face ID of a TreeCollision object
		unsigned getFaceAttribute() const;

		//! get the Collision ID of a body currently colliding
		// unsigned getBodyCollisionID(OgreNewt::Body* body) const;

		//! speed of the collision
		Ogre::Real getNormalSpeed() const;

		//! force of the collision
		/*!
			only valid for objects in a stable state (sitting on top of each other, etc)
		*/
		Ogre::Vector3 getForce() const;

		//! get positoin and normal of the collision
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
		//! This function changes the basis of the contact point to one where the contact normal is alinend to the new direction vector and the tangent direction are recalculated to be perpendicular to the new contact normal.
		//! In 99.9% of the cases the collision system can calculates a very good contact normal.
		//! However this algorithm that calculate the contact normal use as criteria the normal direction that will resolve the interpenetration with the least amount on motion.
		//! There are situations however when this solution is not the best.Take for example a rolling ball over a texelated floor, when the ball is over a flat polygon, 
		//! the contact normal is always perpendicular to the floor and pass by the origin of the sphere, however when the sphere is going across two adjacent polygons, 
		//! the contact normal is now perpendicular to the polygons edge and this does not guarantee they it will pass bay the origin of the sphere,
		//! but we know that the best normal is always the one passing by the origin of the sphere.
		void setNormalDirection(const Ogre::Vector3& dir);

		//! manually set the acceleration along the collision normal.
		void setNormalAcceleration(Ogre::Real accel);

		//! Rotate the tangent direction of the contacts until the primary direction is aligned with the alignVector.
		//! This function rotates the tangent vectors of the contact point until the primary tangent vector and the align vector are perpendicular 
		//! (ex. when the dot product between the primary tangent vector and the alignVector is 1.0). This function can be used in conjunction with 
		//! NewtonMaterialSetContactTangentAcceleration in order to create special effects. For example, conveyor belts, cheap low lod vehicles, slippery surfaces, etc.
		void setRotateTangentDirections(const Ogre::Vector3& dir);

		//! Get the contact force vector in global space.
		//! This means if two bodies collide with non zero relative velocity, the reaction force will be an impulse, which is not a reaction force, 
		//! this will return zero vector. this function will only return meaningful values when the colliding bodies are at rest.
		Ogre::Real getContactForce(OgreNewt::Body* body) const;

		Ogre::Real getContactMaxNormalImpact(void) const;

		void setContactTangentFriction(Ogre::Real friction, int index) const;

		Ogre::Real getContactMaxTangentImpact(int index) const;

		void setContactPruningTolerance(OgreNewt::Body* body0, OgreNewt::Body* body1, Ogre::Real tolerance) const;

		Ogre::Real getContactPruningTolerance(OgreNewt::Body* body0, OgreNewt::Body* body1) const;

		Ogre::Real getContactPenetration(void) const;

		//! Get the contact tangent vector to the contact point.
		void getContactTangentDirections(OgreNewt::Body* body, Ogre::Vector3& dir1, Ogre::Vector3& dir2) const;

		Ogre::Real getClosestDistance(void) const;

		//! removes the contact from the parent contact-joint, this means newton doesn't process this contact
		/*!
			use this function, when you want to ignore this specific contact, but you need to get the next contact before,
			because this will invalidate this class (set all pointers to zero!)
		*/
		void remove();

		//! get the NewtonMaterial from this callback.
		NewtonMaterial* _getNewtonMaterial();

		Ogre::String print(void);

	protected:
		NewtonMaterial* m_material;
		void* m_contact;
		OgreNewt::ContactJoint* m_parent;
	};

}   // end NAMESPACE OgreNewt

#endif
// _INCLUDE_OGRENEWT_CONTACTJOINT

