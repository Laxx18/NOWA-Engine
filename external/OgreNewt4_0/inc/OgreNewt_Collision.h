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
    this class represents a ndShapeInstance wrapping an ndShape,
    which is the Newton 4.0 structure for collision geometry.

    Ownership model (ND4):
      m_shapeInstancePtr (ndSharedPtr<ndShapeInstance>) -- SOLE OWNER of the shape instance.
      m_shapeInstance    (raw ndShapeInstance*)         -- non-owning cache for fast access.
      The ndShape* inside the instance is owned by the instance itself.
      No manual AddRef / Release is needed.
    */
    //! represents a shape for collision detection
    class _OgreNewtExport Collision
    {
    public:
        friend class CollisionSerializer;

        //! default constructor (empty collision, to be filled by subclass)
        Collision(const World* world);

        //! copy constructor -- shares the ndSharedPtr (both objects keep the instance alive)
        Collision(const Collision& shape);

        Collision(const World* world, const ndSharedPtr<ndShapeInstance> shapeInstancePtr);

        //! destructor
        virtual ~Collision();

        // ----------------------------------------------------------------
        // Accessors
        // ----------------------------------------------------------------

        ndShapeInstance* getShapeInstance() const
        {
            return const_cast<ndShapeInstance*>(*m_shapeInstancePtr);
        }
        const ndShapeInstance* getShapeInstanceConst() const
        {
            return *m_shapeInstancePtr;
        }

        const ndShape* getNewtonConstCollision() const
        {
            const ndShapeInstance* inst = *m_shapeInstancePtr;
            return inst ? inst->GetShape() : nullptr;
        }
        ndShape* getNewtonCollision()
        {
            ndShapeInstance* inst = *m_shapeInstancePtr;
            return inst ? const_cast<ndShape*>(inst->GetShape()) : nullptr;
        }

        //! Get the owning SharedPtr. Callers that need to extend lifetime can copy this.
        const ndSharedPtr<ndShapeInstance>& getShapeInstancePtr() const
        {
            return m_shapeInstancePtr;
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

        //! get the Axis-Aligned Bounding Box for this collision shape.
        /*!
         * \warning The returned AABB can be too large! If you need an AABB that fits exactly use the OgreNewt::CollisionTools::CalculateFittingAABB function
         */
        Ogre::AxisAlignedBox getAABB(const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO) const;

        //! Returns the Collisiontype for this Collision
        CollisionPrimitiveType getCollisionPrimitiveType() const
        {
            return getCollisionPrimitiveType(getNewtonConstCollision());
        }

        //! Returns the Collisiontype for the given Newton-Collision
        static CollisionPrimitiveType getCollisionPrimitiveType(const ndShape* col);

        //! Scale the collision shape
        void scaleCollision(const Ogre::Vector3& scale);

        //! friend functions for the Serializer
        friend class OgreNewt::CollisionSerializer;

    protected:
        // ----------------------------------------------------------------
        // Data members -- ND4 ownership model
        // ----------------------------------------------------------------

        //! SOLE OWNER of the shape instance. Determines lifetime.
        ndSharedPtr<ndShapeInstance> m_shapeInstancePtr;

        //! The world this collision belongs to.
        const World* m_world;
    };

    //! represents a collision shape that is explicitly convex.
    class _OgreNewtExport ConvexCollision : public Collision
    {
    public:
        //! constructor
        ConvexCollision(const World* world);

        //! copy from any Collision (must actually be convex)
        ConvexCollision(const Collision& convexShape);

        //! copy from ConvexCollision
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

} // end NAMESPACE OgreNewt

#endif
// _INCLUDE_OGRENEWT_COLLISION
