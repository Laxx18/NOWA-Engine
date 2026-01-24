/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

by Walaber
some changes by melven
adapted for Newton Dynamics 4.0

*/
#ifndef _INCLUDE_OGRENEWT_COLLISION
#define _INCLUDE_OGRENEWT_COLLISION


#include "OgreNewt_Prerequisites.h"


// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{

	enum CollisionPrimitiveType
	{
		BoxPrimitiveType,
		ConePrimitiveType,
		EllipsoidPrimitiveType,
		CapsulePrimitiveType,
		CylinderPrimitiveType,
		CompoundCollisionPrimitiveType,
		ConvexHullPrimitiveType,
		ChamferCylinderPrimitiveType,
		TreeCollisionPrimitiveType,
		NullPrimitiveType,
		HeighFieldPrimitiveType,
		ScenePrimitiveType,
		DeformableType,
		ConcaveHullPrimitiveType
	};

	/*
	CLASS DEFINITION:

	Collision

	USE:
	this class represents a ndShape, which is the newton 4.0 structure
	for collision geometry.
	*/
	//! represents a shape for collision detection
	class _OgreNewtExport Collision
	{
	public:

		friend class CollisionSerializer;

		//! constructor
		Collision(const World* world, bool ownsNewtonCollision = true);

		//! constructor
		Collision(const Collision& shape, bool ownsNewtonCollision = true);

		//! constructor
		Collision(ndShape* collision, const World* world, bool ownsNewtonCollision = true);

		//! destructor
		virtual ~Collision();

		//! retrieve the Newton pointer
		/*!
		retrieves the pointer to the ndShape object.
		*/
		//! const getter für Newton Shape
		const ndShape* getNewtonConstCollision() const
		{
			return m_col;
		}

		//! non-const getter für Newton Shape
		ndShape* getNewtonCollision()
		{
			return const_cast<ndShape*>(m_col);
		}

		ndShapeInstance* getShapeInstance()
		{
			return m_shapeInstance;
		}

		const ndShapeInstance* getShapeInstance() const
		{
			return m_shapeInstance;
		}

		/*!
		Returns the Newton world this collision belongs to.
		*/
		const World* getWorld() const
		{
			return m_world;
		}

		//! get user ID, for collision callback identification
		long getUserID() const;

		//! make unique
		void makeUnique();

		//! get the Axis-Aligned Bounding Box for this collision shape.
		/*!
		* \warning The returned AABB can be too large! If you need an AABB that fits exactly use the OgreNewt::CollisionTools::CalculateFittingAABB function
		*/
		Ogre::AxisAlignedBox getAABB(const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO) const;

		//! Returns the Collisiontype for this Collision
		CollisionPrimitiveType getCollisionPrimitiveType() const
		{
			return getCollisionPrimitiveType(m_col);
		}

		//! Returns the Collisiontype for the given Newton-Collision
		static CollisionPrimitiveType getCollisionPrimitiveType(const ndShape* col);

		//! Scale the collision shape
		void scaleCollision(const Ogre::Vector3& scale);

		//! friend functions for the Serializer
		friend class OgreNewt::CollisionSerializer;

	protected:

		const ndShape* m_col;
		ndShapeInstance* m_shapeInstance = nullptr;
		bool m_OwnsNewtonCollision;
		const World* m_world;

	};



	//! represents a collision shape that is explicitly convex.
	class _OgreNewtExport ConvexCollision : public Collision
	{
	public:
		//! constructor
		ConvexCollision(const World* world);

		//! constructor
		ConvexCollision(const Collision& convexShape);

		//! constructor
		ConvexCollision(const ConvexCollision& convexShape);

		//! destructor
		~ConvexCollision();

		//! calculate the volume of the collision shape, useful for buoyancy calculations.
		Ogre::Real calculateVolume() const;

		//! calculate the moment of inertia for this collision primitive, along with the theoretical center-of-mass for this shape.
		void calculateInertialMatrix(Ogre::Vector3& inertia, Ogre::Vector3& offset) const;
	};


#ifdef OGRENEWT_NO_COLLISION_SHAREDPTR
	typedef Collision* CollisionPtr;
	typedef ConvexCollision* ConvexCollisionPtr;
#else
	typedef OgreNewt::shared_ptr<Collision> CollisionPtr;
	typedef OgreNewt::shared_ptr<ConvexCollision> ConvexCollisionPtr;
#endif


}   // end NAMESPACE OgreNewt

#endif
	// _INCLUDE_OGRENEWT_COLLISION
