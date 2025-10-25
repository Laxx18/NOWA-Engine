/*
    OgreNewt Library

    Ogre implementation of Newton Game Dynamics SDK

    OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

        by Walaber
        some changes by melven

*/

#ifndef _INCLUDE_OGRENEWT_BODYINAABBITERATOR
#define _INCLUDE_OGRENEWT_BODYINAABBITERATOR


#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_World.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{


    class _OgreNewtExport BodyInAABBIterator
    {
    public:
        typedef OgreNewt::function<void(const Body*, void* userdata)> IteratorCallback;

        void go(const Ogre::AxisAlignedBox& aabb, IteratorCallback callback, void* userdata) const;

    protected:
        friend class OgreNewt::World;

        BodyInAABBIterator(const OgreNewt::World* world);

    private:
        const OgreNewt::World* m_world;
        mutable IteratorCallback m_callback;
    };


}   // end NAMESPACE OgreNewt

#endif  // _INCLUDE_OGRENEWT_BODYINAABBITERATOR
