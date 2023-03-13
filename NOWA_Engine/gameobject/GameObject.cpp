#include "NOWAPrecompiled.h"
#include "GameObjectComponent.h"
#include "GameObject.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "GameObjectController.h"
#include "LuaScriptComponent.h"
#include "MyGUIItemBoxComponent.h"
#include "AiLuaComponent.h"
#include "BackgroundScrollComponent.h"
#include "modules/WorkspaceModule.h"
#include "modules/DeployResourceModule.h"
#include "WorkspaceComponents.h"
#include "DatablockPbsComponent.h"
#include "main/AppStateManager.h"
#include "main/Events.h"
#include "main/Core.h"

#include "OgreMeshManager2.h"
#include "OgreConfigFile.h"
#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMeshLodGenerator.h"
#include "OgreMesh2Serializer.h"
#include "OgrePixelCountLodStrategy.h"

#include <array>

namespace
{
	const std::array<Ogre::HlmsTypes, 7> searchHlms =
	{
		Ogre::HLMS_PBS, Ogre::HLMS_TOON, Ogre::HLMS_UNLIT, Ogre::HLMS_USER0,
		Ogre::HLMS_USER1, Ogre::HLMS_USER2, Ogre::HLMS_USER3
	};
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	GameObject::GameObject(Ogre::SceneManager* sceneManager, Ogre::SceneNode* sceneNode, Ogre::MovableObject* movableObject, const Ogre::String& category,
		bool dynamic, GameObject::eType type, unsigned long id)
		: sceneManager(sceneManager),
		sceneNode(sceneNode),
		movableObject(movableObject),
		type(type),
		clampObjectQuery(nullptr),
		centerOffset(Ogre::Vector3::ZERO),
		boundingBoxDraw(nullptr),
		priorId(0),
		oldScale(Ogre::Vector3::UNIT_SCALE),
		selected(false),
		doNotDestroyMovableObject(false),
		bShowDebugData(false),
		luaScript(nullptr)
	{
		// int idd = this->sceneNode->getId();
		
		this->name = new Variant(GameObject::AttrName(), "Default", this->attributes);
		if (0 == id)
		{
			this->id = new Variant(GameObject::AttrId(), NOWA::makeUniqueID(), this->attributes, true);
		}
		else
		{
			this->id = new Variant(GameObject::AttrId(), id, this->attributes, true);
		}
		this->categoryId = new Variant(GameObject::AttrCategoryId(), static_cast<unsigned int>(0), this->attributes);
		this->meshName = new Variant(GameObject::AttrMeshName(), Ogre::String(), this->attributes);
		
		if (GameObject::ITEM == this->type || GameObject::PLANE == this->type)
		{
			Ogre::Item* item = this->getMovableObjectUnsafe<Ogre::Item>();
			if (nullptr != item)
			{
				this->meshName->setValue(item->getMesh()->getName());

				// Later check, if the entity has maybe a different type of data block as PBS, such as Toon, Unlit etc.
				for (size_t i = 0; i < item->getNumSubItems(); i++)
				{
					auto datablock = item->getSubItem(i)->getDatablock();
					Ogre::String datablockName = "Missing";
					if (nullptr != datablock)
					{
						datablockName = *datablock->getNameStr();
					}
					else
					{
						item->getSubItem(i)->setDatablock(datablockName);
					}
					this->dataBlocks.emplace_back(new Variant(GameObject::AttrDataBlock() + Ogre::StringConverter::toString(i), datablockName, this->attributes));

					const Ogre::String* fileName;
					const Ogre::String* resourceGroup;
					datablock->getFilenameAndResourceGroup(&fileName, &resourceGroup);

					if (false == (*fileName).empty())
					{
						Ogre::String data = "File: '" + *fileName + "'\n Resource group name: '" + *resourceGroup + "'";
						this->dataBlocks[i]->setDescription(data);
					}
				}
			}
			else
			{
				this->meshName->setVisible(false);
			}
		}
		else
		{
			Ogre::v1::Entity* entity = this->getMovableObjectUnsafe<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				this->meshName->setValue(entity->getMesh()->getName());

				// Later check, if the entity has maybe a different type of data block as PBS, such as Toon, Unlit etc.
				for (size_t i = 0; i < entity->getNumSubEntities(); i++)
				{
					auto datablock = entity->getSubEntity(i)->getDatablock();
					Ogre::String datablockName = "Missing";
					if (nullptr != datablock)
					{
						datablockName = *datablock->getNameStr();
					}
					else
					{
						entity->getSubEntity(i)->setDatablock(datablockName);
					}
					this->dataBlocks.emplace_back(new Variant(GameObject::AttrDataBlock() + Ogre::StringConverter::toString(i), datablockName, this->attributes));

					const Ogre::String* fileName;
					const Ogre::String* resourceGroup;
					datablock->getFilenameAndResourceGroup(&fileName, &resourceGroup);

					if (false == (*fileName).empty())
					{
						Ogre::String data = "File: '" + *fileName + "'\n Resource group name: '" + *resourceGroup + "'";
						this->dataBlocks[i]->setDescription(data);
					}
				}
			}
			else
			{
				this->meshName->setVisible(false);
			}
		}

		this->meshName->setReadOnly(true);

		// Special treatement for category
		{
			// List all available categories
			std::vector<Ogre::String> allCategories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllCategoriesSoFar();
			this->category = new Variant(GameObject::AttrCategory(), allCategories, this->attributes);
			this->category->setListSelectedValue(category);
			this->category->addUserData(GameObject::AttrActionNeedRefresh());
		}

		this->tagName = new Variant(GameObject::AttrTagName(), Ogre::String(), this->attributes);
		this->size = new Variant(GameObject::AttrSize(), Ogre::Vector3::ZERO, this->attributes);
		this->position = new Variant(GameObject::AttrPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->scale = new Variant(GameObject::AttrScale(), Ogre::Vector3::ZERO, this->attributes);

		this->scale->addUserData(GameObject::AttrActionNeedRefresh());
		this->orientation = new Variant(GameObject::AttrOrientation(), Ogre::Vector3::ZERO, this->attributes);
		this->defaultDirection = new Variant(GameObject::AttrDefaultDirection(), Ogre::Vector3::UNIT_Z, this->attributes);
		this->dynamic = new Variant(GameObject::AttrDynamic(), dynamic, this->attributes);
		this->controlledByClientID = new Variant(GameObject::AttrControlledByClientID(), static_cast<unsigned int>(0), this->attributes);

		// Set some constraints
		this->id->setReadOnly(true);
		bool castShadows = true;
		bool visible = true;

		// game object uses the unique name of the scene node
		if (nullptr != this->sceneNode)
		{
			this->sceneNode->setStatic(!this->dynamic->getBool());
		}
		if (nullptr != this->movableObject)
		{
			this->movableObject->setStatic(!this->dynamic->getBool());
			castShadows = this->movableObject->getCastShadows();
			visible = this->movableObject->getVisible();
		}

		this->castShadows = new Variant(GameObject::AttrCastShadows(), castShadows, this->attributes);
		this->useReflection = new Variant(GameObject::AttrUseReflection(), false, this->attributes);
		this->useReflection->setDescription("Whether this object should reflect the scene. This only can be used if 'setUseReflection' is activated.");

		this->visible = new Variant(GameObject::AttrVisible(), visible, this->attributes);
		this->global = new Variant(GameObject::AttrGlobal(), false, this->attributes);
		this->clampY = new Variant(GameObject::AttrClampY(), false, this->attributes);

		// Note: referenceId will not be cloned, because its to special. The designer must adapt each time the ids!
		this->referenceId = new Variant(GameObject::AttrReferenceId(), static_cast<unsigned long>(0), this->attributes, false);

		if (GameObject::ITEM == type || GameObject::PLANE == type || GameObject::MIRROR == type || GameObject::LIGHT_AREA == type)
		{
			this->renderQueueIndex = new Variant(GameObject::AttrRenderQueueIndex(), static_cast<unsigned int>(NOWA::RENDER_QUEUE_V2_MESH), this->attributes, false);
		}
		else
		{
			this->renderQueueIndex = new Variant(GameObject::AttrRenderQueueIndex(), static_cast<unsigned int>(NOWA::RENDER_QUEUE_V1_MESH), this->attributes, false);
		}
		this->renderDistance = new Variant(GameObject::AttrRenderDistance(), static_cast<unsigned int>(1000), this->attributes, false);
		this->lodDistance = new Variant(GameObject::AttrLodDistance(), 300.0f, this->attributes, false);
		this->shadowRenderingDistance = new Variant(GameObject::AttrShadowDistance(), static_cast<unsigned int>(300), this->attributes, false);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObject] Creating Gameobject " + this->id->getString() + " for scene node name: " + this->sceneNode->getName());

