/*
	OgreNewt Library

	Ogre implementation of Newton Game Dynamics SDK

	Newton 4.0 Contact Notify Bridge
	This bridges Newton 4.0's ndContactNotify system with OgreNewt's callback system

*/
#ifndef _INCLUDE_OGRENEWT_CONTACTNOTIFY
#define _INCLUDE_OGRENEWT_CONTACTNOTIFY

#include "OgreNewt_Prerequisites.h"
#include "ndContactNotify.h"
#include <map>

namespace OgreNewt
{
	class Body;
	class ContactCallback;
	class MaterialPair;

	//! Bridge class that connects Newton 4.0's contact notify system with OgreNewt callbacks
	/*!
		This class inherits from Newton 4.0's ndContactNotify and translates
		the callbacks to the OgreNewt ContactCallback system for backward compatibility.
	*/
	class _OgreNewtExport ContactNotify : public ndContactNotify
	{
	public:
		ContactNotify(OgreNewt::World* ogreNewt);
		virtual ~ContactNotify();

		virtual void OnContactCallback(const ndContact* const contact, ndFloat32 timestep) const override;

		virtual bool OnAabbOverlap(const ndContact* const contact, ndFloat32 timestep) const override;

		virtual bool OnCompoundSubShapeOverlap(const ndContact* const contact, ndFloat32 timestep, const ndShapeInstance* const subShapeA, const ndShapeInstance* const subShapeB) const override;

		//! Set the OgreNewt body associated with this notify
		void setOgreNewtBody(OgreNewt::Body* body);

		//! Get the OgreNewt body associated with this notify
		OgreNewt::Body* getOgreNewtBody() const;

		//! Set user data pointer
		void setUserData(void* data);

		//! Get user data pointer
		void* getUserData() const;

		//! Register a material pair callback for specific material combinations
		void registerMaterialPairCallback(int materialID0, int materialID1, OgreNewt::ContactCallback* callback);

		//! Unregister a material pair callback
		void unregisterMaterialPairCallback(int materialID0, int materialID1);

		//! Find the appropriate callback for a contact between two bodies
		OgreNewt::ContactCallback* findCallback(OgreNewt::Body* body0, OgreNewt::Body* body1) const;

	protected:
		OgreNewt::Body* m_ogreNewtBody;
		void* m_userData;

		// Map to store material pair callbacks: key = (mat0_id << 16) | mat1_id
		std::map<int, OgreNewt::ContactCallback*> m_materialCallbacks;

		//! Generate a key for the material callback map
		int getMaterialPairKey(int mat0, int mat1) const;
	private:
		OgreNewt::World* ogreNewt;
	};

}   // end NAMESPACE OgreNewt

#endif
// _INCLUDE_OGRENEWT_CONTACTNOTIFY