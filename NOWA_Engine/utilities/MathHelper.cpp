#include "NOWAPrecompiled.h"
#include "MathHelper.h"
#include "C2DMatrix.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "OgreBitwise.h"
#include "Vao/OgreAsyncTicket.h"
#include "Terra.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

namespace NOWA
{
	MathHelper* MathHelper::getInstance()
	{
		static MathHelper instance;
		return &instance;
	}

	Ogre::Vector3 MathHelper::calibratedFromScreenSpaceToWorldSpace(Ogre::SceneNode* node, Ogre::Camera* camera, const Ogre::Vector2& mousePosition, const Ogre::Real& offset)
	{
		// calculate ray from camera to viewport
		Ogre::Ray mouseRay = camera->getCameraToViewportRay(mousePosition.x, mousePosition.y);

		Ogre::Vector3 pos = node->getPosition();
		Ogre::Vector3 dir = node->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Z;

		// shoot the ray onto the normale to get the distance
		// rotate the normale always to the obect and get the position of the object
		Ogre::Real r = mouseRay.intersects(Ogre::Plane(dir, pos + (dir * offset))).second;

		return mouseRay.getPoint(r);
	}

	Ogre::Vector4 MathHelper::getCalibratedWindowCoordinates(Ogre::Real x, Ogre::Real y, Ogre::Real width, Ogre::Real height, Ogre::Viewport* viewport)
	{
		Ogre::Real left = (x / Ogre::Real(viewport->getActualWidth())) - (width * 0.5f / Ogre::Real(viewport->getActualWidth()));;
		Ogre::Real top = (y / Ogre::Real(viewport->getActualHeight())) - (height * 0.5f / Ogre::Real(viewport->getActualHeight()));;
		Ogre::Real right = (x / Ogre::Real(viewport->getActualWidth())) + (width * 0.5f / Ogre::Real(viewport->getActualWidth()));
		Ogre::Real bottom = (y / Ogre::Real(viewport->getActualHeight())) + (height * 0.5f / Ogre::Real(viewport->getActualHeight()));
		return Ogre::Vector4(left, top, right, bottom);
	}

	Ogre::Vector4 MathHelper::getCalibratedOverlayCoordinates(Ogre::Real left, Ogre::Real top, Ogre::Real right, Ogre::Real bottom)
	{
		Ogre::Real transformLeft = left * 2.0f - 1.0f;
		Ogre::Real transformTop = 1.0f - top * 2.0f;
		Ogre::Real transformRight = right * 2.0f - 1.0f;
		Ogre::Real transformBottom = 1.0f - bottom * 2.0f;
		return Ogre::Vector4(transformLeft, transformTop, transformRight, transformBottom);
	}

	Ogre::Vector4 MathHelper::getCalibratedOverlayCoordinates(Ogre::Vector4 border)
	{
		return this->getCalibratedOverlayCoordinates(border.x, border.y, border.z, border.w);
	}

	Ogre::Matrix4 MathHelper::buildScaledOrthoMatrix(Ogre::Real left, Ogre::Real right, Ogre::Real bottom, Ogre::Real top, Ogre::Real nearClipDistance, Ogre::Real farClipDistance)
	{
		// https://forums.ogre3d.org/viewtopic.php?f=2&t=26244&start=0
		Ogre::Real invw = 1 / (right - left);
		Ogre::Real invh = 1 / (top - bottom);
		Ogre::Real invd = 1 / (farClipDistance - nearClipDistance);

		Ogre::Matrix4 proj = Ogre::Matrix4::ZERO;
		proj[0][0] = 2 * invw;
		proj[0][3] = -(right + left) * invw;
		proj[1][1] = 2 * invh;
		proj[1][3] = -(top + bottom) * invh;
		proj[2][2] = -2 * invd;
		proj[2][3] = -(farClipDistance + nearClipDistance) * invd;
		proj[3][3] = 1;

		return proj;
	}

	Ogre::Real MathHelper::lowPassFilter(Ogre::Real value, Ogre::Real oldValue, Ogre::Real scale)
	{
		return float((1.0f - scale) * oldValue + scale * value);
	}

	Ogre::Vector3 MathHelper::lowPassFilter(const Ogre::Vector3& value, const Ogre::Vector3& oldValue, Ogre::Real scale)
	{
		return ((1.0f - scale) * oldValue + scale * value);
	}

	Ogre::Real MathHelper::clamp(Ogre::Real value, Ogre::Real min, Ogre::Real max)
	{
		return std::max(min, std::min(max, value));
	}

	Ogre::Real MathHelper::smoothDamp(Ogre::Real current, Ogre::Real& target, Ogre::Real& currentVelocity, Ogre::Real& smoothTime, Ogre::Real maxSpeed, Ogre::Real deltaTime)
	{
		smoothTime = std::fmaxf(0.0001f, smoothTime);
		Ogre::Real num = 2.0f / smoothTime;
		Ogre::Real num2 = num * deltaTime;
		Ogre::Real num3 = 1.0f / (1.0f + num2 + 0.48f * num2 * num2 + 0.235f * num2 * num2 * num2);
		Ogre::Real num4 = current - target;
		Ogre::Real num5 = target;
		Ogre::Real num6 = maxSpeed * smoothTime;
		num4 = Ogre::Math::Clamp(num4, -num6, num6);
		target = current - num4;
		Ogre::Real num7 = (currentVelocity + num * num4) * deltaTime;
		currentVelocity = (currentVelocity - num * num7) * num3;
		Ogre::Real num8 = target + (num4 + num7) * num3;
		if (num5 - current > 0.0f == num8 > num5)
		{
			num8 = num5;
			currentVelocity = (num8 - num5) / deltaTime;
		}
		return num8;
	}

	Ogre::Vector3 MathHelper::smoothDamp(const Ogre::Vector3& current, Ogre::Vector3& target, Ogre::Vector3& currentVelocity, Ogre::Real& smoothTime, Ogre::Real maxSpeed, Ogre::Real deltaTime)
	{
		Ogre::Vector3 resultSmooth;
		resultSmooth.x = smoothDamp(current.x, target.x, currentVelocity.x, smoothTime, maxSpeed, deltaTime);
		resultSmooth.y = smoothDamp(current.y, target.y, currentVelocity.y, smoothTime, maxSpeed, deltaTime);
		resultSmooth.z = smoothDamp(current.z, target.z, currentVelocity.z, smoothTime, maxSpeed, deltaTime);

		return resultSmooth;
	}

	void MathHelper::mouseToViewPort(int mx, int my, Ogre::Real& x, Ogre::Real& y, Ogre::Window* renderWindow)
	{
		x = (Ogre::Real)((Ogre::Real)mx / (Ogre::Real)renderWindow->getWidth());
		y = (Ogre::Real)((Ogre::Real)my / (Ogre::Real)renderWindow->getHeight());
	}

	Ogre::Real MathHelper::round(Ogre::Real number, unsigned int places)
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

	Ogre::Real MathHelper::round(Ogre::Real number)
	{
		if (number == -0.0f)
			number = 0.0f;
		Ogre::Real roundedNumber = number;
		if (roundedNumber >= 0.0f)
		{
			roundedNumber = floorf(number + 0.5f);
		}
		else
		{
			roundedNumber = ceilf(number - 0.5f);
		}
		return roundedNumber;
	}

	Ogre::Vector2 MathHelper::round(Ogre::Vector2 vec, unsigned int places)
	{
		vec.x = round(vec.x, places);
		vec.y = round(vec.y, places);
		return vec;
	}

	Ogre::Vector3 MathHelper::round(Ogre::Vector3 vec, unsigned int places)
	{
		vec.x = round(vec.x, places);
		vec.y = round(vec.y, places);
		vec.z = round(vec.z, places);
		return vec;
	}

	Ogre::Vector4 MathHelper::round(Ogre::Vector4 vec, unsigned int places)
	{
		vec.x = round(vec.x, places);
		vec.y = round(vec.y, places);
		vec.z = round(vec.z, places);
		vec.w = round(vec.w, places);
		return vec;
	}

	Ogre::Quaternion MathHelper::diffPitch(Ogre::Quaternion dest, Ogre::Quaternion src)
	{
		Ogre::Quaternion quat = Ogre::Quaternion::ZERO;
		quat.FromAngleAxis(dest.getPitch() - src.getPitch(), Ogre::Vector3::UNIT_X);
		return quat;
	}

	Ogre::Quaternion MathHelper::diffYaw(Ogre::Quaternion dest, Ogre::Quaternion src)
	{
		Ogre::Quaternion quat = Ogre::Quaternion::ZERO;
		quat.FromAngleAxis(dest.getYaw() - src.getYaw(), Ogre::Vector3::UNIT_Y);
		return quat;
	}

	Ogre::Quaternion MathHelper::diffRoll(Ogre::Quaternion dest, Ogre::Quaternion src)
	{
		Ogre::Quaternion quat = Ogre::Quaternion::ZERO;
		quat.FromAngleAxis(dest.getRoll() - src.getRoll(), Ogre::Vector3::UNIT_Z);
		return quat;
	}

	Ogre::Quaternion MathHelper::diffDegree(Ogre::Quaternion dest, Ogre::Quaternion src, int mode)
	{
		Ogre::Quaternion quat = Ogre::Quaternion::ZERO;
		if (mode == 1)
		{
			quat.FromAngleAxis(dest.getPitch() - src.getPitch(), Ogre::Vector3::UNIT_X);
		}
		else if (mode == 2)
		{
			quat.FromAngleAxis(dest.getYaw() - src.getYaw(), Ogre::Vector3::UNIT_Y);
		}
		else if (mode == 3)
		{
			quat.FromAngleAxis(dest.getRoll() - src.getRoll(), Ogre::Vector3::UNIT_Z);
		}
		return quat;
	}

	Ogre::Vector3 MathHelper::extractEuler(const Ogre::Quaternion& quat)
	{
		Ogre::Radian yaw, pitch, roll;
		Ogre::Matrix3 kRot;
		quat.ToRotationMatrix(kRot);
		// ATTENTION: ZYX?? Why not: XYZ??
		kRot.ToEulerAnglesZYX(yaw, pitch, roll);

		return Ogre::Vector3(pitch.valueRadians(), yaw.valueRadians(), roll.valueRadians());
	}

	Ogre::Quaternion MathHelper::getOrientationFromHeadingPitch(const Ogre::Quaternion& orientation, Ogre::Real steeringAngle, Ogre::Real pitchAngle, const Ogre::Vector3& defaultModelDirection)
	{
		Ogre::Quaternion deltaYaw(Ogre::Radian(steeringAngle), defaultModelDirection);
		Ogre::Quaternion deltaPitch(Ogre::Radian(pitchAngle), Ogre::Vector3::UNIT_X);
		return deltaYaw * orientation * deltaPitch;
	}

