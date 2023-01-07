#include "NOWAPrecompiled.h"
#include "GameObjectController.h"
#include "main/Core.h"
#include "main/Events.h"
#include "main/ProcessManager.h"
#include "main/ScriptEventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsTerrainComponent.h"
#include "PhysicsArtifactComponent.h"
#include "PhysicsRagDollComponent.h"
#include "PhysicsActiveCompoundComponent.h"
#include "PhysicsActiveDestructableComponent.h"
#include "JointComponents.h"
#include "PlayerControllerComponents.h"
#include "PhysicsCompoundConnectionComponent.h"
#include "CameraBehaviorComponents.h"
#include "LuaScriptComponent.h"
#include "main/AppStateManager.h"

#include <algorithm>    // std::set_difference, std::sort

namespace NOWA
{
	DeleteGameObjectsUndoCommand::DeleteGameObjectsUndoCommand(const Ogre::String& appStateName, std::vector<unsigned long> gameObjectIds)
		: appStateName(appStateName),
		gameObjectIds(gameObjectIds),
		dotSceneExportModule(new DotSceneExportModule())
	{
		GameObjectPtr gameObjectPtr = nullptr;
		if (false == this->appStateName.empty())
			gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(this->gameObjectIds[0]);
		else
			gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectIds[0]);
		
