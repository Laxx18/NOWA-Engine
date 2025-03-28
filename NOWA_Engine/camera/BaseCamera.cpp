#include "NOWAPrecompiled.h"
#include "BaseCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "main/InputDeviceCore.h"
#include "utilities/MathHelper.h"
#include "modules/InputDeviceModule.h"
#include "MyGUI_InputManager.h"

namespace NOWA
{
	BaseCamera::BaseCamera(unsigned int id, Ogre::Real moveSpeed, Ogre::Real rotateSpeed, Ogre::Real smoothValue, const Ogre::Vector3& defaultDirection)
		: id(id),
		moveSpeed(moveSpeed),
		rotateSpeed(rotateSpeed),
		smoothValue(smoothValue),
		defaultDirection(defaultDirection),
		camera(nullptr),
		lastValue(Ogre::Vector2::ZERO),
		lastMoveValue(Ogre::Vector3::ZERO),
		firstTimeValueSet(true),
		firstTimeMoveValueSet(true),
		cameraControlLocked(false),
		moveCameraWeight(1.0f),
		rotateCameraWeight(1.0f),
		gravityDirection(Ogre::Vector3::UNIT_Y)
	{
		this->smoothValue = 0.01f;
	}

	BaseCamera::~BaseCamera() 
	{

	}

	void BaseCamera::onSetData(void)
	{
		this->firstTimeValueSet = true;
	}

	void BaseCamera::onClearData(void)
	{
		
	}

	void BaseCamera::postInitialize(Ogre::Camera* camera)
	{
		this->camera = camera;
		// this->camera->setQueryFlags(0);
	}

	void BaseCamera::reset(void)
	{
		this->firstTimeMoveValueSet = true;
		this->lastValue = Ogre::Vector2::ZERO;
		this->lastMoveValue = Ogre::Vector3::ZERO;
		this->firstTimeValueSet = true;
		this->gravityDirection = Ogre::Vector3::ZERO;
	}

	void BaseCamera::setDefaultDirection(const Ogre::Vector3& defaultDirection)
	{
		this->defaultDirection = defaultDirection;
	}

	void BaseCamera::applyGravityDirection(const Ogre::Vector3& gravityDirection)
	{
		this->gravityDirection = gravityDirection;
	}

