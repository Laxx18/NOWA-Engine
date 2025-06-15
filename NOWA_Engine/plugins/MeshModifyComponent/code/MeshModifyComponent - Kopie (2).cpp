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
		if (id == OIS::MB_Left && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == this->activated->getBool())
		{
			this->isPressing = true;

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
					// Store original mesh for finalization later
					this->originalVertices = vertices;
					this->originalVertexCount = vertexCount;
					this->originalIndices = indices;
					this->originalIndexCount = indexCount;
					this->brushPosition = intersectionPoint;
					this->modifyMeshWithBrush(this->brushPosition);
				}
			}
		}

		return false;
	}

	bool MeshModifyComponent::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (true == this->isPressing && nullptr != this->currentEditingObject && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == this->activated->getBool())
		{
			Ogre::Real x = 0.0f;
			Ogre::Real y = 0.0f;
			MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());

			Ogre::Ray ray = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera()->getCameraToViewportRay(x, y);

			Ogre::Vector3 intersectionPoint;
			Ogre::Vector3* vertices = nullptr;
			unsigned long* indices = nullptr;
			size_t vertexCount = 0;
			size_t indexCount = 0;
			if (true == MathHelper::getInstance()->getRaycastDetailsResult(ray, this->raySceneQuery, intersectionPoint, (size_t&)this->currentEditingObject,
				vertices, vertexCount, indices, indexCount))
			{
				if (this->currentEditingObject)
				{
					// Store original mesh for finalization later
					this->originalVertices = vertices;
					this->originalVertexCount = vertexCount;
					this->originalIndices = indices;
					this->originalIndexCount = indexCount;
					this->brushPosition = intersectionPoint;
					this->modifyMeshWithBrush(this->brushPosition);
				}
			}
		}

		return false;
	}

	bool MeshModifyComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		this->isPressing = false;

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

	void MeshModifyComponent::updateVertexBuffer(void)
	{
		// Assuming the original vertices are stored as a flat array of floats (x, y, z per vertex)
		// We need to update the vertex buffer with the modified vertex data (modified by the brush or any other operation)

		Ogre::VertexBufferPacked* vertexBuffer = nullptr;

		// Get the VaoManager from the render system
		Ogre::Root* root = Ogre::Root::getSingletonPtr();
		Ogre::RenderSystem* renderSystem = root->getRenderSystem();
		Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

		// Create an updated vertices vector to hold the modified vertex data
		std::vector<Ogre::Vector3> updatedVertices(originalVertexCount);

		// Fill updatedVertices with the modified vertex data (originalVertices could be updated by brush operation)
		for (size_t i = 0; i < originalVertexCount; ++i)
		{
			updatedVertices[i] = Ogre::Vector3(originalVertices[i * 3], originalVertices[i * 3 + 1], originalVertices[i * 3 + 2]);
		}

		// Now you need to update the vertex buffer with the new modified vertices
		try
		{
			// Update the dynamic vertex buffer with the modified data
			vertexBuffer = vaoManager->createVertexBuffer(
				{ Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION) },   // Vertex elements (position only here)
				originalVertexCount,                                              // Number of vertices
				Ogre::BT_DYNAMIC_PERSISTENT,                                      // Buffer type: dynamic persistent
				updatedVertices.data(),                                           // Pointer to the updated vertex data
				true                                                              // Flag to allow memory mapping
			);
		}
		catch (Ogre::Exception& e)
		{
			// Handle any errors that may occur during vertex buffer update
			OGRE_FREE_SIMD(vertexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
			vertexBuffer = nullptr;
			throw e;
		}

		// Optionally, update any other structures that rely on this updated vertex buffer
		// For example, you might need to notify the mesh or the renderer that the buffer has been updated
	}

