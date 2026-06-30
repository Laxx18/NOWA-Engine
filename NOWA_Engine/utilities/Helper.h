#ifndef HELPER_H
#define HELPER_H

#include "OgreResource.h"

namespace NOWA
{
    // Dummy ManualResourceLoader for procedurally generated foliage cell meshes.
    // These meshes are built entirely in memory and never need reloading from disk.
    // Providing a loader suppresses Ogre's "no manual loader provided" warning which
    // fires when createManual() is called without one.

    class DummyMeshLoader : public Ogre::ManualResourceLoader
    {
    public:
        void prepareResource(Ogre::Resource* resource) override
        {
        }
        void loadResource(Ogre::Resource* resource) override
        {
        }
    };

    DummyMeshLoader gDummyMeshLoader;
}

#endif
