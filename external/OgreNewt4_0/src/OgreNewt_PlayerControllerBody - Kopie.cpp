#include "OgreNewt_Stdafx.h"
#include "OgreNewt_PlayerControllerBody.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_PlayerController.h"

namespace
{
	Ogre::Real round(Ogre::Real number, unsigned int places)
	{
		if (number == -0.0f)
			number = 0.0f;
		Ogre::Real roundedNumber = number;
		roundedNumber *= powf(10, static_cast<Ogre::Real>(places));
		if (roundedNumber >= 0)
		{
			roundedNumber = floorf(roundedNumber + 0.5f);
		}
		else
		{
			roundedNumber = ceilf(roundedNumber - 0.5f);
		}
		roundedNumber /= powf(10, static_cast<Ogre::Real>(places));
		return roundedNumber;
	}
}

namespace OgreNewt
{
	PlayerControllerBody::PlayerControllerBody(World* world, Ogre::SceneManager* sceneManager, 
		const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition, const Ogre::Vector3& direction, 
		Ogre::Real mass, Ogre::Real radius, Ogre::Real height, Ogre::Real stepHeight, PlayerCallback* playerCallback)
		: Body(world, sceneManager),
		m_startOrientation(startOrientation),
		m_startPosition(startPosition),
		m_direction(direction),
		m_mass(mass),
		m_height(height),
		m_radius(radius),
		m_stepHeight(stepHeight),
		m_forwardSpeed(0.0f),
		m_sideSpeed(0.0f),
		m_heading(0.0f),
		m_playerController(nullptr),
		m_playerCallback(playerCallback),
		m_walkSpeed(10.0f),
		m_jumpSpeed(20.0f)
	{
		reCreatePlayer();
	}

	PlayerControllerBody::~PlayerControllerBody()
	{
		if (nullptr != m_playerCallback)
		{
			delete m_playerCallback;
			m_playerCallback = nullptr;
		}

		if (nullptr != m_playerController)
		{
			// Attention: necessary?
			((dCustomPlayerControllerManager*)m_playerController->GetManager())->DestroyController(m_playerController);
			m_playerController = nullptr;
		}
	}

	void PlayerControllerBody::move(Ogre::Real forwardSpeed, Ogre::Real sideSpeed, const Ogre::Radian& headingAngle)
	{
		m_forwardSpeed = forwardSpeed;
		m_sideSpeed = sideSpeed;
		m_heading = /*this->m_startOrientation.getYaw() +*/ headingAngle;
	}

	void PlayerControllerBody::stop(void)
	{
		m_forwardSpeed = 0.0f;
		m_sideSpeed =  0.0f;
		// Do not reset heading, else the player will rotate to 0, which looks ugly
		// m_heading = 0.0f;
	}

	void PlayerControllerBody::toggleCrouch(void)
	{
		m_playerController->ToggleCrouch();
	}

	void PlayerControllerBody::jump(void)
	{
		dVector jumpImpule(m_playerController->GetFrame().RotateVector(dVector(m_jumpSpeed * m_playerController->GetMass(), 0.0f, 0.0f, 0.0f)));
		dVector totalImpulse(m_playerController->GetImpulse() + jumpImpule);
		m_playerController->SetImpulse(totalImpulse);
	}

	void PlayerControllerBody::setVelocity(const Ogre::Vector3& velocity)
	{
		m_playerController->SetVelocity(&velocity.x);
	}

	Ogre::Vector3 PlayerControllerBody::getVelocity(void) const
	{
		dVector tempVelocity = m_playerController->GetVelocity();

		return Ogre::Vector3(tempVelocity.m_x, tempVelocity.m_y, tempVelocity.m_z);
	}

	void PlayerControllerBody::setFrame(const Ogre::Quaternion& frame)
	{
		float matrix[16];
		OgreNewt::Converters::QuatPosToMatrix(frame, Ogre::Vector3::ZERO, &matrix[0]);
		m_playerController->SetFrame(&matrix[0]);
	}

	Ogre::Quaternion PlayerControllerBody::getFrame(void) const
	{
		dMatrix matrix = m_playerController->GetFrame();
		Ogre::Quaternion frame;
		Ogre::Vector3 pos;
		OgreNewt::Converters::MatrixToQuatPos(&matrix[0][0], frame, pos);
		return frame;
	}