#if 1
	void MeshModifyComponent::modifyMeshWithBrush(const Ogre::Vector3& brushPosition)
	{
		// Assuming vertices are stored in a linear array: [x, y, z, x, y, z, ...]
		for (size_t i = 0; i < originalVertexCount; ++i)
		{
			// Retrieve the current vertex from the original vertex array
			Ogre::Vector3 vertex(originalVertices[i * 3], originalVertices[i * 3 + 1], originalVertices[i * 3 + 2]);

			// Get brush intensity at this vertex
			Ogre::Real brushIntensity = this->getBrushValue(i);

			// Calculate the distance between the vertex and the brush position
			Ogre::Real distance = vertex.distance(brushPosition);

			// Apply deformation if within the brush size radius
			if (distance <= this->brushSize->getInt())
			{
				// Calculate the deformation intensity based on distance (closer vertices are affected more)
				Ogre::Real intensity = (1.0f - (distance / this->brushSize->getInt())) * brushIntensity;

				// Apply deformation (adjust the 0.1f value for deformation magnitude)
				vertex += (vertex - brushPosition).normalisedCopy() * intensity * 0.1f;

				// Update the vertex position in the array
				originalVertices[i * 3] = vertex.x;
				originalVertices[i * 3 + 1] = vertex.y;
				originalVertices[i * 3 + 2] = vertex.z;
			}
		}

		// After modifying vertices, update the vertex buffer
		updateVertexBuffer();
	}
#endif

#if 0
	void MeshModifyComponent::modifyMeshWithBrush(const Ogre::Vector3& brushPosition)
	{
		Ogre::String type = this->currentEditingObject->getMovableType();
		if (type.compare("Item") == 0)
		{
			Ogre::Item* item = static_cast<Ogre::Item*>(this->currentEditingObject);
			Ogre::MeshPtr mesh = item->getMesh();

			// Create a submesh, as per your example, for dynamic modification
			Ogre::SubMesh* subMesh = mesh->createSubMesh();

			Ogre::VaoManager* vaoManager = Ogre::Root::getSingleton().getRenderSystem()->getVaoManager();

			// Define vertex data for positions and normals
			Ogre::VertexElement2Vec vertexElements;
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));

			// Create a buffer for vertices, for now, let's assume we get vertices data from your existing mesh
			std::vector<Ogre::Vector3> vertices;  // This should be populated with actual vertex positions
			// Example: Populate vertices based on mesh data here
			// vertices.push_back(...);

			// Create dynamic vertex buffer with persistent storage
			Ogre::VertexBufferPacked* vertexBuffer = vaoManager->createVertexBuffer(vertexElements, vertices.size(), Ogre::BT_DYNAMIC_PERSISTENT, &vertices[0], false);

			// Create index buffer (just for illustration; you'll need real index data)
			Ogre::IndexBufferPacked* indexBuffer = createIndexBuffer(); // Implement this as per your index data

			// Create the VAO (Vertex Array Object)
			Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject({ vertexBuffer }, indexBuffer, Ogre::OT_TRIANGLE_LIST);

			// Attach VAO to the submesh
			subMesh->mVao[Ogre::VpNormal].push_back(vao);
			subMesh->mVao[Ogre::VpShadow].push_back(vao);

			// Set the mesh bounds (for frustum culling and LOD)
			mesh->_setBounds(Ogre::Aabb(Ogre::Vector3::ZERO, Ogre::Vector3::UNIT_SCALE), false);
			mesh->_setBoundingSphereRadius(1.732f);

			// Now apply the brush effect to the vertices
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				Ogre::Vector3& vertex = vertices[i];

				// Get brush intensity for this vertex (modify as needed)
				Ogre::Real brushIntensity = this->getBrushValue(i);

				// Apply brush deformation based on intensity and vertex position
				Ogre::Real distance = vertex.distance(brushPosition);
				if (distance <= this->brushSize->getInt())
				{
					// Apply deformation proportional to the brush intensity
					Ogre::Real intensity = (1.0f - (distance / this->brushSize->getInt())) * brushIntensity;
					vertex += (vertex - brushPosition).normalisedCopy() * intensity * 0.1f;  // Modify 0.1f to adjust deformation magnitude
				}
			}

			// Write the updated vertex data back to the buffer
			vertexBuffer->writeData(0, vertices.size(), &vertices[0]);

			// After modifying the vertices, update the normals
			updateNormals(item);
		}
	}
#endif

