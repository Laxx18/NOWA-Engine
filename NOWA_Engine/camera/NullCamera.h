#include "NOWAPrecompiled.h"

#ifndef NULL_CAMERA_H
#define NULL_CAMERA_H

#include "BaseCamera.h"

namespace NOWA
{
	class EXPORTED NullCamera : public BaseCamera
	{
	public:
		NullCamera(unsigned int id)
			: BaseCamera(id)
		{

		}

		virtual ~NullCamera()
		{

		}

		virtual void moveCamera(Ogre::Real dt)
		{

		}

		virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false)
		{

		}

		virtual Ogre::Vector3 getPosition(void)
		{
			return Ogre::Vector3::ZERO;
		}

		virtual Ogre::Quaternion getOrientation(void)
		{
			return Ogre::Quaternion::IDENTITY;
		}

		virtual Ogre::String getBehaviorType(void)
		{
			return "NO_ACTIVE_CAMERA_" + Ogre::StringConverter::toString(this->id);
		}

		static Ogre::String BehaviorType(void)
		{
			return "NO_ACTIVE_CAMERA";
		}
	protected:

		virtual void onSetData(void)
		{

		}

		virtual void onClearData(void)
		{

		}
	};

}; //namespace end

#endif