	Ogre::Quaternion MathHelper::faceTarget(Ogre::SceneNode* source, Ogre::SceneNode* dest)
	{
		Ogre::Vector3 lookAt = dest->getPosition() - source->getPosition();
		Ogre::Vector3 axes[3];

		source->getOrientation().ToAxes(axes);
		Ogre::Quaternion rotQuat;

		if (lookAt == axes[2])
		{
			rotQuat.FromAngleAxis(Ogre::Radian(Ogre::Math::PI), axes[1]);
		}
		else
		{
			rotQuat = axes[2].getRotationTo(lookAt);
		}
		return rotQuat * source->getOrientation();
	}

	Ogre::Quaternion MathHelper::faceTarget(Ogre::SceneNode* source, Ogre::SceneNode* dest, const Ogre::Vector3& defaultDirection)
	{
		Ogre::Vector3 lookAt = dest->getPosition() - source->getPosition();
		return defaultDirection.getRotationTo(lookAt, Ogre::Vector3::ZERO);
	}

	Ogre::Quaternion MathHelper::faceDirection(Ogre::SceneNode* source, const Ogre::Vector3& direction)
	{
		Ogre::Vector3 axes[3];

		source->getOrientation().ToAxes(axes);
		Ogre::Quaternion rotQuat;

		if (direction == axes[2])
		{
			rotQuat.FromAngleAxis(Ogre::Radian(Ogre::Math::PI), axes[1]);
		}
		else
		{
			rotQuat = axes[2].getRotationTo(direction);
		}
		return rotQuat * source->getOrientation();
	}

	Ogre::Quaternion MathHelper::faceDirection(Ogre::SceneNode* source, const Ogre::Vector3& direction, const Ogre::Vector3& localDirectionVector)
	{
		Ogre::Quaternion currentOrient = source->getOrientation();

		// Get current local direction relative to world space
		Ogre::Vector3 currentDir = currentOrient * localDirectionVector;
		Ogre::Quaternion targetOrientation;
		if ((currentDir + direction).squaredLength() < 0.00005f)
		{
			// Oops, a 180 degree turn (infinite possible rotation axes)
			// Default to yaw i.e. use current UP
			targetOrientation = Ogre::Quaternion(-currentOrient.y, -currentOrient.z, currentOrient.w, currentOrient.x);
		}
		else
		{
			// Derive shortest arc to new direction
			Ogre::Quaternion rotQuat = currentDir.getRotationTo(direction);
			targetOrientation = rotQuat * currentOrient;
		}
		return std::move(targetOrientation);
	}

	Ogre::Quaternion MathHelper::faceDirection(const Ogre::Quaternion& sourceOrientation, const Ogre::Vector3& direction, const Ogre::Vector3& localDirectionVector)
	{
		// Get current local direction relative to world space
		Ogre::Vector3 currentDir = sourceOrientation * localDirectionVector;
		Ogre::Quaternion targetOrientation;
		if ((currentDir + direction).squaredLength() < 0.00005f)
		{
			// Oops, a 180 degree turn (infinite possible rotation axes)
			// Default to yaw i.e. use current UP
			targetOrientation = Ogre::Quaternion(-sourceOrientation.y, -sourceOrientation.z, sourceOrientation.w, sourceOrientation.x);
		}
		else
		{
			// Derive shortest arc to new direction
			Ogre::Quaternion rotQuat = currentDir.getRotationTo(direction);
			targetOrientation = rotQuat * sourceOrientation;
		}
		return std::move(targetOrientation);
	}

	Ogre::Quaternion MathHelper::faceDirectionSlerp(const Ogre::Quaternion& sourceOrientation, const Ogre::Vector3& direction, const Ogre::Vector3& defaultDirection, Ogre::Real dt, Ogre::Real rotationSpeed)
	{
		Ogre::Vector3 currentDirection = sourceOrientation * defaultDirection;
		Ogre::Quaternion destinationOrientation = currentDirection.getRotationTo(direction) * sourceOrientation;

		Ogre::Quaternion rotationBetween = destinationOrientation * sourceOrientation.Inverse();

		Ogre::Radian angle;
		Ogre::Vector3 axis;
		rotationBetween.ToAngleAxis(angle, axis);
		Ogre::Real angleDeg = angle.valueDegrees();

		Ogre::Real maxAngleThisFrame = rotationSpeed * dt;
		Ogre::Real ratio = 1.0f;
		if (angleDeg > maxAngleThisFrame)
		{
			ratio = maxAngleThisFrame / angle.valueDegrees();
		}
		return std::move(Ogre::Quaternion::Slerp(ratio, sourceOrientation, destinationOrientation, true));
	}

	Ogre::Quaternion MathHelper::lookAt(const Ogre::Vector3& unnormalizedTargetDirection, const Ogre::Vector3 fixedAxis)
	{
		Ogre::Quaternion resultOrientation;
		Ogre::Vector3 zAdjustVec = -unnormalizedTargetDirection;
		zAdjustVec.normalise();

		Ogre::Vector3 xVec = fixedAxis.crossProduct(zAdjustVec);
		xVec.normalise();

		Ogre::Vector3 yVec = zAdjustVec.crossProduct(xVec);
		yVec.normalise();

		resultOrientation.FromAxes(xVec, yVec, zAdjustVec);

		return std::move(resultOrientation);
	}

	Ogre::Radian MathHelper::getAngle(const Ogre::Vector3& dir1, const Ogre::Vector3& dir2, const Ogre::Vector3& norm, bool signedAngle)
	{
		Ogre::Real dot = dir1.dotProduct(dir2);
		dot = Ogre::Math::Clamp(dot, -0.9999f, 0.9999f);
		Ogre::Real angle = Ogre::Math::ACos(dot).valueRadians();

		if (signedAngle)
		{
			assert(!norm.isZeroLength());
			Ogre::Vector3 cross = dir1.crossProduct(dir2);
			Ogre::Real sign = norm.dotProduct(cross);
			if (sign < 0.0f)
			{
				angle *= -1.0f;
			}
		}

		return Ogre::Radian(angle);
	}

	Ogre::Real MathHelper::normalizeDegreeAngle(Ogre::Real degree)
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

	Ogre::Real MathHelper::normalizeRadianAngle(Ogre::Real radian)
	{
		Ogre::Real normalizedDegree = normalizeDegreeAngle(((radian * 360.0f) / Ogre::Math::TWO_PI));
		return ((normalizedDegree * Ogre::Math::TWO_PI) / 360.0f);
	}

	bool MathHelper::degreeAngleEquals(Ogre::Real degree0, Ogre::Real degree1)
	{
		Ogre::Real angle0 = normalizeDegreeAngle(degree0);
		Ogre::Real angle1 = normalizeDegreeAngle(degree1);
		if (Ogre::Math::RealEqual(angle0, 360.0f))
		{
			angle0 = 0.0f;
		}
		if (Ogre::Math::RealEqual(angle1, 360.0f))
		{
			angle1 = 0.0f;
		}
		return Ogre::Math::RealEqual(angle0, angle1);
	}

	bool MathHelper::radianAngleEquals(Ogre::Real radian0, Ogre::Real radian1)
	{
		Ogre::Real angle0 = normalizeRadianAngle(radian0);
		Ogre::Real angle1 = normalizeRadianAngle(radian1);
		if (Ogre::Math::RealEqual(angle0, Ogre::Math::TWO_PI))
		{
			angle0 = 0.0;
		}
		if (Ogre::Math::RealEqual(angle1, Ogre::Math::TWO_PI))
		{
			angle1 = 0.0;
		}
		return Ogre::Math::RealEqual(angle0, angle1);
	}

	bool MathHelper::realEquals(Ogre::Real first, Ogre::Real second, Ogre::Real tolerance)
	{
		return Ogre::Math::RealEqual(first, second, tolerance);
	}

	bool MathHelper::vector2Equals(const Ogre::Vector2& first, const Ogre::Vector2& second, Ogre::Real tolerance)
	{
		return Ogre::Math::RealEqual(first.x, second.x, tolerance) && Ogre::Math::RealEqual(first.y, second.y, tolerance);
	}

	bool MathHelper::vector3Equals(const Ogre::Vector3& first, const Ogre::Vector3& second, Ogre::Real tolerance)
	{
		return Ogre::Math::RealEqual(first.x, second.x, tolerance) && Ogre::Math::RealEqual(first.y, second.y, tolerance)
			&& Ogre::Math::RealEqual(first.z, second.z, tolerance);
	}

	bool MathHelper::vector4Equals(const Ogre::Vector4& first, const Ogre::Vector4& second, Ogre::Real tolerance)
	{
		return Ogre::Math::RealEqual(first.x, second.x, tolerance) && Ogre::Math::RealEqual(first.y, second.y, tolerance)
			&& Ogre::Math::RealEqual(first.z, second.z, tolerance) && Ogre::Math::RealEqual(first.w, second.w, tolerance);
	}

	bool MathHelper::quaternionEquals(const Ogre::Quaternion& first, const Ogre::Quaternion& second, Ogre::Real tolerance)
	{
		return Ogre::Math::RealEqual(first.x, second.x, tolerance) && Ogre::Math::RealEqual(first.y, second.y, tolerance)
			&& Ogre::Math::RealEqual(first.z, second.z, tolerance) && Ogre::Math::RealEqual(first.w, second.w, tolerance);
	}

	Ogre::Real MathHelper::degreeAngleToRadian(Ogre::Real degree)
	{
		return ((normalizeDegreeAngle(degree) * Ogre::Math::TWO_PI) / 360.0f);
	}

	Ogre::Real MathHelper::radianAngleToDegree(Ogre::Real radian)
	{
		return normalizeRadianAngle((radian * 360.0f) / Ogre::Math::TWO_PI);
	}

	Ogre::Vector4 MathHelper::quatToDegreeAngleAxis(const Ogre::Quaternion& quat)
	{
		Ogre::Vector3 orientationValues;
		Ogre::Radian rad;
		quat.ToAngleAxis(rad, orientationValues);
		Ogre::Real degree = rad.valueDegrees();
		// w = degree
		return std::move(Ogre::Vector4(degree, orientationValues.x, orientationValues.y, orientationValues.z));
	}

