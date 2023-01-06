/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

by melven

*/
#ifndef _INCLUDE_OGRENEWT_PLAYERCONTROLLER
#define _INCLUDE_OGRENEWT_PLAYERCONTROLLER

#include "OgreNewt_Prerequisites.h"
#include "dPlayerController/dPlayerController.h"
#include "dVehicleManager.h"

class dPlayerControllerContactSolver;

namespace OgreNewt
{
	class PlayerControllerBody;

	class _OgreNewtExport PlayerCallback
	{
	public:
		PlayerCallback()
		{
		}

		virtual ~PlayerCallback()
		{
		}

		virtual Ogre::Real onContactFriction(const OgreNewt::PlayerControllerBody* visitor, const Ogre::Vector3& position, const Ogre::Vector3& normal, int contactId, const OgreNewt::Body* other)
		{
			return 2.0f;
		}

		virtual void onContact(const OgreNewt::PlayerControllerBody* visitor, const Ogre::Vector3& position, const Ogre::Vector3& normal, Ogre::Real penetration, int contactId, const OgreNewt::Body* other)
		{

		}
	};

	class BasicPlayerControllerManager;

	//! PlayerControllerManager
	/*!
	this class implements a player-controller based on the code of the CustomPlayerController-class in the Newton-CustomJoints library
	*/
	class _OgreNewtExport PlayerController : public dPlayerController
	{
	public:
		PlayerController(NewtonWorld* const world, const dMatrix& location, const dMatrix& localAxis, dFloat mass, dFloat radius, dFloat height, dFloat stepHeight, dFloat startHeading, PlayerCallback* playerCallback);

		virtual ~PlayerController();

		// Apply gravity 
		void ApplyMove(dFloat timestep);

	protected:

		virtual dFloat ContactFrictionCallback(const dVector& position, const dVector& normal, int contactId, const NewtonBody* const otherbody) const override;

		virtual void UpdatePlayerStatus(dPlayerControllerContactSolver& contactSolver) override;
	protected:
		PlayerCallback* playerCallback;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class BasicPlayerControllerManager : public dVehicleManager
	{
	public:
		BasicPlayerControllerManager(NewtonWorld* const world);
			
		virtual ~BasicPlayerControllerManager();

		virtual void ApplyInputs(dVehicle* const model, dFloat timestep) override;

		void forceUpdate(void);

		NewtonBody* CreatePlayerController(NewtonWorld* const world, const dMatrix& location, const dMatrix& localAxis, dFloat mass, dFloat radius, dFloat height, dFloat stepHeight, dFloat startHeading, PlayerCallback* playerCallback);

		PlayerController* getPlayerController(void) const;
	protected:
		PlayerController* playerController;
	};

}   // end NAMESPACE OgreNewt


#endif  // _INCLUDE_OGRENEWT_PLAYERCONTROLLER