#if 0
	void MeshModifyComponent::modifyMeshWithBrush(const Ogre::Vector3& brushPosition)
	{
		Ogre::String type = this->currentEditingObject->getMovableType();
		if (type.compare("Item") == 0)
		{
			Ogre::Item* item = static_cast<Ogre::Item*>(this->currentEditingObject);
			Ogre::MeshPtr mesh = item->getMesh();
			Ogre::SubMesh* subMesh = mesh->getSubMesh(0);  // Assuming we modify the first submesh

		

			// Access the vertex buffer
			Ogre::VertexBufferPacked* vertexBuffer = subMesh->getVertexBuffer(0);

			// Lock the buffer for modification
			float* vertices = reinterpret_cast<float*>(vertexBuffer->map(0, ));

			// Modify the vertices using the brush intensity
			for (size_t i = 0; i < mesh->getNumVertices(); ++i)
			{
				// Here we calculate the vertex position using the brush intensity
				Ogre::Vector3 vertexPos(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);

				// Apply brush influence to the Y-coordinate (e.g., create hills)
				float brushIntensity = getBrushIntensity(i, mesh);
				vertexPos.y += brushIntensity;

				// Update the vertex position in the buffer
				vertices[i * 3] = vertexPos.x;
				vertices[i * 3 + 1] = vertexPos.y;
				vertices[i * 3 + 2] = vertexPos.z;
			}

			// Unlock the buffer to apply changes
			vertexBuffer->unmap(Ogre::UO_KEEP_PERSISTENT);

			// After modifying the vertices, update the normals
			updateNormals(item);
		}
	}

