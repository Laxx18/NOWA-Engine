/*
    OgreNewt Library

    Ogre implementation of Newton Game Dynamics SDK

    OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

        by Walaber
        some changes by melven
        adapted for Newton 4.0

*/
#ifndef _INCLUDE_OGRENEWT_MATERIALID
#define _INCLUDE_OGRENEWT_MATERIALID

#include "OgreNewt_Prerequisites.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{

    //! represents a material (deprecated in Newton 4.0)
    /*!
        Newton 4.0 has moved away from the material ID system used in Newton 3.x
        This class is kept for API compatibility but has limited functionality.
        Material properties are now handled through body notify callbacks and shape materials.
    */
    class _OgreNewtExport MaterialID
    {
    public:

        //! constructor
        /*!
            \param world pointer to the OgreNewt::World
        */
        MaterialID(const World* world);

        //! constructor with explicit ID
        /*!
            \param world pointer to the OgreNewt::World
            \param ID the material ID value
        */
        MaterialID(const World* world, int ID);

        //! destructor
        ~MaterialID();

        //! get material ID
        /*!
            In Newton 4.0, this returns a local ID for compatibility purposes only.
            Material system has been redesigned.
        */
        int getID() const { return id; }

    protected:

        friend class OgreNewt::World;
        friend class OgreNewt::MaterialPair;

    private:
        int id;
        const OgreNewt::World* m_world;
        static int s_nextID; // Static counter for generating unique IDs
    };

}   // end NAMESPACE OgreNewt

#endif
// _INCLUDE_OGRENEWT_MATERIALID