	Ogre::Vector3 MathHelper::quatToDegrees(const Ogre::Quaternion& quat)
	{
		Ogre::Matrix3 mat;
		quat.ToRotationMatrix(mat);
		Ogre::Radian pRad;
		Ogre::Radian yRad;
		Ogre::Radian rRad;
		mat.ToEulerAnglesYXZ(yRad, pRad, rRad);
		if (true == this->realEquals(-0.0f, pRad.valueDegrees()))
		{
			pRad = 0.0f;
		}
		if (true == this->realEquals(-0.0f, yRad.valueDegrees()))
		{
			yRad = 0.0f;
		}
		if (true == this->realEquals(-0.0f, rRad.valueDegrees()))
		{
			rRad = 0.0f;
		}
		return std::move(Ogre::Vector3(pRad.valueDegrees(), yRad.valueDegrees(), rRad.valueDegrees()));
	}

	Ogre::Vector3 MathHelper::quatToDegreesRounded(const Ogre::Quaternion& quat)
	{
		Ogre::Matrix3 mat;
		quat.ToRotationMatrix(mat);
		Ogre::Radian pRad;
		Ogre::Radian yRad;
		Ogre::Radian rRad;
		mat.ToEulerAnglesYXZ(yRad, pRad, rRad);
		if (true == this->realEquals(-0.0f, pRad.valueDegrees()))
		{
			pRad = 0.0f;
		}
		if (true == this->realEquals(-0.0f, yRad.valueDegrees()))
		{
			yRad = 0.0f;
		}
		if (true == this->realEquals(-0.0f, rRad.valueDegrees()))
		{
			rRad = 0.0f;
		}

		Ogre::Vector3 rotationValue = Ogre::Vector3(MathHelper::getInstance()->round(pRad.valueDegrees()), MathHelper::getInstance()->round(yRad.valueDegrees()), MathHelper::getInstance()->round(rRad.valueDegrees()));
		return std::move(rotationValue);
	}

	Ogre::Quaternion MathHelper::degreesToQuat(const Ogre::Vector3& degrees)
	{
		Ogre::Degree degX(degrees.x);
		Ogre::Degree degY(degrees.y);
		Ogre::Degree degZ(degrees.z);
		/*if (true == this->realEquals(-0.0f, degX.valueDegrees()))
		{
			degX = 0.0f;
		}
		if (true == this->realEquals(-0.0f, degY.valueDegrees()))
		{
			degY = 0.0f;
		}
		if (true == this->realEquals(-0.0f, degZ.valueDegrees()))
		{
			degZ = 0.0f;
		}*/
		
		Ogre::Radian yRad(degY);
		Ogre::Radian pRad(degX);
		Ogre::Radian rRad(degZ);

		Ogre::Matrix3 mat;

		mat.FromEulerAnglesYXZ(yRad, pRad, rRad);
		Ogre::Quaternion quat;
		quat.FromRotationMatrix(mat);
		return std::move(quat);
	}

	Ogre::Radian MathHelper::getAngleForTargetLocation(const Ogre::Vector3& currentLocation, const Ogre::Quaternion& currentOrientation, const Ogre::Vector3& targetLocation, const Ogre::Vector3& defaultOrientation)
	{
		Ogre::Vector3 goalHeading = targetLocation - currentLocation;
		goalHeading.y = 0.0f;
		goalHeading.normalise();
		Ogre::Vector3 curHeading = currentOrientation * defaultOrientation;
		// curHeading.normalise();

		Ogre::Radian deltaAngle = Ogre::Radian(Ogre::Math::ACos(curHeading.dotProduct(goalHeading)));

		// Calculate the cross product to know if the entity is turning left or right 
		Ogre::Vector3 rotationAxis = curHeading.crossProduct(goalHeading).normalisedCopy();

		if (rotationAxis.y < 0.0f)
			deltaAngle = -deltaAngle;
		//----------------------------------------------------------------------------------------------
		return deltaAngle;
	}

	Ogre::Quaternion MathHelper::computeQuaternion(const Ogre::Vector3& direction, const Ogre::Vector3& upVector)
	{
		Ogre::Quaternion q;
		Ogre::Vector3 zVec = direction;
		zVec.normalise();
		Ogre::Vector3 xVec = upVector.crossProduct(zVec);
		if (xVec.isZeroLength())
		{
			xVec = Ogre::Vector3::UNIT_X;
		}
		xVec.normalise();
		Ogre::Vector3 yVec = zVec.crossProduct(xVec);
		yVec.normalise();
		q.FromAxes(xVec, yVec, zVec);
		return q;
	}

	Ogre::Vector2 MathHelper::rotateVector2(const Ogre::Vector2& in, const Ogre::Radian& angle)
	{
		return Ogre::Vector2(in.x * Ogre::Math::Cos(angle) - in.y * Ogre::Math::Sin(angle),
							 in.x * Ogre::Math::Sin(angle) + in.y * Ogre::Math::Cos(angle));
	}

