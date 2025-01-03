#include "NOWAPrecompiled.h"
#include "DistributedComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "PhysicsActiveComponent.h"
#include "main/Events.h"
#include "main/AppStateManager.h"

// Network
#include "RakPeerInterface.h"
#include "StringTable.h"
#include "RakSleep.h"
#include "RakAssert.h"
#include "MessageIdentifiers.h"
#include "NetworkIDManager.h"
#include "modules/RakNetModule.h"

// must include these as cpp, because somehow there are link errors else, even if the raknet library had been build correctly with these
// so do not know why
#include "RakMemoryOverride.cpp"
#include "VariableListDeltaTracker.cpp"
#include "VariableDeltaSerializer.cpp"

namespace NOWA
{
	// DataStructures::List<DistributedComponent*> DistributedComponent::remoteComponents;
	// DataStructures::List<DistributedComponent*> DistributedComponent::localComponents;
	using namespace rapidxml;
	using namespace luabind;

	DistributedComponent::DistributedComponent()
		: GameObjectComponent(),
		timeStamp(0),
		latencyToServer(20), // later calculate
		oldLatencyToServer(0), //wird nicht verwendet
		latencyToRemotePlayer(0),
		highestLatencyToServer(40), // later calculate the highest latency
		interpolationTime(AppStateManager::getSingletonPtr()->getRakNetModule()->getInterpolationTime()),
		packetSendRate(AppStateManager::getSingletonPtr()->getRakNetModule()->getPacketSendRateMS()),
		clientID(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()),
		sphereSceneQuery(nullptr),
		timeSinceLastAttributesSerialize(0),
		firstTime(true)
	{
		// if its the client, get the local adress for this client
		this->systemAddress = AppStateManager::getSingletonPtr()->getRakNetModule()->getLocalAddress();
		this->strSystemAddress = Ogre::String(AppStateManager::getSingletonPtr()->getRakNetModule()->getLocalAddress().ToString());
	}

	DistributedComponent::~DistributedComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Destructor distributed component for game object: " + this->gameObjectPtr->getName());
		
		if (false == AppStateManager::getSingletonPtr()->getRakNetModule()->isNetworkSzenario())
		{
			return;
		}
		
		// here check against the client id from raknet module, since its unique for each client
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID() == this->gameObjectPtr->getControlledByClientID())
		{
			this->BroadcastDestruction();
		}
	}

	DistributedComponent* DistributedComponent::replicateGameObject(Ogre::String& name, RakNet::ReplicaManager3* replicaManager3)
	{
		GameObjectPtr gameObjectPtr = nullptr;
		// check if to be replicated game object that should be cloned from an existing one
		size_t pos = name.find("_cloned");
		if (pos != Ogre::String::npos)
		{
			name = name.substr(0, pos);
			// create and clone the game object
			gameObjectPtr = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->clone(name);
		}
		else
		{
			// trigger the parse game object event, so that it will be parsed in the scene loader
			// 0 means, the control by client does not matter
			boost::shared_ptr<NOWA::EventDataParseGameObject> parseGameObjectEvent(new NOWA::EventDataParseGameObject(name, 0));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(parseGameObjectEvent);
			// get the existing game object like player, that has been loaded already from the virtual environment but is invisible
			gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(name);
		}
		if (nullptr == gameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DistributedGameObject]: Could not replicate game object for the name: "
				+ name + " because it does not exist!");
			return nullptr;
		}

		auto distributedCompPtr = GameObjectComponent::makeStrongPtr(gameObjectPtr->getComponent<DistributedComponent>());
		if (!distributedCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedGameObject]: Does not exist skipping replication");
			return nullptr;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedGameObject]: Replicating game object for the name: " + name);
