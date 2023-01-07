#include "NOWAPrecompiled.h"
#include "BaseCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "main/InputDeviceCore.h"
#include "utilities/MathHelper.h"
#include "modules/InputDeviceModule.h"

namespace NOWA
{
	BaseCamera::BaseCamera(Ogre::Real moveSpeed, Ogre::Real rotateSpeed, Ogre::Real smoothValue, const Ogre::Vector3& defaultDirection)
		: moveSpeed(moveSpeed),
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
		rotateCameraWeight(1.0f)
	{
		
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
	}

	void BaseCamera::setDefaultDirection(const Ogre::Vector3& defaultDirection)
	{
		this->defaultDirection = defaultDirection;
	}

	void BaseCamera::moveCamera(Ogre::Real dt)
	{
		if (true == cameraControlLocked)
			return;
		
		Ogre::Vector3 moveValue = Ogre::Vector3::ZERO;

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
				moveVertical = 0.0f;

			moveHorizontal = (Ogre::Real)joystickState.mAxes[3].abs / 32767.0f;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: moveVertical " + Ogre::StringConverter::toString(moveVertical));
			if (Ogre::Math::Abs(moveHorizontal) < DEAD_ZONE)
				moveHorizontal = 0.0f;
			

			moveValue += Ogre::Vector3((this->moveSpeed * moveHorizontal) * dt, 0.0f, (this->moveSpeed * moveVertical) * dt * this->moveCameraWeight);

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
				moveValue += Ogre::Vector3(-this->moveSpeed * dt * static_cast<Ogre::Real>(abs), 0.0f, 0.0f);
			}*/
		}
		
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_LEFT))
		{
			moveValue += Ogre::Vector3(-this->moveSpeed * dt * this->moveCameraWeight, 0.0f, 0.0f);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_RIGHT))
		{
			moveValue += Ogre::Vector3(this->moveSpeed * dt * this->moveCameraWeight, 0.0f, 0.0f);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_BACKWARD))
		{
			moveValue += Ogre::Vector3(0.0f, 0.0f, this->moveSpeed * dt * this->moveCameraWeight);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_FORWARD))
		{
			moveValue += Ogre::Vector3(0.0f, 0.0f, -this->moveSpeed * dt * this->moveCameraWeight);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_DOWN))
		{
			moveValue += Ogre::Vector3(0.0f, -this->moveSpeed * dt * this->moveCameraWeight, 0.0f);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_UP))
		{
			moveValue += Ogre::Vector3(0.0f, this->moveSpeed * dt * this->moveCameraWeight, 0.0f);
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
		if (this->firstTimeMoveValueSet)
		{
			this->lastMoveValue = moveValue;
			this->firstTimeMoveValueSet = false;
		}

		if ((moveValue - this->lastMoveValue).squaredLength() > 10.0f)
			this->lastMoveValue = moveValue;

		moveValue.x = NOWA::MathHelper::getInstance()->lowPassFilter(moveValue.x, this->lastMoveValue.x, this->smoothValue);
		moveValue.y = NOWA::MathHelper::getInstance()->lowPassFilter(moveValue.y, this->lastMoveValue.y, this->smoothValue);
		moveValue.z = NOWA::MathHelper::getInstance()->lowPassFilter(moveValue.z, this->lastMoveValue.z, this->smoothValue);

		// Same as camera->moveRelative
		// Ogre::Vector3 trans = this->camera->getParentSceneNode()->getOrientation() * moveValue;
		// this->camera->getParentSceneNode()->translate(trans);
		this->camera->moveRelative(moveValue);

		this->lastMoveValue = moveValue;
	}

	void BaseCamera::rotateCamera(Ogre::Real dt, bool forJoyStick)
	{
		if (true == cameraControlLocked)
			return;

		Ogre::Vector2 rotationValue = Ogre::Vector2::ZERO;

		if (false == forJoyStick)
		{
			const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
			rotationValue.x = -ms.X.rel * this->rotateSpeed * dt * this->rotateCameraWeight;
			rotationValue.y = -ms.Y.rel * this->rotateSpeed * dt * this->rotateCameraWeight;
		}
		else
		{
			OIS::JoyStick* joyStick = NOWA::InputDeviceCore::getSingletonPtr()->getJoystick(0);

			if (nullptr != joyStick)
			{
				Ogre::Real rotateHorizontal = 0.0f; // Range is -1.f for full left to +1.f for full right
				Ogre::Real rotateVertical = 0.0f; // -1.f for full down to +1.f for full up.

				const OIS::JoyStickState& joystickState = joyStick->getJoyStickState();

				// We receive the full analog range of the axes, and so have to implement our
				// own dead zone.  This is an empirical value, since some joysticks have more
				// jitter or creep around the center point than others.  We'll use 5% of the
				// range as the dead zone, but generally you would want to give the user the
				// option to change this.
				const Ogre::Real DEAD_ZONE = 0.09f;

				rotateVertical = (Ogre::Real)joystickState.mAxes[0].abs / -32767.0f;
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: rotateVertical " + Ogre::StringConverter::toString(rotateVertical));
				if (Ogre::Math::Abs(rotateVertical) < DEAD_ZONE)
					rotateVertical = 0.0f;

				rotateHorizontal = (Ogre::Real)joystickState.mAxes[1].abs / -32767.0f;
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: rotateHorizontal " + Ogre::StringConverter::toString(rotateHorizontal));
				if (Ogre::Math::Abs(rotateHorizontal) < DEAD_ZONE)
					rotateHorizontal = 0.0f;

				rotationValue.x = (this->rotateSpeed * 10.0f * rotateHorizontal) * dt * this->rotateCameraWeight;
				rotationValue.y = (this->rotateSpeed * 10.0f * rotateVertical) * dt * this->rotateCameraWeight;
			}
		}
		

		if (this->firstTimeValueSet)
		{
			this->lastValue = rotationValue;
			this->firstTimeValueSet = false;
		}

		if ((this->lastValue - rotationValue).squaredLength() > 10.0f)
			this->lastValue = rotationValue;

		rotationValue.x = NOWA::MathHelper::getInstance()->lowPassFilter(rotationValue.x, this->lastValue.x, this->smoothValue);
		rotationValue.y = NOWA::MathHelper::getInstance()->lowPassFilter(rotationValue.y, this->lastValue.y, this->smoothValue);

		/*this->camera->getParentSceneNode()->_getFullTransformUpdated();
		this->camera->getParentSceneNode()->yaw(Ogre::Degree(rotationValue.x), Ogre::Node::TS_WORLD);
		this->camera->getParentSceneNode()->pitch(Ogre::Degree(rotationValue.y));
		this->camera->getParentSceneNode()->roll(Ogre::Radian(0.0f));*/

		this->camera->yaw(Ogre::Degree(rotationValue.x));
		this->camera->pitch(Ogre::Degree(rotationValue.y));
		this->camera->roll(Ogre::Radian(0.0f));

		this->lastValue = rotationValue;
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
