#ifndef GEOMETRIC_COMPONENT_BASE_H
#define GEOMETRIC_COMPONENT_BASE_H

#include "GameObjectComponent.h"

namespace NOWA
{
    class EXPORTED GeometricComponentBase : public GameObjectComponent
    {
    public:
        // ── Convex decomposition for physics ─────────────────────────────────
        /**
         * @brief One convex collision part exported to the physics system.
         *
         * type          OgreNewt primitive name passed to createCollisionPrimitive():
         *               "Box"        size = (width, height, depth)
         *               "Ellipsoid"  size = (rx, ry, rz)
         *               "Cylinder"   size = (radius, height, radius)
         *               "Cone"       size = (radius, height, radius)
         *               "Capsule"    size = (radius, bodyHeight, radius)
         *               "ConvexHull" vertices must be filled; size/type ignored by Newton
         * position      local offset from the GameObject origin
         * size          interpretation is type-dependent (see above)
         * orientation   local rotation of this part
         * vertices      only used when type == "ConvexHull"
         */
        struct ConvexPart
        {
            ConvexPart()
                : type("Box"),
                position(Ogre::Vector3::ZERO),
                size(Ogre::Vector3::UNIT_SCALE),
                orientation(Ogre::Quaternion::IDENTITY)
            {
            }

            Ogre::String type;
            Ogre::Vector3 position;
            Ogre::Vector3 size;
            Ogre::Quaternion orientation;
            std::vector<Ogre::Vector3> vertices; ///< only for "ConvexHull"
        };

        // ── Interface ─────────────────────────────────────────────────────────

        /**
         * @brief   Returns convex parts that approximate this geometry for physics.
         *
         *          PhysicsActiveCompoundComponent calls this from createDynamicBody()
         *          when hasConvexParts() returns true, bypassing the XML config file.
         *          The default returns an empty list (no-op for non-procedural geometry).
         */
        virtual std::vector<ConvexPart> getConvexParts(void) const
        {
            return {};
        }

        /**
         * @brief   Returns true if this component can supply convex parts.
         *
         *          PhysicsActiveCompoundComponent checks this in postInit() to decide
         *          whether to defer body creation to connect().
         */
        virtual bool hasConvexParts(void) const
        {
            return false;
        }

        /**
         * @see GameObjectComponent::isProcedural
         */
        virtual bool isProcedural(void) const override
        {
            return true;
        }
    };

} // namespace NOWA

#endif // GEOMETRIC_COMPONENT_BASE_H