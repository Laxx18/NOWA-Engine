#ifndef PATH_H
#define PATH_H

#include "defines.h"

namespace NOWA
{
	namespace KI
	{
		class EXPORTED Path
		{
		public:

			Path();

			// Constructor for creating a path with initial random waypoints. MinX/Y & MaxX/Y define the bounding box of the path.
			Path(unsigned int numWaypoints, Ogre::Real minX, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxZ, bool repeat);

			// Returns the current waypoint
			std::pair<bool, Ogre::Vector3> getCurrentWaypoint(void);

			std::pair<bool, std::pair<Ogre::Vector3, Ogre::Quaternion>> getCurrentWaypoint2(void);

			// Returns true if the end of the list has been reached
			bool isFinished(void);

			unsigned int getRemainingWaypoints(void);

			unsigned int getCurrentWaypointIndex(void);

			// Creates a random path which is bound by rectangle described by the min/max values
			std::vector<Ogre::Vector3> createRandomPath(unsigned int numWaypoints, Ogre::Real minX, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxZ);

			void setRepeat(bool repeat);

			bool getRepeat(void) const;

			void setDirectionChange(bool directionChange);

			bool getDirectionChange(void) const;

			void setInvertDirection(bool invertDirection);

			// Adds a waypoint to the end of the path
			void addWayPoint(const Ogre::Vector3& newPoint, const Ogre::Quaternion& orientation = Ogre::Quaternion::IDENTITY);

			// Clears the list and sets a new waypoint
			void setWayPoint(const Ogre::Vector3& newPoint, const Ogre::Quaternion& orientation = Ogre::Quaternion::IDENTITY);

			void clear();

			std::vector<std::pair<Ogre::Vector3, Ogre::Quaternion>> getWayPoints(void) const;

			// Moves the iterator on to the next waypoint in the list
			void setNextWayPoint();

			short getRound(void) const;
		private:

			std::vector<std::pair<Ogre::Vector3, Ogre::Quaternion>> wayPoints;

			// Points to the current waypoint
			std::vector<std::pair<Ogre::Vector3, Ogre::Quaternion>>::iterator currentWaypointItr;

			// Flag to indicate if the path should be looped (The last waypoint connected to the first)
			bool repeat;
			bool directionChange;
			short currentDirection;
			bool valid;
			short round;
			bool invertDirection;
		};
	} //namespace end KI

} //namespace end NOWA

#endif