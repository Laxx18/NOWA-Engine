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
		dynamicVertexBuffer(nullptr),
		dynamicIndexBuffer(nullptr),
		activated(new Variant(MeshModifyComponent::AttrActivated(), true, this->attributes)),
		brushSize(new Variant(MeshModifyComponent::AttrBrushSize(), static_cast<int>(64), this->attributes)),
		brushIntensity(new Variant(MeshModifyComponent::AttrBrushIntensity(), static_cast<int>(255), this->attributes)),
		brushName(new Variant(MeshModifyComponent::AttrBrushName(), this->attributes))
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

		Ogre:Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();

		bool isVET_HALF4;
		bool isIndices32;

		// get the mesh information
#if 1
		MathHelper::getInstance()->getDetailedMeshInformation2(originalItem->getMesh(), this->originalVertexCount, this->originalVertices, this->originalNormals, this->originalTextureCoordinates,
			this->originalIndexCount, this->originalIndices, originalItem->getParentNode()->_getDerivedPositionUpdated(),
			originalItem->getParentNode()->_getDerivedOrientationUpdated(),
			originalItem->getParentNode()->getScale(), isVET_HALF4, isIndices32);
#else
		MathHelper::getInstance()->getDetailedMeshInformation2(originalItem->getMesh(), this->originalVertexCount, this->originalVertices, this->originalNormals, this->originalTextureCoordinates,
			this->originalIndexCount, this->originalIndices, Ogre::Vector3::ZERO,
			Ogre::Quaternion::IDENTITY, Ogre::Vector3::UNIT_SCALE, isVET_HALF4, isIndices32);
#endif

		this->createDynamicMesh(isVET_HALF4, isIndices32);

		this->meshToModify->load();

		// Just one subitem is allowed for now, getNumSubItems will be just 1
		Ogre::Item* itemToModify = this->gameObjectPtr->getSceneManager()->createItem(this->meshToModify, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
		for (size_t i = 0; i < originalItem->getNumSubItems(); i++)
		{
			auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(originalItem->getSubItem(i)->getDatablock());
			if (nullptr != sourceDataBlock)
			{
				itemToModify->getSubItem(i)->setDatablock(sourceDataBlock);

				// sourceDataBlock->mShadowConstantBias = 0.0001f;
				// Deactivate fresnel by default, because it looks ugly
				if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
				{
					sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
				}
			}
		}

		this->gameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
		this->gameObjectPtr->getSceneNode()->attachObject(itemToModify);
		this->gameObjectPtr->init(itemToModify);

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

		// Permanently unmap persistent mapped buffers
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

	bool MeshModifyComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (true == this->brushData.empty())
		{
			return false;
		}

		if (id == OIS::MB_Left && nullptr != this->meshToModify && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == this->activated->getBool())
		{
			this->isPressing = true;

			Ogre::Real x = 0.0f;
			Ogre::Real y = 0.0f;
			MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());

			Ogre::Ray ray = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera()->getCameraToViewportRay(x, y);
			this->brushPosition = ray.getPoint(100.0f);

			this->modifyMeshWithBrush(this->brushPosition);
		}

		return false;
	}

	bool MeshModifyComponent::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (true == this->brushData.empty())
		{
			return false;
		}

		if (true == this->isPressing && nullptr != this->meshToModify && nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget() && true == this->activated->getBool())
		{
			Ogre::Real x = 0.0f;
			Ogre::Real y = 0.0f;
			MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());

			Ogre::Ray ray = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera()->getCameraToViewportRay(x, y);
			this->brushPosition = ray.getPoint(100.0f);

			this->modifyMeshWithBrush(this->brushPosition);
		}

		return false;
	}

	bool MeshModifyComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (true == this->brushData.empty())
		{
			return false;
		}

		this->isPressing = false;

		return false;
	}

