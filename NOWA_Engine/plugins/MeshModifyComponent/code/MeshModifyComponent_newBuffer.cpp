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
		editableItem(nullptr),
		isVET_HALF4(false),
		isIndices32(false),
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
			Ogre::MeshPtr clonedMesh = originalMesh->clone(this->gameObjectPtr->getName() + "_Editable");

			// Set buffer policies for modification
			clonedMesh->setVertexBufferPolicy(Ogre::BT_DEFAULT, true);
			clonedMesh->setIndexBufferPolicy(Ogre::BT_DEFAULT, true);

			// Recalculate bounds and notify
			clonedMesh->load();
			clonedMesh->_dirtyState();

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

			// Get the world transformation matrix and its inverse
			Ogre::Matrix4 invWorldMatrix = this->editableItem->getParentNode()->_getFullTransform().inverse();


			// Get mesh data in world space
			MathHelper::getInstance()->getDetailedMeshInformation2(clonedMesh, this->originalVertexCount, this->originalVertices, this->originalNormals, this->originalTextureCoordinates,
				this->originalIndexCount, this->originalIndices, this->editableItem->getParentNode()->_getDerivedPositionUpdated(),
				this->editableItem->getParentNode()->_getDerivedOrientationUpdated(),
				this->editableItem->getParentNode()->getScale(), this->isVET_HALF4, this->isIndices32);

			// Convert cached vertices to local space
			for (size_t i = 0; i < this->originalVertexCount; ++i)
			{
				this->originalVertices[i] = invWorldMatrix * this->originalVertices[i];
			}

			std::vector<Ogre::Vector3> newVertices(this->originalVertices, this->originalVertices + this->originalVertexCount);
			std::vector<unsigned long> newIndices(this->originalIndices, this->originalIndices + this->originalIndexCount);

			// Now apply the modification to the mesh
			// Here you can proceed with updating the mesh with newVertices, like you were doing
			this->initMeshWithModifiedVertices(this->editableItem, newVertices, this->originalVertexCount, newIndices, this->originalIndexCount);
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

			Ogre::Vector3 intersectionPoint;
			Ogre::Vector3* vertices = nullptr;
			unsigned long* indices = nullptr;
			size_t vertexCount = 0;
			size_t indexCount = 0;

			if (true == MathHelper::getInstance()->getRaycastDetailsResult(ray, this->raySceneQuery, intersectionPoint,
				(size_t&)this->currentEditingObject, vertices, vertexCount, indices, indexCount))
			{
				if (this->currentEditingObject)
				{
					this->isPressing = true;

					// First modification
					this->applyBrushToMesh(this->currentEditingObject, intersectionPoint, this->originalVertices, vertexCount, this->originalIndices, indexCount, false);
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

		if (true == this->isPressing && nullptr != this->currentEditingObject && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == this->activated->getBool())
		{
			Ogre::Real x = 0.0f;
			Ogre::Real y = 0.0f;
			MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());

			Ogre::Ray ray = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera()->getCameraToViewportRay(x, y);

			Ogre::Vector3 intersectionPoint;
			size_t dummyObject;
			Ogre::Vector3* vertices = nullptr;
			unsigned long* indices = nullptr;
			size_t vertexCount = 0;
			size_t indexCount = 0;

			if (true == MathHelper::getInstance()->getRaycastDetailsResult(ray, this->raySceneQuery, intersectionPoint, (size_t&)dummyObject, vertices, vertexCount, indices, indexCount))
			{
				this->applyBrushToMesh(this->currentEditingObject, intersectionPoint, this->originalVertices, this->originalVertexCount, this->originalIndices, this->originalIndexCount, false);
			}
		}

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

	void MeshModifyComponent::applyBrushToMesh(Ogre::MovableObject* movableObject, const Ogre::Vector3& hitPoint, Ogre::Vector3* vertices, size_t vertexCount, unsigned long* indices, size_t indexCount, bool isSphere)
	{
		if (nullptr == movableObject)
		{
			return;
		}

		// Create a copy of the mesh vertices for modification
		std::vector<Ogre::Vector3> modifiedVertices(vertices, vertices + vertexCount);

		// Map brush data to vertices
		int brushSize = this->brushSize->getInt(); // Assuming this is the dimension of your brush (e.g., 64x64)

		for (size_t i = 0; i < vertexCount; i++)
		{
			Ogre::Real intensity = this->getBrushIntensity(i, movableObject);

			// Calculate direction for modification, inverting the effect to create a hill
			Ogre::Vector3 direction = isSphere ? (modifiedVertices[i] - hitPoint).normalisedCopy() : Ogre::Vector3::UNIT_Y;

			// Apply the intensity and create a hill (inverting the effect to push vertices upwards)
			modifiedVertices[i] += direction * intensity;

			// Optional: You can add logic to invert the intensity to create a hill (e.g., negative direction)
			// modifiedVertices[i] -= direction * intensity;  // If you want to lower the vertices instead of raising them
		}

		this->updateMesh(movableObject, modifiedVertices.data(), vertexCount, indices, indexCount);
	}

	Ogre::Real MeshModifyComponent::getBrushIntensity(size_t vertexIndex, Ogre::MovableObject* movableObject)
	{
		if (true == brushData.empty())
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

	void MeshModifyComponent::updateMesh(Ogre::MovableObject* movableObject, Ogre::Vector3* newRawVertices, size_t vertexCount, unsigned long* newRawIndices, size_t indexCount)
	{
		if (nullptr == movableObject)
		{
			return;
		}

		std::vector<Ogre::Vector3> newVertices(newRawVertices, newRawVertices + vertexCount);
		std::vector<unsigned long> newIndices(newRawIndices, newRawIndices + indexCount);

		// Now apply the modification to the mesh
		// Here you can proceed with updating the mesh with newVertices, like you were doing
		this->updateMeshWithModifiedVertices(movableObject, newVertices, vertexCount, newIndices, indexCount);
	}

#if 0
	void MeshModifyComponent::updateMeshWithModifiedVertices(Ogre::MovableObject* movableObject, const std::vector<Ogre::Vector3>& newVertices, size_t vertexCount, const std::vector<unsigned long>& newIndices, size_t indexCount)
	{
		if (nullptr == movableObject)
		{
			return;
		}

		Ogre::String type = movableObject->getMovableType();
		if (type.compare("Entity") == 0)
		{
			Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(movableObject);
			Ogre::v1::MeshPtr mesh = entity->getMesh();

			// Process all submeshes
			size_t submeshCount = mesh->getNumSubMeshes();
			size_t currentVertexOffset = 0;
			size_t currentIndexOffset = 0;

			for (size_t i = 0; i < submeshCount; ++i)
			{
				Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);

				// Update vertex data
				Ogre::v1::VertexData* vertexData = subMesh->useSharedVertices ? mesh->sharedVertexData[0] : subMesh->vertexData[0];
				if (vertexData)
				{
					Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(0);
					size_t numVerticesInSubmesh = vertexData->vertexCount;

					if (currentVertexOffset + numVerticesInSubmesh <= vertexCount)
					{
						void* vData = vbuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);
						memcpy(vData, &newVertices[currentVertexOffset], numVerticesInSubmesh * sizeof(Ogre::Vector3));
						vbuf->unlock();

						currentVertexOffset += numVerticesInSubmesh;
					}
				}

				// Update index data if provided
				if (false == newIndices.empty() && indexCount > 0)
				{
					Ogre::v1::HardwareIndexBufferSharedPtr ibuf = subMesh->indexData[0]->indexBuffer;
					if (ibuf)
					{
						size_t numIndicesInSubmesh = subMesh->indexData[0]->indexCount / 3;  // Triangle count

						if (currentIndexOffset + numIndicesInSubmesh <= indexCount)
						{
							void* iData = ibuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

							// Determine how to copy data based on index buffer type
							if (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_16BIT)
							{
								uint16_t* indices = static_cast<uint16_t*>(iData);
								for (size_t j = 0; j < numIndicesInSubmesh; ++j)
								{
									// Convert from Vector3 to uint16_t indices (each Vector3 represents one triangle)
									indices[j * 3 + 0] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 0]);
									indices[j * 3 + 1] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 1]);
									indices[j * 3 + 2] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 2]);
								}
							}
							else // 32-bit indices
							{
								uint32_t* indices = static_cast<uint32_t*>(iData);
								for (size_t j = 0; j < numIndicesInSubmesh; ++j)
								{
									// Convert from Vector3 to uint32_t indices
									indices[j * 3 + 0] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 0]);
									indices[j * 3 + 1] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 1]);
									indices[j * 3 + 2] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 2]);
								}
							}

							ibuf->unlock();
							currentIndexOffset += numIndicesInSubmesh;
						}
					}
				}
			}

			// Mark the mesh as having changed
			mesh->_dirtyState();
		}
		else if (type.compare("Item") == 0)
		{
			Ogre::Item* item = static_cast<Ogre::Item*>(movableObject);
			Ogre::MeshPtr mesh = item->getMesh();
			Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
			Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

			size_t currentVertexOffset = 0;
			size_t currentIndexOffset = 0;

			Ogre::Mesh::SubMeshVec::const_iterator subMeshIterator = mesh->getSubMeshes().begin();
			while (subMeshIterator != mesh->getSubMeshes().end())
			{
				Ogre::SubMesh* subMesh = *subMeshIterator;
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[Ogre::VpNormal];

				if (!vaos.empty())
				{
					Ogre::VertexArrayObject* vao = vaos[0];
					bool hasIndexBuffer = (vao->getIndexBuffer() != nullptr);
					bool indices32 = false;

					if (hasIndexBuffer)
					{
						indices32 = (vao->getIndexBuffer()->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);
					}

					// Modify Vertex Positions
					Ogre::VertexArrayObject::ReadRequestsVec requests;
					requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
					vao->readRequests(requests);
					vao->mapAsyncTickets(requests);

					unsigned int subMeshVerticesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());

					void* vertexData = requests[0].vertexBuffer->map(0, subMeshVerticesNum);
					if (requests[0].type == Ogre::VET_HALF4)
					{
						Ogre::uint16* pos = static_cast<Ogre::uint16*>(vertexData);
						for (size_t i = 0; i < subMeshVerticesNum && (currentVertexOffset + i) < vertexCount; ++i)
						{
							// Apply new vertex positions
							pos[0] = Ogre::Bitwise::floatToHalf(newVertices[currentVertexOffset + i].x);
							pos[1] = Ogre::Bitwise::floatToHalf(newVertices[currentVertexOffset + i].y);
							pos[2] = Ogre::Bitwise::floatToHalf(newVertices[currentVertexOffset + i].z);

							pos += requests[0].vertexBuffer->getBytesPerElement() / sizeof(Ogre::uint16);
						}
					}
					else if (requests[0].type == Ogre::VET_FLOAT3)
					{
						float* pos = static_cast<float*>(vertexData);
						for (size_t i = 0; i < subMeshVerticesNum && (currentVertexOffset + i) < vertexCount; ++i)
						{
							pos[0] = newVertices[currentVertexOffset + i].x;
							pos[1] = newVertices[currentVertexOffset + i].y;
							pos[2] = newVertices[currentVertexOffset + i].z;

							pos += requests[0].vertexBuffer->getBytesPerElement() / sizeof(float);
						}
					}
					requests[0].vertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
					vao->unmapAsyncTickets(requests);

					currentVertexOffset += subMeshVerticesNum;

					// Modify Index Buffer
					if (!newIndices.empty() && indexCount > 0 && hasIndexBuffer)
					{
						Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
						unsigned int numIndices = static_cast<unsigned int>(indexBuffer->getNumElements());
						unsigned int triangleCount = numIndices / 3;

						if (currentIndexOffset < indexCount)
						{
							void* idxData = indexBuffer->map(0, numIndices);
							if (indices32)
							{
								uint32_t* indexPtr = static_cast<uint32_t*>(idxData);
								for (size_t j = 0; j < triangleCount && (currentIndexOffset + j * 3) < indexCount; ++j)
								{
									indexPtr[j * 3 + 0] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 0]);
									indexPtr[j * 3 + 1] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 1]);
									indexPtr[j * 3 + 2] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 2]);
								}
							}
							else
							{
								uint16_t* indexPtr = static_cast<uint16_t*>(idxData);
								for (size_t j = 0; j < triangleCount && (currentIndexOffset + j * 3) < indexCount; ++j)
								{
									indexPtr[j * 3 + 0] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 0]);
									indexPtr[j * 3 + 1] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 1]);
									indexPtr[j * 3 + 2] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 2]);
								}
							}

							indexBuffer->unmap(Ogre::UO_UNMAP_ALL);
							currentIndexOffset += triangleCount;
						}
					}
				}
				++subMeshIterator;
			}

			mesh->_dirtyState();
		}
	}
