#ifndef PLAYER_DISTRIBUTED_COMPONENT_H
#define PLAYER_DISTRIBUTED_COMPONENT_H

#include "DistributedComponent.h"

namespace OBA
{
	using namespace NET;

	class EXPORTED DistributedPlayerComponent : public DistributedComponent
	{
	public:
		typedef boost::shared_ptr<DistributedPlayerComponent> DistributedPlayerCompPtr;
	public:

		DistributedPlayerComponent();

		virtual ~DistributedPlayerComponent();

		virtual bool init(rapidxml::xml_node<>*& pPropertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getName(void) const override;

		virtual GameObjectComponentPtr clone(void) override;

		static unsigned int getId(void)
		{
			return GameObjectComponent::getIdFromName("DistributedPlayerComponent");
		}

		virtual void update(Ogre::Real dt);

		virtual void WriteAllocationID(RakNet::Connection_RM3* destinationConnection, RakNet::BitStream* allocationIdBitstream) const;

		virtual RakNet::RM3ConstructionState QueryConstruction(RakNet::Connection_RM3* destinationConnection, RakNet::ReplicaManager3* replicaManager3);

		virtual bool QueryRemoteConstruction(RakNet::Connection_RM3* sourceConnection);

		virtual void SerializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* destinationConnection);

		virtual bool DeserializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection);

		virtual void SerializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* destinationConnection);

		virtual bool DeserializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* sourceConnection);

		virtual RakNet::RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3* droppedConnection) const;

		virtual void DeallocReplica(RakNet::Connection_RM3* sourceConnection);

		virtual RakNet::RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3* dDestinationConnection);

		virtual RakNet::RM3SerializationResult Serialize(RakNet::SerializeParameters* serializeParameters);

		virtual void Deserialize(RakNet::DeserializeParameters* deserializeParameters);

		// void setSystemAddress(const RakNet::SystemAddress& systemAddress);

		RakNet::SystemAddress getLocalAddress(void) const;

		void setReplication(const Ogre::String& replication)
		{
			this->replication = replication;
		}

		Ogre::String getReplication(void) const
		{
			return this->replication;
		}
	private:
		Ogre::String replication;

		RakNet::SystemAddress systemAddress;
		

////virtual bool handleEvent(const Event& ev) = 0;
	};

}; //namespace end

#endif