#if 0
	void MeshModifyComponent::createDynamicMesh(bool isVET_HALF4, bool isIndices32)
	{
		Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();

		// Transform back from global to local, both: transform and normals
		// Get the world transformation matrix of the entity
		Ogre::Matrix4 worldMatrix = originalItem->getParentNode()->_getFullTransform();

		// Inverse the world matrix to get object space transformation
		Ogre::Matrix4 invWorldMatrix = worldMatrix.inverse();

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

		Ogre::RenderSystem* renderSystem = Ogre::Root::getSingletonPtr()->getRenderSystem();
		Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

		// Create manual mesh
		this->meshToModify = Ogre::MeshManager::getSingletonPtr()->createManual(this->gameObjectPtr->getName() + "_dynamic", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Ogre::SubMesh* subMesh = this->meshToModify->createSubMesh();

		// Define vertex elements - use correct formats
		Ogre::VertexElement2Vec vertexElements;

		if (false == isVET_HALF4)
		{
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
		}
		else
		{
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_POSITION));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_NORMAL));
		}

		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

		// Create interleaved vertex data
		size_t vertexSize = 0;
		if (isVET_HALF4)
		{
			vertexSize = sizeof(float) * 4 + sizeof(float) * 4 + sizeof(float) * 2; // pos(4) + normal(4) + texcoord(2)
		}
		else
		{
			vertexSize = sizeof(float) * 3 + sizeof(float) * 3 + sizeof(float) * 2; // pos(3) + normal(3) + texcoord(2)
		}

		// Allocate memory for interleaved data - using byte count, not element count
		uint8_t* interleavedData = (uint8_t*)OGRE_MALLOC_SIMD(vertexSize * this->originalVertexCount, Ogre::MEMCATEGORY_GEOMETRY);
		float* pData = (float*)interleavedData;

		// Fill the interleaved data
		for (size_t i = 0; i < this->originalVertexCount; ++i) {
			// Position
			*pData++ = this->originalVertices[i].x;
			*pData++ = this->originalVertices[i].y;
			*pData++ = this->originalVertices[i].z;
			if (isVET_HALF4)
			{
				*pData++ = 1.0f; // w component for VET_FLOAT4
			}

			// Normal
			*pData++ = this->originalNormals[i].x;
			*pData++ = this->originalNormals[i].y;
			*pData++ = this->originalNormals[i].z;
			if (isVET_HALF4)
			{
				*pData++ = 0.0f; // w component for VET_FLOAT4
			}

			// Texture coordinates - check if this variable actually exists!
			if (this->originalTextureCoordinates && i < this->originalTextureCoordinatesCount) {
				*pData++ = this->originalTextureCoordinates[i].x;
				*pData++ = this->originalTextureCoordinates[i].y;
			}
			else {
				// Fallback if texture coordinates aren't available
				*pData++ = 0.0f;
				*pData++ = 0.0f;
			}
		}

		try
		{
			// Create vertex buffer with interleaved data
			this->dynamicVertexBuffer = vaoManager->createVertexBuffer(vertexElements, this->originalVertexCount, Ogre::BT_DYNAMIC_PERSISTENT, interleavedData, false);
		}
		catch (Ogre::Exception& e)
		{
			OGRE_FREE_SIMD(interleavedData, Ogre::MEMCATEGORY_GEOMETRY);
			return;
		}

		Ogre::IndexBufferPacked::IndexType indexType = Ogre::IndexBufferPacked::IT_32BIT;

		if (false == isIndices32)
		{
			indexType = Ogre::IndexBufferPacked::IT_16BIT;
		}

		// Create index buffer with original indices
		try
		{
			this->dynamicIndexBuffer = vaoManager->createIndexBuffer(indexType, this->originalIndexCount, Ogre::BT_DYNAMIC_PERSISTENT, this->originalIndices, false);
		}
		catch (Ogre::Exception& e)
		{
			OGRE_FREE_SIMD(this->originalIndices, Ogre::MEMCATEGORY_GEOMETRY);
			return;
		}

		// Create VAO
		Ogre::VertexBufferPackedVec vertexBuffers;
		vertexBuffers.push_back(this->dynamicVertexBuffer);
		Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, this->dynamicIndexBuffer, Ogre::OT_TRIANGLE_LIST);

		subMesh->mVao[Ogre::VpNormal].push_back(vao);
		subMesh->mVao[Ogre::VpShadow].push_back(vao);

		// Set bounds (same as originalItem)
		Ogre::Aabb originalBounds = originalItem->getMesh()->getAabb();
		this->meshToModify->_setBounds(originalBounds, false);
		this->meshToModify->_setBoundingSphereRadius(originalItem->getMesh()->getBoundingSphereRadius());
	}