#endif


#if 0
	void MeshModifyComponent::updateMeshWithModifiedVertices(Ogre::MovableObject* movableObject, const std::vector<Ogre::Vector3>& newVertices, size_t vertexCount, const std::vector<unsigned long>& newIndices, size_t indexCount)
	{
		if (nullptr == movableObject)
		{
			return;
		}

		Ogre::String type = movableObject->getMovableType();
		if (type.compare("Entity") == 0)
		{
			Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(movableObject);
			Ogre::v1::MeshPtr mesh = entity->getMesh();

			// Process all submeshes
			size_t submeshCount = mesh->getNumSubMeshes();
			size_t currentVertexOffset = 0;
			size_t currentIndexOffset = 0;

			for (size_t i = 0; i < submeshCount; ++i)
			{
				Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);

				// Update vertex data
				Ogre::v1::VertexData* vertexData = subMesh->useSharedVertices ? mesh->sharedVertexData[0] : subMesh->vertexData[0];
				if (vertexData)
				{
					Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(0);
					size_t numVerticesInSubmesh = vertexData->vertexCount;

					if (currentVertexOffset + numVerticesInSubmesh <= vertexCount)
					{
						void* vData = vbuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);
						memcpy(vData, &newVertices[currentVertexOffset], numVerticesInSubmesh * sizeof(Ogre::Vector3));
						vbuf->unlock();

						currentVertexOffset += numVerticesInSubmesh;
					}
				}

				// Update index data if provided
				if (false == newIndices.empty() && indexCount > 0)
				{
					Ogre::v1::HardwareIndexBufferSharedPtr ibuf = subMesh->indexData[0]->indexBuffer;
					if (ibuf)
					{
						size_t numIndicesInSubmesh = subMesh->indexData[0]->indexCount / 3;  // Triangle count

						if (currentIndexOffset + numIndicesInSubmesh <= indexCount)
						{
							void* iData = ibuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

							// Determine how to copy data based on index buffer type
							if (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_16BIT)
							{
								uint16_t* indices = static_cast<uint16_t*>(iData);
								for (size_t j = 0; j < numIndicesInSubmesh; ++j)
								{
									// Convert from Vector3 to uint16_t indices (each Vector3 represents one triangle)
									indices[j * 3 + 0] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 0]);
									indices[j * 3 + 1] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 1]);
									indices[j * 3 + 2] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 2]);
								}
							}
							else // 32-bit indices
							{
								uint32_t* indices = static_cast<uint32_t*>(iData);
								for (size_t j = 0; j < numIndicesInSubmesh; ++j)
								{
									// Convert from Vector3 to uint32_t indices
									indices[j * 3 + 0] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 0]);
									indices[j * 3 + 1] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 1]);
									indices[j * 3 + 2] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 2]);
								}
							}

							ibuf->unlock();
							currentIndexOffset += numIndicesInSubmesh;
						}
					}
				}
			}

			// Mark the mesh as having changed
			mesh->_dirtyState();
		}
		else if (type.compare("Item") == 0)
		{
			Ogre::Item* item = static_cast<Ogre::Item*>(movableObject);
			Ogre::MeshPtr mesh = item->getMesh();

			size_t currentVertexOffset = 0;
			size_t currentIndexOffset = 0;

			for (Ogre::SubMesh* subMesh : mesh->getSubMeshes())
			{
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[Ogre::VpNormal];

				if (!vaos.empty())
				{
					Ogre::VertexArrayObject* vao = vaos[0];

					// Get the current vertex buffer
					Ogre::VertexBufferPacked* vertexBuffer = vao->getVertexBuffers()[0];
					if (!vertexBuffer) continue;

					size_t numVertices = vertexBuffer->getNumElements();
					size_t vertexSize = vertexBuffer->getBytesPerElement();

					// Get the current index buffer
					Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
					size_t numIndices = indexBuffer ? indexBuffer->getNumElements() : 0;

					// --- Update Vertex Buffer ---
					float* vertexData = static_cast<float*>(
						vertexBuffer->map(0, numVertices)
						);

					for (size_t i = 0; i < numVertices; ++i)
					{
						vertexData[i * 3 + 0] = newVertices[currentVertexOffset + i].x;
						vertexData[i * 3 + 1] = newVertices[currentVertexOffset + i].y;
						vertexData[i * 3 + 2] = newVertices[currentVertexOffset + i].z;
					}

					vertexBuffer->unmap(Ogre::UO_KEEP_PERSISTENT);

					// --- Update Index Buffer ---
					if (indexBuffer && numIndices > 0)
					{
						void* indexData = indexBuffer->map(0, numIndices);

						if (indexBuffer->getIndexType() == Ogre::IndexBufferPacked::IT_16BIT)
						{
							uint16_t* indexData16 = static_cast<uint16_t*>(indexData);
							for (size_t i = 0; i < numIndices; ++i)
							{
								indexData16[i] = static_cast<uint16_t>(newIndices[currentIndexOffset + i]);
							}
						}
						else
						{
							uint32_t* indexData32 = static_cast<uint32_t*>(indexData);
							for (size_t i = 0; i < numIndices; ++i)
							{
								indexData32[i] = static_cast<uint32_t>(newIndices[currentIndexOffset + i]);
							}
						}

						indexBuffer->unmap(Ogre::UO_KEEP_PERSISTENT);
					}

					// --- Mark Mesh as Updated ---
					mesh->_dirtyState();

					// Update offsets
					currentVertexOffset += numVertices;
					currentIndexOffset += numIndices;
				}
			}
		}
	}
