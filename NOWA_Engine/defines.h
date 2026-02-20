#ifndef DEFINES_H
#define DEFINES_H

#include <cassert>
#include "boost/weak_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include <functional>
#include "OgreString.h"
#include "Objbase.h" // For guid
#include <functional> // For hash

#ifdef WIN32

#if defined(NOWA_CLIENT_BUILD)
// Import definition
#define EXPORTED __declspec( dllimport )
#else
// Export definition
#define EXPORTED __declspec( dllexport )
#endif

#else
#define EXPORTED
#endif

//#define NOWA_VERSION_MAJOR 2
//#define NOWA_VERSION_MINOR 0
//#define NOWA_VERSION_PATCH 0
//
//#define NOWA_DEFINE_VERSION(major, minor, patch) ((major << 16) | (minor << 8) | patch)
//#define NOWA_VERSION   NOWA_DEFINE_VERSION(NOWA_VERSION_MAJOR, NOWA_VERSION_MINOR, NOWA_VERSION_PATCH)

namespace NOWA
{
	enum eGizmoVisibilityMask
	{
		GIZMO_VISIBLE_MASK_VISIBLE = 1 << 1
	};

	/*
	RenderQueue ID range [0; 100) & [200; 225) default to FAST (i.e. for v2 objects, like Items)
	RenderQueue ID range [100; 200) & [225; 256) default to V1_FAST (i.e. for v1 objects, like v1::Entity)
	By default new Items and other v2 objects are placed in RenderQueue ID 10
	By default new v1::Entity and other v1 objects are placed in RenderQueue ID 110
	*/
	enum eRenderQueues
	{
		RENDER_QUEUE_V2_MESH = 10, // E.g. Ogre::ManualObject, Item
		RENDER_QUEUE_TERRA = 11, // Terra v2
		RENDER_QUEUE_DISTORTION = 16, // fast
		RENDER_QUEUE_V1_MESH = 110, // E.g. Ogre::Entity
		RENDER_QUEUE_PARTICLE_STUFF = 155,
		RENDER_QUEUE_LEGACY = 156,
        RENDER_QUEUE_V2_TRANSPARENT = 205, // Transparent V2 (200-224 range)
		RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND = 220, // E.g. Ogre::ManualObject, Item
		RENDER_QUEUE_V1_OBJECTS_ALWAYS_IN_FOREGROUND = 230, // E.g. Ogre::v1::Entity, Ogre::v1::ManualObject, Ogre::v1::Overlay
		RENDER_QUEUE_GIZMO = 252,
		RENDER_QUEUE_MAX = 254 // E.g. Ogre::v1::Overlay
	};
	
	/*
	* In order to dereference a weak_ptr, you have to cast it to a
	* shared_ptr first.  You still have to check to see if the pointer is dead dead by calling expired() on the weak_ptr,
	* so why not just allow the weak_ptr to be dereferenceable?  It doesn't buy anything to force this extra step because
	* you can still cast a dead weak_ptr to a shared_ptr and crash.  Nice.  Anyway, this function takes some of that
	* headache away.
	*/
	template <class Type>
	static boost::shared_ptr<Type> makeStrongPtr(boost::weak_ptr<Type> weakPtr)
	{
		if (!weakPtr.expired())
		{
			return boost::shared_ptr<Type>(weakPtr);
		}
		else
		{
			return boost::shared_ptr<Type>();
		}
	}
	///! Delivers a hash number to a given string
    ///! note: Two same string are getting the same numbers? Yes, alsways the same!
	static unsigned int getIdFromName(const Ogre::String& name)
	{
		if (name.size() == 0)
		{
			return 0;
		}
		
		std::hash<Ogre::String> hash;
		return static_cast<unsigned int>(hash(name));
	}

	static unsigned long makeUniqueID(void)
	{
		unsigned long uniqueId = -1;
		UUID uuid = { 0 };
		std::string guid;

		::UuidCreate(&uuid);

		RPC_CSTR szUuid = nullptr;
		if (::UuidToStringA(&uuid, &szUuid) == RPC_S_OK)
		{
			guid += (char*)szUuid;
			unsigned long hc = static_cast<unsigned long>(std::hash<std::string>()(guid));
			uniqueId = hc;
			::RpcStringFreeA(&szUuid);
		}
		return uniqueId;
	}

	static int FileFlag = FILE_ATTRIBUTE_OFFLINE;

}; // namespace end

#endif // DEFINES_H