#else

	void MeshModifyComponent::createDynamicMesh(bool isVET_HALF4, bool isIndices32)
	{
		Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();

		// Transform back from global to local, both: transform and normals
		// Get the world transformation matrix of the entity
		Ogre::Matrix4 worldMatrix = originalItem->getParentNode()->_getFullTransform();

		// Inverse the world matrix to get object space transformation
		Ogre::Matrix4 invWorldMatrix = worldMatrix.inverse();

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

		Ogre::RenderSystem* renderSystem = Ogre::Root::getSingletonPtr()->getRenderSystem();
		Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

		// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
		Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName(this->gameObjectPtr->getName() + "_dynamic");
		if (nullptr != resourceV2)
		{
			Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(this->gameObjectPtr->getName() + "_dynamic");
			Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
		}

		// Create manual mesh
		this->meshToModify = Ogre::MeshManager::getSingletonPtr()->createManual(this->gameObjectPtr->getName() + "_dynamic", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Ogre::SubMesh* subMesh = this->meshToModify->createSubMesh();

		// Define vertex elements - use correct formats
		Ogre::VertexElement2Vec vertexElements;

		if (false == isVET_HALF4)
		{
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
		}
		else
		{
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_POSITION));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_NORMAL));
		}

		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

		// Create interleaved vertex data
		size_t vertexSize = 0;
		if (isVET_HALF4)
		{
			vertexSize = /*sizeof(float) **/ 4 + /*sizeof(float) **/ 4 + /*sizeof(float) **/ 2; // pos(4) + normal(4) + texcoord(2)
		}
		else
		{
			vertexSize = /*sizeof(float) **/ 3 +/* sizeof(float) **/ 3 + /*sizeof(float) **/ 2; // pos(3) + normal(3) + texcoord(2)
		}

		// Allocate memory for interleaved data - using byte count, not element count
		uint8_t* interleavedData = (uint8_t*)OGRE_MALLOC_SIMD(vertexSize * this->originalVertexCount, Ogre::MEMCATEGORY_GEOMETRY);
		float* pData = (float*)interleavedData;

		// Fill the interleaved data
		for (size_t i = 0; i < this->originalVertexCount; ++i)
		{
			// Position
			*pData++ = this->originalVertices[i].x;
			*pData++ = this->originalVertices[i].y;
			*pData++ = this->originalVertices[i].z;
			if (isVET_HALF4)
			{
				*pData++ = 1.0f; // w component for VET_FLOAT4
			}

			// Normal
			*pData++ = this->originalNormals[i].x;
			*pData++ = this->originalNormals[i].y;
			*pData++ = this->originalNormals[i].z;
			if (isVET_HALF4)
			{
				*pData++ = 0.0f; // w component for VET_FLOAT4
			}

			// Texture coordinates - no need for a separate count variable
			if (this->originalTextureCoordinates)
			{
				*pData++ = this->originalTextureCoordinates[i].x;
				*pData++ = this->originalTextureCoordinates[i].y;
			}
			else
			{
				// Fallback if texture coordinates aren't available
				*pData++ = 0.0f;
				*pData++ = 0.0f;
			}
		}

		try
		{
			// Create vertex buffer with interleaved data
			this->dynamicVertexBuffer = vaoManager->createVertexBuffer(vertexElements, this->originalVertexCount, Ogre::BT_DYNAMIC_PERSISTENT, interleavedData, false);
		}
		catch (Ogre::Exception& e)
		{
			OGRE_FREE_SIMD(interleavedData, Ogre::MEMCATEGORY_GEOMETRY);
			return;
		}

		Ogre::IndexBufferPacked::IndexType indexType = Ogre::IndexBufferPacked::IT_32BIT;

		if (false == isIndices32)
		{
			indexType = Ogre::IndexBufferPacked::IT_16BIT;
		}

		// Create index buffer with original indices
		try
		{
			this->dynamicIndexBuffer = vaoManager->createIndexBuffer(indexType, this->originalIndexCount, Ogre::BT_DYNAMIC_PERSISTENT, this->originalIndices, false);
		}
		catch (Ogre::Exception& e)
		{
			OGRE_FREE_SIMD(this->originalIndices, Ogre::MEMCATEGORY_GEOMETRY);
			return;
		}

		// Create VAO
		Ogre::VertexBufferPackedVec vertexBuffers;
		vertexBuffers.push_back(this->dynamicVertexBuffer);
		Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, this->dynamicIndexBuffer, Ogre::OT_TRIANGLE_LIST);

		subMesh->mVao[Ogre::VpNormal].push_back(vao);
		subMesh->mVao[Ogre::VpShadow].push_back(vao);

		std::vector<Ogre::Vector3> newVertices(this->originalVertices, this->originalVertices + this->originalVertexCount);
		std::vector<unsigned long> newIndices(this->originalIndices, this->originalIndices + this->originalIndexCount);
		std::vector<Ogre::Vector3> newNormals(this->originalNormals, this->originalNormals + this->originalVertexCount);

		// Set bounds (same as originalItem)
		Ogre::Aabb originalBounds = originalItem->getMesh()->getAabb();
		this->meshToModify->_setBounds(originalBounds, false);
		this->meshToModify->_setBoundingSphereRadius(originalItem->getMesh()->getBoundingSphereRadius());
	}

#endif

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
		std::vector<Ogre::Vector3> newVertices(this->originalVertices, this->originalVertices + this->originalVertexCount);
		std::vector<unsigned long> newIndices(this->originalIndices, this->originalIndices + this->originalIndexCount);

		// TODO: Normals must also be adapated! I have originalNormals now! Or at least recalculated out of modifed vertices!

		// Map the buffer for writing
		float* RESTRICT_ALIAS vertexData = reinterpret_cast<float* RESTRICT_ALIAS>(this->dynamicVertexBuffer->map(0, this->originalVertexCount));

		// Modify vertices using the brush
		for (size_t i = 0; i < newVertices.size(); ++i)
		{
			Ogre::Vector3 vertex = newVertices[i];

			// Compute brush influence
			Ogre::Real brushIntensity = this->getBrushValue(i);
			Ogre::Real distance = vertex.distance(brushPosition);
			if (distance <= this->brushSize->getInt())
			{
				Ogre::Real intensity = (1.0f - (distance / this->brushSize->getInt())) * brushIntensity;
				vertex += (vertex - brushPosition).normalisedCopy() * intensity * 0.1f;
			}

			// Update the buffer
			vertexData[0] = vertex.x;
			vertexData[1] = vertex.y;
			vertexData[2] = vertex.z;

			vertexData += 3;  // Move to the next vertex in the buffer
		}

		// Unmap the buffer to apply the changes
		this->dynamicVertexBuffer->unmap(Ogre::UO_KEEP_PERSISTENT);

		// Update normals (if necessary)
		// updateNormals(this->item);
	}

#if 0
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