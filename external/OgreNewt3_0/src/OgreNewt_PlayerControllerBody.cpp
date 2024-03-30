#include "OgreNewt_Stdafx.h"
#include "OgreNewt_PlayerControllerBody.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"

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

	Ogre::Real normalizeDegreeAngle(Ogre::Real degree)
	{
		if ((degree < 0.0) || (degree > 360.0f))
		{
			degree = std::fmod(degree, 360.0f);
			while (degree < 0.0)
			{
				degree = degree + 360.0f;
			}
			while (degree > 360.0f)
			{
				degree = degree - 360.0f;
			}
		}
		return degree;
	}

	Ogre::Real normalizeRadianAngle(Ogre::Real radian)
	{
		Ogre::Real normalizedDegree = normalizeDegreeAngle(((radian * 360.0f) / Ogre::Math::TWO_PI));
		return ((normalizedDegree * Ogre::Math::TWO_PI) / 360.0f);
	}
}

namespace OgreNewt
{
	PlayerControllerBody::PlayerControllerBody(World* world, Ogre::SceneManager* sceneManager,
		const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition, const Ogre::Vector3& direction,
		Ogre::Real mass, Ogre::Real radius, Ogre::Real height, Ogre::Real stepHeight, unsigned int categoryId, PlayerCallback* playerCallback)
		: Body(world, sceneManager),
		world(world),
		m_startOrientation(startOrientation),
		m_startPosition(startPosition),
		m_oldStartPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		m_oldStartOrientation(Ogre::Quaternion(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, 1.0f)),
		m_direction(direction),
		m_mass(mass),
		m_collisionPositionOffset(Ogre::Vector3::ZERO),
		m_height(height),
		m_radius(radius),
		m_stepHeight(stepHeight),
		m_forwardSpeed(0.0f),
		m_sideSpeed(0.0f),
		m_heading(0.0f),
		m_startHeading(0.0f),
		// Note will be recreated in reCreatePlayer 
		m_playerCallback(nullptr),
		m_walkSpeed(10.0f),
		m_jumpSpeed(20.0f),
		m_canJump(false),
		// Must be created for each body! Else several player controller will not work!
		m_playerControllerManager(new BasicPlayerControllerManager(this->world->getNewtonWorld()))
	{
		reCreatePlayer(startOrientation, startPosition, direction, mass, radius, height, stepHeight, categoryId, playerCallback);
	}

	PlayerControllerBody::~PlayerControllerBody()
	{
		if (nullptr != m_playerCallback)
		{
			delete m_playerCallback;
			m_playerCallback = nullptr;
		}
	}

	void PlayerControllerBody::move(Ogre::Real forwardSpeed, Ogre::Real sideSpeed, const Ogre::Radian& headingAngle)
	{
		m_forwardSpeed = forwardSpeed;
		m_sideSpeed = sideSpeed;
		Ogre::Radian newHeading(m_startHeading + headingAngle);
		m_heading = Ogre::Radian(normalizeRadianAngle(newHeading.valueRadians()));
	}

	void PlayerControllerBody::setHeading(const Ogre::Radian& headingAngle)
	{
		Ogre::Radian newHeading(m_startHeading + headingAngle);
		m_heading = Ogre::Radian(normalizeRadianAngle(newHeading.valueRadians()));
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
		m_playerControllerManager->getPlayerController()->ToggleCrouch();
	}

	void PlayerControllerBody::setVelocity(const Ogre::Vector3& velocity)
	{
		m_playerControllerManager->getPlayerController()->SetVelocity(&velocity.x);
	}

	Ogre::Vector3 PlayerControllerBody::getVelocity(void) const
	{
		dVector tempVelocity = m_playerControllerManager->getPlayerController()->GetVelocity();

		return Ogre::Vector3(tempVelocity.m_x, tempVelocity.m_y, tempVelocity.m_z);
	}

	void PlayerControllerBody::setFrame(const Ogre::Quaternion& frame)
	{
		// float matrix[16];
		// OgreNewt::Converters::QuatPosToMatrix(frame, Ogre::Vector3::ZERO, &matrix[0]);
		// m_playerController->SetFrame(&matrix[0]);
	}

	Ogre::Quaternion PlayerControllerBody::getFrame(void) const
	{
		dMatrix matrix = m_playerControllerManager->getPlayerController()->GetLocalFrame();
		Ogre::Quaternion frame;
		Ogre::Vector3 pos;
		OgreNewt::Converters::MatrixToQuatPos(&matrix[0][0], frame, pos);
		return frame;
	}

