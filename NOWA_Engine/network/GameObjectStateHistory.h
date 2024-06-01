#ifndef GAME_OBJECT_HISTORY_H
#define GAME_OBJECT_HISTORY_H

#include "RakNetTypes.h"
#include "DS_Queue.h"
#include "RakMemoryOverride.h"
#include "defines.h"

namespace NOWA
{

	namespace NET
	{

		#pragma pack(push, 1)

		struct EXPORTED Gamestate
		{
			Gamestate();	// exchange this attributes through a map, where the user can add as many properties as he wants

			unsigned char		useTimeStamp;		//  ID_TIMESTAMP Protokoll
			unsigned char		packetId;			// Eigenes zu verwendetes Protokoll
			char				move;				//Bewegungsstart und -ende
			char				jump;				//Sprungstart und -ende
			char				shoot;				//Schuss
			Ogre::Real			angularVelocity;	//Rotationsgeschwindigkeit um die Y-Achse
			Ogre::Real			height;				//Sprunghoehe
			RakNet::Time		timeStamp;			// Zeitstempelabfrage RakNet::GetTime()
			Ogre::Vector3		bulletDirection;	//Richtung der Flugbahn
		};

		#pragma pack(pop)

		struct EXPORTED GameObjectState
		{
			GameObjectState();
			GameObjectState(RakNet::TimeMS curTime, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

			RakNet::TimeMS		time;
			Ogre::Vector3		position;
			Ogre::Quaternion	orientation;
		};

		struct EXPORTED GameObjectShootState
		{
			GameObjectShootState();
			GameObjectShootState(RakNet::TimeMS curTime, const Ogre::Vector3& position);

			RakNet::TimeMS		time;
			Ogre::Vector3		position;
		};

		class EXPORTED GameObjectStateHistory
		{
		public:
			GameObjectStateHistory();

			void init(RakNet::TimeMS maxWriteInterval, RakNet::TimeMS maxHistoryTime, bool useIntervall = true);

			void enqeue(const Ogre::Vector3& position, const Ogre::Quaternion& orientation, RakNet::TimeMS curTime);

			void readPiecewiseLinearInterpolated(Ogre::Vector3& position, Ogre::Quaternion& orientation, RakNet::TimeMS pastTime);

			GameObjectState getState(unsigned int i);
			GameObjectState getLastState(void);

			void clear(void);

			size_t getHistoryLength(void) const;
		private:
			DataStructures::Queue<GameObjectState> history;
			unsigned int maxLength;
			RakNet::TimeMS writeInterval;
			bool useIntervall;
		};

		//////////////////////////////////////////////////////////////////////

		class EXPORTED GameObjectShootStateHistory
		{
		public:
			void init(RakNet::TimeMS maxWriteInterval, RakNet::TimeMS maxHistoryTime, bool useIntervall = true);

			void enqeue(const Ogre::Vector3& position, RakNet::TimeMS curTime);
			
			void timewarp(GameObjectShootState& resultState, RakNet::TimeMS pastTime);

			GameObjectShootState getState(unsigned int i);

			GameObjectShootState getLastState(void);

			void clear(void);
		private:
			DataStructures::Queue<GameObjectShootState>	history;
			unsigned maxLength;
			RakNet::TimeMS writeInterval;
			bool useIntervall;
		};

	} // namespace end NET

} // namespace end NOWA

#endif
