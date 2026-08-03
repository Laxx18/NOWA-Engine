#ifndef PLATFORM_COMPONENT_BASE_H
#define PLATFORM_COMPONENT_BASE_H

#include "GameObjectComponent.h"

namespace NOWA
{

	class EXPORTED PlatformComponentBase : public GameObjectComponent
	{
	public:
		virtual void setPlatformData(const std::vector<unsigned char>& platformData) = 0;

		virtual std::vector<unsigned char> getPlatformData(void) const = 0;

		/**
         * @brief Finds the nearest point on this platform's path to a given
         *        world-space position, for cross-network platform-to-platform
         *        snapping (e.g. drawing a new platform chain that runs into an
         *        EXISTING, separate platform GameObject and should terminate
         *        flush against it instead of leaving a gap or an off-center
         *        joint).
         * @param[in] worldPos The world-space position to test against (e.g. the
         *        current mouse raycast hit while drawing).
         * @param[in] maxRadius Maximum acceptable distance in meters. If the
         *        nearest point on this platform's path is farther than this,
         *        the implementation returns false and outPoint is left
         *        untouched.
         * @param[out] outPoint The nearest world-space point on this platform's
         *        path, only valid when this function returns true.
         * @return True if a point within maxRadius was found, false otherwise
         *        (e.g. this platform has no segments yet, or nothing is close
         *        enough).
         */
        virtual bool getNearestPointOnPlatform(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outPoint) const = 0;

		/**
         * @see GameObjectComponent::isProcedural
         */
        virtual bool isProcedural(void) const override
        {
            return true;
        }
	};

}; //namespace end

#endif
