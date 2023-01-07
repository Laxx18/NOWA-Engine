#ifndef DISTRIBUTED_COMPONENT_H
#define DISTRIBUTED_COMPONENT_H

#include "ReplicaManager3.h"
#include "GetTime.h"
#include "RakNetTypes.h"
#include "VariableDeltaSerializer.h"
#include "DS_Queue.h"

#include "network/GameObjectStateHistory.h"
#include "gameobject/GameObjectComponent.h"

namespace NOWA
{
	class RakPeerInterface;
	class ReplicaManager3;
	class NetworkIDManager;
	class BitStream;
	class PhysicsComponent;

	using namespace NET;

	class EXPORTED DistributedComponent : public GameObjectComponent, public RakNet::Replica3
	{
	public:
		typedef boost::shared_ptr<DistributedComponent> DistributedCompPtr;

		struct AttributeData
		{
			Ogre::String attributeValue;
			Variant::Types attributeType = Variant::VAR_STRING;
		};
	public:

		friend class RakNetModule;

		DistributedComponent();

		virtual ~DistributedComponent();

		static DistributedComponent* replicateGameObject(Ogre::String& name, RakNet::ReplicaManager3* replicaManager3);

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("DistributedComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "DistributedComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Marks this game object as distributed via network, so that math transform and attributes are synced in a network scenario.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void WriteAllocationID(RakNet::Connection_RM3* destinationConnection, RakNet::BitStream* allocationIdBitstream) const override;

		virtual void SerializeConstructionExisting(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* destinationConnection) override;

		virtual void DeserializeConstructionExisting(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection) override;
	
		virtual RakNet::RM3ConstructionState QueryConstruction(RakNet::Connection_RM3* destinationConnection, RakNet::ReplicaManager3* replicaManager3) override;

		virtual bool QueryRemoteConstruction(RakNet::Connection_RM3* sourceConnection) override;

		virtual void SerializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* destinationConnection) override;

		virtual bool DeserializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection) override;

		virtual void SerializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* destinationConnection) override;

		virtual bool DeserializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* sourceConnection) override;

		virtual RakNet::RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3* droppedConnection) const override;

		virtual void DeallocReplica(RakNet::Connection_RM3* sourceConnection);

		virtual RakNet::RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3* dDestinationConnection) override;

		virtual void OnPoppedConnection(RakNet::Connection_RM3 *droppedConnection) override;

		virtual void OnUserReplicaPreSerializeTick(void) override;

		virtual RakNet::RM3SerializationResult Serialize(RakNet::SerializeParameters* serializeParameters) override;

		virtual void Deserialize(RakNet::DeserializeParameters* deserializeParameters) override;

		void NotifyReplicaOfMessageDeliveryStatus(RakNet::RakNetGUID guid, uint32_t receiptId, bool messageArrived);

		//IP Adresse als String speichern, da RakNet intern die Adressen veraendert, sodass man nicht mehr zuordnen kann, wer welche hat
		//Dies wird jedoch benoetigt, damit man z.B. Chatten kann, damit man weiﬂ von wem zu wem eine Nachricht ueber den Server geschickt werden soll
		void setStrSystemAddress(const Ogre::String& strSystemAddress);

		const Ogre::String& getStrSystemAddress(void) const;

		const RakNet::SystemAddress& getSystemAddress(void) const;

		void setClientID(int clientID);

		int getClientID(void) const;

		AttributeData getAttributeValue(const Ogre::String& attributeName);
// Attention: true type here later!
		void addAttribute(const Ogre::String& attributeName, const Ogre::String& attributeValue, Variant::Types attributeType);

		void changeValue(const Ogre::String& attributeName, const Ogre::String& newAttributeValue, Variant::Types attributeType);

		void changeValue(const Ogre::String& attribute);

		void removeAttribute(const Ogre::String& attributeName);

		std::map<Ogre::String, AttributeData> getAttributes(void);

	private:
		void internalSerializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* destinationConnection);

		bool internalDeserializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection);

		void setAttributes(std::map<Ogre::String, AttributeData> attributes);

		void serializeMovementData(RakNet::VariableDeltaSerializer::SerializationContext& serializationContext, RakNet::SerializeParameters* serializeParameters);

		void deserializeMovementData(RakNet::DeserializeParameters* deserializeParameters);

		void serializeMovementData(RakNet::SerializeParameters* serializeParameters);

		void deserializeMovementData(RakNet::VariableDeltaSerializer::DeserializationContext& deserializationContext, RakNet::DeserializeParameters* deserializeParameters);

		void serializeAttributes(RakNet::SerializeParameters* serializeParameters);

		void deserializeAttributes(RakNet::DeserializeParameters* deserializeParameters);

		void initData(void);

		void createSphereSceneQuery(void);
	protected:
		int	latencyToServer;
		int	oldLatencyToServer;
		int	latencyToRemotePlayer;
		int	highestLatencyToServer;
		int	interpolationTime;
		int	packetSendRate;
		int clientID;
		bool firstTime;
	
		Ogre::String strSystemAddress;
		RakNet::SystemAddress systemAddress;
		RakNet::Time timeStamp;
		Gamestate gamestate;
		GameObjectStateHistory entityStateHistory;
		boost::shared_ptr<PhysicsComponent> physicsActiveCompPtr;
		// Class to save and compare the states of variables this Serialize() to the last Serialize()
		// If the value is different, true is written to the bitStream, followed by the value. Otherwise false is written.
		// It also tracks which variables changed which Serialize() call, so if an unreliable message was lost (ID_SND_RECEIPT_LOSS) those variables are flagged 'dirty' and resent
		RakNet::VariableDeltaSerializer variableDeltaSerializer;

		std::map<Ogre::String, AttributeData> attributes;
		std::map<Ogre::String, AttributeData> attributesInit;

		Ogre::SphereSceneQuery*	sphereSceneQuery;

		struct AttributeWatcher
		{
			enum ChangedWhat
			{
				NOTHING_CHANGED = 0,
				VALUE_CHANGED = 1,
				ATTRIBUTE_ADDED = 2,
				ATTRIBUTE_REMOVED = 3
			};

			AttributeWatcher()
				: changedWhat(AttributeWatcher::NOTHING_CHANGED)
			{

			}

			AttributeWatcher(ChangedWhat changedWhat, Ogre::String attributeName, Ogre::String attributeValue, Variant::Types attributeType)
				: changedWhat(changedWhat),
				attributeName(attributeName),
				attributeValue(attributeValue),
				attributeType(attributeType)
			{

			}

			void clear(void)
			{
				this->changedWhat = AttributeWatcher::NOTHING_CHANGED;
				this->attributeName.clear();
				this->attributeValue.clear();
				this->attributeType = Variant::VAR_STRING;
			}

			ChangedWhat changedWhat;
			Ogre::String attributeName;
			Ogre::String attributeValue;
			Variant::Types attributeType;
		};
		
		DataStructures::Queue<AttributeWatcher> attributesQueue;
		RakNet::Time timeSinceLastAttributesSerialize;
	};

}; //namespace end

#endif