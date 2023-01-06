#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_RayCast.h"
#include "OgreNewt_Tools.h"
// #include "OgreNewt_Debugger.h"
#include "OgreNewt_Collision.h"


namespace OgreNewt
{
	Raycast::Raycast()
	{
	}

	Raycast::~Raycast()
	{
		
	}

	void Raycast::go(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, int threadIndex)
	{
		/* if( world->getDebugger().isRaycastRecording() )
		{
		world->getDebugger().addRay(startpt, endpt);
		}*/

		mLastBody = nullptr;

		// perform the raycast!
		NewtonWorldRayCast(world->getNewtonWorld(), (dFloat *)&startpt, (dFloat *)&endpt, OgreNewt::Raycast::newtonRaycastFilter, this, OgreNewt::Raycast::newtonRaycastPreFilter, threadIndex);

		mLastBody = nullptr;

	}

	dFloat _CDECL Raycast::newtonRaycastFilter(const NewtonBody* const body, const NewtonCollision* const shapeHit, const dFloat* const hitContact, const dFloat* const hitNormal, dLong collisionID, void* const userData, dFloat intersectParam)
	{
		// get our object!
		Raycast* me = (Raycast*)userData;

		Body* bod = (Body*)NewtonBodyGetUserData(body);
		const World* world = bod->getWorld();
		Ogre::Vector3 normal = Ogre::Vector3(hitNormal[0], hitNormal[1], hitNormal[2]);

		if (me->mBodyAdded)
		{
			return intersectParam;
		}

		/* if( world->getDebugger().isRaycastRecording() && world->getDebugger().isRaycastRecordingHitBodies() )
		{
		world->getDebugger().addHitBody(bod);
		}*/

		// collision is no longer required
		if (me->userCallback(bod, nullptr, intersectParam, normal, collisionID))
		{
			return intersectParam;
		}
		else
		{
			return 1.1f;
		}
	}

	unsigned _CDECL Raycast::newtonRaycastPreFilter(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData)
	{
		// get our object!
		Raycast* me = (Raycast*)userData;
		Body* bod = (Body*)NewtonBodyGetUserData(body);
		if (nullptr == bod)
		{
			return 0;
		}

		const World* world = bod->getWorld();

		// User must take care of collision destruction, because here it would be destroyed immediately
		// OgreNewt::CollisionPtr col = CollisionPtr(new Collision(collision, world, false));

		me->mBodyAdded = false;
		me->mLastBody = bod;

		if (me->userPreFilterCallback(bod))
			return 1;
		else
		{
// Attention: Untested!
			if( world->getDebugger().isRaycastRecording() && world->getDebugger().isRaycastRecordingHitBodies() )
			{
				world->getDebugger().addDiscardedBody(bod);
			}
			return 0;
		}
	}



	//--------------------------------
	BasicRaycast::BasicRaycastInfo::BasicRaycastInfo()
		: mBody(nullptr),
		mDistance(-1.0f),
		mNormal(Ogre::Vector3::ZERO)
	{

	}

	BasicRaycast::BasicRaycastInfo::~BasicRaycastInfo()
	{
	}


	BasicRaycast::BasicRaycast()
	{
	}

	BasicRaycast::BasicRaycast(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted)
		: Raycast()
	{
		go(world, startpt, endpt, sorted);
	}


	void BasicRaycast::go(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted)
	{
		// clean the list each time this function is called this way it can be re used for multiple casting. 
		mRayList.clear();
		Raycast::go(world, startpt, endpt);
		if (sorted)
		{
			std::sort(mRayList.begin(), mRayList.end());
		}
	}

	BasicRaycast::~BasicRaycast()
	{
	}


	int BasicRaycast::getHitCount() const
	{
		return (int)mRayList.size();
	}

