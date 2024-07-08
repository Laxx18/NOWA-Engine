#include "NOWAPrecompiled.h"
#include "EditorManager.h"

#include "gameobject/GameObjectController.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "gameobject/PhysicsPlayerControllerComponent.h"
#include "gameobject/JointComponents.h"
#include "gameobject/PlaneComponent.h"
#include "gameobject/PlanarReflectionComponent.h"
#include "gameobject/DatablockPbsComponent.h"
#include "gameobject/DatablockTerraComponent.h"
#include "gameobject/LightDirectionalComponent.h"
#include "gameobject/LightSpotComponent.h"
#include "gameobject/LightPointComponent.h"
#include "gameobject/LightAreaComponent.h"
#include "gameobject/CameraComponent.h"
#include "gameobject/ReflectionCameraComponent.h"
#include "gameobject/WorkspaceComponents.h"
#include "gameobject/NodeComponent.h"
#include "gameobject/LinesComponent.h"
#include "gameobject/ManualObjectComponent.h"
#include "gameobject/RectangleComponent.h"
#include "gameobject/DecalComponent.h"
#include "gameobject/OceanComponent.h"
#include "gameobject/TerraComponent.h"
#include "utilities/MathHelper.h"
#include "modules/InputDeviceModule.h"
#include "main/Core.h"

#include "modules/DotSceneExportModule.h"
#include "modules/DotSceneImportModule.h"
#include "modules/DeployResourceModule.h"
#include "main/AppStateManager.h"

#include "OgreMeshManager2.h"
#include "OgreConfigFile.h"
#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMeshLodGenerator.h"
#include "OgreMesh2Serializer.h"
#include "OgrePixelCountLodStrategy.h"

#include <limits>

namespace NOWA
{
	// Commands
	class GameObjectManipulationUndoCommand : public ICommand
	{
	public:
		GameObjectManipulationUndoCommand(std::vector<EditorManager::GameObjectData> oldGameObjectDataList, std::vector<unsigned long> gameObjectIds)
			: gameObjectIds(std::move(gameObjectIds)),
			oldGameObjectDataList(std::move(oldGameObjectDataList))
		{

		}

		virtual void undo(void) override
		{
			unsigned int i = 0;
			for (auto it = this->gameObjectIds.begin(); it != this->gameObjectIds.end(); ++it)
			{
				unsigned long id = *it;
				// Its just local here the game object ptr and my not cause any trouble
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(*it);
				if (nullptr == gameObjectPtr)
				{
					continue;
				}
				// Scenario: If in simulation a game object has been deleted (e.g. via lua)
				// with GOC undo, then the game object id, is still present here, so a check must be made, else
				// the wrong GO's would be set undo!
				bool match = true;
				do
				{
					if (this->oldGameObjectDataList[i].gameObjectId != 0 && gameObjectPtr->getId() != this->oldGameObjectDataList[i].gameObjectId)
					{
						i++;
						match = false;
					}
					else
					{
						match = true;
					}
				} while (false == match && i < this->oldGameObjectDataList.size());

				if (false == match)
				{
					continue;
				}

				auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());

				if (nullptr != physicsComponent && nullptr != physicsComponent->getBody())
				{
					this->oldGameObjectDataList[i].newPosition = physicsComponent->getPosition();
					this->oldGameObjectDataList[i].newScale = physicsComponent->getScale();
					this->oldGameObjectDataList[i].newOrientation = physicsComponent->getOrientation();
					physicsComponent->setPosition(this->oldGameObjectDataList[i].oldPosition);
					physicsComponent->setScale(this->oldGameObjectDataList[i].oldScale);
					physicsComponent->setOrientation(this->oldGameObjectDataList[i].oldOrientation);

					// Special case: Reset the internal heading of the player controller, else it will be rotated differently each time, the simulation starts
					auto& physicsPlayerControllerCompPtr = boost::dynamic_pointer_cast<NOWA::PhysicsPlayerControllerComponent>(physicsComponent);
					if (nullptr != physicsPlayerControllerCompPtr)
					{
						physicsPlayerControllerCompPtr->move(0.0f, 0.0f, this->oldGameObjectDataList[i].oldOrientation.getYaw());
					}
				}
				else
				{
					// If there is no physics component set the data directly for the game object scene node
					this->oldGameObjectDataList[i].newPosition = gameObjectPtr->getSceneNode()->getPosition();
					gameObjectPtr->getSceneNode()->setPosition(this->oldGameObjectDataList[i].oldPosition);
					this->oldGameObjectDataList[i].newScale = gameObjectPtr->getSceneNode()->getScale();
					gameObjectPtr->getSceneNode()->setScale(this->oldGameObjectDataList[i].oldScale);
					this->oldGameObjectDataList[i].newOrientation = gameObjectPtr->getSceneNode()->getOrientation();
					gameObjectPtr->getSceneNode()->setOrientation(this->oldGameObjectDataList[i].oldOrientation);
				}
				i++;
			}
		}