	Ogre::Vector3 MathHelper::min(const Ogre::Vector3& v1, const Ogre::Vector3& v2)
	{
		return Ogre::Vector3(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z));
	}

	Ogre::Vector3 MathHelper::max(const Ogre::Vector3& v1, const Ogre::Vector3& v2)
	{
		return Ogre::Vector3(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z));
	}

	Ogre::Vector2 MathHelper::min(const Ogre::Vector2& v1, const Ogre::Vector2& v2)
	{
		return Ogre::Vector2(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
	}

	Ogre::Vector2 MathHelper::max(const Ogre::Vector2& v1, const Ogre::Vector2& v2)
	{
		return Ogre::Vector2(std::max(v1.x, v2.x), std::max(v1.y, v2.y));
	}

	Ogre::Degree MathHelper::getRandomAngle()
	{
		return Ogre::Degree(static_cast<int>(rand() % 360));
	}

	Ogre::Vector3 MathHelper::getRandomVector()
	{
		auto getRandomReal = [&]() -> Ogre::Real
		{
			Ogre::Real r = static_cast<Ogre::Real>(rand()) / static_cast<Ogre::Real>(RAND_MAX);
			return r;
		};

		Ogre::Vector3 rndVector(getRandomReal(), getRandomReal(), getRandomReal());
		rndVector.normalise();
		return rndVector;
	}

	Ogre::Quaternion MathHelper::getRandomDirection()
	{
		Ogre::Vector3 axis = getRandomVector();
		Ogre::Quaternion quat(getRandomAngle(), axis);
		return quat;
	}

	Ogre::Vector3 MathHelper::addRandomVectorOffset(const Ogre::Vector3& pos, Ogre::Real offset)
	{
		Ogre::Vector3 v = getRandomVector();
		v *= offset;
		return pos + v;
	}

	Ogre::Vector3 MathHelper::calculateGridValue(Ogre::Real step, const Ogre::Vector3& sourcePosition)
	{
		Ogre::Vector3 value = Ogre::Vector3::ZERO;
		if (0.0f == step)
		{
			return std::move(sourcePosition);
		}
		//calculate the gridvalue
		//only move the object if it passes the critical distance
		//do it without the y coordinate because its for placing an object and would cause that the objects gains high
		value.x = ((round(sourcePosition.x / step)) * step);
		value.y = ((round(sourcePosition.y / step)) * step);
		value.z = ((round(sourcePosition.z / step)) * step);

		return std::move(value);
	}

	Ogre::Vector3 MathHelper::calculateGridTranslation(const Ogre::Vector3& gridFactor, const Ogre::Vector3& sourcePosition, Ogre::MovableObject* movableObject)
	{
		/*
		The moveEntity method handles transforming the point from world space to local space and back, considering the entity's rotation.
		The grid sizes for the x and z axes are used to align the entity to the grid correctly, even when it has different dimensions along these axes.
		The entity's bounding box size is used to determine the grid sizes for snapping.
		*/

		// Transform the world point to the entity's local space
		Ogre::Vector3 localPoint = movableObject->getParentNode()->_getDerivedOrientationUpdated().Inverse() * (sourcePosition - movableObject->getParentNode()->_getDerivedPositionUpdated());

		// Round the local point to the nearest grid size, considering different sizes for x and z axes
		localPoint.x = round(localPoint.x / gridFactor.x) * gridFactor.x;
		localPoint.y = round(localPoint.y / gridFactor.y) * gridFactor.y;
		localPoint.z = round(localPoint.z / gridFactor.z) * gridFactor.z;

		// Transform back to world space
		Ogre::Vector3 newPoint = movableObject->getParentNode()->_getDerivedOrientationUpdated() * localPoint + movableObject->getParentNode()->_getDerivedPositionUpdated();

		return newPoint;
	}

	int MathHelper::wrappedModulo(int n, int cap)
	{
		if (n >= 0)
		{
			return n % cap;
		}
		return (cap - 1) - ((1 + n) % cap);
	}

	Ogre::Real MathHelper::calculateRotationGridValue(Ogre::Real step, Ogre::Real angle)
	{
		if (0.0f == step)
		{
			return angle;
		}

		Ogre::Real degree = Ogre::Math::RadiansToDegrees(angle);
		return Ogre::Math::DegreesToRadians(((round(degree / step)) * step));
	}

	Ogre::Real MathHelper::calculateRotationGridValue(const Ogre::Quaternion& orientation, Ogre::Real step, Ogre::Real angle)
	{
		if (0.0f == step)
		{
			return angle;
		}

		Ogre::Vector3 direction = orientation.yAxis();
		Ogre::Real value = Ogre::Math::ATan2(direction.z, direction.x).valueDegrees();

		return std::fmod((round(value / (2.0f * Ogre::Math::PI / step)) + step), step);
	}

	Ogre::Vector3 MathHelper::getBottomCenterOfMesh(Ogre::SceneNode* sceneNode, Ogre::MovableObject* movableObject) const
	{
		Ogre::Aabb boundingBox = movableObject->getLocalAabb();
		Ogre::Vector3 maximumScaled = boundingBox.getMaximum() * sceneNode->getScale();
		Ogre::Vector3 minimumScaled = boundingBox.getMinimum() * sceneNode->getScale();
		// Do not use padding, as all objects will be a bit above the ground!
		// Ogre::Vector3 padding = (maximumScaled - minimumScaled) * Ogre::v1::MeshManager::getSingleton().getBoundsPaddingFactor();
		Ogre::Vector3 size = ((maximumScaled - minimumScaled) /*- padding * 2.0f*/);
		Ogre::Vector3 centerOffset = minimumScaled + /*padding +*/ (size / 2.0f);
		// Problem here: when physics is activated an object may slithly go up or down either to prevent collision with underlying object or because of gravity
		// But attention: stack mode must be used, if there is an object below, because it also has an height! Else the y position is 0
		Ogre::Real lowestObjectY = minimumScaled.y /*- (padding.y * 2.0f)*/;
		centerOffset.y = 0.0f - lowestObjectY;
		return centerOffset;
	}

	void MathHelper::getMeshInformationWithSkeleton(Ogre::v1::Entity* entity, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices,
		const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale)
	{
		bool addedShared = false;
		size_t currentOffset = 0;
		size_t sharedOffset = 0;
		size_t nextOffset = 0;
		size_t indexOffset = 0;
		vertexCount = indexCount = 0;

		Ogre::v1::MeshPtr mesh = entity->getMesh();


		bool useSoftwareBlendingVertices = entity->hasSkeleton();

		if (useSoftwareBlendingVertices)
		{
			entity->_updateAnimation();
		}

		// Calculate how many vertices and indices we're going to need
		for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
		{
			Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);

			// We only need to add the shared vertices once
			if (subMesh->useSharedVertices)
			{
				if (!addedShared)
				{
					vertexCount += mesh->sharedVertexData[0]->vertexCount;
					addedShared = true;
				}
			}
			else
			{
				vertexCount += subMesh->vertexData[0]->vertexCount;
			}

			// Add the indices
			indexCount += subMesh->indexData[0]->indexCount;
		}


		// Allocate space for the vertices and indices
		vertices = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
		indices = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);

		addedShared = false;

		// Run through the submeshes again, adding the data into the arrays
		for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
		{
			Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);

			//----------------------------------------------------------------
			// GET VERTEXDATA
			//----------------------------------------------------------------

			//Ogre::VertexData* vertex_data = subMesh->useSharedVertices ? mesh->sharedVertexData : subMesh->vertexData;
			Ogre::v1::VertexData* vertex_data;

			//When there is animation:
			if (useSoftwareBlendingVertices)
			{
				vertex_data = subMesh->useSharedVertices ? entity->_getSkelAnimVertexData() : entity->getSubEntity(i)->_getSkelAnimVertexData();
			}
			else
			{
				vertex_data = subMesh->useSharedVertices ? mesh->sharedVertexData[0] : subMesh->vertexData[0];
			}

			if (!subMesh->useSharedVertices || addedShared)
			{
				if (subMesh->useSharedVertices)
				{
					addedShared = true;
					sharedOffset = currentOffset;
				}

				const Ogre::v1::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

				Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

				unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
				//  as second argument. So make it float, to avoid trouble when Ogre::Real will
				//  be comiled/typedefed as double:
				//      Ogre::Real* pReal;
				float* pReal;

				for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
				{
					posElem->baseVertexPointerToElement(vertex, &pReal);

					Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

					vertices[currentOffset + j] = (orient * (pt * scale)) + position;
				}

				vbuf->unlock();
				nextOffset += vertex_data->vertexCount;
			}


			Ogre::v1::IndexData* indexData = subMesh->indexData[0];
			size_t numTris = indexData->indexCount / 3;
			Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;

			bool use32bitindexes = (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);

			unsigned long* pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
			unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);


			size_t offset = (subMesh->useSharedVertices) ? sharedOffset : currentOffset;
			size_t index_start = indexData->indexStart;
			size_t last_index = numTris * 3 + index_start;

			if (use32bitindexes)
			{
				for (size_t k = index_start; k < last_index; ++k)
				{
					indices[indexOffset++] = pLong[k] + static_cast<unsigned long>(offset);
				}
			}
			else
			{
				for (size_t k = index_start; k < last_index; ++k)
				{
					indices[indexOffset++] = static_cast<unsigned long>(pShort[k]) +
						static_cast<unsigned long>(offset);
				}
			}
			ibuf->unlock();
			currentOffset = nextOffset;
		}
	}

	void MathHelper::getMeshInformation(const Ogre::v1::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertexBuffer, size_t& indexCount, unsigned long*& indexBuffer,
		const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale)
	{
		bool addedShared = false;
		size_t currentOffset = 0;
		size_t sharedOffset = 0;
		size_t nextOffset = 0;
		size_t indexOffset = 0;
		unsigned int vertexBufferSize = 0;
		unsigned int indexBufferSize = 0;

		vertexCount = indexCount = 0;

		// Calculate how many vertices and indices we're going to need
		for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
		{
			Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);

			// We only need to add the shared vertices once
			if (subMesh->useSharedVertices)
			{
				if (!addedShared)
				{
					vertexCount += mesh->sharedVertexData[0]->vertexCount;
					addedShared = true;
				}
			}
			else
			{
				vertexCount += subMesh->vertexData[0]->vertexCount;
			}

			// Add the indices
			indexCount += subMesh->indexData[0]->indexCount;
		}


		if (vertexCount > vertexBufferSize)
		{
			vertexBuffer = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
			vertexBufferSize = static_cast<unsigned int>(vertexCount);
		}

		if (indexCount > indexBufferSize)
		{
			indexBuffer = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);
			indexBufferSize = static_cast<unsigned int>(indexCount);
		}

		addedShared = false;

		// Run through the submeshes again, adding the data into the arrays
		for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
		{
			Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);

			Ogre::v1::VertexData* vertexData = subMesh->useSharedVertices ? mesh->sharedVertexData[0] : subMesh->vertexData[0];

			if ((!subMesh->useSharedVertices) || (subMesh->useSharedVertices && !addedShared))
			{
				if (subMesh->useSharedVertices)
				{
					addedShared = true;
					sharedOffset = currentOffset;
				}

				const Ogre::v1::VertexElement* posElem = vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

				Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(posElem->getSource());

				unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
				//  as second argument. So make it float, to avoid trouble when Ogre::Real will
				//  be compiled/typedef-ed as double:
				//      Ogre::Real* pReal;
				float* pReal;

				for (size_t j = 0; j < vertexData->vertexCount; ++j, vertex += vbuf->getVertexSize())
				{
					posElem->baseVertexPointerToElement(vertex, &pReal);

					Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

					vertexBuffer[currentOffset + j] = (orientation * (pt * scale)) + position;
				}

				vbuf->unlock();
				nextOffset += vertexData->vertexCount;
			}


			Ogre::v1::IndexData* indexData = subMesh->indexData[0];
			size_t numTris = indexData->indexCount / 3;
			Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;

			bool use32bitindexes = (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);

			unsigned long* pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
			unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

			size_t offset = (subMesh->useSharedVertices) ? sharedOffset : currentOffset;

			if (use32bitindexes)
			{
				for (size_t k = 0; k < numTris * 3; ++k)
				{
					indexBuffer[indexOffset++] = pLong[k] + static_cast<unsigned long>(offset);
				}
			}
			else
			{
				for (size_t k = 0; k < numTris * 3; ++k)
				{
					indexBuffer[indexOffset++] = static_cast<unsigned long>(pShort[k]) + static_cast<unsigned long>(offset);
				}
			}

			ibuf->unlock();
			currentOffset = nextOffset;
		}
		indexCount = indexOffset;
	}

	void MathHelper::getMeshInformation2(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices,
		const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale)
	{
		//First, we compute the total number of vertices and indices and init the buffers.
		unsigned int numVertices = 0;
		unsigned int numIndices = 0;
		//MeshInfo outInfo; 

		Ogre::Mesh::SubMeshVec::const_iterator subMeshIterator = mesh->getSubMeshes().begin();

		while (subMeshIterator != mesh->getSubMeshes().end())
		{
			Ogre::SubMesh* subMesh = *subMeshIterator;
			numVertices += static_cast<unsigned int>(subMesh->mVao[0][0]->getVertexBuffers()[0]->getNumElements());
			numIndices += static_cast<unsigned int>(subMesh->mVao[0][0]->getIndexBuffer()->getNumElements());

			subMeshIterator++;
		}

		//allocate memory to the input array reference, handleRequest calls delete on this memory
		vertices = OGRE_ALLOC_T(Ogre::Vector3, numVertices, Ogre::MEMCATEGORY_GEOMETRY);
		indices = OGRE_ALLOC_T(unsigned long, numIndices, Ogre::MEMCATEGORY_GEOMETRY);

		vertexCount = numVertices; //used later in handleRequest.
		indexCount = numIndices;

		unsigned int addedVertices = 0;
		unsigned int addedIndices = 0;

		unsigned int index_offset = 0;
		unsigned int subMeshOffset = 0;

		/*
		Read submeshes
		*/
		subMeshIterator = mesh->getSubMeshes().begin();
		while (subMeshIterator != mesh->getSubMeshes().end())
		{
			Ogre::SubMesh* subMesh = *subMeshIterator;
			Ogre::VertexArrayObjectArray vaos = subMesh->mVao[0];

			if (!vaos.empty())
			{
				//Get the first LOD level 
				Ogre::VertexArrayObject* vao = vaos[0];
				bool indices32 = (vao->getIndexBuffer()->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);

				const Ogre::VertexBufferPackedVec& vertexBuffers = vao->getVertexBuffers();
				Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();

				//request async read from buffer 
				Ogre::VertexArrayObject::ReadRequestsVec requests;
				requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));

				vao->readRequests(requests);
				vao->mapAsyncTickets(requests);
				unsigned int subMeshVerticiesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());
				if (requests[0].type == Ogre::VET_HALF4)
				{
					for (size_t i = 0; i < subMeshVerticiesNum; ++i)
					{
						const Ogre::uint16* pos = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
						Ogre::Vector3 vec;
						vec.x = Ogre::Bitwise::halfToFloat(pos[0]);
						vec.y = Ogre::Bitwise::halfToFloat(pos[1]);
						vec.z = Ogre::Bitwise::halfToFloat(pos[2]);

						requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
						vertices[i + subMeshOffset] = (orientation * (vec * scale)) + position;
					}
				}
				else if (requests[0].type == Ogre::VET_FLOAT3)
				{
					for (size_t i = 0; i < subMeshVerticiesNum; ++i)
					{
						const float* pos = reinterpret_cast<const float*>(requests[0].data);
						Ogre::Vector3 vec;
						vec.x = *pos++;
						vec.y = *pos++;
						vec.z = *pos++;
						requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
						vertices[i + subMeshOffset] = (orientation * (vec * scale)) + position;
					}
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage("[MathHelper]: Vertex Buffer type not recognised");
					throw Ogre::Exception(0, "[MathHelper]: Vertex Buffer type not recognised", "getMeshInformation");
				}
				subMeshOffset += subMeshVerticiesNum;
				vao->unmapAsyncTickets(requests);

				////Read index data
				if (indexBuffer)
				{
					Ogre::AsyncTicketPtr asyncTicket = indexBuffer->readRequest(0, indexBuffer->getNumElements());

					unsigned int* pIndices = 0;
					if (indices32)
					{
						pIndices = (unsigned*)(asyncTicket->map());
					}
					else
					{
						unsigned short* pShortIndices = (unsigned short*)(asyncTicket->map());
						pIndices = new unsigned int[indexBuffer->getNumElements()];
						for (size_t k = 0; k < indexBuffer->getNumElements(); k++)
						{
							pIndices[k] = static_cast<unsigned int>(pShortIndices[k]);
						}
					}
					unsigned int bufferIndex = 0;

					for (size_t i = addedIndices; i < addedIndices + indexBuffer->getNumElements(); i++)
					{
						indices[i] = pIndices[bufferIndex] + index_offset;
						bufferIndex++;
					}
					addedIndices += static_cast<unsigned int>(indexBuffer->getNumElements());

					if (!indices32)
						delete[] pIndices;

					asyncTicket->unmap();
				}
				index_offset += static_cast<unsigned int>(vertexBuffers[0]->getNumElements());
			}
			subMeshIterator++;
		}
	}

	void MathHelper::getManualMeshInformation(const Ogre::v1::ManualObject* manualObject, size_t& vertexCount, Ogre::Vector3*& vertices,
		size_t& indexCount, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale)
	{
		std::vector<Ogre::Vector3> returnVertices;
		std::vector<unsigned long> returnIndices;
		unsigned long thisSectionStart = 0;
		for (unsigned int i = 0; i < manualObject->getNumSections(); i++)
		{
			Ogre::v1::ManualObject::ManualObjectSection* section = manualObject->getSection(i);
			Ogre::v1::RenderOperation* renderOp = section->getRenderOperation();
			std::vector<Ogre::Vector3> pushVertices;
			//Collect the vertices
			{
				const Ogre::v1::VertexElement* vertexElement = renderOp->vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				Ogre::v1::HardwareVertexBufferSharedPtr vertexBuffer = renderOp->vertexData->vertexBufferBinding->getBuffer(vertexElement->getSource());

				char* verticesBuffer = (char*)vertexBuffer->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);
				float* positionArrayHolder;

				thisSectionStart = static_cast<unsigned int>(pushVertices.size());

				pushVertices.reserve(renderOp->vertexData->vertexCount);

				for (unsigned int j = 0; j < renderOp->vertexData->vertexCount; j++)
				{
					vertexElement->baseVertexPointerToElement(verticesBuffer + j * vertexBuffer->getVertexSize(), &positionArrayHolder);
					Ogre::Vector3 vertexPos = Ogre::Vector3(positionArrayHolder[0],
						positionArrayHolder[1],
						positionArrayHolder[2]);

					vertexPos = (orient * (vertexPos * scale)) + position;

					pushVertices.push_back(vertexPos);
				}

				vertexBuffer->unlock();
			}
			//Collect the indices
			{
				if (renderOp->useIndexes)
				{
					Ogre::v1::HardwareIndexBufferSharedPtr indexBuffer = renderOp->indexData->indexBuffer;

					if (indexBuffer.isNull() || renderOp->operationType != Ogre::OperationType::OT_TRIANGLE_LIST)
					{
						//No triangles here, so we just drop the collected vertices and move along to the next section.
						continue;
					}
					else
					{
						returnVertices.reserve(returnVertices.size() + pushVertices.size());
						returnVertices.insert(returnVertices.end(), pushVertices.begin(), pushVertices.end());
					}

					unsigned int* pLong = (unsigned int*)indexBuffer->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);
					unsigned short* pShort = (unsigned short*)pLong;

					returnIndices.reserve(returnIndices.size() + renderOp->indexData->indexCount);

					for (size_t j = 0; j < renderOp->indexData->indexCount; j++)
					{
						unsigned long index;
						//We also have got to remember that for a multi section object, each section has
						//different vertices, so the indices will not be correct. To correct this, we
						//have to add the position of the first vertex in this section to the index

						//(At least I think so...)
						if (indexBuffer->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT)
							index = (unsigned long)pLong[j] + thisSectionStart;
						else
							index = (unsigned long)pShort[j] + thisSectionStart;

						returnIndices.push_back(index);
					}

					indexBuffer->unlock();
				}
			}
		}

		//Now we simply return the data.
		indexCount = returnIndices.size();
		vertexCount = returnVertices.size();
		vertices = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
		indices = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);
		for (unsigned long i = 0; i < vertexCount; i++)
		{
			vertices[i] = returnVertices[i];
		}
		for (unsigned long i = 0; i < indexCount; i++)
		{
			indices[i] = returnIndices[i];
		}
	}

	void MathHelper::getManualMeshInformation2(const Ogre::ManualObject* manualObject, size_t& vertexCount, Ogre::Vector3*& vertices,
		size_t& indexCount, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale)
	{
		std::vector<Ogre::Vector3> returnVertices;
		std::vector<unsigned long> returnIndices;
		unsigned long thisSectionStart = 0;

		Ogre::MeshPtr meshPtr = Ogre::MeshManager::getSingleton().createManual("Dummy", "General");

		// Create sub mesh from section
		for (unsigned int i = 0; i < manualObject->getNumSections(); i++)
		{
			Ogre::SubMesh* subMesh = meshPtr->createSubMesh();
			Ogre::ManualObject::ManualObjectSection* section = manualObject->getSection(i);

			//Each Vao pushed to the vector refers to an LOD level.
			subMesh->mVao[0].push_back(section->getVaos(Ogre::VertexPass::VpNormal)[0]); //main buffer
			subMesh->mVao[1].push_back(section->getVaos(Ogre::VertexPass::VpShadow)[0]); //shadow buffer
		}

		// get the mesh information
		this->getMeshInformation2(meshPtr, vertexCount, vertices,
			indexCount, indices, manualObject->getParentNode()->_getDerivedPositionUpdated(),
			manualObject->getParentNode()->_getDerivedOrientationUpdated(),
			manualObject->getParentNode()->getScale());

		// Remove the mesh, because its no more required
		Ogre::MeshManager::getSingleton().remove(meshPtr->getHandle());
	}

	bool MathHelper::getRaycastFromPoint(Ogre::RaySceneQuery* raySceneQuery, Ogre::Camera* camera, Ogre::Vector3& resultPostionOnModel, std::vector<Ogre::MovableObject*>* excludeMovableObjects)
	{
		// check we are initialised
		if (raySceneQuery != nullptr)
		{
			// execute the query, returns a vector of hits
			if (raySceneQuery->execute().size() <= 0)
			{
				// raycast did not hit an objects bounding box
				return false;
			}
		}
		else
		{
			// LOG_ERROR << "Cannot raycast without RaySceneQuery instance" << ENDLOG;
			return false;
		}

		// at this point we have raycast to a series of different objects bounding boxes.
		// we need to test these different objects to see which is the first polygon hit.
		// there are some minor optimizations (distance based) that mean we wont have to
		// check all of the objects most of the time, but the worst case scenario is that
		// we need to test every triangle of every object.
		Ogre::Real closestDistance = -1.0f;
		Ogre::Vector3 closestResult;
		Ogre::RaySceneQueryResult& queryResult = raySceneQuery->getLastResults();

		for (size_t qrIdx = 0; qrIdx < queryResult.size(); qrIdx++)
		{
			Ogre::String type = queryResult[qrIdx].movable->getMovableType();
			if ("Camera" == type)
				continue;

			// Stop checking if we have found a raycast hit that is closer
			// Than all remaining entities
			if (closestDistance >= 0.0f && closestDistance < queryResult[qrIdx].distance)
			{
				break;
			}

			// only check this result if its a hit against an entity
			if (type.compare("Entity") == 0)
			{
				// Get the entity to check
				Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(queryResult[qrIdx].movable);
				bool foundExcludedOne = false;
				// If its the exclude entity, continue the loop with a different ones
				if (nullptr != excludeMovableObjects)
				{
					for (size_t i = 0; i < excludeMovableObjects->size(); i++)
					{
						if (entity == excludeMovableObjects->at(i))
						{
							foundExcludedOne = true;
							continue;
						}
					}

					if (true == foundExcludedOne)
					{
						continue;
					}
				}

				// mesh data to retrieve         
				size_t vertexCount;
				size_t indexCount;
				Ogre::Vector3* vertices = nullptr;
				unsigned long* indices = nullptr;

				// get the mesh information
				this->getMeshInformation(entity->getMesh(), vertexCount, vertices, indexCount, indices,
					entity->getParentNode()->_getDerivedPositionUpdated(),
					entity->getParentNode()->_getDerivedOrientationUpdated(),
					entity->getParentNode()->_getDerivedScaleUpdated());

				// test for hitting individual triangles on the mesh
				bool newClosestFound = false;
				for (int i = 0; i < static_cast<int>(indexCount); i += 3)
				{
					// check for a hit against this triangle
					std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(raySceneQuery->getRay(),
						vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

					// if it was a hit check if its the closest
					if (hit.first)
					{
						if ((closestDistance < 0.0f) || (hit.second < closestDistance))
						{
							// this is the closest so far, save it off
							closestDistance = hit.second;
							newClosestFound = true;
						}
					}
				}

				// free the verticies and indicies memory
				OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
				OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

				// if we found a new closest raycast for this object, update the
				// closestResult before moving on to the next object.
				if (newClosestFound)
				{
					closestResult = raySceneQuery->getRay().getPoint(closestDistance);
					raySceneQuery->clearResults();
				}
			}
		}

		// return the result
		if (closestDistance >= 0.0f)
		{
			// raycast success
			resultPostionOnModel = closestResult;
			return (true);
		}
		else
		{
			// raycast failed
			return (false);
		}
	}

	bool MathHelper::getRaycastFromPoint(int mouseX, int mouseY, Ogre::Camera* camera, Ogre::Window* renderWindow, Ogre::RaySceneQuery* raySceneQuery,
		Ogre::Vector3& resultPositionOnModel, std::vector<Ogre::MovableObject*>* excludeMovableObjects)
	{
		Ogre::Real resultX;
		Ogre::Real resultY;
		this->mouseToViewPort(mouseX, mouseY, resultX, resultY, renderWindow);
		Ogre::Ray ray = camera->getCameraToViewportRay(resultX, resultY);
		raySceneQuery->setRay(ray);
		return this->getRaycastFromPoint(raySceneQuery, camera, resultPositionOnModel, excludeMovableObjects);
	}

	bool MathHelper::getRaycastHeight(Ogre::Real x, Ogre::Real z, Ogre::RaySceneQuery* raySceneQuery, Ogre::Real& height, std::vector<Ogre::MovableObject*>* excludeMovableObjects)
	{
		Ogre::Vector3 result = Ogre::Vector3::ZERO;
		Ogre::Ray verticalRay = Ogre::Ray(Ogre::Vector3(x, 5000.0f, z), Ogre::Vector3::NEGATIVE_UNIT_Y);
		raySceneQuery->setRay(verticalRay);

		// execute the query, returns a vector of hits
		if (raySceneQuery->execute().size() <= 0)
		{
			// raycast did not hit an objects bounding box
			return false;
		}

		// at this point we have raycast to a series of different objects bounding boxes.
		// we need to test these different objects to see which is the first polygon hit.
		// there are some minor optimizations (distance based) that mean we wont have to
		// check all of the objects most of the time, but the worst case scenario is that
		// we need to test every triangle of every object.
		Ogre::Real closestDistance = -1.0f;
		Ogre::Vector3 closestResult;
		Ogre::RaySceneQueryResult& queryResult = raySceneQuery->getLastResults();
		for (size_t qrIdx = 0; qrIdx < queryResult.size(); qrIdx++)
		{
			Ogre::String type = queryResult[qrIdx].movable->getMovableType();
			if ("Camera" == type)
				continue;
			// stop checking if we have found a raycast hit that is closer
			// than all remaining entities
			if (closestDistance >= 0.0f && closestDistance < queryResult[qrIdx].distance)
			{
				break;
			}

			// only check this result if its a hit against an entity
			if (type.compare("Entity") == 0)
			{
				// get the entity to check
				Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(queryResult[qrIdx].movable);
				Ogre::String entityName = entity->getName();

				bool foundExcludedOne = false;
				// If its the exclude entity, continue the loop with a different ones
				if (nullptr != excludeMovableObjects)
				{
					for (size_t i = 0; i < excludeMovableObjects->size(); i++)
					{
						if (entity == excludeMovableObjects->at(i))
						{
							foundExcludedOne = true;
							continue;
						}
					}

					if (true == foundExcludedOne)
					{
						continue;
					}
				}

				// Exclude gizmo
				if ("XArrowGizmoEntity" == entityName || "YArrowGizmoEntity" == entityName || "ZArrowGizmoEntity" == entityName || "SphereGizmoEntity" == entityName)
					continue;

				// mesh data to retrieve         
				size_t vertexCount;
				size_t indexCount;
				Ogre::Vector3* vertices;
				unsigned long* indices;

				// get the mesh information
				this->getMeshInformation(entity->getMesh(), vertexCount, vertices, indexCount, indices,
					entity->getParentNode()->_getDerivedPosition(),
					entity->getParentNode()->_getDerivedOrientation(),
					entity->getParentNode()->_getDerivedScale());

				// test for hitting individual triangles on the mesh
				bool newClosestFound = false;
				for (int i = 0; i < static_cast<int>(indexCount); i += 3)
				{
					// check for a hit against this triangle
					std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(verticalRay, vertices[indices[i]],
						vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

					// if it was a hit check if its the closest
					if (hit.first)
					{
						if ((closestDistance < 0.0f) ||
							(hit.second < closestDistance))
						{
							// this is the closest so far, save it off
							closestDistance = hit.second;
							newClosestFound = true;
						}
					}
				}

				// free the verticies and indicies memory
				OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
				OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

				// if we found a new closest raycast for this object, update the
				// closestResult before moving on to the next object.
				if (newClosestFound)
				{
					// Note: getPoint gives the absolute position!
					closestResult = verticalRay.getPoint(closestDistance);
				}
			}
		}

		// return the result
		if (closestDistance >= 0.0f)
		{
			// raycast success
			result = closestResult;
			height = result.y;
			return true;
		}
		else
		{
			// raycast failed
			return false;
		}
	}

	bool MathHelper::getRaycastResult(const Ogre::Vector3& origin, const Ogre::Vector3& direction, Ogre::RaySceneQuery* raySceneQuery, Ogre::Vector3& result,
		size_t& targetMovableObject, std::vector<Ogre::MovableObject*>* excludeMovableObjects)
	{
		result = Ogre::Vector3::ZERO;
		targetMovableObject = 0;

		Ogre::Ray ray = Ogre::Ray(origin, direction);
		raySceneQuery->setRay(ray);

		// execute the query, returns a vector of hits
		if (raySceneQuery->execute().size() <= 0)
		{
			// raycast did not hit an objects bounding box
			return false;
		}

		// at this point we have raycast to a series of different objects bounding boxes.
		// we need to test these different objects to see which is the first polygon hit.
		// there are some minor optimizations (distance based) that mean we wont have to
		// check all of the objects most of the time, but the worst case scenario is that
		// we need to test every triangle of every object.
		Ogre::Real closestDistance = -1.0f;
		Ogre::Vector3 closestResult;
		Ogre::RaySceneQueryResult& queryResult = raySceneQuery->getLastResults();
		for (size_t qrIdx = 0; qrIdx < queryResult.size(); qrIdx++)
		{
			Ogre::String type = queryResult[qrIdx].movable->getMovableType();
			if ("Camera" == type)
				continue;
			// stop checking if we have found a raycast hit that is closer
			// than all remaining entities
			if (closestDistance >= 0.0f && closestDistance < queryResult[qrIdx].distance)
			{
				break;
			}

			// only check this result if its a hit against an entity
			if (type.compare("Entity") == 0)
			{
				// get the entity to check
				Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(queryResult[qrIdx].movable);
				Ogre::String entityName = entity->getName();

				bool foundExcludedOne = false;
				// If its the exclude entity, continue the loop with a different ones
				if (nullptr != excludeMovableObjects)
				{
					for (size_t i = 0; i < excludeMovableObjects->size(); i++)
					{
						if (entity == excludeMovableObjects->at(i))
						{
							foundExcludedOne = true;
							continue;
						}
					}

					if (true == foundExcludedOne)
					{
						continue;
					}
				}

				// Exclude gizmo
				if ("XArrowGizmoEntity" == entityName || "YArrowGizmoEntity" == entityName || "ZArrowGizmoEntity" == entityName || "SphereGizmoEntity" == entityName)
					continue;

				// mesh data to retrieve         
				size_t vertexCount;
				size_t indexCount;
				Ogre::Vector3* vertices;
				unsigned long* indices;

				// get the mesh information
				this->getMeshInformation(entity->getMesh(), vertexCount, vertices, indexCount, indices,
					entity->getParentNode()->_getDerivedPosition(),
					entity->getParentNode()->_getDerivedOrientation(),
					entity->getParentNode()->_getDerivedScale());

				// test for hitting individual triangles on the mesh
				bool newClosestFound = false;
				for (int i = 0; i < static_cast<int>(indexCount); i += 3)
				{
					// check for a hit against this triangle
					std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(ray, vertices[indices[i]],
						vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

					// if it was a hit check if its the closest
					if (hit.first)
					{
						if ((closestDistance < 0.0f) || (hit.second < closestDistance))
						{
							// this is the closest so far, save it off
							closestDistance = hit.second;
							newClosestFound = true;
						}
					}
				}

				// free the verticies and indicies memory
				OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
				OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

				// if we found a new closest raycast for this object, update the
				// closestResult before moving on to the next object.
				if (newClosestFound)
				{
					targetMovableObject = (size_t)entity;
					closestResult = ray.getPoint(closestDistance);
				}
			}
			else if (type.compare("Item") == 0)
			{
				Ogre::Item* item = dynamic_cast<Ogre::Item*>(queryResult[qrIdx].movable);
				if (nullptr != item)
				{
					// If its the exclude entity, continue the loop with a different ones
					bool foundExcludedOne = false;
					Ogre::String itemName = item->getName();

					// Exclude gizmo
					if ("XArrowGizmoEntity" == itemName || "YArrowGizmoEntity" == itemName || "ZArrowGizmoEntity" == itemName || "SphereGizmoEntity" == itemName)
						continue;

					if (nullptr != excludeMovableObjects)
					{
						for (size_t i = 0; i < excludeMovableObjects->size(); i++)
						{
							if (item == excludeMovableObjects->at(i))
							{
								foundExcludedOne = true;
								continue;
							}
						}

						if (true == foundExcludedOne)
						{
							continue;
						}
					}

					// mesh data to retrieve
					size_t vertexCount;
					size_t indexCount;
					Ogre::Vector3* vertices = nullptr;
					unsigned long* indices = nullptr;

					// get the mesh information
					this->getMeshInformation2(item->getMesh(), vertexCount, vertices,
						indexCount, indices, item->getParentNode()->_getDerivedPositionUpdated(),
						item->getParentNode()->_getDerivedOrientationUpdated(),
						item->getParentNode()->getScale());

					// test for hitting individual triangles on the mesh
					bool newClosestFound = false;
					for (int i = 0; i < static_cast<int> (indexCount); i += 3)
					{
						// check for a hit against this triangle
						std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(raySceneQuery->getRay(),
							vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

						// if it was a hit check if its the closest
						if (hit.first)
						{
							if ((closestDistance < 0.0f) || (hit.second < closestDistance))
							{
								// this is the closest so far, save it off
								closestDistance = hit.second;
								newClosestFound = true;
							}
						}
					}

					// free the verticies and indicies memory
					OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
					OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

					// if we found a new closest raycast for this object, update the
					// closestResult before moving on to the next object.
					if (newClosestFound)
					{
						targetMovableObject = (size_t)item;
						closestResult = raySceneQuery->getRay().getPoint(closestDistance);
					}
				}
			}
			else if ((queryResult[qrIdx].movable != nullptr) &&
				(queryResult[qrIdx].movable->getMovableType().compare("Terra") == 0))
			{
				// Terra
				Ogre::Terra* terra = dynamic_cast<Ogre::Terra*>(queryResult[qrIdx].movable);
				if (nullptr != terra)
				{
					auto resultData = terra->checkRayIntersect(raySceneQuery->getRay());
					if (true == std::get<0>(resultData) && (closestDistance < 0.0f || std::get<3>(resultData) < closestDistance))
					{
						closestResult = std::get<1>(resultData);
						// Terra may have its origin in negative, hence subtract to become positive
						closestDistance = std::get<3>(resultData);
						targetMovableObject = (size_t)terra;
					}
				}
			}
		}

		// return the result
		if (closestDistance >= 0.0f)
		{
			// raycast success
			result = closestResult;
			return true;
		}
		else
		{
			// raycast failed
			return false;
		}
	}

	bool MathHelper::getRaycastHeightAndNormal(Ogre::Real x, Ogre::Real z, Ogre::RaySceneQuery* raySceneQuery, Ogre::Real& height, Ogre::Vector3& normalOnModel)
	{
		Ogre::Vector3 result = Ogre::Vector3::ZERO;
		Ogre::Ray verticalRay = Ogre::Ray(Ogre::Vector3(x, 5000, z), Ogre::Vector3::NEGATIVE_UNIT_Y);
		raySceneQuery->setRay(verticalRay);

		// check we are initialised
		if (raySceneQuery != nullptr)
		{
			// execute the query, returns a vector of hits
			if (raySceneQuery->execute().size() <= 0)
			{
				// raycast did not hit an objects bounding box
				return false;
			}
		}
		else
		{
			// LOG_ERROR << "Cannot raycast without RaySceneQuery instance" << ENDLOG;
			return false;
		}
		// at this point we have raycast to a series of different objects bounding boxes.
		// we need to test these different objects to see which is the first polygon hit.
		// there are some minor optimizations (distance based) that mean we wont have to
		// check all of the objects most of the time, but the worst case scenario is that
		// we need to test every triangle of every object.

		bool newClosestFound = false;
		// Ogre::v1::Polygon polygon;

		Ogre::Real closestDistance = -1.0f;
		Ogre::Vector3 closestResult;
		Ogre::RaySceneQueryResult& queryResult = raySceneQuery->getLastResults();
		for (size_t qrIdx = 0; qrIdx < queryResult.size(); qrIdx++)
		{
			Ogre::String type = queryResult[qrIdx].movable->getMovableType();
			if ("Camera" == type)
				continue;
			// stop checking if we have found a raycast hit that is closer
			// than all remaining entities
			if (closestDistance >= 0.0f && closestDistance < queryResult[qrIdx].distance)
			{
				break;
			}

			// only check this result if its a hit against an entity
			if (type.compare("Entity") == 0)
			{
				// get the entity to check
				Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(queryResult[qrIdx].movable);

				// mesh data to retrieve
				size_t vertexCount;
				size_t indexCount;
				Ogre::Vector3* vertices;
				unsigned long* indices;

				// get the mesh information
				this->getMeshInformation(entity->getMesh(), vertexCount, vertices,
					indexCount, indices, entity->getParentNode()->_getDerivedPositionUpdated(),
					entity->getParentNode()->_getDerivedOrientationUpdated(),
					entity->getParentNode()->getScale());

				// test for hitting individual triangles on the mesh

				for (int i = 0; i < static_cast<int> (indexCount); i += 3)
				{
					// check for a hit against this triangle
					std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(raySceneQuery->getRay(),
						vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

					// if it was a hit check if its the closest
					if (hit.first)
					{
						if ((closestDistance < 0.0f) || (hit.second < closestDistance))
						{
							// this is the closest so far, save it off
							closestDistance = hit.second;
							newClosestFound = true;
							//get the normal from the vertices
							/*polygon.insertVertex(vertices[indices[i]]);
							polygon.insertVertex(vertices[indices[i + 1]]);
							polygon.insertVertex(vertices[indices[i + 2]]);
							normalOnModel = polygon.getNormal();*/

							// Get the normal from the vertices
// Attention: Ogre::Polygon is no more availabke, so is this correct?
							Ogre::Vector3 v1 = vertices[indices[i]] - vertices[indices[i + 1]];
							Ogre::Vector3 v2 = vertices[indices[i + 2]] - vertices[indices[i + 1]];
							normalOnModel = v1.crossProduct(v2);
						}
					}
				}

				// free the verticies and indicies memory
				OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
				OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

				// if we found a new closest raycast for this object, update the
				// closestResult before moving on to the next object.
				if (newClosestFound)
				{
					closestResult = raySceneQuery->getRay().getPoint(closestDistance);
				}
			}
		}
		return newClosestFound;
	}

	bool MathHelper::getRaycastFromPoint(Ogre::RaySceneQuery* raySceneQuery, Ogre::Camera* camera, Ogre::Vector3& resultPositionOnModel, size_t& targetMovableObject, float& closestDistance,
		Ogre::Vector3& normalOnModel, std::vector<Ogre::MovableObject*>* excludeMovableObjects, bool forGizmo)
	{
		bool foundGizmo = false;
		targetMovableObject = 0;
		// check we are initialised
		if (raySceneQuery != nullptr)
		{
			if (true == forGizmo)
			{
				raySceneQuery->setSortByDistance(false);
			}
			// execute the query, returns a vector of hits
			if (raySceneQuery->execute().size() <= 0)
			{
				// raycast did not hit an objects bounding box
				return false;
			}
		}
		else
		{
			// LOG_ERROR << "Cannot raycast without RaySceneQuery instance" << ENDLOG;
			return false;
		}
		// at this point we have raycast to a series of different objects bounding boxes.
		// we need to test these different objects to see which is the first polygon hit.
		// there are some minor optimizations (distance based) that mean we wont have to
		// check all of the objects most of the time, but the worst case scenario is that
		// we need to test every triangle of every object.

		// Ogre::Polygon polygon;

		closestDistance = -1.0f;
		Ogre::Vector3 closestResult;
		Ogre::RaySceneQueryResult& queryResult = raySceneQuery->getLastResults();
		for (size_t qrIdx = 0; qrIdx < queryResult.size(); qrIdx++)
		{
			// Only check this result if its a hit against an entity
			Ogre::String type = queryResult[qrIdx].movable->getMovableType();

			if ("Camera" == type)
				continue;

			// Stop checking if we have found a raycast hit that is closer than all remaining entities
			if (closestDistance >= 0.0f && closestDistance < queryResult[qrIdx].distance && false == forGizmo)
			{
				break;
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MathHelper]: Found type : " + type + " name: " +  queryResult[qrIdx].movable->getName());

			bool foundType = false;

			bool newClosestFound = false;

			// get the entity to check
			Ogre::v1::Entity* entity = dynamic_cast<Ogre::v1::Entity*>(queryResult[qrIdx].movable);
			if (nullptr != entity)
			{
				foundType = true;
				// If its the exclude entity, continue the loop with a different ones
				bool foundExcludedOne = false;
				Ogre::String entityName = entity->getName();
				if (true == forGizmo)
				{
					// Only check for gizmo, no matter how far away it is, or if there are some objects covering the gizmo!
					if ("XArrowGizmoEntity" != entityName && "YArrowGizmoEntity" != entityName && "ZArrowGizmoEntity" != entityName && "SphereGizmoEntity" != entityName)
						continue;
					else
						foundGizmo = true;
				}

				if (nullptr != excludeMovableObjects)
				{
					for (size_t i = 0; i < excludeMovableObjects->size(); i++)
					{
						if (entity == excludeMovableObjects->at(i))
						{
							foundExcludedOne = true;
							continue;
						}
					}

					if (true == foundExcludedOne)
					{
						continue;
					}
				}

				// mesh data to retrieve
				size_t vertexCount;
				size_t indexCount;
				Ogre::Vector3* vertices = nullptr;
				unsigned long* indices = nullptr;

				// get the mesh information
				this->getMeshInformation(entity->getMesh(), vertexCount, vertices,
					indexCount, indices, entity->getParentNode()->_getDerivedPositionUpdated(),
					entity->getParentNode()->_getDerivedOrientationUpdated(),
					entity->getParentNode()->getScale());

				// test for hitting individual triangles on the mesh
				for (int i = 0; i < static_cast<int> (indexCount); i += 3)
				{
					// check for a hit against this triangle
					std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(raySceneQuery->getRay(),
						vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

					// if it was a hit check if its the closest
					if (hit.first)
					{
						if ((closestDistance < 0.0f) || (hit.second < closestDistance) || forGizmo)
						{
							// this is the closest so far, save it off
							closestDistance = hit.second;
							newClosestFound = true;

							// Gizmo found, do not check further
							if (true == foundGizmo)
								break;

							// Get the normal from the vertices
							Ogre::Vector3 v1 = vertices[indices[i]] - vertices[indices[i + 1]];
							Ogre::Vector3 v2 = vertices[indices[i + 2]] - vertices[indices[i + 1]];
							normalOnModel = v1.crossProduct(v2);
							normalOnModel.normalise();
						}
					}
				}

				// free the verticies and indicies memory
				OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
				OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

				// if we found a new closest raycast for this object, update the
				// closestResult before moving on to the next object.
				if (newClosestFound)
				{
					targetMovableObject = (size_t)entity;
					closestResult = raySceneQuery->getRay().getPoint(closestDistance);
					// Gizmo found, do not check further
					if (true == foundGizmo)
						return true;
				}
			}

			if (false == foundType)
			{
				Ogre::Item* item = dynamic_cast<Ogre::Item*>(queryResult[qrIdx].movable);
				if (nullptr != item)
				{
					foundType = true;
					// If its the exclude entity, continue the loop with a different ones
					bool foundExcludedOne = false;
					Ogre::String itemName = item->getName();
					if (true == forGizmo)
					{
						// Only check for gizmo, no matter how far away it is, or if there are some objects covering the gizmo!
						if ("XArrowGizmoEntity" != itemName && "YArrowGizmoEntity" != itemName && "ZArrowGizmoEntity" != itemName && "SphereGizmoEntity" != itemName)
							continue;
						else
							foundGizmo = true;
					}

					if (nullptr != excludeMovableObjects)
					{
						for (size_t i = 0; i < excludeMovableObjects->size(); i++)
						{
							if (item == excludeMovableObjects->at(i))
							{
								foundExcludedOne = true;
								continue;
							}
						}

						if (true == foundExcludedOne)
						{
							continue;
						}
					}

					// mesh data to retrieve
					size_t vertexCount;
					size_t indexCount;
					Ogre::Vector3* vertices = nullptr;
					unsigned long* indices = nullptr;

					// get the mesh information
					this->getMeshInformation2(item->getMesh(), vertexCount, vertices,
						indexCount, indices, item->getParentNode()->_getDerivedPositionUpdated(),
						item->getParentNode()->_getDerivedOrientationUpdated(),
						item->getParentNode()->getScale());

					// test for hitting individual triangles on the mesh
					for (int i = 0; i < static_cast<int> (indexCount); i += 3)
					{
						// check for a hit against this triangle
						std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(raySceneQuery->getRay(),
							vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

						// if it was a hit check if its the closest
						if (hit.first)
						{
							if ((closestDistance < 0.0f) || (hit.second < closestDistance) || forGizmo)
							{
								// this is the closest so far, save it off
								closestDistance = hit.second;
								newClosestFound = true;

								// Gizmo found, do not check further
								if (true == foundGizmo)
									break;

								// Get the normal from the vertices
								Ogre::Vector3 v1 = vertices[indices[i]] - vertices[indices[i + 1]];
								Ogre::Vector3 v2 = vertices[indices[i + 2]] - vertices[indices[i + 1]];
								normalOnModel = v1.crossProduct(v2);
								normalOnModel.normalise();
							}
						}
					}

					// free the verticies and indicies memory
					OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
					OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

					// if we found a new closest raycast for this object, update the
					// closestResult before moving on to the next object.
					if (newClosestFound)
					{
						targetMovableObject = (size_t)item;
						closestResult = raySceneQuery->getRay().getPoint(closestDistance);
						// Gizmo found, do not check further
						if (true == foundGizmo)
							return true;
					}
				}
				else
				{
					// Terra
					Ogre::Terra* terra = dynamic_cast<Ogre::Terra*>(queryResult[qrIdx].movable);
					if (nullptr != terra)
					{
						foundType = true;
						// - 0 later position of terra?

						auto resultData = terra->checkRayIntersect(raySceneQuery->getRay());
						if (true == std::get<0>(resultData) && (closestDistance < 0.0f || std::get<3>(resultData) < closestDistance))
						{
							closestResult = std::get<1>(resultData);
							normalOnModel = std::get<2>(resultData);
							normalOnModel.normalise();
							closestDistance = std::get<3>(resultData);
							targetMovableObject = (size_t)terra;
							newClosestFound = true;
						}
					}
				}

				if (false == foundType)
				{
					// ManualObject
					Ogre::ManualObject* manualObject = dynamic_cast<Ogre::ManualObject*>(queryResult[qrIdx].movable);
					if (nullptr != manualObject)
					{
						foundType = true;

						// mesh data to retrieve
						size_t vertexCount;
						size_t indexCount;
						Ogre::Vector3* vertices = nullptr;
						unsigned long* indices = nullptr;

						// get the manual mesh information
						this->getManualMeshInformation2(manualObject, vertexCount, vertices,
							indexCount, indices, manualObject->getParentNode()->_getDerivedPositionUpdated(),
							manualObject->getParentNode()->_getDerivedOrientationUpdated(),
							manualObject->getParentNode()->getScale());

						// test for hitting individual triangles on the mesh
						for (int i = 0; i < static_cast<int> (indexCount); i += 3)
						{
							// check for a hit against this triangle
							std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(raySceneQuery->getRay(),
								vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

							// if it was a hit check if its the closest
							if (hit.first)
							{
								if ((closestDistance < 0.0f) || (hit.second < closestDistance))
								{
									// this is the closest so far, save it off
									closestDistance = hit.second;
									newClosestFound = true;

									// Get the normal from the vertices
									Ogre::Vector3 v1 = vertices[indices[i]] - vertices[indices[i + 1]];
									Ogre::Vector3 v2 = vertices[indices[i + 2]] - vertices[indices[i + 1]];
									normalOnModel = v1.crossProduct(v2);
									normalOnModel.normalise();
								}
							}
						}

						// free the verticies and indicies memory
						OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
						OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

						// if we found a new closest raycast for this object, update the
						// closestResult before moving on to the next object.
						if (newClosestFound)
						{
							targetMovableObject = (size_t)manualObject;
							closestResult = raySceneQuery->getRay().getPoint(closestDistance);
						}
					}
				}
			}
		}

		// return the result
		if (closestDistance >= 0.0f)
		{
			// raycast success
			resultPositionOnModel = closestResult;
			return true;
		}
		else
		{
			// raycast failed
			return false;
		}
	}

	bool MathHelper::getRaycastFromPoint(int mouseX, int mouseY, Ogre::Camera* camera, Ogre::Window* renderWindow, Ogre::RaySceneQuery* raySceneQuery,
		Ogre::Vector3& resultPositionOnModel, size_t& targetMovableObject, float& closestDistance, Ogre::Vector3& normalOnModel, std::vector<Ogre::MovableObject*>* excludeMovableObjects, bool forGizmo)
	{
		Ogre::Real resultX;
		Ogre::Real resultY;
		this->mouseToViewPort(mouseX, mouseY, resultX, resultY, renderWindow);
		Ogre::Ray ray = camera->getCameraToViewportRay(resultX, resultY);
		raySceneQuery->setRay(ray);
		return this->getRaycastFromPoint(raySceneQuery, camera, resultPositionOnModel, targetMovableObject, closestDistance, normalOnModel, excludeMovableObjects, forGizmo);
	}

	/*
	// http://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
	Distance approximation:
	u32 approx_distance( s32 dx, s32 dy )
	{
	u32 min, max;

	if ( dx < 0 ) dx = -dx;
	if ( dy < 0 ) dy = -dy;

	if ( dx < dy )
	{
	min = dx;
	max = dy;
	} else {
	min = dy;
	max = dx;
	}

	// coefficients equivalent to ( 123/128 * max ) and ( 51/128 * min )
	return ((( max << 8 ) + ( max << 3 ) - ( max << 4 ) - ( max << 1 ) +
	( min << 7 ) - ( min << 5 ) + ( min << 3 ) - ( min << 1 )) >> 8 );
	}
	*/

	Ogre::Aabb MathHelper::getWorldAABB(Ogre::SceneNode* sceneNode)
	{
		Ogre::Aabb aabb;

		// merge with attached objects
		for (size_t i = 0; i < sceneNode->numAttachedObjects(); ++i)
		{
			Ogre::MovableObject* movableObject = sceneNode->getAttachedObject(i);
			// Attention: is that correct with Updated()?
			aabb.merge(movableObject->getWorldAabbUpdated());
		}

		// merge with child nodes
		for (size_t i = 0; i < sceneNode->numChildren(); ++i)
		{
			Ogre::SceneNode* childSceneNode = static_cast<Ogre::SceneNode*>(sceneNode->getChild(i));
			aabb.merge(getWorldAABB(childSceneNode));
		}

		return aabb;
	}

	Ogre::Real MathHelper::mapValue(Ogre::Real valueToMap, Ogre::Real targetMin, Ogre::Real targetMax)
	{
		return targetMin + valueToMap * (targetMax - targetMin);
	}

	Ogre::Real MathHelper::mapValue2(Ogre::Real valueToMap, Ogre::Real sourceMin, Ogre::Real sourceMax, Ogre::Real targetMin, Ogre::Real targetMax)
	{
		return targetMin + ((targetMax - targetMin) * (valueToMap - sourceMin)) / (sourceMax - sourceMin);
	}

	Ogre::Vector3 MathHelper::pointToLocalSpace(const Ogre::Vector3& point, Ogre::Vector3& heading, Ogre::Vector3& side, Ogre::Vector3& position)
	{
		// http://www.codinglabs.net/article_world_view_projection_matrix.aspx
		// Make a copy of the point
		Ogre::Vector3 transPoint = point;
		Ogre::Vector3 transPosition = position;

		// Create a transformation matrix
		C2DMatrix matTransform;

		Ogre::Real tx = -transPosition.dotProduct(heading);
		Ogre::Real tz = -transPosition.dotProduct(side);

		// Create the transformation matrix
		matTransform._11(heading.x); matTransform._12(side.x);
		matTransform._21(heading.z); matTransform._22(side.z);
		matTransform._31(tx);           matTransform._32(tz);

		//now transform the vertices
		matTransform.TransformVector2Ds(transPoint);

		return transPoint;
	}

	Ogre::Vector3 MathHelper::vectorToWorldSpace(const Ogre::Vector3& vector, const Ogre::Vector3& heading, const Ogre::Vector3& side)
	{
		// Make a copy of the point
		Ogre::Vector3 transVec = vector;

		//create a transformation matrix
		C2DMatrix matTransform;

		// Rotate
		matTransform.Rotate(heading, side);

		// Now transform the vertices
		matTransform.TransformVector2Ds(transVec);

		return transVec;
	}

	Ogre::Vector3 MathHelper::pointToWorldSpace(const Ogre::Vector3& point, const Ogre::Vector3& heading, const Ogre::Vector3& side, const Ogre::Vector3& position)
	{
		// Make a copy of the point
		Ogre::Vector3 transPoint = point;

		// Create a transformation matrix
		C2DMatrix matTransform;

		// Rotate
		matTransform.Rotate(heading, side);

		//and translate
		matTransform.Translate(position.x, position.z);

		//now transform the vertices
		matTransform.TransformVector2Ds(transPoint);

		return transPoint;
	}

	bool MathHelper::hasNoTangentsAndCanGenerate(Ogre::v1::VertexDeclaration* vertexDecl)
	{
		bool hasTangents = false;
		bool hasUVs = false;
		const Ogre::v1::VertexDeclaration::VertexElementList& elementList = vertexDecl->getElements();
		Ogre::v1::VertexDeclaration::VertexElementList::const_iterator itor = elementList.begin();
		Ogre::v1::VertexDeclaration::VertexElementList::const_iterator end = elementList.end();

		while (itor != end && !hasTangents)
		{
			const Ogre::v1::VertexElement& vertexElem = *itor;
			if (vertexElem.getSemantic() == Ogre::VES_TANGENT)
			{
				hasTangents = true;
			}
			if (vertexElem.getSemantic() == Ogre::VES_TEXTURE_COORDINATES)
			{
				hasUVs = true;
			}
			++itor;
		}

		return !hasTangents && hasUVs;
	}

	void MathHelper::ensureHasTangents(Ogre::v1::MeshPtr mesh)
	{
		bool generateTangents = false;
		if (mesh->sharedVertexData[0])
		{
			Ogre::v1::VertexDeclaration* vertexDecl = mesh->sharedVertexData[0]->vertexDeclaration;
			generateTangents |= hasNoTangentsAndCanGenerate(vertexDecl);
		}

		for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
		{
			Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);
			if (subMesh->vertexData[0])
			{
				Ogre::v1::VertexDeclaration* vertexDecl = subMesh->vertexData[0]->vertexDeclaration;
				generateTangents |= hasNoTangentsAndCanGenerate(vertexDecl);
			}
		}

		try
		{
			if (generateTangents)
			{
				mesh->buildTangentVectors();
			}
		}
		catch (...)
		{
		}
	}

	void MathHelper::substractOutTangentsForShader(Ogre::v1::Entity* entity)
	{
		try
		{
			// Build tangent vectors, all our meshes use only 1 texture coordset 
			// Note its possible to build into VES_TANGENT now (SM2+)
			Ogre::v1::MeshPtr meshPtr = entity->getMesh();
			unsigned short src, dest;
			if (!meshPtr->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
			{
				meshPtr->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
			}
		}
		catch (...)
		{
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ShaderModule]: Could not generate tangents for : " + entity->getName());
		}
	}

}; // namespace end