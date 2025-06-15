#include "NOWAPrecompiled.h"
#include "MeshModifyComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "OgreBitwise.h"
#include "Vao/OgreAsyncTicket.h"
#include "OgreEntity.h"
#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreSubMesh.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	MeshModifyComponent::MeshModifyComponent()
		: GameObjectComponent(),
		name("MeshModifyComponent"),
		raySceneQuery(nullptr),
		isPressing(false),
		brushPosition(Ogre::Vector3::ZERO),
		currentEditingObject(nullptr),
		originalVertices(nullptr),
		originalVertexCount(0),
		originalIndices(nullptr),
		originalIndexCount(0),
		isVET_HALF4(false),
		isIndices32(false),
		editableItem(nullptr),
		dynamicVertexBuffer(nullptr),
		dynamicIndexBuffer(nullptr),
		activated(new Variant(MeshModifyComponent::AttrActivated(), true, this->attributes)),
		brushSize(new Variant(MeshModifyComponent::AttrBrushSize(), static_cast<int>(64), this->attributes)),
		brushIntensity(new Variant(MeshModifyComponent::AttrBrushIntensity(), static_cast<int>(255), this->attributes)),
		brushName(new Variant(MeshModifyComponent::AttrBrushName(), this->attributes)),
		category(new Variant(MeshModifyComponent::AttrCategory(), Ogre::String(), this->attributes))
	{
		
	}

	MeshModifyComponent::~MeshModifyComponent(void)
	{
		
	}

	const Ogre::String& MeshModifyComponent::getName() const
	{
		return this->name;
	}

	void MeshModifyComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MeshModifyComponent>(MeshModifyComponent::getStaticClassId(), MeshModifyComponent::getStaticClassName());
	}
	
	void MeshModifyComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool MeshModifyComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Category")
		{
			this->category->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr MeshModifyComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool MeshModifyComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Init component for game object: " + this->gameObjectPtr->getName());

		Ogre::StringVectorPtr brushNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Brushes", "*.png");
		std::vector<Ogre::String> compatibleBrushNames(brushNames.getPointer()->size() + 1);
		unsigned int i = 0;
		for (auto& it = brushNames.getPointer()->cbegin(); it != brushNames.getPointer()->cend(); it++)
		{
			compatibleBrushNames[i++] = *it;
		}

		this->brushName->setValue(compatibleBrushNames);
		if (compatibleBrushNames.size() > 1)
		{
			this->brushName->setListSelectedValue(compatibleBrushNames[1]);
		}
		this->brushName->addUserData(GameObject::AttrActionImage());
		this->brushName->addUserData(GameObject::AttrActionNoUndo());

		this->brushSize->setConstraints(1.0f, 5000.0f);
		this->brushSize->addUserData(GameObject::AttrActionNoUndo());

		this->brushIntensity->setConstraints(1.0f, 255.0f);
		this->brushIntensity->addUserData(GameObject::AttrActionNoUndo());

		// If a listener has been added via key/mouse/joystick pressed, a new listener would be inserted during this iteration, which would cause a crash in mouse/key/button release iterator, hence add in next frame
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]() {
			InputDeviceCore::getSingletonPtr()->addMouseListener(this, MeshModifyComponent::getStaticClassName());
			};
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

		Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
		if (nullptr != originalItem)
		{
			Ogre::MeshPtr originalMesh = originalItem->getMesh();
			// Clone the mesh with a new name
			Ogre::MeshPtr clonedMesh = originalMesh->clone(this->gameObjectPtr->getName() + "_Editable", "", Ogre::BT_DEFAULT, Ogre::BT_DEFAULT);

			// Set buffer policies for modification
			clonedMesh->setVertexBufferPolicy(Ogre::BT_DEFAULT, false);
			clonedMesh->setIndexBufferPolicy(Ogre::BT_DEFAULT, false);


			// Recalculate bounds and notify
			clonedMesh->load();
			clonedMesh->_dirtyState();

			for (Ogre::SubMesh* subMesh : clonedMesh->getSubMeshes())
			{
				// Normal means here: casual pass, other option is with shadow caster. So it has nothing todo with vertex normals!
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[Ogre::VpNormal];

				if (false == vaos.empty())
				{
					Ogre::VertexArrayObject* vao = vaos[0];

					this->dynamicVertexBuffer = vao->getBaseVertexBuffer();

					this->dynamicIndexBuffer = vao->getIndexBuffer();
				}
			}

			// Use the cloned mesh instead of the original one
			this->editableItem = this->gameObjectPtr->getSceneManager()->createItem(clonedMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
			for (size_t i = 0; i < this->editableItem->getNumSubItems(); i++)
			{
				auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->editableItem->getSubItem(i)->getDatablock());
				if (nullptr != sourceDataBlock)
				{
					this->editableItem->getSubItem(i)->setDatablock(sourceDataBlock);

					// Deactivate fresnel by default, because it looks ugly
					if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
					{
						sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
					}
				}
			}

			this->gameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->gameObjectPtr->getSceneNode()->attachObject(this->editableItem);
			this->gameObjectPtr->init(this->editableItem);

			

			// Get mesh data in world space
			MathHelper::getInstance()->getDetailedMeshInformation2(clonedMesh, this->originalVertexCount, this->originalVertices, this->originalNormals, this->originalTextureCoordinates,
				this->originalIndexCount, this->originalIndices, this->editableItem->getParentNode()->_getDerivedPositionUpdated(),
				this->editableItem->getParentNode()->_getDerivedOrientationUpdated(),
				this->editableItem->getParentNode()->getScale(), this->isVET_HALF4, this->isIndices32);

			// Get the world transformation matrix and its inverse
			Ogre::Matrix4 invWorldMatrix = this->editableItem->getParentNode()->_getFullTransform().inverse();

			// Get the rotation part of the inverse world matrix for normals
			Ogre::Matrix3 invWorldRotation;
			invWorldMatrix.extract3x3Matrix(invWorldRotation);

			// Transform vertices and normals from world space to object space - do this only ONCE
			for (size_t i = 0; i < this->originalVertexCount; ++i)
			{
				// Transform each vertex by applying the inverse of the world matrix
				this->originalVertices[i] = invWorldMatrix * this->originalVertices[i];
				this->originalNormals[i] = invWorldRotation * this->originalNormals[i];
				// Normalize the normal after transformation
				this->originalNormals[i].normalise();
			}


			//// Convert cached vertices to local space
			//for (size_t i = 0; i < this->originalVertexCount; ++i)
			//{
			//	this->originalVertices[i] = invWorldMatrix * this->originalVertices[i];
			//}
		}

		return true;
	}

	bool MeshModifyComponent::connect(void)
	{
		GameObjectComponent::connect();
		
		return true;
	}

	bool MeshModifyComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();
		return true;
	}

	bool MeshModifyComponent::onCloned(void)
	{
		
		return true;
	}

	void MeshModifyComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		if (nullptr != this->raySceneQuery)
		{
			this->gameObjectPtr->getSceneManager()->destroyQuery(this->raySceneQuery);
			this->raySceneQuery = nullptr;
		}
