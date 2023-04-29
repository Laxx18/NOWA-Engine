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

			// Returns true if the end of the list has been reached
			bool isFinished(void);

			unsigned int getRemainingWaypoints(void);

			// Creates a random path which is bound by rectangle described by the min/max values
			std::vector<Ogre::Vector3> createRandomPath(unsigned int numWaypoints, Ogre::Real minX, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxZ);

			void setRepeat(bool repeat);

			bool getRepeat(void) const;

			void setDirectionChange(bool directionChange);

			bool getDirectionChange(void) const;

			void setInvertDirection(bool invertDirection);

			// Adds a waypoint to the end of the path
			void addWayPoint(const Ogre::Vector3& newPoint);

			// Clears the list and sets a new waypoint
			void setWayPoint(const Ogre::Vector3& newPoint);

			// Methods for setting the path with either another Path or a list of vectors
			void setNewPath(std::vector<Ogre::Vector3> newPath);
			void setNewPath(const Path& path);

			void clear();

			std::vector<Ogre::Vector3> getWayPoints(void) const;

			// Moves the iterator on to the next waypoint in the list
			void setNextWayPoint();
		private:

			std::vector<Ogre::Vector3> wayPoints;

			// Points to the current waypoint
			std::vector<Ogre::Vector3>::iterator currentWaypointItr;

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