	void PlayerControllerBody::reCreatePlayer(const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition, const Ogre::Vector3& direction,
		Ogre::Real mass, Ogre::Real radius, Ogre::Real height, Ogre::Real stepHeight, unsigned int categoryId, PlayerCallback* playerCallback)
	{
		m_startOrientation = startOrientation;
		m_startPosition = startPosition;

		if (m_oldStartPosition == m_startPosition && m_oldStartOrientation == m_startOrientation)
		{
			return;
		}

		m_oldStartPosition = m_startPosition;
		m_oldStartOrientation = m_startOrientation;

		m_direction = direction;
		m_mass = mass;
		m_radius = radius;
		m_height = height;
		m_stepHeight = stepHeight;

		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "----->: " + Ogre::StringConverter::toString(m_startHeading.valueDegrees()));

		if (nullptr != m_playerCallback)
		{
			delete m_playerCallback;
		}

		m_playerCallback = playerCallback;

		dFloat positionOrientation[16];
		OgreNewt::Converters::QuatPosToMatrix(m_startOrientation, m_startPosition, positionOrientation);

		for (unsigned short i = 0; i < 16; i++)
		{
			positionOrientation[i] = round(positionOrientation[i], 6);
		}

		// create a newton player controller
		dMatrix localAxis(dGetIdentityMatrix());
		localAxis[0] = dVector(0.0f, 1.0f, 0.0f, 0.0f); // the y axis is the character up vector
		localAxis[1] =  dVector(m_direction.x, m_direction.y, m_direction.z, 0.0); // dVector(orientationVector.x, orientationVector.y, orientationVector.z, 0.0);
		localAxis[2] = localAxis[0].CrossProduct(localAxis[1]);

		m_body = m_playerControllerManager->CreatePlayerController(world->getNewtonWorld(), &positionOrientation[0], localAxis, m_mass, m_radius, m_height, m_stepHeight, m_startHeading.valueRadians(), m_playerCallback);

		// players by default have the origin at the center of mass of the collision shape.

		// Ogre::Degree defaultSlope(30.0f);
		// m_maxClimbSlope = defaultSlope.valueRadians();
		// m_playerController->SetClimbSlope(defaultSlope.valueRadians());

		NewtonBodySetUserData(m_body, this);

		if (Ogre::Vector3::ZERO != m_collisionPositionOffset)
		{
			dMatrix collisionMatrix;
			NewtonCollisionGetMatrix(NewtonBodyGetCollision(m_body), &collisionMatrix[0][0]);

			collisionMatrix.m_posit = dVector(m_collisionPositionOffset.x, m_collisionPositionOffset.y, m_collisionPositionOffset.z, 0.0f);

			NewtonCollisionSetMatrix(NewtonBodyGetCollision(m_body), &collisionMatrix[0][0]);
		}

		// set the transform callback
		NewtonBodySetTransformCallback(m_body, Body::newtonTransformCallback);

		// Important, so that the current start heading is actualized for start up orientation of the body
		this->move(0.0f, 0.0f, m_startOrientation.getYaw());

		m_playerControllerManager->getPlayerController()->SetHeadingAngle(m_startHeading.valueRadians());

		this->setType(categoryId);

		m_playerControllerManager->forceUpdate();
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

	void PlayerControllerBody::setCollisionPositionOffset(const Ogre::Vector3& collisionPositionOffset)
	{
		m_collisionPositionOffset = collisionPositionOffset;
	}

	Ogre::Vector3 PlayerControllerBody::getCollisionPositionOffset(void) const
	{
		return m_collisionPositionOffset;
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
		return m_playerControllerManager->getPlayerController()->IsAirBorn();
	}

	bool PlayerControllerBody::isOnFloor(void) const
	{
		return m_playerControllerManager->getPlayerController()->IsOnFloor();
	}

	bool PlayerControllerBody::isCrouched(void) const
	{
		return m_playerControllerManager->getPlayerController()->IsCrouched();
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

	void PlayerControllerBody::setCanJump(bool canJump)
	{
		m_canJump = canJump;
	}

	bool PlayerControllerBody::getCanJump(void) const
	{
		return m_canJump;
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
		return m_heading;
	}

	void PlayerControllerBody::setStartOrientation(const Ogre::Quaternion& startOrientation)
	{
		m_startOrientation = startOrientation;
	}

	Ogre::Quaternion PlayerControllerBody::getStartOrientation(void) const
	{
		return m_startOrientation;
	}

	PlayerCallback* PlayerControllerBody::getPlayerCallback(void) const
	{
		return m_playerCallback;
	}
}
