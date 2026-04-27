#ifndef WALL_COMPONENT_BASE_H
#define WALL_COMPONENT_BASE_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED WallComponentBase : public GameObjectComponent
	{
	public:
		virtual void setWallData(const std::vector<unsigned char>& roadData) = 0;

		virtual std::vector<unsigned char> getWallData(void) const = 0;

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