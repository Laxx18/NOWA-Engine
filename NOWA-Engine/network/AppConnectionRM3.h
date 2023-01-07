#ifndef APP_CONNECTION_RM3_H_
#define APP_CONNECTION_RM3_H_

#include "gameobject/DistributedComponent.h"

namespace NOWA
{
	namespace NET
	{
		// One instance of Connection_RM2 is implicitly created per connection that uses ReplicaManager2. The most important function to implement is Construct() as this creates your game objects.
		// It is designed this way so you can override per-connection behavior in your own game classes

		// In this class the game objects of remote clients will be created (replicated)
		class EXPORTED AppConnectionRM3 : public RakNet::Connection_RM3
		{
		public:
			AppConnectionRM3(const RakNet::SystemAddress& systemAddress, RakNet::RakNetGUID guid)
				: RakNet::Connection_RM3(systemAddress, guid)
			{
			}

			virtual ~AppConnectionRM3()
			{
			}

			// Callback used to create objects
			// See Connection_RM2::Construct in ReplicaManager2.h for a full explanation of each parameter
			virtual RakNet::Replica3* AllocReplica(RakNet::BitStream* allocationId, RakNet::ReplicaManager3* replicaManager3)
			{
				char objectName[128];
				// decode string, look if the allocation id "GameObject" has been sent, to create (replicate) this game object 
				// This function is only called for the allocation id "GameObject", when a new game object should be spawned
				RakNet::StringTable::Instance()->DecodeString(objectName, 128, allocationId);

				Ogre::String gameObjectName = objectName;
				// if (strcmp(objectName, "GameObject") == 0)
				{
					// All remote game objects will created from other clients
					// return AppStateManager::getSingletonPtr()->getRakNetModule()->createRemoteGameObject(gameObjectName, replicaManager3);
					return DistributedComponent::replicateGameObject(gameObjectName, replicaManager3);
				}
				return nullptr;
			}
		};

		// Factory class to replicate game objects
		class AppReplicaManager : public RakNet::ReplicaManager3
		{
		public:
			virtual RakNet::Connection_RM3* AllocConnection(const RakNet::SystemAddress& systemAddress, RakNet::RakNetGUID rakNetGUID) const
			{
				return new AppConnectionRM3(systemAddress, rakNetGUID);
			}

			virtual void DeallocConnection(RakNet::Connection_RM3* connection) const
			{
				delete connection;
			}
		};

	} // namespace end NET

} // namespace end NOWA

#endif