// Attention: only the data1 is used!
		distributedCompPtr->SetNetworkID(gameObjectPtr->getId());
		replicaManager3->Reference(distributedCompPtr.get());

		return distributedCompPtr.get();
	}

	bool DistributedComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		while (propertyElement)
		{
			Ogre::String name = XMLConverter::getAttrib(propertyElement, "name");
			Ogre::String value;

			if (Ogre::StringUtil::match(name, "Attribute*", true))
			{
				Ogre::String type = XMLConverter::getAttrib(propertyElement, "type");
				if ("7" == type)
				{
					value = XMLConverter::getAttrib(propertyElement, "data");
					DistributedComponent::AttributeData attributeData;
					attributeData.attributeValue = value;
					attributeData.attributeType = Variant::VAR_STRING;
					this->attributes.insert(std::make_pair(name, attributeData));
					this->attributesInit.insert(std::make_pair(name, attributeData));
				}
				else if ("2" == type)
				{
					value = Ogre::StringConverter::toString(XMLConverter::getAttribReal(propertyElement, "data"));
					DistributedComponent::AttributeData attributeData;
					attributeData.attributeValue = value;
					attributeData.attributeType = Variant::VAR_INT;
					this->attributes.insert(std::make_pair(name, attributeData));
					this->attributesInit.insert(std::make_pair(name, attributeData));
				}
				else if ("6" == type)
				{
					value = Ogre::StringConverter::toString(XMLConverter::getAttribReal(propertyElement, "data"));
					DistributedComponent::AttributeData attributeData;
					attributeData.attributeValue = value;
					attributeData.attributeType = Variant::VAR_REAL;
					this->attributes.insert(std::make_pair(name, attributeData));
					this->attributesInit.insert(std::make_pair(name, attributeData));
				}
				else if ("8" == type)
				{
					value = Ogre::StringConverter::toString(XMLConverter::getAttribVector2(propertyElement, "data"));
					DistributedComponent::AttributeData attributeData;
					attributeData.attributeValue = value;
					attributeData.attributeType = Variant::VAR_VEC2;
					this->attributes.insert(std::make_pair(name, attributeData));
					this->attributesInit.insert(std::make_pair(name, attributeData));
				}
				else if ("9" == type)
				{
					value = Ogre::StringConverter::toString(XMLConverter::getAttribVector3(propertyElement, "data"));
					DistributedComponent::AttributeData attributeData;
					attributeData.attributeValue = value;
					attributeData.attributeType = Variant::VAR_VEC3;
					this->attributes.insert(std::make_pair(name, attributeData));
					this->attributesInit.insert(std::make_pair(name, attributeData));
				}
				else if ("10" == type)
				{
					value = Ogre::StringConverter::toString(XMLConverter::getAttribVector4(propertyElement, "data"));
					// or quaternion
					DistributedComponent::AttributeData attributeData;
					attributeData.attributeValue = value;
					attributeData.attributeType = Variant::VAR_VEC4;
					this->attributes.insert(std::make_pair(name, attributeData));
					this->attributesInit.insert(std::make_pair(name, attributeData));
				}
				else if ("12" == type)
				{
					value = Ogre::StringConverter::toString(XMLConverter::getAttribBool(propertyElement, "data"));
					DistributedComponent::AttributeData attributeData;
					attributeData.attributeValue = value;
					attributeData.attributeType = Variant::VAR_BOOL;
					this->attributes.insert(std::make_pair(name, attributeData));
					this->attributesInit.insert(std::make_pair(name, attributeData));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			else
			{
				break;
			}
		}
		return true;
	}

	GameObjectCompPtr DistributedComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		DistributedCompPtr clonedCompPtr(boost::make_shared<DistributedComponent>());

		
		// main properties
		clonedCompPtr->setAttributes(this->attributesInit);

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		if (false == AppStateManager::getSingletonPtr()->getRakNetModule()->isNetworkSzenario())
		{
			GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
		}

		// test
		clonedGameObjectPtr->setControlledByClientID(this->clientID);

		AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.Reference(clonedCompPtr.get());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool DistributedComponent::postInit(void)
	{
		if (false == AppStateManager::getSingletonPtr()->getRakNetModule()->isNetworkSzenario())
		{
			return true;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Init distributed component for game object: " + this->gameObjectPtr->getName());

		// this object does not belong to any client and is a distributed dynamic object
		if (0 == this->gameObjectPtr->getControlledByClientID())
		{
			this->clientID = 0;
		}

		// add the component, if its controlled by a client
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID() == this->gameObjectPtr->getControlledByClientID())
		{
			// add the distributed game object to the local game objects list
			AppStateManager::getSingletonPtr()->getRakNetModule()->localComponents.Insert(this, _FILE_AND_LINE_);
		}
		// Attention later other component types too, if one is not present, check for another, even physicsartifactcomponent should be used, e.g. to distribute flags
		// get the physics active component
		this->physicsActiveCompPtr = GameObjectComponent::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsComponent>());

		// test to get an attribute value. The developer must know the correct type!
		Ogre::String jumpForce = this->getAttributeValue("AttributeJumpForce").attributeValue;
		if (!jumpForce.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Character jump force: " + jumpForce);
		}

		if (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID() == this->gameObjectPtr->getControlledByClientID())
		{
			this->SetNetworkID(this->gameObjectPtr->getId());
			// add the distributed game object reference to the replica manager
			AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.Reference(this);
		}
		else if (0 == this->gameObjectPtr->getControlledByClientID())
		{
			this->SetNetworkID(this->gameObjectPtr->getId());
			// add the distributed game object reference to the replica manager
			AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.Reference(this);
		}

		if (!AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			//init(..., ...) steuert wie lange die History ist und wie oft ein Wert gespeichert wird, init(100, 1000) bedeutet, das alle 100 ms ein Wert gespeichert wird und demnach 10 Werte in einer Sekunde
			//Historygroesse berechnen, diese muss gross genug sein um auch im Falle eine hoher Latenz und somit einem hohen Vergangenheitswert, 
			//genug abgespeicherte Werte zum Interpolieren zur Verfuegung zu haben
			unsigned int maxHistoryTime = 3 * this->interpolationTime + 4 * this->highestLatencyToServer;
			this->entityStateHistory.init(unsigned long(this->packetSendRate), maxHistoryTime);

			//Hier nur zu Debugzwecken
			unsigned long pastTime = 0;
			/*if (this->highestLatencyToServer < this->interpolationTime)
			{
			pastTime = unsigned long(17) + unsigned long(this->interpolationTime);
			}
			else
			{
			//Wenn die Latenz höher ist, als die Interpolationsspanne, dann wird die hoechste Latenz als Faktor genommen, da diese die Interpolationsspanne mit abdeckt.
			pastTime = unsigned long(17) + unsigned long(this->highestLatencyToServer);
			}*/
			pastTime = this->packetSendRate;

			// causes crash, because myDebugLog is at that point 0
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] max. history time: " + Ogre::StringConverter::toString(maxHistoryTime));
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] highest latency to server: " + Ogre::StringConverter::toString(highestLatencyToServer));
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] interpolation time : " + Ogre::StringConverter::toString(interpolationTime));
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] pasttime: " + Ogre::StringConverter::toString(pastTime));
		}

		return true;
	}

	void DistributedComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == AppStateManager::getSingletonPtr()->getRakNetModule()->isServer() && false == notSimulating && true == AppStateManager::getSingletonPtr()->getRakNetModule()->isNetworkSzenario())
		{
			unsigned long pastTime = 0;
			Ogre::Vector3 position;
			Ogre::Quaternion orientation;
			/*if (this->highestLatencyToServer < this->interpolationTime)
			{
				pastTime = unsigned long(17) + unsigned long(this->interpolationTime);
			}
			else
			{
				//Wenn die Latenz höher ist, als die Interpolationsspanne, dann wird die hoechste Latenz als Faktor genommen, da diese die Interpolationsspanne mit abdeckt.
				pastTime = unsigned long(17) + unsigned long(this->highestLatencyToServer);
			}*/
			pastTime = this->packetSendRate;


			this->entityStateHistory.readPiecewiseLinearInterpolated(position, orientation, RakNet::GetTimeMS() - pastTime);
			if (this->physicsActiveCompPtr)
			{
				this->physicsActiveCompPtr->setPosition(position);
				this->physicsActiveCompPtr->setOrientation(orientation);
			}
		}
	}

	void DistributedComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);
	}

	void DistributedComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);
	}

	Ogre::String DistributedComponent::getClassName(void) const
	{
		return "DistributedComponent";
	}

	Ogre::String DistributedComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void DistributedComponent::WriteAllocationID(RakNet::Connection_RM3* destinationConnection, RakNet::BitStream* allocationIdBitstream) const
	{
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Server write allocation id: " + Ogre::String(destinationConnection->GetSystemAddress().ToString())
				+ " for distributed game object: " + this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID));
			RakNet::StringTable::Instance()->EncodeString(this->gameObjectPtr->getName().c_str(), 128, allocationIdBitstream);
		}
		else if (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID() == this->gameObjectPtr->getControlledByClientID())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] write allocation id: " + Ogre::String(destinationConnection->GetSystemAddress().ToString())
				+ " for distributed game object: " + this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID));
			RakNet::StringTable::Instance()->EncodeString(this->gameObjectPtr->getName().c_str(), 128, allocationIdBitstream);
		}
	}

	RakNet::RM3ConstructionState DistributedComponent::QueryConstruction(RakNet::Connection_RM3* destinationConnection, RakNet::ReplicaManager3* replicaManager3)
	{
		if (0 == this->gameObjectPtr->getControlledByClientID())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Distributed game object: " + this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID)
				+ " already exists remotely");
			return RakNet::RM3CS_ALREADY_EXISTS_REMOTELY;
		}
		else if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Server query construction for destination connection: "
				+ Ogre::String(destinationConnection->GetSystemAddress().ToString())
				+ " for distributed game object: " + this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID));

			return QueryConstruction_ServerConstruction(destinationConnection, AppStateManager::getSingletonPtr()->getRakNetModule()->isServer());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client query construction for destination connection: "
				+ Ogre::String(destinationConnection->GetSystemAddress().ToString())
				+ " for distributed game object: " + this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID));

			return QueryConstruction_ClientConstruction(destinationConnection, AppStateManager::getSingletonPtr()->getRakNetModule()->isServer());
		}

		return RakNet::RM3CS_ALREADY_EXISTS_REMOTELY;
	}

	bool DistributedComponent::QueryRemoteConstruction(RakNet::Connection_RM3* sourceConnection)
	{
		if (0 == this->gameObjectPtr->getControlledByClientID())
		{
			return false;
		}
		else if(AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Server query remote construction from source connection: "
				+ Ogre::String(sourceConnection->GetSystemAddress().ToString()) + " for distributed game object: " 
				+ this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID));

			return QueryRemoteConstruction_ServerConstruction(sourceConnection, AppStateManager::getSingletonPtr()->getRakNetModule()->isServer());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client query remote construction from source connection: "
				+ Ogre::String(sourceConnection->GetSystemAddress().ToString()) + " for distributed game object: "
				+ this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID));

			return QueryRemoteConstruction_ClientConstruction(sourceConnection, AppStateManager::getSingletonPtr()->getRakNetModule()->isServer());
		}
	}

	void DistributedComponent::internalSerializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* destinationConnection)
	{
		// variableDeltaSerializer is a helper class that tracks what variables were sent to what remote system
		// This call adds another remote system to track
		this->variableDeltaSerializer.AddRemoteSystemVariableHistory(destinationConnection->GetRakNetGUID());

		RakNet::StringTable::Instance()->EncodeString(AppStateManager::getSingletonPtr()->getRakNetModule()->getLocalAddress().ToString(), 100, constructionBitstream);
		RakNet::StringTable::Instance()->EncodeString(AppStateManager::getSingletonPtr()->getRakNetModule()->getPlayerName().c_str(), 16, constructionBitstream);
		constructionBitstream->Write(this->clientID);
		constructionBitstream->Write(this->highestLatencyToServer);
		constructionBitstream->Write(this->interpolationTime);
		constructionBitstream->Write(this->packetSendRate);

		if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			// write the start position to the remote game object
			if (this->physicsActiveCompPtr)
			{
				Ogre::Vector3 position = this->physicsActiveCompPtr->getPosition();
				Ogre::Quaternion orientation = this->physicsActiveCompPtr->getOrientation();

				constructionBitstream->WriteAlignedBytes((const unsigned char*)&position, sizeof(position));
				constructionBitstream->WriteAlignedBytes((const unsigned char*)&orientation, sizeof(orientation));
			}
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent]: Serialize construction for id: "
			+ Ogre::StringConverter::toString(this->clientID) + " to: " + Ogre::String(destinationConnection->GetSystemAddress().ToString())
			+ " for object: " + this->gameObjectPtr->getName());

		// also send the size to determine whether to add the attribute or delete
		constructionBitstream->Write(this->attributes.size());
		// serialize attributes
		for (auto& it = this->attributes.cbegin(); it != this->attributes.cend(); ++it)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Serialize construction attribute: " + it->first + " with value: "
				+ it->second.attributeValue + " for client id: " + Ogre::StringConverter::toString(this->clientID));
			constructionBitstream->Write(RakNet::RakString(it->first.data()));
			constructionBitstream->Write(RakNet::RakString(it->second.attributeValue.data()));
		}
	}

	bool DistributedComponent::internalDeserializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection)
	{
		char playerName[16];
		int clientID;
		unsigned int highestLatencyToServer;
		unsigned int interpolationTime;
		unsigned int packetSendRate;
		size_t attributeSize;

		char strSystemAddress[100];

		RakNet::StringTable::Instance()->DecodeString(strSystemAddress, 100, constructionBitstream);
		RakNet::StringTable::Instance()->DecodeString(playerName, 16, constructionBitstream);
		constructionBitstream->Read(clientID);
		constructionBitstream->Read(highestLatencyToServer);
		constructionBitstream->Read(interpolationTime);
		constructionBitstream->Read(packetSendRate);

		this->strSystemAddress = Ogre::String(strSystemAddress);
		this->systemAddress = RakNet::SystemAddress(strSystemAddress);
		if (0 != clientID)
		{
			AppStateManager::getSingletonPtr()->getRakNetModule()->addRemotePlayerName(clientID, playerName);
			this->clientID = clientID;
		}
		this->highestLatencyToServer = highestLatencyToServer;
		this->interpolationTime = interpolationTime;
		this->packetSendRate = packetSendRate;

		// read the start position of remote game object
		if (this->physicsActiveCompPtr)
		{
			if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent]: Server deserialize construction for client id: "
					+ Ogre::StringConverter::toString(clientID) + " from: " + Ogre::String(sourceConnection->GetSystemAddress().ToString())
					+ " for object: " + this->gameObjectPtr->getName());

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent]: Server sets gameobject to correct start position for client id: "
					+ Ogre::StringConverter::toString(this->clientID) + " from: " + Ogre::String(sourceConnection->GetSystemAddress().ToString())
					+ " set position to: " + Ogre::StringConverter::toString(physicsActiveCompPtr->getPosition()) + " for object: " + this->gameObjectPtr->getName());
			}
			else
			{
				Ogre::Vector3 position;
				Ogre::Quaternion orientation;

				constructionBitstream->ReadAlignedBytes((unsigned char*)&position, sizeof(position));
				constructionBitstream->ReadAlignedBytes((unsigned char*)&orientation, sizeof(orientation));

				this->physicsActiveCompPtr->setPosition(position);
				this->physicsActiveCompPtr->setOrientation(orientation);
				
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent]: Client sets gameobject to correct start position for client id: "
					+ Ogre::StringConverter::toString(this->clientID) + " from: " + Ogre::String(sourceConnection->GetSystemAddress().ToString())
					+ " set position to: " + Ogre::StringConverter::toString(physicsActiveCompPtr->getPosition()) + " for object: " + this->gameObjectPtr->getName());

				//init(..., ...) steuert wie lange die History ist und wie oft ein Wert gespeichert wird, init(100, 1000) bedeutet, das alle 100 ms ein Wert gespeichert wird und demnach 10 Werte in einer Sekunde
				//Historygroesse berechnen, diese muss gross genug sein um auch im Falle eine hoher Latenz und somit einem hohen Vergangenheitswert, 
				//genug abgespeicherte Werte zum Interpolieren zur Verfuegung zu haben
				unsigned int maxHistoryTime = 3 * this->interpolationTime + 4 * this->highestLatencyToServer;
				this->entityStateHistory.init(unsigned long(this->packetSendRate), maxHistoryTime);

				//Hier nur zu Debugzwecken
				unsigned long pastTime = 0;
				/*if (this->highestLatencyToServer < this->interpolationTime)
				{
				pastTime = unsigned long(17) + unsigned long(this->interpolationTime);
				}
				else
				{
				//Wenn die Latenz höher ist, als die Interpolationsspanne, dann wird die hoechste Latenz als Faktor genommen, da diese die Interpolationsspanne mit abdeckt.
				pastTime = unsigned long(17) + unsigned long(this->highestLatencyToServer);
				}*/
				pastTime = this->packetSendRate;

				// causes crash, because myDebugLog is at that point 0
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] max. history time: " + Ogre::StringConverter::toString(maxHistoryTime));
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] highest latency to server: " + Ogre::StringConverter::toString(highestLatencyToServer));
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] interpolation time : " + Ogre::StringConverter::toString(interpolationTime));
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] pasttime: " + Ogre::StringConverter::toString(pastTime));


				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent]: Client deserialize construction for client id: "
					+ Ogre::StringConverter::toString(this->clientID) + " from: " + Ogre::String(sourceConnection->GetSystemAddress().ToString())
					+ " set position to: " + Ogre::StringConverter::toString(physicsActiveCompPtr->getPosition()) + " for object: " + this->gameObjectPtr->getName());

				// if this component is controlled by this client, add it to the map with the correct client id
				if (this->clientID == this->gameObjectPtr->getControlledByClientID())
				{
					if (!AppStateManager::getSingletonPtr()->getRakNetModule()->remoteComponents.Has(this->clientID))
					{
						DataStructures::List<DistributedComponent*> components;
						components.Insert(this, _FILE_AND_LINE_);

						AppStateManager::getSingletonPtr()->getRakNetModule()->remoteComponents.SetNew(this->clientID, components);
					}
					else
					{
						auto remoteComponentList = AppStateManager::getSingletonPtr()->getRakNetModule()->remoteComponents.Get(this->clientID);
						remoteComponentList.Insert(this, _FILE_AND_LINE_);
					}
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedGameObject]: Insert game object for the name: " + this->gameObjectPtr->getName()
						+ " clientID: " + Ogre::StringConverter::toString(this->clientID));
				}
			}
		}

		// treat attributes
		constructionBitstream->Read(attributeSize);

		// check whether to add the attribute or delete
		// 1 = add
		// 0 = remove
		// 2 = change
		// 3 = nothing
		unsigned short mode = 3;
		if (this->attributes.size() < attributeSize)
		{
			mode = 1;
		}
		else if (this->attributes.size() == attributeSize)
		{
			mode = 2;
		}
		else
		{
			mode = 0;
		}

		for (auto& it = this->attributes.cbegin(); it != this->attributes.cend(); ++it)
		{
			Ogre::String attributeName;
			Ogre::String attributeValue;

			constructionBitstream->Read(attributeName);
			constructionBitstream->Read(attributeValue);

			if (2 == mode)
			{
				if (attributeValue != this->attributes[attributeName].attributeValue)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Deserialize construction attribute value change: " + attributeName + " with value: "
						+ attributeValue + " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					this->attributes[attributeName].attributeValue = attributeValue;
				}
			}
			else
			{
				if (1 == mode)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Deserialize construction attribute insert: " + attributeName + " with value: "
						+ attributeValue + " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					AttributeData attributeData;
					// Type missing
					attributeData.attributeValue = attributeValue;
					this->attributes.insert(std::make_pair(attributeName, attributeData));
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Deserialize construction attribute remove: " + attributeName + " with value: "
						+ attributeValue + " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					this->attributes.erase(attributeName);
				}
			}
		}

		//Sprunghoehe berechnen
		// this->maxJumpVelocity = 12.0f - Ogre::Real(this->interpolationTime)  * 0.015f;
		//NOWA::Core::getSingletonPtr()->pMyDebugLog->logMessage("maxJumpVelocity: " + Ogre::StringConverter::toString(this->maxJumpVelocity));
		//NOWA::Core::getSingletonPtr()->pMyDebugLog->logMessage("bekommt id: " + Ogre::StringConverter::toString(this->playerID) + " Pos: " + Ogre::StringConverter::toString(this->networkPosition));

		//Physikalischen Koerper an die uebermittelten Lokationsdaten setzen
		// this->pPhysicsBody->setPositionOrientation(this->networkPosition, this->networkOrientation);

		/*if (!AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
		//Spielername setzen, damit er ueber dem Kopf angezeigt werden kann
		// this->pObjectTitle->setCaption(this->name + Ogre::StringConverter::toString(this->playerID) + " " + Ogre::StringConverter::toString(this->energy) + "%");

		//Historygroesse berechnen, diese muss gross genug sein um auch im Falle eine hoher Latenz und somit einem hohen Vergangenheitswert,
		//genug abgespeicherte Werte zum Interpolieren zur Verfuegung zu haben
		unsigned int maxHistoryTime = 3 * this->interpolationTime + 4 * this->highestLatencyToServer;
		//History initialisieren fuer Abspielverzoegerung und Interpolation
		this->entityStateHistory.init(unsigned long(this->packetSendRate), maxHistoryTime);
		NOWA::Core::getSingletonPtr()->pMyDebugLog->logMessage("##########entityStateHistory.init remote players: " + Ogre::StringConverter::toString(maxHistoryTime));

		////Flubber::remoteFlubbers.Insert(this, _FILE_AND_LINE_);
		}

		NOWA::Core::getSingletonPtr()->pMyDebugLog->logMessage("NEWSIZE: " + Ogre::StringConverter::toString(Flubber::remoteFlubbers.Size()));
		if (NOWA::Core::getSingletonPtr()->isServer)
		{
		////////Flubber::remoteFlubbers.Insert(this, playerID, _FILE_AND_LINE_);
		//Historygroesse berechnen, diese muss gross genug sein um auch im Falle eine hoher Latenz und somit einem hohen Vergangenheitswert,
		//genug abgespeicherte Werte zum Interpolieren zur Verfuegung zu haben
		unsigned int maxHistoryTime = 3 * this->interpolationTime + 4 * this->highestLatencyToServer;
		NOWA::Core::getSingletonPtr()->pMyDebugLog->logMessage("##########entityShootStateHistoryOfTheServer.init: " + Ogre::StringConverter::toString(maxHistoryTime));
		//Die History beim Server muss recht gross sein, da darin die Latenz fuer einen entfernten Spieler festgehalten werden muss!
		if (maxHistoryTime < 1000)
		maxHistoryTime = 1000;
		this->entityShootStateHistoryOfTheServer.init(unsigned long(this->packetSendRate), maxHistoryTime);

		//Server besitzt ein Menu um sich Informationen zu allen verbundenen Clients anzuzeigen
		//Spieler in Serverliste eintragen
		NOWA::Core::getSingletonPtr()->pParamsPanel->setParamValue(this->playerID, Ogre::String(pSourceConnection->GetSystemAddress().ToString()) + ":" + Ogre::String(this->name) + Ogre::StringConverter::toString(this->playerID) + ":" + Ogre::String(this->color) + ":Latenz/");
		}*/
		return true;
	}

	void DistributedComponent::SerializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* destinationConnection)
	{
		this->internalSerializeConstruction(constructionBitstream, destinationConnection);
	}

	bool DistributedComponent::DeserializeConstruction(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection)
	{
		return this->internalDeserializeConstruction(constructionBitstream, sourceConnection);
	}

	// Attention: Does not work, its not really implemented in RakNet!
	/// Same as SerializeConstruction(), but for an object that already exists on the remote system.
	/// Used if you return RM3CS_ALREADY_EXISTS_REMOTELY from QueryConstruction
	void DistributedComponent::SerializeConstructionExisting(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)
	{
		this->internalSerializeConstruction(constructionBitstream, destinationConnection);
	};

	/// Same as DeserializeConstruction(), but for an object that already exists on the remote system.
	/// Used if you return RM3CS_ALREADY_EXISTS_REMOTELY from QueryConstruction
	void DistributedComponent::DeserializeConstructionExisting(RakNet::BitStream* constructionBitstream, RakNet::Connection_RM3* sourceConnection)
	{
		this->internalDeserializeConstruction(constructionBitstream, sourceConnection);
	}

	void DistributedComponent::SerializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* destinationConnection)
	{
		//// variableDeltaSerializer is a helper class that tracks what variables were sent to what remote system
		this->variableDeltaSerializer.RemoveRemoteSystemVariableHistory(destinationConnection->GetRakNetGUID());

		//Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] SerializeDestruction : " + Ogre::StringConverter::toString(this->clientID)
		//	+ " for game object: " + this->gameObjectPtr->getName());

		if (0 != this->gameObjectPtr->getControlledByClientID())
		{
			if (!AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client sends destruction player ID : " + Ogre::StringConverter::toString(this->clientID));
				destructionBitstream->Write(this->clientID);
				destructionBitstream->Write(this->gameObjectPtr->getId());
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Server sends destruction player ID : " + Ogre::StringConverter::toString(this->clientID));
				destructionBitstream->Write(this->clientID);
				destructionBitstream->Write(this->gameObjectPtr->getId());
			}
		}
	}

	bool DistributedComponent::DeserializeDestruction(RakNet::BitStream* destructionBitstream, RakNet::Connection_RM3* sourceConnection)
	{
		int clientID;
		long gameObjectID;
		destructionBitstream->Read(clientID);
		destructionBitstream->Read(gameObjectID);
		// hier noch read component id, damit diese aus der Liste gelöscht werden kann und erst wenn die Liste leer ist, alles löschen
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedGameObject] deserialize destruction for object for id: " + Ogre::StringConverter::toString(clientID));

		auto list = AppStateManager::getSingletonPtr()->getRakNetModule()->remoteComponents.Get(clientID);

		int deleteId = -1;

		for (unsigned int i = 0; i < list.Size(); i++)
		{
			// Search for the players in list and delete
			if (list[i]->getOwner()->getId() == gameObjectID)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedGameObject] Remote game object deleted with id: "
					+ Ogre::StringConverter::toString(clientID) + " and index: " + Ogre::StringConverter::toString(i));
				deleteId = i;
			}
		}
		if (-1 != deleteId)
		{
			list.RemoveAtIndex(deleteId);
		}
		if (0 == list.Size())
		{
			AppStateManager::getSingletonPtr()->getRakNetModule()->remoteComponents.Delete(clientID);
		}
		return true;
	}

	RakNet::RM3ActionOnPopConnection DistributedComponent::QueryActionOnPopConnection(RakNet::Connection_RM3* droppedConnection) const
	{
		if (0 == this->gameObjectPtr->getControlledByClientID())
		{
			return RakNet::RM3AOPC_DO_NOTHING;
		}
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			return QueryActionOnPopConnection_Server(droppedConnection);
		}
		else
		{
			return QueryActionOnPopConnection_Client(droppedConnection);
		}
	}

	void DistributedComponent::OnPoppedConnection(RakNet::Connection_RM3 *droppedConnection)
	{
		// Same as in SerializeDestruction(), no longer track this system
		this->variableDeltaSerializer.RemoveRemoteSystemVariableHistory(droppedConnection->GetRakNetGUID());
	}

	void DistributedComponent::DeallocReplica(RakNet::Connection_RM3* sourceConnection)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Delete game object: " + this->gameObjectPtr->getName() + " client id: "
			+ Ogre::StringConverter::toString(this->clientID));
		AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(this->gameObjectPtr->getId());
	}

	RakNet::RM3QuerySerializationResult DistributedComponent::QuerySerialization(RakNet::Connection_RM3* destinationConnection)
	{
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			// if its the server, it transmitts the data
			return QuerySerialization_ServerSerializable(destinationConnection, AppStateManager::getSingletonPtr()->getRakNetModule()->isServer());
		}
		else
		{
			// if its the client, the client transmitts the data
			return QuerySerialization_ClientSerializable(destinationConnection, AppStateManager::getSingletonPtr()->getRakNetModule()->isServer());
		}
	}

	void DistributedComponent::OnUserReplicaPreSerializeTick(void)
	{
		/// Required by VariableDeltaSerializer::BeginIdenticalSerialize()
		this->variableDeltaSerializer.OnPreSerializeTick();
	}

	void DistributedComponent::NotifyReplicaOfMessageDeliveryStatus(RakNet::RakNetGUID guid, uint32_t receiptId, bool messageArrived)
	{
		// When using UNRELIABLE_WITH_ACK_RECEIPT, the system tracks which variables were updated with which sends
		// So it is then necessary to inform the system of messages arriving or lost
		// Lost messages will flag each variable sent in that update as dirty, meaning the next Serialize() call will resend them with the current values
		// this->variableDeltaSerializer.OnMessageReceipt(guid, receiptId, messageArrived);
	}

	void DistributedComponent::serializeMovementData(RakNet::SerializeParameters* serializeParameters)
	{
		serializeParameters->pro[0].reliability = UNRELIABLE;

		// do not send data to me itself
		if (serializeParameters->destinationConnection->GetSystemAddress() != AppStateManager::getSingletonPtr()->getRakNetModule()->getLocalAddress())
		{
			// server sents resulting pos/orient to clients
			if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
			{
				if (physicsActiveCompPtr)
				{
					// with area of interest
					if (this->sphereSceneQuery)
					{
						//In einem Bereich von 60 Metern nachschauen, ob sich irgend ein Spieler dort befindet
						//Diese entfernten Spieler sind von potentiellen Intresse fuer den aktuellen Spieler
						//Bearbeitung eingetragen
						Ogre::Sphere updateSphere(this->physicsActiveCompPtr->getPosition(), static_cast<Ogre::Real>(AppStateManager::getSingletonPtr()->getRakNetModule()->getAreaOfInterest()));
						this->sphereSceneQuery->setSphere(updateSphere);

						Ogre::SceneQueryResultMovableList& result = this->sphereSceneQuery->execute().movables;
						for (auto& it = result.cbegin(); it != result.cend(); ++it)
						{
							Ogre::MovableObject* movableObject = *it;

							

							if (movableObject->getQueryFlags() == this->gameObjectPtr->getMovableObject()->getQueryFlags())
							{
								// Flubber* pInterestingFlubber = Ogre::any_cast<Flubber*>(pMovableObject->getUserAny());
								// GameObjectPtr gameObjectPtr = Ogre::any_cast<GameObjectPtr>(movableObject->getUserAny());
								GameObject* gameObject = Ogre::any_cast<GameObject*>(movableObject->getUserObjectBindings().getUserAny());
								auto distributedComponent = NOWA::GameObjectComponent::makeStrongPtr(gameObject->getComponent<DistributedComponent>());
								if (distributedComponent)
								{
									Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Sphere for movable: " + movableObject->getName()
										+ " query flags: " + Ogre::StringConverter::toString(movableObject->getQueryFlags())
										+ " for distributed game object: " + this->gameObjectPtr->getName() + " with query flags: "
										+ Ogre::StringConverter::toString(this->gameObjectPtr->getMovableObject()->getQueryFlags())
										+ " system adress: " + Ogre::String(distributedComponent->getSystemAddress().ToString()) + " dest adress: "
										+ Ogre::String(serializeParameters->destinationConnection->GetSystemAddress().ToString()));
									// if (distributedComponent->getSystemAddress() == serializeParameters->destinationConnection->GetSystemAddress())
									{
										Ogre::Vector3 position = this->physicsActiveCompPtr->getPosition();
										Ogre::Quaternion orientation = this->physicsActiveCompPtr->getOrientation();

										serializeParameters->outputBitstream[0].Write(RakNet::GetTime());
										serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&position, sizeof(position));
										serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&orientation, sizeof(orientation));
										/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Server serialize position: " + Ogre::StringConverter::toString(position)
											+ " for distributed game object: " + this->gameObjectPtr->getName() 
											+ " to: " + Ogre::String(serializeParameters->destinationConnection->GetSystemAddress().ToString()));*/
									}
								}
							}
						}
					}
					else // without area of interest
					{
						Ogre::Vector3 position = this->physicsActiveCompPtr->getPosition();
						Ogre::Quaternion orientation = this->physicsActiveCompPtr->getOrientation();

						serializeParameters->outputBitstream[0].Write(RakNet::GetTime());
						serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&position, sizeof(position));
						serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&orientation, sizeof(orientation));
						/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Server serialize position: " + Ogre::StringConverter::toString(position)
							+ " for distributed game object: " + this->gameObjectPtr->getName() + " to: " + Ogre::String(serializeParameters->destinationConnection->GetSystemAddress().ToString()));*/
					}
				}
			}
			else
			{
				if (physicsActiveCompPtr)
				{
					// client only sends relative data (velocity, angular velocity) to server
					Ogre::Vector3 velocity = this->physicsActiveCompPtr->getBody()->getVelocity();
					// orientation maybe transmitted directly even it is absolute, because with orientation, nearly no cheating is possible
					Ogre::Vector3 omega = this->physicsActiveCompPtr->getBody()->getOmega();

					// if (velocity != Ogre::Vector3::ZERO || omega != Ogre::Vector3::ZERO)
					{
						// serializeParameters->outputBitstream[0].Write(RakNet::GetTime());
						// serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&velocity, sizeof(velocity));
						// serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&omega, sizeof(omega));
						// Ogre::LogManager::getSingletonPtr()->logMessage("v: " + Ogre::StringConverter::toString(velocity) + " o: " + Ogre::StringConverter::toString(omega));
						/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client serialize velocity: " + Ogre::StringConverter::toString(velocity)
							+ " for distributed game object: " + this->gameObjectPtr->getName() + " to: " + Ogre::String(serializeParameters->destinationConnection->GetSystemAddress().ToString()));*/
					}
				}
			}
		}
	}

	void DistributedComponent::serializeMovementData(RakNet::VariableDeltaSerializer::SerializationContext& serializationContext, RakNet::SerializeParameters* serializeParameters)
	{
		// server sents resulting pos/orient to clients
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			if (this->physicsActiveCompPtr)
			{
				// with area of interest
				if (this->sphereSceneQuery)
				{
					bool transmit = false;

					// if this is the serverside game object, send it for that client
					if (this->systemAddress == serializeParameters->destinationConnection->GetSystemAddress())
					{
						transmit = true;
					}
					else
					{
						// hier mal so versuchen: 
						// RemotePlayer für destination addresse erhalten,
						// sphere scene query auf der Position des anderen Spielers machen und dann schauen und senden
						//In einem Bereich von 60 Metern nachschauen, ob sich irgend ein Spieler dort befindet
						//Diese entfernten Spieler sind von potentiellen Intresse fuer den aktuellen Spieler
						//Bearbeitung eingetragen
						Ogre::Sphere updateSphere(this->physicsActiveCompPtr->getPosition(), static_cast<Ogre::Real>(AppStateManager::getSingletonPtr()->getRakNetModule()->getAreaOfInterest()));
						this->sphereSceneQuery->setSphere(updateSphere);

						Ogre::SceneQueryResultMovableList& result = this->sphereSceneQuery->execute().movables;
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] " + this->gameObjectPtr->getName() + " Sphere movable size: "
						// 	+ Ogre::StringConverter::toString(result.size()));

						DistributedCompPtr interestedDistributedComponent = nullptr;

						Ogre::String message = this->gameObjectPtr->getName() + ": found gameobjects: " + Ogre::StringConverter::toString(result.size()) + ": ";
						for (auto& it = result.cbegin(); it != result.cend(); ++it)
						{
							Ogre::MovableObject* movableObject = *it;

							GameObject* interestGameObject = Ogre::any_cast<GameObject*>(movableObject->getUserObjectBindings().getUserAny());
							message += interestGameObject->getName() + ", ";

							interestedDistributedComponent = NOWA::GameObjectComponent::makeStrongPtr(interestGameObject->getComponent<DistributedComponent>());
							if (0 != this->gameObjectPtr->getControlledByClientID())
							{
								//Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Sending to: " + movableObject->getName()
								//	+ " query flags: " + Ogre::StringConverter::toString(movableObject->getQueryFlags())
								//	+ " for distributed game object: " + this->gameObjectPtr->getName() + " with query flags: "
								//	+ Ogre::StringConverter::toString(this->gameObjectPtr->getEntity()->getQueryFlags())
								//	+ " movable system adress: " + Ogre::String(interestedDistributedComponent->getSystemAddress().ToString())
								//	// + " this system adress: " + Ogre::String(this->systemAddress.ToString())
								//	+ " dest adress: " + Ogre::String(serializeParameters->destinationConnection->GetSystemAddress().ToString()));
							}
							if (interestedDistributedComponent->getSystemAddress() == serializeParameters->destinationConnection->GetSystemAddress()
								&& interestGameObject->getName() != this->gameObjectPtr->getName())
							{
								transmit = true;
								break;
							}
						}
						Ogre::LogManager::getSingletonPtr()->logMessage(message);
					}

					
					if (transmit)
					{
						/*if (0 != this->gameObjectPtr->getControlledByClientID())
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Sending to: " + movableObject->getName()
								+ " query flags: " + Ogre::StringConverter::toString(movableObject->getQueryFlags())
								+ " for distributed game object: " + this->gameObjectPtr->getName() + " with query flags: "
								+ Ogre::StringConverter::toString(this->gameObjectPtr->getEntity()->getQueryFlags())
								+ " movable system adress: " + Ogre::String(interestedDistributedComponent->getSystemAddress().ToString())
								// + " this system adress: " + Ogre::String(this->systemAddress.ToString())
								+ " dest adress: " + Ogre::String(serializeParameters->destinationConnection->GetSystemAddress().ToString()));
						}*/
						// Put all variables to be sent unreliably on the same channel, then specify the send type for that channel
						// serializeParameters->pro[0].reliability = UNRELIABLE_WITH_ACK_RECEIPT;
						serializeParameters->pro[0].reliability = UNRELIABLE;
						// Sending unreliably with an ack receipt requires the receipt number, and that you inform the system of ID_SND_RECEIPT_ACKED and ID_SND_RECEIPT_LOSS
						// serializeParameters->pro[0].sendReceipt = replicaManager->GetRakPeerInterface()->IncrementNextSendReceipt();
						// serializeParameters->messageTimestamp = RakNet::GetTime();

						// Begin writing all variables to be sent UNRELIABLE_WITH_ACK_RECEIPT 
						// variableDeltaSerializer.BeginUnreliableAckedSerialize(&serializationContext, serializeParameters->destinationConnection->GetRakNetGUID(),
						// 	&serializeParameters->outputBitstream[0], serializeParameters->pro[0].sendReceipt);
						variableDeltaSerializer.BeginIdenticalSerialize(&serializationContext, serializeParameters->whenLastSerialized == 0, &serializeParameters->outputBitstream[0]);

						Ogre::Vector3 position = this->physicsActiveCompPtr->getPosition();
						Ogre::Quaternion orientation = this->physicsActiveCompPtr->getOrientation();

						// Write each variable
						variableDeltaSerializer.SerializeVariable(&serializationContext, RakNet::GetTime());
						variableDeltaSerializer.SerializeVariable(&serializationContext, position.x);
						variableDeltaSerializer.SerializeVariable(&serializationContext, position.y);
						variableDeltaSerializer.SerializeVariable(&serializationContext, position.z);
						variableDeltaSerializer.SerializeVariable(&serializationContext, orientation.w);
						variableDeltaSerializer.SerializeVariable(&serializationContext, orientation.x);
						variableDeltaSerializer.SerializeVariable(&serializationContext, orientation.y);
						variableDeltaSerializer.SerializeVariable(&serializationContext, orientation.z);
						// Tell the system this is the last variable to be written
						variableDeltaSerializer.EndSerialize(&serializationContext);
						return;
					}
					/*else
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Not sending to: " + movableObject->getName()
							+ " system adress: " + Ogre::String(interestedDistributedComponent->getSystemAddress().ToString()));
					}*/
				}
				else
				{
					// Put all variables to be sent unreliably on the same channel, then specify the send type for that channel
					// serializeParameters->pro[0].reliability = UNRELIABLE_WITH_ACK_RECEIPT;
					serializeParameters->pro[0].reliability = UNRELIABLE;
					// Sending unreliably with an ack receipt requires the receipt number, and that you inform the system of ID_SND_RECEIPT_ACKED and ID_SND_RECEIPT_LOSS
					// serializeParameters->pro[0].sendReceipt = replicaManager->GetRakPeerInterface()->IncrementNextSendReceipt();
					// serializeParameters->messageTimestamp = RakNet::GetTime();

					// Begin writing all variables to be sent UNRELIABLE_WITH_ACK_RECEIPT 
					// variableDeltaSerializer.BeginUnreliableAckedSerialize(&serializationContext, serializeParameters->destinationConnection->GetRakNetGUID(),
					// 	&serializeParameters->outputBitstream[0], serializeParameters->pro[0].sendReceipt);
					variableDeltaSerializer.BeginIdenticalSerialize(&serializationContext, serializeParameters->whenLastSerialized == 0, &serializeParameters->outputBitstream[0]);

					Ogre::Vector3 position = this->physicsActiveCompPtr->getPosition();
					Ogre::Quaternion orientation = this->physicsActiveCompPtr->getOrientation();

					// Write each variable
					variableDeltaSerializer.SerializeVariable(&serializationContext, RakNet::GetTime());
					variableDeltaSerializer.SerializeVariable(&serializationContext, position.x);
					variableDeltaSerializer.SerializeVariable(&serializationContext, position.y);
					variableDeltaSerializer.SerializeVariable(&serializationContext, position.z);
					variableDeltaSerializer.SerializeVariable(&serializationContext, orientation.w);
					variableDeltaSerializer.SerializeVariable(&serializationContext, orientation.x);
					variableDeltaSerializer.SerializeVariable(&serializationContext, orientation.y);
					variableDeltaSerializer.SerializeVariable(&serializationContext, orientation.z);
					// Tell the system this is the last variable to be written
					variableDeltaSerializer.EndSerialize(&serializationContext);
				}
			}
		}
		else // Client
		{
			/*if (this->physicsActiveCompPtr)
			{
				// Put all variables to be sent unreliably on the same channel, then specify the send type for that channel
				// serializeParameters->pro[0].reliability = UNRELIABLE_WITH_ACK_RECEIPT;
				serializeParameters->pro[0].reliability = UNRELIABLE;
				// Sending unreliably with an ack receipt requires the receipt number, and that you inform the system of ID_SND_RECEIPT_ACKED and ID_SND_RECEIPT_LOSS
				// serializeParameters->pro[0].sendReceipt = replicaManager->GetRakPeerInterface()->IncrementNextSendReceipt();
				// serializeParameters->messageTimestamp = RakNet::GetTime();

				// Begin writing all variables to be sent UNRELIABLE_WITH_ACK_RECEIPT 
				// variableDeltaSerializer.BeginUnreliableAckedSerialize(&serializationContext, serializeParameters->destinationConnection->GetRakNetGUID(),
				// 	&serializeParameters->outputBitstream[0], serializeParameters->pro[0].sendReceipt);
				variableDeltaSerializer.BeginIdenticalSerialize(&serializationContext, serializeParameters->whenLastSerialized == 0, &serializeParameters->outputBitstream[0]);

				// client only sends relative data (velocity, angular velocity) to server
				Ogre::Vector3 velocity = this->physicsActiveCompPtr->getVelocity();
				// orientation maybe transmitted directly even it is absolute, because with orientation, nearly no cheating is possible
				Ogre::Vector3 omega = this->physicsActiveCompPtr->getBody()->getOmega();

				// Write each variable
				variableDeltaSerializer.SerializeVariable(&serializationContext, RakNet::GetTime());
				variableDeltaSerializer.SerializeVariable(&serializationContext, velocity.x);
				variableDeltaSerializer.SerializeVariable(&serializationContext, velocity.y);
				variableDeltaSerializer.SerializeVariable(&serializationContext, velocity.z);
				variableDeltaSerializer.SerializeVariable(&serializationContext, omega.x);
				variableDeltaSerializer.SerializeVariable(&serializationContext, omega.y);
				variableDeltaSerializer.SerializeVariable(&serializationContext, omega.z);
				// Tell the system this is the last variable to be written
				variableDeltaSerializer.EndSerialize(&serializationContext);
			}*/
		}
	}

	void DistributedComponent::deserializeMovementData(RakNet::DeserializeParameters* deserializeParameters)
	{
		// Doing this because we are also lagging position and orientation behind by DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES
		// Without it, the kernel would pop immediately but would not start moving
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			if (this->physicsActiveCompPtr)
			{
				if (deserializeParameters->bitstreamWrittenTo[0])
				{
					// Ogre::Vector3 velocity;
					// Ogre::Vector3 omega;
					// RakNet::Time timestampFromClient;

					// deserializeParameters->serializationBitstream[0].Read(timestampFromClient);

					// deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&velocity, sizeof(velocity));
					// deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&omega, sizeof(omega));

					/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Server velocity changed: " + Ogre::StringConverter::toString(velocity)
					 	+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName() 
						+ " from: " + Ogre::String(deserializeParameters->sourceConnection->GetSystemAddress().ToString()));*/

					// this->physicsActiveCompPtr->setVelocity(velocity);
					// this->physicsActiveCompPtr->getBody()->setOmega(omega);
				}
			}
		}
		else // Client
		{
			if (this->physicsActiveCompPtr)
			{
				if (deserializeParameters->bitstreamWrittenTo[0])
				{
					Ogre::Vector3 position;
					Ogre::Quaternion orientation;
					// Ogre::Vector3 velocity;
					RakNet::Time timestampFromServer;

					deserializeParameters->serializationBitstream[0].Read(timestampFromServer);

					deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&position, sizeof(position));
					// pDeserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&velocity, sizeof(velocity));
					deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&orientation, sizeof(orientation));

					/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client position changed: " + Ogre::StringConverter::toString(position)
						+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName()
						+ " from: " + Ogre::String(deserializeParameters->sourceConnection->GetSystemAddress().ToString()));*/

					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client position changed: " + Ogre::StringConverter::toString(position)
					// 	+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					//vom Server erhaltenen Spielzustand in History fuer den lokalen Spieler einfuegen
					this->entityStateHistory.enqeue(position, orientation, RakNet::GetTimeMS());
				}

				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ReplicatedComponent] Client deserialize position: " + Ogre::StringConverter::toString(position)
				// 	+ " for distributed game object: " + this->gameObjectPtr->getName() + " from: " + Ogre::String(deserializeParameters->sourceConnection->GetSystemAddress().ToString()));

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
				// Flubber::pMyFlubber->latencyToServer = NOWA::Core::getSingletonPtr()->lowPassFilter((float)Flubber::pMyFlubber->latencyToServer, (float)Flubber::pMyFlubber->oldLatencyToServer);

				// Flubber::pMyFlubber->oldLatencyToServer = Flubber::pMyFlubber->latencyToServer;
				//this->latencyLabel->setCaption("Latenz: " + Ogre::StringConverter::toString(Flubber::pMyFlubber->highestLatencyToServer) + "ms");
			}
		}
	}

	void DistributedComponent::deserializeMovementData(RakNet::VariableDeltaSerializer::DeserializationContext& deserializationContext, RakNet::DeserializeParameters* deserializeParameters)
	{
		// Doing this because we are also lagging position and orientation behind by DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES
		// Without it, the kernel would pop immediately but would not start moving
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			/*if (this->physicsActiveCompPtr)
			{
				if (deserializeParameters->bitstreamWrittenTo[0])
				{
					RakNet::Time timestampFromClient;
					Ogre::Vector3 velocity = this->physicsActiveCompPtr->getVelocity();
					Ogre::Vector3 omega = this->physicsActiveCompPtr->getBody()->getOmega();

					this->variableDeltaSerializer.BeginDeserialize(&deserializationContext, &deserializeParameters->serializationBitstream[0]);
					if (this->variableDeltaSerializer.DeserializeVariable(&deserializationContext, timestampFromClient))
					{

					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, velocity.x))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client velocity x changed: " + Ogre::StringConverter::toString(velocity.x)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, velocity.y))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client velocity y changed: " + Ogre::StringConverter::toString(velocity.y)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, velocity.z))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client velocity z changed: " + Ogre::StringConverter::toString(velocity.z)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}

					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, omega.x))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client omega x changed: " + Ogre::StringConverter::toString(omega.x)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, omega.y))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client omega y changed: " + Ogre::StringConverter::toString(omega.y)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, omega.z))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client omega z changed: " + Ogre::StringConverter::toString(omega.z)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}

					this->physicsActiveCompPtr->setVelocity(velocity);
					this->physicsActiveCompPtr->getBody()->setOmega(omega);

					variableDeltaSerializer.EndDeserialize(&deserializationContext);
				}
			}*/
		}
		else // Client
		{
			if (this->physicsActiveCompPtr)
			{
				if (deserializeParameters->bitstreamWrittenTo[0])
				{
					RakNet::Time timestampFromServer;
					Ogre::Vector3 position = this->physicsActiveCompPtr->getPosition();
					Ogre::Quaternion orientation = this->physicsActiveCompPtr->getOrientation();

					variableDeltaSerializer.BeginDeserialize(&deserializationContext, &deserializeParameters->serializationBitstream[0]);
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, timestampFromServer))
					{

					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, position.x))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client position x changed: " + Ogre::StringConverter::toString(position.x)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, position.y))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client position y changed: " + Ogre::StringConverter::toString(position.y)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, position.z))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client position z changed: " + Ogre::StringConverter::toString(position.z)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}

					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, orientation.w))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client orientation w changed: " + Ogre::StringConverter::toString(orientation.w)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, orientation.x))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client orientation x changed: " + Ogre::StringConverter::toString(orientation.x)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, orientation.y))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client orientation y changed: " + Ogre::StringConverter::toString(orientation.y)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}
					if (variableDeltaSerializer.DeserializeVariable(&deserializationContext, orientation.z))
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client orientation z changed: " + Ogre::StringConverter::toString(orientation.z)
							+ " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
					}

					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client deserialize position: " + Ogre::StringConverter::toString(position)
					// 	+ " for distributed game object: " + this->gameObjectPtr->getName() + " from: " + Ogre::String(deserializeParameters->sourceConnection->GetSystemAddress().ToString()));

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
					// Flubber::pMyFlubber->latencyToServer = NOWA::Core::getSingletonPtr()->lowPassFilter((float)Flubber::pMyFlubber->latencyToServer, (float)Flubber::pMyFlubber->oldLatencyToServer);

					// Flubber::pMyFlubber->oldLatencyToServer = Flubber::pMyFlubber->latencyToServer;
					//this->latencyLabel->setCaption("Latenz: " + Ogre::StringConverter::toString(Flubber::pMyFlubber->highestLatencyToServer) + "ms");


					//vom Server erhaltenen Spielzustand in History fuer den lokalen Spieler einfuegen
					this->entityStateHistory.enqeue(position, orientation, RakNet::GetTimeMS());

					this->variableDeltaSerializer.EndDeserialize(&deserializationContext);
				}
			}
		}
	}

	void DistributedComponent::initData(void)
	{
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->isServer())
		{
			// if its the server, the server controls the clients, hence he needs the adresses for the clients
			// e.g. for area of interest, to check which client to send data etc.
			// this->clientID = AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID();
			// this->systemAddress = AppStateManager::getSingletonPtr()->getRakNetModule()->getClients()[this->clientID];
			// this->strSystemAddress = AppStateManager::getSingletonPtr()->getRakNetModule()->getClients()[this->clientID].ToString();

			for (unsigned int i = 0; i < AppStateManager::getSingletonPtr()->getRakNetModule()->getClients().size(); i++)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] #Addresses of clients: " + Ogre::String(AppStateManager::getSingletonPtr()->getRakNetModule()->getClients()[i].ToString()));
			}
			
			if (0 != AppStateManager::getSingletonPtr()->getRakNetModule()->getAreaOfInterest() && 0 != this->gameObjectPtr->getControlledByClientID())
			{
				this->createSphereSceneQuery();
			}

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] #Address: " + Ogre::String(systemAddress.ToString()));
		}
	}

	void DistributedComponent::createSphereSceneQuery(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Create sphere scene query distributed game object id: "
			+ Ogre::StringConverter::toString(this->gameObjectPtr->getId())
			+ " and query flags: " + Ogre::StringConverter::toString(this->gameObjectPtr->getMovableObject()->getQueryFlags()));
		this->sphereSceneQuery = this->gameObjectPtr->getSceneManager()->createSphereQuery(Ogre::Sphere(this->gameObjectPtr->getPosition(),
			static_cast<Ogre::Real>(AppStateManager::getSingletonPtr()->getRakNetModule()->getAreaOfInterest())), AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId("+Player+movable"));

		// this->gameObjectPtr->getEntity()->getQueryFlags()
	}

	RakNet::RM3SerializationResult DistributedComponent::Serialize(RakNet::SerializeParameters* serializeParameters)
	{
		if (this->firstTime)
		{
			this->initData();
			this->firstTime = false;
		}
		
		RakNet::VariableDeltaSerializer::SerializationContext serializationContext;
	
		this->serializeMovementData(serializationContext, serializeParameters);
	
		RakNet::Time time = RakNet::GetTimeMS();
		// call just once a seconds
		if (time - this->timeSinceLastAttributesSerialize >= 1000)
		{
			this->serializeAttributes(/*serializationContext,*/ serializeParameters);
			this->timeSinceLastAttributesSerialize = time;
		}
	
		// This return type makes is to ReplicaManager3 itself does not do a memory compare. we entirely control serialization ourselves here.
		// Use RM3SR_SERIALIZED_ALWAYS instead of RM3SR_SERIALIZED_ALWAYS_IDENTICALLY to support sending different data to different system, which is needed when using unreliable and dirty variable resends
		// return RakNet::RM3SR_SERIALIZED_ALWAYS;
		return RakNet::RM3SR_BROADCAST_IDENTICALLY;
	}

	void DistributedComponent::Deserialize(RakNet::DeserializeParameters* deserializeParameters)
	{
		RakNet::VariableDeltaSerializer::DeserializationContext deserializationContext;
	
		this->deserializeMovementData(deserializationContext, deserializeParameters);

		this->deserializeAttributes(/*deserializationContext,*/ deserializeParameters);
	}

	void DistributedComponent::serializeAttributes(RakNet::SerializeParameters* serializeParameters)
	{
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->remoteComponents.Size() == AppStateManager::getSingletonPtr()->getRakNetModule()->getAllowedPlayerCount())
		{
			for (unsigned int i = 0; i < this->attributesQueue.Size(); i++)
			{
				// hier nur wenn auch der Client connected hat, also server starten und dann viel später erst client
				serializeParameters->pro[1].reliability = RELIABLE_ORDERED;
				// serialize the attributes of the character if changed
				serializeParameters->outputBitstream[1].Write(static_cast<unsigned short>(this->attributesQueue[i].changedWhat));
				serializeParameters->outputBitstream[1].Write(RakNet::RakString(this->attributesQueue[i].attributeName.data()));
				serializeParameters->outputBitstream[1].Write(RakNet::RakString(this->attributesQueue[i].attributeValue.data()));
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Serialize attribute name: " + this->attributesQueue[i].attributeName + " with value: "
					+ this->attributesQueue[i].attributeValue + " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
				this->attributesQueue.Pop();
				i--;
			}
		}
	}

	void DistributedComponent::deserializeAttributes(RakNet::DeserializeParameters* deserializeParameters)
	{
		if (deserializeParameters->bitstreamWrittenTo[1])
		{
			RakNet::RakString attributeName;
			RakNet::RakString attributeValue;
			unsigned short changedWhat;
			deserializeParameters->serializationBitstream[1].Read(changedWhat);
			if (AttributeWatcher::VALUE_CHANGED == changedWhat)
			{
				deserializeParameters->serializationBitstream[1].Read(attributeName);
				deserializeParameters->serializationBitstream[1].Read(attributeValue);
				this->attributes[attributeName.C_String()].attributeValue = attributeValue.C_String();
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Deserialize attribute name for change: " + Ogre::String(attributeName.C_String()) + " with value: "
					+ Ogre::String(attributeValue.C_String()) + " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
			}
			changedWhat = 0;
			deserializeParameters->serializationBitstream[1].Read(changedWhat);
			if (AttributeWatcher::ATTRIBUTE_ADDED == changedWhat)
			{
				deserializeParameters->serializationBitstream[1].Read(attributeName);
				deserializeParameters->serializationBitstream[1].Read(attributeValue);

				AttributeData attributeData;
				// Type missing
				attributeData.attributeValue = attributeValue.C_String();

				this->attributes.insert(std::make_pair(attributeName.C_String(), attributeData));
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Deserialize attribute name for insert: " + Ogre::String(attributeName.C_String()) + " with value: "
					+ Ogre::String(attributeValue.C_String()) + " for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
			}
			changedWhat = 0;
			deserializeParameters->serializationBitstream[1].Read(changedWhat);
			if (AttributeWatcher::ATTRIBUTE_REMOVED == changedWhat)
			{
				deserializeParameters->serializationBitstream[1].Read(attributeName);
				deserializeParameters->serializationBitstream[1].Read(attributeValue);
				if (this->attributes.find(attributeName.C_String()) != this->attributes.end())
				{
					this->attributes.erase(attributeName.C_String());
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Deserialize attribute name for remove: " + Ogre::String(attributeName.C_String()) +
							" for client id: " + Ogre::StringConverter::toString(this->clientID) + " and object: " + this->gameObjectPtr->getName());
				}
			}
		}
	}

	void DistributedComponent::setStrSystemAddress(const Ogre::String& strSystemAddress)
	{
		this->strSystemAddress = strSystemAddress;
	}

	const Ogre::String& DistributedComponent::getStrSystemAddress(void) const
	{
		return this->strSystemAddress;
	}

	const RakNet::SystemAddress& DistributedComponent::getSystemAddress(void) const
	{
		return this->systemAddress;
	}

	void DistributedComponent::setClientID(int clientID)
	{
		this->clientID = clientID;
	}

	int DistributedComponent::getClientID(void) const
	{
		return this->clientID;
	}

	DistributedComponent::AttributeData DistributedComponent::getAttributeValue(const Ogre::String& attributeName)
	{
		auto it = this->attributes.find(attributeName);
		if (it != this->attributes.end())
		{
			return it->second;
		}
		DistributedComponent::AttributeData attributeData;
		return attributeData;
	}

	void DistributedComponent::addAttribute(const Ogre::String& attributeName, const Ogre::String& attributeValue, Variant::Types attributeType)
	{
		DistributedComponent::AttributeData attributeData;
		attributeData.attributeValue = attributeValue;
		attributeData.attributeType = attributeType;
		this->attributes.insert(std::make_pair(attributeName, attributeData));
		this->attributesQueue.Push(AttributeWatcher(AttributeWatcher::ATTRIBUTE_ADDED, attributeName, attributeValue, attributeType), _FILE_AND_LINE_);
	}

	void DistributedComponent::changeValue(const Ogre::String& attributeName, const Ogre::String& newAttributeValue, Variant::Types attributeType)
	{
		if (attributeName.empty())
		{
			return;
		}
		auto it = this->attributes.find(attributeName);
		if (it != this->attributes.end())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Setting new attribute value: " + newAttributeValue
				+ " for distributed game object: " + this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID));
