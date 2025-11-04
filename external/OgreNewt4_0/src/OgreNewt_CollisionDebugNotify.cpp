#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionDebugNotify.h"

namespace OgreNewt
{
	CollisionDebugNotify::CollisionDebugNotify(Ogre::ManualObject* lines, const Ogre::Vector3& scale)
		: m_lines(lines), m_scale(scale), m_index(0)
	{
	}

	void CollisionDebugNotify::DrawPolygon(ndInt32 vertexCount, const ndVector* const faceArray, const ndEdgeType* const)
	{
		if (vertexCount < 3 || !m_lines)
			return;

		// Draw triangle edges
		for (ndInt32 i = 0; i < vertexCount; i++)
		{
			ndInt32 j = (i + 1) % vertexCount;

			Ogre::Vector3 v0(
				faceArray[i].m_x / m_scale.x,
				faceArray[i].m_y / m_scale.y,
				faceArray[i].m_z / m_scale.z
			);

			Ogre::Vector3 v1(
				faceArray[j].m_x / m_scale.x,
				faceArray[j].m_y / m_scale.y,
				faceArray[j].m_z / m_scale.z
			);

			m_lines->position(v0);
			m_lines->index(m_index++);
			m_lines->position(v1);
			m_lines->index(m_index++);
		}
	}
}
