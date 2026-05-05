#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionDebugNotify.h"

namespace OgreNewt
{
    CollisionDebugNotify::CollisionDebugNotify(Ogre::ManualObject* lines)
        : m_lines(lines),
        m_index(0)
    {
    }

    void CollisionDebugNotify::DrawPolygon(ndInt32 vertexCount, const ndVector* const faceArray, const ndEdgeType* const)
    {
        if (vertexCount < 3 || !m_lines)
        {
            return;
        }

        for (ndInt32 i = 0; i < vertexCount; i++)
        {
            ndInt32 j = (i + 1) % vertexCount;
            // Vertices are in Newton local space — scene node applies scale automatically
            Ogre::Vector3 v0(faceArray[i].m_x, faceArray[i].m_y, faceArray[i].m_z);
            Ogre::Vector3 v1(faceArray[j].m_x, faceArray[j].m_y, faceArray[j].m_z);
            m_lines->position(v0);
            m_lines->index(m_index++);
            m_lines->position(v1);
            m_lines->index(m_index++);
        }
    }
}
