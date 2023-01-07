#include "DistributedPlayerComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include <boost/make_shared.hpp>

// Network
#include <RakPeerInterface.h>
#include <StringTable.h>
#include <RakSleep.h>
#include <RakAssert.h>
#include <MessageIdentifiers.h>
#include <NetworkIDManager.h>

#include "modules/RakNetModule.h"

#include "gameobject/PhysicsActiveComponent.h"

namespace OBA
{
	DistributedPlayerComponent::DistributedPlayerComponent()
		: DistributedComponent(),
		systemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS)
	{

	}

	DistributedPlayerComponent::~DistributedPlayerComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("[DistributedPlayerComponent] Destructor player distributed component for id: " + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
	}

	bool DistributedPlayerComponent::init(rapidxml::xml_node<>*& pPropertyElement, const Ogre::String& filename)
	{
		bool success = DistributedComponent::init(pPropertyElement, filename);

		pPropertyElement = pPropertyElement->next_sibling("property");
		if (pPropertyElement)
		{
			if (XMLConverter::getAttrib(pPropertyElement, "name") == "Replication")
			{
				this->replication = XMLConverter::getAttribBool(pPropertyElement, "data", "Server");
				pPropertyElement = pPropertyElement->next_sibling("property");
			}

		}
		return true;
	}

	GameObjectComponentPtr DistributedPlayerComponent::clone(void)
	{
		DistributedPlayerCompPtr clonedCompPtr = boost::static_pointer_cast<DistributedPlayerComponent>(DistributedComponent::clone());
		
		clonedCompPtr->setReplication(this->replication);
		

		return clonedCompPtr;

		// return boost::make_shared<DistributedPlayerComponent>(*this);
	}

	bool DistributedPlayerComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("[DistributedPlayerComponent] Init player distributed component for id: " + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));

		bool success = DistributedComponent::postInit();

		// set the system address for this client
		this->systemAddress = RakNetModule::getInstance()->getLocalAddress();

		return success;
	}

	void DistributedPlayerComponent::update(Ogre::Real dt)
	{
			if (!RakNetModule::getInstance()->isServer())
			{
				//Wenn keine Latenz herrscht wird etwas länger gepuffert um sicher zu gehen, das weit genug in die Vergangenheit gegangen wurde, ansonste gibt es kleine Störungen während der Bewegung
				unsigned long pastTime = 0;
				Ogre::Vector3 position;
				Ogre::Quaternion orientation;
				if (this->highestLatencyToServer < this->interpolationTime)
				{
					pastTime = unsigned long(17) + unsigned long(this->interpolationTime);
				}
				else
				{
					//Wenn die Latenz höher ist, als die Interpolationsspanne, dann wird die hoechste Latenz als Faktor genommen, da diese die Interpolationsspanne mit abdeckt.
					pastTime = unsigned long(17) + unsigned long(this->highestLatencyToServer);
				}

				//unsigned long pastTime = unsigned long(this->highestLatencyToServer)  + unsigned long(this->interpolationTime);
				//pastTime = unsigned long(dt*1000.0f) + unsigned long(this->interpolationTime) + unsigned long(this->highestLatencyToServer);

				//unsigned long pastTime = unsigned long(dt*1000.0f) + Flubber::pMyFlubber->latencyToServer + this->highestLatencyToServer + unsigned long(this->interpolationTime);
				//OBA::Ogre::LogManager::getSingletonPtr()->logMessage("pasttime: " + Ogre::StringConverter::toString(pastTime));
				//Read wird staendig 60x die Sekunde aufgerufen und dementsprechend sind die Intervalle 0.016 waherend der Interpolation
				this->entityStateHistory.readPiecewiseLinearInterpolated(position, orientation, RakNet::GetTimeMS() - pastTime);
				this->physicsActiveCompPtr->setPosition(position);
				this->physicsActiveCompPtr->setOrientation(orientation);

			}
			else
			{

				// server simulates a movement for testing
				// this->physicsActiveCompPtr->setVelocity(this->physicsActiveCompPtr->getVelocity() * Ogre::Vector3(0.0f, 1.0f, 0.0f) + 
				// 	(this->physicsActiveCompPtr->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Z) * this->physicsActiveCompPtr->getSpeed());
			}
	}

	Ogre::String DistributedPlayerComponent::getName(void) const
	{
		return "DistributedPlayerComponent";
	}

	void DistributedPlayerComponent::WriteAllocationID(RakNet::Connection_RM3* destinationConnection, RakNet::BitStream* allocationIdBitstream) const
	{
		// write the name of the scene node, so this component can be replicated, if wished
		RakNet::StringTable::Instance()->EncodeString(this->gameObjectPtr->getName().c_str(), 128, allocationIdBitstream);
	}

	RakNet::RM3ConstructionState DistributedPlayerComponent::QueryConstruction(RakNet::Connection_RM3* destinationConnection,
		RakNet::ReplicaManager3* replicaManager3)
	{
		
			return RakNet::RM3CS_ALREADY_EXISTS_REMOTELY;
	}

	bool DistributedPlayerComponent::QueryRemoteConstruction(RakNet::Connection_RM3* sourceConnection)
	{
		return RakNet::RM3CS_ALREADY_EXISTS_REMOTELY;
	}

	void DistributedPlayerComponent::SerializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* destinationConnection)
	{

	}

	bool DistributedPlayerComponent::DeserializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection)
	{
		return false;
	}

	void DistributedPlayerComponent::SerializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* destinationConnection)
	{

	}

	bool DistributedPlayerComponent::DeserializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* sourceConnection)
	{
		return false;
	}

	RakNet::RM3ActionOnPopConnection DistributedPlayerComponent::QueryActionOnPopConnection(RakNet::Connection_RM3* droppedConnection) const
	{
		return RakNet::RM3AOPC_DO_NOTHING;
	}

	void DistributedPlayerComponent::DeallocReplica(RakNet::Connection_RM3* sourceConnection)
	{
		if (this->distributed)
		{
			delete this;
		}
	}

	RakNet::RM3QuerySerializationResult DistributedPlayerComponent::QuerySerialization(RakNet::Connection_RM3* destinationConnection)
	{
		if (!this->distributed)
		{
			return RakNet::RM3QSR_NEVER_CALL_SERIALIZE;
		}
		else
		{
			if (RakNetModule::getInstance()->isServer())
			{
				// RM3QSR_CALL_SERIALIZE
				// if its the server, it transmitts the data
				return QuerySerialization_ServerSerializable(destinationConnection, RakNetModule::getInstance()->isServer());
				// Ogre::LogManager::getSingletonPtr()->logMessage("[Server] QuerySerialization");
				// return RakNet::RM3QSR_CALL_SERIALIZE;
			}
			else
			{
				// if its the client, the client transmitts the data
				return QuerySerialization_ClientSerializable(destinationConnection, RakNetModule::getInstance()->isServer());
				// Ogre::LogManager::getSingletonPtr()->logMessage("[Client] QuerySerialization");
				// return RakNet::RM3QSR_NEVER_CALL_SERIALIZE;
				// return RakNet::RM3QSR_CALL_SERIALIZE;
			}

		}
	}

	RakNet::RM3SerializationResult DistributedPlayerComponent::Serialize(RakNet::SerializeParameters* serializeParameters)
	{
		if (!this->distributed)
		{
			return RakNet::RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION;
		}
		else
		{
			// Autoserialize causes a network packet to go out when any of these member variables change.
			// RakAssert(RakNetModule::getInstance()->isServer() == true);
			// serializeParameters->destinationConnection->GetSystemAddress

			// if (RakNetModule::getInstance()->isServer()) {
			// do not send data to me itself
			// if (serializeParameters->destinationConnection->GetSystemAddress() != RakNetModule::getInstance()->getLocalSystemAddress()) {
			// server sents resulting pos/orient to clients
			if (RakNetModule::getInstance()->isServer())
			{
				Ogre::Vector3 position = this->physicsActiveCompPtr->getPosition();
				Ogre::Quaternion orientation = this->physicsActiveCompPtr->getOrientation();
				// Ogre::Vector3 velocity = this->pPhysicsBody->getVelocity();

				// if (RakNetModule::getInstance()->isServer()) {
				serializeParameters->outputBitstream[0].Write(RakNet::GetTime());
				// }
				// pSerializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)RakNet::GetTime(), sizeof(RakNet::GetTime()));
				serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&position, sizeof(position));
				// pSerializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&velocity, sizeof(velocity));
				serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&orientation, sizeof(orientation));
				Ogre::LogManager::getSingletonPtr()->logMessage("server writes");
			}
			else
			{
				// client only sends relative data (velocity, angular velocity) to server
				Ogre::Vector3 velocity = this->physicsActiveCompPtr->getVelocity();
				// orientation maybe transmitted directly even it is absolute, because with orientation, nearly no cheating is possible
				Ogre::Vector3 omega = this->physicsActiveCompPtr->getBody()->getOmega();

				if (velocity != Ogre::Vector3::ZERO || omega != Ogre::Vector3::ZERO)
				{
					serializeParameters->outputBitstream[0].Write(RakNet::GetTime());

					serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&velocity, sizeof(velocity));
					serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&omega, sizeof(omega));
					Ogre::LogManager::getSingletonPtr()->logMessage("v: " + Ogre::StringConverter::toString(velocity) + " o: " + Ogre::StringConverter::toString(omega));
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage("v o zero");
				}
			}
			return RakNet::RM3SR_BROADCAST_IDENTICALLY;
		}
	}

	void DistributedPlayerComponent::Deserialize(RakNet::DeserializeParameters* deserializeParameters)
	{
		if (this->distributed)
		{
			//bool lastIsKernel = isKernel;

			// Doing this because we are also lagging position and orientation behind by DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES
			// Without it, the kernel would pop immediately but would not start moving
			if (!RakNetModule::getInstance()->isServer())
			{
				Ogre::Vector3 position;
				Ogre::Quaternion orientation;
				// Ogre::Vector3 velocity;
				RakNet::Time timestampFromServer;

				deserializeParameters->serializationBitstream[0].Read(timestampFromServer);

				deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&position, sizeof(position));
				// pDeserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&velocity, sizeof(velocity));
				deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&orientation, sizeof(orientation));

				// Every time we get a network packet, we write it to the transformation history class.
				// This class, given a time in the past, can then return to us an interpolated position of where we should be in at that time
				// transformationHistory.Write(position,velocity,orientation,RakNet::GetTimeMS());


				//Vorher muss getestet werden, ob der Zeitstempel groesser ist als die jetzige Zeit, da es sich um unsigned handelt, wuerde man
				//ohne zu testen einfach abziehen: z.b. 23424534 - 23424535, dann kaeme bei signed -1 heraus, aber bei unsigned: 4294967394, da unsigned long 4 byte hat und es einen Ueberlauf gibt, somit wird vom Maximum 1 abgezogen
				//da unsigned nicht Minus werden kann!
				// if (RakNet::GetTime() > timestampFromServer + (unsigned long)(dt*1000.0f))
				// Flubber::pMyFlubber->latencyToServer = ((unsigned long)(RakNet::GetTime() - timestampFromServer - (unsigned long)(17)));
				// else
				// Flubber::pMyFlubber->latencyToServer = 0;

				//Tiefpassfilterberechnung, wird jedoch nicht angewandt!
				// Flubber::pMyFlubber->latencyToServer = OBA::Core::getSingletonPtr()->lowPassFilter((float)Flubber::pMyFlubber->latencyToServer, (float)Flubber::pMyFlubber->oldLatencyToServer);

				// Flubber::pMyFlubber->oldLatencyToServer = Flubber::pMyFlubber->latencyToServer;
				//this->latencyLabel->setCaption("Latenz: " + Ogre::StringConverter::toString(Flubber::pMyFlubber->highestLatencyToServer) + "ms");


				//vom Server erhaltenen Spielzustand in History fuer den lokalen Spieler einfuegen
				this->entityStateHistory.enqeue(position, orientation, RakNet::GetTimeMS());
			}
			else
			{
				Ogre::Vector3 velocity;
				Ogre::Vector3 omega;
				RakNet::Time timestampFromClient;

				deserializeParameters->serializationBitstream[0].Read(timestampFromClient);

				deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&velocity, sizeof(velocity));
				deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&omega, sizeof(omega));

				this->physicsActiveCompPtr->setVelocity(velocity);
				this->physicsActiveCompPtr->getBody()->setOmega(omega);
				Ogre::LogManager::getSingletonPtr()->logMessage("server reads");
			}
		}
	}

	/*void DistributedPlayerComponent::setSystemAddress(const RakNet::SystemAddress& systemAddress)
	{
	this->systemAddress = systemAddress;
	}*/

	RakNet::SystemAddress DistributedPlayerComponent::getLocalAddress(void) const
	{
		return this->systemAddress;
	}

}; // namespace end