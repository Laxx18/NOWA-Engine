#include "network/DistributedGameObject.h"

// modules
#include "modules/RakNetModule.h"

namespace OBA
{
	namespace NET
	{

		DistributedGameObject::DistributedGameObject(bool distributed)
			:	timeStamp(0),
				latencyToServer(20), // later calculate
				oldLatencyToServer(0), //wird nicht verwendet
				latencyToRemotePlayer(0),
				highestLatencyToServer(40), // later calculate the highest latency
				interpolationTime(RakNetModule::getInstance()->getInterpolationTime()),
				packetSendRate(RakNetModule::getInstance()->getPacketSendRateMS()),
				systemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS),
				distributed(distributed)
		{

		}

		DistributedGameObject::~DistributedGameObject(void)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("[DistributedGameObject] DistributedGameObject destroyed");
		}

		void DistributedGameObject::WriteAllocationID(RakNet::Connection_RM3* pDestinationConnection, RakNet::BitStream* pAllocationIdBitstream) const
		{
			// do nothing here for default
		}

		RakNet::RM3ConstructionState DistributedGameObject::QueryConstruction(RakNet::Connection_RM3* pDestinationConnection, 
			RakNet::ReplicaManager3* pReplicaManager3)
		{
			if (!this->distributed) {
				return RakNet::RM3CS_NEVER_CONSTRUCT;
			} else {
				return RakNet::RM3CS_ALREADY_EXISTS_REMOTELY;
			}
		}

		bool DistributedGameObject::QueryRemoteConstruction(RakNet::Connection_RM3* pSourceConnection)
		{
			if (!this->distributed) {
				return false;
			} else {
				return RakNet::RM3CS_ALREADY_EXISTS_REMOTELY;
			}
		}

		void DistributedGameObject::SerializeConstruction(RakNet::BitStream* pConstructionBitstream, RakNet::Connection_RM3* pDestinationConnection)
		{

		}

		bool DistributedGameObject::DeserializeConstruction(RakNet::BitStream* pConstructionBitstream, RakNet::Connection_RM3* pSourceConnection)
		{
			return false;
		}

		void DistributedGameObject::SerializeDestruction(RakNet::BitStream* pDestructionBitstream, RakNet::Connection_RM3* pDestinationConnection)
		{
		
		}

		bool DistributedGameObject::DeserializeDestruction(akNet::BitStream* pDestructionBitstream, RakNet::Connection_RM3* pSourceConnection)
		{
			return false;
		}

		RakNet::RM3ActionOnPopConnection DistributedGameObject::QueryActionOnPopConnection(RakNet::Connection_RM3* pDroppedConnection) const
		{
			return RakNet::RM3AOPC_DO_NOTHING;
		}

		void DistributedGameObject::DeallocReplica(RakNet::Connection_RM3* pSourceConnection) 
		{
			if (this->distributed) {
				delete this;
			}
		}
		
		RakNet::RM3QuerySerializationResult DistributedGameObject::QuerySerialization(RakNet::Connection_RM3* pDestinationConnection)
		{
			if (!this->distributed) {
				return RakNet::RM3QSR_NEVER_CALL_SERIALIZE;
			} else {
				if (RakNetModule::getInstance()->isServer()) {
					// RM3QSR_CALL_SERIALIZE
					// if its the server, it transmitts the data
					return QuerySerialization_ServerSerializable(pDestinationConnection, RakNetModule::getInstance()->isServer());
				} else {
					// if its the client, the client transmitts the data
					return QuerySerialization_ClientSerializable(pDestinationConnection, RakNetModule::getInstance()->isServer());
				}
			}
		}
		
		RakNet::RM3SerializationResult DistributedGameObject::Serialize(RakNet::SerializeParameters* pSerializeParameters)
		{
			if (!this->distributed) {
				return RakNet::RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION;
			} else {
				// Autoserialize causes a network packet to go out when any of these member variables change.
				RakAssert(RakNetModule::getInstance()->isServer() == true);
				/*serializeParameters->outputBitstream[0].Write(isKernel);
				serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&position,sizeof(position));
				serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&velocity,sizeof(velocity));
				serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&orientation,sizeof(orientation));*/
				return RakNet::RM3SR_BROADCAST_IDENTICALLY;
			}
		}	
		
		void DistributedGameObject::Deserialize(RakNet::DeserializeParameters* pDeserializeParameters)
		{
			if (this->distributed) {
				/*bool lastIsKernel = isKernel;

				// Doing this because we are also lagging position and orientation behind by DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES
				// Without it, the kernel would pop immediately but would not start moving
				deserializeParameters->serializationBitstream[0].Read(isKernel);
				if (isKernel==false && lastIsKernel==true)
				popCountdown=DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES;

				deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&position,sizeof(position));
				deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&velocity,sizeof(velocity));
				deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&orientation,sizeof(orientation));

				// Scene node starts invisible until we deserialize the intial startup data
				// This data could also have been passed in SerializeConstruction()
				sceneNode->setVisible(true,true);

				// Every time we get a network packet, we write it to the transformation history class.
				// This class, given a time in the past, can then return to us an interpolated position of where we should be in at that time
				transformationHistory.Write(position,velocity,orientation,RakNet::GetTimeMS());*/
			}
		}

		void DistributedGameObject::setSystemAddress(const RakNet::SystemAddress& systemAddress)
		{
			this->systemAddress = systemAddress;
		}

		RakNet::SystemAddress DistributedGameObject::getSystemAddress(void) const
		{
			return this->systemAddress;
		}

		void DistributedGameObject::setStrSystemAddress(const Ogre::String& strSystemAddress)
		{
			this->strSystemAddress = strSystemAddress;
		}

		Ogre::String DistributedGameObject::getStrSystemAddress(void) const
		{
			return this->strSystemAddress;
		}

		void DistributedGameObject::setDistributed(bool distributed)
		{
			this->distributed = distributed;
		}

		bool DistributedGameObject::isDistributed(void) const
		{
			return this->distributed;
		}

	} // namespace end NET

} // namespace end OBA