	void PlayerControllerBody::reCreatePlayer()
	{
		if (nullptr != m_playerController)
		{
			((dCustomPlayerControllerManager*)m_playerController->GetManager())->DestroyController(m_playerController);
			m_playerController = nullptr;
		}

		dFloat positionOrientation[16];
		OgreNewt::Converters::QuatPosToMatrix(m_startOrientation, m_startPosition, positionOrientation);
		// OgreNewt::Converters::QuatPosToMatrix(Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, positionOrientation);

		Ogre::Vector3 orientationVector = m_startOrientation * m_direction;

		for (unsigned short i = 0; i < 16; i++)
		{
			positionOrientation[i] = round(positionOrientation[i], 6);
		}

		orientationVector.x = round(orientationVector.x, 6);
		orientationVector.y = round(orientationVector.y, 6);
		orientationVector.z = round(orientationVector.z, 6);
		
		if (Ogre::Math::RealEqual(orientationVector.x, 0.0f))
		{
			orientationVector.x = 0.0f;
		}
		if (Ogre::Math::RealEqual(orientationVector.y, 0.0f))
		{
			orientationVector.y = 0.0f;
		}
		if (Ogre::Math::RealEqual(orientationVector.z, 0.0f))
		{
			orientationVector.z = 0.0f;
		}

		// create a newton player controller
		dMatrix localAxis(dGetIdentityMatrix());
		localAxis[0] = dVector(0.0f, 1.0f, 0.0f, 0.0f); // the y axis is the character up vector
		localAxis[1] = dVector(orientationVector.x, orientationVector.y, orientationVector.z, 0.0);
		localAxis[2] = localAxis[0].CrossProduct(localAxis[1]);

		m_playerController = m_world->getPlayerControllerManager()->CreateController(&positionOrientation[0], localAxis, m_mass, m_radius, m_height, m_stepHeight, m_playerCallback);
		// players by default have the origin at the center of mass of the collision shape.

		// Ogre::Degree defaultSlope(30.0f);
		// m_maxClimbSlope = defaultSlope.valueRadians();
		// m_playerController->SetClimbSlope(defaultSlope.valueRadians());

		// get body from player, and set some parameter
		m_body = m_playerController->GetBody();
		NewtonBodySetUserData(m_body, this);

		// NewtonCollisionSetMatrix(NewtonBodyGetCollision(m_body), &playerAxis[0][0]);

		// setLinearDamping(m_world->getDefaultLinearDamping() * (60.0f / m_world->getUpdateFPS()));
		// setAngularDamping(m_world->getDefaultAngularDamping() * (60.0f / m_world->getUpdateFPS()));

		// set the transform callback
		NewtonBodySetTransformCallback(m_body, Body::newtonTransformCallback);

		// this->setFrame(Ogre::Quaternion(m_startOrientation.getYaw(), m_direction));

		this->stop();
	}

	void PlayerControllerBody::setDirection(const Ogre::Vector3& direction)
	{
		m_direction = direction;
	}

	Ogre::Vector3 PlayerControllerBody::getDirection(void) const
	{
		return m_direction;
	}

	void PlayerControllerBody::setMass(Ogre::Real mass)
	{
		m_mass = mass;
	}

	void PlayerControllerBody::setRadius(Ogre::Real radius)
	{
		m_radius = radius;
	}

	Ogre::Real PlayerControllerBody::getRadius(void) const
	{
		return m_radius;
	}

	void PlayerControllerBody::setHeight(Ogre::Real height)
	{
		m_height = height;
	}

	Ogre::Real PlayerControllerBody::getheight(void) const
	{
		return m_height;
	}

	void PlayerControllerBody::setStepHeight(Ogre::Real stepHeight)
	{
		m_stepHeight = stepHeight;
	}

	Ogre::Real PlayerControllerBody::getStepHeight(void) const
	{
		return m_stepHeight;
	}

	Ogre::Vector3 PlayerControllerBody::getUpDirection(void) const
	{
		dVector upDir/* = m_playerController->GetUpDir()*/;
		return Ogre::Vector3(upDir.m_x, upDir.m_y, upDir.m_z);
	}

	bool PlayerControllerBody::isInFreeFall(void) const
	{
		return m_playerController->IsAirBorn();
	}

	bool PlayerControllerBody::isOnFloor(void) const
	{
		return m_playerController->IsOnFloor();
	}

	bool PlayerControllerBody::isCrouched(void) const
	{
		return m_playerController->IsCrouched();
	}

	void PlayerControllerBody::setWalkSpeed(Ogre::Real walkSpeed)
	{
		m_walkSpeed = walkSpeed;
	}

	Ogre::Real PlayerControllerBody::getWalkSpeed(void) const
	{
		return m_walkSpeed;
	}

	void PlayerControllerBody::setJumpSpeed(Ogre::Real jumpSpeed)
	{
		m_jumpSpeed = jumpSpeed;
	}

	Ogre::Real PlayerControllerBody::getJumpSpeed(void) const
	{
		return m_jumpSpeed;
	}

	Ogre::Real PlayerControllerBody::getForwardSpeed(void) const
	{
		return m_forwardSpeed;
	}

	Ogre::Real PlayerControllerBody::getSideSpeed(void) const
	{
		return m_sideSpeed;
	}

	Ogre::Radian PlayerControllerBody::getHeading(void) const
	{
		return /*this->m_startOrientation.getYaw() +*/ m_heading;
	}

	void PlayerControllerBody::setStartOrientation(const Ogre::Quaternion& startOrientation)
	{
		m_startOrientation = startOrientation;
	}

	PlayerCallback* PlayerControllerBody::getPlayerCallback(void) const
	{
		return m_playerCallback;
	}
}
