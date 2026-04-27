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
         * @see GameObjectComponent::isProcedural
         */
        virtual bool isProcedural(void) const override
        {
            return true;
        }
	};

}; //namespace end

#endif