;
		InputDeviceCore::getSingletonPtr()->removeMouseListener(MeshModifyComponent::getStaticClassName());

		if (nullptr != this->dynamicVertexBuffer)
		{
			if (this->dynamicVertexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
			{
				this->dynamicVertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
			}

			if (this->dynamicVertexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
			{
				this->dynamicVertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
			}

			// Ogre does the destruction
			// vaoManager->destroyVertexBuffer(this->dynamicVertexBuffer);
			this->dynamicVertexBuffer = nullptr;

			// vaoManager->destroyIndexBuffer(this->dynamicIndexBuffer);
			this->dynamicIndexBuffer = nullptr;
		}
	}
	
	void MeshModifyComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void MeshModifyComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void MeshModifyComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void MeshModifyComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (MeshModifyComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (MeshModifyComponent::AttrBrushName() == attribute->getName())
		{
			this->setBrushName(attribute->getListSelectedValue());
		}
		else if (MeshModifyComponent::AttrBrushSize() == attribute->getName())
		{
			this->setBrushSize(attribute->getInt());
		}
		else if (MeshModifyComponent::AttrBrushIntensity() == attribute->getName())
		{
			this->setBrushIntensity(attribute->getReal());
		}
		else if (MeshModifyComponent::AttrCategory() == attribute->getName())
		{
			this->setCategory(attribute->getString());
		}
	}

	void MeshModifyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Category"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->category->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MeshModifyComponent::getClassName(void) const
	{
		return "MeshModifyComponent";
	}

	Ogre::String MeshModifyComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void MeshModifyComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool MeshModifyComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void MeshModifyComponent::setBrushName(const Ogre::String& brushName)
	{
		this->brushName->setListSelectedValue(brushName);

		Ogre::Image2 brushImage;
		brushImage.load(brushName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		brushImage.resize(this->brushSize->getInt(), this->brushSize->getInt());

		size_t size = brushImage.getWidth() * brushImage.getHeight();
		if (0 == size)
			return;

		this->brushData.resize(size, 0);

		Ogre::ColourValue cval;
		for (uint32 y = 0; y < brushImage.getHeight(); y++)
		{
			for (uint32 x = 0; x < brushImage.getWidth(); x++)
			{
				cval = brushImage.getColourAt(x, y, 0);
				this->brushData[y * brushImage.getWidth() + x] = cval.r;
			}
		}
	}

	Ogre::String MeshModifyComponent::getBrushName(void) const
	{
		return this->brushName->getListSelectedValue();
	}

	void MeshModifyComponent::setBrushSize(int brushSize)
	{
		this->brushSize->setValue(brushSize);
	}

	int MeshModifyComponent::getBrushSize(void) const
	{
		return this->brushSize->getInt();
	}

	void MeshModifyComponent::setBrushIntensity(Ogre::Real brushIntensity)
	{
		this->brushIntensity->setValue(brushIntensity);
	}

	Ogre::Real MeshModifyComponent::getBrushIntensity(void) const
	{
		return this->brushIntensity->getReal();
	}

	void MeshModifyComponent::setCategory(const Ogre::String& category)
	{
		this->category->setValue(category);
	}

	Ogre::String MeshModifyComponent::getCategory(void) const
	{
		return this->category->getString();
	}

	bool MeshModifyComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (true == this->brushData.empty())
		{
			return false;
		}

		if (id == OIS::MB_Left && nullptr != this->editableItem && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == this->activated->getBool())
		{
			Ogre::Real x = 0.0f;
			Ogre::Real y = 0.0f;
			MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());

			Ogre::Ray ray = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera()->getCameraToViewportRay(x, y);
			if (nullptr == this->raySceneQuery)
			{
				this->raySceneQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
				this->raySceneQuery->setSortByDistance(true);
			}
			else
			{
				this->raySceneQuery->setQueryMask(GameObjectController::ALL_CATEGORIES_ID);
			}

			Ogre::Vector3* vertices = nullptr;
			unsigned long* indices = nullptr;
			size_t vertexCount = 0;
			size_t indexCount = 0;

			if (true == MathHelper::getInstance()->getRaycastDetailsResult(ray, this->raySceneQuery, this->brushPosition,
				(size_t&)this->currentEditingObject, vertices, vertexCount, indices, indexCount))
			{
				if (this->currentEditingObject)
				{
					this->isPressing = true;

					// First modification
					this->modifyMeshWithBrush(this->brushPosition);
				}
			}

		}

		return false;
	}

	bool MeshModifyComponent::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (true == this->brushData.empty())
		{
			return false;
		}

		/*if (true == this->isPressing && nullptr != this->currentEditingObject && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == this->activated->getBool())
		{
			Ogre::Real x = 0.0f;
			Ogre::Real y = 0.0f;
			MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());

			Ogre::Ray ray = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera()->getCameraToViewportRay(x, y);

			size_t dummyObject;
			Ogre::Vector3* vertices = nullptr;
			unsigned long* indices = nullptr;
			size_t vertexCount = 0;
			size_t indexCount = 0;

			if (true == MathHelper::getInstance()->getRaycastDetailsResult(ray, this->raySceneQuery, this->brushPosition, (size_t&)dummyObject, vertices, vertexCount, indices, indexCount))
			{
				this->modifyMeshWithBrush(this->brushPosition);
			}
		}*/

		return false;
	}

	bool MeshModifyComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (true == this->brushData.empty())
		{
			return false;
		}

		if (id == OIS::MB_Left && true == this->isPressing && nullptr != this->currentEditingObject && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == this->activated->getBool())
		{
			// Create a new mesh from the modified data
			// this->updateMesh(currentEditingObject, this->originalVertices, this->originalVertexCount, this->originalIndices, this->originalIndexCount);

			// Cleanup
			OGRE_FREE(originalVertices, Ogre::MEMCATEGORY_GEOMETRY);
			OGRE_FREE(originalIndices, Ogre::MEMCATEGORY_GEOMETRY);
			this->originalVertices = nullptr;
			this->originalIndices = nullptr;
			this->originalVertexCount = 0;
			this->originalIndexCount = 0;

			this->isPressing = false;
			this->currentEditingObject = nullptr;
		}

		return false;
	}

	Ogre::Real MeshModifyComponent::getBrushValue(size_t vertexIndex)
	{
		if (true == this->brushData.empty())
		{
			return 0.0f;
		}

		size_t brushWidth = this->brushSize->getInt(); // Brush width (e.g., 64)
		size_t brushHeight = this->brushSize->getInt(); // Brush height (e.g., 64)

		// Map vertexIndex to a corresponding brush index
		size_t brushX = (vertexIndex % brushWidth); // X coordinate in brush image
		size_t brushY = (vertexIndex / brushWidth); // Y coordinate in brush image

		// Ensure we are within bounds of the brush data
		size_t index = brushY * brushWidth + brushX;

		// Get the brush value, if within bounds
		Ogre::Real value = (index < this->brushData.size()) ? this->brushData[index] : 0.0f;

		return value;
	}

	void MeshModifyComponent::modifyMeshWithBrush(const Ogre::Vector3& brushPosition)
	{
		std::vector<float> interleavedData;
		interleavedData.reserve(originalVertexCount * 8); // 8 floats per vertex (3 pos, 3 normal, 2 texcoords)

		std::vector<Ogre::Vector3> modifiedVertices(this->originalVertices, this->originalVertices + this->originalVertexCount);
		std::vector<Ogre::Vector3> newNormals(originalVertexCount, Ogre::Vector3::ZERO);

		// Modify vertices and reset normals in one loop
		for (size_t i = 0; i < originalVertexCount; ++i)
		{
			Ogre::Vector3& vertex = modifiedVertices[i];

			// Compute brush influence
			Ogre::Real brushIntensity = this->getBrushValue(i);
			Ogre::Real distance = vertex.distance(brushPosition);
			if (distance <= this->brushSize->getInt())
			{
				Ogre::Real intensity = (1.0f - (distance / this->brushSize->getInt())) * brushIntensity;
				vertex += (vertex - brushPosition).normalisedCopy() * intensity * 0.1f;
			}

			// Reset normals (to prepare for accumulation)
			newNormals[i] = Ogre::Vector3::ZERO;
		}

		// Compute face normals and accumulate
		for (size_t i = 0; i < originalIndexCount; i += 3)
		{
			size_t i0 = originalIndices[i];
			size_t i1 = originalIndices[i + 1];
			size_t i2 = originalIndices[i + 2];

			Ogre::Vector3& v0 = modifiedVertices[i0];
			Ogre::Vector3& v1 = modifiedVertices[i1];
			Ogre::Vector3& v2 = modifiedVertices[i2];

			Ogre::Vector3 edge1 = v1 - v0;
			Ogre::Vector3 edge2 = v2 - v0;
			Ogre::Vector3 normal = edge1.crossProduct(edge2);
			normal.normalise();

			// Accumulate normals
			newNormals[i0] += normal;
			newNormals[i1] += normal;
			newNormals[i2] += normal;
		}

		// Normalize accumulated normals and interleave data in a single loop
		for (size_t i = 0; i < originalVertexCount; ++i)
		{
			newNormals[i].normalise();

			// Add position (3 floats)
			interleavedData.push_back(modifiedVertices[i].x);
			interleavedData.push_back(modifiedVertices[i].y);
			interleavedData.push_back(modifiedVertices[i].z);

			// Add recalculated normal (3 floats)
			interleavedData.push_back(newNormals[i].x);
			interleavedData.push_back(newNormals[i].y);
			interleavedData.push_back(newNormals[i].z);

			// Add texture coordinates (2 floats)
			interleavedData.push_back(this->originalTextureCoordinates[i].x);
			interleavedData.push_back(this->originalTextureCoordinates[i].y);
		}


		// Upload modified vertices to the dynamic vertex buffer
		this->dynamicVertexBuffer->upload(interleavedData.data(), 0, originalVertexCount);
	}

