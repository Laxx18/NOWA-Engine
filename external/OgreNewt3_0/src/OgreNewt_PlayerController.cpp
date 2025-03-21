#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_PlayerController.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_PlayerControllerBody.h"
#include "dPlayerController/dPlayerControllerContactSolver.h"

namespace OgreNewt
{

	PlayerController::PlayerController(NewtonWorld* const world, const dMatrix& location, const dMatrix& localAxis, dFloat mass, dFloat radius, dFloat height, dFloat stepHeight, dFloat startHeading, PlayerCallback* playerCallback)
		: dPlayerController(world, location, localAxis, mass, radius, height, stepHeight),
		playerCallback(playerCallback),
		m_active(true)
	{

	}

	PlayerController::~PlayerController()
	{

	}

	void PlayerController::ApplyMove(dFloat timestep)
	{
		OgreNewt::PlayerControllerBody* const playerControllerBody = (OgreNewt::PlayerControllerBody*)NewtonBodyGetUserData(this->GetBody());
		Ogre::Vector3 bodyGravity = playerControllerBody->getGravity();

		const dVector gravity(GetLocalFrame().RotateVector(dVector(bodyGravity.y, 0.0f, 0.0f, 0.0f)));
		const dVector totalImpulse(GetImpulse() + gravity.Scale(GetMass() * timestep));
		SetImpulse(totalImpulse);
	}

	void PlayerController::setActive(bool active)
	{
		m_active = active;
	}

	dFloat PlayerController::ContactFrictionCallback(const dVector& position, const dVector& normal, int contactId, const NewtonBody* const otherbody) const
	{
		if (nullptr != this->playerCallback)
		{
			OgreNewt::PlayerControllerBody* const playerControllerBody = (OgreNewt::PlayerControllerBody*)NewtonBodyGetUserData(this->GetBody());

			const OgreNewt::Body* otherBody = (const OgreNewt::Body*) NewtonBodyGetUserData(otherbody);
			return this->playerCallback->onContactFriction(playerControllerBody, Ogre::Vector3(position.m_x, position.m_y, position.m_z), Ogre::Vector3(normal.m_x, normal.m_y, normal.m_z), contactId, otherBody);
		}
		else
		{
			return 2.0f;
		}
	}

	void PlayerController::UpdatePlayerStatus(dPlayerControllerContactSolver& contactSolver)
	{
		dPlayerController::UpdatePlayerStatus(contactSolver);

		if (nullptr != this->playerCallback)
		{
			OgreNewt::PlayerControllerBody* const playerControllerBody = (OgreNewt::PlayerControllerBody*)NewtonBodyGetUserData(this->GetBody());

			for (int i = 0; i < contactSolver.m_contactCount; i++)
			{
				NewtonWorldConvexCastReturnInfo& contact = contactSolver.m_contactBuffer[i];
				const NewtonBody* const hitBody = contact.m_hitBody;
				const OgreNewt::Body* otherBody = (const OgreNewt::Body*)NewtonBodyGetUserData(hitBody);

				const Ogre::Vector3 position = Ogre::Vector3(&contact.m_point[0]);
				const Ogre::Vector3 normal = Ogre::Vector3(&contact.m_normal[0]);

				this->playerCallback->onContact(playerControllerBody, position, normal, contact.m_penetration, contact.m_contactID, otherBody);
			}
		}
	}

	void PlayerController::PreUpdate(dFloat timestep)
	{
		if (m_active)
		{
			dPlayerController::PreUpdate(timestep);
		}
	}

	// a friction value of 10 will allows the player to climb an 85 dregrees slope. so any contact normal with the slightest deviation from being horizontal will generate some vertical motion. That's my theory but I will try to recrate it anyway.

	// I think the solution is to unconditionally or maybe optionl make contact on the wall of the cylinder friction less and only contact on the round patch been tractions contacts.
	// that seems should like an attractive proposition the only way to know if to implement and try.

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BasicPlayerControllerManager::BasicPlayerControllerManager(NewtonWorld* const world)
		: dVehicleManager(world),
		playerController(nullptr)
		
