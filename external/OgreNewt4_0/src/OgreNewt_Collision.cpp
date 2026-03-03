#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_World.h"

namespace OgreNewt
{

    // ------------------------------------------------------------------
    // Default constructor -- empty collision, subclass fills m_shapeInstancePtr
    // ------------------------------------------------------------------
    Collision::Collision(const World* world) :
        m_shapeInstancePtr(),
        m_world(world)
    {
    }

    // ------------------------------------------------------------------
    // Copy constructor -- shares the ndSharedPtr (refcount incremented)
    // Both Collision objects keep the ndShapeInstance alive.
    // ------------------------------------------------------------------
    Collision::Collision(const Collision& shape) :
        m_shapeInstancePtr(shape.m_shapeInstancePtr), // share ownership
        m_world(shape.m_world)
    {
    }

    Collision::Collision(const World* world, const ndSharedPtr<ndShapeInstance> shapeInstancePtr)
        : m_shapeInstancePtr(shapeInstancePtr),
        m_world(world)
    {
    }

    // ------------------------------------------------------------------
    // Destructor -- ndSharedPtr handles cleanup automatically.
    // When the last Collision sharing this ndShapeInstance is destroyed,
    // the ndShapeInstance (and its internal ndShape) are freed.
    // ------------------------------------------------------------------
    Collision::~Collision()
    {
        // m_shapeInstancePtr destructor runs automatically (RAII).
    }

    // ------------------------------------------------------------------
    // getUserID
    // ------------------------------------------------------------------
    long Collision::getUserID() const
    {
        const ndShape* shape = getNewtonConstCollision();
        if (shape)
        {
            ndShapeInfo info = shape->GetShapeInfo();
            return info.m_collisionType;
        }
        return 0;
    }

    // ------------------------------------------------------------------
    // getAABB -- uses the shape instance (respects scale & local matrix)
    // ------------------------------------------------------------------
    Ogre::AxisAlignedBox Collision::getAABB(const Ogre::Quaternion& orient, const Ogre::Vector3& pos) const
    {
        Ogre::AxisAlignedBox box;

        if (m_shapeInstancePtr)
        {
            ndMatrix matrix;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, matrix);

            ndVector p0, p1;
            m_shapeInstancePtr->CalculateAabb(matrix, p0, p1);

            box.setMinimum(Ogre::Vector3(p0.m_x, p0.m_y, p0.m_z));
            box.setMaximum(Ogre::Vector3(p1.m_x, p1.m_y, p1.m_z));
        }
        return box;
    }

    // ------------------------------------------------------------------
    // getCollisionPrimitiveType (static)
    // ------------------------------------------------------------------
    CollisionPrimitiveType Collision::getCollisionPrimitiveType(const ndShape* col)
    {
        if (!col)
        {
            return NullPrimitiveType;
        }

        // const_cast is needed because ND4's GetAs* methods are non-const.
        ndShape* nc = const_cast<ndShape*>(col);

        if (nc->GetAsShapeBox())
        {
            return BoxPrimitiveType;
        }
        if (nc->GetAsShapeSphere())
        {
            return EllipsoidPrimitiveType;
        }
        if (nc->GetAsShapeCylinder())
        {
            return CylinderPrimitiveType;
        }
        if (nc->GetAsShapeCapsule())
        {
            return CapsulePrimitiveType;
        }
        if (nc->GetAsShapeCone())
        {
            return ConePrimitiveType;
        }
        if (nc->GetAsShapeConvexHull())
        {
            return ConvexHullPrimitiveType;
        }
        if (nc->GetAsShapeCompound())
        {
            return CompoundCollisionPrimitiveType;
        }
        if (nc->GetAsShapeStaticMesh())
        {
            return TreeCollisionPrimitiveType;
        }

        return NullPrimitiveType;
    }

    // ------------------------------------------------------------------
    // scaleCollision -- operates on the shape instance
    // ------------------------------------------------------------------
    void Collision::scaleCollision(const Ogre::Vector3& scale)
    {
        if (m_shapeInstancePtr)
        {
            m_shapeInstancePtr->SetScale(ndVector(scale.x, scale.y, scale.z, ndFloat32(0.0f)));
        }
    }

    // ==================================================================
    // ConvexCollision
    // ==================================================================

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
        const ndShape* shape = getNewtonConstCollision();
        if (shape)
        {
            return Ogre::Real(shape->GetVolume());
        }
        return 0.0f;
    }

    void ConvexCollision::calculateInertialMatrix(Ogre::Vector3& inertia, Ogre::Vector3& offset) const
    {
        const ndShape* shape = getNewtonConstCollision();
        if (shape)
        {
            ndMatrix alignMatrix(ndGetIdentityMatrix());
            ndVector localScale(ndFloat32(1.0f), ndFloat32(1.0f), ndFloat32(1.0f), ndFloat32(1.0f));
            ndMatrix matrix(ndGetIdentityMatrix());

            ndMatrix inertiaMatrix = shape->CalculateInertiaAndCenterOfMass(alignMatrix, localScale, matrix);

            // Diagonal = principal inertia, position row = center of mass
            inertia.x = inertiaMatrix[0][0];
            inertia.y = inertiaMatrix[1][1];
            inertia.z = inertiaMatrix[2][2];

            offset.x = inertiaMatrix[3][0];
            offset.y = inertiaMatrix[3][1];
            offset.z = inertiaMatrix[3][2];
        }
    }

} // namespace OgreNewt
