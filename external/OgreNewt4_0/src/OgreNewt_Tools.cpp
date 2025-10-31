#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include <iostream>

#ifdef __APPLE__
#   include <Ogre/OgreFontManager.h>
#   include <Ogre/OgreMaterialManager.h>
#   include <Ogre/OgreCamera.h>
#   include <Ogre/OgreHardwareBufferManager.h>
#   include <Ogre/OgreRoot.h>
#else
#   include <Components/Overlay/include/OgreFontManager.h>
#   include <OgreMaterialManager.h>
#   include <OgreCamera.h>
#   include <OgreHardwareBufferManager.h>
#   include <OgreRoot.h>
#endif

namespace OgreNewt
{
	namespace Converters
	{
		void MatrixToQuatPos(const ndMatrix& m, Ogre::Quaternion& quat, Ogre::Vector3& pos)
		{
			// ND4 layout: m_front (X), m_up (Y), m_right (Z), m_posit (translation)
			const Ogre::Vector3 front(m.m_front.m_x, m.m_front.m_y, m.m_front.m_z);
			const Ogre::Vector3 up(m.m_up.m_x, m.m_up.m_y, m.m_up.m_z);
			const Ogre::Vector3 right(m.m_right.m_x, m.m_right.m_y, m.m_right.m_z);

			Ogre::Matrix3 rot;
			rot.FromAxes(front, up, right);
			quat.FromRotationMatrix(rot);

			pos = Ogre::Vector3(m.m_posit.m_x, m.m_posit.m_y, m.m_posit.m_z);
		}

		void QuatPosToMatrix(const Ogre::Quaternion& quatIn, const Ogre::Vector3& pos, ndMatrix& outMatrix)
		{
			Ogre::Quaternion q = quatIn;
			q.normalise();

			Ogre::Matrix3 rot;
			q.ToRotationMatrix(rot);

			// Ogre::Matrix3::GetColumn(i) returns axis vectors in row-major form
			Ogre::Vector3 front = rot.GetColumn(0); // X-axis
			Ogre::Vector3 up = rot.GetColumn(1); // Y-axis
			Ogre::Vector3 right = rot.GetColumn(2); // Z-axis

			outMatrix.m_front = ndVector(front.x, front.y, front.z, 0.0f);
			outMatrix.m_up = ndVector(up.x, up.y, up.z, 0.0f);
			outMatrix.m_right = ndVector(right.x, right.y, right.z, 0.0f);
			outMatrix.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);
		}

		void MatrixToMatrix4(const ndMatrix& m, Ogre::Matrix4& out)
		{
			// Column 0: front (X)
			out[0][0] = m.m_front.m_x;  out[1][0] = m.m_front.m_y;  out[2][0] = m.m_front.m_z;  out[3][0] = 0.0f;
			// Column 1: up (Y)
			out[0][1] = m.m_up.m_x;     out[1][1] = m.m_up.m_y;     out[2][1] = m.m_up.m_z;     out[3][1] = 0.0f;
			// Column 2: right (Z)
			out[0][2] = m.m_right.m_x;  out[1][2] = m.m_right.m_y;  out[2][2] = m.m_right.m_z;  out[3][2] = 0.0f;
			// Column 3: translation
			out[0][3] = m.m_posit.m_x;  out[1][3] = m.m_posit.m_y;  out[2][3] = m.m_posit.m_z;  out[3][3] = 1.0f;
		}

		void Matrix4ToMatrix(const Ogre::Matrix4& in, ndMatrix& m)
		{
			m.m_front = ndVector(in[0][0], in[1][0], in[2][0], 0.0f);
			m.m_up = ndVector(in[0][1], in[1][1], in[2][1], 0.0f);
			m.m_right = ndVector(in[0][2], in[1][2], in[2][2], 0.0f);
			m.m_posit = ndVector(in[0][3], in[1][3], in[2][3], 1.0f);

#ifdef _DEBUG
			m.TestOrthogonal(1.0e-4f);
#endif
		}

		Ogre::Quaternion grammSchmidt(const Ogre::Vector3& pin)
		{
			Ogre::Vector3 front, up, right;
			front = pin;

			front.normalise();
			if (Ogre::Math::Abs(front.z) > 0.577f)
				right = front.crossProduct(Ogre::Vector3(-front.y, front.z, 0.0f));
			else
				right = front.crossProduct(Ogre::Vector3(-front.y, front.x, 0.0f));
			right.normalise();
			up = right.crossProduct(front);

			Ogre::Matrix3 ret;
			ret.FromAxes(front, up, right);

			Ogre::Quaternion quat;
			quat.FromRotationMatrix(ret);

			return quat;
		}

		ndVector OgreToNewtonVector(const Ogre::Vector3& vec)
		{
			return ndVector(vec.x, vec.y, vec.z, 0.0f);
		}

