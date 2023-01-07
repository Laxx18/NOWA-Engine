#ifndef VEHICLE_H
#define VEHICLE_H

#include "defines.h"

namespace NOWA
{
	//class EXPORTED Vehicle : public OgreNewt::RayCastVehicle
	//{
	//public:

	//	Vehicle(OgreNewt::Body* carBody, unsigned int tireCount);

	//	virtual ~Vehicle();

	//protected:
	//	void setTireTransform(void* tireUserData, const Ogre::Matrix4& transformMatrixInGlobalScape) const;

	//private:



	//};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//class EXPORTED VehiclePlacementRaycast : public OgreNewt::BasicRaycast
	//{
	//public:
	//	VehiclePlacementRaycast(OgreNewt::Body* vehicle, Ogre::Vector3& location)
	//		: OgreNewt::BasicRaycast()
	//	{
	//			// ignore go as if will no do the user filter correctly
	//			Ogre::Vector3 start(location);
	//			Ogre::Vector3 end(location);

	//			start.y += 100.0f;
	//			end.y -= 100.0f;

	//			// re-cast the ray this time filtering the car body
	//			this->body = vehicle;
	//			go(vehicle->getWorld(), start, end, true);

	//			OgreNewt::BasicRaycast::BasicRaycastInfo info = getFirstHit();
	//			this->elevation = 0.5f + start.y + (end.y - start.y) * info.mDistance;
	//	}

	//	// skip the car body 
	//	virtual bool userPreFilterCallback(OgreNewt::Body* body)
	//	{
	//		// ray form casting hi own body
	//		return (body != this->body);
	//	}

	//	Ogre::Real getElevation(void) const
	//	{
	//		return this->elevation;
	//	}

	//private:
	//	Ogre::Real elevation;
	//	OgreNewt::Body* body;

	//};

}; //namespace end

#endif