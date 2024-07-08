/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.


please note that the "boost" library files included here are not of my creation, refer to those files and boost.org for information.


by Walaber
some changes by melven

*/

#ifndef _INCLUDE_OGRENEWT_KINEMATIC_BODY
#define _INCLUDE_OGRENEWT_KINEMATIC_BODY

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{
	/*
	CLASS DEFINITION:

	Body

	USE:
	this class represents a Newton kinematic rigid body, that is similiar to a classic body but does not react on collisions
	*/
	//! main class for all Rigid Bodies in the system.
	class _OgreNewtExport KinematicBody : public Body
	{
	public:
		KinematicBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC);

		/// Integrates the velocity for a kinematic body. Note: This function should be called in an update loop, if its desired that a kinematic body should be moved via velocity or rotated via omega
		void integrateVelocity(Ogre::Real dt);

		//! destructor
		virtual ~KinematicBody();

		typedef OgreNewt::function<void(OgreNewt::Body*)> KinematicContactCallback;

		void setKinematicContactCallback(KinematicContactCallback callback);

		template<class c>
		void setKinematicContactCallback(OgreNewt::function<void(c*, OgreNewt::Body*)> callback, c* instancedClassPointer)
		{
			setKinematicContactCallback(OgreNewt::bind(callback, instancedClassPointer, std::placeholders::_1));
		}

		void removeKinematicContactCallback() { m_kinematicContactCallback = nullptr; }
	protected:
		KinematicContactCallback m_kinematicContactCallback;
	};

};

#endif

