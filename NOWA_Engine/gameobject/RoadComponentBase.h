#ifndef ROAD_COMPONENT_BASE_H
#define ROAD_COMPONENT_BASE_H

#include "GameObjectComponent.h"

namespace NOWA
{

	class EXPORTED RoadComponentBase : public GameObjectComponent
	{
	public:
		virtual void setRoadData(const std::vector<unsigned char>& roadData) = 0;

		virtual std::vector<unsigned char> getRoadData(void) const = 0;

		/**
         * @brief Finds the nearest point on this road's centerline to a given
         *        world-space position, for cross-network road-to-road snapping
         *        (e.g. drawing a new road that runs into an EXISTING, separate
         *        road GameObject and should terminate flush against it instead
         *        of leaving a gap or an off-center joint).
         * @param[in] worldPos The world-space position to test against (e.g. the
         *        current mouse raycast hit while drawing).
         * @param[in] maxRadius Maximum acceptable distance in meters. If the
         *        nearest point on this road's centerline is farther than this,
         *        the implementation returns false and outPoint is left
         *        untouched.
         * @param[out] outPoint The nearest world-space point on this road's
         *        centerline, only valid when this function returns true.
         * @return True if a point within maxRadius was found, false otherwise
         *        (e.g. this road has no segments yet, or nothing is close
         *        enough).
         */
        virtual bool getNearestPointOnRoad(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outPoint) const = 0;

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