		Ogre::Vector3 NewtonToOgreVector(const ndVector& vec)
		{
			return Ogre::Vector3(vec.m_x, vec.m_y, vec.m_z);
		}
	}

	namespace CollisionTools
	{
		// The Newton 4.0 API has changed significantly, so these functions may need different approaches

		int CollisionPointDistance(const OgreNewt::World* world, const Ogre::Vector3& globalpt,
			const OgreNewt::CollisionPtr& col, const Ogre::Quaternion& colorient, const Ogre::Vector3& colpos,
			Ogre::Vector3& retpt, Ogre::Vector3& retnormal, int threadIndex)
		{
			// TODO: Implement using Newton 4.0 ndShape API
			// Newton 4.0 uses different collision query methods
			ndMatrix matrix;
			Converters::QuatPosToMatrix(colorient, colpos, matrix);

			// Placeholder - needs proper Newton 4.0 implementation
			retpt = colpos;
			retnormal = Ogre::Vector3::UNIT_Y;
			return 0;
		}

		int CollisionClosestPoint(const OgreNewt::World* world, const OgreNewt::CollisionPtr& colA, const Ogre::Quaternion& colOrientA, const Ogre::Vector3& colPosA,
			const OgreNewt::CollisionPtr& colB, const Ogre::Quaternion& colOrientB, const Ogre::Vector3& colPosB,
			Ogre::Vector3& retPosA, Ogre::Vector3& retPosB, Ogre::Vector3& retNorm, int threadIndex)
		{
			// TODO: Implement using Newton 4.0 ndShape API
			ndMatrix matrixA, matrixB;
			Converters::QuatPosToMatrix(colOrientA, colPosA, matrixA);
			Converters::QuatPosToMatrix(colOrientB, colPosB, matrixB);

			// Placeholder - needs proper Newton 4.0 implementation
			retPosA = colPosA;
			retPosB = colPosB;
			retNorm = Ogre::Vector3::UNIT_Y;
			return 0;
		}

		int CollisionCollide(const OgreNewt::World* world, int maxSize,
			const OgreNewt::CollisionPtr& colA, const Ogre::Quaternion& colOrientA, const Ogre::Vector3& colPosA,
			const OgreNewt::CollisionPtr& colB, const Ogre::Quaternion& colOrientB, const Ogre::Vector3& colPosB,
			Ogre::Vector3* retContactPts, Ogre::Vector3* retNormals, Ogre::Real* retPenetrations, int threadIndex)
		{
			// TODO: Implement using Newton 4.0 collision detection API
			ndMatrix matrixA, matrixB;
			Converters::QuatPosToMatrix(colOrientA, colPosA, matrixA);
			Converters::QuatPosToMatrix(colOrientB, colPosB, matrixB);

			// Placeholder - needs proper Newton 4.0 implementation
			return 0;
		}

		int CollisionCollideContinue(const OgreNewt::World* world, int maxSize, Ogre::Real timeStep,
			const OgreNewt::CollisionPtr& colA, const Ogre::Quaternion& colOrientA, const Ogre::Vector3& colPosA, const Ogre::Vector3& colVelA, const Ogre::Vector3& colOmegaA,
			const OgreNewt::CollisionPtr& colB, const Ogre::Quaternion& colOrientB, const Ogre::Vector3& colPosB, const Ogre::Vector3& colVelB, const Ogre::Vector3& colOmegaB,
			Ogre::Real& retTimeOfImpact, Ogre::Vector3* retContactPts, Ogre::Vector3* retNormals, Ogre::Real* retPenetrations, int threadIndex)
		{
			// TODO: Implement using Newton 4.0 continuous collision API
			ndMatrix matrixA, matrixB;
			Converters::QuatPosToMatrix(colOrientA, colPosA, matrixA);
			Converters::QuatPosToMatrix(colOrientB, colPosB, matrixB);

			// Placeholder - needs proper Newton 4.0 implementation
			retTimeOfImpact = 0.0f;
			return 0;
		}

		Ogre::Real CollisionRayCast(const OgreNewt::CollisionPtr& col, const Ogre::Vector3& startPt, const Ogre::Vector3& endPt,
			Ogre::Vector3& retNorm, int64_t& retColID)
		{
			// TODO: Implement using Newton 4.0 raycast API
			// Newton 4.0 uses ndShapeInstance::RayCast or similar methods

			// Placeholder - needs proper Newton 4.0 implementation
			retNorm = Ogre::Vector3::UNIT_Y;
			retColID = 0;
			return 1.0f;
		}

		Ogre::AxisAlignedBox CollisionCalculateFittingAABB(const OgreNewt::CollisionPtr& col, const Ogre::Quaternion& orient, const Ogre::Vector3& pos)
		{
			Ogre::Vector3 max, min;

			for (int i = 0; i < 3; i++)
			{
				Ogre::Vector3 currentDir, supportVertex;

				currentDir = Ogre::Vector3::ZERO;
				currentDir[i] = 1;
				currentDir = orient * currentDir;
				supportVertex = CollisionSupportVertex(col, currentDir) - pos;
				max[i] = supportVertex.dotProduct(currentDir);

				currentDir *= -1.0f;
				supportVertex = CollisionSupportVertex(col, currentDir) - pos;
				min[i] = -supportVertex.dotProduct(currentDir);
			}

			return Ogre::AxisAlignedBox(min, max);
		}

		Ogre::Vector3 CollisionSupportVertex(const OgreNewt::CollisionPtr& col, const Ogre::Vector3& dir)
		{
			// TODO: Implement using Newton 4.0 support vertex API
			// Newton 4.0 ndShape has SupportVertex methods

			// Placeholder - needs proper Newton 4.0 implementation
			return Ogre::Vector3::ZERO;
		}
	}

	namespace Springs
	{
		Ogre::Real calculateSpringDamperAcceleration(Ogre::Real deltaTime, Ogre::Real spingK,
			Ogre::Real stretchDistance, Ogre::Real springDamping, Ogre::Real dampVelocity)
		{
			// Spring-damper physics calculation (independent of Newton version)
			Ogre::Real springAccel = spingK * stretchDistance;
			Ogre::Real damperAccel = springDamping * dampVelocity;
			return springAccel - damperAccel;
		}
	}

	namespace OgreAddons
	{
		// MovableText implementation commented out - can be re-enabled if needed
	}
}