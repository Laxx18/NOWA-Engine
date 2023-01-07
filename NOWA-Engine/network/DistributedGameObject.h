#ifndef DISTRIBUTED_GAME_OBJECT_H
#define DISTRIBUTED_GAME_OBJECT_H

// Network
#include <RakPeerInterface.h>
#include <GetTime.h>
#include <StringTable.h>
#include <RakSleep.h>
#include <RakAssert.h>
#include <MessageIdentifiers.h>
#include <ReplicaManager3.h>
#include <NetworkIDManager.h>

// Ogre
#include <OGRE/OgreString.h>

#include "defines.h"

namespace OBA
{
	namespace NET
	{
		class RakAssert;
		class BitStream;

		class EXPORTED DistributedGameObject : public RakNet::Replica3
		{
		public:
			DistributedGameObject(bool distributed = false);

			virtual ~DistributedGameObject();

			static DistributedGameObject* createLocalGameObject(RakNet::ReplicaManager3* replicaManager3);
			
			static DistributedGameObject* createRemoteGameObject(RakNet::ReplicaManager3* replicaManager3);

			virtual void WriteAllocationID(RakNet::Connection_RM3* destinationConnection, RakNet::BitStream* allocationIdBitstream) const;
			
			virtual RakNet::RM3ConstructionState QueryConstruction(RakNet::Connection_RM3* destinationConnection, RakNet::ReplicaManager3* replicaManager3);
		
			virtual bool QueryRemoteConstruction(RakNet::Connection_RM3* sourceConnection);
			
			virtual void SerializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* destinationConnection);

			virtual bool DeserializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection);

			virtual void SerializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* destinationConnection);

			virtual bool DeserializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* sourceConnection);

			virtual RakNet::RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3* droppedConnection) const;
			
			virtual void DeallocReplica(RakNet::Connection_RM3* sourceConnection);

			virtual RakNet::RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3* destinationConnection);
			
			virtual RakNet::RM3SerializationResult Serialize(RakNet::SerializeParameters* serializeParameters);
			
			virtual void Deserialize(RakNet::DeserializeParameters* deserializeParameters);

			//IP Adresse als String speichern, da RakNet intern die Adressen veraendert, sodass man nicht mehr zuordnen kann, wer welche hat
			//Dies wird jedoch benoetigt, damit man z.B. Chatten kann, damit man weiﬂ von wem zu wem eine Nachricht ueber den Server geschickt werden soll
			void setStrSystemAddress(const Ogre::String& strSystemAddress);

			Ogre::String getStrSystemAddress(void) const;

			void setSystemAddress(const RakNet::SystemAddress& systemAddress);

			RakNet::SystemAddress getSystemAddress(void) const;

			void setDistributed(bool distributed);
			bool isDistributed(void) const;

		protected:
			
			int	latencyToServer;
			int	oldLatencyToServer;
			int	latencyToRemotePlayer;
			int	highestLatencyToServer;
			int	interpolationTime;
			int	packetSendRate;
			RakNet::SystemAddress systemAddress;
			Ogre::String strSystemAddress;
			RakNet::Time timeStamp;
			bool distributed;
		};

	} // namespace end NET

} // namespace end OBA

#endif

