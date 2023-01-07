#include "NOWAPrecompiled.h"
#include "RakAssert.h"
#include "RakMemoryOverride.h"
#include "RakNetTime.h"
#include "GameObjectStateHistory.h"

namespace NOWA
{

	namespace NET
	{
		Gamestate::Gamestate()
			: useTimeStamp(false),
			timeStamp(0),
			packetId(0),
			move(0),
			jump(0),
			shoot(0),
			angularVelocity(0),
			bulletDirection(Ogre::Vector3::ZERO)
		{

		}

		//////////////////////////////////////////////

		// Game state for history for playout delays functionality

		GameObjectState::GameObjectState()
			: ///time			(RakNet::GetTimeMS()),  fix it!!!
			position(Ogre::Vector3::ZERO),
			orientation(Ogre::Quaternion::IDENTITY)
		{

		}

		GameObjectState::GameObjectState(RakNet::TimeMS curTime, const Ogre::Vector3 & position, const Ogre::Quaternion & orientation)
			:
			time(curTime),
			position(position),
			orientation(orientation)
		{

		}

		//////////////////////////////////////////////

		// Game state for history for time warp functionality

		GameObjectShootState::GameObjectShootState()
			:
			////time			(RakNet::GetTimeMS()),  fix it!!!
			position(Ogre::Vector3::ZERO)
		{
		}

		GameObjectShootState::GameObjectShootState(RakNet::TimeMS curTime, const Ogre::Vector3 & position)
			:
			time(curTime),
			position(position)
		{
		}

		//////////////////////////////////////////////

		void GameObjectStateHistory::init(RakNet::TimeMS maxWriteInterval, RakNet::TimeMS maxHistoryTime, bool useIntervall)
		{
			//Intervall benutzen?
			this->useIntervall = useIntervall;
			//Setzen, wie lange Daten in die History gespeichert werden sollen, bis alte Werte entfernt werden wenn neue hinzukommen
			writeInterval = maxWriteInterval;
			//Aus der Speicherdauer und dem Intervall die Groeße der History berechnen
			maxLength = maxHistoryTime / maxWriteInterval + 1;
			//Queue: history
			this->history.ClearAndForceAllocation(maxLength + 1, _FILE_AND_LINE_);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectStateHistory] History size: 0-" + Ogre::StringConverter::toString(maxLength - 1));
			RakAssert(writeInterval > 0);
		}

		//Hier wird für einen Client jeweils gepusht
		void GameObjectStateHistory::enqeue(const Ogre::Vector3 & position, const Ogre::Quaternion & orientation, RakNet::TimeMS curTime)
		{
			//Wenn die History noch leer ist, kann direkt eingetragen werden
			if (this->history.Size() == 0)
			{
				history.Push(GameObjectState(curTime, position, orientation), _FILE_AND_LINE_);
				//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("s0pos: " + Ogre::StringConverter::toString(position));
			}
			else
			{
				//Queue: hinten neue Daten hinein, wenn Puffer voll ist, vorne rauswerfen
				const GameObjectState & lastEntityState = this->history.PeekTail();
				//Nur pushen, wenn die Zeit dazu gekommen ist, also wenn aktuelle zeit - zeit der letzten Pushung >= 100ms ist

				if (curTime - lastEntityState.time >= writeInterval || this->useIntervall)
				{
					//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("Eintrag nach: " + Ogre::StringConverter::toString(curTime - lastEntityState.time));
					history.Push(GameObjectState(curTime, position, orientation), _FILE_AND_LINE_);

					//for (int i = this->history.Size() - 1; i >= 0; i--)
					//	NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("i: " + Ogre::StringConverter::toString(i) + " - pos: " +  Ogre::StringConverter::toString(history[i].position) + " - t: " + Ogre::StringConverter::toString(history[i].time));
					//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("spos: " + Ogre::StringConverter::toString(position));
					if (history.Size() > maxLength)
					{
						history.Pop();
					}
				}
			}
		}

		void GameObjectStateHistory::readPiecewiseLinearInterpolated(Ogre::Vector3 & outPosition, Ogre::Quaternion & outOrientation, RakNet::TimeMS pastTime)
			//pastTime = 100 ms in der Vergangenheit also: curTime - 100; curTime aktuell vergangene Zeit in ms beim Client (Vergangenheitswert)
		{
			//Nichts bisher aufgezeichnet
			if (this->history.Size() == 0)
				return;

			//Interpolationsgrenzen behandeln
			if (pastTime <= this->history[0].time)
			{
				//In diesem Fall muss nicht interpoliert werden, da sich das Objekt am Startpunkt befindet
				outPosition = this->history[0].position;
				outOrientation = this->history[0].orientation;
				return;
			}
			else if (pastTime >= this->history[this->history.Size() - 1].time)
			{
				//In diesem Fall muss nicht interpoliert werden, da sich das Objekt am Endpunkt befindet.
				outPosition = this->history[this->history.Size() - 1].position;
				outOrientation = this->history[this->history.Size() - 1].orientation;
				return;
			}

			//Ansonsten befindet sich das Objekt irgendwo zwischen irgendwelchen 2 Punkten, jetzt gilt es herauszufinden zwischen welchen 2?
			int i = 0;
			//Von hinten nach vorne laufen (hinten sind die neueren Werte und vorne die Aelteren (Vergangenheit))
			for (i = history.Size() - 1; i >= 0; i--)
			{
				//Wenn der Vergangenheitszeitpunkt >= dem Zeitpunkt eines Punktes ist, dann wurde das Intervall zwischen den 2 Punkten gefunden
				if (pastTime >= this->history[i].time)
					break;
			}

			//Hier wird berechnet anhand der Vergangenheitszeit wie weit die Interpolation schon fortgeschritten ist
			//progress liegt im Intervall [0, 1], 0 ist die Startposition und 1 ist die Endposition
			Ogre::Real progress = (Ogre::Real)(pastTime - this->history[i].time) / (Ogre::Real)(this->history[i + 1].time - this->history[i].time);
			//Interpolieren!
			//outPosition = this->history[i].position + (this->history[i + 1].position - this->history[i].position) * progress;
			outPosition = (1.0f - progress) * this->history[i].position + progress * this->history[i + 1].position;
			outOrientation = Ogre::Quaternion::Slerp(progress, this->history[i].orientation, this->history[i + 1].orientation, true);

			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("position: " + Ogre::StringConverter::toString(position));
			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("zwischen zwei Werten: " + Ogre::StringConverter::toString(u));
			return;
		}