		virtual void redo(void)
		{
			unsigned int i = 0;
			for (auto& it = this->gameObjectIds.begin(); it != this->gameObjectIds.end(); ++it)
			{
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(*it);
				if (nullptr == gameObjectPtr)
				{
					continue;
				}

				bool match = true;
				do
				{
					if (this->oldGameObjectDataList[i].gameObjectId != 0 && gameObjectPtr->getId() != this->oldGameObjectDataList[i].gameObjectId)
					{
						i++;
						match = false;
					}
					else
					{
						match = true;
					}
				} while (false == match && i < this->oldGameObjectDataList.size());

				if (false == match)
				{
					continue;
				}

				auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());

				if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
				{
					physicsComponent->setPosition(this->oldGameObjectDataList[i].newPosition);
					physicsComponent->setScale(this->oldGameObjectDataList[i].newScale);
					physicsComponent->setOrientation(this->oldGameObjectDataList[i].newOrientation);

					// Special case: Reset the internal heading of the player controller, else it will be rotated differently each time, the simulation starts
					auto& physicsPlayerControllerCompPtr = boost::dynamic_pointer_cast<NOWA::PhysicsPlayerControllerComponent>(physicsComponent);
					if (nullptr != physicsPlayerControllerCompPtr)
					{
						physicsPlayerControllerCompPtr->move(0.0f, 0.0f, this->oldGameObjectDataList[i].newOrientation.getYaw());
					}
				}
				else
				{
					// If there is no physics component set the data directly for the game object scene node
					gameObjectPtr->getSceneNode()->setPosition(this->oldGameObjectDataList[i].newPosition);
					gameObjectPtr->getSceneNode()->setScale(this->oldGameObjectDataList[i].newScale);
					gameObjectPtr->getSceneNode()->setOrientation(this->oldGameObjectDataList[i].newOrientation);
				}
				i++;
			}
		}
	private:
		std::vector<EditorManager::GameObjectData> oldGameObjectDataList;
		std::vector<unsigned long> gameObjectIds;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class AttributeGameObjectUndoCommand : public ICommand
	{
	public:
		AttributeGameObjectUndoCommand(std::vector<EditorManager::GameObjectData> oldGameObjectDataList)
			: oldGameObjectDataList(std::move(oldGameObjectDataList))
		{

		}

		virtual ~AttributeGameObjectUndoCommand()
		{
			for (size_t i = 0; i < this->oldGameObjectDataList.size(); i++)
			{
				// Pointer data has been cloned in snapshot, so it must be deleted
				if (this->oldGameObjectDataList[i].oldAttribute)
				{
					delete this->oldGameObjectDataList[i].oldAttribute;
					this->oldGameObjectDataList[i].oldAttribute = nullptr;
				}
				if (this->oldGameObjectDataList[i].newAttribute)
				{
					delete this->oldGameObjectDataList[i].newAttribute;
					this->oldGameObjectDataList[i].oldAttribute = nullptr;
				}
			}
		}

		virtual void undo(void) override
		{
			size_t count = oldGameObjectDataList.size();
			for (size_t i = 0; i < count; i++)
			{
				// Get the old attribute of the old game object and set the value
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(oldGameObjectDataList[i].gameObjectId);
				if (nullptr == gameObjectPtr)
				{
					continue;
				}
				gameObjectPtr->actualizeValue(this->oldGameObjectDataList[i].oldAttribute);

				// Special treatment for transform
				if ("Position" == this->oldGameObjectDataList[i].oldAttribute->getName())
				{
					auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
					if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						physicsComponent->setPosition(this->oldGameObjectDataList[i].oldAttribute->getVector3());
					}
				}
				else if ("Scale" == this->oldGameObjectDataList[i].oldAttribute->getName())
				{
					auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
					if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						physicsComponent->setScale(this->oldGameObjectDataList[i].oldAttribute->getVector3());
					}
				}
				else if ("Orientation" == this->oldGameObjectDataList[i].oldAttribute->getName())
				{
					auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
					if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						Ogre::Vector3 degreeOrientation = this->oldGameObjectDataList[i].oldAttribute->getVector3();
						physicsComponent->setOrientation(MathHelper::getInstance()->degreesToQuat(degreeOrientation));
					}
				}
				else if ("Category" == this->oldGameObjectDataList[i].oldAttribute->getName())
				{
					auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
					if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						physicsComponent->getBody()->setType(this->oldGameObjectDataList[i].oldAttribute->getUInt());
					}
				}
			}
		}

		virtual void redo(void)
		{
			// For redo its different for transform as for undo. For redo, the relative data must be set, because in the newAttribute, the relative values are stored
			// and for undo, the current old absolute transform for each object
			size_t count = oldGameObjectDataList.size();
			for (size_t i = 0; i < count; i++)
			{
				// Get the new attribute of the new game object and set the value
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(oldGameObjectDataList[i].gameObjectId);
				if (nullptr == gameObjectPtr)
				{
					continue;
				}
				gameObjectPtr->actualizeValue(this->oldGameObjectDataList[i].newAttribute);

				// Special treatment for transform
				if ("Position" == this->oldGameObjectDataList[i].newAttribute->getName())
				{
					auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
					if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						// if its just for one game object, set position absolute, else translate because for several setting new value, translation is done
						if (1 == count)
						{
							physicsComponent->setPosition(this->oldGameObjectDataList[i].newAttribute->getVector3());
						}
						else
						{
							physicsComponent->translate(this->oldGameObjectDataList[i].newAttribute->getVector3());
						}
					}
				}
				else if ("Scale" == this->oldGameObjectDataList[i].newAttribute->getName())
				{
					auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
					if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						if (1 == count)
						{
							physicsComponent->setScale(this->oldGameObjectDataList[i].newAttribute->getVector3());
						}
						else
						{
							physicsComponent->setScale(physicsComponent->getScale() + this->oldGameObjectDataList[i].newAttribute->getVector3());
						}
					}
				}
				else if ("Orientation" == this->oldGameObjectDataList[i].newAttribute->getName())
				{
					auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
					if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						Ogre::Vector3 degreeOrientation = this->oldGameObjectDataList[i].newAttribute->getVector3();
						if (1 == count)
						{
							physicsComponent->setOrientation(MathHelper::getInstance()->degreesToQuat(degreeOrientation));
						}
						else
						{
							physicsComponent->rotate(MathHelper::getInstance()->degreesToQuat(degreeOrientation));
						}
					}
				}
				else if ("Category" == this->oldGameObjectDataList[i].oldAttribute->getName())
				{
					auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
					if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						physicsComponent->getBody()->setType(this->oldGameObjectDataList[i].newAttribute->getUInt());
					}
				}
			}
		}
	private:
		std::vector<EditorManager::GameObjectData> oldGameObjectDataList;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Attention: What if via delete command the game object is deleted, is this game object component still valid? Maybe I need also Ids to get the GameObjectComponent from the ID?
	class AttributeGameObjectComponentUndoCommand : public ICommand
	{
	public:
		AttributeGameObjectComponentUndoCommand(std::vector<EditorManager::GameObjectData> oldGameObjectDataList)
			: oldGameObjectDataList(std::move(oldGameObjectDataList))
		{

		}

		virtual ~AttributeGameObjectComponentUndoCommand()
		{
			// Pointer data has been cloned in snapshot, so it must be deleted
			for (size_t i = 0; i < this->oldGameObjectDataList.size(); i++)
			{
				// Pointer data has been cloned in snapshot, so it must be deleted
				if (this->oldGameObjectDataList[i].oldAttribute)
				{
					delete this->oldGameObjectDataList[i].oldAttribute;
					this->oldGameObjectDataList[i].oldAttribute = nullptr;
				}
				if (this->oldGameObjectDataList[i].newAttribute)
				{
					delete this->oldGameObjectDataList[i].newAttribute;
					this->oldGameObjectDataList[i].oldAttribute = nullptr;
				}
			}
		}

		virtual void undo(void) override
		{
			// Get the old attribute of the old game object and set the value
			size_t count = oldGameObjectDataList.size();
			for (size_t i = 0; i < count; i++)
			{
				// Get the old attribute of the old game object and set the value
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(oldGameObjectDataList[i].gameObjectId);
				if (nullptr == gameObjectPtr)
				{
					continue;
				}

				auto& gameObjectCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponentByIndex(this->oldGameObjectDataList[i].componentIndex));
				if (nullptr != gameObjectCompPtr)
				{
					gameObjectCompPtr->actualizeValue(this->oldGameObjectDataList[i].oldAttribute);
				}
			}
		}

		virtual void redo(void)
		{
			// Get the new attribute of the new game object and set the value
			size_t count = oldGameObjectDataList.size();
			for (size_t i = 0; i < count; i++)
			{
				// Get the old attribute of the old game object and set the value
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(oldGameObjectDataList[i].gameObjectId);
				if (nullptr == gameObjectPtr)
				{
					continue;
				}
				auto& gameObjectCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponentByIndex(this->oldGameObjectDataList[i].componentIndex));
				if (nullptr != gameObjectCompPtr)
				{
					gameObjectCompPtr->actualizeValue(this->oldGameObjectDataList[i].newAttribute);
				}
			}
		}
	private:
		std::vector<EditorManager::GameObjectData> oldGameObjectDataList;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class AddGameObjectUndoCommand : public ICommand
	{
	public:

		AddGameObjectUndoCommand(Ogre::SceneManager* sceneManager, Ogre::SceneNode* placeNode, std::vector<Ogre::String> meshData, GameObject::eType type)
			: sceneManager(sceneManager),
			position(placeNode->getPosition()),
			scale(placeNode->getScale()),
			orientation(placeNode->getOrientation()),
			meshData(meshData),
			type(type),
			objectNode(nullptr),
			id(0)
		{
			this->redo();
		}

		virtual ~AddGameObjectUndoCommand()
		{

		}

		virtual void undo(void) override
		{
			if (0 != this->id)
			{
				AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(id);
			}
		}

		virtual void redo(void) override
		{
			// Create custom scenenode to store temporary data
			this->objectNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);

			if (GameObject::OCEAN == this->type || GameObject::TERRA == this->type)
			{
				this->position = Ogre::Vector3::ZERO;
				this->orientation = Ogre::Quaternion::IDENTITY;
				this->scale = Ogre::Vector3::UNIT_SCALE;
			}

			this->objectNode->setPosition(this->position);
			this->objectNode->setOrientation(this->orientation);
			this->objectNode->setScale(this->scale);
			// this->sceneManager->findMovableObjects

			// Only create id once, so that when undo, redo, the id is the same
			if (0 == this->id)
			{
				this->id = NOWA::makeUniqueID();
			}

			Ogre::String meshName;
			Ogre::MovableObject* newMovableObject = nullptr;

			if (GameObject::OCEAN != this->type && GameObject::TERRA != this->type && GameObject::DECAL != this->type
				&& GameObject::LIGHT_AREA != this->type)
			{
				meshName = this->meshData[0];

				if (GameObject::ITEM == this->type || GameObject::PLANE == this->type)
				{
					Ogre::Item* newItem = this->sceneManager->createItem(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::SCENE_STATIC);
					this->objectNode->attachObject(newItem);

					newMovableObject = newItem;

					// Copy the datablocks from place entity to new one
					for (size_t i = 1; i < this->meshData.size(); i++)
					{
						newItem->getSubItem(i - 1)->setDatablock(this->meshData[i]);
					}
				}
				else
				{
					Ogre::v1::Entity* newEntity = this->sceneManager->createEntity(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::SCENE_STATIC);
					this->objectNode->attachObject(newEntity);

					newMovableObject = newEntity;

					// Copy the datablocks from place entity to new one
					for (size_t i = 1; i < this->meshData.size(); i++)
					{
						newEntity->getSubEntity(i - 1)->setDatablock(this->meshData[i]);
					}
				}

				// Get name without .mesh
				size_t found = meshName.find(".");
				if (Ogre::String::npos != found)
				{
					meshName = meshName.substr(0, found);
				}
			}
			else
			{
				if (GameObject::OCEAN == this->type)
					meshName = "Ocean";
				else if (GameObject::TERRA == this->type)
					meshName = "Terra";
				else if (GameObject::LIGHT_AREA == this->type)
					meshName = "LightArea";
				else if (GameObject::DECAL == this->type)
					meshName = "Decal";
			}

			// Do not use # anymore, because its reserved in mygui as code-word the # and everything after that will be removed!
			Ogre::String gameObjectName = meshName + "_0";
			AppStateManager::getSingletonPtr()->getGameObjectController()->getValidatedGameObjectName(gameObjectName);

			if (GameObject::OCEAN != this->type && GameObject::TERRA != this->type && GameObject::DECAL != this->type
				&& GameObject::LIGHT_AREA != this->type)
			{
				newMovableObject->setName(gameObjectName);
			}

			this->objectNode->setName(gameObjectName);
			GameObjectPtr gameObjectPtr = GameObjectFactory::getInstance()->createGameObject(this->sceneManager, this->objectNode, newMovableObject, this->type, this->id);
			if (nullptr != gameObjectPtr)
			{
				if (GameObject::PLANE == this->type)
				{
					// Add the plane component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, DatablockPbsComponent::getStaticClassName());
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, PlaneComponent::getStaticClassName());
				}
				else if (GameObject::MIRROR == this->type)
				{
					// Add the planar reflection component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, PlanarReflectionComponent::getStaticClassName());
				}
				else if (GameObject::LIGHT_DIRECTIONAL == this->type)
				{
					// Add the light direcitional component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, LightDirectionalComponent::getStaticClassName());
				}
				else if (GameObject::LIGHT_SPOT == this->type)
				{
					// Add the light spot component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, LightSpotComponent::getStaticClassName());
				}
				else if (GameObject::LIGHT_POINT == this->type)
				{
					// Add the light point component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, LightPointComponent::getStaticClassName());
				}
				else if (GameObject::LIGHT_AREA == this->type)
				{
					// Add the light area component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, LightAreaComponent::getStaticClassName());
				}
				else if (GameObject::CAMERA == this->type)
				{
					// This is required, because when a camera is created via the editor, it must be placed where placenode has been when the user clicked the mouse button
					// But when a camera is loaded from world, it must not have an orientation, else there are ugly side effects
					CameraComponent::setJustCreated(true);
					// Add the camera component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, CameraComponent::getStaticClassName());
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, WorkspacePbsComponent::getStaticClassName());

					CameraComponent::setJustCreated(true);
				}
				else if (GameObject::REFLECTION_CAMERA == this->type)
				{
					// Add the camera component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, ReflectionCameraComponent::getStaticClassName());
				}
				else if (GameObject::SCENE_NODE == this->type)
				{
					// Add the scene node component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, NodeComponent::getStaticClassName());
				}
				else if (GameObject::LINES == this->type)
				{
					// Add the lines component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, LinesComponent::getStaticClassName());
				}
				else if (GameObject::MANUAL_OBJECT == this->type)
				{
					// Add the manual object component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, ManualObjectComponent::getStaticClassName());
				}
				else if (GameObject::RECTANGLE == this->type)
				{
					// Add the manual object component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, RectangleComponent::getStaticClassName());
				}
				else if (GameObject::DECAL == this->type)
				{
					// Add the scene decal component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, DecalComponent::getStaticClassName());
					// NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, PlaneComponent::getStaticClassName());
				}
				else if (GameObject::OCEAN == this->type)
				{
					// Add the ocean component
					// NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, OceanComponent::getStaticClassName());
				}
				else if (GameObject::TERRA == this->type)
				{
					// Add the terra component
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, TerraComponent::getStaticClassName());
					NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, DatablockTerraComponent::getStaticClassName());
				}
				// Register after the component has been created
				AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
			}
		}

	private:
		Ogre::SceneManager* sceneManager;
		Ogre::SceneNode* objectNode;
		Ogre::Vector3 position;
		Ogre::Vector3 scale;
		Ogre::Quaternion orientation;
		std::vector<Ogre::String> meshData;
		GameObject::eType type;
		unsigned long id;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class AddComponentUndoCommand : public ICommand
	{
	public:

		AddComponentUndoCommand(std::vector<unsigned long> gameObjectIds, const Ogre::String& componentClassName)
			: gameObjectIds(gameObjectIds),
			componentClassName(componentClassName)
		{
			this->componentsOccurrenceIndex.resize(this->gameObjectIds.size());
			this->redo();
		}

		virtual ~AddComponentUndoCommand()
		{

		}

		virtual void undo(void) override
		{
			for (size_t i = 0; i < this->gameObjectIds.size(); i++)
			{
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectIds[i]);
				if (nullptr != gameObjectPtr)
				{
					gameObjectPtr->deleteComponent(this->componentClassName, this->componentsOccurrenceIndex[i]);
				}
			}
		}

		virtual void redo(void) override
		{
			for (size_t i = 0; i < this->gameObjectIds.size(); i++)
			{
				// Ptr's are required so get from the internal game objects id the game object ptr
				NOWA::GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectIds[i]);
				if (nullptr != gameObjectPtr)
				{
					// No post init etc. necessary, since its done inside createComponent
					auto& gameObjectCompPtr = GameObjectFactory::getInstance()->createComponent(gameObjectPtr, this->componentClassName);
					if (nullptr != gameObjectCompPtr)
					{
						// Save the occurrence index for deletion
						this->componentsOccurrenceIndex[i] = gameObjectCompPtr->getOccurrenceIndex();
						// If its e.g. the buoyancy component, change the current category name to the required one
						/*if (BuoyancyComponent::getStaticClassName() == this->componentClassName)
						{
							gameObjectPtr->changeCategory(gameObjectPtr->getCategory(), BuoyancyComponent::getStaticRequiredCategory());
						}*/
						/*else if (PhysicsAttractorComponent::getStaticClassName() == this->componentClassName)
						{
							gameObjectPtr->changeCategory(gameObjectPtr->getCategory(), PhysicsAttractorComponent::getStaticRequiredCategory());
						}*/
					}
				}
			}
		}

	private:
		std::vector<unsigned long> gameObjectIds;
		Ogre::String componentClassName;
		std::vector<unsigned int> componentsOccurrenceIndex;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class DeleteComponentUndoCommand : public ICommand
	{
	public:

		DeleteComponentUndoCommand(std::vector<unsigned long> gameObjectIds, unsigned int index)
			: gameObjectIds(gameObjectIds),
			index(index)
		{
			this->componentsToAddStream.resize(this->gameObjectIds.size());
			this->deleteComponent();
		}

		virtual ~DeleteComponentUndoCommand()
		{

		}

		virtual void undo(void) override
		{
			this->createComponent();
		}

		virtual void redo(void) override
		{
			this->deleteComponent();
		}
	private:
		void deleteComponent(void)
		{
			for (size_t i = 0; i < this->gameObjectIds.size(); i++)
			{
				// Add stream for each game objects component, because each component can have different values, when being deleted, and when restored, those values must be restored
				rapidxml::xml_document<> doc;
				// xml_node<>* userDataXML = doc.allocate_node(node_element, "userData");
				rapidxml::xml_node<>* propertyXML = doc.allocate_node(rapidxml::node_element, "userData");

				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectIds[i]);
				if (nullptr != gameObjectPtr)
				{
					// Write component data
					auto& gameObjectCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponentByIndex(this->index));
					if (nullptr != gameObjectCompPtr)
					{
						gameObjectCompPtr->writeXML(propertyXML, doc, "");
						gameObjectPtr->deleteComponentByIndex(this->index);
					}
				}

				doc.append_node(propertyXML);

				std::ostrstream out;
				out << doc;
				out << '\0';
				// Set the stream
				this->componentsToAddStream[i] = out.str();
			}

		}

		void createComponent(void)
		{
			// For collision serializaion
			Ogre::String fileName = Core::getSingletonPtr()->getSectionPath("Projects")[0];

			for (size_t i = 0; i < this->gameObjectIds.size(); i++)
			{
				rapidxml::xml_document<> XMLDoc;

				// There are x-streams for x-components, because it may be that a user deleted the component of x-game objects, so that data must be restored again
				std::vector<char> sceneCopy(this->componentsToAddStream[i].begin(), this->componentsToAddStream[i].end());
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

				rapidxml::xml_node<>* userDataElement = XMLDoc.first_node("userData");
				rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");
				if (nullptr != propertyElement)
				{
					GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectIds[i]);
					if (nullptr != gameObjectPtr)
					{
						// Component is created from xml, but post init is not done
						GameObjectCompPtr componentPtr = GameObjectFactory::getInstance()->createComponent(propertyElement, fileName, gameObjectPtr);
						if (componentPtr)
						{

							// Add circular association in order, that a component can call the owner gameobject and over that, another component to use its functionality
							componentPtr->setOwner(gameObjectPtr);
							gameObjectPtr->insertComponent(componentPtr, this->index);

							// Do not forget to post init the component, so that necessary data like collision hull will be created
							if (!componentPtr->postInit())
							{
								Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeleteComponentUndoCommand]: Error: Could not post init component during undo: "
									+ componentPtr->getClassName() + " for game object: " + gameObjectPtr->getName());
							}
						}
					}
				}
			}
		}
	private:
		std::vector<unsigned long> gameObjectIds;
		unsigned int index;
		std::vector<Ogre::String> componentsToAddStream;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CloneGameObjectGroupUndoCommand : public ICommand
	{
	public:

		CloneGameObjectGroupUndoCommand(EditorManager* editorManager, std::vector<unsigned long> gameObjectIds)
			: editorManager(editorManager),
			gameObjectIds(gameObjectIds),
			dotSceneExportModule(new DotSceneExportModule()),
			dotSceneImportModule(new DotSceneImportModule(editorManager->getSceneManager()))
		{
			// Resize with zero, because the id's will be filled later
			this->gameObjectClonedIds.resize(this->gameObjectIds.size(), 0);
			this->redo();
		}

		virtual ~CloneGameObjectGroupUndoCommand()
		{

		}

		virtual void undo(void) override
		{
			AppStateManager::getSingletonPtr()->getGameObjectController()->stop();

			rapidxml::xml_document<> doc;
			rapidxml::xml_node<>* nodesXML = doc.allocate_node(rapidxml::node_element, "nodes");
			for (size_t i = 0; i < this->gameObjectIds.size(); i++)
			{
				// Store the cloned game objects in stream and delete them from scene
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectClonedIds[i]);
				if (nullptr != gameObjectPtr)
				{
					// Write all the game object data to stream
					this->dotSceneExportModule->exportNode(gameObjectPtr->getSceneNode(), nodesXML, doc, true, "", false);
					AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObjectPtr);
				}
				else
				{
					// If the to be cloned game object does no more exist, e.g. prior deleted, cloning is no more possible
					this->gameObjectIds.clear();
					this->gameObjectClonedIds.clear();
					break;
				}
			}
			doc.append_node(nodesXML);

			std::ostrstream out;
			out << doc;
			out << '\0';
			// Set the stream
			this->gameObjectsToAddStream = out.str();
		}

		virtual void redo(void) override
		{
			this->editorManager->getSelectionManager()->clearSelection();
			// First disconnect game objects, because during clone, when there are joint components etc. only those to be cloned joint components may be in a list
			// to find the new predecessor. If there would be all other, a wrong predecessor joint could be found!
			AppStateManager::getSingletonPtr()->getGameObjectController()->stop();

			if (false == this->gameObjectsToAddStream.empty())
			{
				// If the cloned game objects had been deleted, their ids are also gone, so get them from stream
				this->createGameObjects();
			}
			else
			{
				for (size_t i = 0; i < this->gameObjectIds.size(); i++)
				{
					// Clone with the clone target id to get the same game object id, as the one that had been deleted in undo
					GameObjectPtr clonedGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->clone(this->gameObjectIds[i], nullptr, this->gameObjectClonedIds[i]);
					if (nullptr != clonedGameObject)
					{
						this->editorManager->getSelectionManager()->select(clonedGameObject->getId());
						// Important: Set the cloned name, for undo redo
						this->gameObjectClonedIds[i] = clonedGameObject->getId();
					}
					else
					{
						// If the to be cloned game object does no more exist, e.g. prior deleted, cloning is no more possible
						this->gameObjectIds.clear();
						this->gameObjectClonedIds.clear();
						break;
					}
				}
			}

			// Connect just after cloning, so that onCloned and connect is called. E.g. for joints its necessary to call it right after cloning, so that 
			// the correct predecssor via priorId can be found and set to another id, so that when cloned a second time, it will not be found again!
			// But only for the cloned ones
			// Internally game object component can react on onCloned method and search for its target game object that has been cloned by its prior id
			AppStateManager::getSingletonPtr()->getGameObjectController()->connectClonedGameObjects(this->gameObjectClonedIds);
		}

		void createGameObjects(void)
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
		}
	private:
		EditorManager* editorManager;
		std::vector<unsigned long> gameObjectIds;
		std::vector<unsigned long> gameObjectClonedIds;
		Ogre::String gameObjectsToAddStream;
		DotSceneExportModule* dotSceneExportModule;
		DotSceneImportModule* dotSceneImportModule;
	};

	class CloneGameObjectsUndoCommand : public ICommand
	{
	public:

		CloneGameObjectsUndoCommand(EditorManager* editorManager, std::vector<unsigned long> gameObjectIds)
			: editorManager(editorManager),
			gameObjectIds(gameObjectIds)
		{
			// Resize with zero, because the id's will be filled later
			this->gameObjectClonedIds.resize(this->gameObjectIds.size(), 0);
			this->redo();
		}

		virtual ~CloneGameObjectsUndoCommand()
		{

		}

		virtual void undo(void) override
		{
			AppStateManager::getSingletonPtr()->getGameObjectController()->stop();

			for (size_t i = 0; i < this->gameObjectIds.size(); i++)
			{
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->gameObjectClonedIds[i]);
				if (nullptr != gameObjectPtr)
				{
					AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObjectPtr);
				}
				else
				{
					// If the to be cloned game object does no more exist, e.g. prior deleted, cloning is no more possible
					this->gameObjectIds.clear();
					this->gameObjectClonedIds.clear();
					break;
				}
			}
		}

		virtual void redo(void) override
		{
			this->editorManager->getSelectionManager()->clearSelection();
			// First disconnect game objects, because during clone, when there are joint components etc. only those to be cloned joint components may be in a list
			// to find the new predecessor. If there would be all other, a wrong predecessor joint could be found!
			AppStateManager::getSingletonPtr()->getGameObjectController()->stop();
			for (size_t i = 0; i < this->gameObjectIds.size(); i++)
			{
				// Clone with the clone target id to get the same game object id, as the one that had been deleted in undo
				GameObjectPtr clonedGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->clone(this->gameObjectIds[i], nullptr, this->gameObjectClonedIds[i]);
				if (nullptr != clonedGameObject)
				{
					this->editorManager->getSelectionManager()->select(clonedGameObject->getId());
					// Important: Set the cloned name, for undo redo
					this->gameObjectClonedIds[i] = clonedGameObject->getId();
				}
				else
				{
					// If the to be cloned game object does no more exist, e.g. prior deleted, cloning is no more possible
					this->gameObjectIds.clear();
					this->gameObjectClonedIds.clear();
					break;
				}
			}
			// Connect just after cloning, so that onCloned and connect is called. E.g. for joints its necessary to call it right after cloning, so that 
			// the correct predecssor via priorId can be found and set to another id, so that when cloned a second time, it will not be found again!
			// But only for the cloned ones
			// Internally game object component can react on onCloned method and search for its target game object that has been cloned by its prior id
			AppStateManager::getSingletonPtr()->getGameObjectController()->connectClonedGameObjects(this->gameObjectClonedIds);

			this->editorManager->setManipulationMode(EditorManager::EDITOR_TRANSLATE_MODE);
			this->editorManager->setGizmoToGameObjectsCenter();
		}
	private:
		EditorManager* editorManager;
		std::vector<unsigned long> gameObjectIds;
		std::vector<unsigned long> gameObjectClonedIds;
		bool changeToTranslateMode;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CameraPositionUndoCommand : public ICommand
	{
	public:
		CameraPositionUndoCommand(Ogre::Camera* camera)
			: camera(camera),
			oldPosition(camera->getPositionForViewUpdate()),
			newPosition(Ogre::Vector3::ZERO)
		{

		}

		virtual void undo(void) override
		{
			this->newPosition = this->camera->getPosition();
			this->camera->setPosition(this->oldPosition);
		}

		virtual void redo(void) override
		{
			this->camera->setPosition(this->newPosition);
		}
	private:
		Ogre::Vector3 newPosition;
		Ogre::Vector3 oldPosition;
		Ogre::Camera* camera;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CameraOrientationUndoCommand : public ICommand
	{
	public:
		CameraOrientationUndoCommand(Ogre::Camera* camera)
			: camera(camera),
			oldOrientation(camera->getOrientationForViewUpdate()),
			newOrientation(Ogre::Quaternion::IDENTITY)
		{

		}

		virtual void undo(void) override
		{
			this->newOrientation = this->camera->getOrientation();
			this->camera->setOrientation(this->oldOrientation);
		}

		virtual void redo(void) override
		{
			this->camera->setOrientation(this->newOrientation);
		}
	private:
		Ogre::Quaternion oldOrientation;
		Ogre::Quaternion newOrientation;
		Ogre::Camera* camera;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class CameraTransformUndoCommand : public ICommand
	{
	public:
		CameraTransformUndoCommand(Ogre::Camera* camera)
			: camera(camera),
			oldPosition(camera->getPositionForViewUpdate()),
			newPosition(Ogre::Vector3::ZERO),
			oldOrientation(camera->getOrientationForViewUpdate()),
			newOrientation(Ogre::Quaternion::IDENTITY)
		{

		}

		virtual void undo(void) override
		{
			this->newPosition = this->camera->getPosition();
			this->newOrientation = this->camera->getOrientation();
			this->camera->setPosition(this->oldPosition);
			this->camera->setOrientation(this->oldOrientation);
		}

		virtual void redo(void) override
		{
			this->camera->setPosition(this->newPosition);
			this->camera->setOrientation(this->newOrientation);
		}
	private:
		Ogre::Camera* camera;
		Ogre::Vector3 newPosition;
		Ogre::Vector3 oldPosition;
		Ogre::Quaternion oldOrientation;
		Ogre::Quaternion newOrientation;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class TerrainModifyUndoCommand : public ICommand
	{
	public:
		TerrainModifyUndoCommand(const std::vector<Ogre::uint16>& oldHeightData, const std::vector<Ogre::uint16>& newHeightData, TerraComponent* terraComponent)
			: oldHeightData(oldHeightData),
			newHeightData(newHeightData),
			terraComponent(terraComponent)
		{

		}

		virtual void undo(void) override
		{
			this->terraComponent->setHeightData(this->oldHeightData);
		}

		virtual void redo(void) override
		{
			this->terraComponent->setHeightData(this->newHeightData);
		}
	private:
		std::vector<Ogre::uint16> oldHeightData;
		std::vector<Ogre::uint16> newHeightData;
		TerraComponent* terraComponent;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class TerrainPaintUndoCommand : public ICommand
	{
	public:
		TerrainPaintUndoCommand(const std::vector<Ogre::uint8>& oldBlendWeightData, const std::vector<Ogre::uint8>& newBlendWeightData, TerraComponent* terraComponent)
			: oldBlendWeightData(oldBlendWeightData),
			newBlendWeightData(newBlendWeightData),
			terraComponent(terraComponent)
		{

		}

		virtual void undo(void) override
		{
			this->terraComponent->setBlendWeightData(this->oldBlendWeightData);
		}

		virtual void redo(void) override
		{
			this->terraComponent->setBlendWeightData(this->newBlendWeightData);
		}
	private:
		std::vector<Ogre::uint8> oldBlendWeightData;
		std::vector<Ogre::uint8> newBlendWeightData;
		TerraComponent* terraComponent;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	EditorManager::EditorManager()
		: sceneManager(nullptr),
		camera(nullptr),
		mouseButtonId(OIS::MB_Left),
		gizmo(nullptr),
		viewportGrid(nullptr),
		selectionManager(nullptr),
		placeNode(nullptr),
		tempPlaceMovableNode(nullptr),
		tempPlaceMovableObject(nullptr),
		gizmoQuery(nullptr),
		placeObjectQuery(nullptr),
		toBePlacedObjectQuery(nullptr),
		isGizmoMoving(false),
		mouseIdPressed(false),
		manipulationMode(EDITOR_NO_MODE),
		placeMode(EDITOR_PLACE_MODE_STACK),
		translateMode(EDITOR_TRANSLATE_MODE_NORMAL),
		cameraView(EDITOR_CAMERA_VIEW_FRONT),
		hitPoint(Ogre::Vector3::ZERO),
		oldHitPoint(Ogre::Vector3::ZERO),
		gridStep(0.0f),
		cumulatedPlaceNodeTranslationDistance(0.0f),
		startHitPoint(Ogre::Vector3::ZERO),
		translateStartPoint(Ogre::Vector3::ZERO),
		rotateStartOrientation(Ogre::Quaternion::IDENTITY),
		rotateStartOrientationGizmoX(Ogre::Quaternion::IDENTITY),
		rotateStartOrientationGizmoY(Ogre::Quaternion::IDENTITY),
		rotateStartOrientationGizmoZ(Ogre::Quaternion::IDENTITY),
		oldGizmoOrientation(Ogre::Quaternion::IDENTITY),
		absoluteAngle(0.0f),
		stepAngleDelta(0.0f),
		useUndoRedoForPicker(true),
		movePicker(nullptr),
		movePicker2(nullptr),
		timeSinceLastUpdate(1.0f),
		currentPlaceType(GameObject::NONE),
		pickForce(20.0f),
		constraintAxis(Ogre::Vector3::ZERO),
		isInSimulation(false),
		rotateFactor(0.0f),
		terraComponent(nullptr),
		boundingBoxSize(Ogre::Vector3::ZERO)
	{

	}

	EditorManager::~EditorManager()
	{
		if (this->gizmo != nullptr)
		{
			delete this->gizmo;
			this->gizmo = nullptr;
		}
		if (this->viewportGrid != nullptr)
		{
			delete this->viewportGrid;
			this->viewportGrid = nullptr;
		}

		if (nullptr != this->selectionManager)
		{
			delete this->selectionManager;
			this->selectionManager = nullptr;
		}
		if (nullptr != this->placeNode)
		{
			this->sceneManager->destroySceneNode(this->placeNode);
			this->placeNode = nullptr;

		}
		this->destroyTempPlaceMovableObjectNode();
		if (this->movePicker != nullptr)
		{
			delete this->movePicker;
			this->movePicker = nullptr;
		}
		if (this->movePicker2 != nullptr)
		{
			delete this->movePicker2;
			this->movePicker2 = nullptr;
		}
		this->sceneManager->destroyQuery(this->gizmoQuery);
		this->sceneManager->destroyQuery(this->placeObjectQuery);
		this->sceneManager->destroyQuery(this->toBePlacedObjectQuery);
		this->gizmoQuery = nullptr;
		this->placeObjectQuery = nullptr;
		this->toBePlacedObjectQuery = nullptr;
		this->terraComponent = nullptr;
	}

	void EditorManager::init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::String& categories, OIS::MouseButtonID mouseButtonId,
		SelectionManager::ISelectionObserver* selectionObserver, Ogre::Real thickness,
		const Ogre::String& materialNameX, const Ogre::String& materialNameY, const Ogre::String& materialNameZ, const Ogre::String& materialNameHighlight,
		const Ogre::String& materialNameSelection)
	{
		this->sceneManager = sceneManager;
		this->camera = camera;
		this->mouseButtonId = mouseButtonId;

		AppStateManager::getSingletonPtr()->getGameObjectController()->registerCategory("Default");

		this->gizmo = new Gizmo();
		this->gizmo->init(this->sceneManager, this->camera, thickness, materialNameX, materialNameY, materialNameZ, materialNameHighlight);

		this->gizmoQuery = this->sceneManager->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
		this->gizmoQuery->setSortByDistance(true);

		// Generate query for place object (stack mode) for all kinds of categories
		this->placeObjectQuery = this->sceneManager->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
		this->placeObjectQuery->setSortByDistance(true);

		this->toBePlacedObjectQuery = this->sceneManager->createRayQuery(Ogre::Ray(), AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId("Default"));
		this->toBePlacedObjectQuery->setSortByDistance(true);

		this->selectionManager = new SelectionManager();
		this->placeNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC, Ogre::Vector3(0.0f, 20.0f, 0.0f));
		this->placeNode->setName("PlaceNode");
		this->placeNode->setVisible(false);

		this->selectionManager->init(this->sceneManager, this->camera, categories, mouseButtonId, selectionObserver, materialNameSelection);

		this->viewportGrid = new ViewportGrid(this->sceneManager, this->camera);
		this->viewportGrid->setEnabled(false);

		this->movePicker = new KinematicPicker();
		this->movePicker->init(this->sceneManager, this->camera, 100, NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories));

		this->movePicker2 = new Picker();
		this->movePicker2->init(this->sceneManager, this->camera, 100, NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories), true);

		// Grid1: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Peter%27s+Grid+System&highlight=material
		// Grid2: http://www.ogre3d.org/tikiwiki/EditorGridSystem
		// Imgizmo video: https://github.com/CedricGuillemet/ImGuizmo
		// arc formula: https://www.allegro.cc/forums/thread/594175/713916
	}

	bool EditorManager::handleKeyPress(const OIS::KeyEvent& keyEventRef)
	{
		bool handled = false;
		// Do not set to grid mode if user does some clipboard operation
		if (GetAsyncKeyState(VK_LMENU))
		{
			// hier merken welche gridstep zuletzt eingestellt war
			if (EDITOR_ROTATE_MODE1 == this->manipulationMode || EDITOR_ROTATE_MODE2 == this->manipulationMode)
			{
				this->gridStep = 5.0f;
			}
			else
			{
				this->gridStep = 1.0f;
			}
			// this->viewportGrid->setPerspectiveSize(this->gridStep);
			handled = true;
		}
		else if (OIS::KC_ESCAPE == keyEventRef.key)
		{
			// When pressing escape set the place entity visible false or hide the gizmo
			if (EDITOR_PLACE_MODE == this->manipulationMode)
			{
				handled = true;
				this->deactivatePlaceMode();
				this->manipulationMode = EDITOR_SELECT_MODE;
			}
			else if (EDITOR_TRANSLATE_MODE == this->manipulationMode || EDITOR_ROTATE_MODE1 == this->manipulationMode
				|| EDITOR_ROTATE_MODE2 == this->manipulationMode || EDITOR_SCALE_MODE == this->manipulationMode)
			{
				handled = true;
				this->manipulationMode = EDITOR_SELECT_MODE;
				this->gizmo->setEnabled(false);
				// Here send event, that select mode is active! Create just one event with the mode! to handle all other checks to
			}
			boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(this->manipulationMode));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
		}

		if (false == this->isInSimulation)
		{
			this->selectionManager->handleKeyPress(keyEventRef);
		}

		return handled;
	}

	void EditorManager::handleKeyRelease(const OIS::KeyEvent& keyEventRef)
	{
		if (false == this->isInSimulation)
		{
			this->selectionManager->handleKeyRelease(keyEventRef);
		}
		this->gridStep = 0.0f;
	}

	void EditorManager::handleMouseMove(const OIS::MouseEvent& evt)
	{
		this->selectionManager->isSelecting = false;

		Ogre::Real x = 0.0f;
		Ogre::Real y = 0.0f;
		MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());
		this->mouseHitRay = this->camera->getCameraToViewportRay(x, y);

		if (EDITOR_PLACE_MODE == this->manipulationMode)
		{
			// Handle here, how to place objects
			this->movePlaceNode(this->mouseHitRay);
		}
		else
		{
			if (true == this->mouseIdPressed && EDITOR_SELECT_MODE <= this->manipulationMode && EDITOR_ROTATE_MODE2 >= this->manipulationMode
				&& Gizmo::GIZMO_NONE == this->gizmo->getState() && false == this->isGizmoMoving && EDITOR_PICKER_MODE != this->manipulationMode
				&& false == this->isInSimulation)
			{
				this->selectionManager->handleMouseMove(evt);
			}

			// Select the gizmo arrow depending which the mouse hit
			if (EDITOR_SELECT_MODE < this->manipulationMode && EDITOR_ROTATE_MODE2 >= this->manipulationMode && false == this->selectionManager->getIsSelectingRectangle()
				&& true == this->gizmo->isEnabled())
			{
				this->selectGizmo(evt, this->mouseHitRay);
			}

			// manipulate objects
			if (true == this->mouseIdPressed && EDITOR_PICKER_MODE == this->manipulationMode && false == this->selectionManager->getIsSelectingRectangle())
			{
				// if (this->getSelectionManager()->getSelectedGameObjects().size() == 1)
				{
					// Only pull if its a kind of physics object
					// auto& physicsComponent = NOWA::makeStrongPtr(this->getSelectionManager()->getSelectedGameObjects().begin()->second.gameObject->getComponent<NOWA::PhysicsComponent>());
					// if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
					{
						this->pickForce = static_cast<int>(this->pickForce);
						if (this->pickForce < 1.0f)
							this->pickForce = 1.0f;
						else if (this->pickForce > 50.0f)
							this->pickForce = 50.0f;

	
						if (GetAsyncKeyState(VK_LCONTROL))
						{
							NOWA::PhysicsComponent* component = this->movePicker2->grab(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt(),
								Ogre::Vector2(static_cast<Ogre::Real>(evt.state.X.abs), static_cast<Ogre::Real>(evt.state.Y.abs)),
								Core::getSingletonPtr()->getOgreRenderWindow(), this->pickForce);
						}
						else
						{
							NOWA::PhysicsComponent* component = this->movePicker->grab(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt(),
								Ogre::Vector2(static_cast<Ogre::Real>(evt.state.X.abs), static_cast<Ogre::Real>(evt.state.Y.abs)),
								Core::getSingletonPtr()->getOgreRenderWindow());
						}
					}
				}
			}
			else if (true == this->mouseIdPressed && Gizmo::GIZMO_NONE < this->gizmo->getState() && false == this->selectionManager->getIsSelectingRectangle()
				&& EDITOR_SELECT_MODE < this->manipulationMode && EDITOR_ROTATE_MODE2 >= this->manipulationMode)
			{
				this->manipulateObjects(this->mouseHitRay);
			}
		}
	}

	void EditorManager::handleMousePress(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		Ogre::Real x = 0.0f;
		Ogre::Real y = 0.0f;
		MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());
		Ogre::Ray hitRay = this->camera->getCameraToViewportRay(x, y);

		if (id == this->mouseButtonId && EDITOR_PLACE_MODE == this->manipulationMode)
		{
			// Only do for the 3 first modes, because physical mode is created earlier
			if (nullptr != this->tempPlaceMovableObject && true == this->tempPlaceMovableObject->isAttached())
			{
				// 1. is the mesh name and the rest are the data block names
				std::vector<Ogre::String> meshData;

				Ogre::v1::Entity* tempEntity = dynamic_cast<Ogre::v1::Entity*>(this->tempPlaceMovableObject);
				if (nullptr != tempEntity)
				{
					meshData.resize(1 + tempEntity->getNumSubEntities());
					meshData[0] = tempEntity->getMesh()->getName();
					for (size_t i = 0; i < tempEntity->getNumSubEntities(); i++)
					{
						meshData[i + 1] = *tempEntity->getSubEntity(i)->getDatablock()->getNameStr();
					}
				}
				else
				{
					Ogre::Item* tempItem = dynamic_cast<Ogre::Item*>(this->tempPlaceMovableObject);
					if (nullptr != tempItem)
					{
						meshData.resize(1 + tempItem->getNumSubItems());
						meshData[0] = tempItem->getMesh()->getName();
						for (size_t i = 0; i < tempItem->getNumSubItems(); i++)
						{
							meshData[i + 1] = *tempItem->getSubItem(i)->getDatablock()->getNameStr();
						}
					}
				}

				// Create GameObject etc. and push to undo stack
				this->sceneManipulationCommandModule.pushCommand(std::make_shared<AddGameObjectUndoCommand>(this->sceneManager, this->tempPlaceMovableNode, meshData, this->currentPlaceType));
				// Sent event that scene has been modified
				boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
				this->placeNode->resetOrientation();

				// Regenerate categories
				boost::shared_ptr<EventDataGenerateCategories> eventDataGenerateCategories(new EventDataGenerateCategories());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGenerateCategories);
			}
			else
			{
				// When group is placed, the group is cloned, so that it can be placed several times, until the user presses escape or chooses another manipulation mode
				if (this->groupGameObjectIds.size() > 0)
				{
					std::vector<unsigned long> tempGameObjectGroup(this->groupGameObjectIds.size());
					// Do not change to translate mode after clone (false parameter)
					for (size_t i = 0; i < this->groupGameObjectIds.size(); i++)
					{
						tempGameObjectGroup[i] = std::get<0>(this->groupGameObjectIds[i]);
					}

					this->sceneManipulationCommandModule.pushCommand(std::make_shared<CloneGameObjectGroupUndoCommand>(this, tempGameObjectGroup));

					// Sent event that scene has been modified
					boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);

					// Regenerate categories
					boost::shared_ptr<EventDataGenerateCategories> eventDataGenerateCategories(new EventDataGenerateCategories());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGenerateCategories);
				}
			}
			this->gizmo->setEnabled(false);
		}
		else if (id == this->mouseButtonId && Gizmo::GIZMO_NONE == this->gizmo->getState()
			&& this->manipulationMode >= EDITOR_SELECT_MODE && this->manipulationMode <= EDITOR_ROTATE_MODE2 && this->manipulationMode != EDITOR_PLACE_MODE)
		{
			if (false == this->isInSimulation)
				this->selectionManager->handleMousePress(evt, id);

			this->setGizmoToGameObjectsCenter();

			if (true == this->selectionManager->getSelectedGameObjects().empty())
			{
				this->viewportGrid->setPosition(Ogre::Vector3::ZERO);
				this->viewportGrid->setOrientation(Ogre::Quaternion::IDENTITY);
			}
			// Show and hide the gizmo
			if (this->manipulationMode >= EDITOR_TRANSLATE_MODE && false == this->selectionManager->getSelectedGameObjects().empty()
				&& EDITOR_PICKER_MODE != this->manipulationMode)
			{
				this->gizmo->setEnabled(true);
			}
			else
			{
				this->gizmo->setEnabled(false);
			}

			this->mouseIdPressed = true;
		}

		if (id == this->mouseButtonId && EDITOR_PICKER_MODE == this->manipulationMode)
		{
			this->mouseIdPressed = true;
			this->gizmo->setEnabled(false);
			this->snapshotSelectedGameObjects();
		}
		// a gizmoState > 0 is always true!
		else if (id == this->mouseButtonId && EDITOR_SELECT_MODE < this->manipulationMode && EDITOR_ROTATE_MODE2 >= this->manipulationMode && Gizmo::GIZMO_NONE < this->gizmo->getState())
		{
			// Get the first ray hit
			this->getRayStartPoint(hitRay);
			this->translateStartPoint = this->gizmo->getPosition();
			this->rotateStartOrientation = this->gizmo->getOrientation();

			this->snapshotSelectedGameObjects();

			this->rotateStartOrientationGizmoX = this->gizmo->getArrowNodeX()->_getDerivedOrientationUpdated();
			this->rotateStartOrientationGizmoY = this->gizmo->getArrowNodeY()->_getDerivedOrientationUpdated();
			this->rotateStartOrientationGizmoZ = this->gizmo->getArrowNodeZ()->_getDerivedOrientationUpdated();
			this->startHitPoint = this->oldHitPoint;
			this->mouseIdPressed = true;
		}
		else if (id == this->mouseButtonId && (EDITOR_TERRAIN_MODIFY_MODE == this->manipulationMode || EDITOR_TERRAIN_SMOOTH_MODE == this->manipulationMode || EDITOR_TERRAIN_PAINT_MODE == this->manipulationMode))
		{
			this->gizmo->setEnabled(false);
			this->mouseIdPressed = true;
			if (nullptr != this->terraComponent)
			{
				auto hitData = this->terraComponent->checkRayIntersect(hitRay);
				if (true == std::get<0>(hitData))
				{
					if (EDITOR_TERRAIN_MODIFY_MODE == this->manipulationMode)
					{
						this->terraComponent->modifyTerrainStart(std::get<1>(hitData), this->terraComponent->getStrength());
					}
					else if (EDITOR_TERRAIN_SMOOTH_MODE == this->manipulationMode)
					{
						this->terraComponent->smoothTerrainStart(std::get<1>(hitData), this->terraComponent->getStrength());
					}
					else if (EDITOR_TERRAIN_PAINT_MODE == this->manipulationMode)
					{
						this->terraComponent->paintTerrainStart(std::get<1>(hitData), this->terraComponent->getBrushIntensity(), this->terraComponent->getImageLayerId());
					}
				}
			}
		}

		if (EDITOR_ROTATE_MODE1 == this->manipulationMode || EDITOR_ROTATE_MODE2 == this->manipulationMode)
		{
			this->gizmo->setSphereEnabled(false);
		}

		if (this->gizmo->isEnabled())
		{
			this->selectGizmo(evt, hitRay);
		}

		if (EDITOR_PLACE_MODE != this->manipulationMode)
		{
			if (nullptr != this->tempPlaceMovableObject && true == this->tempPlaceMovableObject->isAttached())
			{
				this->destroyTempPlaceMovableObjectNode();
				this->placeNode->setVisible(false);
				this->currentPlaceType = GameObject::NONE;
			}
		}
	}

	void EditorManager::handleMouseRelease(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (id == this->mouseButtonId)
		{
			this->mouseIdPressed = false;
		}

		if (id == this->mouseButtonId && (EDITOR_TERRAIN_MODIFY_MODE == this->manipulationMode || EDITOR_TERRAIN_SMOOTH_MODE == this->manipulationMode || EDITOR_TERRAIN_PAINT_MODE == this->manipulationMode))
		{
			if (nullptr != this->terraComponent)
			{
				if (EDITOR_TERRAIN_MODIFY_MODE == this->manipulationMode)
				{
					auto&& heightData = this->terraComponent->modifyTerrainFinished();
					this->snapshotTerraHeightMap(heightData.first, heightData.second, this->terraComponent);
				}
				else if (EDITOR_TERRAIN_SMOOTH_MODE == this->manipulationMode)
				{
					auto&& heightData = this->terraComponent->smoothTerrainFinished();
					this->snapshotTerraHeightMap(heightData.first, heightData.second, this->terraComponent);
				}
				else if (EDITOR_TERRAIN_PAINT_MODE == this->manipulationMode)
				{
					auto&& blendWeightData = this->terraComponent->paintTerrainFinished();
					this->snapshotTerraBlendMap(blendWeightData.first, blendWeightData.second, this->terraComponent);
				}
			}
		}

		if (id == this->mouseButtonId && true == this->selectionManager->getIsSelectingRectangle())
		{
			if (false == this->isInSimulation)
				this->selectionManager->handleMouseRelease(evt, id);

			// Show and hide the gizmo
			if (EDITOR_TRANSLATE_MODE <= this->manipulationMode && false == this->selectionManager->getSelectedGameObjects().empty())
			{
				this->gizmo->setEnabled(true);
			}
			else
			{
				this->gizmo->setEnabled(false);
			}
			// Calculate the center to put the gizmo there
			this->gizmo->setPosition(this->calculateCenter());
		}

		/*if (true == this->selectionManager->getIsSelecting())
			this->applySelectedObjectsStartOrientation();*/

		if (EDITOR_PICKER_MODE == this->manipulationMode)
		{
			this->movePicker->release();
			this->movePicker2->release();
			if (true == useUndoRedoForPicker)
			{
				// Work with ids instead of pointers for undo/redo, because pointer may become invalid when deleted via undo/redo
				std::vector<unsigned long> gameObjectIds = this->getSelectedGameObjectIds();
				if (gameObjectIds.size() > 0)
				{
					this->sceneManipulationCommandModule.pushCommand(std::make_shared<GameObjectManipulationUndoCommand>(this->oldGameObjectDataList, gameObjectIds));
				}
			}
		}

		if (id == this->mouseButtonId && true == this->isGizmoMoving)
		{
			if (EDITOR_SELECT_MODE <= this->manipulationMode && EDITOR_ROTATE_MODE2 >= this->manipulationMode && false == this->selectionManager->getSelectedGameObjects().empty())
			{
				std::vector<unsigned long> gameObjectIds = this->getSelectedGameObjectIds();
				if (gameObjectIds.size() > 0)
				{
					this->sceneManipulationCommandModule.pushCommand(std::make_shared<GameObjectManipulationUndoCommand>(this->oldGameObjectDataList, gameObjectIds));
					// Sent event that scene has been modified
					boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
				}
			}

			if (EDITOR_TRANSLATE_MODE == this->manipulationMode)
			{
				// Sents event that translation for the given selected game objects is finished
				for (auto& it = this->getSelectionManager()->getSelectedGameObjects().begin(); it != this->getSelectionManager()->getSelectedGameObjects().end(); ++it)
				{
					boost::shared_ptr<NOWA::EventDataTranslateFinished> eventDataTranslateFinished(new NOWA::EventDataTranslateFinished(it->second.gameObject->getId(), it->second.gameObject->getPosition()));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTranslateFinished);
				}
				this->gizmo->setPosition(this->calculateCenter());
				this->gizmo->hideLine();
			}
			if (EDITOR_SCALE_MODE == this->manipulationMode)
			{
				this->gizmo->hideLine();
			}
			else if (EDITOR_ROTATE_MODE1 == this->manipulationMode || EDITOR_ROTATE_MODE2 == this->manipulationMode)
			{
				// Sents event that rotation for the given selected game objects is finished
				for (auto& it = this->getSelectionManager()->getSelectedGameObjects().begin(); it != this->getSelectionManager()->getSelectedGameObjects().end(); ++it)
				{
					boost::shared_ptr<NOWA::EventDataRotateFinished> eventDataRotateFinished(new NOWA::EventDataRotateFinished(it->second.gameObject->getId(), it->second.gameObject->getOrientation()));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRotateFinished);
				}
				// Attention: in vwe objects are somehow rotated here!?
				this->gizmo->hideCircle();
				this->absoluteAngle = 0.0f;
				this->stepAngleDelta = 0.0f;
			}
		}

		this->isGizmoMoving = false;
	}

	void EditorManager::filterCategories(Ogre::String& categories)
	{
		unsigned int generatedCategoryId = this->selectionManager->filterCategories(categories);
		this->movePicker->updateQueryMask(generatedCategoryId);
		this->movePicker2->updateQueryMask(generatedCategoryId);
	}

	void EditorManager::focusCameraGameObject(GameObject* gameObject)
	{
		if (nullptr != gameObject)
		{
			this->snapshotCameraTransform();

			// World aabb updated uses also the scale of the node!
			Ogre::Real distance = gameObject->getMovableObject()->getWorldAabbUpdated().getRadius() * 8.0f;

			this->camera->setPosition(gameObject->getPosition() + (this->camera->getOrientation() * Ogre::Vector3(0, 0, distance)));

			// Same as: camera->lookAt
			this->camera->lookAt(gameObject->getPosition());
			// this->camera->roll(Ogre::Radian(0.0f));
		}
	}

	void EditorManager::selectGizmo(const OIS::MouseEvent& evt, const Ogre::Ray& hitRay)
	{
		if (false == this->isGizmoMoving)
		{
			Ogre::v1::Entity* gizmoEntity = nullptr;
			Ogre::Vector3 result = Ogre::Vector3::ZERO;
			Ogre::Real closestDistance = 0.0f;
			Ogre::Vector3 normal = Ogre::Vector3::ZERO;

			// MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("DebugLabel")->setCaption("");

			// Check if there is an hit with an polygon of an entity
			if (MathHelper::getInstance()->getRaycastFromPoint(evt.state.X.abs, evt.state.Y.abs, this->camera, Core::getSingletonPtr()->getOgreRenderWindow(),
				this->gizmoQuery, result, (size_t&)gizmoEntity, closestDistance, normal, nullptr, true))
			{
				Ogre::Vector3 gizmoPosition = this->gizmo->getSelectedNode()->_getDerivedPositionUpdated();

				Ogre::Vector3 gizmoDirectionX = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().xAxis();
				Ogre::Vector3 gizmoDirectionY = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().yAxis();
				Ogre::Vector3 gizmoDirectionZ = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().zAxis();

				this->gizmo->redefineFirstPlane(gizmoDirectionX, gizmoPosition);
				this->gizmo->redefineSecondPlane(gizmoDirectionY, gizmoPosition);
				this->gizmo->redefineThirdPlane(gizmoDirectionZ, gizmoPosition);

				Ogre::Real vX = this->gizmo->getFirstPlane().projectVector(hitRay.getDirection()).length();
				Ogre::Real vY = this->gizmo->getSecondPlane().projectVector(hitRay.getDirection()).length();
				Ogre::Real vZ = this->gizmo->getThirdPlane().projectVector(hitRay.getDirection()).length();

				if (this->gizmo->getArrowEntityX() == gizmoEntity)
				{
					// http://www.ogre3d.org/forums/viewtopic.php?f=2&t=41739

					// Change the color of the arrow, when the user hovers over it
					this->gizmo->highlightXArrow();
					vX = 10000.0f;

					if (EDITOR_ROTATE_MODE1 == this->manipulationMode || EDITOR_ROTATE_MODE2 == this->manipulationMode)
					{
						this->resultPlane.redefine(gizmoDirectionY, this->gizmo->getPosition());
					}
				}
				else if (this->gizmo->getArrowEntityY() == gizmoEntity)
				{
					this->gizmo->highlightYArrow();
					vY = 10000.0f;
					if (EDITOR_ROTATE_MODE1 == this->manipulationMode || EDITOR_ROTATE_MODE2 == this->manipulationMode)
					{
						this->resultPlane.redefine(gizmoDirectionZ, this->gizmo->getPosition());
					}
				}
				else if (this->gizmo->getArrowEntityZ() == gizmoEntity)
				{
					this->gizmo->highlightZArrow();
					vZ = 10000.0f;
					if (EDITOR_ROTATE_MODE1 == this->manipulationMode || EDITOR_ROTATE_MODE2 == this->manipulationMode)
					{
						this->resultPlane.redefine(gizmoDirectionX, this->gizmo->getPosition());
					}
				}
				else if (this->gizmo->getSphereEntity() == gizmoEntity
					&& EDITOR_ROTATE_MODE1 != this->manipulationMode && EDITOR_ROTATE_MODE2 != this->manipulationMode)
				{
					this->gizmo->highlightSphere();

					this->resultPlane.redefine(this->camera->getDerivedDirection(), gizmoPosition);
					// MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("DebugLabel")->setCaption("Sphere Gizmo hit");
					return;
				}
				else
				{
					this->gizmo->unHighlightGizmo();

					Ogre::Vector3 cameraBack = this->camera->getDerivedDirection();
					cameraBack = -cameraBack;
					this->resultPlane.redefine(cameraBack, gizmoPosition);
					// MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("DebugLabel")->setCaption("No Gizmo hit");
					return;
				}

				if (EDITOR_ROTATE_MODE1 != this->manipulationMode && EDITOR_ROTATE_MODE2 != this->manipulationMode)
				{
					if (vX < vY && vX < vZ)
					{
						this->resultPlane = this->gizmo->getFirstPlane();
						// this->gizmo->_debugShowResultPlane(1);
					}
					else
					{
						if (vY < vX && vY < vZ)
						{
							this->resultPlane = this->gizmo->getSecondPlane();
							// this->gizmo->_debugShowResultPlane(2);
						}
						else
						{
							this->resultPlane = this->gizmo->getThirdPlane();
							// this->gizmo->_debugShowResultPlane(3);
						}
					}
				}
			}
			else
			{
				this->gizmo->unHighlightGizmo();
			}
		}
	}

	bool EditorManager::getRayHitPoint(const Ogre::Ray& hitRay)
	{
		std::pair<bool, Ogre::Real> intersectPoint1 = hitRay.intersects(this->resultPlane);

		if (intersectPoint1.first)
		{
			this->hitPoint = hitRay.getPoint(intersectPoint1.second);

			if (this->constraintAxis.x != 0.0f)
			{
				this->hitPoint = Ogre::Vector3(this->constraintAxis.x, this->hitPoint.y, this->hitPoint.z);
			}
			if (this->constraintAxis.y != 0.0f)
			{
				this->hitPoint = Ogre::Vector3(this->hitPoint.x, this->constraintAxis.y, this->hitPoint.z);
			}
			if (this->constraintAxis.z != 0.0f)
			{
				this->hitPoint = Ogre::Vector3(this->hitPoint.x, this->hitPoint.y, this->constraintAxis.z);
			}

			return true;
		}
		return false;
	}

	void EditorManager::manipulateObjects(const Ogre::Ray& hitRay)
	{
		if (this->getRayHitPoint(hitRay))
		{
			Ogre::Vector3 unitScale = Ogre::Vector3::ZERO;
			// http://www.ogre3d.org/forums/viewtopic.php?f=25&t=83011
			// http://www.ogre3d.org/forums/viewtopic.php?f=2&t=41739

			// http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Pick+Drag+Drop&structure=Cookbook
			// Important!!!! http://www.ogre3d.org/forums/viewtopic.php?f=5&t=58572

			if (EDITOR_ROTATE_MODE1 != this->manipulationMode && EDITOR_ROTATE_MODE2 != this->manipulationMode)
			{
				Ogre::Vector3 gizmoDirectionX = Ogre::Vector3::ZERO;
				Ogre::Vector3 gizmoDirectionY = Ogre::Vector3::ZERO;
				Ogre::Vector3 gizmoDirectionZ = Ogre::Vector3::ZERO;

				switch (this->gizmo->getState())
				{
				case Gizmo::GIZMO_ARROW_X:
				{
					gizmoDirectionX = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().xAxis();
					break;
				}
				case Gizmo::GIZMO_ARROW_Y:
				{
					gizmoDirectionY = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().yAxis();
					break;
				}
				case Gizmo::GIZMO_ARROW_Z:
				{
					gizmoDirectionZ = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().zAxis();
					break;
				}
				case Gizmo::GIZMO_SPHERE:
				{
					gizmoDirectionX = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().xAxis();
					gizmoDirectionY = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().yAxis();
					gizmoDirectionZ = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().zAxis();
					break;
				}
				}

				// use dot product because it will go from 0 to 180, so there is when e.g. rotation x about 180 degree
				// its still possible to move the arrow x in the correct direction
				Ogre::Vector3 moveDirection = this->hitPoint - this->oldHitPoint;
				// dot product give the strength, so the corresponding gizmo direction is scaled by the strength, and the fine thing is
				// that when the x is rotated about 180, then vPos1 = -1, when 90 degree vPos1 = 0
				Ogre::Vector3 vPos1 = (gizmoDirectionX.dotProduct(moveDirection) * gizmoDirectionX);
				Ogre::Vector3 vPos2 = (gizmoDirectionY.dotProduct(moveDirection) * gizmoDirectionY);
				Ogre::Vector3 vPos3 = (gizmoDirectionZ.dotProduct(moveDirection) * gizmoDirectionZ);

				Ogre::Vector3 translateVector = vPos1 + vPos2 + vPos3;

				if (EDITOR_TRANSLATE_MODE == this->manipulationMode)
				{
					this->moveObjects(translateVector);
				}
				else if (EDITOR_SCALE_MODE == this->manipulationMode)
				{
					if (Gizmo::GIZMO_SPHERE == this->gizmo->getState())
					{
						unitScale = this->calculateUnitScaleFactor(translateVector);
						this->scaleObjects(unitScale);
					}
					else
					{
						this->scaleObjects(translateVector);
					}
				}
			}
			else
			{
				this->rotateObjects();
			}
		}
		// Actualize the hit point
		this->oldHitPoint = this->hitPoint;
	}

	Ogre::Vector3 EditorManager::getHitPointOnFloor(const Ogre::Ray& hitRay)
	{
		std::pair<bool, Ogre::Vector3> intersectPoint;
		Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
		// Build an infinite invisible plane to get an touchresult of the ray
		Ogre::Plane plane(Ogre::Vector3::UNIT_Y, 0);

		if (this->constraintAxis.x != 0.0f)
		{
			plane = Ogre::Plane(Ogre::Vector3::UNIT_X, 0);
		}
		if (this->constraintAxis.z != 0.0f)
		{
			plane = Ogre::Plane(Ogre::Vector3::UNIT_Z, 0);
		}

		// Check the result
		intersectPoint = hitRay.intersects(plane);
		if (intersectPoint.first)
		{
			internalHitPoint.x = hitRay.getPoint(intersectPoint.second.x).x;
			internalHitPoint.y = hitRay.getPoint(intersectPoint.second.x).y;
			internalHitPoint.z = hitRay.getPoint(intersectPoint.second.z).z;
		}

		if (this->constraintAxis.x != 0.0f)
		{
			internalHitPoint = Ogre::Vector3(this->constraintAxis.x, internalHitPoint.y, internalHitPoint.z);
		}
		if (this->constraintAxis.y != 0.0f)
		{
			internalHitPoint = Ogre::Vector3(internalHitPoint.x, this->constraintAxis.y, internalHitPoint.z);
		}
		if (this->constraintAxis.z != 0.0f)
		{
			internalHitPoint = Ogre::Vector3(internalHitPoint.x, internalHitPoint.y, this->constraintAxis.z);
		}

		return internalHitPoint;
	}

	void EditorManager::movePlaceNode(const Ogre::Ray& hitRay)
	{
		Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
		// this->placeNode->setPosition(Ogre::Vector3::ZERO);
		Ogre::MovableObject* hitMovableObject = nullptr;

		switch (this->placeMode)
		{
			case EDITOR_PLACE_MODE_NORMAL:
			{
				internalHitPoint = this->getHitPointOnFloor(hitRay);
				break;
			}
			case EDITOR_PLACE_MODE_STACK:
			{
				Ogre::Real closestDistance = 0.0f;
				Ogre::Vector3 normal = Ogre::Vector3::ZERO;

				// Check if there is an hit with an polygon of an entity and stack the to be placed object
				this->placeObjectQuery->setRay(hitRay);

				/*Ogre::Ray innerObjectHitPointRay = Ogre::Ray(Ogre::Vector3(this->placeNode->getPosition().x, this->placeNode->getPosition().y + 0.1f, this->placeNode->getPosition().z), this->camera->getDirection());
				this->toBePlacedObjectQuery->setRay(innerObjectHitPointRay);
				Ogre::Vector3 vertexPosOfInnerModel = Ogre::Vector3::ZERO;
				MathHelper::getInstance()->getRaycastFromPoint(this->toBePlacedObjectQuery, vertexPosOfInnerModel, (unsigned long&)tempEntity, closestDistance, normal);

				if (Ogre::Vector3::ZERO != vertexPosOfInnerModel)
				{
					Ogre::Ray hitRay = Ogre::Ray(vertexPosOfInnerModel, this->camera->getDirection());
					this->placeObjectQuery->setRay(hitRay);
					MathHelper::getInstance()->getRaycastFromPoint(this->placeObjectQuery, internalHitPoint, (unsigned long&)tempEntity, closestDistance, normal, this->tempPlaceEntity);
				}
				else*/
				{
					// Check whether to add just the temp place entity to excluded list, or the whole group
					std::vector<Ogre::MovableObject*> excludeMovableObjects;
					if (this->groupGameObjectIds.size() > 0)
					{
						for (size_t i = 0; i < this->groupGameObjectIds.size(); i++)
						{
							GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(std::get<0>(this->groupGameObjectIds[i]));
							excludeMovableObjects.emplace_back(gameObjectPtr->getMovableObject());
						}
					}
					else if (nullptr != this->tempPlaceMovableObject)
					{
						excludeMovableObjects.emplace_back(this->tempPlaceMovableObject);
					}
					MathHelper::getInstance()->getRaycastFromPoint(this->placeObjectQuery, this->camera, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects);
				}

				// If nothing to stack to, calc the hit point on floor plane
				if (nullptr == hitMovableObject)
				{
					internalHitPoint = this->getHitPointOnFloor(hitRay);
				}

				break;
			}
			case EDITOR_PLACE_MODE_STACK_ORIENTATED:
			{
				Ogre::Real closestDistance = 0.0f;
				Ogre::Vector3 normal = Ogre::Vector3::ZERO;

				// Check if there is an hit with an polygon of an entity and stack the to be placed object and orientate the object
				this->placeObjectQuery->setRay(hitRay);

				// Check whether to add just the temp place entity to excluded list, or the whole group
				std::vector<Ogre::MovableObject*> excludeMovableObjects;
				if (this->groupGameObjectIds.size() > 0)
				{
					for (size_t i = 0; i < this->groupGameObjectIds.size(); i++)
					{
						GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(std::get<0>(this->groupGameObjectIds[i]));
						excludeMovableObjects.emplace_back(gameObjectPtr->getMovableObject());
					}
				}
				else if (nullptr != this->tempPlaceMovableObject)
				{
					excludeMovableObjects.emplace_back(this->tempPlaceMovableObject);
				}
				if (MathHelper::getInstance()->getRaycastFromPoint(this->placeObjectQuery, this->camera, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects))
				{
					this->placeNode->setDirection(normal, Ogre::Node::TS_PARENT, Ogre::Vector3::NEGATIVE_UNIT_Y);
				}
				else
				{
					this->placeNode->resetOrientation();
				}

				// If nothing to stack to, calc the hit point on floor plane
				if (nullptr == hitMovableObject)
				{
					internalHitPoint = this->getHitPointOnFloor(hitRay);
				}
				break;
			}
		}

		if (this->gridStep > 0.0f)
		{
			// Let the node follow the mouse step by step based on the gridsize
			if (nullptr != this->tempPlaceMovableObject)
			{
				if (GetAsyncKeyState(VK_LMENU))
				{
					Ogre::Vector3 direction = (this->tempPlaceMovableNode->_getDerivedPositionUpdated() - internalHitPoint).normalisedCopy();

					Ogre::Aabb boundingBox;
					Ogre::Aabb box = this->tempPlaceMovableNode->getAttachedObject(0)->getLocalAabb();

					Ogre::Vector3 newGridPoint = MathHelper::getInstance()->calculateGridTranslation(box.getSize() * this->gridStep, internalHitPoint, this->tempPlaceMovableNode->getAttachedObject(0));

					if (this->constraintAxis.x != 0.0f)
						newGridPoint = Ogre::Vector3(this->constraintAxis.x, newGridPoint.y, newGridPoint.z);
					if (this->constraintAxis.y != 0.0f)
						newGridPoint = Ogre::Vector3(newGridPoint.x, this->constraintAxis.y, newGridPoint.z);
					if (this->constraintAxis.z != 0.0f)
						newGridPoint = Ogre::Vector3(newGridPoint.x, newGridPoint.y, this->constraintAxis.z);

					this->placeNode->setPosition(newGridPoint);
				}
				else
				{
					Ogre::Vector3 resultVector = MathHelper::getInstance()->calculateGridValue(this->gridStep, internalHitPoint);

					if (this->constraintAxis.x != 0.0f)
						resultVector = Ogre::Vector3(this->constraintAxis.x, resultVector.y, resultVector.z);
					if (this->constraintAxis.y != 0.0f)
						resultVector = Ogre::Vector3(resultVector.x, this->constraintAxis.y, resultVector.z);
					if (this->constraintAxis.z != 0.0f)
						resultVector = Ogre::Vector3(resultVector.x, resultVector.y, this->constraintAxis.z);
					this->placeNode->_setDerivedPosition(resultVector);
				}
			}
		}
		else
		{
			// Set the node to the mouse position
			if (this->constraintAxis.x != 0.0f)
				internalHitPoint = Ogre::Vector3(this->constraintAxis.x, internalHitPoint.y, internalHitPoint.z);
			if (this->constraintAxis.y != 0.0f)
				internalHitPoint = Ogre::Vector3(internalHitPoint.x, this->constraintAxis.y, internalHitPoint.z);
			if (this->constraintAxis.z != 0.0f)
				internalHitPoint = Ogre::Vector3(internalHitPoint.x, internalHitPoint.y, this->constraintAxis.z);
			this->placeNode->_setDerivedPosition(internalHitPoint);
		}

		if (this->groupGameObjectIds.size() > 0)
		{
			this->applyGroupTransform();
		}
		else if (nullptr != this->tempPlaceMovableObject)
		{
			this->applyPlaceMovableTransform();
		}
	}

	void EditorManager::rotatePlaceNode(void)
	{
		if (NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState().Z.rel > 0.0f)
		{
			this->rotateFactor += 10.0f;
		}
		else if (NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState().Z.rel < 0.0f)
		{
			this->rotateFactor -= 10.0f;
		}
		if (nullptr != this->placeNode && this->rotateFactor != 0.0f)
		{
			this->placeNode->setOrientation(Ogre::Quaternion(Ogre::Degree(this->rotateFactor), Ogre::Vector3::UNIT_Y));
		}
	}

	void EditorManager::applyPlaceMovableTransform(void)
	{
		// Set position with center and y = 0 with offset to center when place node is orientated
		this->tempPlaceMovableNode->_setDerivedPosition(this->placeNode->_getDerivedPositionUpdated() + (this->placeNode->getOrientation()
			* MathHelper::getInstance()->getBottomCenterOfMesh(this->tempPlaceMovableNode, this->tempPlaceMovableObject)));
		this->tempPlaceMovableNode->setOrientation(this->placeNode->_getDerivedOrientationUpdated());
	}

	void EditorManager::applyGroupTransform(void)
	{
		// If there is a group of game objects, move them with their internal offset
		// This was hard: x-objects are loaded which had an position in the world, these object shall be placed to the place node position but keeping the relative position
		// to each other. They should be placed so that the place node position is in the middle of all objects but y is zero, so that they can be placed at zero, no matter how
		// high the objects are placed ot each other

		for (size_t i = 0; i < this->groupGameObjectIds.size(); i++)
		{
			GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(std::get<0>(this->groupGameObjectIds[i]));

			// Place node position is global and on the top goes offset position (current object pos relative to center)
			Ogre::Vector3 targetPosition = this->placeNode->_getDerivedPositionUpdated() + (this->placeNode->_getDerivedOrientationUpdated() * std::get<1>(this->groupGameObjectIds[i]));

			auto& physicsComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::PhysicsComponent>());
			if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
			{
				physicsComponent->setPosition(targetPosition);
				// Set orientation as the place node + the current object orientation
				physicsComponent->setOrientation(this->placeNode->_getDerivedOrientationUpdated() * std::get<2>(this->groupGameObjectIds[i]));
			}
			else
			{
				// If there is no physics component set the data directly for the game object scene node
				gameObjectPtr->setAttributePosition(targetPosition);
				// Set orientation as the place node + the current object orientation
				gameObjectPtr->getSceneNode()->setOrientation(this->placeNode->_getDerivedOrientationUpdated() * std::get<2>(this->groupGameObjectIds[i]) /** gameObjectPtr->getSceneNode()->getOrientation()*/);
			}
		}
	}

	void EditorManager::destroyTempPlaceMovableObjectNode(void)
	{
		if (nullptr != this->tempPlaceMovableNode)
		{
			this->tempPlaceMovableNode->detachAllObjects();
			this->sceneManager->destroySceneNode(this->tempPlaceMovableNode);

			if (nullptr != this->tempPlaceMovableObject)
			{
				this->sceneManager->destroyMovableObject(this->tempPlaceMovableObject);
			}
			this->tempPlaceMovableNode = nullptr;
			this->tempPlaceMovableObject = nullptr;
		}
	}

	// More modern version: Everything placed is now item be default instead of entity! Even animation does work for item!
	// Unfortunately causes crash e.g. with spaceGun2.mesh
	void EditorManager::attachMeshToPlaceNode(const Ogre::String& meshName, GameObject::eType type, Ogre::v1::Mesh* mesh)
	{
		// Note: Its always an entity, no matter if internally its an billboard or something else, because the entity is visible geometry for the editor!
		this->destroyTempPlaceMovableObjectNode();

		Ogre::v1::MeshPtr v1Mesh;
		Ogre::MeshPtr v2Mesh;

		try
		{
			if (GameObject::PLANE == type || GameObject::MIRROR == type)
			{
				throw Ogre::Exception(Ogre::Exception::ERR_RT_ASSERTION_FAILED, "", "");
			}

			// Checks the mesh serializer version, in order to consider whether to create an item (version >= 2.0 or entity < 2.0)
			Ogre::String resourceFilePathName = Core::getSingletonPtr()->getResourceFilePathName(meshName);
			resourceFilePathName += "/" + meshName;
			Ogre::String content = Core::getSingletonPtr()->readContent(resourceFilePathName, 2, 40);
			size_t serializerStartPos = content.find("[");
			Ogre::String serializerVersion = "1.4";

			if (serializerStartPos != Ogre::String::npos)
			{
				// Get the length of the modulename inside the file
				size_t serializerEndPos = content.find("]", serializerStartPos);
				if (serializerEndPos != Ogre::String::npos)
				{
					size_t length = serializerEndPos - serializerStartPos - 1;
					if (length > 0)
					{
						Ogre::String serializerName = content.substr(serializerStartPos + 1, length);
						size_t serializerVersionStartPos = serializerName.find("_v");
						if (serializerVersionStartPos != Ogre::String::npos)
						{
							serializerVersion = serializerName.substr(serializerVersionStartPos + 2, serializerName.length() - serializerVersionStartPos - 2);
						}
					}
				}
			}

			bool canBeV2Mesh = false;
			if (serializerVersion == "1.100")
			{
				canBeV2Mesh = true;
			}
			else if (serializerVersion == "1.8")
			{
				canBeV2Mesh = true;
			}
			else if (serializerVersion == "1.4")
			{
				canBeV2Mesh = true;
			}
			else if (serializerVersion == "1.41")
			{
				canBeV2Mesh = true;
			}
			else if (serializerVersion == "1.3")
			{
				canBeV2Mesh = true;
			}
			else if (serializerVersion == "1.2")
			{
				canBeV2Mesh = true;
			}
			else if (serializerVersion == "1.1")
			{
				canBeV2Mesh = true;
			}
			else if (serializerVersion == "2.1 R0 LEGACYV1")
			{
				canBeV2Mesh = true;
			}

			// It must also be checked, if this kind of mesh shall be used. Because no ragdolling is possible, but pose weighting etc. is possible and its more efficient for rendering etc.
			canBeV2Mesh &= !Core::getSingletonPtr()->getUseEntityType();

			// Does crash if v2 on node.mesh
			if (meshName == "Node.mesh")
			{
				canBeV2Mesh = false;
			}

			// See OgreMesh.h
			/*
			friend class MeshSerializerImpl_v1_10;
			friend class MeshSerializerImpl_v1_8;
			friend class MeshSerializerImpl_v1_4;
			friend class MeshSerializerImpl_v1_3;
			friend class MeshSerializerImpl_v1_2;
			friend class MeshSerializerImpl_v1_1;
			*/
			// More to follow
		
			
			if ((v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->getByName(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
			{
				v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->load(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
																		Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
			}

			// Ogre::Root::getSingletonPtr()->renderOneFrame();

			if (nullptr == v1Mesh)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[EditorManager] Error cannot create entity, because the mesh: '"
																+ meshName + "' could not be created.");
				return;
			}

			if (false == canBeV2Mesh)
			{
				this->tempPlaceMovableObject = this->sceneManager->createEntity(v1Mesh);
				this->tempPlaceMovableObject->setName("PlaceEntity");
				this->tempPlaceMovableObject->setQueryFlags(AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId("Default"));
				this->tempPlaceMovableObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V1_MESH);

				DeployResourceModule::getInstance()->tagResource(meshName, v1Mesh->getGroup());
			}
			else
			{
				if ((v2Mesh = Ogre::MeshManager::getSingletonPtr()->getByName(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
				{
					v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, v1Mesh.get(), true, true, true);
				}
				v1Mesh->unload();

				this->tempPlaceMovableObject = this->sceneManager->createItem(v2Mesh);
				this->tempPlaceMovableObject->setName("PlaceEntity");
				this->tempPlaceMovableObject->setQueryFlags(AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId("Default"));
				this->tempPlaceMovableObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
				type = GameObject::ITEM;

				DeployResourceModule::getInstance()->tagResource(meshName, v2Mesh->getGroup());
			}
		}
		catch (Ogre::Exception&)
		{
			try
			{
				if (GameObject::PLANE == type || GameObject::MIRROR == type)
				{
					v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, mesh, true, true, true);
					mesh->unload();
				}
				else
				{
					// Maybe its a v2 mesh
					if ((v2Mesh = Ogre::MeshManager::getSingletonPtr()->getByName(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
					{
						v2Mesh = Ogre::MeshManager::getSingletonPtr()->load(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
					}
				}
				if (nullptr == v2Mesh)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[EditorManager] Error cannot create entity, because the mesh: '"
																	+ meshName + "' could not be created.");
					return;
				}

				this->tempPlaceMovableObject = this->sceneManager->createItem(v2Mesh);
				this->tempPlaceMovableObject->setName("PlaceEntity");
				this->tempPlaceMovableObject->setQueryFlags(AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId("Default"));
				this->tempPlaceMovableObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
				// If it is not a v1 mesh or a plane, its a v2 mesh, so set the proper type
				if (GameObject::PLANE != type)
				{
					type = GameObject::ITEM;
				}
			}
			catch (...)
			{
				return;
			}
		}

		Ogre::v1::Entity* tempEntity = dynamic_cast<Ogre::v1::Entity*>(this->tempPlaceMovableObject);
		if (nullptr != tempEntity)
		{
			if ("Plane" == meshName)
			{
				if (nullptr != tempEntity)
				{
					tempEntity->setDatablock("GroundDirtPlane");
				}
			}

			for (size_t i = 0; i < tempEntity->getNumSubEntities(); i++)
			{
				auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(tempEntity->getSubEntity(i)->getDatablock());
				if (nullptr != sourceDataBlock)
				{
					// Deactivate fresnel by default, because it looks ugly
					if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
					{
						sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
					}
				}
			}
		}
		else
		{
			Ogre::Item* tempItem = dynamic_cast<Ogre::Item*>(this->tempPlaceMovableObject);
			if (nullptr != tempItem)
			{
				if (GameObject::PLANE == type)
				{
					if (nullptr != tempItem)
					{
						tempItem->setDatablock("GroundDirtPlane");
					}
				}
				// Change the addressing mode of the roughness map to wrap via code.
				// Detail maps default to wrap, but the rest to clamp.
				Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(tempItem->getSubItem(0)->getDatablock());
				//Make a hard copy of the sampler block
				auto sampleBlock = datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS);
				if (nullptr != sampleBlock)
				{
					Ogre::HlmsSamplerblock samplerblockCopy(*sampleBlock);
					samplerblockCopy.mU = Ogre::TAM_WRAP;
					samplerblockCopy.mV = Ogre::TAM_WRAP;
					samplerblockCopy.mW = Ogre::TAM_WRAP;
					//Set the new samplerblock. The Hlms system will
					//automatically create the API block if necessary
					datablock->setSamplerblock(Ogre::PBSM_ROUGHNESS, samplerblockCopy);
				}

				for (size_t i = 0; i < tempItem->getNumSubItems(); i++)
				{
					auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(tempItem->getSubItem(i)->getDatablock());
					if (nullptr != sourceDataBlock)
					{
						// Deactivate fresnel by default, because it looks ugly
						if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
						{
							sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
						}
					}
				}
			}
		}

		// this->tempPlaceEntity->setQueryFlags(VWE::PLACEOBJECT_MASK);
		// MathHelper::getInstance()->ensureHasTangents(this->tempPlaceEntity->getMesh());
		// MathHelper::getInstance()->substractOutTangentsForShader(this->tempPlaceEntity);

		/*for (size_t i = 0; i < this->tempPlaceEntity->getNumSubEntities(); i++)
		{
			this->tempPlaceEntity->getSubEntity(i)->setDatablock(this->tempPlaceEntity->getSubEntity(i)->getDatablock());
		}*/

		this->tempPlaceMovableNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC, Ogre::Vector3(0.0f, 20.0f, 0.0f));
		this->tempPlaceMovableNode->setName("TempPlaceEntityNode");
		this->tempPlaceMovableNode->attachObject(this->tempPlaceMovableObject);
		this->placeNode->setVisible(true);
		this->placeNode->setPosition(Ogre::Vector3(0.0f, 20.0f, 0.0f));
		this->placeNode->setOrientation(Ogre::Quaternion::IDENTITY);
		this->selectionManager->clearSelection();
		this->applyPlaceMovableTransform();

		this->currentPlaceType = type;
		this->manipulationMode = EDITOR_PLACE_MODE;
		boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(this->manipulationMode));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
	}

	void EditorManager::attachOtherResourceToPlaceNode(GameObject::eType type)
	{
		this->manipulationMode = EDITOR_PLACE_MODE;

		// Note: Its always an entity, no matter if internally its an billboard or something else, because the entity is visible geometry for the editor!
		if (GameObject::PLANE == type)
		{
			Ogre::ResourcePtr resourceV1 = Ogre::v1::MeshManager::getSingletonPtr()->getResourceByName("Plane");
			// Destroy a potential plane v1, because an error occurs (plane with name ... already exists)
			if (nullptr != resourceV1)
			{
				Ogre::v1::MeshManager::getSingletonPtr()->destroyResourcePool("Plane");
				Ogre::v1::MeshManager::getSingletonPtr()->remove(resourceV1->getHandle());
			}

			// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
			Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName("Plane");
			if (nullptr != resourceV2)
			{
				Ogre::MeshManager::getSingletonPtr()->destroyResourcePool("Plane");
				Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
			}

			Ogre::v1::MeshPtr meshV1 = Ogre::v1::MeshManager::getSingleton().createPlane("Plane", "General", Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f), 50.0f, 50.0f, 2, 2, true,
					1, 20.0f, 20.0f, Ogre::Vector3::UNIT_Z, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
			
			this->attachMeshToPlaceNode(meshV1->getName(), type, meshV1.get());
		}
		else if (GameObject::MIRROR == type)
		{
			Ogre::ResourcePtr resourceV1 = Ogre::v1::MeshManager::getSingletonPtr()->getResourceByName("Mirror");
			// Destroy a potential plane v1, because an error occurs (plane with name ... already exists)
			if (nullptr != resourceV1)
			{
				Ogre::v1::MeshManager::getSingletonPtr()->destroyResourcePool("Mirror");
				Ogre::v1::MeshManager::getSingletonPtr()->remove(resourceV1->getHandle());
			}

			// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
			Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName("Mirror");
			if (nullptr != resourceV2)
			{
				Ogre::MeshManager::getSingletonPtr()->destroyResourcePool("Mirror");
				Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
			}

			Ogre::v1::MeshPtr meshV1 = Ogre::v1::MeshManager::getSingleton().createPlane("Mirror",
					"General", Ogre::Plane(Ogre::Vector3::UNIT_Z, Ogre::Vector3::ZERO /* or here distance 1??*/), 1.0f, 1.0f, 1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Y);
			
			this->attachMeshToPlaceNode(meshV1->getName(), type, meshV1.get());

			Ogre::Item* tempItem = dynamic_cast<Ogre::Item*>(this->tempPlaceMovableObject);
			if (nullptr != tempItem)
			{
				tempItem->setDatablock("NOWAGlassRoughness2");
			}
		}
		else if (GameObject::LIGHT_DIRECTIONAL == type)
		{
			this->attachMeshToPlaceNode("LightDirectional.mesh", type);
		}
		else if (GameObject::LIGHT_SPOT == type)
		{
			this->attachMeshToPlaceNode("LightSpot.mesh", type);
		}
		else if (GameObject::LIGHT_POINT == type)
		{
			this->attachMeshToPlaceNode("LightPoint.mesh", type);
		}
		else if (GameObject::LIGHT_AREA == type)
		{
			this->attachMeshToPlaceNode("LightDirectional.mesh", type);
		}
		else if (GameObject::CAMERA == type)
		{
			this->attachMeshToPlaceNode("Camera.mesh", type);
		}
		else if (GameObject::REFLECTION_CAMERA == type)
		{
			this->attachMeshToPlaceNode("Camera.mesh", type);
		}
		else if (GameObject::SCENE_NODE == type)
		{
			this->attachMeshToPlaceNode("Node.mesh", type);
		}
		else if (GameObject::LINES == type)
		{
			this->attachMeshToPlaceNode("Node.mesh", type);
		}
		else if (GameObject::MANUAL_OBJECT == type)
		{
			this->attachMeshToPlaceNode("Node.mesh", type);
		}
		else if (GameObject::RECTANGLE == type)
		{
			this->attachMeshToPlaceNode("Node.mesh", type);
		}
		else if (GameObject::DECAL == type)
		{
			this->attachMeshToPlaceNode("Node.mesh", type);
		}
		else if (GameObject::OCEAN == type)
		{
			//this->destroyTempPlaceMovableObjectNode();
			//this->tempPlaceMovableNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC, Ogre::Vector3(0.0f, 0.0f, 0.0f));
			//this->tempPlaceMovableNode->setName("TempPlaceEntityNode");
			//std::vector<Ogre::String> meshData(1);
			//meshData[0] = "";
			//// Create GameObject etc. and push to undo stack
			//this->sceneManipulationCommandModule.pushCommand(std::make_shared<AddGameObjectUndoCommand>(this->sceneManager, this->tempPlaceMovableNode, meshData, GameObject::OCEAN));
			//// Change mode, because else, this will be always placed, because on mouse click additionally an object is placed when in place mode
			//this->manipulationMode = EditorManager::EDITOR_SELECT_MODE;
			this->attachMeshToPlaceNode("Node.mesh", type);
		}
		else if (GameObject::TERRA == type)
		{
			this->attachMeshToPlaceNode("Node.mesh", type);
		}

		this->currentPlaceType = type;
		boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(this->manipulationMode));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
	}

	void EditorManager::attachGroupToPlaceNode(const std::vector<unsigned long>& gameObjectIds)
	{
		if (gameObjectIds.size() > 0)
		{
			this->destroyTempPlaceMovableObjectNode();
			this->manipulationMode = EDITOR_PLACE_MODE;
			this->placeNode->setVisible(true);
			this->selectionManager->clearSelection();

			// Center calculation for applyGroupTransform etc. must only be calculated once! When attached!

			// First calculate the center and the deepest objects y-position and set that y position for center as offset for all objects

			Ogre::Vector3 center = Ogre::Vector3::ZERO;
			Ogre::Vector3 cumulated = Ogre::Vector3::ZERO;
			Ogre::Real lowestObjectY = std::numeric_limits<Ogre::Real>::max();

			this->groupGameObjectIds.resize(gameObjectIds.size());
			for (size_t i = 0; i < this->groupGameObjectIds.size(); i++)
			{
				std::get<0>(this->groupGameObjectIds[i]) = gameObjectIds[i];

				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(std::get<0>(this->groupGameObjectIds[i]));

				Ogre::Vector3 position = gameObjectPtr->getPosition();

				if (position.y < lowestObjectY)
				{
					lowestObjectY = gameObjectPtr->getMovableObject()->getWorldAabbUpdated().getMinimum().y;
				}
				cumulated += position;
			}
			center = cumulated / static_cast<Ogre::Real>(this->groupGameObjectIds.size());
			center.y = lowestObjectY;

			// Next get rid of each absolute position in the world, but get the relative position from each object to the center
			for (size_t i = 0; i < this->groupGameObjectIds.size(); i++)
			{
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(std::get<0>(this->groupGameObjectIds[i]));

				std::get<1>(this->groupGameObjectIds[i]) = gameObjectPtr->getPosition() - center;
				std::get<2>(this->groupGameObjectIds[i]) = gameObjectPtr->getOrientation();
			}

			// Usage this->groupGameObjectIds[i].second as new target position in applyGroupTarget! So that even stacking etc. does work, because the positions are not influenced by
			// this center offset calculation anymore!

			boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(this->manipulationMode));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
		}
	}

	void EditorManager::rotateObjects(void)
	{
		/*MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("DebugLabel")->setCaption("-f: " + Ogre::StringConverter::toString(Ogre::Math::RadiansToDegrees(fromAngle))
		+ " t: " + Ogre::StringConverter::toString(Ogre::Math::RadiansToDegrees(toAngle)));*/

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow")->setCaption("");

		this->oldGizmoOrientation = this->gizmo->getOrientation();

		bool yRotation = false;
		Ogre::Vector3 gizmoDirectionX = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().xAxis();
		Ogre::Vector3 gizmoDirectionY = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().yAxis();
		Ogre::Vector3 gizmoDirectionZ = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().zAxis();

		Ogre::Vector3 destDir = this->hitPoint - this->gizmo->getPosition();
		destDir.normalise();
		Ogre::Vector3 srcDir = this->oldHitPoint - this->gizmo->getPosition();
		srcDir.normalise();

		Ogre::Vector3 firstHitDirection = this->startHitPoint - this->gizmo->getPosition();
		firstHitDirection.normalise();

		Ogre::Quaternion rotation = Ogre::Quaternion::IDENTITY;

		// Check whether the absolute rotation is negative or positive. Positive means counter clockwise
		bool counterClockWise = (this->absoluteAngle >= 0.0f);

		switch (this->gizmo->getState())
		{
		case Gizmo::GIZMO_ARROW_X:
		{
			if (EDITOR_TRANSLATE_MODE_STACK == this->placeMode || EDITOR_TRANSLATE_MODE_STACK_ORIENTATED == this->placeMode)
			{
				yRotation = true;
			}
			// Get the correct angle with direction sign from src hit direction to dest hit direction, which is just a relative delta, calculated between this function calls
			Ogre::Radian angleDelta = MathHelper::getInstance()->getAngle(srcDir, destDir, gizmoDirectionY, true);
			// Calculate the absolute angle
			this->absoluteAngle -= angleDelta.valueRadians();

			Ogre::Real fromAngle = MathHelper::getInstance()->getAngle(firstHitDirection, this->rotateStartOrientationGizmoX.xAxis(), gizmoDirectionY, true).valueRadians();
			// Get the end angle, which is from + absolute angle. Note that absolute angle is necessary to avoid skweing when going over 180 degree,
			// so its possible to rotate e.g. 1243 degree
			Ogre::Real toAngle = fromAngle + this->absoluteAngle;

			this->gizmo->drawCircle(this->rotateStartOrientationGizmoX, fromAngle, toAngle, counterClockWise, 1.0f, "TransparentRedNoLighting");

			if (this->gridStep > 0.0f)
			{
				this->stepAngleDelta += Ogre::Math::Abs(angleDelta.valueDegrees());
				if (this->stepAngleDelta >= this->gridStep)
				{
					if (angleDelta.valueDegrees() <= 0.0f)
					{
						rotation = Ogre::Quaternion(Ogre::Degree(-this->gridStep), Ogre::Vector3::UNIT_Y);
					}
					else
					{
						rotation = Ogre::Quaternion(Ogre::Degree(this->gridStep), Ogre::Vector3::UNIT_Y);
					}
					// this->absoluteAngle = MathHelper::getInstance()->calculateRotationGridValue(this->gridStep, this->absoluteAngle);
					this->absoluteAngle = MathHelper::getInstance()->calculateRotationGridValue(this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated(), this->gridStep, this->absoluteAngle);
					this->stepAngleDelta -= this->gridStep;
				}
			}
			else
			{
				rotation = Ogre::Quaternion(angleDelta, Ogre::Vector3::UNIT_Y);
			}
			this->gizmo->setRotationCaption(Ogre::StringConverter::toString(Ogre::Math::RadiansToDegrees(this->absoluteAngle)), Ogre::ColourValue::Black);

			break;
		}
		case Gizmo::GIZMO_ARROW_Y:
		{
			Ogre::Radian angleDelta = MathHelper::getInstance()->getAngle(srcDir, destDir, gizmoDirectionZ, true);
			this->absoluteAngle -= angleDelta.valueRadians();

			Ogre::Real fromAngle = MathHelper::getInstance()->getAngle(firstHitDirection, this->rotateStartOrientationGizmoY.xAxis(), gizmoDirectionZ, true).valueRadians();
			Ogre::Real toAngle = fromAngle + this->absoluteAngle;

			this->gizmo->drawCircle(this->rotateStartOrientationGizmoY, fromAngle, toAngle, counterClockWise, 1.0f, "TransparentGreenNoLighting");
			if (this->gridStep > 0.0f)
			{
				this->stepAngleDelta += Ogre::Math::Abs(angleDelta.valueDegrees());
				if (this->stepAngleDelta >= this->gridStep)
				{
					if (angleDelta.valueDegrees() <= 0.0f)
					{
						rotation = Ogre::Quaternion(Ogre::Degree(-this->gridStep), Ogre::Vector3::UNIT_Z);
					}
					else
					{
						rotation = Ogre::Quaternion(Ogre::Degree(this->gridStep), Ogre::Vector3::UNIT_Z);
					}
					this->absoluteAngle = MathHelper::getInstance()->calculateRotationGridValue(this->gridStep, this->absoluteAngle);
					this->stepAngleDelta -= this->gridStep;
				}
			}
			else
			{
				rotation = Ogre::Quaternion(angleDelta, Ogre::Vector3::UNIT_Z);
			}
			this->gizmo->setRotationCaption(Ogre::StringConverter::toString(Ogre::Math::RadiansToDegrees(this->absoluteAngle)), Ogre::ColourValue::Black);

			break;
		}
		case Gizmo::GIZMO_ARROW_Z:
		{
			// Z was really hard to get right, because it must be orientated 270 degree about z instead of 90, see gizmo.cpp!
			Ogre::Radian angleDelta = MathHelper::getInstance()->getAngle(srcDir, destDir, gizmoDirectionX, true);
			this->absoluteAngle -= angleDelta.valueRadians();

			Ogre::Real fromAngle = MathHelper::getInstance()->getAngle(firstHitDirection, this->rotateStartOrientationGizmoZ.xAxis(), gizmoDirectionX, true).valueRadians();
			Ogre::Real toAngle = fromAngle + this->absoluteAngle;

			this->gizmo->drawCircle(this->rotateStartOrientationGizmoZ, fromAngle, toAngle, counterClockWise, 1.0f, "TransparentBlueNoLighting");

			if (this->gridStep > 0.0f)
			{
				this->stepAngleDelta += Ogre::Math::Abs(angleDelta.valueDegrees());
				if (this->stepAngleDelta >= this->gridStep)
				{
					if (angleDelta.valueDegrees() <= 0.0f)
					{
						rotation = Ogre::Quaternion(Ogre::Degree(-this->gridStep), Ogre::Vector3::UNIT_X);
					}
					else
					{
						rotation = Ogre::Quaternion(Ogre::Degree(this->gridStep), Ogre::Vector3::UNIT_X);
					}
					this->absoluteAngle = MathHelper::getInstance()->calculateRotationGridValue(this->gridStep, this->absoluteAngle);
					this->stepAngleDelta -= this->gridStep;
				}
			}
			else
			{
				rotation = Ogre::Quaternion(angleDelta, Ogre::Vector3::UNIT_X);
			}
			this->gizmo->setRotationCaption(Ogre::StringConverter::toString(Ogre::Math::RadiansToDegrees(this->absoluteAngle)), Ogre::ColourValue::Black);

			break;
		}
		}

		this->gizmo->rotate(rotation);



		// Note: rotation is just current for one axis, but in order to rotate and translate objects correctly, rotation all axes are required, so use gizmoRotationDelta 
		Ogre::Quaternion gizmoRotationDelta = this->gizmo->getOrientation() * this->oldGizmoOrientation.Inverse();

		// Create a quaternion for each rotation
		Ogre::Quaternion qx(Ogre::Degree(gizmoRotationDelta.getPitch()), Ogre::Vector3::UNIT_X);
		Ogre::Quaternion qy(Ogre::Degree(gizmoRotationDelta.getYaw()), Ogre::Vector3::UNIT_Y);
		Ogre::Quaternion qz(Ogre::Degree(gizmoRotationDelta.getRoll()), Ogre::Vector3::UNIT_Z);

		// Apply the rotations
		Ogre::Quaternion combinedRotation = qx * qy * qz;

		auto& selectedGameObjects = this->selectionManager->getSelectedGameObjects();

		// Check if rotation is about y, and ray cast objects below to manipulate the height
		if (true == yRotation)
		{
			// Set the new position for either physics component or the game object
			for (auto& selectedGameObject : selectedGameObjects)
			{
				// Get y-data (stack translate mode?)
				auto hitData = this->getTranslateYData(selectedGameObject.second.gameObject);

				bool success = std::get<0>(hitData);
				Ogre::Real height = std::get<1>(hitData);
				Ogre::Vector3 normal = std::get<2>(hitData);

				auto& physicsComponent = makeStrongPtr(selectedGameObject.second.gameObject->getComponent<PhysicsComponent>());
				if (nullptr != physicsComponent)
				{
					if (EDITOR_ROTATE_MODE1 == this->manipulationMode)
					{
						Ogre::Quaternion orientation = physicsComponent->getOrientation();
						Ogre::Quaternion localRotation = combinedRotation * orientation;
						physicsComponent->setOrientation(localRotation);
					}
					else if (EDITOR_ROTATE_MODE2 == this->manipulationMode)
					{
						// Get the position and orientation of the entity
						Ogre::Vector3 position = selectedGameObject.second.gameObject->getPosition();
						Ogre::Quaternion orientation = selectedGameObject.second.gameObject->getOrientation();

						// Calculate the new position relative to the gizmo's center
						Ogre::Vector3 newPosition = rotateAroundPoint(position, this->gizmo->getPosition(), combinedRotation);

						// Set the new position and calculate the new orientation
						physicsComponent->setPosition(newPosition);

						// Rotate around the gizmo's position with local orientation
						Ogre::Quaternion localRotation = combinedRotation * orientation;
						physicsComponent->setOrientation(localRotation);

						if (Ogre::Vector3::ZERO != normal)
						{
							physicsComponent->setDirection(normal, Ogre::Vector3::NEGATIVE_UNIT_Y);
						}
					}
				}
				else
				{
					if (EDITOR_ROTATE_MODE1 == this->manipulationMode)
					{
						Ogre::Quaternion orientation = selectedGameObject.second.gameObject->getOrientation();
						Ogre::Quaternion localRotation = combinedRotation * orientation;
						selectedGameObject.second.gameObject->getSceneNode()->setOrientation(localRotation);
					}
					else if (EDITOR_ROTATE_MODE2 == this->manipulationMode)
					{				
						// Get the position and orientation of the entity
						Ogre::Vector3 position = selectedGameObject.second.gameObject->getPosition();
						Ogre::Quaternion orientation = selectedGameObject.second.gameObject->getOrientation();

						// Calculate the new position relative to the gizmo's center
						Ogre::Vector3 newPosition = rotateAroundPoint(position, this->gizmo->getPosition(), combinedRotation);

						// Set the new position and calculate the new orientation
						selectedGameObject.second.gameObject->getSceneNode()->setPosition(newPosition);

						// Rotate around the gizmo's position with local orientation
						Ogre::Quaternion localRotation = combinedRotation * orientation;
						selectedGameObject.second.gameObject->getSceneNode()->setOrientation(localRotation);

						if (Ogre::Vector3::ZERO != normal)
						{
							selectedGameObject.second.gameObject->getSceneNode()->setDirection(normal, Ogre::Node::TS_PARENT, Ogre::Vector3::NEGATIVE_UNIT_Y);
						}
					}
				}
			}
		}
		else
		{
			unsigned int i = 0;
			// Set the new position for either physics component or the game object
			for (auto& selectedGameObject : selectedGameObjects)
			{
				auto& physicsComponent = makeStrongPtr(selectedGameObject.second.gameObject->getComponent<PhysicsComponent>());
				if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
				{
					if (EDITOR_ROTATE_MODE1 == this->manipulationMode)
					{
						Ogre::Quaternion orientation = physicsComponent->getOrientation();
						Ogre::Quaternion localRotation = combinedRotation * orientation;
						physicsComponent->setOrientation(localRotation);
					}
					else if (EDITOR_ROTATE_MODE2 == this->manipulationMode)
					{
						// Get the position and orientation of the entity
						Ogre::Vector3 position = selectedGameObject.second.gameObject->getPosition();
						Ogre::Quaternion orientation = selectedGameObject.second.gameObject->getOrientation();

						// Calculate the new position relative to the gizmo's center
						Ogre::Vector3 newPosition = rotateAroundPoint(position, this->gizmo->getPosition(), combinedRotation);

						// Set the new position and calculate the new orientation
						physicsComponent->setPosition(newPosition);

						// Rotate around the gizmo's position with local orientation
						Ogre::Quaternion localRotation = combinedRotation * orientation;
						physicsComponent->setOrientation(localRotation);
					}
				}
				else
				{
					if (EDITOR_ROTATE_MODE1 == this->manipulationMode)
					{
						Ogre::Quaternion orientation = selectedGameObject.second.gameObject->getOrientation();
						Ogre::Quaternion localRotation = combinedRotation * orientation;
						selectedGameObject.second.gameObject->getSceneNode()->setOrientation(localRotation);
					}
					else if (EDITOR_ROTATE_MODE2 == this->manipulationMode)
					{
						// Get the position and orientation of the entity
						Ogre::Vector3 position = selectedGameObject.second.gameObject->getPosition();
						Ogre::Quaternion orientation = selectedGameObject.second.gameObject->getOrientation();

						// Calculate the new position relative to the gizmo's center
						Ogre::Vector3 newPosition = rotateAroundPoint(position, this->gizmo->getPosition(), combinedRotation);

						// Set the new position and calculate the new orientation
						selectedGameObject.second.gameObject->getSceneNode()->setPosition(newPosition);

						// Rotate around the gizmo's position with local orientation
						Ogre::Quaternion localRotation = combinedRotation * orientation;
						selectedGameObject.second.gameObject->getSceneNode()->setOrientation(localRotation);
					}
				}
				i++;
			}
		}
	}

	bool EditorManager::getRayStartPoint(const Ogre::Ray& hitRay)
	{
		bool success = this->getRayHitPoint(hitRay);
		// if the gizmo was hit
		if (true == success)
		{
			// actualize the startpoint
			this->oldHitPoint = this->hitPoint;
			this->isGizmoMoving = true;
		}
		return success;
	}

	std::tuple<bool, Ogre::Real, Ogre::Vector3> EditorManager::getTranslateYData(GameObject* gameObject)
	{
		bool success = false;
		Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		// Depending on translate mode adjust the height, or orientation of GO's
		switch (this->translateMode)
		{
		case EDITOR_TRANSLATE_MODE_STACK:
		case EDITOR_TRANSLATE_MODE_STACK_ORIENTATED:
		{
			Ogre::v1::Entity* hitMovableObject = nullptr;
			Ogre::Real closestDistance = 0.0f;

			// Shoot ray at object position down and exclude that object and gizmo
			Ogre::Ray hitRay = Ogre::Ray(gameObject->getSceneNode()->getPosition() + Ogre::Vector3(0.0f, gameObject->getSize().y * 10.0f, 0.0f), Ogre::Vector3::NEGATIVE_UNIT_Y);
			// Check if there is an hit with an polygon of an entity and stack the to be placed object
			this->placeObjectQuery->setRay(hitRay);

			std::vector<Ogre::MovableObject*> excludeMovableObjects(5);
			excludeMovableObjects[0] = gameObject->getMovableObject<Ogre::v1::Entity>();
			excludeMovableObjects[1] = this->gizmo->getArrowEntityX();
			excludeMovableObjects[2] = this->gizmo->getArrowEntityY();
			excludeMovableObjects[3] = this->gizmo->getArrowEntityZ();
			excludeMovableObjects[4] = this->gizmo->getSphereEntity();
			MathHelper::getInstance()->getRaycastFromPoint(this->placeObjectQuery, this->camera, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects);

			// If nothing to stack to, calc the hit point on floor plane
			if (nullptr == hitMovableObject)
			{
				internalHitPoint = this->getHitPointOnFloor(hitRay);
			}
			// Move the gizmo to the bottom center of the entity mesh
			Ogre::v1::Entity* entity = gameObject->getMovableObject<Ogre::v1::Entity>();
			if (entity)
			{
				internalHitPoint += MathHelper::getInstance()->getBottomCenterOfMesh(gameObject->getSceneNode(), gameObject->getMovableObject<Ogre::v1::Entity>());
			}
			success = true;
			break;
		}
		}

		if (EDITOR_TRANSLATE_MODE_STACK_ORIENTATED != this->translateMode)
		{
			normal = Ogre::Vector3::ZERO;
		}

		return std::make_tuple(success, internalHitPoint.y, normal);
	}

	Ogre::Vector3 EditorManager::calculateGizmoGridTranslation(const Ogre::Vector3& gridFactor, const Ogre::Vector3& internalHitPoint, Ogre::MovableObject* movableObject)
	{
		Ogre::Vector3 newPos = internalHitPoint;
		Ogre::Vector3 localPoint = movableObject->getParentNode()->_getDerivedOrientationUpdated().Inverse() * (newPos - movableObject->getParentNode()->_getDerivedPositionUpdated());

		// Round the local point to the nearest grid size, considering different sizes for x and z axes
		localPoint.x = round(localPoint.x / gridFactor.x) * gridFactor.x;
		localPoint.y = round(localPoint.y / gridFactor.y) * gridFactor.y;
		localPoint.z = round(localPoint.z / gridFactor.z) * gridFactor.z;
		newPos = movableObject->getParentNode()->_getDerivedOrientationUpdated() * localPoint + movableObject->getParentNode()->_getDerivedPositionUpdated();

		return newPos;
	}

	Ogre::Vector3 EditorManager::rotateAroundPoint(const Ogre::Vector3& point, const Ogre::Vector3& center, const Ogre::Quaternion& rotation)
	{
		Ogre::Vector3 offset = point - center;
		offset = rotation * offset;
		return center + offset;
	}

	void EditorManager::moveObjects(Ogre::Vector3& offset)
	{
		if (this->gridStep > 0.0f)
		{
			Ogre::Vector3 oldGridPos;
			Ogre::Vector3 newGridPos;
			// Special case, the grid has the size of the objects bounding box
			if (1 == this->getSelectionManager()->getSelectedGameObjects().size() && GetAsyncKeyState(VK_LMENU))
			{
				auto& gameObject = this->getSelectionManager()->getSelectedGameObjects().begin()->second.gameObject;

				oldGridPos = MathHelper::getInstance()->calculateGridValue(this->gridStep, this->gizmo->getPosition());

				this->gizmo->translate(offset);
				newGridPos = MathHelper::getInstance()->calculateGridValue(this->gridStep, this->gizmo->getPosition());
			}
			else
			{
				//first calculate the old grid position
				oldGridPos = MathHelper::getInstance()->calculateGridValue(1.0f, this->gizmo->getPosition());

				//translate the select node to the new position
				this->gizmo->translate(offset);

				// then calculate the new grid position after the select node has been translated
				newGridPos = MathHelper::getInstance()->calculateGridValue(1.0f, this->gizmo->getPosition());
			}
			//translate the objects to the new mouse position
			offset = newGridPos - oldGridPos;
		}
		else
		{
			// translate the select node to the new position directly
			this->gizmo->translate(offset);
		}

		auto& selectedGameObjects = this->selectionManager->getSelectedGameObjects();

		size_t i = 0;
		// Set the new position for either physics component or the game object
		for (auto& selectedGameObject : selectedGameObjects)
		{
			// Get y-data (stack translate mode?)
			auto hitData = this->getTranslateYData(selectedGameObject.second.gameObject);

			bool success = std::get<0>(hitData);
			Ogre::Real height = std::get<1>(hitData);
			Ogre::Vector3 normal = std::get<2>(hitData);

			if (true == success)
			{
				offset = Ogre::Vector3(offset.x, 0.0f, offset.z);
			}
			auto& physicsComponent = makeStrongPtr(selectedGameObject.second.gameObject->getComponent<PhysicsComponent>());

			if (nullptr != physicsComponent && physicsComponent->getBody() != nullptr)
			{
				/*if (1 == this->getSelectionManager()->getSelectedGameObjects().size())
				{
					Ogre::Vector3 pos = physicsComponent->getPosition();
					if (this->constraintAxis.x != 0.0f)
						physicsComponent->setPosition(Ogre::Vector3(this->constraintAxis.x, pos.y, pos.z));
					if (this->constraintAxis.y != 0.0f)
						physicsComponent->setPosition(Ogre::Vector3(pos.x, this->constraintAxis.y, pos.z));
					if (this->constraintAxis.z != 0.0f)
						physicsComponent->setPosition(Ogre::Vector3(pos.x, pos.y, this->constraintAxis.z));
				}*/

				if (true == success)
				{
					physicsComponent->setPosition(physicsComponent->getPosition().x, height, physicsComponent->getPosition().z);
				}

				if (GetAsyncKeyState(VK_LMENU))
				{
					Ogre::Vector3 internalHitPoint = this->gizmo->getPosition() + this->selectedObjectsStartOffsets[i];
					// Ogre::Vector3 internalHitPoint = selectedGameObject.second.gameObject->getPosition() - this->gizmo->getPosition();
					Ogre::Vector3 newGridPoint = this->calculateGizmoGridTranslation(this->boundingBoxSize * this->gridStep, internalHitPoint, selectedGameObject.second.gameObject->getMovableObject());

					if (this->constraintAxis.x != 0.0f)
						newGridPoint = Ogre::Vector3(this->constraintAxis.x, newGridPoint.y, newGridPoint.z);
					if (this->constraintAxis.y != 0.0f)
						newGridPoint = Ogre::Vector3(newGridPoint.x, this->constraintAxis.y, newGridPoint.z);
					if (this->constraintAxis.z != 0.0f)
						newGridPoint = Ogre::Vector3(newGridPoint.x, newGridPoint.y, this->constraintAxis.z);

					physicsComponent->setPosition(newGridPoint);
				}
				else
				{
					physicsComponent->translate(offset);
				}
				if (Ogre::Vector3::ZERO != normal)
				{
					physicsComponent->setDirection(normal, Ogre::Vector3::NEGATIVE_UNIT_Y);
				}
			}
			else
			{
				/*Ogre::Vector3 pos = selectedGameObject.second->getSceneNode()->getPosition();
				if (this->constraintAxis.x != 0.0f)
					selectedGameObject.second->getSceneNode()->setPosition(Ogre::Vector3(this->constraintAxis.x, pos.y, pos.z));
				if (this->constraintAxis.y != 0.0f)
					selectedGameObject.second->getSceneNode()->setPosition(Ogre::Vector3(pos.x, this->constraintAxis.y, pos.z));
				if (this->constraintAxis.z != 0.0f)
					selectedGameObject.second->getSceneNode()->setPosition(Ogre::Vector3(pos.x, pos.y, this->constraintAxis.z));*/
				
				if (true == success)
				{
					selectedGameObject.second.gameObject->getSceneNode()->setPosition(selectedGameObject.second.gameObject->getSceneNode()->getPosition().x, height, 
						selectedGameObject.second.gameObject->getSceneNode()->getPosition().z);
				}

				if (GetAsyncKeyState(VK_LMENU))
				{
					Ogre::Vector3 internalHitPoint = this->gizmo->getPosition()/* + this->selectedObjectsStartOffsets[i]*/;
					// Ogre::Vector3 internalHitPoint = selectedGameObject.second.gameObject->getPosition() - this->gizmo->getPosition();
					
					Ogre::Vector3 newGridPoint = this->calculateGizmoGridTranslation(this->boundingBoxSize * this->gridStep, internalHitPoint, selectedGameObject.second.gameObject->getMovableObject());
					if (this->constraintAxis.x != 0.0f)
						newGridPoint = Ogre::Vector3(this->constraintAxis.x, newGridPoint.y, newGridPoint.z);
					if (this->constraintAxis.y != 0.0f)
						newGridPoint = Ogre::Vector3(newGridPoint.x, this->constraintAxis.y, newGridPoint.z);
					if (this->constraintAxis.z != 0.0f)
						newGridPoint = Ogre::Vector3(newGridPoint.x, newGridPoint.y, this->constraintAxis.z);

					selectedGameObject.second.gameObject->getSceneNode()->setPosition(newGridPoint);
				}
				else
				{
					selectedGameObject.second.gameObject->getSceneNode()->translate(offset);
				}
				
				if (Ogre::Vector3::ZERO != normal)
				{
					selectedGameObject.second.gameObject->getSceneNode()->setDirection(normal, Ogre::Node::TS_PARENT, Ogre::Vector3::NEGATIVE_UNIT_Y);
				}
			}

			i++;
		}

		// Draw line and set caption with current translation offset
		Ogre::Vector3 translatedDistanceVector;
		if (this->gridStep > 0.0f)
		{
			translatedDistanceVector = MathHelper::getInstance()->calculateGridValue(this->gridStep, this->gizmo->getPosition() - this->translateStartPoint);
		}
		else
		{
			translatedDistanceVector = this->gizmo->getPosition() - this->translateStartPoint;
		}

		switch (this->gizmo->getState())
		{
		case Gizmo::GIZMO_ARROW_X:
		{
			this->gizmo->drawLine(this->translateStartPoint, this->gizmo->getPosition(), 1.0f, "RedNoLighting");
			this->gizmo->setTranslationCaption(Ogre::StringConverter::toString(translatedDistanceVector.x), Ogre::ColourValue::Black);
			break;
		}
		case Gizmo::GIZMO_ARROW_Y:
		{
			this->gizmo->drawLine(this->translateStartPoint, this->gizmo->getPosition(), 1.0f, "GreenNoLighting");
			this->gizmo->setTranslationCaption(Ogre::StringConverter::toString(translatedDistanceVector.y), Ogre::ColourValue::Black);
			break;
		}
		case Gizmo::GIZMO_ARROW_Z:
		{
			this->gizmo->drawLine(this->translateStartPoint, this->gizmo->getPosition(), 1.0f, "BlueNoLighting");
			this->gizmo->setTranslationCaption(Ogre::StringConverter::toString(translatedDistanceVector.z), Ogre::ColourValue::Black);
			break;
		}
		case Gizmo::GIZMO_SPHERE:
		{
			this->gizmo->drawLine(this->translateStartPoint, this->gizmo->getPosition(), 1.0f, "YellowNoLighting");
			this->gizmo->setTranslationCaption(Ogre::StringConverter::toString(translatedDistanceVector), Ogre::ColourValue(1.0f, 0.0f, 1.0f, 1.0f));
			break;
		}
		}
	}

	void EditorManager::scaleObjects(Ogre::Vector3& offset)
	{
		auto& selectedGameObjects = this->selectionManager->getSelectedGameObjects();
		for (auto& entry : selectedGameObjects)
		{
			if (this->gridStep > 0.0f)
			{
				//first calculate the old grid position
				Ogre::Vector3 oldGridPos = MathHelper::getInstance()->calculateGridValue(this->gridStep, this->gizmo->getPosition());

				//translate the select node to the new position
				this->gizmo->translate(offset);

				// then calculate the new grid position after the select node has been translated
				Ogre::Vector3 newGridPos = MathHelper::getInstance()->calculateGridValue(this->gridStep, this->gizmo->getPosition());
				//translate the objects to the new mouse position
				offset = newGridPos - oldGridPos;
			}

#if 0
			Ogre::Vector3 distanceVec = this->hitPoint - this->oldHitPoint;
			Ogre::Real distance = this->oldHitPoint.distance(this->hitPoint);
			Ogre::Real dot = this->gizmo->getCurrentDirection().dotProduct(distanceVec);

			Ogre::Real sign = 1.0f;
			if (dot < 0.0f)
			{
				sign = -1.0f;
			}
#endif
			Ogre::Vector3 scaleFactor = offset;
			{
				// vertices[i + subMeshOffset] = (orientation * (vec * scale)) + pos;

				auto& phyicsComponent = makeStrongPtr(entry.second.gameObject->getComponent<PhysicsComponent>());
				if (nullptr != phyicsComponent)
				{
					phyicsComponent->setScale(phyicsComponent->getScale() + scaleFactor);
				}
				else
				{
					entry.second.gameObject->getSceneNode()->scale(scaleFactor);
				}
			}
		}
	}

	void EditorManager::setConstraintAxis(const Ogre::Vector3& constraintAxis)
	{
		this->constraintAxis = constraintAxis;
		if (nullptr != this->gizmo)
		{
			this->gizmo->setConstraintAxis(constraintAxis);
		}
	}

	Ogre::Vector3 EditorManager::calculateUnitScaleFactor(const Ogre::Vector3& translateVector)
	{
		Ogre::Vector3 distanceVec = this->hitPoint - this->oldHitPoint;
		Ogre::Real distance = this->oldHitPoint.distance(this->hitPoint);
		Ogre::Real dot = this->gizmo->getCurrentDirection().dotProduct(distanceVec);
		
		Ogre::Vector3 unitScale;

		if (dot < 0.0f)
		{
			//scale the object with the half length of the vector in the negative direction
			unitScale.x = -translateVector.length() / 2.0f;
			unitScale.y = -translateVector.length() / 2.0f;
			unitScale.z = -translateVector.length() / 2.0f;

		}
		else
		{
			//scale the object with the half length of the vector
			unitScale.x = translateVector.length() / 2.0f;
			unitScale.y = translateVector.length() / 2.0f;
			unitScale.z = translateVector.length() / 2.0f;
		}
		return std::move(unitScale);
	}

	void EditorManager::update(Ogre::Real dt)
	{
		this->gizmo->update();
		this->viewportGrid->update();

		if (EDITOR_PLACE_MODE == this->manipulationMode)
		{
			this->rotatePlaceNode();
		}
		else if (EDITOR_PICKER_MODE == this->manipulationMode && false == this->selectionManager->getIsSelectingRectangle())
		{
			this->pickForce += static_cast<Ogre::Real>(NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState().Z.rel) / 100.0f;
		}
		else if (true == this->mouseIdPressed && (EDITOR_TERRAIN_MODIFY_MODE == this->manipulationMode || EDITOR_TERRAIN_SMOOTH_MODE == this->manipulationMode || EDITOR_TERRAIN_PAINT_MODE == this->manipulationMode))
		{
			if (nullptr != this->terraComponent)
			{
				auto hitData = this->terraComponent->checkRayIntersect(this->mouseHitRay);
				if (true == std::get<0>(hitData))
				{
					if (EDITOR_TERRAIN_MODIFY_MODE == this->manipulationMode)
					{
						this->terraComponent->modifyTerrain(std::get<1>(hitData), this->terraComponent->getStrength() * dt * 60.0f);
					}
					else if (EDITOR_TERRAIN_SMOOTH_MODE == this->manipulationMode)
					{
						this->terraComponent->smoothTerrain(std::get<1>(hitData), this->terraComponent->getStrength() * dt * 60.0f);
					}
					else if (EDITOR_TERRAIN_PAINT_MODE == this->manipulationMode)
					{
						this->terraComponent->paintTerrain(std::get<1>(hitData), this->terraComponent->getBrushIntensity() * dt * 60.0f, this->terraComponent->getImageLayerId());
					}
				}
			}
		}

		if (this->timeSinceLastUpdate <= 0.0f)
		{
			if (true == this->selectionManager->getSelectedGameObjects().empty())
			{
				this->gizmo->setEnabled(false);
			}
			this->timeSinceLastUpdate = 1.0f;
		}
		else
		{
			this->timeSinceLastUpdate -= dt;
		}
	}

	Ogre::Vector3 EditorManager::calculateCenter(void)
	{
		// Calculate the center of all selected objects
		// this algorithmus is based on the arithmetic average
		Ogre::Vector3 cumulated = Ogre::Vector3::ZERO;
		auto& selectedGameObjects = this->selectionManager->getSelectedGameObjects();
		int count = static_cast<int>(selectedGameObjects.size());

		if (0 == count)
		{
			return Ogre::Vector3::ZERO;
		}

		for (auto& entry : selectedGameObjects)
		{
			cumulated += entry.second.gameObject->getSceneNode()->getPosition();
		}

		Ogre::Vector3 center = cumulated / static_cast<Ogre::Real>(count);
		return std::move(center);
	}

	void EditorManager::setGizmoToGameObjectsCenter(void)
	{
		// Calculate the center to put the gizmo there
		this->gizmo->setPosition(this->calculateCenter());
		this->viewportGrid->setPosition(this->gizmo->getPosition());

		// Force correct gizmo orientation
		if (this->selectionManager->getSelectedGameObjects().size() == 1)
		{
			this->gizmo->setOrientation(this->selectionManager->getSelectedGameObjects().begin()->second.gameObject->getOrientation());
			this->viewportGrid->setOrientation(this->gizmo->getOrientation());
		}
		else
		{
			this->gizmo->setOrientation(Ogre::Quaternion::IDENTITY);
			this->viewportGrid->setOrientation(Ogre::Quaternion::IDENTITY);
		}

		this->boundingBoxSize == Ogre::Vector3::ZERO;
		this->selectedObjectsStartOffsets.clear();

		auto& selectedGameObjects = this->selectionManager->getSelectedGameObjects();

		// Calculates the global bounding box for grid movement
		Ogre::Vector3 minCorner(std::numeric_limits<Ogre::Real>::max(), std::numeric_limits<Ogre::Real>::max(), std::numeric_limits<Ogre::Real>::max());
		Ogre::Vector3 maxCorner(std::numeric_limits<Ogre::Real>::lowest(), std::numeric_limits<Ogre::Real>::lowest(), std::numeric_limits<Ogre::Real>::lowest());

		for (auto& entry : selectedGameObjects)
		{
			
			Ogre::Matrix4 transformMatrix = entry.second.gameObject->getSceneNode()->_getFullTransform();

			for (unsigned int i = 0; i < entry.second.gameObject->getSceneNode()->numAttachedObjects(); ++i)
			{
				Ogre::MovableObject* obj = entry.second.gameObject->getSceneNode()->getAttachedObject(i);
				Ogre::Aabb localBox = obj->getLocalAabb();

				Ogre::Vector3 localMin = localBox.getMinimum();
				Ogre::Vector3 localMax = localBox.getMaximum();

				// Transform local AABB corners to world space
				Ogre::Vector3 worldCorners[8];
				worldCorners[0] = transformMatrix * localMin;
				worldCorners[1] = transformMatrix * Ogre::Vector3(localMin.x, localMin.y, localMax.z);
				worldCorners[2] = transformMatrix * Ogre::Vector3(localMin.x, localMax.y, localMin.z);
				worldCorners[3] = transformMatrix * Ogre::Vector3(localMin.x, localMax.y, localMax.z);
				worldCorners[4] = transformMatrix * Ogre::Vector3(localMax.x, localMin.y, localMin.z);
				worldCorners[5] = transformMatrix * Ogre::Vector3(localMax.x, localMin.y, localMax.z);
				worldCorners[6] = transformMatrix * Ogre::Vector3(localMax.x, localMax.y, localMin.z);
				worldCorners[7] = transformMatrix * localMax;

				// Update the bounding box corners relative to the gizmo position
				for (auto& corner : worldCorners)
				{
					corner -= this->gizmo->getPosition(); // Adjust relative to gizmo position
					minCorner.makeFloor(corner);
					maxCorner.makeCeil(corner);
				}
			}

			Ogre::Vector3 offset = entry.second.gameObject->getSceneNode()->getPosition() - this->gizmo->getPosition();
			this->selectedObjectsStartOffsets.emplace_back(offset);
		}

		this->boundingBoxSize = maxCorner - minCorner;
	}

	Ogre::SceneManager* EditorManager::getSceneManager(void) const
	{
		return this->sceneManager;
	}

	Gizmo* EditorManager::getGizmo(void)
	{
		return this->gizmo;
	}

	SelectionManager* EditorManager::getSelectionManager(void) const
	{
		return this->selectionManager;
	}

	void EditorManager::setManipulationMode(EditorManager::eManipulationMode manipulationMode)
	{
		this->manipulationMode = manipulationMode;
		if (EDITOR_SELECT_MODE == this->manipulationMode || EDITOR_PICKER_MODE == this->manipulationMode || EDITOR_PLACE_MODE == this->manipulationMode)
		{
			this->gizmo->setEnabled(false);
			this->isGizmoMoving = false;
		}
		else if (EDITOR_TERRAIN_MODIFY_MODE == this->manipulationMode || EDITOR_TERRAIN_SMOOTH_MODE == this->manipulationMode || EDITOR_TERRAIN_PAINT_MODE == this->manipulationMode)
		{
			auto terraGameObjectList = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromType(GameObject::TERRA);
			for (auto terraGameObjectPtr : terraGameObjectList)
			{
				if (nullptr != terraGameObjectPtr)
				{
					auto terraCompPtr = NOWA::makeStrongPtr(terraGameObjectPtr->getComponent<TerraComponent>());
					if (nullptr != terraCompPtr)
					{
						this->terraComponent = terraCompPtr.get();
					}
				}
			}
		}
		else
		{
			this->gizmo->setPosition(this->calculateCenter());
			this->viewportGrid->setPosition(this->gizmo->getPosition());
			if (this->selectionManager->getSelectedGameObjects().size() == 1)
			{
				this->gizmo->setOrientation(this->selectionManager->getSelectedGameObjects().begin()->second.gameObject->getOrientation());
				this->viewportGrid->setOrientation(this->gizmo->getOrientation());
			}
			else
			{
				this->viewportGrid->setOrientation(Ogre::Quaternion::IDENTITY);
				this->gizmo->setOrientation(Ogre::Quaternion::IDENTITY);
			}
			this->gizmo->setEnabled(true);
			if (EDITOR_TRANSLATE_MODE == this->manipulationMode)
			{
				this->gizmo->changeToTranslateGizmo();
			}
			else if (EDITOR_SCALE_MODE == this->manipulationMode)
			{
				this->gizmo->changeToScaleGizmo();
			}
			else if (EDITOR_ROTATE_MODE1 == this->manipulationMode || EDITOR_ROTATE_MODE2 == this->manipulationMode)
			{
				this->gizmo->changeToRotateGizmo();
			}
		}
		if (this->manipulationMode != EDITOR_PLACE_MODE)
		{
			this->deactivatePlaceMode();
		}
	}

	EditorManager::eManipulationMode EditorManager::getManipulationMode(void) const
	{
		return this->manipulationMode;
	}

	void EditorManager::setPlaceMode(EditorManager::ePlaceMode placeMode)
	{
		this->placeMode = placeMode;
	}

	EditorManager::ePlaceMode EditorManager::getPlaceMode(void) const
	{
		return this->placeMode;
	}

	void EditorManager::setTranslateMode(EditorManager::eTranslateMode translateMode)
	{
		this->translateMode = translateMode;
	}

	EditorManager::eTranslateMode EditorManager::getTranslateMode(void) const
	{
		return this->translateMode;
	}

	void EditorManager::setCameraView(EditorManager::eCameraView cameraView)
	{
		this->cameraView = cameraView;
		switch (cameraView)
		{
		case EDITOR_CAMERA_VIEW_FRONT:
		{
			this->camera->setOrientation(Ogre::Quaternion(Ogre::Degree(0.0f), Ogre::Vector3::UNIT_Y));
			break;
		}
		case EDITOR_CAMERA_VIEW_TOP:
		{
			this->camera->setOrientation(Ogre::Quaternion(Ogre::Degree(270.0f), Ogre::Vector3::UNIT_X));
			break;
		}
		case EDITOR_CAMERA_VIEW_BACK:
		{
			this->camera->setOrientation(Ogre::Quaternion(Ogre::Degree(-180.0f), Ogre::Vector3::UNIT_Y));
			break;
		}
		case EDITOR_CAMERA_VIEW_BOTTOM:
		{
			this->camera->setOrientation(Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::UNIT_X));;
			break;
		}
		case EDITOR_CAMERA_VIEW_LEFT:
		{
			this->camera->setOrientation(Ogre::Quaternion(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_Y));
			break;
		}
		case EDITOR_CAMERA_VIEW_RIGHT:
		{
			this->camera->setOrientation(Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::UNIT_Y));
			break;
		}
		}
	}

	EditorManager::eCameraView  EditorManager::getCameraView(void) const
	{
		return this->cameraView;
	}

	void EditorManager::deactivatePlaceMode(void)
	{
		this->destroyTempPlaceMovableObjectNode();
		this->placeNode->setVisible(false);
		// Delete the group
		for (size_t i = 0; i < this->groupGameObjectIds.size(); i++)
		{
			GameObjectPtr thisGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(std::get<0>(this->groupGameObjectIds[i]));
			AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(thisGameObjectPtr);
		}
		this->groupGameObjectIds.clear();
		
	}

	void EditorManager::setGridStep(Ogre::Real gridStep)
	{
		this->gridStep = gridStep;
		this->viewportGrid->setPerspectiveSize(this->gridStep);
	}

	Ogre::Real EditorManager::getGridStep(void) const
	{
		return this->gridStep;
	}

	void EditorManager::setViewportGridEnabled(bool enable)
	{
		this->viewportGrid->setEnabled(enable);
	}

	bool EditorManager::getViewportGridEnabled(void) const
	{
		return this->viewportGrid->getEnabled();
	}

	std::vector<unsigned long> EditorManager::getSelectedGameObjectIds(void)
	{
		// Work with ids instead of pointers for undo/redo, because pointer may become invalid when deleted via undo/redo
		std::vector<unsigned long> gameObjectIds(this->selectionManager->getSelectedGameObjects().size());
		unsigned int i = 0;
		for (auto& it = this->selectionManager->getSelectedGameObjects().cbegin(); it != this->selectionManager->getSelectedGameObjects().cend(); ++it)
		{
			gameObjectIds[i++] = it->second.gameObject->getId();
		}
		return std::move(gameObjectIds);
	}

	std::vector<unsigned long> EditorManager::getAllGameObjectIds(void)
	{
		// Work with ids instead of pointers for undo/redo, because pointer may become invalid when deleted via undo/redo
		std::vector<unsigned long> gameObjectIds(AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->size());
		unsigned int i = 0;
		for (auto& it = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cbegin(); it != AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cend(); ++it)
		{
			gameObjectIds[i++] = it->second->getId();
		}
		return std::move(gameObjectIds);
	}

	void EditorManager::startSimulation(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[EditorManager] Simulation started");
		// Physics simulation will be corrupt!
		if (nullptr != AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt())
		{
			// Internally calls invalidate cache, so that all newton data is set to default for deterministic simulations, when started again
			AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()->cleanUp();
		}

		this->oldGameObjectDataList.clear();

		// Store the data from the selected game objects before any manipulation has been made
		for (auto& it = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cbegin(); it != AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cend(); ++it)
		{
			GameObjectData gameObjectData;
			gameObjectData.oldPosition = it->second->getPosition();
			gameObjectData.oldOrientation = it->second->getOrientation();
			gameObjectData.oldScale = it->second->getSceneNode()->getScale();
			gameObjectData.gameObjectId = it->second->getId();
			this->oldGameObjectDataList.emplace_back(std::move(gameObjectData));

			// Call also final init, because e.g. buoyance etc. will connect at this point important data
			// Is done in connectGameObjects
			// it->second->connect();
		}

		AppStateManager::getSingletonPtr()->getGameObjectController()->start();

		std::vector<unsigned long> gameObjectIds = this->getAllGameObjectIds();
		if (gameObjectIds.size() > 0)
		{
			this->sceneManipulationCommandModule.pushCommand(std::make_shared<GameObjectManipulationUndoCommand>(this->oldGameObjectDataList, gameObjectIds));
		}

		this->isInSimulation = true;
	}

	void EditorManager::stopSimulation(bool withUndo)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[EditorManager] Simulation stopped");

		this->isInSimulation = false;
		AppStateManager::getSingletonPtr()->getGameObjectController()->stop();
		if (true == withUndo)
		{
			this->undo();
		}
		// Physics simulation will be corrupt!
		if (nullptr != AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt())
		{
			// Internally calls invalidate cache, so that all newton data is set to default for deterministic simulations, when started again
			AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()->cleanUp();
		}
	}

	void EditorManager::snapshotSelectedGameObjects(void)
	{
		this->oldGameObjectDataList.clear();
		// Store the data from the selected game objects before any manipulation has been made
		for (auto& it = this->selectionManager->getSelectedGameObjects().begin(); it != this->selectionManager->getSelectedGameObjects().end(); ++it)
		{
			GameObjectData gameObjectData;
			gameObjectData.oldPosition = it->second.gameObject->getPosition();
			gameObjectData.oldOrientation = it->second.gameObject->getOrientation();
			gameObjectData.oldScale = it->second.gameObject->getSceneNode()->getScale();
			this->oldGameObjectDataList.emplace_back(std::move(gameObjectData));
		}
	}

	void EditorManager::snapshotOldGameObjectAttribute(std::vector<GameObject*> gameObjects, const Ogre::String& attributeName)
	{
		this->oldGameObjectDataList.clear();
		for (size_t i = 0; i < gameObjects.size(); i++)
		{
			// Get the attribute name and get the attributes from the game objects and its value
			// make a copy for the old attribute for undo/redo
			GameObjectData gameObjectData;
			gameObjectData.gameObjectId = gameObjects[i]->getId();
			// A copy must be made, so that the value is not overriden
			NOWA::Variant* currentAttribute = gameObjects[i]->getAttribute(attributeName);
			// Old attribute must be the value before several values had been set to the same value, so get from each game object
			gameObjectData.oldAttribute = currentAttribute->clone();

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "snapshotGameObjectAttribute name: " + gameObjectData.oldAttribute->getName()
			// 	+ " old value: " + gameObjectData.oldAttribute->getOldString() + " new value: " + gameObjectData.newAttribute->getString());
			this->oldGameObjectDataList.emplace_back(std::move(gameObjectData));
		}
	}

	void EditorManager::snapshotOldGameObjectComponentAttribute(std::vector<GameObject*> gameObjects, std::vector<GameObjectComponent*> gameObjectComponts, const Ogre::String& attributeName)
	{
		this->oldGameObjectDataList.clear();
		for (size_t i = 0; i < gameObjects.size(); i++)
		{
			// Get the attribute name and get the attributes from the game objects and its value
			// make a copy for the old attribute for undo/redo
			GameObjectData gameObjectData;
			gameObjectData.gameObjectId = gameObjects[i]->getId();
			// A copy must be made, so that the value is not overriden
			NOWA::Variant* currentAttribute = gameObjectComponts[i]->getAttribute(attributeName);
			// Old attribute must be the value before several values had been set to the same value, so get from each game object
			gameObjectData.oldAttribute = currentAttribute->clone();
			gameObjectData.componentIndex = gameObjects[i]->getIndexFromComponent(gameObjectComponts[i]);

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "snapshotGameObjectAttribute name: " + gameObjectData.oldAttribute->getName()
			// 	+ " old value: " + gameObjectData.oldAttribute->getOldString() + " new value: " + gameObjectData.newAttribute->getString());
			this->oldGameObjectDataList.emplace_back(std::move(gameObjectData));
		}
	}

	void EditorManager::snapshotNewGameObjectAttribute(Variant* newAttribute)
	{
		for (size_t i = 0; i < this->oldGameObjectDataList.size(); i++)
		{
			this->oldGameObjectDataList[i].newAttribute = newAttribute->clone();
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "snapshotGameObjectAttribute name: " + gameObjectData.oldAttribute->getName()
			// 	+ " old value: " + gameObjectData.oldAttribute->getOldString() + " new value: " + gameObjectData.newAttribute->getString());
		}

		// Push the Command
		this->sceneManipulationCommandModule.pushCommand(std::make_shared<AttributeGameObjectUndoCommand>(this->oldGameObjectDataList));
		// Sent event that scene has been modified
		boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
	}

	void EditorManager::snapshotNewGameObjectComponentAttribute(Variant* newAttribute)
	{
		for (size_t i = 0; i < this->oldGameObjectDataList.size(); i++)
		{
			this->oldGameObjectDataList[i].newAttribute = newAttribute->clone();
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "snapshotGameObjectAttribute name: " + gameObjectData.oldAttribute->getName()
			// 	+ " old value: " + gameObjectData.oldAttribute->getOldString() + " new value: " + gameObjectData.newAttribute->getString());
		}

		// Push the Command
		this->sceneManipulationCommandModule.pushCommand(std::make_shared<AttributeGameObjectComponentUndoCommand>(this->oldGameObjectDataList));
		// Sent event that scene has been modified
		boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
	}

	void EditorManager::snapshotCameraPosition(void)
	{
		this->cameraCommandModule.pushCommand(std::make_shared<CameraPositionUndoCommand>(this->camera));
	}

	void EditorManager::snapshotCameraOrientation(void)
	{
		this->cameraCommandModule.pushCommand(std::make_shared<CameraOrientationUndoCommand>(this->camera));
	}

	void EditorManager::snapshotCameraTransform(void)
	{
		this->cameraCommandModule.pushCommand(std::make_shared<CameraTransformUndoCommand>(this->camera));
	}

	void EditorManager::snapshotTerraHeightMap(const std::vector<Ogre::uint16>& oldHeightData, const std::vector<Ogre::uint16>& newHeightData, TerraComponent* terraComponent)
	{
		this->sceneManipulationCommandModule.pushCommand(std::make_shared<TerrainModifyUndoCommand>(oldHeightData, newHeightData, terraComponent));
	}

	void EditorManager::snapshotTerraBlendMap(const std::vector<Ogre::uint8>& oldBlendWeightData, const std::vector<Ogre::uint8>& newBlendWeightData, TerraComponent* terraComponent)
	{
		this->sceneManipulationCommandModule.pushCommand(std::make_shared<TerrainPaintUndoCommand>(oldBlendWeightData, newBlendWeightData, terraComponent));
	}

	void EditorManager::deleteGameObjects(const std::vector<unsigned long> gameObjectIds)
	{
		if (gameObjectIds.size() > 0)
		{
			this->sceneManipulationCommandModule.pushCommand(std::make_shared<DeleteGameObjectsUndoCommand>("", gameObjectIds));
			// Sent event that scene has been modified
			boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
		}
	}

	void EditorManager::addComponent(const std::vector<unsigned long> gameObjectIds, const Ogre::String& componentClassName)
	{
		if (gameObjectIds.size() > 0)
		{
			this->sceneManipulationCommandModule.pushCommand(std::make_shared<AddComponentUndoCommand>(gameObjectIds, componentClassName));
			// Sent event that scene has been modified
			boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
		}
	}

	void EditorManager::deleteComponent(const std::vector<unsigned long> gameObjectIds, unsigned int index)
	{
		if (gameObjectIds.size() > 0)
		{
			this->sceneManipulationCommandModule.pushCommand(std::make_shared<DeleteComponentUndoCommand>(gameObjectIds, index));
			// Sent event that scene has been modified
			boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
		}
	}

	void EditorManager::cloneGameObjects(const std::vector<unsigned long> gameObjectIds)
	{
		if (gameObjectIds.size() > 0)
		{
			// Change to translate mode after cloned (true)
			this->sceneManipulationCommandModule.pushCommand(std::make_shared<CloneGameObjectsUndoCommand>(this, gameObjectIds));
			// Sent event that scene has been modified
			boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
		}
	}

	void EditorManager::undo(void)
	{
		this->sceneManipulationCommandModule.undo();
		this->setGizmoToGameObjectsCenter();
	}

	void EditorManager::redo(void)
	{
		this->sceneManipulationCommandModule.redo();
		this->setGizmoToGameObjectsCenter();
	}

	bool EditorManager::canUndo(void)
	{
		return this->sceneManipulationCommandModule.canUndo();
	}

	bool EditorManager::canRedo(void)
	{
		return this->sceneManipulationCommandModule.canRedo();
	}

	void EditorManager::cameraUndo(void)
	{
		this->cameraCommandModule.undo();
	}

	void EditorManager::cameraRedo(void)
	{
		this->cameraCommandModule.redo();
	}

	bool EditorManager::canCameraUndo(void)
	{
		return this->cameraCommandModule.canUndo();
	}

	bool EditorManager::canCameraRedo(void)
	{
		return this->cameraCommandModule.canRedo();
	}

	void EditorManager::setUseUndoRedoForPicker(bool useUndoRedoForPicker)
	{
		this->useUndoRedoForPicker = useUndoRedoForPicker;
	}

	bool EditorManager::getUseUndoRedoForPicker(void) const
	{
		return this->useUndoRedoForPicker;
	}

	Ogre::Real EditorManager::getPickForce(void) const
	{
		return this->movePicker2->getPickForce();
	}

	void EditorManager::optimizeScene(bool optimize)
	{
		const auto& gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

		if (true == optimize)
		{
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); it++)
			{
				auto& gameObject = it->second;
				{
					auto& component = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::PhysicsArtifactComponent>());
					if (nullptr != component)
					{
						gameObject->setDynamic(false);
						continue;
					}
				}

				{
					auto& component = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::JointComponent>());
					if (nullptr != component)
					{
						if ("JointComponent" == component->getClassName())
						{
							gameObject->setDynamic(false);
							continue;
						}
					}
				}
			}
		}
		else
		{
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); it++)
			{
				it->second->setDynamic(true);
			}
		}

		boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
	}

	void EditorManager::activateTestSelectedGameObjects(bool bActivate)
	{
		if (true == bActivate)
		{
			auto& selectedGameObjects = this->selectionManager->getSelectedGameObjects();
			if (selectedGameObjects.size() > 0)
			{
				const auto& allGameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

				for (auto& it2 = allGameObjects->cbegin(); it2 != allGameObjects->cend(); ++it2)
				{
					bool foundSelectedGameObject = false;
					for (auto& it = selectedGameObjects.cbegin(); it != selectedGameObjects.cend(); ++it)
					{
						if (it->second.gameObject->getId() == it2->second->getId())
						{
							foundSelectedGameObject = true;

							break;
						}
					}

					if (false == foundSelectedGameObject)
					{
						const auto& gameObjectComponents = it2->second->getComponents();
						std::vector<bool> activatedParts(gameObjectComponents->size());

						for (size_t i = 0; i < gameObjectComponents->size(); i++)
						{
							auto component = std::get<COMPONENT>(gameObjectComponents->at(i)).get();
							// Store if it was activated before
							activatedParts[i] = component->isActivated();
							// Deactivate the component
							Variant* activated = component->getAttribute("Activated");
							if (nullptr != activated)
							{
								activated->setValue(false);
							}
						}

						this->oldActivatedMap.emplace(it2->second->getId(), activatedParts);

					}
				}

			}
		}
		else
		{
			for (auto& it = this->oldActivatedMap.begin(); it != this->oldActivatedMap.end(); ++it)
			{
				auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(it->first);
				if (nullptr != gameObjectPtr)
				{
					const auto& gameObjectComponents = gameObjectPtr->getComponents();

					for (size_t i = 0; i < gameObjectComponents->size(); i++)
					{
						auto component = std::get<COMPONENT>(gameObjectComponents->at(i)).get();
						// Set old activated state
						Variant* activated = component->getAttribute("Activated");
						if (nullptr != activated)
						{
							activated->setValue(it->second[i]);
						}
					}
				}
			}

			this->oldActivatedMap.clear();
		}
	}

}; // namespace end