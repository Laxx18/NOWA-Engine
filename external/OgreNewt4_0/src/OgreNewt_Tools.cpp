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
		void MatrixToQuatPos(const ndMatrix& matrix, Ogre::Quaternion& quat, Ogre::Vector3& pos)
		{
			// Convert Newton 4.0 ndMatrix to Ogre Quaternion and position
			using namespace Ogre;

			quat = Quaternion(Matrix3(
				matrix[0][0], matrix[0][1], matrix[0][2],
				matrix[1][0], matrix[1][1], matrix[1][2],
				matrix[2][0], matrix[2][1], matrix[2][2]
			));

			pos = Vector3(matrix.m_posit.m_x, matrix.m_posit.m_y, matrix.m_posit.m_z);
		}

		void QuatPosToMatrix(const Ogre::Quaternion& _quat, const Ogre::Vector3& pos, ndMatrix& matrix)
		{
			// Convert Ogre Quaternion and position to Newton 4.0 ndMatrix
			using namespace Ogre;
			Matrix3 rot;
			Vector3 xcol, ycol, zcol;

			Ogre::Quaternion quat(_quat);
			quat.normalise();
			quat.ToRotationMatrix(rot);

			xcol = rot.GetColumn(0);
			ycol = rot.GetColumn(1);
			zcol = rot.GetColumn(2);

			matrix[0][0] = xcol.x;
			matrix[1][0] = xcol.y;
			matrix[2][0] = xcol.z;
			matrix[3][0] = 0.0f;

			matrix[0][1] = ycol.x;
			matrix[1][1] = ycol.y;
			matrix[2][1] = ycol.z;
			matrix[3][1] = 0.0f;

			matrix[0][2] = zcol.x;
			matrix[1][2] = zcol.y;
			matrix[2][2] = zcol.z;
			matrix[3][2] = 0.0f;

			matrix.m_posit.m_x = pos.x;
			matrix.m_posit.m_y = pos.y;
			matrix.m_posit.m_z = pos.z;
			matrix.m_posit.m_w = 1.0f;
		}

		void MatrixToMatrix4(const ndMatrix& matrix_in, Ogre::Matrix4& matrix_out)
		{
			// Convert Newton 4.0 ndMatrix to Ogre::Matrix4
			matrix_out = Ogre::Matrix4(
				matrix_in[0][0], matrix_in[0][1], matrix_in[0][2], matrix_in[0][3],
				matrix_in[1][0], matrix_in[1][1], matrix_in[1][2], matrix_in[1][3],
				matrix_in[2][0], matrix_in[2][1], matrix_in[2][2], matrix_in[2][3],
				matrix_in[3][0], matrix_in[3][1], matrix_in[3][2], matrix_in[3][3]
			);
		}

		void Matrix4ToMatrix(const Ogre::Matrix4& matrix_in, ndMatrix& matrix_out)
		{
			// Convert Ogre::Matrix4 to Newton 4.0 ndMatrix
			matrix_out[0][0] = matrix_in[0][0];
			matrix_out[1][0] = matrix_in[1][0];
			matrix_out[2][0] = matrix_in[2][0];
			matrix_out[3][0] = matrix_in[3][0];

			matrix_out[0][1] = matrix_in[0][1];
			matrix_out[1][1] = matrix_in[1][1];
			matrix_out[2][1] = matrix_in[2][1];
			matrix_out[3][1] = matrix_in[3][1];

			matrix_out[0][2] = matrix_in[0][2];
			matrix_out[1][2] = matrix_in[1][2];
			matrix_out[2][2] = matrix_in[2][2];
			matrix_out[3][2] = matrix_in[3][2];

			matrix_out[0][3] = matrix_in[0][3];
			matrix_out[1][3] = matrix_in[1][3];
			matrix_out[2][3] = matrix_in[2][3];
			matrix_out[3][3] = matrix_in[3][3];
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