#endif


	void NOWA::MeshModifyComponent::updateNormals(Ogre::Item* item)
	{
#if 0
		if (!item)
		{
			return;
		}

		Ogre::MeshPtr mesh = item->getMesh();

		for (size_t subMeshIdx = 0; subMeshIdx < mesh->getNumSubMeshes(); ++subMeshIdx)
		{
			Ogre::SubMesh* subMesh = mesh->getSubMesh(subMeshIdx);

			if (!subMesh->useSharedVertices || !subMesh->getVertexData())
			{
				continue;
			}

			Ogre::VertexBufferPacked* vertexBuffer = subMesh->getVertexData()->vertexBufferBinding->getBuffer(0);
			size_t numVertices = vertexBuffer->getNumElements();

			std::vector<Ogre::Vector3> newNormals(numVertices, Ogre::Vector3::ZERO);
			Ogre::IndexBufferPacked* indexBuffer = subMesh->getIndexData()->indexBuffer;
			const size_t numIndices = indexBuffer->getNumElements();

			for (size_t i = 0; i < numIndices; i += 3)
			{
				size_t idx0 = indexBuffer->getElement(i);
				size_t idx1 = indexBuffer->getElement(i + 1);
				size_t idx2 = indexBuffer->getElement(i + 2);

				Ogre::Vector3 v0, v1, v2;
				vertexBuffer->readData(idx0, 1, &v0);
				vertexBuffer->readData(idx1, 1, &v1);
				vertexBuffer->readData(idx2, 1, &v2);

				Ogre::Vector3 edge1 = v1 - v0;
				Ogre::Vector3 edge2 = v2 - v0;
				Ogre::Vector3 normal = edge1.crossProduct(edge2);
				normal.normalise();

				newNormals[idx0] += normal;
				newNormals[idx1] += normal;
				newNormals[idx2] += normal;
			}

			for (size_t i = 0; i < numVertices; ++i)
			{
				newNormals[i].normalise();
				vertexBuffer->writeData(i, 1, &newNormals[i]);
			}
		}
#endif
	}

	void MeshModifyComponent::updateMesh(Ogre::MovableObject* movableObject, Ogre::Vector3* newRawVertices, size_t vertexCount, unsigned long* newRawIndices, size_t indexCount)
	{
		if (nullptr == movableObject)
		{
			return;
		}

		std::vector<Ogre::Vector3> newVertices(newRawVertices, newRawVertices + vertexCount);
		std::vector<unsigned long> newIndices(newRawIndices, newRawIndices + indexCount);

		// Get the world transformation matrix of the entity
		Ogre::Matrix4 worldMatrix = movableObject->getParentNode()->_getFullTransform();

		// Inverse the world matrix to get object space transformation
		Ogre::Matrix4 invWorldMatrix = worldMatrix.inverse();

		// Transform vertices from world space to object space
		for (size_t i = 0; i < vertexCount; ++i)
		{
			// Transform each vertex by applying the inverse of the world matrix
			newVertices[i] = invWorldMatrix * newVertices[i];
		}

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

					Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
					size_t numIndices = indexBuffer ? indexBuffer->getNumElements() : 0;

					// --- Update Vertex Buffer ---
					Ogre::BufferType bufferType = vertexBuffer->getBufferType();

					if (bufferType == Ogre::BT_DYNAMIC_DEFAULT ||
						bufferType == Ogre::BT_DYNAMIC_PERSISTENT ||
						bufferType == Ogre::BT_DYNAMIC_PERSISTENT_COHERENT)
					{
						// Dynamic buffers, so map and update in place
						float* vertexData = static_cast<float*>(vertexBuffer->map(0, numVertices));

						for (size_t i = 0; i < numVertices; ++i)
						{
							// Update positions (x, y, z)
							vertexData[i * 6 + 0] = newVertices[currentVertexOffset + i].x;
							vertexData[i * 6 + 1] = newVertices[currentVertexOffset + i].y;
							vertexData[i * 6 + 2] = newVertices[currentVertexOffset + i].z;

							// ATTENTION: Calculate normalized normal once
							Ogre::Vector3 normal = newVertices[currentVertexOffset + i].normalisedCopy();

							// Update normals (nx, ny, nz)
							vertexData[i * 6 + 3] = normal.x;
							vertexData[i * 6 + 4] = normal.y;
							vertexData[i * 6 + 5] = normal.z;
						}

						vertexBuffer->unmap(Ogre::UO_KEEP_PERSISTENT);
					}
					else
					{

						Ogre::RenderSystem* renderSystem = Ogre::Root::getSingletonPtr()->getRenderSystem();
						Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();
						// If the buffer is not dynamic, create a new dynamic buffer and upload the data
						// Create a new vertex buffer for this submesh (mutable)
						// Create the vertex elements for the new buffer (for positions and normals)
						Ogre::VertexElement2Vec vertexElements;
						vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
						vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));

						// Create the dynamic vertex buffer with the correct elements
						Ogre::VertexBufferPacked* newVertexBuffer = vaoManager->createVertexBuffer(
							vertexElements,                // Vertex elements (position and normal)
							numVertices,                   // Number of vertices
							Ogre::BT_DYNAMIC_DEFAULT,      // Buffer type, dynamic
							nullptr,                       // No initial data
							false                          // Don't keep as shadow
						);

						std::vector<Ogre::Vector3> combinedData(newVertices.size());
						for (size_t i = 0; i < newVertices.size(); ++i)
						{
							combinedData[i] = Ogre::Vector3(newVertices[i].x, newVertices[i].y, newVertices[i].z);
							// Combine vertices and normals into one structure for uploading
						}

						// Upload the combined data to the newly created dynamic buffer
						newVertexBuffer->upload(combinedData.data(), 0, newVertices.size() * sizeof(Ogre::Vector3));


						// Manually replace the old buffer with the new one
						// Get the current vertex buffers of the VAO and replace the first element
						const std::vector<Ogre::VertexBufferPacked*>& vertexBuffers = vao->getVertexBuffers();
						if (!vertexBuffers.empty())
						{
							vertexBuffers[0] = newVertexBuffer;  // Replacing the first vertex buffer
						}
					}

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

					mesh->_dirtyState();

					currentVertexOffset += numVertices;
					currentIndexOffset += numIndices;
				}
			}
		}
	}

	bool MeshModifyComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (gameObject->getComponentCount<MeshModifyComponent>() < 2 && nullptr != gameObject->getMovableObjectUnsafe<Ogre::Item>())
		{
			return true;
		}
		return false;
	}

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