// Attention: Attribute type missing
			it->second.attributeValue = newAttributeValue;
			this->attributesQueue.Push(AttributeWatcher(AttributeWatcher::VALUE_CHANGED, attributeName, newAttributeValue, attributeType), _FILE_AND_LINE_);
		}
	}

	void DistributedComponent::removeAttribute(const Ogre::String& attributeName)
	{
		auto it = this->attributes.find(attributeName);
		if (it != this->attributes.end())
		{
			this->attributesQueue.Push(AttributeWatcher(AttributeWatcher::ATTRIBUTE_REMOVED,  it->first, it->second.attributeValue, it->second.attributeType), _FILE_AND_LINE_);

			this->attributes.erase(it);
		}
	}

	void DistributedComponent::changeValue(const Ogre::String& attribute)
	{
		if (false == attribute.empty())
		{
// Attention: Attribute type missing

			size_t pos = attribute.find(":");
			if (Ogre::String::npos != pos)
			{
				Ogre::String name = attribute.substr(0, pos);
				Ogre::String value = attribute.substr(pos + 1, static_cast<unsigned int>(attribute.length()));
				auto it = this->attributes.find(name);
				if (it != this->attributes.cend())
				{
					it->second.attributeValue = value;
					// Type missing
					this->attributesQueue.Push(AttributeWatcher(AttributeWatcher::VALUE_CHANGED, name, value, Variant::VAR_NULL), _FILE_AND_LINE_);
				}
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DistributedComponent] Could not change value for attribute: " + attribute
						+ "because the bitstream is corrupt for distributed game object: " + this->gameObjectPtr->getName() + " and clientId: " + Ogre::StringConverter::toString(this->clientID));
			}
		}
	}

	std::map<Ogre::String, DistributedComponent::AttributeData> DistributedComponent::getAttributes(void)
	{
		return this->attributes;
	}

	void DistributedComponent::setAttributes(std::map<Ogre::String, DistributedComponent::AttributeData> attributes)
	{
		for (auto& it = attributes.cbegin(); it != attributes.cend(); ++it)
		{
			this->attributes.insert(std::make_pair(it->first, it->second));
		}
	}

}; // namespace end