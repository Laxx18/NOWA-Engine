#include "NOWAPrecompiled.h"
#include "Path.h"

namespace NOWA
{
	namespace KI
	{
		Path::Path()
			: repeat(false),
			directionChange(false),
			valid(false),
			currentDirection(1),
			round(0),
			invertDirection(false)
		{
			this->currentWaypointItr = this->wayPoints.begin();
		}

		Path::Path(unsigned int numWaypoints, Ogre::Real minX, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxZ, bool repeat)
			: repeat(repeat),
			directionChange(false),
			currentDirection(1),
			round(0)
		{
			this->createRandomPath(numWaypoints, minX, minZ, maxX, maxZ);
			this->currentWaypointItr = this->wayPoints.begin();
			this->valid = true;
		}

		std::pair<bool, Ogre::Vector3> Path::getCurrentWaypoint(void)
		{
			if (false == this->isFinished())
			{
				// assert ( (this->wayPoints.size() > 0) && "[Path::getCurrentWaypoint] Path list is empty");
				return std::pair<bool, Ogre::Vector3>(this->valid, *this->currentWaypointItr);
			}
			else
			{
				return std::pair<bool, Ogre::Vector3>(false, Ogre::Vector3());
			}
		}

		bool Path::isFinished(void)
		{
			if (this->wayPoints.size() > 0)
			{
				bool finished = false;
				// Check if path follow run is finished
				if (this->currentWaypointItr == this->wayPoints.end())
				{
					if (1 == this->round && false == this->directionChange)
					{
						finished = true;
					}
					else if (2 == this->round)
					{
						finished = true;
					}
				}
				else if (this->currentWaypointItr == this->wayPoints.begin())
				{
					if (1 == this->round && false == this->directionChange)
					{
						finished = true;
					}
					else if (2 == this->round)
					{
						finished = true;
					}
				}
					
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] finished: " + Ogre::StringConverter::toString(finished));
				return finished;
			}
			else
			{
				return true;
			}
		}

		unsigned int Path::getRemainingWaypoints(void)
		{
			return std::distance(this->currentWaypointItr, this->wayPoints.end());
		}

		std::vector<Ogre::Vector3> Path::createRandomPath(unsigned int numWaypoints, Ogre::Real minX, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxZ)
		{
			this->wayPoints.clear();

			Ogre::Real midX = (maxX + minX) / 2.0f;
			Ogre::Real midZ = (maxZ + minZ) / 2.0f;

			Ogre::Real smaller = std::min(midX, midZ);

			// example: 2 * PI = 360 degree / 6 = 60 spacing
			Ogre::Real spacing = Ogre::Math::TWO_PI / (Ogre::Real)numWaypoints;

			for (unsigned int i = 0; i < numWaypoints; ++i)
			{
				Ogre::Real radialDist = Ogre::Math::RangeRandom(smaller * 0.2f, smaller);

				Ogre::Vector3 rndOffset(radialDist, 0.0f, 0.0f);
				Ogre::Quaternion tempOrient = Ogre::Quaternion(Ogre::Radian(i * spacing), Ogre::Vector3::UNIT_Y);

				// origin = zero, tempOrient * rndOffset = rotation around origin with a rnd offset
				Ogre::Vector3 newPos = Ogre::Vector3::ZERO + (tempOrient * rndOffset);

				// origin moved to midX, midZ
				newPos.x += midX;
				newPos.y = 0.0f;
				newPos.z += midZ;

				this->wayPoints.push_back(Ogre::Vector3(newPos));
			}

			this->currentWaypointItr = this->wayPoints.begin();
			this->valid = true;
			return this->wayPoints;
		}

		void Path::setRepeat(bool repeat)
		{
			this->repeat = repeat;
		}

		bool Path::getRepeat(void) const
		{
			return this->repeat;
		}

		void Path::setDirectionChange(bool directionChange)
		{
			this->directionChange = directionChange;
		}

		bool Path::getDirectionChange(void) const
		{
			return this->directionChange;
		}

		void Path::setInvertDirection(bool invertDirection)
		{
			if (true == invertDirection)
			{
				if (1 == this->currentDirection)
				{
					this->currentDirection = -1;
					// Remember: Never set the itr to wayPoints.end(), as its not valid!, each for loop increments as long as itr != wayPoints.end()
					this->currentWaypointItr = this->wayPoints.end() - 1;
				}
				else
				{
					this->currentDirection = 1;
					this->currentWaypointItr = this->wayPoints.begin();
				}
			}
		}