#if 0
	bool MeshModifyComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID && gameObject->getComponentCount<MeshModifyComponent>() < 2)
		{
			return true;
		}
		return false;
	}
#else
	bool MeshModifyComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (gameObject->getComponentCount<MeshModifyComponent>() < 2 && nullptr != gameObject->getMovableObjectUnsafe<Ogre::Item>())
		{
			if (1 == gameObject->getMovableObjectUnsafe<Ogre::Item>()->getNumSubItems())
			{
				return true;
			}
		}
		return false;
	}
#endif

	// Lua registration part

	MeshModifyComponent* getMeshModifyComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MeshModifyComponent>(gameObject->getComponentWithOccurrence<MeshModifyComponent>(occurrenceIndex)).get();
	}

	MeshModifyComponent* getMeshModifyComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MeshModifyComponent>(gameObject->getComponent<MeshModifyComponent>()).get();
	}

	MeshModifyComponent* getMeshModifyComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MeshModifyComponent>(gameObject->getComponentFromName<MeshModifyComponent>(name)).get();
	}

	void MeshModifyComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<MeshModifyComponent, GameObjectComponent>("MeshModifyComponent")
			.def("setActivated", &MeshModifyComponent::setActivated)
			.def("isActivated", &MeshModifyComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "class inherits GameObjectComponent", MeshModifyComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "GameObject getOwner()", "Gets the owner game object.");

		gameObjectClass.def("getMeshModifyComponentFromName", &getMeshModifyComponentFromName);
		gameObjectClass.def("getMeshModifyComponent", (MeshModifyComponent * (*)(GameObject*)) & getMeshModifyComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getMeshModifyComponentFromIndex", (MeshModifyComponent * (*)(GameObject*, unsigned int)) & getMeshModifyComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castMeshModifyComponent", &GameObjectController::cast<MeshModifyComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MeshModifyComponent castMeshModifyComponent(MeshModifyComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

}; //namespace end