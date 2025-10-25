#include "OgreNewt_Stdafx.h"
#include "OgreNewt_MaterialID.h"
#include "OgreNewt_World.h"

namespace OgreNewt
{
    // Static member initialization
    int MaterialID::s_nextID = 0;

    MaterialID::MaterialID(const World* world)
    {
        m_world = world;
        // Newton 4.0 doesn't have material group IDs anymore
        // Generate a unique ID for compatibility
        id = s_nextID++;
    }

    MaterialID::MaterialID(const World* world, int ID)
    {
        m_world = world;
        id = ID;

        // Update static counter if needed
        if (ID >= s_nextID)
        {
            s_nextID = ID + 1;
        }
    }

    MaterialID::~MaterialID()
    {
        // Newton 4.0 doesn't require material cleanup
    }

}