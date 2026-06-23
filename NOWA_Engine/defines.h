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

	enum eType
    {
        NONE = 0,
        ITEM,
        SCENE_NODE, // E.g. for waypoints
        PLANE,
        MIRROR,
        CAMERA,
        REFLECTION_CAMERA,
        LIGHT_DIRECTIONAL,
        TERRA,
        OCEAN,
        CUSTOM
    };

	struct EXPORTED GameObjectTypeDescriptor
    {
        eType type = eType::CUSTOM;
        Ogre::String displayName;
        Ogre::String meshToDisplay;
        bool needsMeshItem = true;
        bool forceZeroTransform = false;
        bool enterMeshModifyMode = false;
        Ogre::Vector3 defaultDirection = Ogre::Vector3::NEGATIVE_UNIT_Z;
        std::vector<Ogre::String> autoComponents;
        bool guardWithPluginCheck = false;
        std::function<void()> preComponentsCallback;
        std::function<void()> postComponentsCallback;
    };

}; // namespace end

#endif // DEFINES_H