	void BaseCamera::moveCamera(Ogre::Real dt)
	{
		if (true == cameraControlLocked)
			return;

		MyGUI::Widget* widget = MyGUI::InputManager::getInstance().getMouseFocusWidget();
		if (nullptr != widget)
		{
			return;
		}
		
		Ogre::Vector3 moveValue = Ogre::Vector3::ZERO;
		bool isMoving = false;

		OIS::JoyStick* joyStick = NOWA::InputDeviceCore::getSingletonPtr()->getJoystick(0);

		if (nullptr != joyStick)
		{
			Ogre::Real moveHorizontal = 0.0f; // Range is -1.f for full left to +1.f for full right
			Ogre::Real moveVertical = 0.0f; // -1.f for full down to +1.f for full up.

			const OIS::JoyStickState& joystickState = joyStick->getJoyStickState();

			// We receive the full analog range of the axes, and so have to implement our
			// own dead zone.  This is an empirical value, since some joysticks have more
			// jitter or creep around the center point than others.  We'll use 5% of the
			// range as the dead zone, but generally you would want to give the user the
			// option to change this.
			const Ogre::Real DEAD_ZONE = 0.05f;

			moveVertical = (Ogre::Real)joystickState.mAxes[2].abs / 32767.0f;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: moveHorizontal " + Ogre::StringConverter::toString(moveHorizontal));
			if (Ogre::Math::Abs(moveVertical) < DEAD_ZONE)
			{
				isMoving = false;
			}
			else
			{
				isMoving = true;
			}

			moveHorizontal = (Ogre::Real)joystickState.mAxes[3].abs / 32767.0f;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: moveVertical " + Ogre::StringConverter::toString(moveVertical));
			if (Ogre::Math::Abs(moveHorizontal) < DEAD_ZONE)
			{
				isMoving = false;
			}
			else
			{
				isMoving = true;
			}

			moveValue += Ogre::Vector3((this->moveSpeed * moveHorizontal), 0.0f, (this->moveSpeed * moveVertical) * this->moveCameraWeight);

			//// POV hat info is only currently supported on Windows, but the value is
			//// guaranteed to be 65535 if it's not supported, so we can check its range.
			//const u16 povDegrees = joystickState.mPOV. / 100;
			//if (povDegrees < 360)
			//{
			//	if (povDegrees > 0 && povDegrees < 180)
			//		moveHorizontal = 1.f;
			//	else if (povDegrees > 180)
			//		moveHorizontal = -1.f;

			//	if (povDegrees > 90 && povDegrees < 270)
			//		moveVertical = -1.f;
			//	else if (povDegrees > 270 || povDegrees < 90)
			//		moveVertical = +1.f;
			//}
			
			/*int abs = joyStick->getJoyStickState().mAxes[0].abs;
			if (abs != 0)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[BaseCamera]: Abs " + Ogre::StringConverter::toString(abs));
				moveValue += Ogre::Vector3(-this->moveSpeed  * static_cast<Ogre::Real>(abs), 0.0f, 0.0f);
			}*/
		}
		
		// Normalize movement speed by frame time
		Ogre::Real normalizedMoveSpeed = this->moveSpeed * this->moveCameraWeight;

		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_LEFT))
		{
			moveValue += Ogre::Vector3(-normalizedMoveSpeed, 0.0f, 0.0f);
			isMoving = true;
		}
		else if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_RIGHT))
		{
			moveValue += Ogre::Vector3(normalizedMoveSpeed, 0.0f, 0.0f);
			isMoving = true;
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_BACKWARD))
		{
			moveValue += Ogre::Vector3(0.0f, 0.0f, normalizedMoveSpeed);
			isMoving = true;
		}
		else if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_FORWARD))
		{
			moveValue += Ogre::Vector3(0.0f, 0.0f, -normalizedMoveSpeed);
			isMoving = true;
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_DOWN))
		{
			moveValue += Ogre::Vector3(0.0f, -normalizedMoveSpeed, 0.0f);
			isMoving = true;
		}
		else if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_UP))
		{
			moveValue += Ogre::Vector3(0.0f, normalizedMoveSpeed, 0.0f);
			isMoving = true;
		}
		
		if (this->camera->getProjectionType() == Ogre::PT_ORTHOGRAPHIC)
		{
			Ogre::Real height = this->camera->getOrthoWindowHeight() + moveValue.z;
			// min window size
			//h = std::max(h, 0.0001f);
			if (height > 0.0001f)
			{
				Ogre::Real width = height * this->camera->getAspectRatio();
				this->camera->setOrthoWindow(width, height);
			}
			if (this->camera->getOrthoWindowHeight() < 1.0f)
			{
				this->camera->setOrthoWindowHeight(1.0f);
			}
		}
		if (true == this->firstTimeMoveValueSet)
		{
			this->lastMoveValue = moveValue;
			this->firstTimeMoveValueSet = false;
		}

		// Adjust low-pass filter interpolation based on frame time
		Ogre::Real dynamicSmoothValue = this->smoothValue * dt;

		// If not moving, gradually reduce lastMoveValue to zero
		if (false == isMoving)
		{
			moveValue = Ogre::Vector3::ZERO;
			dynamicSmoothValue = this->smoothValue * dt * 1000.0f; // Faster deceleration
		}

		moveValue = NOWA::MathHelper::getInstance()->lowPassFilter(moveValue, this->lastMoveValue, dynamicSmoothValue);

		if (moveValue.length() > 0.0001f)
		{
			// Move camera relative to frame time
			this->camera->moveRelative(moveValue);
			this->lastMoveValue = moveValue;
		}
		else
		{
			this->lastMoveValue = Ogre::Vector3::ZERO;
		}
	}

	void BaseCamera::rotateCamera(Ogre::Real dt, bool forJoyStick)
	{
		if (true == cameraControlLocked)
		{
			return;
		}

		Ogre::Vector2 rotationValue = Ogre::Vector2::ZERO;

		// Normalize rotation speed by frame time
		Ogre::Real normalizedRotateSpeed = this->rotateSpeed * this->rotateCameraWeight;
		bool isRotating = false;

		// Input handling remains the same
		if (false == forJoyStick)
		{
			const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
			isRotating = ms.X.rel != 0 || ms.Y.rel != 0;
			if (true == isRotating)
			{
				rotationValue.x = -ms.X.rel * normalizedRotateSpeed;
				rotationValue.y = -ms.Y.rel * normalizedRotateSpeed;
			}
		}
		else
		{
			OIS::JoyStick* joyStick = NOWA::InputDeviceCore::getSingletonPtr()->getJoystick(0);
			if (nullptr != joyStick)
			{
				Ogre::Real rotateHorizontal = 0.0f;
				Ogre::Real rotateVertical = 0.0f;
				const OIS::JoyStickState& joystickState = joyStick->getJoyStickState();
				const Ogre::Real DEAD_ZONE = 0.09f;

				rotateVertical = (Ogre::Real)joystickState.mAxes[0].abs / -32767.0f;
				if (Ogre::Math::Abs(rotateVertical) < DEAD_ZONE)
				{
					rotateVertical = 0.0f;
					isRotating = false;
				}
				else
				{
					isRotating = true;
				}

				rotateHorizontal = (Ogre::Real)joystickState.mAxes[1].abs / -32767.0f;
				if (Ogre::Math::Abs(rotateHorizontal) < DEAD_ZONE)
				{
					rotateHorizontal = 0.0f;
					isRotating = false;
				}
				else
				{
					isRotating = true;
				}

				rotationValue.x = normalizedRotateSpeed * 10.0f * rotateHorizontal;
				rotationValue.y = normalizedRotateSpeed * 10.0f * rotateVertical;
			}
		}

		if (this->firstTimeValueSet)
		{
			this->lastValue = rotationValue;
			this->firstTimeValueSet = false;
		}

		// Adjust low-pass filter interpolation based on frame time
		Ogre::Real dynamicSmoothValue = this->smoothValue * dt;

		// If not rotating, gradually reduce lastValue to zero
		if (false == isRotating)
		{
			rotationValue = Ogre::Vector2::ZERO;
			dynamicSmoothValue = this->smoothValue * dt * 1000.0f; // Faster deceleration
		}

		rotationValue.x = NOWA::MathHelper::getInstance()->lowPassFilter(rotationValue.x, this->lastValue.x, dynamicSmoothValue);
		rotationValue.y = NOWA::MathHelper::getInstance()->lowPassFilter(rotationValue.y, this->lastValue.y, dynamicSmoothValue);

		if (rotationValue.length() > 0.0001f)
		{
			// Create a local coordinate system based on gravity
			Ogre::Vector3 upVector = -this->gravityDirection;

			// Temporarily set camera's up vector to match the gravity-based up
			this->camera->setFixedYawAxis(true, upVector);

			// Apply rotations using standard camera methods
			this->camera->yaw(Ogre::Degree(rotationValue.x));
			this->camera->pitch(Ogre::Degree(rotationValue.y));
			this->camera->roll(Ogre::Radian(0.0f));

			// Reset fixed yaw axis setting if needed
			if (this->gravityDirection.directionEquals(Ogre::Vector3::UNIT_Y, Ogre::Radian(0.001f)))
			{
				// For normal gravity, keep Y as up
				this->camera->setFixedYawAxis(true, Ogre::Vector3::UNIT_Y);
			}

			this->lastValue = rotationValue;
		}
		else
		{
			this->lastValue = Ogre::Vector2::ZERO;
		}
	}

	Ogre::Vector3 BaseCamera::getPosition(void)
	{
		return this->camera->getPositionForViewUpdate();
	}

	Ogre::Quaternion BaseCamera::getOrientation(void)
	{
		return this->camera->getOrientationForViewUpdate();
	}

	void BaseCamera::setMoveSpeed(Ogre::Real moveSpeed)
	{
		this->moveSpeed = moveSpeed;
	}

	void BaseCamera::setRotationSpeed(Ogre::Real rotationSpeed)
	{
		this->rotateSpeed = rotationSpeed;
	}

	unsigned int BaseCamera::getId(void) const
	{
		return this->id;
	}

	Ogre::Camera* BaseCamera::getCamera(void) const
	{
		return this->camera;
	}

	void BaseCamera::setCameraControlLocked(bool cameraControlLocked)
	{
		this->cameraControlLocked = cameraControlLocked;
	}

	bool BaseCamera::getCameraControlLocked(void) const
	{
		return this->cameraControlLocked;
	}
	
	void BaseCamera::setSmoothValue(Ogre::Real smoothValue)
	{
		this->smoothValue = smoothValue;
	}
	
	Ogre::Real BaseCamera::getSmoothValue(void) const
	{
		return this->smoothValue;
	}
	
}; //namespace end