	{
		
	}

	BasicPlayerControllerManager::~BasicPlayerControllerManager()
	{

	}

	void BasicPlayerControllerManager::ApplyInputs(dVehicle* const model, dFloat timestep)
	{
		PlayerController* controller = (PlayerController*)model->GetAsPlayerController();
		dAssert(controller);
		controller->ApplyMove(timestep);

		OgreNewt::PlayerControllerBody* const playerControllerBody = (OgreNewt::PlayerControllerBody*)NewtonBodyGetUserData(controller->GetBody());

		Ogre::Real forwardSpeed = 0.0f;
		Ogre::Real strafeSpeed = 0.0f;

		if (playerControllerBody->getForwardSpeed() > 0.0f)
		{
			forwardSpeed = playerControllerBody->getWalkSpeed();
		}
		else if (playerControllerBody->getForwardSpeed() < 0.0f)
		{
			forwardSpeed = -playerControllerBody->getWalkSpeed();
		}

		if (playerControllerBody->getSideSpeed() > 0.0f)
		{
			forwardSpeed = playerControllerBody->getWalkSpeed();
		}
		else if (playerControllerBody->getSideSpeed() < 0.0f)
		{
			forwardSpeed = -playerControllerBody->getWalkSpeed();
		}
		
		// TODO: What about double or x-times jump?
		if (0.0f != playerControllerBody->getCanJump() && controller->IsOnFloor())
		{
			const dVector jumpImpule(controller->GetLocalFrame().RotateVector(dVector(playerControllerBody->getJumpSpeed() * controller->GetMass(), 0.0f, 0.0f, 0.0f)));
			const dVector totalImpulse(controller->GetImpulse() + jumpImpule);
			controller->SetImpulse(totalImpulse);
			playerControllerBody->setCanJump(false);
		}

		if (playerControllerBody->getForwardSpeed() && playerControllerBody->getSideSpeed())
		{
			const Ogre::Real invMag = playerControllerBody->getWalkSpeed() / Ogre::Math::Sqrt(forwardSpeed * forwardSpeed + strafeSpeed * strafeSpeed);
			forwardSpeed *= invMag;
			strafeSpeed *= invMag;
		}

		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "forward: " + Ogre::StringConverter::toString(forwardSpeed) + " side: "
		//  	+ Ogre::StringConverter::toString(strafeSpeed) + " heading: " + Ogre::StringConverter::toString(playerControllerBody->getHeading().valueDegrees()));

		// Set player linear and angular velocity
		controller->SetForwardSpeed(forwardSpeed);
		controller->SetLateralSpeed(strafeSpeed);
		controller->SetHeadingAngle(playerControllerBody->getHeading().valueRadians());
	}

	void BasicPlayerControllerManager::forceUpdate(void)
	{
		this->PreUpdate(0.016f, 0);
		this->PostStep(0.016f, 0);
		this->PostUpdate(0.016f, 0);
	}

	NewtonBody* BasicPlayerControllerManager::CreatePlayerController(NewtonWorld* const world, const dMatrix& location, const dMatrix& localAxis, dFloat mass, dFloat radius, dFloat height, dFloat stepHeight, dFloat startHeading, PlayerCallback* playerCallback)
	{
		if (nullptr != this->playerController)
		{
			// Note: Also destroys the player controller etc. So this is correct!
			NewtonDestroyBody(this->playerController->GetBody());
			this->playerController = nullptr;
		}
		this->playerController = new PlayerController(world, location, localAxis, mass, radius, height, stepHeight, startHeading, playerCallback);

		AddRoot(this->playerController);

		return this->playerController->GetBody();
	}

	PlayerController* BasicPlayerControllerManager::getPlayerController(void) const
	{
		return this->playerController;
	}

}   // end NAMESPACE OgreNewt

