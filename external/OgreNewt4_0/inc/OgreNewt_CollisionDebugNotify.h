/*
OgreNewt Library 4.0

Ogre implementation of Newton Game Dynamics SDK 4.0
*/

#ifndef _INCLUDE_OGRENEWT_COLLISION_DEBUG_NOTIFY
#define _INCLUDE_OGRENEWT_COLLISION_DEBUG_NOTIFY

#include "OgreNewt_Prerequisites.h"

namespace OgreNewt
{
    class CollisionDebugNotify : public ndShapeDebugNotify
    {
    public:
        CollisionDebugNotify(Ogre::ManualObject* lines);

        virtual void DrawPolygon(ndInt32 vertexCount, const ndVector* const faceArray, const ndEdgeType* const edgeType) override;
    protected:
        Ogre::ManualObject* m_lines;
        ndInt32 m_index;
    };
}

#endif