#endif

	void MeshModifyComponent::initMeshWithModifiedVertices(Ogre::MovableObject* movableObject, const std::vector<Ogre::Vector3>& newVertices, size_t vertexCount, const std::vector<unsigned long>& newIndices, size_t indexCount)
	{
		if (nullptr == movableObject)
		{
			return;
		}

		Ogre::String type = movableObject->getMovableType();
		if (type.compare("Item") == 0)
		{
			Ogre::Item* item = static_cast<Ogre::Item*>(movableObject);
			Ogre::MeshPtr mesh = item->getMesh();

			size_t currentVertexOffset = 0;
			size_t currentIndexOffset = 0;

			for (Ogre::SubMesh* subMesh : mesh->getSubMeshes())
			{
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[Ogre::VpNormal];

				if (!vaos.empty())
				{
					Ogre::VertexArrayObject* vao = vaos[0];

					Ogre::VertexBufferPacked* vertexBuffer = vao->getVertexBuffers()[0];
					if (!vertexBuffer) continue;

					size_t numVertices = vertexBuffer->getNumElements();
					size_t vertexSize = vertexBuffer->getBytesPerElement();

					// --- Create New Vertex Buffer ---
					Ogre::RenderSystem* renderSystem = Ogre::Root::getSingletonPtr()->getRenderSystem();
					Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

					// Define vertex elements (position, normal, texture coordinates)
					Ogre::VertexElement2Vec vertexElements;
					vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
					vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
					vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_HALF2, Ogre::VES_TEXTURE_COORDINATES));

					size_t vSize = numVertices * 8; // 3 for position + 3 for normal + 2 for texture coords

					Ogre::VertexBufferPacked* newVertexBuffer = vaoManager->createVertexBuffer(
						vertexElements,
						vSize,
						Ogre::BT_DYNAMIC_DEFAULT,
						nullptr,
						false
					);

					// Copy vertex data into combined buffer
					std::vector<float> combinedData(vSize);

					for (size_t i = 0; i < numVertices; ++i)
					{
						combinedData[i * 8 + 0] = newVertices[i].x;
						combinedData[i * 8 + 1] = newVertices[i].y;
						combinedData[i * 8 + 2] = newVertices[i].z;

						combinedData[i * 8 + 3] = this->originalNormals[i].x;
						combinedData[i * 8 + 4] = this->originalNormals[i].y;
						combinedData[i * 8 + 5] = this->originalNormals[i].z;

						combinedData[i * 8 + 6] = this->originalTextureCoordinates[i].x;
						combinedData[i * 8 + 7] = this->originalTextureCoordinates[i].y;
					}

					newVertexBuffer->upload(combinedData.data(), 0, vSize);

					// --- Create New Index Buffer ---
					Ogre::IndexBufferPacked* oldIndexBuffer = vao->getIndexBuffer();
					Ogre::IndexBufferPacked* newIndexBuffer = nullptr;

					if (oldIndexBuffer)
					{
						newIndexBuffer = vaoManager->createIndexBuffer(
							oldIndexBuffer->getIndexType(),
							oldIndexBuffer->getNumElements(),
							Ogre::BT_DYNAMIC_DEFAULT,
							nullptr,
							false
						);

						newIndexBuffer->upload(newIndices.data(), 0, indexCount);
					}

					// --- Create a New VAO ---
					Ogre::VertexBufferPackedVec newVertexBuffers = { newVertexBuffer };

					Ogre::VertexArrayObject* newVao = vaoManager->createVertexArrayObject(
						newVertexBuffers,
						newIndexBuffer,
						vao->getOperationType()
					);

					// --- Replace the old VAO with the new one ---
					subMesh->mVao[Ogre::VpNormal].clear();
					subMesh->mVao[Ogre::VpNormal].push_back(newVao);

					mesh->_dirtyState();
				}
			}
		}
	}

