#include "NOWAPrecompiled.h"
#include "MathHelper.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "OgreBitwise.h"
#include "Vao/OgreAsyncTicket.h"
#include "Terra.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"
#include "modules/GraphicsModule.h"

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

	Ogre::Vector2 MathHelper::round(const Ogre::Vector2& vec, unsigned int places)
	{
		Ogre::Vector2 result = vec;
		result.x = round(result.x, places);
		result.y = round(result.y, places);
		return result;
	}

	Ogre::Vector3 MathHelper::round(const Ogre::Vector3& vec, unsigned int places)
	{
		Ogre::Vector3 result = vec;
		result.x = round(result.x, places);
		result.y = round(result.y, places);
		result.z = round(result.z, places);
		return result;
	}

	Ogre::Vector4 MathHelper::round(const Ogre::Vector4& vec, unsigned int places)
	{
		Ogre::Vector4 result = vec;
		result.x = round(result.x, places);
		result.y = round(result.y, places);
		result.z = round(result.z, places);
		result.w = round(result.w, places);
		return result;
	}

	Ogre::Quaternion MathHelper::diffPitch(const Ogre::Quaternion& dest, const Ogre::Quaternion& src)
	{
		Ogre::Quaternion quat = Ogre::Quaternion::ZERO;
		quat.FromAngleAxis(dest.getPitch() - src.getPitch(), Ogre::Vector3::UNIT_X);
		return quat;
	}

	Ogre::Quaternion MathHelper::diffYaw(const Ogre::Quaternion& dest, const Ogre::Quaternion& src)
	{
		Ogre::Quaternion quat = Ogre::Quaternion::ZERO;
		quat.FromAngleAxis(dest.getYaw() - src.getYaw(), Ogre::Vector3::UNIT_Y);
		return quat;
	}

	Ogre::Quaternion MathHelper::diffRoll(const Ogre::Quaternion& dest, const Ogre::Quaternion& src)
	{
		Ogre::Quaternion quat = Ogre::Quaternion::ZERO;
		quat.FromAngleAxis(dest.getRoll() - src.getRoll(), Ogre::Vector3::UNIT_Z);
		return quat;
	}

	Ogre::Quaternion MathHelper::diffDegree(const Ogre::Quaternion& dest, const Ogre::Quaternion& src, int mode)
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

#if 0
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
#else
	Ogre::Quaternion MathHelper::faceDirectionSlerp(
		const Ogre::Quaternion& sourceOrientation,
		const Ogre::Vector3& direction,
		const Ogre::Vector3& defaultDirection,
		Ogre::Real dt,
		Ogre::Real rotationSpeedDegPerSec)
	{
		Ogre::Vector3 currentDirection = sourceOrientation * defaultDirection;

		// Get rotation needed to face direction
		Ogre::Quaternion deltaRotation = currentDirection.getRotationTo(direction);
		Ogre::Quaternion destinationOrientation = deltaRotation * sourceOrientation;

		// Extract angle between orientations
		Ogre::Radian angle;
		Ogre::Vector3 axis;
		Ogre::Quaternion rotationBetween = destinationOrientation * sourceOrientation.Inverse();
		rotationBetween.ToAngleAxis(angle, axis);

		// Clamp rotation per frame
		Ogre::Real maxAngleThisFrame = Ogre::Degree(rotationSpeedDegPerSec * dt).valueRadians();

		if (angle.valueRadians() < 1e-6f) // Already facing, or very close
		{
			return sourceOrientation;
		}

		Ogre::Real ratio = std::min<Ogre::Real>(1.0f, maxAngleThisFrame / angle.valueRadians());

		// Interpolate by the clamped ratio
		return Ogre::Quaternion::Slerp(ratio, sourceOrientation, destinationOrientation, true);
	}
