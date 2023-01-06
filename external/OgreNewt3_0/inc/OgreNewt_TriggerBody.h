/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.


please note that the "boost" library files included here are not of my creation, refer to those files and boost.org for information.


by Lax

*/

#ifndef _INCLUDE_OGRENEWT_TRIGGER_BODY
#define _INCLUDE_OGRENEWT_TRIGGER_BODY

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{

	/*
	CLASS DEFINITION:

	Body

	USE:
	this class represents a Newton trigger, that can use a body but has the ability to let the developer trigger when another object will enter this one
	*/
	//!
	class _OgreNewtExport TriggerBody : public Body
	{
	public:
		//! constructor.
		/*!
		creates a trigger in an OgreNewt::World, based on a specific collision shape in order to be able to react when another object enters, is inside or leaves this one
		\param body pointer to the OgreNewt::Body for trigger collision
		\param sceneManager pointer to for debug collisions
		\param triggerCallback pointer This pointer must be created on heap and not deleted, it will be deleted automatically
		\note Internally this body will be deleted and a kinematic body created and delivered
		*/
		TriggerBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, TriggerCallback* triggerCallback, 
		Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC);

		//! destructor
		virtual ~TriggerBody();

		void reCreateTrigger(const OgreNewt::CollisionPtr& col);

		TriggerCallback* getTriggerCallback(void) const;

		/// Integrates the velocity for a kinematic body. Note: This function should be called in an update loop, if its desired that a kinematic body should be moved via velocity or rotated via omega
		void integrateVelocity(Ogre::Real dt);
	private:
		dCustomTriggerController* m_triggerController;
		TriggerCallback* m_triggerCallback;
	};

};

#endif