		//Zustand zum Index zurueckliefern
		GameObjectState GameObjectStateHistory::getState(unsigned int i)
		{
			return this->history[i];
		}

		//Letzten Zustand zurueckliefern
		GameObjectState GameObjectStateHistory::getLastState(void)
		{
			return this->history.PeekTail();
		}

		//History leeren
		void GameObjectStateHistory::clear(void)
		{
			this->history.Clear(_FILE_AND_LINE_);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void GameObjectShootStateHistory::init(RakNet::TimeMS maxWriteInterval, RakNet::TimeMS maxHistoryTime, bool useIntervall)
		{
			//Intervall benutzen?
			this->useIntervall = useIntervall;
			//Setzen, wie lange Daten in die History gespeichert werden sollen, bis alte Werte entfernt werden wenn neue hinzukommen
			writeInterval = maxWriteInterval;
			//Aus der Speicherdauer und dem Intervall die Groeße der History berechnen
			maxLength = maxHistoryTime / maxWriteInterval + 1;
			//Queue: history
			history.ClearAndForceAllocation(maxLength + 1, _FILE_AND_LINE_);
			Ogre::LogManager::getSingletonPtr()->logMessage("ShootStateHistorySize: 0-" + Ogre::StringConverter::toString(maxLength - 1));
			RakAssert(writeInterval > 0);
		}

		//Hier wird für einen Client jeweils gepusht
		void GameObjectShootStateHistory::enqeue(const Ogre::Vector3 & position, RakNet::TimeMS curTime)
		{
			//Wenn die History noch leer ist, kann direkt eingetragen werden
			if (this->history.Size() == 0)
			{
				history.Push(GameObjectShootState(curTime, position), _FILE_AND_LINE_);
				//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("s0pos: " + Ogre::StringConverter::toString(position));
			}
			else
			{
				//Queue: hinten neue Daten hinein, wenn Puffer voll, vorne rauswerfen
				const GameObjectShootState & lastEntityState = this->history.PeekTail();
				//Nur pushen, wenn die Zeit dazu gekommen ist, also wenn aktuelle zeit - zeit der letzten Pushung >= 100ms ist

				if (curTime - lastEntityState.time >= writeInterval || this->useIntervall)
				{
					//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("Eintrag nach: " + Ogre::StringConverter::toString(curTime - lastCell.time));
					history.Push(GameObjectShootState(curTime, position), _FILE_AND_LINE_);
					for (int i = this->history.Size() - 1; i >= 0; i--)

						//for (int i = this->history.Size() - 1; i >= 0; i--)
						//	NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("i: " + Ogre::StringConverter::toString(i) + " - pos: " +  Ogre::StringConverter::toString(history[i].position) + " - t: " + Ogre::StringConverter::toString(history[i].time));
						//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("spos: " + Ogre::StringConverter::toString(position));
						if (history.Size() > maxLength)
						{
							history.Pop();
						}
				}
			}
		}

		void GameObjectShootStateHistory::timewarp(GameObjectShootState& resultEntityState, RakNet::TimeMS pastTime)
		{
			//Interpolationsgrenzen behandeln
			if (pastTime <= this->history[0].time)
			{
				resultEntityState = this->history[0];
				return;
			}
			else if (pastTime >= this->history[this->history.Size() - 1].time)
			{
				//In diesem Fall muss nicht interpoliert werden, da sich das Objekt am Endpunkt befindet.
				resultEntityState = this->history[this->history.Size() - 1];
				return;
			}

			//Von hinten nach vorne laufen (hinten sind die neueren Werte und vorne die Aelteren)
			for (int i = history.Size() - 1; i >= 0; i--)
			{
				//Wenn der Vergangenheitszeitpunkt >= dem Zeitpunkt eines Punktes ist, dann wurde das Intervall zwischen den 2 Punkten gefunden
				if (pastTime >= this->history[i].time)
				{
					resultEntityState = this->history[i];
					//Ogre::LogManager::getSingletonPtr()->logMessage("match to index: " + Ogre::StringConverter::toString(i));
					return;
				}
			}
		}

		//Zustand zum Index zurueckliefern
		GameObjectShootState GameObjectShootStateHistory::getState(unsigned int i)
		{
			return this->history[i];
		}

		//Letzten Zustand zurueckliefern
		GameObjectShootState GameObjectShootStateHistory::getLastState(void)
		{
			return this->history.PeekTail();
		}

		//History leeren
		void GameObjectShootStateHistory::clear(void)
		{
			this->history.Clear(_FILE_AND_LINE_);
		}

	} // namespace end NET

} // namespace end NOWA