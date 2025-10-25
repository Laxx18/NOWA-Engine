#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Tools.h"

namespace OgreNewt
{
	Collision::Collision(const World* world, bool ownsNewtonCollision)
		: m_col(nullptr),
		m_OwnsNewtonCollision(ownsNewtonCollision),
		m_world(world)
	{
	}

	Collision::Collision(const Collision& shape, bool ownsNewtonCollision)
		: m_col(shape.m_col),
		m_OwnsNewtonCollision(ownsNewtonCollision),
		m_world(shape.m_world)
	{
		if (m_col && ownsNewtonCollision)
		{
			m_col = m_col->AddRef();
		}
	}

	Collision::Collision(ndShape* collision, const World* world, bool ownsNewtonCollision)
		: m_col(collision),
		m_OwnsNewtonCollision(ownsNewtonCollision),
		m_world(world)
	{
		if (m_col && ownsNewtonCollision)
		{
			m_col = m_col->AddRef();
		}
	}

	Collision::~Collision()
	{
		if (m_world->getNewtonWorld() && m_col && m_OwnsNewtonCollision)
		{
			m_col->Release();
			m_col = nullptr;
		}
	}

	long Collision::getUserID() const
	{
		if (m_col)
		{
			ndShapeInfo info = m_col->GetShapeInfo();
			return info.m_collisionType;
		}
		return 0;
	}

	void Collision::makeUnique()
	{
		if (m_col)
		{
			const ndShape* newCol = m_col->AddRef();
			if (m_OwnsNewtonCollision)
			{
				m_col->Release();
			}
			m_col = newCol;
			m_OwnsNewtonCollision = true;
		}
	}

	Ogre::AxisAlignedBox Collision::getAABB(const Ogre::Quaternion& orient, const Ogre::Vector3& pos) const
	{
		Ogre::AxisAlignedBox box;

		if (m_col)
		{
			ndMatrix matrix;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, matrix);

			ndVector p0, p1;
			m_col->CalculateAabb(matrix, p0, p1);

			box.setMinimum(Ogre::Vector3(p0.m_x, p0.m_y, p0.m_z));
			box.setMaximum(Ogre::Vector3(p1.m_x, p1.m_y, p1.m_z));
		}
		return box;
	}

	CollisionPrimitiveType Collision::getCollisionPrimitiveType(const ndShape* col)
	{
		if (!col) return NullPrimitiveType;

		if (const_cast<ndShape*>(col)->GetAsShapeBox()) return BoxPrimitiveType;
		if (const_cast<ndShape*>(col)->GetAsShapeSphere()) return EllipsoidPrimitiveType;
		if (const_cast<ndShape*>(col)->GetAsShapeCylinder()) return CylinderPrimitiveType;
		if (const_cast<ndShape*>(col)->GetAsShapeCapsule()) return CapsulePrimitiveType;
		if (const_cast<ndShape*>(col)->GetAsShapeCone()) return ConePrimitiveType;
		if (const_cast<ndShape*>(col)->GetAsShapeConvexHull()) return ConvexHullPrimitiveType;
		if (const_cast<ndShape*>(col)->GetAsShapeCompound()) return CompoundCollisionPrimitiveType;
		if (const_cast<ndShape*>(col)->GetAsShapeStaticMesh()) return TreeCollisionPrimitiveType;

		return NullPrimitiveType;
	}

	void Collision::scaleCollision(const Ogre::Vector3& scale)
	{
		// This would need to be applied at the body level through ndShapeInstance
		// For now, this is a placeholder
	}

	ConvexCollision::ConvexCollision(const OgreNewt::World* world) : Collision(world)
	{
	}

	ConvexCollision::ConvexCollision(const Collision& convexShape) : Collision(convexShape)
	{
	}

	ConvexCollision::ConvexCollision(const ConvexCollision& convexShape) : Collision(convexShape)
	{
	}

	ConvexCollision::~ConvexCollision()
	{
	}

	Ogre::Real ConvexCollision::calculateVolume() const
	{
		if (m_col)
		{
			return Ogre::Real(m_col->GetVolume());
		}
		return 0.0f;
	}

	void ConvexCollision::calculateInertialMatrix(Ogre::Vector3& inertia, Ogre::Vector3& offset) const
	{
		if (m_col)
		{
			ndMatrix alignMatrix(ndGetIdentityMatrix());
			ndVector localScale(ndFloat32(1.0f), ndFloat32(1.0f), ndFloat32(1.0f), ndFloat32(1.0f));
			ndMatrix matrix(ndGetIdentityMatrix());

			ndMatrix inertiaMatrix = m_col->CalculateInertiaAndCenterOfMass(alignMatrix, localScale, matrix);

			// Extract diagonal inertia values from the returned matrix
			inertia.x = inertiaMatrix[0][0];
			inertia.y = inertiaMatrix[1][1];
			inertia.z = inertiaMatrix[2][2];

			// The center of mass is in the position part of the returned matrix
			offset.x = inertiaMatrix[3][0];
			offset.y = inertiaMatrix[3][1];
			offset.z = inertiaMatrix[3][2];
		}
	}
}