#if 1
	void MeshModifyComponent::updateMeshWithModifiedVertices(Ogre::MovableObject* movableObject, const std::vector<Ogre::Vector3>& newVertices, size_t vertexCount, const std::vector<unsigned long>& newIndices, size_t indexCount)
	{
		if (nullptr == movableObject)
		{
			return;
		}

		Ogre::String type = movableObject->getMovableType();
		if (type.compare("Entity") == 0)
		{
			Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(movableObject);
			Ogre::v1::MeshPtr mesh = entity->getMesh();

			// Process all submeshes
			size_t submeshCount = mesh->getNumSubMeshes();
			size_t currentVertexOffset = 0;
			size_t currentIndexOffset = 0;

			for (size_t i = 0; i < submeshCount; ++i)
			{
				Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);

				// Update vertex data
				Ogre::v1::VertexData* vertexData = subMesh->useSharedVertices ? mesh->sharedVertexData[0] : subMesh->vertexData[0];
				if (vertexData)
				{
					Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(0);
					size_t numVerticesInSubmesh = vertexData->vertexCount;

					if (currentVertexOffset + numVerticesInSubmesh <= vertexCount)
					{
						void* vData = vbuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);
						memcpy(vData, &newVertices[currentVertexOffset], numVerticesInSubmesh * sizeof(Ogre::Vector3));
						vbuf->unlock();

						currentVertexOffset += numVerticesInSubmesh;
					}
				}

				// Update index data if provided
				if (false == newIndices.empty() && indexCount > 0)
				{
					Ogre::v1::HardwareIndexBufferSharedPtr ibuf = subMesh->indexData[0]->indexBuffer;
					if (ibuf)
					{
						size_t numIndicesInSubmesh = subMesh->indexData[0]->indexCount / 3;  // Triangle count

						if (currentIndexOffset + numIndicesInSubmesh <= indexCount)
						{
							void* iData = ibuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

							// Determine how to copy data based on index buffer type
							if (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_16BIT)
							{
								uint16_t* indices = static_cast<uint16_t*>(iData);
								for (size_t j = 0; j < numIndicesInSubmesh; ++j)
								{
									// Convert from Vector3 to uint16_t indices (each Vector3 represents one triangle)
									indices[j * 3 + 0] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 0]);
									indices[j * 3 + 1] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 1]);
									indices[j * 3 + 2] = static_cast<uint16_t>(newIndices[currentIndexOffset + j * 3 + 2]);
								}
							}
							else // 32-bit indices
							{
								uint32_t* indices = static_cast<uint32_t*>(iData);
								for (size_t j = 0; j < numIndicesInSubmesh; ++j)
								{
									// Convert from Vector3 to uint32_t indices
									indices[j * 3 + 0] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 0]);
									indices[j * 3 + 1] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 1]);
									indices[j * 3 + 2] = static_cast<uint32_t>(newIndices[currentIndexOffset + j * 3 + 2]);
								}
							}

							ibuf->unlock();
							currentIndexOffset += numIndicesInSubmesh;
						}
					}
				}
			}

			// Mark the mesh as having changed
			mesh->_dirtyState();
		}
		else if (type.compare("Item") == 0)
		{
			Ogre::Item* item = static_cast<Ogre::Item*>(movableObject);
			Ogre::MeshPtr mesh = item->getMesh();

			size_t currentVertexOffset = 0;
			size_t currentIndexOffset = 0;

			for (Ogre::SubMesh* subMesh : mesh->getSubMeshes())
			{
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[Ogre::VpNormal];

				if (!vaos.empty())
				{
					Ogre::VertexArrayObject* vao = vaos[0];

					Ogre::VertexBufferPacked* vertexBuffer = vao->getVertexBuffers()[0];
					if (!vertexBuffer) continue;

					size_t numVertices = vertexBuffer->getNumElements();
					size_t vertexSize = vertexBuffer->getBytesPerElement();

					// --- Create New Vertex Buffer ---
					Ogre::RenderSystem* renderSystem = Ogre::Root::getSingletonPtr()->getRenderSystem();
					Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

					// Define vertex elements (position, normal, texture coordinates)
					Ogre::VertexElement2Vec vertexElements;
					vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
					vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
					vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_HALF2, Ogre::VES_TEXTURE_COORDINATES));

					size_t vSize = numVertices * 8;

					Ogre::VertexBufferPacked* newVertexBuffer = vaoManager->createVertexBuffer(
						vertexElements,
						vSize,
						Ogre::BT_DYNAMIC_DEFAULT,
						nullptr,
						false
					);

					// Copy vertex data into combined buffer
					std::vector<float> combinedData(numVertices);

					for (size_t i = 0; i < numVertices; ++i)
					{
						combinedData[i * 8 + 0] = newVertices[i].x;
						combinedData[i * 8 + 1] = newVertices[i].y;
						combinedData[i * 8 + 2] = newVertices[i].z;

						combinedData[i * 8 + 3] = this->originalNormals[i].x;
						combinedData[i * 8 + 4] = this->originalNormals[i].y;
						combinedData[i * 8 + 5] = this->originalNormals[i].z;

						combinedData[i * 8 + 6] = this->originalTextureCoordinates[i].x;
						combinedData[i * 8 + 7] = this->originalTextureCoordinates[i].y;
					}

					newVertexBuffer->upload(combinedData.data(), 0, numVertices);

					// --- Create New Index Buffer ---
					Ogre::IndexBufferPacked* oldIndexBuffer = vao->getIndexBuffer();
					Ogre::IndexBufferPacked* newIndexBuffer = nullptr;

					if (oldIndexBuffer)
					{
						newIndexBuffer = vaoManager->createIndexBuffer(
							oldIndexBuffer->getIndexType(),
							oldIndexBuffer->getNumElements(),
							Ogre::BT_DYNAMIC_DEFAULT,
							nullptr,
							false
						);

						newIndexBuffer->upload(newIndices.data(), 0, indexCount);
					}

					// --- Create a New VAO ---
					Ogre::VertexBufferPackedVec newVertexBuffers = { newVertexBuffer };

					Ogre::VertexArrayObject* newVao = vaoManager->createVertexArrayObject(
						newVertexBuffers,
						newIndexBuffer,
						vao->getOperationType()
					);

					// --- Replace the old VAO with the new one ---
					subMesh->mVao[Ogre::VpNormal].clear();
					subMesh->mVao[Ogre::VpNormal].push_back(newVao);

					mesh->_dirtyState();
				}
			}
		}
	}
#endif

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