		this->meshName->setDescription("Mesh location: '" + Core::getSingletonPtr()->getResourceFilePathName(this->meshName->getString()) + "'");
		this->category->setDescription("Only 30 distinct types are possible. Use also tag names in combination with categories. E.g. category: 'Enemy', tag names: 'Stone', 'EnemyMonster', 'Boss1', 'Boss2' etc.");
		this->tagName->setDescription("Tags are like sub-categories. E.g. several game objects may belong to the category 'Enemy', but one group may have a tag name 'Stone', the other 'Ship1', 'Ship2' etc. "
			"This is useful when doing ray - casts on graphics base or physics base or creating physics materials between categories, to further distinquish, which tag has been hit in order to remove different energy amount.");
		this->dynamic->setDescription("Set to true, if you are sure, the game object will be moved always, else set to false due to performance reasons.");
		this->global->setDescription("This is useful if a project does consist of several scenes, which do share this game object, so when a change is made, it will be available for all scenes.");
		this->clampY->setDescription("This is useful when game object is loaded, so that it will be automatically placed upon the next lower game object."
							"Especially when the game object is a global one and will be loaded for different worlds, that start at a different height."
							"If there is no game object below, the next game object above is searched.If this also does not exist, the current y coordinate is just used.");
		this->renderQueueIndex->setDescription("Sets the render queue group index, which controls the rendering order of the game object. Possible values are from 0 to 255. Constraints: "
			"RenderQueue ID range [0; 100) & [200; 225) default to FAST (i.e. for v2 objects, like Items); RenderQueue ID range[100; 200)& [225; 256) default to V1_FAST(i.e. for v1 objects, like v1::Entity)");
		this->renderDistance->setDescription("Sets the render distance in meters til which the game object will be rendered. Default value is 1000.");
		this->shadowRenderingDistance->setDescription("Sets the shadow rendering distance in meters til which the game object's shadow will be rendered. Default set to 300 meters, which means the game object's shadow will be rendered up to 300 meters.");
		this->lodDistance->setDescription("Sets the lod distance in meters til which the game object vertex count will be reduced. Default set to 300, which means that for this game object no lod levels will be generated and the mesh never reduced. "
			"There are always 4 levels of reduction, beginning at the given lod distance and increasing based on the bounding radius of the mesh of the game object. The mesh with the lod levels is also stored on the file disc.");
		this->defaultDirection->setDescription("The default local direction of the loaded mesh. Note: Its important: If the created mesh e.g. points to x-axis, its default direction should be set to (1 0 0)."
			" If it points to z-direction, its default direction should be set to (0 0 1) etc..");
	}

	GameObject::~GameObject()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObject] Destroying Gameobject "
			+ this->id->getString());

		this->delayedAddCommponentList.clear();

		this->luaScript = nullptr;
			
		// Delete all attributes
		auto& it = this->attributes.begin();

		while (it != this->attributes.end())
		{
			Variant* variant = it->second;
			delete variant;
			variant = nullptr;
			++it;
		}
		this->attributes.clear();

		if (nullptr != this->boundingBoxDraw)
		{
			this->sceneManager->destroyWireAabb(this->boundingBoxDraw);
			this->boundingBoxDraw = nullptr;
		}

		if (nullptr != this->clampObjectQuery)
		{
			this->sceneManager->destroyQuery(this->clampObjectQuery);
			this->clampObjectQuery = nullptr;
		}

		if (nullptr != this->sceneNode)
		{
			auto nodeIt = this->sceneNode->getChildIterator();
			while (nodeIt.hasMoreElements())
			{
				//go through all scenenodes in the scene
				Ogre::Node* subNode = nodeIt.getNext();
				subNode->removeAllChildren();
			}

			this->sceneNode->detachAllObjects();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObject] Destroying scene node: "
				+ this->sceneNode->getName());
			this->sceneManager->destroySceneNode(this->sceneNode);
			this->sceneNode = nullptr;
		}

		if (nullptr != this->movableObject && false == this->doNotDestroyMovableObject)
		{
			// Attention: does this work?
			if (this->sceneManager->hasMovableObject(this->movableObject))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObject] Destroying movable object: "
					+ this->movableObject->getName());

				this->sceneManager->destroyMovableObject(this->movableObject);
				this->movableObject = nullptr;
			}
		}

		// if this assert fires, the map was not empty and GameObject::destroy() has not been called
		assert(this->gameObjectComponents.empty());
	}

	bool GameObject::init(Ogre::MovableObject* newMovableObject)
	{
		if (nullptr != newMovableObject)
		{
			// If there is a new movable object and the game object has one, then first destroy this one
			if (nullptr != this->movableObject && false == this->doNotDestroyMovableObject)
			{
				if (this->sceneManager->hasMovableObject(this->movableObject))
				{
					this->sceneNode->detachObject(this->movableObject);
					this->sceneManager->destroyMovableObject(this->movableObject);
					this->movableObject = nullptr;
				}
			}

			this->movableObject = newMovableObject;
		}
		// Terrain e.g. has no entity
		if (nullptr != this->movableObject)
		{
			this->movableObject->getUserObjectBindings().setUserAny(Ogre::Any(this));
			this->movableObject->setQueryFlags(this->categoryId->getUInt());

			// calculate size and offset from center
			// Ogre::AxisAlignedBox boundingBox = newEntity->getMesh()->getBounds();
			this->refreshSize();

			if (GameObject::ITEM == this->type || GameObject::PLANE == this->type)
			{
				Ogre::Item* item = this->getMovableObjectUnsafe<Ogre::Item>();
				for (size_t i = 0; i < item->getNumSubItems(); i++)
				{
					auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
					if (nullptr != sourceDataBlock)
					{
						// sourceDataBlock->mShadowConstantBias = 0.0001f;
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
				Ogre::v1::Entity* entity = this->getMovableObject<Ogre::v1::Entity>();
				if (nullptr != entity)
				{
					for (size_t i = 0; i < entity->getNumSubEntities(); i++)
					{
						auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(entity->getSubEntity(i)->getDatablock());
						if (nullptr != sourceDataBlock)
						{
							// sourceDataBlock->mShadowConstantBias = 0.0001f;
							// Deactivate fresnel by default, because it looks ugly
							if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
							{
								sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
							}
						}
					}
				}
			}
			
			this->setRenderQueueIndex(this->movableObject->getRenderQueueGroup());
			// Note: Ogre runs in kilometers where as NOWA in meters as newton does
			unsigned int renderDistance = static_cast<unsigned int>(this->movableObject->getRenderingDistance());
			// Movable Object has infinite render distance, get global one. Is by default infinite too!
			if (0 == renderDistance)
			{
				renderDistance = Core::getSingletonPtr()->getGlobalRenderDistance();
				if (0 == renderDistance)
				{
					renderDistance = 1000;
				}
			}
			this->renderDistance->setValue(renderDistance);
			this->movableObject->setRenderingDistance(static_cast<Ogre::Real>(renderDistance));
		}
		this->sceneNode->getUserObjectBindings().setUserAny(Ogre::Any(this));

		if (nullptr == this->boundingBoxDraw)
		{
			this->boundingBoxDraw = sceneManager->createWireAabb();
			this->boundingBoxDraw->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		}

		return true;
	}

	void GameObject::refreshSize(void)
	{
		if (nullptr != this->movableObject)
		{
			Ogre::Aabb boundingBox = this->movableObject->getLocalAabb();
			Ogre::Vector3 padding = (boundingBox.getMaximum() - boundingBox.getMinimum()) * Ogre::v1::MeshManager::getSingleton().getBoundsPaddingFactor();
			Ogre::Vector3 scale = this->sceneNode->getScale();
			this->size->setReadOnly(false);
			this->size->setValue(((boundingBox.getMaximum() - boundingBox.getMinimum()) - padding * 2.0f) * scale);
			this->size->setReadOnly(true);
			this->centerOffset = (boundingBox.getMinimum() + padding + (size->getVector3() / 2.0f)) * scale;
		}
	}

	void GameObject::setDataBlocks(const std::vector<Ogre::String>& loadedDatablocks)
	{
		// Backward compatibility, only refresh if there are more data blocks loaded
		if (loadedDatablocks.size() >= this->dataBlocks.size())
		{
			this->dataBlocks.resize(loadedDatablocks.size());
			for (size_t i = 0; i < loadedDatablocks.size(); i++)
			{
				if ("" != loadedDatablocks[i])
				{
					if (nullptr != this->dataBlocks[i])
					{
						this->dataBlocks[i]->setValue(loadedDatablocks[i]);
					}
				}
			}
		}
	}

	bool GameObject::postInit(void)
	{
		bool isInGame = Core::getSingletonPtr()->getIsGame();
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectComponents.size());)
		{
			if (false == std::get<COMPONENT>(this->gameObjectComponents[i])->postInit())
			{
				this->deleteComponentByIndex(i);
				// return true;
			}
			else
			{
				if (false == isInGame)
				{
					// Resets all attribute setter back
					std::get<COMPONENT>(this->gameObjectComponents[i])->resetChanges();
				}
				i++;
			}
		}
		return true;
	}

	void GameObject::resetVariants(void)
	{
		Ogre::String oldCustomString = this->getCustomDataString();
		this->setCustomDataString(GameObject::AttrCustomDataSkipCreation());

		// Resets all variants so that all setter of all this game object and all its components are called
		for (size_t i = 0; i < this->attributes.size(); i++)
		{
			if (true == this->attributes[i].second->hasChanged())
			{
				this->actualizeValue(this->attributes[i].second);
			}
			this->attributes[i].second->resetChange();
		}

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectComponents.size()); i++)
		{
			auto gameObjectCompPtr = std::get<COMPONENT>(this->gameObjectComponents[i]);

			auto luaScriptComponent = boost::dynamic_pointer_cast<LuaScriptComponent>(gameObjectCompPtr);
			if (nullptr == luaScriptComponent)
			{
				std::get<COMPONENT>(this->gameObjectComponents[i])->resetVariants();
			}
		}
		this->setCustomDataString(oldCustomString);
	}

	void GameObject::resetChanges()
	{
		for (size_t i = 0; i < this->attributes.size(); i++)
		{
			this->attributes[i].second->resetChange();
		}
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectComponents.size()); i++)
		{
			std::get<COMPONENT>(this->gameObjectComponents[i])->resetChanges();
		}
	}

	bool GameObject::connect(void)
	{
		// Get a possible lua script from corresponding component
		boost::shared_ptr<LuaScriptComponent> luaScriptCompPtr;
		bool luaScriptNoCompileErrors = true;
		// Problem: When lua script is connected first and intialisations done inside, e.g. ragdoll etc. no ragdoll is yet available, so only compile
		
		luaScriptCompPtr = NOWA::makeStrongPtr(this->getComponent<LuaScriptComponent>());
		if (nullptr != luaScriptCompPtr)
		{
			// If its a lua script component, compile it before all other components, because the lua script must be compiled and is maybe invalid, so that other components can react on that
			luaScriptNoCompileErrors = luaScriptCompPtr->compileScript();
		}

		boost::shared_ptr<AiLuaComponent> aiLuaCompPtr = NOWA::makeStrongPtr(this->getComponent<AiLuaComponent>());
		
		if (true == luaScriptNoCompileErrors)
		{
			for (const auto& component : this->gameObjectComponents)
			{
				const GameObjectCompPtr gameObjectCompPtr = std::get<COMPONENT>(component);
				if (luaScriptCompPtr != gameObjectCompPtr && aiLuaCompPtr != gameObjectCompPtr)
				{
					gameObjectCompPtr->bConnectedSuccess = gameObjectCompPtr->connect();
				}
			}
		}

		// Connect after all other components, so that all other game objects and components are available for the lua script
		if (true == luaScriptNoCompileErrors)
		{
			if (nullptr != luaScriptCompPtr)
			{
				luaScriptCompPtr->connect();
			}
			// If there is an ai lua component, it must be connected after the lua script! Because it may be, that it is using variables from lua script (connect) function in its state
			if (nullptr != aiLuaCompPtr)
			{
				aiLuaCompPtr->connect();
			}
		}
		return true;
	}

	bool GameObject::disconnect(void)
	{
		for (const auto& component : this->gameObjectComponents)
		{
			if (false == std::get<COMPONENT>(component)->disconnect())
			{
				return false;
			}
		}
		return true;
	}

	bool GameObject::onCloned(void)
	{
		for (const auto& component : this->gameObjectComponents)
		{
			if (false == std::get<COMPONENT>(component)->onCloned())
			{
				return false;
			}
		}
		return true;
	}

	void GameObject::destroy(void)
	{
		for (auto& it = this->gameObjectComponents.cbegin(); it != this->gameObjectComponents.cend(); ++it)
		{
			std::get<COMPONENT>(*it)->onRemoveComponent();
		}
		this->gameObjectComponents.clear();
	}

	void GameObject::pause(void)
	{
		for (const auto& component : this->gameObjectComponents)
		{
			std::get<COMPONENT>(component)->pause();
		}
	}

	void GameObject::resume(void)
	{
		for (const auto& component : this->gameObjectComponents)
		{
			std::get<COMPONENT>(component)->resume();
		}
	}

	void GameObject::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == this->delayedAddCommponentList.empty())
		{
			for (auto& it = this->delayedAddCommponentList.cbegin(); it != this->delayedAddCommponentList.cend();)
			{
				GameObjectCompPtr gameObjectCompPtr = (*it).first;
				bool bConnect = (*it).second;

				if (gameObjectCompPtr)
				{
					this->addComponent(gameObjectCompPtr);
					if (true == bConnect)
					{
						gameObjectCompPtr->bConnectedSuccess = gameObjectCompPtr->connect();
					}
					it = this->delayedAddCommponentList.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		for (auto& it = this->gameObjectComponents.cbegin(); it != this->gameObjectComponents.cend(); ++it)
		{
			if (true == std::get<COMPONENT>(*it)->bConnectedSuccess)
				std::get<COMPONENT>(*it)->update(dt, notSimulating);
		}
		this->position->setValue(this->sceneNode->getPosition());
		this->scale->setValue(this->sceneNode->getScale());
		if (this->sceneNode->getScale() != this->oldScale)
			this->refreshSize();

		this->oldScale = this->sceneNode->getScale();
		// Set in the form degreeX, degreeY, degreeZ
		this->orientation->setValue(MathHelper::getInstance()->quatToDegreesRounded(this->sceneNode->getOrientation()));
	}

	void GameObject::lateUpdate(Ogre::Real dt, bool notSimulating)
	{
		for (auto& it = this->gameObjectComponents.cbegin(); it != this->gameObjectComponents.cend(); ++it)
		{
			if (std::get<COMPONENT>(*it)->getClassId() == LuaScriptComponent::getStaticClassId())
				boost::static_pointer_cast<LuaScriptComponent>(std::get<COMPONENT>(*it))->lateUpdate(dt, notSimulating);
			else if (std::get<COMPONENT>(*it)->getClassName() == BackgroundScrollComponent::getStaticClassName())
				boost::static_pointer_cast<BackgroundScrollComponent>(std::get<COMPONENT>(*it))->lateUpdate(dt, notSimulating);
		}
	}
	void GameObject::actualizeValue(Variant* attribute)
	{
		if (GameObject::AttrName() == attribute->getName())
		{
			this->setName(attribute->getString());
		}
		else if (GameObject::AttrCategory() == attribute->getName())
		{
			// Call with the default value and the new one from selected list item
			this->category->setListSelectedValue(attribute->getListSelectedValue());
			this->changeCategory(attribute->getListSelectedOldValue(), attribute->getListSelectedValue());
			this->categoryId->setValue(AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId(attribute->getListSelectedValue()));
		
			this->movableObject->setQueryFlags(this->categoryId->getUInt());
		}
		if (GameObject::AttrTagName() == attribute->getName())
		{
			this->tagName->setValue(attribute->getString());
		}
		else if (GameObject::AttrPosition() == attribute->getName())
		{
			this->position->setValue(attribute->getVector3());
			this->sceneNode->setPosition(attribute->getVector3());
		}
		else if (GameObject::AttrScale() == attribute->getName())
		{
			this->scale->setValue(attribute->getVector3());
			this->sceneNode->setScale(attribute->getVector3());
			// Ogre::AxisAlignedBox boundingBox = this->entity->getMesh()->getBounds();
			this->refreshSize();
			this->oldScale = attribute->getVector3();
		}
		else if (GameObject::AttrOrientation() == attribute->getName())
		{
			Ogre::Vector3 degreeOrientation = attribute->getVector3();
			this->orientation->setValue(attribute->getVector3());

			this->sceneNode->setOrientation(MathHelper::getInstance()->degreesToQuat(attribute->getVector3()));
		}
		else if (GameObject::AttrDynamic() == attribute->getName())
		{
			this->setDynamic(attribute->getBool());
		}
		else if (GameObject::AttrControlledByClientID() == attribute->getName())
		{
			this->controlledByClientID->setValue(attribute->getULong());
			// later
		}
		
		else if (GameObject::AttrCastShadows() == attribute->getName())
		{
			this->castShadows->setValue(attribute->getBool());
			this->movableObject->setCastShadows(this->castShadows->getBool());
		}
		else if (GameObject::AttrUseReflection() == attribute->getName())
		{
			this->setUseReflection(attribute->getBool());
		}
		else if (GameObject::AttrDefaultDirection() == attribute->getName())
		{
			this->setDefaultDirection(attribute->getVector3());
		}
		else if (GameObject::AttrVisible() == attribute->getName())
		{
			this->setVisible(attribute->getBool());
		}
		else if (GameObject::AttrGlobal() == attribute->getName())
		{
			this->global->setValue(attribute->getBool());
		}
		else if (GameObject::AttrClampY() == attribute->getName())
		{
			this->clampY->setValue(attribute->getBool());
			if (true == attribute->getBool())
			{
				this->performRaycastForYClamping();

				// Since a position is adapted, components maybe will also react (like physics component) to set the physical position
				for (const auto& component : this->gameObjectComponents)
				{
					std::get<COMPONENT>(component)->actualizeValue(attribute);
				}
			}
		}
		else if (GameObject::AttrReferenceId() == attribute->getName())
		{
			this->referenceId->setValue(attribute->getULong());
		}
		else if (GameObject::AttrRenderQueueIndex() == attribute->getName())
		{
			this->setRenderQueueIndex(attribute->getUInt());
		}
		else if (GameObject::AttrRenderDistance() == attribute->getName())
		{
			this->setRenderDistance(attribute->getUInt());
		}
		else if (GameObject::AttrLodDistance() == attribute->getName())
		{
			this->setLodDistance(attribute->getReal());
		}
		else if (GameObject::AttrShadowDistance() == attribute->getName())
		{
			this->setShadowRenderingDistance(attribute->getUInt());
		}
		else
		{
			// Note: If snapshot of game object is done, data block must not be set when resetting variant values, else each other game objects will share this datablock!
			if (GameObject::AttrCustomDataSkipCreation() != this->getCustomDataString())
			{
				this->setDatablock(attribute);
			}
		}
	}

	void GameObject::setDatablock(NOWA::Variant* attribute)
	{
		if (GameObject::ITEM == this->type)
		{
			Ogre::Item* item = this->getMovableObjectUnsafe<Ogre::Item>();
			if (nullptr != item)
			{
				for (size_t i = 0; i < this->dataBlocks.size(); i++)
				{
					if (GameObject::AttrDataBlock() + Ogre::StringConverter::toString(i) == attribute->getName())
					{
						Ogre::Hlms* hlms = nullptr;
						// Go through all types of registered hlms and check if the data block exists and set the data block
						for (auto searchHlmsIt = searchHlms.begin(); searchHlmsIt != searchHlms.end(); ++searchHlmsIt)
						{
							hlms = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(*searchHlmsIt);
							if (nullptr != hlms)
							{
								if (nullptr != hlms->getDatablock(attribute->getString()) /*&& "Missing" != *this->item->getSubItem(i)->getDatablock()->getNameStr()*/)
								{
									item->getSubItem(i)->setDatablock(attribute->getString());
									auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
									if (nullptr != sourceDataBlock)
									{
										// Deactivate fresnel by default, because it looks ugly
										if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
										{
											sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
										}
									}
									this->dataBlocks[i]->setValue(attribute->getString());
									// ((Ogre::HlmsPbsDatablock*)hlms->getDatablock(attribute->getString()))->setBackgroundDiffuse
									break;
								}
								else
								{
									Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObject] Could not set data block name: '" + attribute->getString() + "', because it does not exist");
									attribute->setValue(*item->getSubItem(i)->getDatablock()->getNameStr());
								}
							}
						}
					}
				}
			}
		}
		else
		{
			Ogre::v1::Entity* entity = this->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				for (size_t i = 0; i < this->dataBlocks.size(); i++)
				{
					if (GameObject::AttrDataBlock() + Ogre::StringConverter::toString(i) == attribute->getName())
					{
						Ogre::Hlms* hlms = nullptr;
						// Go through all types of registered hlms and check if the data block exists and set the data block
						for (auto searchHlmsIt = searchHlms.begin(); searchHlmsIt != searchHlms.end(); ++searchHlmsIt)
						{
							hlms = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(*searchHlmsIt);
							if (nullptr != hlms)
							{
								if (nullptr != hlms->getDatablock(attribute->getString()) /*&& "Missing" != *this->entity->getSubEntity(i)->getDatablock()->getNameStr()*/)
								{
									entity->getSubEntity(i)->setDatablock(attribute->getString());
									auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(entity->getSubEntity(i)->getDatablock());
									if (nullptr != sourceDataBlock)
									{
										// Deactivate fresnel by default, because it looks ugly
										if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
										{
											sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
										}
									}
									this->dataBlocks[i]->setValue(attribute->getString());
									// ((Ogre::HlmsPbsDatablock*)hlms->getDatablock(attribute->getString()))->setBackgroundDiffuse
									break;
								}
								else
								{
									Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObject] Could not set data block name: '" + attribute->getString() + "', because it does not exist");
									attribute->setValue(*entity->getSubEntity(i)->getDatablock()->getNameStr());
								}
							}
						}
					}
				}
			}
		}
	}

	void GameObject::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Object"));
		propertyXML->append_attribute(doc.allocate_attribute("data", "GameObject"));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Id"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->id->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Category"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->category->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TagName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->tagName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ControlledByClient"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->controlledByClientID->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Static"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(!this->dynamic->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseReflection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->useReflection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Visible"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->visible->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DefaultDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->defaultDirection->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Global"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->global->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ClampY"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->clampY->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReferenceId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->referenceId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RenderQueueIndex"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->renderQueueIndex->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RenderDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->renderDistance->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LodDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->lodDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowRenderingDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shadowRenderingDistance->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->dataBlocks.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString("DataBlock" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->dataBlocks[i]->getString())));
			propertiesXML->append_node(propertyXML);
		}
		
		for (const auto& component : this->gameObjectComponents)
		{
			std::get<COMPONENT>(component)->writeXML(propertiesXML, doc, filePath);
		}
	}

	void GameObject::showDebugData(void)
	{
		this->bShowDebugData = !this->bShowDebugData;
		for (const auto& component : this->gameObjectComponents)
		{
			std::get<COMPONENT>(component)->showDebugData();
		}
	}

	bool GameObject::getShowDebugData(void) const
	{
		return this->bShowDebugData;
	}

	unsigned long GameObject::getId(void) const
	{
		return this->id->getULong();
	}

	void GameObject::setName(const Ogre::String& name)
	{
		Ogre::String tempName;
		if (true == name.empty())
			tempName = "Default";
		else
			tempName = name;

		AppStateManager::getSingletonPtr()->getGameObjectController()->getValidatedGameObjectName(tempName, this->id->getULong());
		this->name->setValue(tempName);

		this->sceneNode->setName(tempName);
		if (nullptr != this->movableObject)
		{
			this->movableObject->setName(tempName);
		}
	}

	const Ogre::String GameObject::getName(void) const
	{
		return this->name->getString();
	}

	const Ogre::String GameObject::getUniqueName(void) const
	{
		return this->name->getString() + "_" + this->id->getString();
	}

	Ogre::SceneManager* GameObject::getSceneManager(void) const
	{
		return this->sceneManager;
	}

	Ogre::SceneNode* GameObject::getSceneNode(void) const
	{
		return this->sceneNode;
	}

	void GameObject::changeCategory(const Ogre::String& oldCategory, const Ogre::String& newCategory)
	{
		this->category->setListSelectedValue(newCategory);
		AppStateManager::getSingletonPtr()->getGameObjectController()->changeCategory(this, oldCategory, newCategory);
	}

	void GameObject::changeCategory(const Ogre::String& newCategory)
	{
		AppStateManager::getSingletonPtr()->getGameObjectController()->changeCategory(this, this->category->getListSelectedValue(), newCategory);
		this->category->setListSelectedValue(newCategory);
	}

	Ogre::String GameObject::getCategory(void) const
	{
		return this->category->getListSelectedValue();
	}

	unsigned int GameObject::getCategoryId(void) const
	{
		return this->categoryId->getUInt();
	}

	void GameObject::setTagName(const Ogre::String& tagName)
	{
		this->tagName->setValue(tagName);
	}

	Ogre::String GameObject::getTagName(void) const
	{
		return this->tagName->getString();
	}

	GameObjectComponents* GameObject::getComponents(void)
	{
		return &this->gameObjectComponents;
	}

	void GameObject::addComponent(GameObjectCompPtr gameObjectCompPtr)
	{
		// add besides the derived type id the parent id, so that its later possible to get the component for the parent, instead directly these derived component
		this->gameObjectComponents.emplace_back(gameObjectCompPtr->getClassId(), gameObjectCompPtr->getParentClassId(), gameObjectCompPtr->getParentParentClassId(), gameObjectCompPtr);
		gameObjectCompPtr->index = static_cast<unsigned short>(this->gameObjectComponents.size()) - 1;

		unsigned int i = 0;
		// If there is already such component increment the occurrence
		for (auto& component : this->gameObjectComponents)
		{
			if (std::get<COMPONENT>(component) != gameObjectCompPtr)
			{
				std::get<COMPONENT>(component)->onOtherComponentAdded(gameObjectCompPtr->index);
			}
			// Check for class name and parent class name for occurrence, e.g. JointPassiveSliderComponent -> JointComponent, JointKinematicComponent -> JointComponent
			Ogre::String thisClassName = std::get<COMPONENT>(component)->getClassName();
			Ogre::String newClassName = gameObjectCompPtr->getClassName();
			Ogre::String thisParentClassName = std::get<COMPONENT>(component)->getParentClassName();
			Ogre::String newClassParentName = gameObjectCompPtr->getParentClassName();
			if (thisClassName == newClassName || thisParentClassName == thisClassName)
			{
				std::get<COMPONENT>(component)->occurrenceIndex = i;
				i++;
			}
		}
	}

	void GameObject::addDelayedComponent(GameObjectCompPtr gameObjectCompPtr, bool bConnect)
	{
		this->delayedAddCommponentList.emplace_back(gameObjectCompPtr, bConnect);
	}

	bool GameObject::insertComponent(GameObjectCompPtr gameObjectCompPtr, unsigned int index)
	{
		if (index < 0)
		{
			return false;
		}

		// Set the index for the component
		gameObjectCompPtr->index = index;

		if (index >= this->gameObjectComponents.size())
		{
			this->gameObjectComponents.emplace_back(gameObjectCompPtr->getClassId(), gameObjectCompPtr->getParentClassId(), gameObjectCompPtr->getParentParentClassId(), gameObjectCompPtr);
			gameObjectCompPtr->index = static_cast<unsigned short>(this->gameObjectComponents.size()) - 1;
		}
		else
		{
			// inserts besides the derived type id the parent id, so that its later possible to get the component for the parent, instead directly these derived component
			this->gameObjectComponents.insert(this->gameObjectComponents.begin() + index,
				std::make_tuple(gameObjectCompPtr->getClassId(), gameObjectCompPtr->getParentClassId(), gameObjectCompPtr->getParentParentClassId(), gameObjectCompPtr));
			gameObjectCompPtr->index = index;
		}

		unsigned int i = 0;
		// If there is already such component increment the occurrence index
		for (auto& it = this->gameObjectComponents.begin(); it != this->gameObjectComponents.end(); ++it)
		{
			if (std::get<COMPONENT>(*it) != gameObjectCompPtr)
			{
				std::get<COMPONENT>(*it)->onOtherComponentAdded(gameObjectCompPtr->index);
			}

			// Check for class name and parent class name for occurrence, e.g. JointPassiveSliderComponent -> JointComponent, JointKinematicComponent -> JointComponent
			if (std::get<COMPONENT>(*it)->getClassName() == gameObjectCompPtr->getClassName()/* || std::get<COMPONENT>(*it)->getParentClassName() == gameObjectCompPtr->getParentClassName()*/)
			{
				std::get<COMPONENT>(*it)->occurrenceIndex = i;
				i++;
			}
		}
		return true;
	}

	bool GameObject::moveComponentUp(unsigned int index)
	{
		if (index >= this->gameObjectComponents.size())
		{
			return false;
		}
		else if (index < 1)
		{
			return false;
		}

		// Make a copy of the element
		auto currentComponentData = this->gameObjectComponents[index];
		// Remove the element
		this->gameObjectComponents.erase(this->gameObjectComponents.begin() + index);
		// Insert at an different place
		this->gameObjectComponents.insert(this->gameObjectComponents.begin() + index - 1,
			std::make_tuple(std::get<COMPONENT>(currentComponentData)->getClassId(), std::get<COMPONENT>(currentComponentData)->getParentClassId(), 
				std::get<COMPONENT>(currentComponentData)->getParentParentClassId(), std::get<COMPONENT>(currentComponentData)));

		// Decrement also the index
		std::get<COMPONENT>(currentComponentData)->index--;

		unsigned int i = 0;
		// If there is already such component increment the occurrence
		for (auto& it = this->gameObjectComponents.begin(); it != this->gameObjectComponents.end(); ++it)
		{
			// Check for class name and parent class name for occurrence, e.g. JointPassiveSliderComponent -> JointComponent, JointKinematicComponent -> JointComponent
			if (std::get<COMPONENT>(*it)->getClassName() == std::get<COMPONENT>(currentComponentData)->getClassName() 
				/*|| std::get<COMPONENT>(*it)->getParentClassName() == std::get<COMPONENT>(*currentIt)->getParentClassName()*/)
			{
				std::get<COMPONENT>(*it)->occurrenceIndex = i;
				i++;
			}
		}

		return true;
	}

	bool GameObject::moveComponentDown(unsigned int index)
	{
		if (index >= this->gameObjectComponents.size() - 1)
		{
			return false;
		}
		else if (index < 0)
		{
			return false;
		}
		
		// Make a copy of the element
		auto currentComponentData = this->gameObjectComponents[index];
		// Remove the element
		this->gameObjectComponents.erase(this->gameObjectComponents.begin() + index);
		// Insert at an different place
		this->gameObjectComponents.insert(this->gameObjectComponents.begin() + index + 1,
			std::make_tuple(std::get<COMPONENT>(currentComponentData)->getClassId(), std::get<COMPONENT>(currentComponentData)->getParentClassId(), 
				std::get<COMPONENT>(currentComponentData)->getParentParentClassId(), std::get<COMPONENT>(currentComponentData)));

		// Increment also the index
		std::get<COMPONENT>(currentComponentData)->index++;
		
		unsigned int i = 0;
		// If there is already such component increment the occurrence
		for (auto& it = this->gameObjectComponents.begin(); it != this->gameObjectComponents.end(); ++it)
		{
			// Check for class name and parent class name for occurrence, e.g. JointPassiveSliderComponent -> JointComponent, JointKinematicComponent -> JointComponent
			if (std::get<COMPONENT>(*it)->getClassName() == std::get<COMPONENT>(currentComponentData)->getClassName() /*||
				std::get<COMPONENT>(*it)->getParentClassName() == std::get<COMPONENT>(*currentIt)->getParentClassName()*/)
			{
				std::get<COMPONENT>(*it)->occurrenceIndex = i;
				i++;
			}
		}

		return true;
	}

	bool GameObject::moveComponent(unsigned int index)
	{
		if (index >= this->gameObjectComponents.size())
		{
			return false;
		}
		else if (index < 1)
		{
			return false;
		}

		// Make a copy of the element
		auto currentComponentData = this->gameObjectComponents[this->gameObjectComponents.size() - 1];
		// Remove the element
		this->gameObjectComponents.erase(this->gameObjectComponents.begin() + this->gameObjectComponents.size() - 1);
		// Insert at an different place
		this->gameObjectComponents.insert(this->gameObjectComponents.begin() + index,
										  std::make_tuple(std::get<COMPONENT>(currentComponentData)->getClassId(), std::get<COMPONENT>(currentComponentData)->getParentClassId(),
										  std::get<COMPONENT>(currentComponentData)->getParentParentClassId(), std::get<COMPONENT>(currentComponentData)));

		this->actualizeComponentsIndices();

		return true;
	}

	boost::weak_ptr<GameObjectComponent> GameObject::getComponentByIndex(unsigned int index)
	{
		if (index >= this->gameObjectComponents.size())
		{
			return boost::weak_ptr<GameObjectComponent>();
		}
		else if (index < 0)
		{
			return boost::weak_ptr<GameObjectComponent>();
		}
		return std::get<COMPONENT>(this->gameObjectComponents[index]);
	}

	int GameObject::getIndexFromComponent(GameObjectComponent* gameObjectComponent)
	{
		for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
		{
			if (std::get<COMPONENT>(this->gameObjectComponents[i]).get() == gameObjectComponent)
			{
				return static_cast<int>(i);
			}
		}
		return -1;
	}

	int GameObject::getOccurrenceIndexFromComponent(GameObjectComponent* gameObjectComponent)
	{
		for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
		{
			if (std::get<COMPONENT>(this->gameObjectComponents[i]).get() == gameObjectComponent)
			{
				return std::get<COMPONENT>(this->gameObjectComponents[i])->getOccurrenceIndex();
			}
		}
		return -1;
	}

	bool GameObject::deleteComponent(const Ogre::String& componentClassName, unsigned int componentOccurrenceIndex)
	{
		unsigned int componentClassId = NOWA::getIdFromName(componentClassName);
		
		bool success = this->internalDeleteComponent(componentClassName, componentClassId, componentOccurrenceIndex);

		this->actualizeComponentsIndices();

		return success;
	}

	void GameObject::actualizeComponentsIndices(void)
	{
		std::vector<unsigned int> seen(this->gameObjectComponents.size(), 0);
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->gameObjectComponents.size()); i++)
		{
			if (seen[i] == 0)
			{
				unsigned int count = 0;
				for (unsigned int j = i; j < static_cast<unsigned int>(this->gameObjectComponents.size()); j++)
				{
					Ogre::String thisClassName = std::get<COMPONENT>(this->gameObjectComponents[i])->getClassName();
					Ogre::String newClassName = std::get<COMPONENT>(this->gameObjectComponents[j])->getClassName();
					Ogre::String thisParentClassName = std::get<COMPONENT>(this->gameObjectComponents[i])->getParentClassName();
					if (thisClassName == newClassName || thisParentClassName == thisClassName)
					{
						std::get<COMPONENT>(this->gameObjectComponents[j])->occurrenceIndex = count++;
						seen[j] = 1;
					}
				}
			}

			std::get<COMPONENT>(this->gameObjectComponents[i])->index = i;
		}
	}

	bool GameObject::deleteComponent(GameObjectComponent* gameObjectComponent)
	{
		for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
		{
			if (std::get<COMPONENT>(this->gameObjectComponents[i]).get() == gameObjectComponent)
			{
				return this->deleteComponentByIndex(gameObjectComponent->index);
			}
		}
		return false;
	}

	bool GameObject::deleteComponentByIndex(unsigned int index)
	{
		if (index >= this->gameObjectComponents.size())
		{
			return false;
		}
		else if (index < 0)
		{
			return false;
		}

		auto& component = NOWA::makeStrongPtr(this->getComponentByIndex(index));
		if (nullptr != component)
		{
			// If a lua script component has been removed, reset the lua script pointer
			auto luaScriptComponent = boost::dynamic_pointer_cast<LuaScriptComponent>(component);
			if (nullptr != luaScriptComponent)
			{
				this->luaScript = nullptr;
			}
			
			size_t originSize = this->gameObjectComponents.size();
			for (auto& it = this->gameObjectComponents.begin(); it != this->gameObjectComponents.end(); ++it)
			{
				// Call for all other components the event
				// if (std::get<COMPONENT>(*it)->getClassName() != component->getClassName() || std::get<COMPONENT>(*it)->occurrenceIndex != component->occurrenceIndex)
				if (std::get<COMPONENT>(*it)->getClassName() != component->getClassName())
				{
					std::get<COMPONENT>(*it)->onOtherComponentRemoved(index);
					// If component has been removed in onOtherComponentRemoved function, break here, else crash occurs 
					if (this->gameObjectComponents.size() < originSize)
					{
						break;
					}
				}
			}
			
			// Call to give a chance to react before the component is removed
			component->onRemoveComponent();
			this->gameObjectComponents.erase(this->gameObjectComponents.begin() + index);

			this->actualizeComponentsIndices();
		}
		return false;
	}

	bool GameObject::internalDeleteComponent(const Ogre::String& componentClassName, unsigned int componentClassId, unsigned int componentOccurrenceIndex)
	{
		// If a lua script component has been removed, reset the lua script pointer
		if ("LuaScriptComponent" == componentClassName)
		{
			this->luaScript = nullptr;
		}
		
		bool found = false;
		unsigned int i = 0;
		for (auto it = this->gameObjectComponents.cbegin(); it != this->gameObjectComponents.cend();)
		{
			if (std::get<CLASS_ID>(*it) == componentClassId || std::get<PARENT_CLASS_ID>(*it) == componentClassId || std::get<PARENT_PARENT_CLASS_ID>(*it) == componentClassId)
			{
				if (std::get<COMPONENT>(*it)->getOccurrenceIndex() == componentOccurrenceIndex)
				{
					// Send event, that component has been deleted
					boost::shared_ptr<EventDataDeleteComponent> eventDataDeleteComponent(new EventDataDeleteComponent(this->id->getULong(), componentClassName, i));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataDeleteComponent);
					// Call to give a chance to react before the component is removed
					std::get<COMPONENT>(*it)->onRemoveComponent();
					// Frees the category, internally a check will be made if there is still a game object with the category, if not it will be un-occupied, for a next new category
					// Do not free category when comopnent is delete, this is none sense, because the game object may still exist, even without components!
					// AppStateManager::getSingletonPtr()->getGameObjectController()->freeCategoryFromGameObject(it->second.second->getOwner()->getCategory());
					it = this->gameObjectComponents.erase(it);
					found = true;
				}
				else
				{
					++it;
					i++;
				}
			}
			else
			{
				++it;
				i++;
			}

			if (true == found)
			{
				break;
			}
		}

		if (true == found)
		{
			// Build new indices for all remaining components
			i = 0;
			for (auto it = this->gameObjectComponents.cbegin(); it != this->gameObjectComponents.cend(); ++it)
			{
				std::get<COMPONENT>(*it)->index = i++;
			}
		}

		return found;
	}

	void GameObject::setAttributePosition(const Ogre::Vector3& position)
	{
		this->position->setValue(position);
		this->sceneNode->_setDerivedPosition(position);
	}

	void GameObject::setAttributeScale(const Ogre::Vector3& scale)
	{
		this->scale->setValue(scale);
		this->sceneNode->setScale(scale);
		this->refreshSize();
		this->oldScale = scale;
	}

	void GameObject::setAttributeOrientation(const Ogre::Quaternion& orientation)
	{
		// Set in the form degree, x-axis, y-axis, z-axis
		this->orientation->setValue(MathHelper::getInstance()->quatToDegreesRounded(orientation));
		this->sceneNode->_setDerivedOrientation(orientation);
	}

	void GameObject::setDefaultDirection(const Ogre::Vector3& defaultDirection)
	{
		this->defaultDirection->setValue(defaultDirection);
		boost::shared_ptr<NOWA::EventDataDefaultDirectionChanged> eventDataDefaultDirectionChanged(new NOWA::EventDataDefaultDirectionChanged(this->id->getULong(), defaultDirection));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataDefaultDirectionChanged);
	}

	Ogre::Vector3 GameObject::getDefaultDirection(void) const
	{
		return this->defaultDirection->getVector3();
	}
	
	Ogre::Vector3 GameObject::getPosition(void) const
	{
		return this->sceneNode->getPosition();
	}

	Ogre::Quaternion GameObject::getOrientation(void) const
	{
		return this->sceneNode->getOrientation();
	}

	Ogre::Vector3 GameObject::getScale(void) const
	{
		return this->sceneNode->getScale();
	}

	void GameObject::setControlledByClientID(unsigned int controlledByClientID)
	{
		this->controlledByClientID->setValue(controlledByClientID);
	}

	unsigned int GameObject::getControlledByClientID(void) const
	{
		return this->controlledByClientID->getUInt();
	}

	void GameObject::setDynamic(bool dynamic)
	{
		this->dynamic->setValue(dynamic);
		if (nullptr != this->sceneNode)
		{
			if (false == dynamic && false == this->sceneNode->isStatic())
			{
				// Does automatically set all its movable objects to this state
				this->sceneNode->setStatic(true);
			}
			else if (true == dynamic && true == this->sceneNode->isStatic())
			{
				this->sceneNode->setStatic(false);
			}
		}
	}

	const Ogre::Vector3 GameObject::getSize(void) const
	{
		return this->size->getVector3();
	}

	const Ogre::Vector3 GameObject::getCenterOffset(void) const
	{
		return this->centerOffset;
	}

	const Ogre::Vector3 GameObject::getBottomOffset(void) const
	{
		Ogre::Vector3 bottomCenterOfMesh = Ogre::Vector3::ZERO;
		if (GameObject::ITEM == this->type)
		{
			bottomCenterOfMesh = MathHelper::getInstance()->getBottomCenterOfMesh(this->sceneNode, this->getMovableObjectUnsafe<Ogre::Item>());
		}
		else
		{
			bottomCenterOfMesh = MathHelper::getInstance()->getBottomCenterOfMesh(this->sceneNode, this->getMovableObjectUnsafe<Ogre::v1::Entity>());
		}

		return bottomCenterOfMesh;
	}

	Ogre::Vector3 GameObject::getMiddle(void) const
	{
		Ogre::Vector3 centerBottom = Ogre::Vector3::ZERO;

		if (GameObject::ITEM == this->type)
		{
			centerBottom = MathHelper::getInstance()->getBottomCenterOfMesh(this->sceneNode, this->getMovableObjectUnsafe<Ogre::Item>());
		}
		else
		{
			centerBottom = MathHelper::getInstance()->getBottomCenterOfMesh(this->sceneNode, this->getMovableObjectUnsafe<Ogre::v1::Entity>());
		}

		return centerBottom + this->size->getVector3() * 0.5f /** Ogre::Vector3(1.0f, 0.0f, 1.0f)*/;
	}

	bool GameObject::isDynamic(void) const
	{
		return this->dynamic->getBool();
	}

	void GameObject::setVisible(bool visible)
	{
		bool isVisible = true;
		if (nullptr != this->movableObject)
		{
			isVisible = this->movableObject->getVisible();
		}
		if (isVisible != visible)
		{
			this->visible->setValue(visible);
			this->sceneNode->setVisible(visible);
			if (nullptr != this->movableObject)
			{
				this->movableObject->setVisible(this->visible->getBool());
			}
		}
	}

	bool GameObject::isVisible(void) const
	{
		return this->visible->getBool();
	}

	void GameObject::setActivated(bool activated)
	{
		for (const auto& component : this->gameObjectComponents)
		{
			std::get<COMPONENT>(component)->setActivated(activated);
		}
	}

	void GameObject::setUseReflection(bool useReflection)
	{
		this->useReflection->setValue(useReflection);

		if (GameObject::ITEM == this->type || GameObject::PLANE == this->type)
		{
			Ogre::Item* item = this->getMovableObjectUnsafe<Ogre::Item>();
			if (nullptr != item)
			{
				for (size_t i = 0; i < item->getNumSubItems(); i++)
				{
					Ogre::HlmsPbsDatablock* pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
					if (nullptr != pbsDatablock)
					{
						if (true == this->useReflection->getBool())
						{
							WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
							if (nullptr != workspaceBaseComponent)
							{
								pbsDatablock->setTexture(static_cast<Ogre::uint8>(Ogre::PBSM_REFLECTION), workspaceBaseComponent->getDynamicCubemapTexture());
								pbsDatablock->setWorkflow(Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow);
								pbsDatablock->setFresnel(Ogre::Vector3(1.0f, 1.0f, 1.0f), false);
								pbsDatablock->setRoughness(0.001f);
								this->setDataBlockPbsReflectionTextureName("cubemap");
							}
						}
						else
						{
							auto reflectionTexture = pbsDatablock->getTexture(Ogre::PbsTextureTypes::PBSM_REFLECTION);
							if (nullptr != reflectionTexture)
							{
								pbsDatablock->setTexture(static_cast<Ogre::uint8>(Ogre::PBSM_REFLECTION), nullptr);
								this->setDataBlockPbsReflectionTextureName("");
							}
						}
					}
				}
			}
		}
		else
		{
			Ogre::v1::Entity* entity = this->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				for (size_t i = 0; i < entity->getNumSubEntities(); i++)
				{
					Ogre::HlmsPbsDatablock* pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(entity->getSubEntity(i)->getDatablock());
					if (nullptr != pbsDatablock)
					{
						if (true == this->useReflection->getBool())
						{
							WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
							if (nullptr != workspaceBaseComponent)
							{
								pbsDatablock->setTexture(Ogre::PBSM_REFLECTION, workspaceBaseComponent->getDynamicCubemapTexture());
								pbsDatablock->setWorkflow(Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow);
								pbsDatablock->setFresnel(Ogre::Vector3(1.0f, 1.0f, 1.0f), false);
								pbsDatablock->setRoughness(0.001f);
								this->setDataBlockPbsReflectionTextureName("cubemap");
							}
						}
						else
						{
							auto reflectionTexture = pbsDatablock->getTexture(Ogre::PbsTextureTypes::PBSM_REFLECTION);
							if (nullptr != reflectionTexture)
							{
								pbsDatablock->setTexture(static_cast<Ogre::uint8>(Ogre::PBSM_REFLECTION), nullptr);
								this->setDataBlockPbsReflectionTextureName("");
							}
						}
					}
				}
			}
		}
	}

	bool GameObject::getUseReflection(void) const
	{
		return this->useReflection->getBool();
	}
	
	void GameObject::setGlobal(bool global)
	{
		this->global->setValue(global);
	}
	
	bool GameObject::getGlobal(void) const
	{
		return this->global->getBool();
	}

	void GameObject::setClampY(bool clampY)
	{
		this->clampY->setValue(clampY);
	}
	
	bool GameObject::getClampY(void) const
	{
		return this->clampY->getBool();
	}

	void GameObject::setReferenceId(unsigned long referenceId)
	{
		this->referenceId->setValue(referenceId);
	}

	unsigned long GameObject::getReferenceId(void) const
	{
		return this->referenceId->getULong();
	}

	void GameObject::setRenderQueueIndex(unsigned int renderQueueIndex)
	{
		if (renderQueueIndex > 254)
			renderQueueIndex = 254;

		// [0; 100) & [200; 225) default to FAST(i.e. for v2 objects, like Items); RenderQueue ID range[100; 200) & [225; 256)


		Ogre::Item* item = this->getMovableObject<Ogre::Item>();
		if (nullptr != item)
		{
			if (NOWA::RENDER_QUEUE_V1_MESH == renderQueueIndex ||
				NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND == renderQueueIndex ||
				renderQueueIndex <= 1)
			{
				renderQueueIndex = NOWA::RENDER_QUEUE_V2_MESH;
			}

			if (renderQueueIndex >= 100)
				renderQueueIndex = 99;

			if (renderQueueIndex < 200 && renderQueueIndex > 225)
				renderQueueIndex = 200;
		}
		else
		{
			Ogre::v1::Entity* entity = this->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				if (NOWA::RENDER_QUEUE_V2_MESH == renderQueueIndex || renderQueueIndex <= 1)
				{
					renderQueueIndex = NOWA::RENDER_QUEUE_V1_MESH;
				}

				if (renderQueueIndex < 100)
					renderQueueIndex = 100;

				if (renderQueueIndex >= 200 && renderQueueIndex < 225)
					renderQueueIndex = 226;
			}
		}

		this->renderQueueIndex->setValue(renderQueueIndex);
		if (nullptr != this->movableObject)
		{
			this->movableObject->setRenderQueueGroup(static_cast<Ogre::uint8>(renderQueueIndex));
		}
	}

	unsigned int GameObject::getRenderQueueIndex(void) const
	{
		return this->renderQueueIndex->getUInt();
	}

	void GameObject::setRenderDistance(unsigned int renderDistance)
	{
		// unsigned int negative not possible!
		if (renderDistance < 0)
		{
			renderDistance = 5;
		}
		this->renderDistance->setValue(renderDistance);

		if (0 != renderDistance)
		{
			if (static_cast<unsigned int>(this->lodDistance->getReal()) > this->renderDistance->getUInt())
			{
				this->setLodDistance(static_cast<Ogre::Real>(this->renderDistance->getUInt()));
			}

			if (this->shadowRenderingDistance->getUInt() > this->renderDistance->getUInt())
			{
				this->setShadowRenderingDistance(this->renderDistance->getUInt());
			}
		}

		if (nullptr != this->movableObject && renderDistance > 0)
		{
			this->movableObject->setRenderingDistance(static_cast<Ogre::Real>(renderDistance));
		}
	}

	unsigned int GameObject::getRenderDistance(void) const
	{
		return this->renderDistance->getUInt();
	}

	void GameObject::setLodDistance(Ogre::Real lodDistance)
	{
		if (lodDistance < 0.0f)
		{
			lodDistance = 5.0f;
		}

		if (0 != this->renderDistance->getUInt())
		{
			if (static_cast<unsigned int>(lodDistance) > this->renderDistance->getUInt())
			{
				lodDistance = static_cast<Ogre::Real>(this->renderDistance->getUInt());
			}
		}

		this->lodDistance->setValue(lodDistance);

		if (lodDistance > 0.0f)
		{
			Ogre::v1::MeshPtr v1Mesh;
			Ogre::MeshPtr v2Mesh;
			GameObject::eType type = GameObject::ENTITY;

			// Attention with hbu_static, there is also dynamic
			Ogre::String tempMeshFile;

			Ogre::Item* item = this->getMovableObject<Ogre::Item>();
			if (nullptr != item)
			{
				tempMeshFile = item->getMesh()->getName();

				try
				{
					if ((v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->getByName(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
					{
						v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->load(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
																				Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
					}

					// Generate LOD levels
					Ogre::LodConfig lodConfig;

					Ogre::MeshLodGenerator lodGenerator;
					lodGenerator.getAutoconfig(v1Mesh, lodConfig);

					lodConfig.strategy = Ogre::LodStrategyManager::getSingleton().getDefaultStrategy();

					Ogre::Real factor[3] = { 0.0f, 0.0f, 0.0f };

					// Calculate factor:
					// 0 = 211788 / 41834 = 5
					// 1 = 41834 / 13236 = 3.16
					// 2 = 13236 / 5421 = 2.14

					for (short i = 0; i < lodConfig.levels.size(); i++)
					{
						if (i < lodConfig.levels.size() - 1)
						{
							factor[i] = lodConfig.levels[i].distance / lodConfig.levels[i + 1].distance;
						}
					}

					// E.g. lodDistance = 10
					// 0 = 10 * 5 = 50 meter
					// 1 = 10 * 3.14 = 30 meter
					// 2 = 10 * 2.14 = 20 meter
					// 3 = 10 meter
					for (short i = 0; i < lodConfig.levels.size(); i++)
					{
						if (i < lodConfig.levels.size() - 1)
						{
							lodConfig.levels[i].distance = lodConfig.strategy->transformUserValue(lodDistance * factor[i]);
						}
						else
						{
							lodConfig.levels[i].distance = lodConfig.strategy->transformUserValue(lodDistance);
						}
					}

					lodGenerator.generateLodLevels(lodConfig);

					DeployResourceModule::getInstance()->removeResource(tempMeshFile);

					if ((v2Mesh = Ogre::MeshManager::getSingletonPtr()->getByName(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
					{
						v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(tempMeshFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, v1Mesh.get(), true, true, true);
						v2Mesh->load();
					}

					v1Mesh->unload();
					Ogre::v1::MeshManager::getSingletonPtr()->remove(v1Mesh);

					DeployResourceModule::getInstance()->tagResource(tempMeshFile, v2Mesh->getGroup());

					

					Ogre::ConfigFile cf;
					cf.load(Core::getSingletonPtr()->getResourcesName());

					// Save the v2 mesh *with* LODs to disk
					Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());

					Ogre::String filePathName;
					auto& locationList = Ogre::ResourceGroupManager::getSingletonPtr()->getResourceLocationList(v1Mesh->getGroup());
					for (auto it = locationList.cbegin(); it != locationList.cend(); ++it)
					{
						Ogre::String tempFilePathName = (*it)->archive->getName() + "//" + tempMeshFile;

						std::ifstream file(tempFilePathName.c_str());

						if (file.good())
						{
							filePathName = tempFilePathName;
							break;
						}
					}

					if (false == filePathName.empty())
					{
						serializer.exportMesh(v2Mesh.get(), filePathName);
					}
				}
				catch (...)
				{
				}
			}
			else
			{

				Ogre::v1::Entity* entity = this->getMovableObject<Ogre::v1::Entity>();
				if (nullptr != entity)
				{
					tempMeshFile = entity->getMesh()->getName();

					if ((v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->getByName(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
					{
						v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->load(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
																				Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
					}

					// Generate LOD levels
					Ogre::LodConfig lodConfig;

					Ogre::MeshLodGenerator lodGenerator;
					lodGenerator.getAutoconfig(v1Mesh, lodConfig);

					lodConfig.strategy = Ogre::LodStrategyManager::getSingleton().getDefaultStrategy();

					Ogre::Real factor[3] = { 0.0f, 0.0f, 0.0f };

					// Calculate factor:
					// 0 = 211788 / 41834 = 5
					// 1 = 41834 / 13236 = 3.16
					// 2 = 13236 / 5421 = 2.14

					for (short i = 0; i < lodConfig.levels.size(); i++)
					{
						if (i < lodConfig.levels.size() - 1)
						{
							factor[i] = lodConfig.levels[i].distance / lodConfig.levels[i + 1].distance;
						}
					}

					// E.g. lodDistance = 10
					// 0 = 10 * 5 = 50 meter
					// 1 = 10 * 3.14 = 30 meter
					// 2 = 10 * 2.14 = 20 meter
					// 3 = 10 meter
					for (short i = 0; i < lodConfig.levels.size(); i++)
					{
						if (i < lodConfig.levels.size() - 1)
						{
							lodConfig.levels[i].distance = lodConfig.strategy->transformUserValue(lodDistance * factor[i]);
						}
						else
						{
							lodConfig.levels[i].distance = lodConfig.strategy->transformUserValue(lodDistance);
						}
					}

					lodGenerator.generateLodLevels(lodConfig);

					Ogre::ConfigFile cf;
					cf.load(Core::getSingletonPtr()->getResourcesName());

					// Save the v2 mesh *with* LODs to disk
					Ogre::v1::MeshSerializer serializer;

					Ogre::String filePathName;
					auto& locationList = Ogre::ResourceGroupManager::getSingletonPtr()->getResourceLocationList(v1Mesh->getGroup());
					for (auto it = locationList.cbegin(); it != locationList.cend(); ++it)
					{
						Ogre::String tempFilePathName = (*it)->archive->getName() + "//" + tempMeshFile;

						std::ifstream file(tempFilePathName.c_str());

						if (file.good())
						{
							filePathName = tempFilePathName;
							break;
						}
					}

					if (false == filePathName.empty())
					{
						// Exports the mesh
						serializer.exportMesh(v1Mesh.get(), filePathName);
					}
				}
			}
		}
	}

	Ogre::Real GameObject::getLodDistance(void) const
	{
		return this->lodDistance->getReal();
	}

	void GameObject::setShadowRenderingDistance(unsigned int shadowRenderingDistance)
	{
		if (shadowRenderingDistance < 0)
		{
			shadowRenderingDistance = 5;
		}

		if (0 != this->renderDistance->getUInt())
		{
			if (shadowRenderingDistance > this->renderDistance->getUInt())
			{
				shadowRenderingDistance = this->renderDistance->getUInt();
			}
		}

		this->shadowRenderingDistance->setValue(shadowRenderingDistance);
		if (nullptr != this->movableObject && shadowRenderingDistance > 0)
		{
			this->movableObject->setShadowRenderingDistance(static_cast<Ogre::Real>(shadowRenderingDistance));
		}
	}

	unsigned int GameObject::getShadowRenderingDistance(void) const
	{
		return this->shadowRenderingDistance->getUInt();
	}

	std::pair<bool, Ogre::Real> GameObject::performRaycastForYClamping(void)
	{
		bool success = false;
		// Ogre::Vector3 result = Ogre::Vector3::ZERO;
		Ogre::Real closestDistance = 0.0f;

		if (true == this->clampY->getBool())
		{
			// Render must be called, else, when game object is loaded somehow ogre is not ready, and just shadow camera are found for ray cast and no objects!
			Ogre::Root::getSingletonPtr()->renderOneFrame();

			// Generate query for clamp object query for all kinds of categories
			if (nullptr == this->clampObjectQuery)
			{
				this->clampObjectQuery = this->sceneManager->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
				this->clampObjectQuery->setSortByDistance(true);
			}

			Ogre::MovableObject* hitMovableObject = nullptr;

			// Exclude this movable object
			std::vector<Ogre::MovableObject*> excludeMovableObjects(1);
			excludeMovableObjects[0] = this->getMovableObject();

			if (MathHelper::getInstance()->getRaycastHeight(this->sceneNode->getPosition().x, this->sceneNode->getPosition().z, this->clampObjectQuery, closestDistance, &excludeMovableObjects))
			{
				// Move the game object to the bottom center of the entity mesh
				closestDistance += this->getBottomOffset().y;

				bool nonDynamicGameObject = false;
				if (false == this->isDynamic())
				{
					nonDynamicGameObject = true;
					this->setDynamic(true);
				}
				this->sceneNode->setPosition(this->sceneNode->getPosition().x, closestDistance, this->sceneNode->getPosition().z);
				if (true == nonDynamicGameObject)
				{
					this->setDynamic(false);
				}
				success = true;
			}
		}

		return std::make_pair(success, closestDistance);
	}

	void GameObject::showBoundingBox(bool show)
	{
		if (true == show && nullptr != this->boundingBoxDraw)
		{
			this->boundingBoxDraw->track(this->movableObject);
		}
		else
		{
			this->boundingBoxDraw->track(nullptr);
		}
	}

	Variant* GameObject::getAttribute(const Ogre::String& attributeName)
	{
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->attributes.size()); i++)
		{
			if (this->attributes[i].first == attributeName)
			{
				return this->attributes[i].second;
			}
		}
		// In game object not found, check all components
		for (auto& it = this->gameObjectComponents.cbegin(); it != this->gameObjectComponents.end(); ++it)
		{
			return std::get<COMPONENT>(*it)->getAttribute(attributeName);
		}
		return nullptr;
	}

	std::vector<std::pair<Ogre::String, Variant*>>& GameObject::getAttributes(void)
	{
		return this->attributes;
	}

	GameObject::eType GameObject::getType(void) const
	{
		return this->type;
	}

	void GameObject::setOriginalMeshNameOnLoadFailure(const Ogre::String& originalMeshName)
	{
		this->originalMeshName = originalMeshName;
	}

	Ogre::String GameObject::getOriginalMeshNameOnLoadFailure(void) const
	{
		return this->originalMeshName;
	}

	void GameObject::setCustomDataString(const Ogre::String& customDataString)
	{
		this->customDataString = customDataString;
	}

	Ogre::String GameObject::getCustomDataString(void) const
	{
		return this->customDataString;
	}

	bool GameObject::isSelected(void) const
	{
		return this->selected;
	}

	void GameObject::setDoNotDestroyMovableObject(bool doNotDestroy)
	{
		this->doNotDestroyMovableObject = doNotDestroy;
	}
	
	LuaScript* GameObject::getLuaScript(void) const
	{
		return this->luaScript;
	}

	void GameObject::setConnectedGameObject(boost::weak_ptr<GameObject> weakConnectedGameObjectPtr)
	{
		this->connectedGameObjectPtr = NOWA::makeStrongPtr(weakConnectedGameObjectPtr);
	}

	void GameObject::setDataBlockPbsReflectionTextureName(const Ogre::String& textureName)
	{
		unsigned int i = 0;
		boost::shared_ptr<DatablockPbsComponent> datablockPbsCompPtr = nullptr;
		do
		{
			datablockPbsCompPtr = NOWA::makeStrongPtr(this->getComponentWithOccurrence<DatablockPbsComponent>(i));
			if (nullptr != datablockPbsCompPtr)
			{
				datablockPbsCompPtr->setReflectionTextureName(textureName);
				i++;
			}
		} while (nullptr != datablockPbsCompPtr);
	}

	boost::weak_ptr<GameObject> GameObject::getConnectedGameObjectPtr(void) const
	{
		return this->connectedGameObjectPtr;
	}

	std::vector<Ogre::String> GameObject::getDatablockNames(void)
	{
		std::vector<Ogre::String> datablockNames(this->dataBlocks.size());

		for (size_t i = 0; i < this->dataBlocks.size(); i++)
		{
			if (nullptr != this->dataBlocks[i])
			{
				datablockNames[i] = dataBlocks[i]->getString();
			}
		}

		return datablockNames;
	}

}; //namespace end