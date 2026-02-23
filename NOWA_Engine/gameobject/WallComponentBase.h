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
	};

}; //namespace end

#endif