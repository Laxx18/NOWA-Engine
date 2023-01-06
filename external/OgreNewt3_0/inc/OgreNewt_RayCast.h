/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

by Walaber
some changes by melven

*/

#ifndef _INCLUDE_OGRENEWT_RAYCAST
#define _INCLUDE_OGRENEWT_RAYCAST

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_Tools.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{

	//! general raycast
	/*!
	General class representing a raycast query in the Newton world.  this class should be inherited to create specific raycast behavior.
	*/
	class _OgreNewtExport Raycast
	{
	public:

		//! constructor
		Raycast();

		//! destuctor.
		virtual ~Raycast();

		//! performs the raycast.
		/*!
		call after creating the object.
		\param world pointer to the OgreNewt::World
		\param startpt starting point of ray in global space
		\param endpt ending point of ray in global space
		*/
		void go(const OgreNewt::World* world, const Ogre::Vector3& startpt,
			const Ogre::Vector3& endpt, int threadIndex = 0);

		//! user callback pre-filter function.
		/*!
		This function is an optional pre-filter to completely ignore specific bodies during the raycast.
		return false from this function to ignore this body, return true (default) to accept it.
		*/
		virtual bool userPreFilterCallback(OgreNewt::Body* body)
		{
			return true;
		}

		//! user callback filter function
		/*! user callback function.
		This function must be implemented by the user.
		Newton calls this function for each body intersected by the ray.  however it doesn't
		necessarily go perfect cloest-to-farthest order.
		return true and the callback will only be called for bodies closer to the start point than the current body.
		return false and the callback will call for any other bodies, even those farther than the current one.
		*/
		virtual bool userCallback(OgreNewt::Body* body,
			CollisionPtr collision, Ogre::Real distance,
			const Ogre::Vector3& normal, dLong collisionID) = 0;

		friend dFloat
			CollisionPrimitives::TreeCollision::newtonRayCastCallback(
				const NewtonBody* body, const NewtonCollision* col,
				dFloat distance, dFloat *normal, int faceId, void *userData);
	protected:
		//! save the last OgreNewt::Body from the newtonRaycastPreFilter to use this for example the TreeCollisionRayCallback
		OgreNewt::Body *mLastBody;

		//! save if this body was already added by RayCastCallback from TreeCollision
		bool mBodyAdded;

	private:

		//! callback used for running the raycast itself... used internally
		static dFloat _CDECL newtonRaycastFilter(const NewtonBody* const body, const NewtonCollision* const shapeHit, const dFloat* const hitContact, const dFloat* const hitNormal, dLong collisionID, void* const userData, dFloat intersectParam);

		//! callback used for running the raycast prefilder... used internally
		static unsigned _CDECL newtonRaycastPreFilter(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData);
	};

	//! Basic implementation of the raycast
	/*!
	This class is provided for general raycast use.  it returns information about all bodies hit by the ray.
	*/
	class _OgreNewtExport BasicRaycast : public Raycast
	{
	public:
		//! simple class that represents a single raycast rigid body intersection.
		class _OgreNewtExport BasicRaycastInfo
		{
		public:
			BasicRaycastInfo();
			~BasicRaycastInfo();

			Ogre::Real getDistance()
			{
				return mDistance;
			}
			
			OgreNewt::Body* getBody()
			{
				return mBody;
			}
			
			dLong getCollisionId()
			{
				return mCollisionID;
			}
			
			Ogre::Vector3 getNormal()
			{
				return mNormal;
			}
			
			Ogre::Quaternion getNormalOrientation()
			{
				return OgreNewt::Converters::grammSchmidt(mNormal);
			}

			Ogre::Real mDistance; //!< dist from point1 of the raycast, in range [0,1].
			OgreNewt::Body* mBody; //!< pointer to body intersected with
			dLong mCollisionID; //!< collision ID of the primitive hit by the ray (for compound collision bodies)
			Ogre::Vector3 mNormal; //!< normal of intersection.

			bool operator<(const BasicRaycastInfo& rhs) const
			{
				return mDistance < rhs.mDistance;
			}
		};

		//! empty constructor
		BasicRaycast();

		//! constructor
		/*!
		performs a raycast, then the results can be queried from the object after creation.
		\param world pointer to the OgreNewt::World
		\param startpt starting point of the ray in global space
		\param endpt ending point of the ray in global space
		\param sorted sort the results by distance
		*/
		BasicRaycast(const OgreNewt::World* world,
			const Ogre::Vector3& startpt, const Ogre::Vector3& endpt,
			bool sorted);

		//! perform a raycast
		/*!
		performs a raycast, then the results can be queried from the object after creation.
		\param world pointer to the OgreNewt::World
		\param startpt starting point of the ray in global space
		\param endpt ending point of the ray in global space
		\param sorted sort the results by distance
		*/
		void go(const OgreNewt::World* world, const Ogre::Vector3& startpt,
			const Ogre::Vector3& endpt, bool sorted);

		//! destuctor.
		~BasicRaycast();

		//! the all-important custom callback function.
		bool userCallback(Body* body, CollisionPtr col, Ogre::Real distance,
			const Ogre::Vector3& normal, dLong collisionID);

		// ------------------------------------------------------
		// the following functions can be used to retrieve information about the bodies collided by the ray.

		//! how many bodies did we hit?
		int getHitCount() const;

		//! retrieve the raycast info for a specific hit.
		BasicRaycastInfo getInfoAt(unsigned int hitnum) const;

		//! get the closest body hit by the ray.
		BasicRaycastInfo getFirstHit() const;

	private:

		// container for results.
		typedef std::vector<BasicRaycastInfo> RaycastInfoList;

		RaycastInfoList mRayList;
	};

	//! general raycast
	/*!
	General class representing a raycast query in the Newton world.  this class should be inherited to create specific raycast behavior.
	*/
	class _OgreNewtExport HeightFieldRaycast
	{
	public:

		//! constructor
		HeightFieldRaycast();

		//! destuctor.
		virtual ~HeightFieldRaycast();

		//! performs the raycast.  
		/*!
		call after creating the object.
		\param world pointer to the OgreNewt::World
		\param startpt starting point of ray in global space
		\param endpt ending point of ray in global space
		*/
		void go(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt);


		//! user callback pre-filter function.
		/*!
		This function is an optional pre-filter to completely ignore specific bodies during the raycast.
		return false from this function to ignore this body, return true (default) to accept it.
		*/
		virtual bool userPreFilterCallback(OgreNewt::Body* body)
		{
			return true;
		}

		//! user callback filter function
		/*! user callback function.
		This function must be implemented by the user.
		Newton calls this function for each body intersected by the ray.  however it doesn't
		necessarily go perfect cloest-to-farthest order.
		return true and the callback will only be called for bodies closer to the start point than the current body.
		return false and the callback will call for any other bodies, even those farther than the current one.
		*/
		virtual bool userCallback(OgreNewt::Body* body, Ogre::Real distance, const Ogre::Vector3& normal, dLong collisionID) = 0;

		friend float CollisionPrimitives::HeightField::newtonRayCastCallback(const NewtonBody* const body, const NewtonCollision* const heightFieldCollision, dFloat interception, int row, int col, dFloat* normal, int faceId, void* usedData);
	protected:

		//! save the last OgreNewt::Body from the newtonRaycastPreFilter to use this for example the HeightFieldRayCallback
		OgreNewt::Body *m_heightfieldcollisioncallback_lastbody;

		//! save if this body was already added by RayCastCallback from HeightFieldCollision
		bool m_heightfieldcollisioncallback_bodyalreadyadded;

	private:
		//! callback used for running the raycast itself... used internally
		static float _CDECL newtonRaycastFilter(const NewtonBody* const body, const NewtonCollision* const shapeHit, const dFloat* const hitContact, const dFloat* const hitNormal, dLong collisionID, void* const userData, dFloat intersectParam);

		//! callback used for running the raycast prefilder... used internally
		static unsigned _CDECL newtonRaycastPreFilter(const NewtonBody* body, const NewtonCollision* collision, void* userData);
	};


} // end NAMESPACE OgreNewt


#endif    // _INCLUDE_OGRENEWT_RAYCAST