#endif

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

	Ogre::Quaternion MathHelper::computeLookAtQuaternion(const Ogre::Vector3& position, const Ogre::Vector3& target, const Ogre::Vector3& worldUp)
	{
		Ogre::Vector3 direction = target - position;
		direction.normalise();

		Ogre::Vector3 forward = -direction;
		Ogre::Vector3 up = worldUp;

		if (fabs(forward.dotProduct(up)) > 0.999f)
		{
			up = Ogre::Vector3::UNIT_Z;
		}

		Ogre::Vector3 right = up.crossProduct(forward).normalisedCopy();
		Ogre::Vector3 trueUp = forward.crossProduct(right).normalisedCopy();

		Ogre::Matrix3 rotMatrix;
		rotMatrix.FromAxes(right, trueUp, forward);

		return Ogre::Quaternion(rotMatrix);
	}

	Ogre::Quaternion MathHelper::computeDirectionQuaternion(const Ogre::Vector3& direction, const Ogre::Vector3& fallbackUp)
	{
		if (direction.isZeroLength())
		{
			return Ogre::Quaternion::IDENTITY;
		}

		Ogre::Vector3 forward = -direction.normalisedCopy(); // Ogre camera looks down -Z

		// Compute right and up vectors
		Ogre::Vector3 right = fallbackUp.crossProduct(forward);
		if (right.isZeroLength())
		{
			right = Ogre::Vector3::UNIT_X; // fallback for parallel up/forward
		}

		right.normalise();
		Ogre::Vector3 up = forward.crossProduct(right);
		up.normalise();

		Ogre::Quaternion q;
		q.FromAxes(right, up, forward); // note: Ogre uses right, up, forward
		return q;
    }

    Ogre::Quaternion MathHelper::computeLandingOrientation(const Ogre::Quaternion& currentOrient, const Ogre::Vector3& surfaceNormal, const Ogre::Vector3& defaultDirection)
    {
        // Compute ship's current world forward using the mesh's default direction.
        Ogre::Vector3 worldFwd = currentOrient * defaultDirection;

        // Project onto the surface plane to get the horizontal heading.
        Ogre::Vector3 fwdH = worldFwd - worldFwd.dotProduct(surfaceNormal) * surfaceNormal;
        if (fwdH.squaredLength() < 0.001f)
        {
            // Ship is pointing straight into or away from the planet.
            // Fall back to ship's right axis instead.
            Ogre::Vector3 worldRight = currentOrient * Ogre::Vector3::UNIT_X;
            fwdH = worldRight - worldRight.dotProduct(surfaceNormal) * surfaceNormal;
            if (fwdH.squaredLength() < 0.001f)
            {
                // Last resort: any direction perpendicular to the normal.
                fwdH = surfaceNormal.perpendicular();
            }
        }
        fwdH.normalise();

        // R1: rotate local +Y to align with surfaceNormal.
        // getRotationTo() handles all edge cases (parallel / anti-parallel vectors) internally.
        Ogre::Quaternion R1 = Ogre::Vector3::UNIT_Y.getRotationTo(surfaceNormal);

        // After R1 the default forward direction lands at:
        Ogre::Vector3 rotatedFwd = R1 * defaultDirection;
        Ogre::Vector3 rotatedFwdH = rotatedFwd - rotatedFwd.dotProduct(surfaceNormal) * surfaceNormal;
        if (rotatedFwdH.squaredLength() < 0.001f)
        {
            // Forward happens to be perfectly vertical after R1 -- no twist needed.
            return R1;
        }
        rotatedFwdH.normalise();

        // R2: twist around surfaceNormal so the forward heading matches fwdH.
        Ogre::Quaternion R2 = rotatedFwdH.getRotationTo(fwdH);

        // Combined orientation: align up first, then twist to correct heading.
        return R2 * R1;
    }

	//Ogre::Quaternion MathHelper::computeLandingOrientation(const Ogre::Quaternion& currentOrient, const Ogre::Vector3& surfaceNormal, const Ogre::Vector3& defaultDirection)
 //   {
 //       // Compute the ship's current forward direction in world space using the
 //       // mesh-specific default direction set in NOWA-Design (e.g. UNIT_Z for the fighter).
 //       Ogre::Vector3 worldFwd = currentOrient * defaultDirection;

 //       // Project onto the surface plane to get the horizontal heading.
 //       Ogre::Vector3 fwdH = worldFwd - worldFwd.dotProduct(surfaceNormal) * surfaceNormal;

 //       if (fwdH.squaredLength() < 0.01f)
 //       {
 //           // Degenerate: ship pointing straight toward or away from the body.
 //           // Try an alternate axis perpendicular to defaultDir to get a usable heading.
 //           Ogre::Vector3 altLocal = (std::abs(defaultDirection.z) < 0.9f) ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_X;
 //           Ogre::Vector3 altWorld = currentOrient * altLocal;
 //           fwdH = altWorld - altWorld.dotProduct(surfaceNormal) * surfaceNormal;
 //           if (fwdH.squaredLength() < 0.01f)
 //           {
 //               return currentOrient;
 //           }
 //       }
 //       fwdH.normalise();

 //       // Build target rotation matrix.
 //       // Column 1 (local Y) = surfaceNormal -- ship stands upright on the body.
 //       // The column that corresponds to defaultDir is set to fwdH (or -fwdH for
 //       // negative-direction axes) so the ship's forward stays in its original heading.
 //       // The remaining column is derived from the cross product to maintain
 //       // right-handedness.
 //       //
 //       // Handles UNIT_Z, NEGATIVE_UNIT_Z, UNIT_X, NEGATIVE_UNIT_X automatically.
 //       Ogre::Vector3 col0, col1, col2;
 //       col1 = surfaceNormal;

 //       const float absX = std::abs(defaultDirection.x);
 //       const float absY = std::abs(defaultDirection.y);
 //       const float absZ = std::abs(defaultDirection.z);

 //       if (absZ >= absX && absZ >= absY)
 //       {
 //           // Forward is along the Z axis (UNIT_Z or NEGATIVE_UNIT_Z -- most common).
 //           // col2 = direction of local +Z in world = fwdH if UNIT_Z, -fwdH if NEGATIVE_UNIT_Z.
 //           col2 = fwdH * (defaultDirection.z > 0.0f ? 1.0f : -1.0f);
 //           col0 = col1.crossProduct(col2); // X = Y cross Z
 //           if (col0.squaredLength() < 1e-6f)
 //           {
 //               return currentOrient;
 //           }
 //           col0.normalise();
 //           col2 = col0.crossProduct(col1); // reorthogonalize Z = X cross Y
 //           col2.normalise();
 //       }
 //       else if (absX >= absY)
 //       {
 //           // Forward is along the X axis (UNIT_X or NEGATIVE_UNIT_X).
 //           // col0 = direction of local +X in world.
 //           col0 = fwdH * (defaultDirection.x > 0.0f ? 1.0f : -1.0f);
 //           col2 = col0.crossProduct(col1); // Z = X cross Y
 //           if (col2.squaredLength() < 1e-6f)
 //           {
 //               return currentOrient;
 //           }
 //           col2.normalise();
 //           col0 = col1.crossProduct(col2); // reorthogonalize X = Y cross Z
 //           col0.normalise();
 //       }
 //       else
 //       {
 //           // Forward is along the Y axis -- unusual for a ship, but handle gracefully.
 //           // Fall back: treat fwdH as Z axis.
 //           col2 = fwdH;
 //           col0 = col1.crossProduct(col2);
 //           if (col0.squaredLength() < 1e-6f)
 //           {
 //               return currentOrient;
 //           }
 //           col0.normalise();
 //           col2 = col0.crossProduct(col1);
 //           col2.normalise();
 //       }

 //       Ogre::Matrix3 m;
 //       m.SetColumn(0, col0);
 //       m.SetColumn(1, col1);
 //       m.SetColumn(2, col2);
 //       return Ogre::Quaternion(m);
 //   }

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

	Ogre::Real MathHelper::min(Ogre::Real v1, Ogre::Real v2)
	{
		return std::min(v1, v2);
	}

	Ogre::Real MathHelper::max(Ogre::Real v1, Ogre::Real v2)
	{
		return std::max(v1, v2);
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

	Ogre::Vector3 MathHelper::calculateGridTranslation(const Ogre::Vector3& gridFactor, const Ogre::Vector3& targetWorldPosition, Ogre::MovableObject* movableObject)
	{
		Ogre::Vector3 newPos = targetWorldPosition;
		Ogre::Vector3 localPoint = movableObject->getParentNode()->_getDerivedOrientationUpdated().Inverse() * (newPos - movableObject->getParentNode()->_getDerivedPositionUpdated());

		// 1. Transform the world position to the object's local space (accounting for rotation)
		// 2. Snap in local space(using the object's local dimensions)
		// 3. Transform back to world space

		// Round the local point to the nearest grid size, considering different sizes for x and z axes
		localPoint.x = round(localPoint.x / gridFactor.x) * gridFactor.x;
		localPoint.y = round(localPoint.y / gridFactor.y) * gridFactor.y;
		localPoint.z = round(localPoint.z / gridFactor.z) * gridFactor.z;
		newPos = movableObject->getParentNode()->_getDerivedOrientationUpdated() * localPoint + movableObject->getParentNode()->_getDerivedPositionUpdated();

		return newPos;
	}

	Ogre::Vector3 MathHelper::getBottomCenterOfMesh(Ogre::SceneNode* sceneNode, Ogre::MovableObject* movableObject) const
	{
		Ogre::Aabb boundingBox = movableObject->getLocalAabb();
		Ogre::Vector3 maximumScaled = boundingBox.getMaximum() * sceneNode->getScale();
		Ogre::Vector3 minimumScaled = boundingBox.getMinimum() * sceneNode->getScale();
		Ogre::Vector3 size = ((maximumScaled - minimumScaled));
		Ogre::Vector3 centerOffset = minimumScaled + (size / 2.0f);
		// Problem here: when physics is activated an object may slithly go up or down either to prevent collision with underlying object or because of gravity
		// But attention: stack mode must be used, if there is an object below, because it also has an height! Else the y position is 0
		Ogre::Real lowestObjectY = minimumScaled.y;
		centerOffset.y = 0.0f - lowestObjectY;
		return centerOffset;
    }

    Ogre::Vector3 MathHelper::getPlacementYOffset(Ogre::SceneNode* sceneNode, Ogre::MovableObject* movableObject) const
    {
        Ogre::Aabb boundingBox = movableObject->getLocalAabb();
        Ogre::Vector3 minimumScaled = boundingBox.getMinimum() * sceneNode->getScale();

        return Ogre::Vector3(0.0f, -minimumScaled.y, 0.0f);
    }

#if 0
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
#endif

	void MathHelper::getMeshInformation(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orientation,
        const Ogre::Vector3& scale)
    {
        // Safe null outputs — callers check vertexCount/indexCount before using
        // the pointers, so setting them to 0/nullptr here covers error cases.
        vertexCount = indexCount = 0;
        vertices = nullptr;
        indices = nullptr;

        if (mesh.isNull())
        {
            return;
        }

        const size_t meshHandle = mesh->getHandle();

        // ── 1. Fast path: CPU cache ───────────────────────────────────────────────
        // Grab a ref-counted pointer to the cache entry under the lock, then
        // release the lock before doing any allocation or transform work.
        // This keeps the critical section tiny and avoids holding the lock while
        // calling OGRE_ALLOC_T (which may block on the allocator).
        {
            std::shared_ptr<const RaycastMeshCache> cached;
            {
                std::lock_guard<std::mutex> lk(this->meshCacheMutex);
                auto it = this->meshCache.find(meshHandle);
                if (it != this->meshCache.end())
                {
                    cached = it->second;
                }
            }

            if (cached)
            {
                vertexCount = cached->localVerts.size();
                indexCount = cached->indices.size();

                if (0 == vertexCount || 0 == indexCount)
                {
                    return; // valid cached empty mesh (e.g. dummy VAO)
                }

                // allocate memory to the input array reference, handleRequest calls delete on this memory
                vertices = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
                indices = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);

                // Apply world transform on-the-fly from the cached local-space data.
                // This matches exactly what the slow path produced before: each
                // vertex is (orientation * (localPos * scale)) + position.
                for (size_t i = 0; i < vertexCount; ++i)
                {
                    vertices[i] = (orientation * (cached->localVerts[i] * scale)) + position;
                }

                for (size_t i = 0; i < indexCount; ++i)
                {
                    indices[i] = cached->indices[i];
                }

                return;
            }
        }

        // ── 2. Cache miss — GPU readback on render thread ─────────────────────────
        // enqueueAndWait blocks the logic thread until the render thread has run
        // the lambda. Inside the lambda we are on the render thread, so all Ogre
        // GPU operations (readRequests / mapAsyncTickets / readRequest) are safe.
        //
        // This stall is at most one render-frame long (~7 ms at 144 fps) and
        // happens ONLY ONCE per unique mesh handle. After this call returns, every
        // subsequent call for the same mesh hits the fast path above.
        //
        // The new cache entry is built inside the lambda into a local struct on the
        // stack of this function (newData). enqueueAndWait guarantees the lambda
        // has finished before we touch newData again, so there is no race.

        auto newData = std::make_shared<RaycastMeshCache>();

        {
            Ogre::MeshPtr meshCopy = mesh; // keep mesh alive across the wait
            RaycastMeshCache* dst = newData.get();

            NOWA::GraphicsModule::RenderCommand gpuReadback = [meshCopy, dst]()
            {
                for (Ogre::SubMesh* subMesh : meshCopy->getSubMeshes())
                {
                    if (subMesh->mVao[0].empty())
                    {
                        continue;
                    }

                    Ogre::VertexArrayObject* vao = subMesh->mVao[0][0];
                    Ogre::IndexBufferPacked* ib = vao->getIndexBuffer();

                    // Skip VAOs without an index buffer (e.g. navmesh visual debug
                    // geometry, which is created with nullptr as the index buffer).
                    // Those Items have queryFlags=0 so they never reach this code in
                    // normal operation, but we guard here for safety.
                    if (nullptr == ib || 0 == ib->getNumElements())
                    {
                        continue;
                    }

                    // ── Vertex readback ──────────────────────────────────────────
                    Ogre::VertexArrayObject::ReadRequestsVec requests;
                    requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
                    vao->readRequests(requests);
                    vao->mapAsyncTickets(requests);

                    const size_t indexBase = dst->localVerts.size();
                    const size_t numVerts = requests[0].vertexBuffer->getNumElements();

                    if (requests[0].type == Ogre::VET_HALF4)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            const Ogre::uint16* pos = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
                            Ogre::Vector3 vec;
                            vec.x = Ogre::Bitwise::halfToFloat(pos[0]);
                            vec.y = Ogre::Bitwise::halfToFloat(pos[1]);
                            vec.z = Ogre::Bitwise::halfToFloat(pos[2]);
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                            dst->localVerts.push_back(vec);
                        }
                    }
                    else if (requests[0].type == Ogre::VET_FLOAT3)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            const float* pos = reinterpret_cast<const float*>(requests[0].data);
                            Ogre::Vector3 vec;
                            vec.x = *pos++;
                            vec.y = *pos++;
                            vec.z = *pos++;
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                            dst->localVerts.push_back(vec);
                        }
                    }
                    else if (requests[0].type == Ogre::VET_FLOAT4)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            const float* pos = reinterpret_cast<const float*>(requests[0].data);
                            Ogre::Vector3 vec;
                            vec.x = pos[0];
                            vec.y = pos[1];
                            vec.z = pos[2];
                            // pos[3] = w (often 1.0f) -> ignore
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                            dst->localVerts.push_back(vec);
                        }
                    }
                    else
                    {
                        vao->unmapAsyncTickets(requests);
                        Ogre::LogManager::getSingletonPtr()->logMessage("[MathHelper]: Vertex Buffer type not recognised");
                        // Leave dst empty for this submesh — safe, we just miss
                        // those triangles in the raycast.
                        continue;
                    }

                    vao->unmapAsyncTickets(requests);

                    // ── Index readback ───────────────────────────────────────────
                    const bool is32 = (ib->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);
                    Ogre::AsyncTicketPtr asyncTicket = ib->readRequest(0, ib->getNumElements());

                    const unsigned int* pIndices = nullptr;
                    if (is32)
                    {
                        pIndices = reinterpret_cast<const unsigned int*>(asyncTicket->map());
                    }
                    else
                    {
                        const unsigned short* pShortIndices = reinterpret_cast<const unsigned short*>(asyncTicket->map());
                        unsigned int* tmp = new unsigned int[ib->getNumElements()];
                        for (size_t k = 0; k < ib->getNumElements(); ++k)
                        {
                            tmp[k] = static_cast<unsigned int>(pShortIndices[k]);
                        }
                        asyncTicket->unmap();

                        for (size_t i = 0; i < ib->getNumElements(); ++i)
                        {
                            dst->indices.push_back(static_cast<unsigned long>(tmp[i]) + static_cast<unsigned long>(indexBase));
                        }

                        delete[] tmp;
                        continue; // index data already pushed, skip the push loop below
                    }

                    for (size_t i = 0; i < ib->getNumElements(); ++i)
                    {
                        dst->indices.push_back(static_cast<unsigned long>(pIndices[i]) + static_cast<unsigned long>(indexBase));
                    }

                    asyncTicket->unmap();
                }
            };

            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(gpuReadback), "MathHelper::getMeshInformation::gpuCache");
        }

        // Store in cache (under lock) before serving.
        // The shared_ptr is immutable from this point on — readers don't need a lock.
        {
            std::lock_guard<std::mutex> lk(this->meshCacheMutex);
            this->meshCache[meshHandle] = newData;
        }

        // ── Serve from the freshly populated cache ────────────────────────────────
        vertexCount = newData->localVerts.size();
        indexCount = newData->indices.size();

        if (0 == vertexCount || 0 == indexCount)
        {
            return;
        }

        // allocate memory to the input array reference, handleRequest calls delete on this memory
        vertices = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
        indices = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);

        for (size_t i = 0; i < vertexCount; ++i)
        {
            vertices[i] = (orientation * (newData->localVerts[i] * scale)) + position;
        }
        for (size_t i = 0; i < indexCount; ++i)
        {
            indices[i] = newData->indices[i];
        }
    }

	void MathHelper::getDetailedMeshInformation(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orientation,
        const Ogre::Vector3& scale, bool& isVET_HALF4, bool& isIndices32)
    {
        // Safe null outputs
        vertexCount = indexCount = 0;
        vertices = nullptr;
        indices = nullptr;
        isVET_HALF4 = isIndices32 = false;

        if (mesh.isNull())
        {
            return;
        }

        const size_t meshHandle = mesh->getHandle();

        // ── 1. Fast path: CPU cache ───────────────────────────────────────────────
        {
            std::shared_ptr<const RaycastMeshCache> cached;
            {
                std::lock_guard<std::mutex> lk(this->meshCacheMutex);
                auto it = this->meshCache.find(meshHandle);
                if (it != this->meshCache.end())
                {
                    cached = it->second;
                }
            }
            if (cached)
            {
                vertexCount = cached->localVerts.size();
                indexCount = cached->indices.size();
                isVET_HALF4 = cached->isVET_HALF4;
                isIndices32 = cached->isIndices32;
                if (0 == vertexCount || 0 == indexCount)
                {
                    return;
                }

                // Compute matrix for normals that handles both rotation and non-uniform scaling
                Ogre::Matrix3 rotMatrix;
                orientation.ToRotationMatrix(rotMatrix);

                // Create a scaling matrix for normal transformation
                Ogre::Matrix3 scaleMatrix;
                scaleMatrix[0][0] = scale.x;
                scaleMatrix[1][1] = scale.y;
                scaleMatrix[2][2] = scale.z;

                // Combine rotation and scaling, then compute inverse-transpose
                Ogre::Matrix3 normalMatrix = rotMatrix * scaleMatrix;
                Ogre::Matrix3 invNormalMatrix = normalMatrix.Inverse().Transpose();

                vertices = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
                indices = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);
                for (size_t i = 0; i < vertexCount; ++i)
                {
                    vertices[i] = (orientation * (cached->localVerts[i] * scale)) + position;
                }
                for (size_t i = 0; i < indexCount; ++i)
                {
                    indices[i] = cached->indices[i];
                }
                return;
            }
        }

        // ── 2. Cache miss — GPU readback on render thread (one-time cost) ─────────
        auto newData = std::make_shared<RaycastMeshCache>();
        {
            Ogre::MeshPtr meshCopy = mesh;
            RaycastMeshCache* dst = newData.get();

            NOWA::GraphicsModule::RenderCommand gpuReadback = [meshCopy, dst]()
            {
                for (const auto& subMesh : meshCopy->getSubMeshes())
                {
                    Ogre::VertexArrayObjectArray vaos = subMesh->mVao[0];
                    if (vaos.empty())
                    {
                        continue;
                    }

                    Ogre::VertexArrayObject* vao = vaos[0];
                    Ogre::IndexBufferPacked* ib = vao->getIndexBuffer();
                    if (nullptr == ib || 0 == ib->getNumElements())
                    {
                        continue;
                    }

                    dst->isIndices32 = (ib->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);

                    // ── Vertex positions ──────────────────────────────────────────
                    Ogre::VertexArrayObject::ReadRequestsVec requests;
                    requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
                    vao->readRequests(requests);
                    vao->mapAsyncTickets(requests);

                    const size_t indexBase = dst->localVerts.size();
                    const size_t numVerts = requests[0].vertexBuffer->getNumElements();

                    // Check vertex format and set isVET_HALF4 appropriately
                    dst->isVET_HALF4 = (requests[0].type == Ogre::VET_HALF4);

                    // Assuming interleaved data where:
                    // position is first (x, y, z), followed by normal (nx, ny, nz) for each vertex
                    if (requests[0].type == Ogre::VET_HALF4)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            const Ogre::uint16* data = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
                            // Extract position
                            Ogre::Vector3 pos;
                            pos.x = Ogre::Bitwise::halfToFloat(data[0]);
                            pos.y = Ogre::Bitwise::halfToFloat(data[1]);
                            pos.z = Ogre::Bitwise::halfToFloat(data[2]);
                            // Update data pointers to next vertex (for interleaved data)
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                            dst->localVerts.push_back(pos);
                        }
                    }
                    else if (requests[0].type == Ogre::VET_FLOAT3)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            // Read position data
                            const float* pos = reinterpret_cast<const float*>(requests[0].data);
                            Ogre::Vector3 vertexPosition;
                            vertexPosition.x = *pos++;
                            vertexPosition.y = *pos++;
                            vertexPosition.z = *pos++;
                            // Apply transformations - fixed to use the input position parameter correctly
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                            dst->localVerts.push_back(vertexPosition);
                        }
                    }
                    else if (requests[0].type == Ogre::VET_FLOAT4)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            const float* pos = reinterpret_cast<const float*>(requests[0].data);
                            Ogre::Vector3 vec;
                            vec.x = pos[0];
                            vec.y = pos[1];
                            vec.z = pos[2];
                            // pos[3] = w (often 1.0f) -> ignore
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                            dst->localVerts.push_back(vec);
                        }
                    }
                    else
                    {
                        vao->unmapAsyncTickets(requests);
                        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Unsupported vertex format!", "getDetailedMeshInformation");
                    }

                    vao->unmapAsyncTickets(requests);

                    // ── Index readback ────────────────────────────────────────────
                    Ogre::AsyncTicketPtr asyncTicket = ib->readRequest(0, ib->getNumElements());
                    if (dst->isIndices32)
                    {
                        const unsigned int* pIndices = reinterpret_cast<const unsigned int*>(asyncTicket->map());
                        for (size_t i = 0; i < ib->getNumElements(); ++i)
                        {
                            dst->indices.push_back(static_cast<unsigned long>(pIndices[i]) + static_cast<unsigned long>(indexBase));
                        }
                        asyncTicket->unmap();
                    }
                    else
                    {
                        const unsigned short* pShortIndices = reinterpret_cast<const unsigned short*>(asyncTicket->map());
                        for (size_t i = 0; i < ib->getNumElements(); ++i)
                        {
                            dst->indices.push_back(static_cast<unsigned long>(pShortIndices[i]) + static_cast<unsigned long>(indexBase));
                        }
                        asyncTicket->unmap();
                    }
                }
            };

            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(gpuReadback), "MathHelper::getDetailedMeshInformation::gpuCache");
        }

        {
            std::lock_guard<std::mutex> lk(this->meshCacheMutex);
            this->meshCache[meshHandle] = newData;
        }

        // ── Serve from freshly populated cache ─────────────────────────────────
        vertexCount = newData->localVerts.size();
        indexCount = newData->indices.size();
        isVET_HALF4 = newData->isVET_HALF4;
        isIndices32 = newData->isIndices32;
        if (0 == vertexCount || 0 == indexCount)
        {
            return;
        }

        // Compute matrix for normals that handles both rotation and non-uniform scaling
        Ogre::Matrix3 rotMatrix;
        orientation.ToRotationMatrix(rotMatrix);

        // Create a scaling matrix for normal transformation
        Ogre::Matrix3 scaleMatrix;
        scaleMatrix[0][0] = scale.x;
        scaleMatrix[1][1] = scale.y;
        scaleMatrix[2][2] = scale.z;

        // Combine rotation and scaling, then compute inverse-transpose
        Ogre::Matrix3 normalMatrix = rotMatrix * scaleMatrix;
        Ogre::Matrix3 invNormalMatrix = normalMatrix.Inverse().Transpose();

        vertices = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
        indices = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);
        for (size_t i = 0; i < vertexCount; ++i)
        {
            vertices[i] = (orientation * (newData->localVerts[i] * scale)) + position;
        }
        for (size_t i = 0; i < indexCount; ++i)
        {
            indices[i] = newData->indices[i];
        }
    }

	void MathHelper::getDetailedMeshInformation2(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices, Ogre::Vector3*& normals,
        Ogre::Vector2*& textureCoords, // Added parameter for texture coordinates
        size_t& indexCount, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale, bool& isVET_HALF4, bool& isIndices32)
    {
        // Safe null outputs
        vertexCount = indexCount = 0;
        vertices = normals = nullptr;
        textureCoords = nullptr;
        indices = nullptr;
        isVET_HALF4 = isIndices32 = false;

        if (mesh.isNull())
        {
            return;
        }

        const size_t meshHandle = mesh->getHandle();

        // ── 1. Fast path: CPU cache ───────────────────────────────────────────────
        {
            std::shared_ptr<const DetailedMeshCache> cached;
            {
                std::lock_guard<std::mutex> lk(this->meshCacheMutex);
                auto it = this->detailedMeshCache.find(meshHandle);
                if (it != this->detailedMeshCache.end())
                {
                    cached = it->second;
                }
            }
            if (cached)
            {
                vertexCount = cached->localVerts.size();
                indexCount = cached->indices.size();
                isVET_HALF4 = cached->isVET_HALF4;
                isIndices32 = cached->isIndices32;
                if (0 == vertexCount || 0 == indexCount)
                {
                    return;
                }

                // Compute matrix for normals that handles both rotation and non-uniform scaling
                Ogre::Matrix3 rotMatrix;
                orientation.ToRotationMatrix(rotMatrix);

                // Create a scaling matrix for normal transformation
                Ogre::Matrix3 scaleMatrix;
                scaleMatrix[0][0] = scale.x;
                scaleMatrix[1][1] = scale.y;
                scaleMatrix[2][2] = scale.z;

                // Combine rotation and scaling, then compute inverse-transpose
                Ogre::Matrix3 normalMatrix = rotMatrix * scaleMatrix;
                Ogre::Matrix3 invNormalMatrix = normalMatrix.Inverse().Transpose();

                vertices = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
                normals = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
                textureCoords = OGRE_ALLOC_T(Ogre::Vector2, vertexCount, Ogre::MEMCATEGORY_GEOMETRY); // Allocate for texture coordinates
                indices = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);

                for (size_t i = 0; i < vertexCount; ++i)
                {
                    // Store the transformed data - fixed to use the input position parameter correctly
                    vertices[i] = (orientation * (cached->localVerts[i] * scale)) + position;
                    normals[i] = invNormalMatrix * cached->localNormals[i];
                    textureCoords[i] = cached->localUVs[i]; // Store texture coordinates
                }
                for (size_t i = 0; i < indexCount; ++i)
                {
                    indices[i] = cached->indices[i];
                }
                return;
            }
        }

        // ── 2. Cache miss — GPU readback on render thread (one-time cost) ─────────
        auto newData = std::make_shared<DetailedMeshCache>();
        {
            Ogre::MeshPtr meshCopy = mesh;
            DetailedMeshCache* dst = newData.get();

            NOWA::GraphicsModule::RenderCommand gpuReadback = [meshCopy, dst]()
            {
                for (const auto& subMesh : meshCopy->getSubMeshes())
                {
                    Ogre::VertexArrayObjectArray vaos = subMesh->mVao[0];
                    if (vaos.empty())
                    {
                        continue;
                    }

                    Ogre::VertexArrayObject* vao = vaos[0];
                    Ogre::IndexBufferPacked* ib = vao->getIndexBuffer();
                    if (nullptr == ib || 0 == ib->getNumElements())
                    {
                        continue;
                    }

                    dst->isIndices32 = (ib->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);

                    Ogre::VertexArrayObject::ReadRequestsVec requests;
                    requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
                    requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_NORMAL));
                    requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_TEXTURE_COORDINATES)); // Add request for texture coordinates
                    vao->readRequests(requests);
                    vao->mapAsyncTickets(requests);

                    const size_t indexBase = dst->localVerts.size();
                    const size_t numVerts = requests[0].vertexBuffer->getNumElements();

                    // Check vertex format and set isVET_HALF4 appropriately
                    dst->isVET_HALF4 = (requests[0].type == Ogre::VET_HALF4);

                    // Assuming interleaved data where:
                    // position is first (x, y, z), followed by normal (nx, ny, nz) for each vertex
                    if (requests[0].type == Ogre::VET_HALF4)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            const Ogre::uint16* data = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
                            // Extract position
                            Ogre::Vector3 pos;
                            pos.x = Ogre::Bitwise::halfToFloat(data[0]);
                            pos.y = Ogre::Bitwise::halfToFloat(data[1]);
                            pos.z = Ogre::Bitwise::halfToFloat(data[2]);

                            // Extract normal
                            const Ogre::uint16* norm = reinterpret_cast<const Ogre::uint16*>(requests[1].data);
                            Ogre::Vector3 n;
                            n.x = Ogre::Bitwise::halfToFloat(norm[0]);
                            n.y = Ogre::Bitwise::halfToFloat(norm[1]);
                            n.z = Ogre::Bitwise::halfToFloat(norm[2]);

                            // Extract texture coordinates
                            const Ogre::uint16* texCoords = reinterpret_cast<const Ogre::uint16*>(requests[2].data);
                            Ogre::Vector2 uv;
                            uv.x = Ogre::Bitwise::halfToFloat(texCoords[0]);
                            uv.y = Ogre::Bitwise::halfToFloat(texCoords[1]);

                            // Update data pointers to next vertex (for interleaved data)
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                            requests[1].data += requests[1].vertexBuffer->getBytesPerElement();
                            requests[2].data += requests[2].vertexBuffer->getBytesPerElement();

                            dst->localVerts.push_back(pos);
                            dst->localNormals.push_back(n);
                            dst->localUVs.push_back(uv);
                        }
                    }
                    else if (requests[0].type == Ogre::VET_FLOAT3)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            // Read position data
                            const float* pos = reinterpret_cast<const float*>(requests[0].data);
                            Ogre::Vector3 vertexPosition;
                            vertexPosition.x = *pos++;
                            vertexPosition.y = *pos++;
                            vertexPosition.z = *pos++;
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();

                            // Read normal data
                            const float* norm = reinterpret_cast<const float*>(requests[1].data);
                            Ogre::Vector3 n;
                            n.x = *norm++;
                            n.y = *norm++;
                            n.z = *norm++;
                            requests[1].data += requests[1].vertexBuffer->getBytesPerElement();

                            // Read texture coordinate data
                            const float* texCoords = reinterpret_cast<const float*>(requests[2].data);
                            Ogre::Vector2 uv;
                            uv.x = *texCoords++;
                            uv.y = *texCoords++;
                            requests[2].data += requests[2].vertexBuffer->getBytesPerElement();

                            // Apply transformations - fixed to use the input position parameter correctly
                            dst->localVerts.push_back(vertexPosition);
                            dst->localNormals.push_back(n);
                            dst->localUVs.push_back(uv); // Store texture coordinates
                        }
                    }
                    else if (requests[0].type == Ogre::VET_FLOAT4)
                    {
                        for (size_t i = 0; i < numVerts; ++i)
                        {
                            // Read position data (float4: x,y,z,w) -> ignore w
                            const float* pos = reinterpret_cast<const float*>(requests[0].data);
                            Ogre::Vector3 vertexPosition;
                            vertexPosition.x = pos[0];
                            vertexPosition.y = pos[1];
                            vertexPosition.z = pos[2];
                            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();

                            // Read normal data
                            const float* norm = reinterpret_cast<const float*>(requests[1].data);
                            Ogre::Vector3 n;
                            n.x = norm[0];
                            n.y = norm[1];
                            n.z = norm[2];
                            requests[1].data += requests[1].vertexBuffer->getBytesPerElement();

                            // Read texture coordinate data
                            const float* texCoords = reinterpret_cast<const float*>(requests[2].data);
                            Ogre::Vector2 uv;
                            uv.x = texCoords[0];
                            uv.y = texCoords[1];
                            requests[2].data += requests[2].vertexBuffer->getBytesPerElement();

                            // Apply transformations
                            dst->localVerts.push_back(vertexPosition);
                            dst->localNormals.push_back(n);
                            dst->localUVs.push_back(uv);
                        }
                    }
                    else
                    {
                        vao->unmapAsyncTickets(requests);
                        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Unsupported vertex format!", "getDetailedMeshInformation2");
                    }

                    vao->unmapAsyncTickets(requests);

                    // ── Index readback ────────────────────────────────────────────
                    Ogre::AsyncTicketPtr asyncTicket = ib->readRequest(0, ib->getNumElements());
                    if (dst->isIndices32)
                    {
                        const unsigned int* pIndices = reinterpret_cast<const unsigned int*>(asyncTicket->map());
                        for (size_t i = 0; i < ib->getNumElements(); ++i)
                        {
                            dst->indices.push_back(static_cast<unsigned long>(pIndices[i]) + static_cast<unsigned long>(indexBase));
                        }
                        asyncTicket->unmap();
                    }
                    else
                    {
                        const unsigned short* pShortIndices = reinterpret_cast<const unsigned short*>(asyncTicket->map());
                        for (size_t i = 0; i < ib->getNumElements(); ++i)
                        {
                            dst->indices.push_back(static_cast<unsigned long>(pShortIndices[i]) + static_cast<unsigned long>(indexBase));
                        }
                        asyncTicket->unmap();
                    }
                }
            };

            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(gpuReadback), "MathHelper::getDetailedMeshInformation2::gpuCache");
        }

        {
            std::lock_guard<std::mutex> lk(this->meshCacheMutex);
            this->detailedMeshCache[meshHandle] = newData;
        }

        // ── Serve from freshly populated cache ─────────────────────────────────
        vertexCount = newData->localVerts.size();
        indexCount = newData->indices.size();
        isVET_HALF4 = newData->isVET_HALF4;
        isIndices32 = newData->isIndices32;
        if (0 == vertexCount || 0 == indexCount)
        {
            return;
        }

        // Compute matrix for normals that handles both rotation and non-uniform scaling
        Ogre::Matrix3 rotMatrix;
        orientation.ToRotationMatrix(rotMatrix);

        // Create a scaling matrix for normal transformation
        Ogre::Matrix3 scaleMatrix;
        scaleMatrix[0][0] = scale.x;
        scaleMatrix[1][1] = scale.y;
        scaleMatrix[2][2] = scale.z;

        // Combine rotation and scaling, then compute inverse-transpose
        Ogre::Matrix3 normalMatrix = rotMatrix * scaleMatrix;
        Ogre::Matrix3 invNormalMatrix = normalMatrix.Inverse().Transpose();

        vertices = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
        normals = OGRE_ALLOC_T(Ogre::Vector3, vertexCount, Ogre::MEMCATEGORY_GEOMETRY);
        textureCoords = OGRE_ALLOC_T(Ogre::Vector2, vertexCount, Ogre::MEMCATEGORY_GEOMETRY); // Allocate for texture coordinates
        indices = OGRE_ALLOC_T(unsigned long, indexCount, Ogre::MEMCATEGORY_GEOMETRY);

        for (size_t i = 0; i < vertexCount; ++i)
        {
            // Store the transformed data - fixed to use the input position parameter correctly
            vertices[i] = (orientation * (newData->localVerts[i] * scale)) + position;
            normals[i] = invNormalMatrix * newData->localNormals[i];
            textureCoords[i] = newData->localUVs[i]; // Store texture coordinates
        }
        for (size_t i = 0; i < indexCount; ++i)
        {
            indices[i] = newData->indices[i];
        }
    }

	void MathHelper::getManualMeshInformation(const Ogre::ManualObject* manualObject, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient,
        const Ogre::Vector3& scale)
    {
        // Use a unique mesh name per ManualObject pointer to avoid MeshManager
        // name collisions when this is called for multiple different ManualObjects
        // in the same frame (e.g. the editor selecting multiple objects).
        Ogre::String meshName = "Dummy_" + Ogre::StringConverter::toString(reinterpret_cast<size_t>(manualObject));

        // Remove any leftover mesh from a previous call for this ManualObject.
        // Its cache entry in meshCache remains valid (same handle is not reused).
        {
            Ogre::MeshPtr existing = Ogre::MeshManager::getSingleton().getByName(meshName, "General");
            if (!existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        }

        Ogre::MeshPtr meshPtr = Ogre::MeshManager::getSingleton().createManual(meshName, "General");

        // Create sub mesh from section
        for (unsigned int i = 0; i < manualObject->getNumSections(); i++)
        {
            Ogre::SubMesh* subMesh = meshPtr->createSubMesh();
            Ogre::ManualObject::ManualObjectSection* section = manualObject->getSection(i);

            // Each Vao pushed to the vector refers to an LOD level.
            subMesh->mVao[0].push_back(section->getVaos(Ogre::VertexPass::VpNormal)[0]); // main buffer
            subMesh->mVao[1].push_back(section->getVaos(Ogre::VertexPass::VpShadow)[0]); // shadow buffer
        }

        // get the mesh information
        this->getMeshInformation(meshPtr, vertexCount, vertices, indexCount, indices, manualObject->getParentNode()->_getDerivedPositionUpdated(), manualObject->getParentNode()->_getDerivedOrientationUpdated(),
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
			if (type.compare("Item") == 0)
			{
				// Get the item to check
				Ogre::Item* item = static_cast<Ogre::Item*>(queryResult[qrIdx].movable);
				bool foundExcludedOne = false;
				// If its the exclude item, continue the loop with a different ones
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
                this->getMeshInformation(item->getMesh(), vertexCount, vertices, indexCount, indices, item->getParentNode()->_getDerivedPositionUpdated(), item->getParentNode()->_getDerivedOrientationUpdated(),
                    item->getParentNode()->_getDerivedScaleUpdated());

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
				Ogre::Item* item = static_cast<Ogre::Item*>(queryResult[qrIdx].movable);
                Ogre::String itemName = item->getName();

				bool foundExcludedOne = false;
				// If its the exclude entity, continue the loop with a different ones
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

				// Exclude gizmo
                if ("XArrowGizmoItem" == itemName || "YArrowGizmoItem" == itemName || "ZArrowGizmoItem" == itemName || "SphereGizmoItem" == itemName)
					continue;

				// mesh data to retrieve         
				size_t vertexCount;
				size_t indexCount;
				Ogre::Vector3* vertices;
				unsigned long* indices;

				// get the mesh information
                this->getMeshInformation(item->getMesh(), vertexCount, vertices, indexCount, indices, item->getParentNode()->_getDerivedPosition(), item->getParentNode()->_getDerivedOrientation(), item->getParentNode()->_getDerivedScale());

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

			if (type.compare("Item") == 0)
			{
				Ogre::Item* item = dynamic_cast<Ogre::Item*>(queryResult[qrIdx].movable);
				if (nullptr != item)
				{
					// If its the exclude entity, continue the loop with a different ones
					bool foundExcludedOne = false;
					Ogre::String itemName = item->getName();

					// Exclude gizmo
					if ("XArrowGizmoItem" == itemName || "YArrowGizmoItem" == itemName || "ZArrowGizmoItem" == itemName || "SphereGizmoItem" == itemName)
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
					this->getMeshInformation(item->getMesh(), vertexCount, vertices,
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

	bool MathHelper::getRaycastDetailsResult(const Ogre::Ray& ray, Ogre::RaySceneQuery* raySceneQuery, Ogre::Vector3& result, size_t& targetMovableObject,
		Ogre::Vector3*& outVertices, size_t& outVertexCount, unsigned long*& outIndices, size_t& outIndexCount)
	{
		result = Ogre::Vector3::ZERO;
		targetMovableObject = 0;
		outVertices = nullptr;
		outIndices = nullptr;
		outVertexCount = 0;
		outIndexCount = 0;
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

			if (type.compare("Item") == 0)
			{
				Ogre::Item* item = dynamic_cast<Ogre::Item*>(queryResult[qrIdx].movable);
				if (nullptr != item)
				{
					// If its the exclude entity, continue the loop with a different ones
					bool foundExcludedOne = false;
					Ogre::String itemName = item->getName();

					// Exclude gizmo
					if ("XArrowGizmoItem" == itemName || "YArrowGizmoItem" == itemName || "ZArrowGizmoItem" == itemName || "SphereGizmoItem" == itemName)
					{
						continue;
					}

					// mesh data to retrieve
					size_t vertexCount;
					size_t indexCount;
					Ogre::Vector3* vertices = nullptr;
					unsigned long* indices = nullptr;

					// get the mesh information
					this->getMeshInformation(item->getMesh(), vertexCount, vertices,
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

					// if we found a new closest raycast for this object, update the
					// closestResult before moving on to the next object.
					if (true == newClosestFound)
					{
						targetMovableObject = (size_t)item;
						outVertexCount = vertexCount;
						outIndexCount = indexCount;
						outVertices = vertices;
						outIndices = indices;

						closestResult = raySceneQuery->getRay().getPoint(closestDistance);
					}
					else
					{
						// free the verticies and indicies memory
						OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
						OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);
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

			// only check this result if its a hit against an item
			if (type.compare("Item") == 0)
			{
				// get the entity to check
				Ogre::Item* item = static_cast<Ogre::Item*>(queryResult[qrIdx].movable);

				// mesh data to retrieve
				size_t vertexCount;
				size_t indexCount;
				Ogre::Vector3* vertices;
				unsigned long* indices;

				// get the mesh information
                this->getMeshInformation(item->getMesh(), vertexCount, vertices, indexCount, indices, item->getParentNode()->_getDerivedPositionUpdated(), item->getParentNode()->_getDerivedOrientationUpdated(), item->getParentNode()->getScale());

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
            {
                continue;
            }

            // Stop checking if we have found a raycast hit that is closer than all remaining entities
            if (closestDistance >= 0.0f && closestDistance < queryResult[qrIdx].distance && false == forGizmo)
            {
                break;
            }

            // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MathHelper]: Found type : " + type + " name: " +  queryResult[qrIdx].movable->getName());

            bool foundType = false;

            bool newClosestFound = false;

            // get the entity to check

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
                    if ("XArrowGizmoItem" != itemName && "YArrowGizmoItem" != itemName && "ZArrowGizmoItem" != itemName && "SphereGizmoItem" != itemName)
                    {
                        continue;
                    }
                    else
                    {
                        foundGizmo = true;
                    }
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
                this->getMeshInformation(item->getMesh(), vertexCount, vertices, indexCount, indices, item->getParentNode()->_getDerivedPositionUpdated(), item->getParentNode()->_getDerivedOrientationUpdated(), item->getParentNode()->getScale());

                // test for hitting individual triangles on the mesh
                for (int i = 0; i < static_cast<int>(indexCount); i += 3)
                {
                    // check for a hit against this triangle
                    std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(raySceneQuery->getRay(), vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

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
                            {
                                break;
                            }

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
                    {
                        return true;
                    }
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
                    this->getManualMeshInformation(manualObject, vertexCount, vertices, indexCount, indices, manualObject->getParentNode()->_getDerivedPositionUpdated(), manualObject->getParentNode()->_getDerivedOrientationUpdated(),
                        manualObject->getParentNode()->getScale());

                    // test for hitting individual triangles on the mesh
                    for (int i = 0; i < static_cast<int>(indexCount); i += 3)
                    {
                        // check for a hit against this triangle
                        std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(raySceneQuery->getRay(), vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

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

    bool MathHelper::getRaycastForFrame(int mouseX, int mouseY, Ogre::Camera* camera, Ogre::Window* renderWindow, Ogre::RaySceneQuery* raySceneQuery, std::vector<Ogre::MovableObject*>& excludeObjects, Ogre::Vector3& outPosition)
    {
        // If another worker already computed the raycast for this exact mouse
        // position, return the cached result immediately — no second raycast needed.
        // Mouse position only changes when the user actually moves the mouse,
        // so this is a reliable key: same pixel = same world position.
        if (true == this->cachedRaycast.valid && this->cachedRaycast.cachedMouseX == mouseX && this->cachedRaycast.cachedMouseY == mouseY)
        {
            outPosition = this->cachedRaycast.clickedPosition;
            return true;
        }

        // If a raycast is already in progress for a different position,
        // return the last known valid result to avoid stalling
        if (true == this->cachedRaycast.inProgress)
        {
            if (true == this->cachedRaycast.valid)
            {
                outPosition = this->cachedRaycast.clickedPosition;
                return true;
            }
            return false;
        }

        // New mouse position — perform raycast
        this->cachedRaycast.inProgress = true;
        this->cachedRaycast.valid = false;
        this->cachedRaycast.cachedMouseX = mouseX;
        this->cachedRaycast.cachedMouseY = mouseY;

        size_t movableObjLocal = 0;
        Ogre::Vector3 normalLocal = Ogre::Vector3::ZERO;
        float closestDistLocal = 0.0f;

        bool success = this->getRaycastFromPoint(mouseX, mouseY, camera, renderWindow, raySceneQuery, outPosition, movableObjLocal, closestDistLocal, normalLocal, &excludeObjects, false);

        this->cachedRaycast.valid = success;
        this->cachedRaycast.clickedPosition = outPosition;
        this->cachedRaycast.inProgress = false;

        return success;
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

	void MathHelper::tweakUnlitDatablock(const Ogre::String& datablockName)
	{
		Ogre::Hlms* hlms = Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
		Ogre::HlmsUnlit* hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlms);

		Ogre::HlmsBlendblock blendblock;
		blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA); // Example: Transparent alpha blending

		Ogre::HlmsMacroblock macroblock;
		macroblock.mCullMode = Ogre::CULL_NONE; // Example: No culling
		macroblock.mDepthCheck = true;
		macroblock.mDepthWrite = true;

		// Check if datablock already exists
		Ogre::HlmsDatablock* existingDatablock = hlmsUnlit->getDatablock(datablockName);

		if (false == existingDatablock)
		{
			Ogre::HlmsUnlitDatablock* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
				hlmsUnlit->createDatablock(datablockName, datablockName, Ogre::HlmsMacroblock(macroblock), Ogre::HlmsBlendblock(blendblock), Ogre::HlmsParamVec()));

			existingDatablock = datablock;
		}

		existingDatablock->setMacroblock(macroblock);
		existingDatablock->setBlendblock(blendblock);

		// Set color usage
		static_cast<Ogre::HlmsUnlitDatablock*>(existingDatablock)->setUseColour(true); // Enable manual colour
	}

}; // namespace end