#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Tools.h"



namespace OgreNewt
{


	Collision::Collision(const World* world, bool ownsNewtonCollision)
		: m_col(nullptr),
		m_OwnsNewtonCollision(ownsNewtonCollision)
	{
		m_world = world;
	}

	Collision::Collision(const Collision& shape, bool ownsNewtonCollision)
		: m_col(shape.m_col),
		m_OwnsNewtonCollision(ownsNewtonCollision)
	{
		m_world = shape.m_world;
		if (m_col)
		{
			NewtonCollisionCreateInstance(m_col);
			NewtonCollisionSetUserData(m_col, this);
		}
	}

	Collision::Collision(const NewtonCollision* collision, const World* world, bool ownsNewtonCollision)
		: m_col((NewtonCollision*)collision),
		m_OwnsNewtonCollision(ownsNewtonCollision)
	{
		m_world = world;
		NewtonCollisionCreateInstance(m_col);
		NewtonCollisionSetUserData(m_col, this);
	}

	Collision::~Collision()
	{
		if (m_world->getNewtonWorld() && m_col && m_OwnsNewtonCollision)
		{
			NewtonDestroyCollision(m_col);
		}
	}


	Ogre::AxisAlignedBox Collision::getAABB(const Ogre::Quaternion& orient, const Ogre::Vector3& pos) const
	{
		Ogre::AxisAlignedBox box;

		if (m_col)
		{

			OgreNewt::CollisionPtr ptr(new OgreNewt::Collision(*this));
			box = CollisionTools::CollisionCalculateFittingAABB(ptr, orient, pos);
		}
		return box;
	}

	CollisionPrimitiveType Collision::getCollisionPrimitiveType(const NewtonCollision *col)
	{
		NewtonCollisionInfoRecord *info = new NewtonCollisionInfoRecord();

		NewtonCollisionGetInfo(col, info);

		return static_cast<CollisionPrimitiveType>(info->m_collisionType);
	}

	void Collision::scaleCollision(const Ogre::Vector3& scale)
	{
		NewtonCollisionSetScale(m_col, scale.x, scale.y, scale.z);
	}

	NewtonCollisionInfoRecord* Collision::getInfo(const NewtonCollision *col)
	{
		NewtonCollisionInfoRecord *info = new NewtonCollisionInfoRecord();

		NewtonCollisionGetInfo(col, info);

		return info;
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

}   // end NAMESPACE OgreNewt

