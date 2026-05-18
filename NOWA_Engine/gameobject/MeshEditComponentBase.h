#ifndef MESH_EDIT_COMPONENT_BASE_H
#define MESH_EDIT_COMPONENT_BASE_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED MeshEditComponentBase : public GameObjectComponent
	{
	public:
        virtual std::vector<unsigned char> getMeshData(void) const = 0;

        virtual void setMeshData(const std::vector<unsigned char>& data) = 0;
	};

}; //namespace end

#endif