		void Path::addWayPoint(const Ogre::Vector3& waypoint)
		{
			if (this->wayPoints.size() > 0)
			{
				auto currentWaypoint = this->getCurrentWaypoint();
				if (true == currentWaypoint.first)
				{
					Ogre::Real distSq = waypoint.squaredDistance(currentWaypoint.second);
					if (distSq < 0.2f * 0.2f)
					{
						// Skip if the new way point sits totally near the current one.
						return;
					}
				}
			}
			this->wayPoints.push_back(waypoint);
			this->currentWaypointItr = this->wayPoints.begin();
			this->valid = true;
		}

		void Path::setWayPoint(const Ogre::Vector3& waypoint)
		{
			this->wayPoints.clear();
			this->wayPoints.push_back(waypoint);
			this->currentWaypointItr = this->wayPoints.begin();
			this->valid = true;
			this->round = 0;
			this->currentDirection = 1;
		}

		void Path::setNewPath(std::vector<Ogre::Vector3> newPath)
		{
			if (newPath.size() > 0)
			{
				this->wayPoints = newPath;
				this->currentWaypointItr = this->wayPoints.begin();
				this->valid = true;
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] setNewPath not valid");
				this->valid = false;
			}
		}

		void Path::setNewPath(const Path& path)
		{
			this->wayPoints = path.getWayPoints();
			this->currentWaypointItr = this->wayPoints.begin();
			this->valid = true;
			this->round = 0;
			this->currentDirection = 1;
		}

		void Path::clear(void)
		{
			this->wayPoints.clear();
			this->valid = false;
			this->currentWaypointItr = this->wayPoints.begin();
			this->round = 0;
			this->currentDirection = 1;
		}

		void Path::setNextWayPoint()
		{
			// This function is called, when a next waypoint is reached
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] directionChange: " + Ogre::StringConverter::toString(this->currentDirection));
		
			// If the direction is positve, increment the waypoint when reached
			if (1 == this->currentDirection)
			{
				++this->currentWaypointItr;
				// If it was the last waypoint
				if (this->currentWaypointItr == wayPoints.end())
				{
					if (false == this->repeat)
					{
						this->valid = false;
					}

					// Valid false, but itr must be set to begin, else an exception will occur, because the itr is invalid when it is at the end!
					this->currentWaypointItr = wayPoints.begin();
					// if repeat is off, increment the round
					if (false == this->repeat)
					{
						this->round++;
					}

					// Without direction change the agent may run just one round
					if (1 == this->round && false == this->directionChange)
					{
						this->clear();
						return;
					}
					else if (2 == this->round)
					{
						// with direction change 2 rounds
						this->clear();
						return;
					}
					
					// Change direction, if there are no waypoints left and start at the end to run back
					if (true == this->directionChange)
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] Change direction to -1 and start at end");
						this->currentWaypointItr = this->wayPoints.end();
						--this->currentWaypointItr;
						this->currentDirection = -1;
						this->valid = true;
					}
				}
			}
			else
			{
				// No more waypoints for inverted direction
				if (this->currentWaypointItr == wayPoints.begin())
				{
					if (false == this->repeat)
					{
						this->round++;
					}
					
					if (1 == this->round && false == this->directionChange)
					{
						this->clear();
						return;
					}
					else if (2 == this->round)
					{
						this->clear();
						return;
					}
					
					// Change direction, if there are no waypoints left and start at the beginning
					if (true == this->directionChange)
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] Change direction to 1 and start at beginning");
						this->currentWaypointItr = this->wayPoints.begin();
						this->currentDirection = 1;
					}
				}
			}

			// If the direction is negative, start at the end and decrement the waypoint when reached
			if (-1 == this->currentDirection)
			{
				if (this->currentWaypointItr != wayPoints.begin())
				{
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] decrement");
					--this->currentWaypointItr;
				}
			}
		}

		std::vector<Ogre::Vector3> Path::getWayPoints(void) const
		{
			return this->wayPoints;
		}

	}; // namespace end KI

}; // namespace end NOWA