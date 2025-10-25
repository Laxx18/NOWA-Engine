/*
    OgreNewt Library 4.0

    Ogre implementation of Newton Game Dynamics SDK 4.0

    OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

        by Walaber
        some changes by melven
        Newton 4.0 adaptation
*/

#ifndef __INCLUDE_OGRENEWT_PREREQ__
#define __INCLUDE_OGRENEWT_PREREQ__

#ifdef __APPLE__
#    include <Carbon/Carbon.h>
#   include <Ogre/OgreVector3.h>
#   include <Ogre/OgreQuaternion.h>
#   include <Ogre/OgreMovableObject.h>
#   include <Ogre/OgreRenderable.h>
#   include <Ogre/OgreNode.h>
#   include <Ogre/OgreFrameListener.h>
#   include <Ogre/OgreSceneNode.h>
#   include <Ogre/OgreSceneManager.h>
#   include <Ogre/OgreManualObject.h>
#   ifndef OGRENEWT_NO_OGRE_ANY
#       include <Ogre/OgreAny.h>
#   endif
#else
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreMovableObject.h>
#include <OgreRenderable.h>
#include <OgreNode.h>
#include <OgreFrameListener.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreManualObject2.h>
#include <OgreFont.h>
#   ifndef OGRENEWT_NO_OGRE_ANY
#       include "OgreNewt_Any.h"
#   endif
#endif

#include <ndNewton.h>
#include <ndWorld.h>
#include <ndBody.h>
#include <ndBodyDynamic.h>
#include <ndBodyKinematic.h>
#include <ndShape.h>
#include <ndShapeInstance.h>

#include <functional>
#include <memory>
using namespace std::placeholders;

namespace OgreNewt
{
    using std::function;
    using std::bind;
    using std::shared_ptr;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#   define _CDECL _cdecl
#   if defined( _OGRENEWT_EXPORTS ) && defined( _OGRENEWT_DYNAMIC )
#       define _OgreNewtExport __declspec( dllexport )
#   elif defined( _OGRENEWT_DYNAMIC )
#       define _OgreNewtExport __declspec( dllimport )
#   else
#       define _OgreNewtExport
#   endif
#else // Linux / Mac OSX etc
#   define _OgreNewtExport
#   define _CDECL
#endif

namespace OgreNewt
{
    class World;
    class MaterialID;
    class Joint;
    class CustomJoint;
    class Contact;
    class MaterialPair;
    class Body;
    class Collision;
    class CollisionSerializer;
    class ConvexCollision;
    class Debugger;

    //! helper class: OgreNewt-classes can derive from this class to implement a destructor-callback
    template<class DerivedClass>
    class _DestructorCallback
    {
    public:
        typedef OgreNewt::function<void(DerivedClass*)> DestructorCallbackFunction;

        _DestructorCallback() : m_callback(nullptr) {}

        virtual ~_DestructorCallback()
        {
            if (m_callback)
                m_callback((DerivedClass*)(this));
        }

        void removeDestructorCallback() { m_callback = nullptr; }

        void setDestructorCallback(DestructorCallbackFunction fun) { m_callback = fun; }

        template<class c> void setDestructorCallback(OgreNewt::function<void(c*, DerivedClass*)> callback, c* instancedClassPointer)
        {
            setDestructorCallback(OgreNewt::bind(callback, instancedClassPointer, _1));
        }
    private:
        DestructorCallbackFunction m_callback;
    };
}

#endif