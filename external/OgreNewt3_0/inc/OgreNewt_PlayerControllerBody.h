/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.


please note that the "boost" library files included here are not of my creation, refer to those files and boost.org for information.


by Walaber
some changes by melven

*/

#ifndef _INCLUDE_OGRENEWT_PLAYER_CONTROLLER_BODY
#define _INCLUDE_OGRENEWT_PLAYER_CONTROLLER_BODY

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_PlayerController.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{
	class PlayerCallback;

	/*
	CLASS DEFINITION:

	Body

	USE:
	this class represents a Newton player controller rigid body!
	*/
	//! main class for all Rigid Bodies in the system.
	class _OgreNewtExport PlayerControllerBody : public Body
	{
	public:
		PlayerControllerBody(World* world, Ogre::SceneManager* sceneManager, const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition, const Ogre::Vector3& direction, 
			Ogre::Real mass, Ogre::Real radius, Ogre::Real height, Ogre::Real stepHeight, unsigned int categoryId, PlayerCallback* playerCallback = nullptr);

		//! destructor
		virtual ~PlayerControllerBody();

		void move(Ogre::Real forwardSpeed, Ogre::Real sideSpeed, const Ogre::Radian& headingAngle);

		void setHeading(const Ogre::Radian& headingAngle);

		void stop(void);

		void toggleCrouch(void);

		void setVelocity(const Ogre::Vector3& velocity);

		Ogre::Vector3 getVelocity(void) const;

		void setFrame(const Ogre::Quaternion& frame);

		Ogre::Quaternion getFrame(void) const;

		void reCreatePlayer(const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition, const Ogre::Vector3& direction,
			Ogre::Real mass, Ogre::Real radius, Ogre::Real height, Ogre::Real stepHeight, unsigned int categoryId, PlayerCallback* playerCallback);

		void setDirection(const Ogre::Vector3& direction);

		Ogre::Vector3 getDirection(void) const;

		void setGravityDirection(const Ogre::Vector3& gravityDirection);

		void setMass(Ogre::Real mass);

		void setCollisionPositionOffset(const Ogre::Vector3& collisionPositionOffset);

		Ogre::Vector3 getCollisionPositionOffset(void) const;

		void setRadius(Ogre::Real radius);

		Ogre::Real getRadius(void) const;

		void setHeight(Ogre::Real height);

		Ogre::Real getheight(void) const;

		void setStepHeight(Ogre::Real stepHeight);

		Ogre::Real getStepHeight(void) const;

		Ogre::Vector3 getUpDirection(void) const;

		bool isInFreeFall(void) const;

		bool isOnFloor(void) const;

		bool isCrouched(void) const;

		void setWalkSpeed(Ogre::Real walkSpeed);

		Ogre::Real getWalkSpeed(void) const;

		void setJumpSpeed(Ogre::Real jumpSpeed);

		Ogre::Real getJumpSpeed(void) const;

		void setCanJump(bool canJump);

		bool getCanJump(void) const;

		Ogre::Real getForwardSpeed(void) const;

		Ogre::Real getSideSpeed(void) const;

		Ogre::Radian getHeading(void) const;

		void setStartOrientation(const Ogre::Quaternion& startOrientation);

		Ogre::Quaternion getStartOrientation(void) const;

		void setActive(bool active);

		bool isActive(void) const;

		PlayerCallback* getPlayerCallback(void) const;

	protected:
		World* world;
		BasicPlayerControllerManager* m_playerControllerManager;

		Ogre::Vector3 m_startPosition;
		Ogre::Quaternion m_startOrientation;
		Ogre::Vector3 m_oldStartPosition;
		Ogre::Quaternion m_oldStartOrientation;
		Ogre::Vector3 m_direction;
		Ogre::Vector3 m_gravityDirection;
		Ogre::Real m_mass;
		Ogre::Real m_radius;
		Ogre::Real m_height;
		Ogre::Vector3 m_collisionPositionOffset;

		Ogre::Real m_stepHeight;

		Ogre::Real m_forwardSpeed;
		Ogre::Real m_sideSpeed;
		Ogre::Radian m_heading;
		Ogre::Radian m_startHeading;
		Ogre::Real m_walkSpeed;
		Ogre::Real m_jumpSpeed;
		bool m_canJump;
		bool m_active;

		PlayerCallback* m_playerCallback;
	};

};

#endif