	BasicRaycast::BasicRaycastInfo BasicRaycast::getFirstHit() const
	{
		//return the closest hit...
		BasicRaycast::BasicRaycastInfo ret;

		Ogre::Real dist = Ogre::Math::POS_INFINITY;

		RaycastInfoList::const_iterator it;
		for (it = mRayList.begin(); it != mRayList.end(); it++)
		{
			if (it->mDistance < dist)
			{
				dist = it->mDistance;
				ret = (*it);
			}
		}


		return ret;
	}

	BasicRaycast::BasicRaycastInfo BasicRaycast::getInfoAt(unsigned int hitnum) const
	{
		if (hitnum < mRayList.size())
		{
			return mRayList.at(hitnum);
		}
		else
		{
			BasicRaycast::BasicRaycastInfo emptyInfo;

			return emptyInfo;
		}
	}

	bool BasicRaycast::userCallback(OgreNewt::Body* body, OgreNewt::CollisionPtr collision, Ogre::Real distance, const Ogre::Vector3& normal, dLong collisionID)
	{
		// create a new infor object.
		BasicRaycast::BasicRaycastInfo newinfo;

		newinfo.mBody = body;
		newinfo.mDistance = distance;
		newinfo.mNormal = normal;
		newinfo.mCollisionID = collisionID;

		mRayList.push_back(newinfo);

		return false;
	}

	HeightFieldRaycast::HeightFieldRaycast()
	{
	}
	HeightFieldRaycast::~HeightFieldRaycast()
	{
	}


	void HeightFieldRaycast::go(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt)
	{
		if (world->getDebugger().isRaycastRecording())
		{
			world->getDebugger().addRay(startpt, endpt);
		}


		m_heightfieldcollisioncallback_lastbody = NULL;

		// perform the raycast!
		NewtonWorldRayCast(world->getNewtonWorld(), (float*)&startpt, (float*)&endpt, OgreNewt::HeightFieldRaycast::newtonRaycastFilter, this, OgreNewt::HeightFieldRaycast::newtonRaycastPreFilter, 0);


		m_heightfieldcollisioncallback_lastbody = NULL;
	}

	float _CDECL HeightFieldRaycast::newtonRaycastFilter(const NewtonBody* const body, const NewtonCollision* const shapeHit, const dFloat* const hitContact, const dFloat* const hitNormal, dLong collisionID, void* const userData, dFloat intersectParam)
	{
		// get our object!
		HeightFieldRaycast* me = (HeightFieldRaycast*)userData;

		Body* bod = (Body*)NewtonBodyGetUserData(body);
		const World* world = bod->getWorld();
		Ogre::Vector3 normal = Ogre::Vector3(hitNormal[0], hitNormal[1], hitNormal[2]);


		if (me->m_heightfieldcollisioncallback_bodyalreadyadded)
			return intersectParam;




		if (world->getDebugger().isRaycastRecording() && world->getDebugger().isRaycastRecordingHitBodies())
		{
			world->getDebugger().addHitBody(bod);
		}


		if (me->userCallback(bod, intersectParam, normal, collisionID))
			return intersectParam;
		else
			return 1.1f;

	}

	unsigned _CDECL HeightFieldRaycast::newtonRaycastPreFilter(const NewtonBody *body, const NewtonCollision *collision, void* userData)
	{
		// get our object!
		HeightFieldRaycast* me = (HeightFieldRaycast*)userData;

		Body* bod = (Body*)NewtonBodyGetUserData(body);
		const World* world = bod->getWorld();



		me->m_heightfieldcollisioncallback_bodyalreadyadded = false;
		me->m_heightfieldcollisioncallback_lastbody = bod;

		if (me->userPreFilterCallback(bod))
			return 1;
		else
		{

			if (world->getDebugger().isRaycastRecording() && world->getDebugger().isRaycastRecordingHitBodies())
			{
				world->getDebugger().addDiscardedBody(bod);
			}

			return 0;
		}
	}

}   // end NAMESPACE OgreNewt