		if (nullptr != gameObjectPtr)
		{
			this->dotSceneImportModule = new DotSceneImportModule(gameObjectPtr->getSceneManager());
			this->deleteGameObjects();
		}
	}

	DeleteGameObjectsUndoCommand::~DeleteGameObjectsUndoCommand()
	{
		if (nullptr != this->dotSceneExportModule)
		{
			delete this->dotSceneExportModule;
			this->dotSceneExportModule = nullptr;
		}
		if (nullptr != this->dotSceneImportModule)
		{
			delete this->dotSceneImportModule;
			this->dotSceneImportModule = nullptr;
		}
	}

	void DeleteGameObjectsUndoCommand::undo(void)
	{
		this->createGameObjects();
	}

	void DeleteGameObjectsUndoCommand::redo(void)
	{
		this->deleteGameObjects();
	}

	std::vector<unsigned long> DeleteGameObjectsUndoCommand::getIds(void) const
	{
		return this->gameObjectIds;
	}

	void DeleteGameObjectsUndoCommand::deleteGameObjects(void)
	{
		// Joints must be all disconnected, because another one may point to this joint as predecessor joint and else the component and the game object could not be deleted
		// AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->disconnectGameObjects();
			
		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* nodesXML = doc.allocate_node(rapidxml::node_element, "nodes");
		for (size_t i = 0; i < this->gameObjectIds.size(); i++)
		{
			GameObjectPtr gameObjectPtr = nullptr;
			if (false == this->appStateName.empty())
				gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(this->gameObjectIds[i]);
			else
				gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectIds[i]);

			if (nullptr != gameObjectPtr)
			{
				// Write all the game object data to stream
				this->dotSceneExportModule->exportNode(gameObjectPtr->getSceneNode(), nodesXML, doc, false, "");
				if (false == this->appStateName.empty())
					AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->deleteGameObject(gameObjectPtr->getId());
				else
					AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObjectPtr->getId());
			}
		}
		doc.append_node(nodesXML);

		std::ostrstream out;
		out << doc;
		out << '\0';
		// Set the stream
		this->gameObjectsToAddStream = out.str();
	}

	void DeleteGameObjectsUndoCommand::createGameObjects(void)
	{
		rapidxml::xml_document<> XMLDoc;
		rapidxml::xml_node<>* xmlNodes;

		std::vector<char> sceneCopy(this->gameObjectsToAddStream.begin(), this->gameObjectsToAddStream.end());
		sceneCopy.emplace_back('\0');
		try
		{
			XMLDoc.parse<0>(&sceneCopy[0]);
		}
		catch (rapidxml::parse_error& error)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[EditorManager] Could not parse node: " + Ogre::String(error.what())
				+ " at: " + Ogre::String(error.where<char>()));
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[EditorManager] Could not parse node: " + Ogre::String(error.what())
				+ " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
		}

		xmlNodes = XMLDoc.first_node("nodes");
		this->dotSceneImportModule->processNodes(xmlNodes);

		// Post init new game objects when created
		for (size_t i = 0; i < this->gameObjectIds.size(); i++)
		{
			GameObjectPtr gameObjectPtr = nullptr;
			if (false == this->appStateName.empty())
				gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(this->gameObjectIds[i]);
			else
				gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectIds[i]);

			if (nullptr != gameObjectPtr)
			{
				gameObjectPtr->postInit();
			}
		}
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SnapshotGameObjectsCommand::SnapshotGameObjectsCommand(const Ogre::String& appStateName)
		: appStateName(appStateName),
		dotSceneExportModule(new DotSceneExportModule())
	{
		GameObjectPtr gameObjectPtr = nullptr;

		if (false == this->appStateName.empty())
			gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(GameObjectController::MAIN_GAMEOBJECT_ID);
		else
			gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(GameObjectController::MAIN_GAMEOBJECT_ID);

		if (nullptr != gameObjectPtr)
		{
			this->dotSceneImportModule = new DotSceneImportModule(gameObjectPtr->getSceneManager());
			this->snapshotGameObjects();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SnapshotGameObjectsCommand] Could not snapshot because there is no main game object!");
		}
	}

	SnapshotGameObjectsCommand::~SnapshotGameObjectsCommand()
	{
		if (nullptr != this->dotSceneExportModule)
		{
			delete this->dotSceneExportModule;
			this->dotSceneExportModule = nullptr;
		}
		if (nullptr != this->dotSceneImportModule)
		{
			delete this->dotSceneImportModule;
			this->dotSceneImportModule = nullptr;
		}
	}

	void SnapshotGameObjectsCommand::undo(void)
	{
		this->resetGameObjects();
	}

	void SnapshotGameObjectsCommand::redo(void)
	{
		this->snapshotGameObjects();
	}

	void SnapshotGameObjectsCommand::snapshotGameObjects(void)
	{
		// Creates a snapshot of the game objects and its components
		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* nodesXML = doc.allocate_node(rapidxml::node_element, "nodes");

		if (false == this->appStateName.empty())
			this->gameObjectsIdsBeforeSnapShot = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getAllGameObjectIds();
		else
			this->gameObjectsIdsBeforeSnapShot = AppStateManager::getSingletonPtr()->getGameObjectController()->getAllGameObjectIds();

		for (size_t i = 0; i < this->gameObjectsIdsBeforeSnapShot.size(); i++)
		{
			GameObjectPtr gameObjectPtr = nullptr;
			if (false == this->appStateName.empty())
				gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(this->gameObjectsIdsBeforeSnapShot[i]);
			else
				gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectsIdsBeforeSnapShot[i]);

			if (nullptr != gameObjectPtr)
			{
				// Write all the game object data to stream
				this->dotSceneExportModule->exportNode(gameObjectPtr->getSceneNode(), nodesXML, doc, false, "");
			}
		}
		doc.append_node(nodesXML);

		std::ostrstream out;
		out << doc;
		out << '\0';
		// Set the stream
		this->gameObjectsToAddStream = out.str();
	}

	void SnapshotGameObjectsCommand::resetGameObjects(void)
	{
		rapidxml::xml_document<> XMLDoc;
		rapidxml::xml_node<>* xmlNodes;

		// Determine if there are game object which do not exist anymore. These ones must be loaded completely
		std::vector<unsigned long> currentGameObjectsIds;

		if (false == this->appStateName.empty())
			currentGameObjectsIds = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getAllGameObjectIds();
		else
			currentGameObjectsIds = AppStateManager::getSingletonPtr()->getGameObjectController()->getAllGameObjectIds();

		std::vector<unsigned long> differenceList;
		std::set_difference(this->gameObjectsIdsBeforeSnapShot.begin(), this->gameObjectsIdsBeforeSnapShot.end(), currentGameObjectsIds.begin(), currentGameObjectsIds.end(),
			std::inserter(differenceList, differenceList.begin()));


		std::vector<char> sceneCopy(this->gameObjectsToAddStream.begin(), this->gameObjectsToAddStream.end());
		sceneCopy.emplace_back('\0');
		try
		{
			XMLDoc.parse<0>(&sceneCopy[0]);
		}
		catch (rapidxml::parse_error& error)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[EditorManager] Could not parse node: " + Ogre::String(error.what())
				+ " at: " + Ogre::String(error.where<char>()));
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[EditorManager] Could not parse node: " + Ogre::String(error.what())
				+ " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
		}

		xmlNodes = XMLDoc.first_node("nodes");

		this->dotSceneImportModule->setMissingGameObjectIds(differenceList);
		this->dotSceneImportModule->processNodes(xmlNodes, nullptr, true);
		differenceList.clear();
		this->dotSceneImportModule->setMissingGameObjectIds(differenceList);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	class DeleteDelayedProcess : public NOWA::Process
	{
	public:
		explicit DeleteDelayedProcess(const Ogre::String& appStateName, const unsigned long id)
			: appStateName(appStateName),
			id(id)
		{

		}
	protected:
		virtual void onInit(void) override
		{
			this->succeed();
			AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->deleteGameObjectImmediately(this->id);
		}

		virtual void onUpdate(float dt) override
		{
			this->succeed();
		}
	private:
		Ogre::String appStateName;
		unsigned long id;
	};
	
	////////////////////////////////////////////////////////////////////////////////////////////
	
	GameObjectController::GameObjectController(const Ogre::String& appStateName)
		: appStateName(appStateName),
		gameObjects(new GameObjects),
		shiftIndex(0), // 0 - 4 are reserved for gizmo, see defines.h
		triggerUpdateTimer(0.0f),
		sphereSceneQuery(nullptr),
		currentSceneManager(nullptr),
		sphereQueryUpdateFrequency(0.5f),
		alreadyDestroyed(false),
		isSimulating(false),
		deleteGameObjectsUndoCommand(nullptr),
		nextGameObjectIndex(0)
	{
		
	}

	GameObjectController::~GameObjectController()
	{
		assert((this->gameObjects != nullptr) && "[GameObjectController::~GameObjectController: Gameobjects map is already null");
		this->triggeredGameObjects.clear();
		delete this->gameObjects;
	}
	
	GameObjectPtr GameObjectController::clone(const Ogre::String& originalGameObjectName, Ogre::SceneNode* parentNode, unsigned long targetId, const Ogre::Vector3& targetPosition,
		const Ogre::Quaternion& targetOrientation, const Ogre::Vector3& targetScale)
	{
		GameObjectPtr originalGameObjectPtr = this->getGameObjectFromName(originalGameObjectName);
		if (nullptr == originalGameObjectPtr)
		{
			return GameObjectPtr();
		}
		return this->internalClone(originalGameObjectPtr, parentNode, targetId, targetPosition, targetOrientation, targetScale);
	}

	GameObjectPtr GameObjectController::clone(unsigned long originalGameObjectId, Ogre::SceneNode* parentNode, unsigned long targetId, const Ogre::Vector3& targetPosition,
		const Ogre::Quaternion& targetOrientation, const Ogre::Vector3& targetScale)
	{
		GameObjectPtr originalGameObjectPtr = this->getGameObjectFromId(originalGameObjectId);
		if (nullptr == originalGameObjectPtr)
		{
			return GameObjectPtr();
		}
		return this->internalClone(originalGameObjectPtr, parentNode, targetId, targetPosition, targetOrientation, targetScale);
	}

	GameObjectPtr GameObjectController::internalClone(GameObjectPtr originalGameObjectPtr, Ogre::SceneNode* parentNode, unsigned long targetId, const Ogre::Vector3& targetPosition,
		const Ogre::Quaternion& targetOrientation, const Ogre::Vector3& targetScale)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectController]: Cloning game object from the original name: " + originalGameObjectPtr->getName());

		Ogre::SceneManager* sceneManager = originalGameObjectPtr->getSceneManager();
		Ogre::SceneNode* originalSceneNode = originalGameObjectPtr->getSceneNode();
		
		// Get name without .mesh
		Ogre::MovableObject* originalMovableObject = originalGameObjectPtr->getMovableObject();
		// Do not use # anymore, because its reserved in mygui as code-word the # and everything after that will be removed!
		Ogre::String validatedName = originalGameObjectPtr->getMovableObject()->getName()/* + "_cloned_0"*/; // problem here: cloned_0 will be appended each time!
		this->getValidatedGameObjectName(validatedName);

		Ogre::SceneNode* clonedSceneNode = nullptr;

		// Note: Attributes have correct transform, scene node maybe not
		Ogre::Vector3 position = originalGameObjectPtr->position->getVector3();
		Ogre::Quaternion orientation = MathHelper::getInstance()->degreesToQuat(originalGameObjectPtr->orientation->getVector3());
		Ogre::Vector3 scale = originalGameObjectPtr->scale->getVector3();

		if (Ogre::Vector3::ZERO != targetPosition)
			position = targetPosition;

		if (Ogre::Quaternion::IDENTITY != targetOrientation)
			orientation = targetOrientation;

		if (Ogre::Vector3::UNIT_SCALE != targetScale)
			scale = targetScale;

		if (nullptr != parentNode)
		{
			clonedSceneNode = parentNode->createChildSceneNode(originalGameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC,
				position, orientation);
			clonedSceneNode->setName(validatedName);
			clonedSceneNode->setScale(scale);
		}
		else
		{
			clonedSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode(originalGameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC,
				position, orientation);
			clonedSceneNode->setName(validatedName);
			clonedSceneNode->setScale(scale);
		}

		Ogre::MovableObject* clonedMovableObject = nullptr;
			
		if (GameObject::ENTITY == originalGameObjectPtr->getType())
		{
			clonedMovableObject = sceneManager->createEntity(static_cast<Ogre::v1::Entity*>(originalMovableObject)->getMesh(), originalSceneNode->isStatic() ? Ogre::SCENE_STATIC : Ogre::SCENE_DYNAMIC);
		}
		else if (GameObject::ITEM == originalGameObjectPtr->getType())
		{
			clonedMovableObject = sceneManager->createItem(static_cast<Ogre::Item*>(originalMovableObject)->getMesh(), originalSceneNode->isStatic() ? Ogre::SCENE_STATIC : Ogre::SCENE_DYNAMIC);
		}
		clonedMovableObject->setName(validatedName);
		clonedSceneNode->attachObject(clonedMovableObject);
		if (nullptr != clonedMovableObject)
		{
			if (GameObject::ENTITY == originalGameObjectPtr->getType())
			{
				// also clone each sub material, so that each cloned entity has its own material which can be manipulated, whithout affecting the other entities
				for (unsigned int i = 0; i < static_cast<Ogre::v1::Entity*>(originalMovableObject)->getNumSubEntities(); i++)
				{
					static_cast<Ogre::v1::Entity*>(clonedMovableObject)->getSubEntity(i)->setDatablock(static_cast<Ogre::v1::Entity*>(originalMovableObject)->getSubEntity(i)->getDatablock());
				}
			}
			else if (GameObject::ITEM == originalGameObjectPtr->getType())
			{
				// also clone each sub material, so that each cloned entity has its own material which can be manipulated, whithout affecting the other entities
				for (unsigned int i = 0; i < static_cast<Ogre::Item*>(originalMovableObject)->getNumSubItems(); i++)
				{
					static_cast<Ogre::Item*>(originalMovableObject)->getSubItem(i)->setDatablock(static_cast<Ogre::Item*>(originalMovableObject)->getSubItem(i)->getDatablock());
				}
			}
		}

		clonedSceneNode->setVisible(originalMovableObject->getVisible());
		clonedMovableObject->setVisible(originalMovableObject->getVisible());
		clonedMovableObject->setCastShadows(originalMovableObject->getCastShadows());
		clonedMovableObject->setQueryFlags(originalMovableObject->getQueryFlags());

		// attention with: no ref by category, since each attribute, that is no reference must use boost::ref
		GameObjectPtr clonedGameObjectPtr(boost::make_shared<GameObject>(sceneManager, clonedSceneNode, clonedMovableObject,
			originalGameObjectPtr->getCategory(), originalGameObjectPtr->isDynamic(), originalGameObjectPtr->getType(), targetId));

		// Store during cloning the prior id, which this game object had, in order to be able to retrieve connection of game objects when cloned
		clonedGameObjectPtr->priorId = originalGameObjectPtr->getId();

		clonedGameObjectPtr->setTagName(originalGameObjectPtr->getTagName());
		clonedGameObjectPtr->setControlledByClientID(originalGameObjectPtr->getControlledByClientID());
		clonedGameObjectPtr->setUseReflection(originalGameObjectPtr->getUseReflection());
		clonedGameObjectPtr->setDefaultDirection(originalGameObjectPtr->getDefaultDirection());
		clonedGameObjectPtr->setGlobal(originalGameObjectPtr->getGlobal());
		clonedGameObjectPtr->setClampY(originalGameObjectPtr->getClampY());

		if (!clonedGameObjectPtr->init())
		{
			return GameObjectPtr();
		}

		if (Ogre::Vector3::ZERO != targetPosition)
		{
			clonedGameObjectPtr->getSceneNode()->setPosition(targetPosition);
		}
		if (Ogre::Quaternion::IDENTITY != targetOrientation)
		{
			clonedGameObjectPtr->getSceneNode()->setOrientation(targetOrientation);
		}
		if (Ogre::Vector3(1.0f, 1.0f, 1.0f) != targetScale)
		{
			clonedGameObjectPtr->getSceneNode()->setScale(targetScale);
		}

		// Order is important since category id is generated and maybe required in postInit!
		this->registerGameObject(clonedGameObjectPtr);

		// for (auto GameObjectComponentPt)
		GameObjectComponents* gameobjectComponents = originalGameObjectPtr->getComponents();

		for (auto& it = gameobjectComponents->cbegin(); it != gameobjectComponents->cend(); ++it)
		{
			std::get<COMPONENT>(*it)->clone(clonedGameObjectPtr);
		}

		// Now that the gameobject has been fully created, run the post init phase
		if (!clonedGameObjectPtr->postInit())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController]: Could not post init cloned game object: " + clonedGameObjectPtr->getSceneNode()->getName());
			return GameObjectPtr();
		}

		// here no connect disconnect, because else the original game object may be found for predecessor id etc. which would be a mess

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectController]: Returning cloned game object: " + clonedGameObjectPtr->getSceneNode()->getName());
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectController]: with components:");

		for (auto& it = clonedGameObjectPtr->getComponents()->cbegin(); it != clonedGameObjectPtr->getComponents()->cend(); ++it)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectController]: Name: " + std::get<COMPONENT>(*it)->getClassName());
		}


		boost::shared_ptr<EventDataNewGameObject> newGameObjectEvent(boost::make_shared<EventDataNewGameObject>(clonedGameObjectPtr->getId()));
		AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(newGameObjectEvent);

		return clonedGameObjectPtr;
	}

	void GameObjectController::update(Ogre::Real dt, bool notSimulating)
	{
		// updates the active GameObjects
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			it->second->update(dt, notSimulating);
		}
		if (false == notSimulating)
		{
			// Update moving behaviors
			for (auto& it = this->movingBehaviors.cbegin(); it != this->movingBehaviors.cend(); ++it)
			{
				it->second->update(dt);
			}
			// Update moving behaviors 2D
			/*for (auto& it = this->movingBehaviors2D.cbegin(); it != this->movingBehaviors2D.cend(); ++it)
			{
				it->second->update(dt);
			}*/
		}
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			it->second->lateUpdate(dt, notSimulating);
		}

		// Looks if there are some GameObjects to delete post mortem and deletes them
		// this has been solved this way because a distributed component cannot delete its whole game object within its class
		// therefore it must not be deleted immediately but after the update process is done (post mortem)
		if (false == this->delayedDeleterList.empty())
		{
			for (auto& it = this->delayedDeleterList.cbegin(); it != this->delayedDeleterList.cend();)
			{
				GameObjectPtr gameObjectPtr = this->getGameObjectFromId(*it);

				if (nullptr != gameObjectPtr)
				{
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectController] Deleting delayed gameobject: " + gameObjectPtr->getName());
					this->deleteGameObjectImmediately(gameObjectPtr);
					it = this->delayedDeleterList.erase(it);
				}
				else
				{
					++it;
				}
			}
			this->delayedDeleterList.clear();
		}
	}

	void GameObjectController::lateUpdate(Ogre::Real dt, bool notSimulating)
	{
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			it->second->lateUpdate(dt, notSimulating);
		}
	}

	GameObjectPtr GameObjectController::getGameObjectFromId(const unsigned long id) const
	{
		//find the GameObject
		GameObjects::const_iterator& it = this->gameObjects->find(id);
		// if the object cannot be found beyond the active object, search for it in the passive objects
		if (it != this->gameObjects->end())
		{
			return it->second;
		}

		return GameObjectPtr();
	}

	std::vector<unsigned long> GameObjectController::getAllGameObjectIds(void)
	{
		std::vector<unsigned long> gameObjectIds(this->gameObjects->size());
		int i = 0;
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			gameObjectIds[i++] = it->first;
		}

		return gameObjectIds;
	}

	GameObjectPtr GameObjectController::getClonedGameObjectFromPriorId(const unsigned long priorId) const
	{
		// Search by prior id, maybe this game object was another one in a previous life before he has been cloned so get it!
		for (auto& it2 = this->gameObjects->cbegin(); it2 != this->gameObjects->cend(); ++it2)
		{
			// Note: This function may take longer to find the object, but it is rarely called, since in most cases the GO will be found, only when cloned, this will be necessary
			if (it2->second->priorId == priorId)
			{
				// blur the trace -> no because, else its not possible to e.g. when having waypoints copied, in which a waypoint is used twice to close a circle, to get the prior object
				// it2->second->priorId = 0;
				// Proceed a search for the game object, that had this prior id, but has now a new one and get it!
				return getGameObjectFromId(it2->second->getId());
			}
		}
		return GameObjectPtr();
	}

	void GameObjectController::registerGameObject(GameObjectPtr gameObjectPtr)
	{
		// Destroy content can be called again
		this->alreadyDestroyed = false;
		this->gameObjects->emplace(gameObjectPtr->getId(), gameObjectPtr);
		this->registerType(gameObjectPtr, gameObjectPtr->category->getListSelectedValue());
	}

	const OgreNewt::MaterialID* GameObjectController::getMaterialID(GameObject* gameObject, OgreNewt::World* ogreNewt)
	{
		// if its the default category get the world default category, which can be used for everything
		/*if ("Default" == gameObjectPtr->getCategory())
		{
			return ogreNewt->getDefaultMaterialID();
		}*/
		std::map<Ogre::String, OgreNewt::MaterialID*>::const_iterator& it = this->materialIDMap.find(gameObject->getCategory());
		OgreNewt::MaterialID* pMaterialID;
		// if the type is not in the map, then its a new type, add a new material ID for collision
		if (it == this->materialIDMap.end())
		{
			pMaterialID = new OgreNewt::MaterialID(ogreNewt);
			this->materialIDMap.insert(std::make_pair(gameObject->getCategory(), pMaterialID));
		}
		else
		{
			pMaterialID = this->materialIDMap.find(gameObject->getCategory())->second;
		}
		return pMaterialID;
	}

	const OgreNewt::MaterialID* GameObjectController::getMaterialIDFromCategory(const Ogre::String& category, OgreNewt::World* ogreNewt)
	{
		// if its the default category get the world default category, which can be used for everything
	/*	if ("Default" == category)
		{
			return ogreNewt->getDefaultMaterialID();
		}*/
		std::map<Ogre::String, OgreNewt::MaterialID*>::const_iterator& it = this->materialIDMap.find(category);
		// if there is the corresponding category, deliver it, else deliver nullptr
		if (it != this->materialIDMap.end())
		{
			return it->second;
		}
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController] A material for the category id: " + category + " does not exist!");
		return nullptr;
	}

	void GameObjectController::destroyContent(std::vector<Ogre::String>& excludeGameObjectNames)
	{
		if (false == this->alreadyDestroyed)
		{
			auto& it = this->materialIDMap.begin();
			while (it != this->materialIDMap.end())
			{
				delete it->second;
				it->second = nullptr;
				++it;
			}
			this->materialIDMap.erase(this->materialIDMap.begin(), this->materialIDMap.end());
			this->materialIDMap.clear();

			AppStateManager::getSingletonPtr()->getScriptEventManager()->destroyContent();

			// do not delete with iterator since the iterator changes then during the loop
			for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
			{
				it->second->setActivated(false);
				it->second->disconnect();

				bool canDestroy = true;
				// Do not destroy game object, that are excluded from destruction (which is seldom the case and optional)
				for (auto it2 = excludeGameObjectNames.begin(); it2 != excludeGameObjectNames.end();)
				{
					if (it->second->getName() == *it2)
					{
						canDestroy = false;
						it2 = excludeGameObjectNames.erase(it2);
						break;
					}
					else
					{
						++it2;
					}
				}
				
				if (true == canDestroy)
				{
					// signal the event, so that other game objects have the chance to react if necessary
					boost::shared_ptr<EventDataDeleteGameObject> deleteGameObjectEvent(boost::make_shared<EventDataDeleteGameObject>(it->second->getId()));
					AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(deleteGameObjectEvent);
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectController] Deleting gameobject: " + it->second->getName());
					assert((it->second != nullptr) && "[GameObjectController::deleteAllGameObjects] Gameobject not found");
					it->second->destroy();
				}
			}

			// Joints must be destroyed here, because a joint depends on its 2 bodies, which must live at this point, so this must be called, before the destructors of all components are called!
			// this->disconnectJoints();
			this->jointComponentMap.clear();
			this->playerControllerComponentMap.clear();
			this->physicsCompoundConnectionComponentMap.clear();
			this->commandModule.clear(); // Must be cleared, because shared ptr's are used and GOC lives to long, because its a singleton

			this->vehicleCollisionMap.clear();
			this->tempVehicleObjectMap.clear();
			this->vehicleChildTempMap.clear();
			this->delayedDeleterList.clear();
			this->triggeredGameObjects.clear();
			this->movingBehaviors.clear();
			// this->movingBehaviors2D.clear();
			this->shiftIndex = 0;
			this->triggerUpdateTimer = 0.0f;
			this->sphereQueryUpdateFrequency = 0.5f;
			this->isSimulating = false;

			// delete the sphere scene query
			if (this->currentSceneManager)
			{
				if (this->sphereSceneQuery)
				{
					this->currentSceneManager->destroyQuery(this->sphereSceneQuery);
					this->sphereSceneQuery = nullptr;
				}
				this->currentSceneManager = nullptr;

				this->detachAndDestroyAllTriggerObserver();
			}

			// this->pActiveGameObjects->erase(this->pActiveGameObjects->begin(), this->pActiveGameObjects->end());
			this->gameObjects->clear();
			this->typeDBMap.clear();
			this->shiftIndex = 0;
			this->triggeredGameObjects.clear();


			// Attention since GameObjectController is a singleton and the lifecycle is beyond the AppState's lifecycle
			// That means, if an AppState is exited and started and GameObjects are created

			this->alreadyDestroyed = true;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Cannot destroy content, as it is already destroyed!");
		}
	}
	
	void GameObjectController::undo(void)
	{
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]() { this->commandModule.undo(); };
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}
		
	void GameObjectController::undoAll(void)
	{
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]() { this->commandModule.undoAll(); };
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}
	
	void GameObjectController::redo(void)
	{
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]() { this->commandModule.redo(); };
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}
	
	void GameObjectController::redoAll(void)
	{	
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]() { this->commandModule.redoAll(); };
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	bool GameObjectController::getIsSimulating(void) const
	{
		return this->isSimulating;
	}

	void GameObjectController::deleteGameObjectImmediately(GameObjectPtr gameObjectPtr)
	{
		// Frees the category, internally a check will be made if there is still a game object with the category, if not it will be un-occupied, for a next new category
		this->freeCategoryFromGameObject(gameObjectPtr->getCategory());
		unsigned long id = gameObjectPtr->getId();
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectController] Deleting gameobject: " + gameObjectPtr->getName());
		boost::shared_ptr<EventDataDeleteGameObject> deleteGameObjectEvent(boost::make_shared<EventDataDeleteGameObject>(gameObjectPtr->getId()));
		AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(deleteGameObjectEvent);
		// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[GameObjectController] delete: " + Ogre::StringConverter::toString(gameObjectPtr->getId()));
		this->gameObjects->erase(gameObjectPtr->getId());
		this->triggeredGameObjects.erase(gameObjectPtr->getId());
		this->movingBehaviors.erase(gameObjectPtr->getId());
		// this->movingBehaviors2D.erase(gameObjectPtr->getId());

		gameObjectPtr->destroy();
	}

	void GameObjectController::deleteGameObjectImmediately(unsigned long id)
	{
		GameObjects::const_iterator& it = this->gameObjects->find(id);
		if (it != this->gameObjects->end())
		{
			this->deleteGameObjectImmediately(it->second);
		}
	}

	void GameObjectController::deleteGameObject(const unsigned long id)
	{
		boost::shared_ptr<EventDataDeleteGameObject> deleteGameObjectEvent(boost::make_shared<EventDataDeleteGameObject>(id));
		AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(deleteGameObjectEvent);
		// Do net delete immediately but push it to another list to delete later after the update process is done
		/*NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		delayProcess->attachChild(NOWA::ProcessPtr(new DeleteDelayedProcess(this->appStateName, id)));
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);*/

		this->delayedDeleterList.emplace(id);
	}

	void GameObjectController::deleteGameObjectWithUndo(const unsigned long id)
	{
		std::vector<unsigned long> gameObjectIds = { id };
		
		// Do net delete immediately but push it to another list to delete later after the update process is done
		// boost::shared_ptr<EventDataDeleteGameObject> deleteGameObjectEvent(boost::make_shared<EventDataDeleteGameObject>(id));
		// AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(deleteGameObjectEvent);

		if (nullptr == this->deleteGameObjectsUndoCommand)
		{
			this->deleteGameObjectsUndoCommand = std::make_shared<DeleteGameObjectsUndoCommand>(this->appStateName, gameObjectIds);
			this->commandModule.pushCommand(this->deleteGameObjectsUndoCommand);
		}
		else
		{
			// Only push, if its not the same id, this could happen if, e.g. using a contact solver a game object is hit x-times, but deletion is done with delay,
			// so it could be, that the same game object will be pushed x-times and also deleted x-times, so undo would not be possible anymore
			if (id != this->deleteGameObjectsUndoCommand->getIds().front())
			{
				this->deleteGameObjectsUndoCommand.reset();
				this->deleteGameObjectsUndoCommand = std::make_shared<DeleteGameObjectsUndoCommand>(this->appStateName, gameObjectIds);
				this->commandModule.pushCommand(this->deleteGameObjectsUndoCommand);
			}
		}
	}

	void GameObjectController::snapshotGameObjects()
	{
		this->commandModule.pushCommand(std::make_shared<SnapshotGameObjectsCommand>(this->appStateName));

		// Procedure as follows: 
		// Before all game objects gets connected (simulation start)
		// Snapshot is created of all game objects: saving current attribute values for all game objects
		// If all game objects gets disconnected (simulation ended)
		// Undo is called, so that all game objects are processed and their attributes just loaded (init function) not set!
		// Internally each attributes stores in its changed flag. Tthat is, if during the simulation any attribute has changed since last snapshot.
		// For all game object's and components attributes which have the changed flag set, actualizeValue is called with the value from the snapshot (resetting everything)
		// If some game objects have gone missing (deleted during simulation, e.g. enemy killed), they will be restored fully again.
		// The changed flags will be reset (set to false)
	}
	
	void GameObjectController::deleteGameObject(GameObjectPtr gameObjectPtr)
	{
		// Do net delete immediately but push it to another list to delete later after the update process is done
		/*NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.2f));
		delayProcess->attachChild(NOWA::ProcessPtr(new DeleteDelayedProcess(this->appStateName, gameObjectPtr->getId())));
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);*/
		this->freeCategoryFromGameObject(gameObjectPtr->getCategory());
		this->deleteGameObject(gameObjectPtr->getId());
	}

	void GameObjectController::deleteDelayedGameObject(const unsigned long id, Ogre::Real timeOffsetSec)
	{
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(timeOffsetSec));
		delayProcess->attachChild(NOWA::ProcessPtr(new DeleteDelayedProcess(this->appStateName, id)));
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	void GameObjectController::addJointComponent(boost::weak_ptr<JointComponent> jointCompWeakPtr)
	{
		JointCompPtr jointCompPtr = NOWA::makeStrongPtr(jointCompWeakPtr);
		if (nullptr != jointCompPtr)
		{
			if (0 == jointCompPtr->getId())
			{
				return;
			}
			auto& existingJointCompPtr = this->jointComponentMap.find(jointCompPtr->getId());
			if (existingJointCompPtr == this->jointComponentMap.cend())
			{
				this->jointComponentMap.insert(std::make_pair(jointCompPtr->getId(), jointCompPtr));
			}
		}
	}

	void GameObjectController::removeJointComponent(const unsigned long id)
	{
		if (0 == id)
		{
			return;
		}
		auto& it = this->jointComponentMap.find(id);
		if (it != this->jointComponentMap.end())
		{
			this->jointComponentMap.erase(it);
		}
	}

	void GameObjectController::removeJointComponentBreakJointChain(const unsigned long jointId)
	{
		if (0 == jointId)
		{
			return;
		}
		auto& it = this->jointComponentMap.find(jointId);
		if (it != this->jointComponentMap.end())
		{
			auto& predecessorJointCompPtr = NOWA::makeStrongPtr(this->getJointComponent(it->second->getPredecessorId()));
			if (nullptr != predecessorJointCompPtr)
			{
				// Reset also predecessor/target joint ptr, else the GameObject cannot be deleted
				predecessorJointCompPtr->releaseJoint(true);
				this->internalRemoveJointComponentBreakJointChain(predecessorJointCompPtr->getPredecessorId());
				this->internalRemoveJointComponentBreakJointChain(predecessorJointCompPtr->getTargetId());
			}
			else
			{
				// Search for the joint component, that points to the removed one
				for (auto& it2 = this->jointComponentMap.cbegin(); it2 != this->jointComponentMap.cend(); ++it2)
				{
					if (it2->second->getPredecessorId() == jointId || it2->second->getTargetId() == jointId)
					{
						auto& jointCompPtr = NOWA::makeStrongPtr(this->getJointComponent(it2->second->getId()));
						if (nullptr != jointCompPtr)
						{
							// Reset also predecessor/target joint ptr, else the GameObject cannot be deleted
							jointCompPtr->releaseJoint(true);
							this->internalRemoveJointComponentBreakJointChain(jointCompPtr->getPredecessorId());
							this->internalRemoveJointComponentBreakJointChain(jointCompPtr->getTargetId());
						}
					}
				}
			}
			this->jointComponentMap.erase(it);
		}
	}

	void GameObjectController::internalRemoveJointComponentBreakJointChain(const unsigned long jointId)
	{
		if (0 == jointId)
		{
			return;
		}
		auto& it = this->jointComponentMap.find(jointId);
		if (it != this->jointComponentMap.end())
		{
			auto& predecessorJointCompPtr = NOWA::makeStrongPtr(this->getJointComponent(it->second->getPredecessorId()));
			if (nullptr != predecessorJointCompPtr)
			{
				predecessorJointCompPtr->releaseJoint();
				this->internalRemoveJointComponentBreakJointChain(predecessorJointCompPtr->getPredecessorId());
				this->internalRemoveJointComponentBreakJointChain(predecessorJointCompPtr->getTargetId());
			}
			else
			{
				// Search for the joint component, that points to the removed one
				for (auto& it2 = this->jointComponentMap.cbegin(); it2 != this->jointComponentMap.cend(); ++it2)
				{
					if (it->second->getPredecessorId() == jointId)
					{
						auto& predecessorJointCompPtr = NOWA::makeStrongPtr(this->getJointComponent(it->second->getPredecessorId()));
						predecessorJointCompPtr->releaseJoint();
						this->internalRemoveJointComponentBreakJointChain(predecessorJointCompPtr->getPredecessorId());
						this->internalRemoveJointComponentBreakJointChain(predecessorJointCompPtr->getTargetId());
					}
				}
			}
		}
	}

	boost::weak_ptr<JointComponent> GameObjectController::getJointComponent(const unsigned long id) const
	{
		if (0 == id)
		{
			return boost::weak_ptr<JointComponent>();
		}
		// First try: search by id
		auto& it = this->jointComponentMap.find(id);
		if (it != this->jointComponentMap.end())
		{
			// assert(id != it->second->getId() && "Id and predecessorJointCompPtr are the same!");
			return it->second;
		}
		// Nothing found, next try: search by prior id, maybe this joint was another one in a previous life before he has been cloned so get him!
		for (auto& it = this->jointComponentMap.cbegin(); it != this->jointComponentMap.cend(); ++it)
		{
			if (it->second->getPriorId() == id)
			{
				// blur the trace --> No do not blur the trace, else other game objects with joints, that are referring to this id, would not able to set this joint as predecessor!
				// For example a catapult with 4 wheels, where each wheel does reference the catapult!
				// it->second->internalSetPriorId(0);

				// assert(id != it->second->getId() && "Id and predecessorJointCompPtr are the same!");
				return it->second;
			}
		}

		return boost::weak_ptr<JointComponent>();
	}

	void GameObjectController::addPlayerController(boost::weak_ptr<PlayerControllerComponent> playerControllerCompWeakPtr)
	{
		PlayerControllerCompPtr playerControllerCompPtr = NOWA::makeStrongPtr(playerControllerCompWeakPtr);
		if (nullptr != playerControllerCompPtr)
		{
			if (0 == playerControllerCompPtr->getOwner()->getId())
			{
				return;
			}
			auto& existingPlayerControllerCompPtr = this->playerControllerComponentMap.find(playerControllerCompPtr->getOwner()->getId());
			if (existingPlayerControllerCompPtr == this->playerControllerComponentMap.cend())
			{
				this->playerControllerComponentMap.insert(std::make_pair(playerControllerCompPtr->getOwner()->getId(), playerControllerCompPtr));
			}
		}
	}

	void GameObjectController::removePlayerController(const unsigned long id)
	{
		if (0 == id)
		{
			return;
		}
		auto& it = this->playerControllerComponentMap.find(id);
		if (it != this->playerControllerComponentMap.end())
		{
			it->second->setActivated(false);
			this->playerControllerComponentMap.erase(it);
		}
	}

	void GameObjectController::activatePlayerController(bool active, const unsigned long id, bool onlyOneActive)
	{
		if (true == onlyOneActive)
		{
			for (auto& it = this->playerControllerComponentMap.begin(); it != this->playerControllerComponentMap.end(); it++)
			{
				it->second->getOwner()->selected = false;
				// Do not deactivate if once active, else its no more possible to select one player which shall advance to its goal and select another one to advance to a different goal and animate each
				// player, because the state machine is not updated if not active
				// it->second->setActivated(false);
				if (nullptr != it->second->getCameraBehaviorComponent())
				{
					it->second->getCameraBehaviorComponent()->setActivated(false);
				}
			}
		}
		auto& existingPlayerControllerIt = this->playerControllerComponentMap.find(id);
		if (existingPlayerControllerIt != this->playerControllerComponentMap.cend())
		{
			existingPlayerControllerIt->second->getOwner()->selected = active;
			existingPlayerControllerIt->second->setActivated(active);
			if (nullptr != existingPlayerControllerIt->second->getCameraBehaviorComponent())
			{
				existingPlayerControllerIt->second->getCameraBehaviorComponent()->setActivated(active);
			}
		}
	}

	void GameObjectController::deactivateAllPlayerController(void)
	{
		for (auto& it = this->playerControllerComponentMap.begin(); it != this->playerControllerComponentMap.end(); it++)
		{
			it->second->setActivated(false);
			if (nullptr != it->second->getCameraBehaviorComponent())
			{
				it->second->getCameraBehaviorComponent()->setActivated(false);
			}
		}
	}

	void GameObjectController::addPhysicsCompoundConnectionComponent(boost::weak_ptr<PhysicsCompoundConnectionComponent> physicsCompoundConnectionCompWeakPtr)
	{
		PhysicsCompoundConnectionCompPtr physicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(physicsCompoundConnectionCompWeakPtr);
		if (nullptr != physicsCompoundConnectionCompPtr)
		{
			if (0 == physicsCompoundConnectionCompPtr->getId())
			{
				return;
			}
			auto& existingPhysicsCompoundConnectionCompPtr = this->physicsCompoundConnectionComponentMap.find(physicsCompoundConnectionCompPtr->getId());
			if (existingPhysicsCompoundConnectionCompPtr == this->physicsCompoundConnectionComponentMap.cend())
			{
				this->physicsCompoundConnectionComponentMap.insert(std::make_pair(physicsCompoundConnectionCompPtr->getId(), physicsCompoundConnectionCompPtr));
			}
		}
	}

	void GameObjectController::removePhysicsCompoundConnectionComponent(const unsigned long id)
	{
		if (0 == id)
		{
			return;
		}
		auto& it = this->physicsCompoundConnectionComponentMap.find(id);
		if (it != this->physicsCompoundConnectionComponentMap.end())
		{
			this->physicsCompoundConnectionComponentMap.erase(it);
		}
	}

	boost::weak_ptr<PhysicsCompoundConnectionComponent> GameObjectController::getPhysicsRootCompoundConnectionComponent(const unsigned long rootId) const
	{
		if (0 == rootId)
		{
			return boost::weak_ptr<PhysicsCompoundConnectionComponent>();
		}
		// First try: search by id
		auto& it = this->physicsCompoundConnectionComponentMap.find(rootId);
		// Check also if its the root (predecessor id = 0)
		if (it != this->physicsCompoundConnectionComponentMap.end() && it->second->getRootId() == 0)
		{
			return it->second;
		}
		// Nothing found, next try: search by prior id, maybe this compound connection was another one in a previous life before he has been cloned so get him!
		for (auto& it = this->physicsCompoundConnectionComponentMap.cbegin(); it != this->physicsCompoundConnectionComponentMap.cend(); ++it)
		{
			// Check also if its the root (predecessor id = 0)
			if (it->second->getPriorId() == rootId && it->second->getRootId() == 0)
			{
				// blur the trace
				it->second->internalSetPriorId(0);
				return it->second;
			}
		}

		return boost::weak_ptr<PhysicsCompoundConnectionComponent>();
	}

	/*class MovingBehaviorDeleter
	{
	public:
		void operator()(KI::MovingBehavior* movingBehavior)
		{
			AppStateManager::getSingletonPtr()->getGameObjectController()->removeMovingBehavior(movingBehavior->getAgentId());
			delete movingBehavior;
		}
	};*/

	boost::shared_ptr<KI::MovingBehavior> GameObjectController::addMovingBehavior(const unsigned long gameObjectId)
	{
		auto& it = this->movingBehaviors.find(gameObjectId);
		if (it == this->movingBehaviors.end())
		{
			// boost::shared_ptr<KI::MovingBehavior> movingBehaviorPtr(new KI::MovingBehavior(gameObjectId), MovingBehaviorDeleter());
			boost::shared_ptr<KI::MovingBehavior> movingBehaviorPtr(boost::make_shared<KI::MovingBehavior>(gameObjectId));
			this->movingBehaviors.emplace(gameObjectId, movingBehaviorPtr);
			return movingBehaviorPtr;
		}
		else
		{
			return it->second;
		}
	}

	void GameObjectController::removeMovingBehavior(const unsigned long gameObjectId)
	{
		this->movingBehaviors.erase(gameObjectId);
	}

	boost::shared_ptr<KI::MovingBehavior> GameObjectController::getMovingBehavior(const unsigned long gameObjectId)
	{
		auto& it = this->movingBehaviors.find(gameObjectId);
		if (it != this->movingBehaviors.end())
		{
			return it->second;
		}
		return nullptr;
	}

	void GameObjectController::connectJoints(void)
	{
		for (auto& currentJointComponent : this->jointComponentMap)
		{
			if (0 == currentJointComponent.second->getPredecessorId())
			{
				// If the current joint component has no predecessor it is the root, so create the joint
				bool success = currentJointComponent.second->createJoint();
				if (!success)
				{
					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not create joint: " + currentJointComponent.second->getOwner()->getName());
				}
			}
			else
			{
				// Find the predecessor joint component of the current joint component
				auto& predecessorJointCompPtr = NOWA::makeStrongPtr(this->getJointComponent(currentJointComponent.second->getPredecessorId()));
				if (nullptr == predecessorJointCompPtr)
				{
					Ogre::String jointName = currentJointComponent.second->getName();
					if (true == jointName.empty())
					{
						jointName = currentJointComponent.second->getOwner()->getName();
					}

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not create joint '" + jointName + "' because there is no such predecessor joint id: "
						+ Ogre::StringConverter::toString(currentJointComponent.second->getPredecessorId()));
				}
				else
				{
					// When found connect the predecessor body to the current joint component
					currentJointComponent.second->connectPredecessorId(predecessorJointCompPtr->getId());

					// Only create the joint here, if there is no target id, because if there is one, the joint is never the less created in the below section
					if (0 == currentJointComponent.second->getTargetId())
					{
						// and create the joint
						bool success = currentJointComponent.second->createJoint();
						if (!success)
						{
							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not create joint: " + currentJointComponent.second->getOwner()->getName());
						}
					}
				}

				if (0 == currentJointComponent.second->getTargetId())
				{
					continue;
				}

				// Find the target joint handler of the current joint handler
				auto& targetJointCompPtr = NOWA::makeStrongPtr(this->getJointComponent(currentJointComponent.second->getTargetId()));
				if (nullptr == targetJointCompPtr)
				{
					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not create joint because there is no such target joint id: "
						+ Ogre::StringConverter::toString(currentJointComponent.second->getTargetId()));
				}
				else
				{
					// When found connect the predecessor body to the current joint handler
					currentJointComponent.second->connectTargetId(targetJointCompPtr->getId());
					// and create the joint
					bool success = currentJointComponent.second->createJoint();
					if (!success)
					{
						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not create joint: " + currentJointComponent.second->getOwner()->getName());
					}
				}
			}
		}
	}

	//void GameObjectController::disconnectJoints(void)
	//{
	//	for (auto& currentJointComponent : this->jointComponentMap)
	//	{
	//		currentJointComponent.second->releaseConnectionToOtherJoints();
	//		currentJointComponent.second->releaseJoint();
	//		// Important: Do not set the predecessor id to null, because its an attribute, maybe also used in an editor and hence the predecessor will never be found!
	//	}
	//	this->jointComponentMap.clear();
	//}

	void GameObjectController::connectCompoundCollisions(void)
	{
		std::vector<PhysicsCompoundConnectionCompPtr> physicsCompoundConnectionRootComponentList;
		
		for (auto& currentPhysicsCompoundConnectionComponent : this->physicsCompoundConnectionComponentMap)
		{
			if (0 == currentPhysicsCompoundConnectionComponent.second->getRootId())
			{
				// If the current compound component has no predecessor it is the root, add it to the root list and continue gathering
				physicsCompoundConnectionRootComponentList.emplace_back(currentPhysicsCompoundConnectionComponent.second);
			}
			else
			{
				// Find the root compound connection of the current compound connection
				auto& rootPhysicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(this->getPhysicsRootCompoundConnectionComponent(currentPhysicsCompoundConnectionComponent.second->getRootId()));
				if (nullptr == rootPhysicsCompoundConnectionCompPtr)
				{
					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not create compound collision because there is no such root game object id: "
						+ Ogre::StringConverter::toString(currentPhysicsCompoundConnectionComponent.second->getRootId()));
				}
				else
				{
					// When found physics active component of compound connection add it to the list of the roots
					auto& physicsActiveCompPtr = NOWA::makeStrongPtr(currentPhysicsCompoundConnectionComponent.second->getOwner()->getComponent<PhysicsActiveComponent>());
					if (nullptr != physicsActiveCompPtr)
					{
						rootPhysicsCompoundConnectionCompPtr->addPhysicsActiveComponent(physicsActiveCompPtr.get());
					}
					else
					{
						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not add compound collision because the compound connection has no physics active component for game object id: "
							+ Ogre::StringConverter::toString(currentPhysicsCompoundConnectionComponent.second->getRootId()));
					}
				}
			}
		}

		for (size_t i = 0; i < physicsCompoundConnectionRootComponentList.size(); i++)
		{
			auto& currentPhysicsCompoundConnectionComponent = physicsCompoundConnectionRootComponentList[i];
			// If the current compound component has no predecessor it is the root, so create the compound collision, for all connectec compounds in the 
			// PhysicsCompoundConnectionComponent list
			bool success = currentPhysicsCompoundConnectionComponent->createCompoundCollision();
			if (!success)
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not compound collision for game object: " 
					+ currentPhysicsCompoundConnectionComponent->getOwner()->getName());
			}
		}

		
		
		//this->tempCompoundObjectMap.clear();
		//
		//for (auto& it = this->compoundCollisionMap.cbegin(); it != this->compoundCollisionMap.cend(); ++it)
		//{
		//	std::vector<PhysicsActiveComponent*> compoundCollisionPropertiesList = it->second;
		//	PhysicsActiveComponent* rootPhysicsComponent = it->first.second;
		//	rootPhysicsComponent->createCompoundBody(compoundCollisionPropertiesList);
		//}
		//// after creation clear the map!
		//this->compoundCollisionMap.clear();
	}

	void GameObjectController::disconnectCompoundCollisions(void)
	{
		/*for (auto& it = this->compoundCollisionMap.cbegin(); it != this->compoundCollisionMap.cend(); ++it)
		{
			std::vector<PhysicsActiveComponent*> compoundCollisionPropertiesList = it->second;
			PhysicsActiveComponent* rootPhysicsComponent = it->first.second;
			rootPhysicsComponent->destroyCompoundBody(compoundCollisionPropertiesList);
		}
		this->tempCompoundObjectMap.clear();
		this->compoundCollisionMap.clear();*/
	}

	void GameObjectController::connectVehicles(void)
	{
		//this->tempVehicleObjectMap.clear();

		//for (auto& it = this->vehicleCollisionMap.cbegin(); it != this->vehicleCollisionMap.cend(); ++it)
		//{
		//	std::vector<PhysicsActiveComponent*> vehiclePropertiesList = it->second;
		//	PhysicsActiveComponent* rootPhysicsComponent = it->first.second;
		//	// here return Vehicle
		//	rootPhysicsComponent->createVehicle(vehiclePropertiesList);
		//}
		//// after creation clear the map!
		//this->vehicleCollisionMap.clear();
	}

	void GameObjectController::disconnectVehicles(void)
	{
		//for (auto& it = this->vehicleCollisionMap.cbegin(); it != this->vehicleCollisionMap.cend(); ++it)
		//{
		//	std::vector<PhysicsActiveComponent*> vehiclePropertiesList = it->second;
		//	PhysicsActiveComponent* rootPhysicsComponent = it->first.second;
		//	// here return Vehicle
		//	rootPhysicsComponent->destroyVehicle(vehiclePropertiesList);
		//}
		//this->vehicleCollisionMap.clear();
	}

	void GameObjectController::start(void)
	{
		// Must be called, else all initial transforms, which are used via _getDerivedPosition() and not _getDerivedPositionUpdated() are 0 0 0 etc.!
		Ogre::Root::getSingletonPtr()->renderOneFrame();

		// Clears lua errors etc.
		LuaScriptApi::getInstance()->clear();

		if (true == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
		{
			AppStateManager::getSingletonPtr()->getOgreRecastModule(this->appStateName)->handleSceneModified(0);
			AppStateManager::getSingletonPtr()->getOgreRecastModule(this->appStateName)->buildNavigationMesh();
		}
		
		this->isSimulating = true;
		AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->start();

		LuaScriptApi::getInstance()->runInitScript("init.lua");
		
		GameObjectPtr mainGameObjectPtr = nullptr;
		// If all game objects had been loaded finally initialize all components of all objects
		// This is done here, because at this time, all game objects are loaded and in the final init, its possible for a component to get data from other game objects or components
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			const auto& gameObjectPtr = it->second;
			if (MAIN_GAMEOBJECT_ID != gameObjectPtr->getId())
			{
				gameObjectPtr->connect();
			}
			else
			{
				// 1111111111L = MainGameObject, do connect when everything has been handled because in the MainGameObject may have components, that are relied on other game objects
				// which must be already connected!
				mainGameObjectPtr = gameObjectPtr;
			}
		}

		// First connect compound collision objects, because joints need a valid phyics object
		this->connectCompoundCollisions();

		// Connect physics joints
		this->connectJoints();

		// Connect vehicles
		this->connectVehicles();

		if (nullptr != mainGameObjectPtr)
		{
			mainGameObjectPtr->connect();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Warning Could not get main game object!");
		}
	}

	void GameObjectController::stop(void)
	{
		// Resets the command, so that deletion of game object can be processed again.
		// See @deleteGameObjectWithUndo for more information
		if (nullptr != this->deleteGameObjectsUndoCommand)
		{
			this->deleteGameObjectsUndoCommand.reset();
		}
		this->nextGameObjectIndex = 0;
		// Delete all user defined attributes (when lua script has been disconnected and re-connected, this is required)
		AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->stop();
		AppStateManager::getSingletonPtr()->getScriptEventManager(this->appStateName)->destroyContent();

		this->deactivateAllPlayerController();

		this->isSimulating = false;
		
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			const auto& gameObjectPtr = it->second;
			if (!gameObjectPtr->disconnect())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Could not disconnect game object: '" + gameObjectPtr->getName() + "'");
				throw Ogre::Exception(Ogre::Exception::ERR_FILE_NOT_FOUND, "[GameObjectController] Could not disconnect game object: '" + gameObjectPtr->getName() + "'", "NOWA");
			}
		}

		LuaScriptApi::getInstance()->stopInitScript("init.lua");
		
		this->disconnectCompoundCollisions();
		// this->disconnectJoints();
		this->jointComponentMap.clear();
		this->physicsCompoundConnectionComponentMap.clear();
		this->disconnectVehicles();
	}

	void GameObjectController::pause(void)
	{
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			it->second->pause();
		}
	}

	void GameObjectController::resume(void)
	{
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			it->second->resume();
		}
	}

	void GameObjectController::connectClonedGameObjects(const std::vector<unsigned long> gameObjectIds)
	{
		if (true == AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
		{
			AppStateManager::getSingletonPtr()->getOgreRecastModule()->buildNavigationMesh();
		}
		
		// If all game objects had been loaded finally initialize all components of all objects
		// This is done here, because at this time, all game objects are loaded and in the final init, its possible for a component to get data from other game objects or components
		for (size_t i = 0; i < gameObjectIds.size(); i++)
		{
			GameObjectPtr gameObjectPtr = this->getGameObjectFromId(gameObjectIds[i]);
			if (nullptr != gameObjectPtr)
			{
				if (false == gameObjectPtr->onCloned())
				{
					// throw Ogre::Exception(Ogre::Exception::ERR_FILE_NOT_FOUND, "[GameObjectController] Could not connect game object: '" + gameObjectPtr->getName() + "'", "NOWA");
				}
			}
			else
			{
				/*throw Ogre::Exception(Ogre::Exception::ERR_FILE_NOT_FOUND, "[GameObjectController] Could not connect game object id: '" + 
					Ogre::StringConverter::toString(gameObjectIds[i]) + "' because it does not exist", "NOWA");*/
			}
		}

		// First connect compound collision objects, because joints need a valid phyics object
		this->connectCompoundCollisions();

		// Connect physics joints
		this->connectJoints();

		// Connect vehicles
		this->connectVehicles();
	}

	std::vector<unsigned long> GameObjectController::getIdsFromCategory(const Ogre::String& category) const
	{
		std::vector<unsigned long> vec;
		// do not delete with iterator since the iterator changes then during the loop
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (it->second->getCategory() == category)
			{
				vec.emplace_back(it->second->getId());
			}
		}
		return std::move(vec);
	}

	std::vector<unsigned long> GameObjectController::getOtherIdsFromCategory(GameObjectPtr excludedGameObjectPtr, const Ogre::String& category) const
	{
		std::vector<unsigned long> vec;
		// do not delete with iterator since the iterator changes then during the loop
		// Attention here with find, because its o(log2n) and not o(n)
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (it->second->getCategory() == category && it->second != excludedGameObjectPtr)
			{
				vec.emplace_back(it->second->getId());
			}
		}
		return std::move(vec);
	}

	GameObjectPtr GameObjectController::getGameObjectFromName(const Ogre::String& name) const
	{
		// Attention here with find, because its o(log2n) and not o(n)
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (it->second->getSceneNode()->getName() == name)
			{
				return it->second;
			}
		}

		return GameObjectPtr();
	}

	std::vector<GameObjectPtr> GameObjectController::getGameObjectsFromNamePrefix(const Ogre::String& pattern) const
	{
		std::vector<GameObjectPtr> vec;
		// Attention here with find, because its o(log2n) and not o(n) but how? because it should work with the pattern, how could i use find(pattern)?
		// http://stackoverflow.com/questions/288775/stlmap-walk-through-list-or-use-find
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (Ogre::StringUtil::match(it->second->getSceneNode()->getName(), pattern, false))
			{
				vec.emplace_back(it->second);
			}
		}

		return std::move(vec);
	}

	std::vector<GameObjectPtr> GameObjectController::getGameObjectsFromType(GameObject::eType type) const
	{
		std::vector<GameObjectPtr> vec;

		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (type == it->second->getType())
			{
				vec.emplace_back(it->second);
			}
		}

		return std::move(vec);
	}

	GameObjectPtr GameObjectController::getGameObjectFromNamePrefix(const Ogre::String& pattern) const
	{
		// Attention here with find, because its o(log2n) and not o(n)
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (Ogre::StringUtil::match(it->second->getSceneNode()->getName(), pattern, false))
			{
				return it->second;
			}
		}

		return GameObjectPtr();
	}

	std::vector<GameObjectPtr> GameObjectController::getGameObjectsFromCategory(const Ogre::String& category)
	{
		std::vector<GameObjectPtr> vec;

		unsigned int finalCategories = this->generateCategoryId(category);
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if ((it->second->getCategoryId() | finalCategories) == finalCategories)
			{
				vec.emplace_back(it->second);
			}
		}
		return std::move(vec);
	}
	
	std::vector<GameObjectPtr> GameObjectController::getGameObjectsFromCategoryId(unsigned int categoryId)
	{
		std::vector<GameObjectPtr> vec;

		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if ((it->second->getCategoryId() | categoryId) == categoryId)
			{
				vec.emplace_back(it->second);
			}
		}
		return std::move(vec);
	}

	std::pair<std::vector<GameObjectPtr>, std::vector<GameObjectPtr>> GameObjectController::getGameObjectsFromCategorySeparate(const Ogre::String& category, 
		const std::vector<Ogre::String> separateGameObjects)
	{
		std::vector<GameObjectPtr> vec1;
		std::vector<GameObjectPtr> vec2;

		unsigned int finalCategories = this->generateCategoryId(category);
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if ((it->second->getCategoryId() | finalCategories) == finalCategories)
			{
				bool addedToSeparateList = false;
				for (size_t i = 0; i < separateGameObjects.size(); i++)
				{
					if (it->second->getName() == separateGameObjects[i])
					{
						vec2.emplace_back(it->second);
						addedToSeparateList = true;
						break;
					}
				}

				if (false == addedToSeparateList)
					vec1.emplace_back(it->second);
			}
		}
		return std::make_pair(vec1, vec2);
	}

	GameObject* GameObjectController::getNextGameObject(unsigned int categoryIds)
	{
		const auto& gameObjects = this->getGameObjectsFromCategoryId(categoryIds);

		if (true == gameObjects.empty())
		{
			return nullptr;
		}

		// Ring indexing
		if (this->nextGameObjectIndex > gameObjects.size() - 1)
		{
			this->nextGameObjectIndex = 0;
		}

		const auto nextGameObjectPtr = gameObjects[this->nextGameObjectIndex++];
		if (nullptr != nextGameObjectPtr)
		{
			// Is it part of the category ids?
			if ((nextGameObjectPtr->getCategoryId() | categoryIds) == categoryIds)
			{
				return nextGameObjectPtr.get();
			}
		}

		return nullptr;
	}

	GameObject* GameObjectController::getNextGameObject(const Ogre::String& strCategoryIds)
	{
		unsigned int categoryIds = this->getCategoryId(strCategoryIds);

		return getNextGameObject(categoryIds);
	}

	std::vector<GameObjectPtr> GameObjectController::getGameObjectsControlledByClientId(unsigned int clientID) const
	{
		std::vector<GameObjectPtr> vec;
		// only search for game objects that are controlled by a client. Client id 0 means, it is not controlled
		if (0 == clientID)
		{
			return std::move(vec);
		}

		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (it->second->getControlledByClientID() == clientID)
			{
				vec.emplace_back(it->second);
			}
		}
		return std::move(vec);
	}

	std::vector<GameObjectPtr> GameObjectController::getGameObjectsFromTagName(const Ogre::String& tagName) const
	{
		std::vector<GameObjectPtr> vec;
		// only search for game objects that are controlled by a client. Client id 0 means, it is not controlled
		if (true == tagName.empty())
		{
			return std::move(vec);
		}

		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (it->second->getTagName() == tagName)
			{
				vec.emplace_back(it->second);
			}
		}
		return std::move(vec);
	}

	GameObjectPtr GameObjectController::getGameObjectFromReferenceId(const unsigned long referenceId) const
	{
		return this->getGameObjectFromId(referenceId);
	}

	std::vector<GameObjectCompPtr> GameObjectController::getGameObjectComponentsFromReferenceId(const unsigned long referenceId) const
	{
		std::vector<GameObjectCompPtr> vec;

		auto& gameObjectPtr = this->getGameObjectFromId(referenceId);
		if (nullptr == gameObjectPtr)
		{
			return std::move(vec);
		}

		// Search for a component with the same id
		GameObjectComponents* gameobjectComponents = gameObjectPtr->getComponents();

		for (auto& it = gameobjectComponents->cbegin(); it != gameobjectComponents->cend(); ++it)
		{
			GameObjectCompPtr gameObjectComponent = std::get<COMPONENT>(*it);

			if (gameObjectComponent->getReferenceId() == referenceId)
			{
				vec.emplace_back(gameObjectComponent);
			}
		}
		return std::move(vec);
	}

	void GameObjectController::activateGameObjectComponentsFromReferenceId(const unsigned long referenceId, bool activate)
	{
		auto& gameObjectPtr = this->getGameObjectFromId(referenceId);
		if (nullptr == gameObjectPtr)
		{
			return;
		}

		// Search for a component with the same id
		GameObjectComponents* gameobjectComponents = gameObjectPtr->getComponents();

		for (auto& it = gameobjectComponents->cbegin(); it != gameobjectComponents->cend(); ++it)
		{
			GameObjectCompPtr gameObjectComponent = std::get<COMPONENT>(*it);

			if (gameObjectComponent->getReferenceId() == referenceId)
			{
				gameObjectComponent->setActivated(activate);
			}
		}
	}
	std::vector<Ogre::String> GameObjectController::getAllCategoriesSoFar(void) const
	{
		std::vector<Ogre::String> vec;
		for (auto& it = this->typeDBMap.cbegin(); it != this->typeDBMap.cend(); ++it)
		{
			vec.emplace_back(it->first);
		}
		return std::move(vec);
	}

	bool GameObjectController::hasCategory(const Ogre::String& category) const
	{
		auto& found = this->typeDBMap.find(category);
		return found != this->typeDBMap.cend();
	}

	std::map<unsigned long, GameObjectPtr> const* GameObjectController::getGameObjects(void) const
	{
		return this->gameObjects;
	}

	void GameObjectController::registerType(boost::shared_ptr<GameObject> gameObjectPtr, const Ogre::String& category)
	{
		this->registerType(gameObjectPtr.get(), category);
	}

	void GameObjectController::registerType(GameObject* gameObject, const Ogre::String& category)
	{
		unsigned int categoryId = 1;
		// skip default category, since its for everything
		/*if ("Default" == category)
		{
		return;
		}*/
		// http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Intermediate+Tutorial+3
		// only 31 distinct types are possible!
		auto& it = this->typeDBMap.find(category);
		// if the type is not in the map, then its a new type, so increment the physics type id
		if (it == this->typeDBMap.end())
		{
			bool foundFreeId = false;
			// First check if there is an old remaining category id, that is not occupied
			for (auto& it = this->typeDBMap.begin(); it != this->typeDBMap.end(); ++it)
			{
				if (false == it->second.second)
				{
					foundFreeId = true;
					// Change the key
					std::swap(this->typeDBMap[category], it->second);
					this->typeDBMap.erase(it);
					break;
				}
			}

			if (false == foundFreeId)
			{
				// deliver the categoryId to the gameobject
				categoryId <<= this->shiftIndex++;
				// unsigned int = 4 Byte = FF FF FF FF
				if (32 == this->shiftIndex)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Querymask overflow, because to many different categories are defined. Allowed are up to 31.");
					throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameObjectController] Querymask overflow, because to many different categories are defined. Allowed are up to 31.", "NOWA");
				}
				// name, id, occupied
				this->typeDBMap.emplace(category, std::pair<unsigned int, bool>(categoryId, true));
			}
		}
		else
		{
			// set the knowing id but do not insert to map
			categoryId = it->second.first;
		}

		// set the knowing id but do not insert to map
		gameObject->categoryId->setValue(categoryId);
		Ogre::MovableObject* movableObject = gameObject->getMovableObject();
		if (nullptr != movableObject)
		{
			movableObject->setQueryFlags(categoryId);
		}
	}

	unsigned int GameObjectController::registerCategory(const Ogre::String& category)
	{
		unsigned int categoryId = 1;
		// skip default category, since its for everything
		/*if ("Default" == category)
		{
		return;
		}*/
		// http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Intermediate+Tutorial+3
		// only 31 distinct types are possible!
		auto& foundIt = this->typeDBMap.find(category);
		// if the type is not in the map, then its a new type, so increment the physics type id
		if (foundIt == this->typeDBMap.end())
		{
			bool foundFreeId = false;
			// First check if there is an old remaining category id, that is not occupied
			for (auto& it = this->typeDBMap.begin(); it != this->typeDBMap.end(); ++it)
			{
				// Not occupied
				if (false == it->second.second)
				{
					// Occupie the category!
					it->second.second = true;
					foundFreeId = true;
					// Change the key
					std::swap(this->typeDBMap[category], it->second);
					this->typeDBMap.erase(it);
					break;
				}
			}
			
			if (false == foundFreeId)
			{
				// shift the category id
				categoryId <<= this->shiftIndex++;
				// unsigned int = 4 Byte = FF FF FF FF
				if (32 == this->shiftIndex)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Querymask overflow, because to many different categories are defined. Allowed are up to 31.");
					throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameObjectController] Querymask overflow, because to many different categories are defined. Allowed are up to 31.", "NOWA");
				}
				// name, id, occupied
				this->typeDBMap.emplace(category, std::pair<unsigned int, bool>(categoryId, true));
			}
		}
		return categoryId;
	}

	void GameObjectController::changeCategory(GameObject* gameObject, const Ogre::String& oldCategory, const Ogre::String& newCategory)
	{
		// Check if there is a remaining game object with the old category, if not set the category occupied = false, so that a new category may use that one
		bool found = false;
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (oldCategory == it->second->getCategory())
			{
				found = true;
				break;
			}
		}

		if (false == found)
		{
			auto& it = this->typeDBMap.find(oldCategory);
			if (it != this->typeDBMap.end())
			{
				// set occupied = false
				it->second.second = false;
			}
		}
		this->registerType(gameObject, newCategory);

		// Since a category may change at any time, adjust the material group id
		auto& physicsCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsComponent>());
		if (nullptr != physicsCompPtr)
		{
			if (nullptr != physicsCompPtr->getBody())
			{
				physicsCompPtr->getBody()->setMaterialGroupID(this->getMaterialID(gameObject, physicsCompPtr->getOgreNewt()));
				physicsCompPtr->getBody()->setType(gameObject->getCategoryId());
			}
		}
	}

	void GameObjectController::freeCategoryFromGameObject(const Ogre::String& category)
	{
		bool found = false;
		for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
		{
			if (category == it->second->getCategory())
			{
				found = true;
				break;
			}
		}

		if (false == found)
		{
			auto& it = this->typeDBMap.find(category);
			if (it != this->typeDBMap.end())
			{
				// set occupied = false
				it->second.second = false;
			}
		}
	}

	unsigned int GameObjectController::getCategoryId(const Ogre::String& category)
	{
		auto& it = this->typeDBMap.find(category);
		if (it != this->typeDBMap.end())
		{
			return it->second.first;
		}
		return 0;
	}

	unsigned int GameObjectController::generateCategoryId(const Ogre::String& categoryNames)
	{
		Ogre::String tempCategoryNames = categoryNames;
		// Ogre::StringUtil::toLowerCase(tempCategoryNames);
		if (tempCategoryNames == "All")
		{
			return GameObjectController::ALL_CATEGORIES_ID;
		}

		if (tempCategoryNames.substr(0, 1) != "-" && tempCategoryNames.substr(0, 1) != "+")
		{
			tempCategoryNames = "+" + tempCategoryNames;
		}

		unsigned int generatedCategoryId = 0;
		Ogre::StringVector::iterator it;
		Ogre::StringVector categoryNamesToAdd;
		Ogre::StringVector categoryNamesToSubstract;

		short addIndex = -1;
		short substractIndex = -1;
		unsigned int i = 0;
		for (; i < tempCategoryNames.size(); i++)
		{
			if (tempCategoryNames[i] == '+')
			{
				if (substractIndex != -1)
				{
					categoryNamesToSubstract.push_back(tempCategoryNames.substr(substractIndex + 1, i - substractIndex - 1));
					substractIndex = -1;
				}
				else if (addIndex != -1)
				{
					categoryNamesToAdd.push_back(tempCategoryNames.substr(addIndex + 1, i - addIndex - 1));
					addIndex = -1;
				}
				addIndex = i;
			}
			else if (tempCategoryNames[i] == '-')
			{
				if (addIndex != -1)
				{
					categoryNamesToAdd.push_back(tempCategoryNames.substr(addIndex + 1, i - addIndex - 1));
					addIndex = -1;
				}
				else if (substractIndex != -1)
				{
					categoryNamesToSubstract.push_back(tempCategoryNames.substr(substractIndex + 1, i - substractIndex - 1));
					substractIndex = -1;
				}
				substractIndex = i;
			}
		}
		// finally if there comes no sign anymore, add the category so far
		if (substractIndex != -1)
		{
			categoryNamesToSubstract.push_back(tempCategoryNames.substr(substractIndex + 1, i - substractIndex - 1));
		}
		else if (addIndex != -1)
		{
			categoryNamesToAdd.push_back(tempCategoryNames.substr(addIndex + 1, i - addIndex - 1));
		}

		if (categoryNamesToAdd.size() > 0)
		{
			for (it = categoryNamesToAdd.begin(); it != categoryNamesToAdd.end(); ++it)
			{
				Ogre::String categoryName = *it;
				// match to lower case
				// Ogre::StringUtil::toLowerCase(categoryName);
				if (categoryName == "All")
				{
					generatedCategoryId |= GameObjectController::ALL_CATEGORIES_ID;
				}
				else
				{
					unsigned int currentCategoryId = this->getCategoryId(*it);
					// if there is no such category throw an exception to warn the developer
					if (currentCategoryId == 0)
					{
						// Not necessary, because if its a global scene, it may be that the given category is in another scene
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Warning could not find category id for " + *it);
						currentCategoryId = 0;
					}
					// add the bits
					generatedCategoryId |= currentCategoryId;
				}
			}
		}
		if (categoryNamesToSubstract.size() > 0)
		{
			for (it = categoryNamesToSubstract.begin(); it != categoryNamesToSubstract.end(); ++it)
			{
				Ogre::String categoryName = *it;
				// match to lower case
				// Ogre::StringUtil::toLowerCase(categoryName);
				unsigned int currentCategoryId = this->getCategoryId(*it);
				// if there is no such category throw an exception to warn the developer
				if (currentCategoryId == -1)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController] Warning could not find category id for " + *it);
					currentCategoryId = 0;
				}
				generatedCategoryId &= ~currentCategoryId;
			}
		}
		return generatedCategoryId;
	}

	std::vector<Ogre::String> GameObjectController::getAffectedCategories(const Ogre::String& categoryNames)
	{
		std::vector<Ogre::String> vec;

		Ogre::String tempCategoryNames = categoryNames;
		// Ogre::StringUtil::toLowerCase(tempCategoryNames);
		if (tempCategoryNames == "All")
		{
			for (auto& it = this->typeDBMap.cbegin(); it != this->typeDBMap.cend(); ++it)
			{
				vec.emplace_back(it->first);
			}
			return std::move(vec);
		}

		Ogre::StringVector::iterator it;
		Ogre::StringVector categoryNamesToAdd;
		Ogre::StringVector categoryNamesToSubstract;

		short addIndex = -1;
		short substractIndex = -1;
		int i = 0;
		for (; i < static_cast<int>(categoryNames.size()); i++)
		{
			if (categoryNames[i] == '+')
			{
				if (substractIndex != -1)
				{
					categoryNamesToSubstract.push_back(categoryNames.substr(substractIndex + 1, i - substractIndex - 1));
					substractIndex = -1;
				}
				else if (addIndex != -1)
				{
					categoryNamesToAdd.push_back(categoryNames.substr(addIndex + 1, i - addIndex - 1));
					addIndex = -1;
				}
				addIndex = i;
			}
			else if (categoryNames[i] == '-')
			{
				if (addIndex != -1)
				{
					categoryNamesToAdd.push_back(categoryNames.substr(addIndex + 1, i - addIndex - 1));
					addIndex = -1;
				}
				else if (substractIndex != -1)
				{
					categoryNamesToSubstract.push_back(categoryNames.substr(substractIndex + 1, i - substractIndex - 1));
					substractIndex = -1;
				}
				substractIndex = i;
			}
		}
		// finally if there comes no sign anymore, add the category so far
		if (substractIndex != -1)
		{
			categoryNamesToSubstract.push_back(categoryNames.substr(substractIndex + 1, i - substractIndex - 1));
		}
		else if (addIndex != -1)
		{
			categoryNamesToAdd.push_back(categoryNames.substr(addIndex + 1, i - addIndex - 1));
		}
		// First add all category ids
		if (categoryNamesToAdd.size() > 0)
		{
			for (it = categoryNamesToAdd.begin(); it != categoryNamesToAdd.end(); ++it)
			{
				Ogre::String categoryName = *it;
				// match to lower case
				// Ogre::StringUtil::toLowerCase(categoryName);
				if (categoryName == "All")
				{
					for (auto& it = this->typeDBMap.cbegin(); it != this->typeDBMap.cend(); ++it)
					{
						vec.emplace_back(it->first);
					}
				}
				else
				{
					vec.emplace_back(categoryName);
				}
			}
		}
		// Substract from the category ids that had been added
		if (categoryNamesToSubstract.size() > 0)
		{
			for (it = categoryNamesToSubstract.begin(); it != categoryNamesToSubstract.end(); ++it)
			{
				Ogre::String categoryNameToSubstract = *it;
				// match to lower case
				// Ogre::StringUtil::toLowerCase(categoryNameToSubstract);
				for (int i = 0; i < static_cast<int>(vec.size()); i++)
				{
					Ogre::String currentCategoryName = vec[i];
					// Ogre::StringUtil::toLowerCase(currentCategoryName);
					if (categoryNameToSubstract == currentCategoryName)
					{
						vec.erase(vec.begin() + i);
						break;
					}
				}
			}
		}

		return std::move(vec);
	}

	void GameObjectController::getValidatedGameObjectName(Ogre::String& gameObjectName, unsigned long excludeId)
	{
		auto& gameObjectPtr = this->getGameObjectFromName(gameObjectName);
		if (nullptr == gameObjectPtr)
		{
			return;
		}
		else
		{
			// found itsself so skip
			if (gameObjectPtr->getId() == excludeId)
			{
				return;
			}
			
			unsigned int id = 0;
			Ogre::String validatedGameObjectName = gameObjectName;
			do
			{
				size_t found = validatedGameObjectName.rfind("_");
				if (Ogre::String::npos != found)
				{
					validatedGameObjectName = validatedGameObjectName.substr(0, found + 1);
				}
				else
				{
					validatedGameObjectName += "_";
				}
				// Do not use # anymore, because its reserved in mygui as code-word the # and everything after that will be removed!
				validatedGameObjectName += Ogre::StringConverter::toString(id++);
			} while (nullptr != this->getGameObjectFromName(validatedGameObjectName));
			gameObjectName = validatedGameObjectName;
		}
	}

	void GameObjectController::createAllGameObjectsForShaderCacheGeneration(Ogre::SceneManager* sceneManager)
	{
		// Get all meshes from all available resource groups
		Ogre::StringVector groups = Ogre::ResourceGroupManager::getSingletonPtr()->getResourceGroups();
		std::vector<Ogre::String>::iterator groupIt = groups.begin();
		while (groupIt != groups.end())
		{
			Ogre::String resourceGroupName = (*groupIt);
			if (resourceGroupName != "Essential" && resourceGroupName != "General" && resourceGroupName != "PostProcessing" && resourceGroupName != "Hlms"
				&& resourceGroupName != "NOWA" && resourceGroupName != "Internal" && resourceGroupName != "AutoDetect" && resourceGroupName != "Lua")
			{

				Ogre::StringVectorPtr dirs = Ogre::ResourceGroupManager::getSingletonPtr()->findResourceNames((*groupIt), "*.mesh");
				std::vector<Ogre::String>::iterator meshIt = dirs->begin();

				int x = 0;
				// int z = 0;

				while (meshIt != dirs->end())
				{
					Ogre::SceneNode* node = sceneManager->getRootSceneNode()->createChildSceneNode();
					Ogre::v1::Entity* entity = sceneManager->createEntity(*meshIt);
					MathHelper::getInstance()->substractOutTangentsForShader(entity);

					node->attachObject(entity);

					/*if (x % 100 == 0)
					{
						z++;
					}*/

					node->setPosition(Ogre::Vector3(static_cast<Ogre::Real>(x) * entity->getWorldRadius() * 2.0f, 0.0f, 0.0f/*static_cast<Ogre::Real>(z) * entity->getWorldRadius() * 2.0f*/));

					Ogre::String meshName = entity->getMesh()->getName();

					// Get name without .mesh
					size_t found = meshName.find(".");
					if (Ogre::String::npos != found)
					{
						meshName = meshName.substr(0, found);
					}

					// Do not use # anymore, because its reserved in mygui as code-word the # and everything after that will be removed!
					Ogre::String gameObjectName = meshName + "_0";
					this->getValidatedGameObjectName(gameObjectName);

					entity->setName(gameObjectName);
					node->setName(gameObjectName);
					GameObjectPtr newGameObjectPtr = GameObjectFactory::getInstance()->createGameObject(sceneManager, node, entity, GameObject::ENTITY);
					if (nullptr == newGameObjectPtr)
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectController]: Could not create game object for mesh name: '"
							+ meshName + "'");
					}

					++meshIt;
					x++;
				}

			}
			++groupIt;
		}
	}

	GameObject* GameObjectController::selectGameObject(int x, int y, Ogre::Camera* camera, Ogre::RaySceneQuery* raySceneQuery, bool raycastFromPoint)
	{
		GameObject* gameObject = nullptr;
		
		if (true == raycastFromPoint)
		{
			Ogre::MovableObject* targetMovableObject = nullptr;
			Ogre::Vector3 result = Ogre::Vector3::ZERO;
			Ogre::Real closestDistance = -1.0f;
			Ogre::Vector3 normal = Ogre::Vector3::ZERO;

			//check if there is an hit with an polygon of an entity
			if (MathHelper::getInstance()->getRaycastFromPoint(x, y, camera, Core::getSingletonPtr()->getOgreRenderWindow(),
				raySceneQuery, result, (size_t&)targetMovableObject, closestDistance, normal))
			{
				try
				{
					gameObject = Ogre::any_cast<GameObject*>((targetMovableObject)->getUserObjectBindings().getUserAny());
					return gameObject;
				}
				catch (...)
				{
					return nullptr;
				}
			}
		}
		else
		{
			Ogre::Real resultX;
			Ogre::Real resultY;
			MathHelper::getInstance()->mouseToViewPort(x, y, resultX, resultY, Core::getSingletonPtr()->getOgreRenderWindow());
			Ogre::Ray selectRay = camera->getCameraToViewportRay(resultX, resultY);
			raySceneQuery->setRay(selectRay);
			Ogre::RaySceneQueryResult& result = raySceneQuery->execute();

			for (const auto& it : result)
			{
				// Store the relevant picking information
				try
				{
					// here no shared_ptr because in this scope the game object should not extend the lifecycle! Only shared where really necessary
					gameObject = Ogre::any_cast<GameObject*>(it.movable->getUserObjectBindings().getUserAny());
					// GameObjectPtr gameObjectPtr = Ogre::any_cast<GameObject*>((*it).movable->getUserAny());
					return gameObject;
				}
				catch (...)
				{
					// if its a game object or else, catch the throw and return from the function
					return nullptr;
				}
			}
		}
		return nullptr;
	}

	GameObject* GameObjectController::selectGameObject(int x, int y, Ogre::Camera* camera, OgreNewt::World* ogreNewt, unsigned int categoryIds, Ogre::Real maxDistance, bool sorted)
	{
		GameObject* gameObject = nullptr;

		Ogre::Real resultX;
		Ogre::Real resultY;
		MathHelper::getInstance()->mouseToViewPort(x, y, resultX, resultY, Core::getSingletonPtr()->getOgreRenderWindow());
		// Get a world space ray as cast from the camera through a viewport position
		Ogre::Ray selectRay = camera->getCameraToViewportRay(resultX, resultY);
		Ogre::Vector3 startPoint = selectRay.getOrigin();
		Ogre::Vector3 endPoint = selectRay.getPoint(maxDistance);

		Ogre::Vector3 pos = Ogre::Vector3::ZERO;
		OgreNewt::BasicRaycast ray(ogreNewt, startPoint, endPoint, sorted);
		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				try
				{
					// here no shared_ptr because in this scope the game object should not extend the lifecycle! Only shared where really necessary
					Ogre::SceneNode* tempNode = static_cast<Ogre::SceneNode*>(info.mBody->getOgreNode());
					if (tempNode)
					{
						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserObjectBindings().getUserAny());
						// PhysicsComponent* physicsComponent = Ogre::any_cast<PhysicsComponent*>(info.mBody->getUserData());
						// GameObjectPtr gameObjectPtr = Ogre::any_cast<GameObject*>((*it).movable->getUserAny());
						// return physicsComponent->getOwner().get();
					}
					else
					{
						return nullptr;
					}
				}
				catch (...)
				{
					// if its a game object or else, catch the throw and return from the function
					return nullptr;
				}
			}
		}
		return gameObject;
	}

	std::pair<bool, Ogre::Vector3> GameObjectController::getTargetBodyPosition(int x, int y, Ogre::Camera* camera, OgreNewt::World* ogreNewt, unsigned int categoryIds, Ogre::Real maxDistance, bool sorted)
	{
		bool success = false;
		Ogre::Vector3 globalPos = Ogre::Vector3::ZERO;
		Ogre::Real resultX;
		Ogre::Real resultY;
		MathHelper::getInstance()->mouseToViewPort(x, y, resultX, resultY, Core::getSingletonPtr()->getOgreRenderWindow());
		// Get a world space ray as cast from the camera through a viewport position
		Ogre::Ray selectRay = camera->getCameraToViewportRay(resultX, resultY);

		// Get ray origing and end coordinates
		Ogre::Vector3 start = selectRay.getOrigin();
		Ogre::Vector3 end = selectRay.getPoint(maxDistance);

		OgreNewt::BasicRaycast ray(ogreNewt, start, end, sorted);
		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				globalPos = selectRay.getPoint(maxDistance * info.mDistance);
				success = true;
			}
		}
		return std::pair<bool, Ogre::Vector3>(success, globalPos);
	}

	void GameObjectController::initAreaForActiveObjects(Ogre::SceneManager* sceneManager, const Ogre::Vector3& position, Ogre::Real distance, unsigned int categoryIds, Ogre::Real updateFrequency)
	{
		this->currentSceneManager = sceneManager;
		this->sphereSceneQuery = this->currentSceneManager->createSphereQuery(Ogre::Sphere(position, distance));
		this->sphereSceneQuery->setQueryMask(categoryIds);
		this->sphereQueryUpdateFrequency = updateFrequency;
	}

	void GameObjectController::attachTriggerObserver(ITriggerSphereQueryObserver* triggerSphereQueryObserver)
	{
		this->triggerSphereQueryObservers.emplace_back(triggerSphereQueryObserver);
	}

	void GameObjectController::detachAndDestroyTriggerObserver(ITriggerSphereQueryObserver* triggerSphereQueryObserver)
	{
		auto it = std::find(this->triggerSphereQueryObservers.cbegin(), this->triggerSphereQueryObservers.cend(), triggerSphereQueryObserver);
		if (it != this->triggerSphereQueryObservers.end())
		{
			ITriggerSphereQueryObserver* observer = *it;
			this->triggerSphereQueryObservers.erase(it);
			delete observer;
			observer = nullptr;
		}
	}

	void GameObjectController::detachAndDestroyAllTriggerObserver(void)
	{
		for (auto& it = this->triggerSphereQueryObservers.cbegin(); it != this->triggerSphereQueryObservers.cend();)
		{
			ITriggerSphereQueryObserver* observer = *it;
			it = this->triggerSphereQueryObservers.erase(it);
			delete observer;
			observer = nullptr;
		}
		this->triggerSphereQueryObservers.clear();
	}

	void GameObjectController::checkAreaForActiveObjects(const Ogre::Vector3& position, Ogre::Real dt, Ogre::Real distance, unsigned int categoryIds)
	{
		if (0.0f < this->sphereQueryUpdateFrequency)
		{
			this->triggerUpdateTimer += dt;
		}
		// check area only 2x a second
		if (this->triggerUpdateTimer >= this->sphereQueryUpdateFrequency)
		{
			// this->triggerUpdateTimer = this->sphereQueryUpdateFrequency;
			// Attention:
			this->triggerUpdateTimer = 0.0f;

			Ogre::Sphere updateSphere(position, distance);
			this->sphereSceneQuery->setSphere(updateSphere);
			if (0 < categoryIds)
			{
				this->sphereSceneQuery->setQueryMask(categoryIds);
			}

			// check objects in range
			Ogre::SceneQueryResultMovableList& result = this->sphereSceneQuery->execute().movables;
			for (auto& it = result.cbegin(); it != result.cend(); ++it)
			{
				Ogre::MovableObject* movableObject = *it;

				// if the query flags are part of the final category
				// unsigned int finalCategory = this->sphereSceneQuery->getQueryMask() & movableObject->getQueryFlags();
				// if (this->sphereSceneQuery->getQueryMask() == finalCategory)
				{
					NOWA::GameObject* gameObject = Ogre::any_cast<NOWA::GameObject*>(movableObject->getUserObjectBindings().getUserAny());
					if (gameObject)
					{
						// when this is active, the onEnter callback will only be called once for the object!
						auto& it = this->triggeredGameObjects.find(gameObject->getId());
						if (it == this->triggeredGameObjects.end())
						{
// here also set the script to be called, so that in ActiveObjectsResultCallback can be run in script and call spawn callback for another script?
							for (auto& subIt = this->triggerSphereQueryObservers.cbegin(); subIt != this->triggerSphereQueryObservers.cend(); ++subIt)
							{
								// notify the observer
								(*subIt)->onEnter(gameObject);
							}
							// add the game object to a map
							this->triggeredGameObjects.emplace(gameObject->getId(), gameObject);
						}
					}
				}
			}

			// go through the map with the triggered game objects that are in range
			for (auto& it = this->triggeredGameObjects.cbegin(); it != this->triggeredGameObjects.cend(); ++it)
			{
				GameObject* gameObject = it->second;
				Ogre::Vector3& direction = position - gameObject->getPosition();
				// if a game objects comes out of the range, remove it and notify the observer
				Ogre::Real distanceToGameObject = direction.squaredLength();
				if (distanceToGameObject > distance * distance)
				{
					for (auto& subIt = this->triggerSphereQueryObservers.cbegin(); subIt != this->triggerSphereQueryObservers.cend(); ++subIt)
					{
						(*subIt)->onLeave(gameObject);
					}
					this->triggeredGameObjects.erase(it->first);
					break;
				}
			}
		}
	}

}; //namespace end NOWA