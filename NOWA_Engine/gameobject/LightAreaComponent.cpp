#include "NOWAPrecompiled.h"

#include "LightAreaComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	LightAreaComponent::LightAreaComponent()
		: GameObjectComponent(),
		light(nullptr),
		datablock(nullptr),
		diffuseColor(new Variant(LightAreaComponent::AttrDiffuseColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		specularColor(new Variant(LightAreaComponent::AttrSpecularColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		//Increase the strength 10x to showcase this light. Area approx lights are not
		//physically based so the value is more arbitrary than the other light types
		powerScale(new Variant(LightAreaComponent::AttrPowerScale(), 1.0f * 10.0f, this->attributes)),
		direction(new Variant(LightAreaComponent::AttrDirection(), Ogre::Vector3(0.0f, -1.0f, 0.0f), this->attributes)),
		affectParentNode(new Variant(LightAreaComponent::AttrAffectParentNode(), false, this->attributes)),
		castShadows(new Variant(LightAreaComponent::AttrCastShadows(), true, this->attributes)),
		attenuationRadius(new Variant(LightAreaComponent::AttrAttenuationRadius(), Ogre::Vector2(10.0f, 0.1f), this->attributes)),
		rectSize(new Variant(LightAreaComponent::AttrRectSize(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
		diffuseMipStart(new Variant(LightAreaComponent::AttrDiffuseMipStart(), 0.4f, this->attributes)),
		doubleSided(new Variant(LightAreaComponent::AttrDoubleSided(), false, this->attributes)),
		areaLightType(new Variant(LightAreaComponent::AttrAreaLightType(), { Ogre::String("Area Approximation"), Ogre::String("Area Accurate") }, this->attributes))
	{
		this->diffuseColor->addUserData(GameObject::AttrActionColorDialog());
		this->specularColor->addUserData(GameObject::AttrActionColorDialog());
		this->areaLightType->addUserData(GameObject::AttrActionNeedRefresh());
		this->affectParentNode->setVisible(false); // does a lot of mess
		this->direction->setVisible(false); // does a lot of mess
		
		// Show all available textures with alpha channel in list
		//std::vector<Ogre::String> strTextureNames;
		//std::vector<Ogre::String> filters = { "png", "bmp" };
		//
		//// No texture
		//strTextureNames.emplace_back("");

		//Ogre::TextureManager::ResourceMapIterator texIt = Ogre::TextureManager::getSingletonPtr()->getResourceIterator();
		//while (texIt.hasMoreElements())
		//{
		//	strTextureNames.emplace_back(texIt.getNext()->getName());
		//}
		//-> here directly with ogre see dotsceneimportmoduleauto& resourceGroupNames = Core::getSingletonPtr()->getResourcesGroupNames();

		//for (auto& resourceGroupName : resourceGroupNames)
		//{
		//	if ("Skies" == resourceGroupName)
		//		continue;
		//	// Ogre::StringVector extensions = Ogre::Codec::getExtensions();
		//	// for (Ogre::StringVector::iterator itExt = extensions.begin(); itExt != extensions.end(); ++itExt)
		//	for (auto& filter : filters)
		//	{
		//		Ogre::StringVectorPtr names = Ogre::ResourceGroupManager::getSingletonPtr()->findResourceNames(resourceGroupName, "*." + filter/**itExt*/);
		//		for (Ogre::StringVector::iterator itName = names->begin(); itName != names->end(); ++itName)
		//		{
		//			strTextureNames.emplace_back(*itName);
		//		}
		//	}
		//}
		
		this->textureName = new Variant(LightAreaComponent::AttrTextureName(), "lightAreaPattern2.png", this->attributes);
		this->textureName->setDescription("If no texture is set, no texture mask is used and the light will emit from complete plane");
		this->textureName->addUserData(GameObject::AttrActionFileOpenDialog(), "NOWA");
	}

	LightAreaComponent::~LightAreaComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightAreaComponent] Destructor light area component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->light)
		{
			// First remove data block from entity, after that it can be destroyed
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject <Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				for (size_t i = 0; i < entity->getNumSubEntities(); i++)
				{
					entity->getSubEntity(i)->_setNullDatablock();
				}
			}
// Attention:
			this->light->setTexture(nullptr);
			this->gameObjectPtr->getSceneNode()->detachObject(this->light);
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->light);
			this->light = nullptr;
		}
		//Setup an unlit material, double-sided, with textures
        //(if it has one) and same color as the light.
        //IMPORTANT: these materials are never destroyed once they're not needed (they will
        //be destroyed by Ogre on shutdown). Watchout for this to prevent memory leaks in
        //a real implementation
// Attention: Here delete datablock etc.

		if (nullptr != this->datablock)
		{

			auto& linkedRenderabled = this->datablock->getLinkedRenderables();

			// Only destroy if the datablock is not used else where
			if (true == linkedRenderabled.empty())
			{
				this->datablock->getCreator()->destroyDatablock(this->datablock->getName());
				this->datablock = nullptr;
			}
// Attention:
			/*this->datablock->setTexture(0, nullptr);
			Ogre::Hlms* hlmsUnlit = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
			hlmsUnlit->destroyDatablock(this->datablock->getName());
			this->datablock = nullptr;*/
		}
	}

	bool LightAreaComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Diffuse")
		{
			this->diffuseColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Specular")
		{
			this->specularColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PowerScale")
		{
			this->powerScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Direction")
		{
			this->direction->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AffectParentNode")
		{
			this->affectParentNode->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CastShadows")
		{
			this->castShadows->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttenuationRadius")
		{
			this->attenuationRadius->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		/*if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RectSize")
		{
			this->rectSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}*/
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextureName")
		{
			this->textureName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseMipStart")
		{
			this->diffuseMipStart->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DoubleSided")
		{
			this->doubleSided->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AreaLightType")
		{
			this->areaLightType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr LightAreaComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LightAreaCompPtr clonedCompPtr(boost::make_shared<LightAreaComponent>());

		
		clonedCompPtr->setDiffuseColor(this->diffuseColor->getVector3());
		clonedCompPtr->setSpecularColor(this->specularColor->getVector3());
		clonedCompPtr->setPowerScale(this->powerScale->getReal());
		clonedCompPtr->setDirection(this->direction->getVector3());
		clonedCompPtr->setAffectParentNode(this->affectParentNode->getBool());
		clonedCompPtr->setCastShadows(this->castShadows->getBool());
		clonedCompPtr->setAttenuationRadius(this->attenuationRadius->getVector2());
		clonedCompPtr->setRectSize(this->rectSize->getVector2());
		clonedCompPtr->setTextureName(this->textureName->getString());
		clonedCompPtr->setDiffuseMipStart(this->diffuseMipStart->getReal());
		clonedCompPtr->setDoubleSided(this->doubleSided->getBool());
		clonedCompPtr->setAreaLightType(this->areaLightType->getListSelectedValue());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool LightAreaComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightAreaComponent] Init light directional component for game object: " + this->gameObjectPtr->getName());

		// Lights somehow cannot be set static or dynamic, so hide the property
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		// this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(false);

		this->createLight();
		return true;
	}

	bool LightAreaComponent::connect(void)
	{
		
		return true;
	}

	bool LightAreaComponent::disconnect(void)
	{
		
		return true;
	}

	void LightAreaComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void LightAreaComponent::createLight(void)
	{
		Ogre::Hlms* hlmsUnlit = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
		Ogre::Hlms* hlmsPbs = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		
		///////////////////////////////////////Create Area Light//////////////////////////////////////////////////////////////
		if (nullptr == this->light)
		{
			this->light = this->gameObjectPtr->getSceneManager()->createLight();

			this->light->setDiffuseColour(this->diffuseColor->getVector3().x, this->diffuseColor->getVector3().y, this->diffuseColor->getVector3().z);
			this->light->setSpecularColour(this->specularColor->getVector3().x, this->specularColor->getVector3().y, this->specularColor->getVector3().z);
			this->light->setPowerScale(this->powerScale->getReal());
			// this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
			this->light->setCastShadows(this->castShadows->getBool());
			this->setAttenuationRadius(this->attenuationRadius->getVector2());
			this->gameObjectPtr->getSceneNode()->attachObject(light);
		}
		
		///////////////////////////////////////Setup Datablock//////////////////////////////////////////////////////////////
        if (nullptr == this->datablock)
		{
			//Set the texture mask to PBS.
// Attention: Look out for strange effects!
			
			assert( dynamic_cast<Ogre::HlmsPbs*>(hlmsPbs) );
			Ogre::HlmsPbs* pbs = static_cast<Ogre::HlmsPbs*>(hlmsPbs);

			Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

			Ogre::TextureGpu* areaTexture = textureManager->findTextureNoThrow(this->textureName->getString());
			if (nullptr != areaTexture)
			{
				textureManager->destroyTexture(areaTexture);
				areaTexture = nullptr;
			}

			//Set the array index of the light mask in mAreaMaskTex
			this->light->mTextureLightMaskIdx = 1u;

			//We know beforehand that floor_bump.PNG & co are 512x512. This is important!!!
			//(because it must match the resolution of the texture created via reservePoolId)
			/*const char *textureNames[4] = { "floor_bump.PNG", "grassWalpha.tga",
											"MtlPlat2.jpg", "Panels_Normal_Obj.png" };*/
// Attention: How does it work with the pools (c_areaLightsPoolId) ??
			if (false == this->textureName->getString().empty())
			{
				areaTexture = textureManager->createOrRetrieveTexture(this->textureName->getString(), this->textureName->getString(),
					Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::Diffuse, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
					this->light->mTextureLightMaskIdx);
			}

			//Set the texture for this area light. The parameters to createOrRetrieveTexture
			//do not matter much, as the texture has already been created.
			/*areaTexture = textureManager->createOrRetrieveTexture(
				this->textureName->getString(), Ogre::GpuPageOutStrategy::AlwaysKeepSystemRamCopy,
					Ogre::TextureFlags::AutomaticBatching,
					Ogre::TextureTypes::Type2D, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);*/

			if (nullptr == areaTexture)
			{
				return;
			}

			// areaTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
			// areaTexture->waitForData();
			textureManager->waitForStreamingCompletion();

			this->light->setTexture(areaTexture);
// Attention: How to control this?
			pbs->setAreaLightMasks(areaTexture);
			pbs->setAreaLightForwardSettings(2u, 2u);
			
			//Setup an unlit material, double-sided, with textures
			//(if it has one) and same color as the light.
			//IMPORTANT: these materials are never destroyed once they're not needed (they will
			//be destroyed by Ogre on shutdown). Watchout for this to prevent memory leaks in
			//a real implementation
			const Ogre::String materialName = "LightPlaneMaterial" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

			Ogre::HlmsMacroblock macroblock;
			macroblock.mCullMode = Ogre::CULL_NONE;
			Ogre::HlmsDatablock* datablockBase = hlmsUnlit->createDatablock(materialName, materialName, macroblock, Ogre::HlmsBlendblock(), Ogre::HlmsParamVec());

			assert(dynamic_cast<Ogre::HlmsUnlitDatablock*>(datablockBase));
			this->datablock = static_cast<Ogre::HlmsUnlitDatablock*>(datablockBase);

			if (light->mTextureLightMaskIdx != std::numeric_limits<Ogre::uint16>::max())
			{
				Ogre::HlmsSamplerblock samplerblock;
				samplerblock.mMaxAnisotropy = 8.0f;
				samplerblock.setFiltering(Ogre::TFO_ANISOTROPIC);

				datablock->setTexture(0, areaTexture, &samplerblock);
			}

			datablock->setUseColour(true);
			datablock->setColour(light->getDiffuseColour());
// Attention: Is this necessary?
			// datablock->setTextureSwizzle(0, Ogre::HlmsUnlitDatablock::R_MASK, Ogre::HlmsUnlitDatablock::R_MASK, Ogre::HlmsUnlitDatablock::R_MASK, Ogre::HlmsUnlitDatablock::R_MASK);
		
			//Control the diffuse mip (this is the default value)
			this->light->mTexLightMaskDiffuseMipStart = (Ogre::uint16)(this->diffuseMipStart->getReal() * 65535); // (Ogre::uint16)(0.95f * 65535);
			this->light->setDoubleSided(this->doubleSided->getBool());
		}
		/*else
		{
			this->light->mTextureLightMaskIdx = std::numeric_limits<Ogre::uint16>::max();
		}*/

		///////////////////////////////////////Create Areaplane//////////////////////////////////////////////////////////////
// Here predefined PlaneComponent? with area texture?
		Ogre::String meshName = this->gameObjectPtr->getName()/* + "mesh"*/;
		Ogre::ResourcePtr resourceV1 = Ogre::v1::MeshManager::getSingletonPtr()->getResourceByName(meshName);
		// Destroy a potential plane v1, because an error occurs (plane with name ... already exists)
		if (nullptr != resourceV1)
		{
			Ogre::v1::MeshManager::getSingletonPtr()->destroyResourcePool(meshName);
			Ogre::v1::MeshManager::getSingletonPtr()->remove(resourceV1->getHandle());
		}

		// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
		Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName(meshName);
		if (nullptr != resourceV2)
		{
			Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(meshName);
			Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
		}
		Ogre::v1::MeshPtr lightPlaneMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
			Ogre::Plane(Ogre::Vector3::UNIT_Z, 0.0f), 1.0f, 1.0f, 1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Y, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

        Ogre::MeshPtr lightPlaneMesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, lightPlaneMeshV1.get(), true, true, true);
        lightPlaneMeshV1->unload();
		lightPlaneMeshV1.setNull();

		const Ogre::Vector2 rectSize = light->getRectSize();
		
		// Later: Make scene node and entity static!
		Ogre::Item* item = this->gameObjectPtr->getSceneManager()->createItem(lightPlaneMesh, Ogre::SCENE_DYNAMIC);

		this->gameObjectPtr->getSceneNode()->setScale( rectSize.x, rectSize.y, 1.0f );
		this->gameObjectPtr->getSceneNode()->attachObject(item);
		// this->gameObjectPtr->getSceneNode()->_setDerivedOrientation(Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::UNIT_X));

		// Set the here newly created entity for this game object
		this->gameObjectPtr->init(item);
		this->gameObjectPtr->setRenderQueueIndex(NOWA::RENDER_QUEUE_V2_MESH);

		// Attention:
		// entity->setCastShadows(false);
		// Register after the component has been created
		AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);

		item->setDatablock(this->datablock);
		this->datablock->setUseColour(true);
		this->datablock->setColour(this->light->getDiffuseColour());
		
        // Math the plane size to that of the area light
		this->setRectSize(this->rectSize->getVector2());

		this->light->setAffectParentNode(this->affectParentNode->getBool());

		this->setDirection(this->direction->getVector3());

		this->setAreaLightType(this->areaLightType->getListSelectedValue());

        /* For debugging ranges & AABBs
        Ogre::WireAabb *wireAabb = sceneManager->createWireAabb();
        wireAabb->track( light );*/
	}

	void LightAreaComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LightAreaComponent::AttrDiffuseColor() == attribute->getName())
		{
			this->setDiffuseColor(attribute->getVector3());
		}
		else if (LightAreaComponent::AttrSpecularColor() == attribute->getName())
		{
			this->setSpecularColor(attribute->getVector3());
		}
		else if (LightAreaComponent::AttrPowerScale() == attribute->getName())
		{
			this->setPowerScale(attribute->getReal());
		}
		else if (LightAreaComponent::AttrDirection() == attribute->getName())
		{
			this->setDirection(attribute->getVector3());
		}
		else if (LightAreaComponent::AttrAffectParentNode() == attribute->getName())
		{
			this->setAffectParentNode(attribute->getBool());
		}
		else if (LightAreaComponent::AttrCastShadows() == attribute->getName())
		{
			this->setCastShadows(attribute->getBool());
		}
		else if (LightAreaComponent::AttrAttenuationRadius() == attribute->getName())
		{
			this->setAttenuationRadius(attribute->getVector2());
		}
		else if (LightAreaComponent::AttrRectSize() == attribute->getName())
		{
			this->setRectSize(attribute->getVector2());
		}
		else if (LightAreaComponent::AttrTextureName() == attribute->getName())
		{
			this->setTextureName(attribute->getString());
		}
		else if (LightAreaComponent::AttrDiffuseMipStart() == attribute->getName())
		{
			this->setDiffuseMipStart(attribute->getReal());
		}
		else if (LightAreaComponent::AttrDoubleSided() == attribute->getName())
		{
			this->setDoubleSided(attribute->getBool());
		}
		else if (LightAreaComponent::AttrAreaLightType() == attribute->getName())
		{
			this->setAreaLightType(attribute->getListSelectedValue());
		}
		
	}

	void LightAreaComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Diffuse"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Specular"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->specularColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PowerScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->powerScale->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Direction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->direction->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AffectParentNode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->affectParentNode->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CastShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->castShadows->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttenuationRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attenuationRadius->getVector2())));
		propertiesXML->append_node(propertyXML);
		
// Attention: Write template method in XMLConverter, depending on set type, internally type string is set etc. so that here is only 1 line instead each time 5 lines!
		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RectSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rectSize->getVector2())));
		propertiesXML->append_node(propertyXML);*/
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseMipStart"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseMipStart->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DoubleSided"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->doubleSided->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AreaLightType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->areaLightType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	void LightAreaComponent::setActivated(bool activated)
	{
		if (nullptr != this->light)
		{
			this->light->setVisible(activated);
		}
	}

	bool LightAreaComponent::isActivated(void) const
	{
		if (nullptr != this->light)
		{
			return this->light->isVisible();
		}
		return false;
	}

	Ogre::String LightAreaComponent::getClassName(void) const
	{
		return "LightAreaComponent";
	}

	Ogre::String LightAreaComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void LightAreaComponent::setDiffuseColor(const Ogre::Vector3& diffuseColor)
	{
		this->diffuseColor->setValue(diffuseColor);
		if (nullptr != this->light)
		{
			this->light->setDiffuseColour(diffuseColor.x, diffuseColor.y, diffuseColor.z);
			if (nullptr != this->datablock)
			{
				this->datablock->setColour(this->light->getDiffuseColour());
			}
		}
	}

	Ogre::Vector3 LightAreaComponent::getDiffuseColor(void) const
	{
		return this->diffuseColor->getVector3();
	}

	void LightAreaComponent::setSpecularColor(const Ogre::Vector3& specularColor)
	{
		this->specularColor->setValue(specularColor);
		if (nullptr != this->light)
		{
			this->light->setSpecularColour(specularColor.x, specularColor.y, specularColor.z);
		}
	}

	Ogre::Vector3 LightAreaComponent::getSpecularColor(void) const
	{
		return this->specularColor->getVector3();
	}

	void LightAreaComponent::setPowerScale(Ogre::Real powerScale)
	{
		if (powerScale < 0.1f)
			powerScale = 0.1f;
		this->powerScale->setValue(powerScale);
		if (nullptr != this->light)
		{
			this->light->setPowerScale(powerScale);
		}
	}

	Ogre::Real LightAreaComponent::getPowerScale(void) const
	{
		return this->powerScale->getReal();
	}

	void LightAreaComponent::setDirection(const Ogre::Vector3& direction)
	{
		this->direction->setValue(direction);
		if (nullptr != this->light)
		{
			this->light->setDirection(direction.normalisedCopy());
		}
	}

	Ogre::Vector3 LightAreaComponent::getDirection(void) const
	{
		return this->direction->getVector3();
	}

	void LightAreaComponent::setAffectParentNode(bool affectParentNode)
	{
		this->affectParentNode->setValue(affectParentNode);
		if (nullptr != this->light)
		{
			this->light->setAffectParentNode(affectParentNode);
		}
	}

	bool LightAreaComponent::getAffectParentNode(void) const
	{
		return this->affectParentNode->getBool();
	}

	void LightAreaComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
		if (nullptr != this->light)
		{
			this->light->setCastShadows(castShadows);
		}
	}

	bool LightAreaComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}

	void LightAreaComponent::setAttenuationRadius(const Ogre::Vector2& attenuationValues)
	{
		this->attenuationRadius->setValue(attenuationValues);
		if (nullptr != this->light)
		{
			this->light->setAttenuationBasedOnRadius(attenuationValues.x, attenuationValues.y);
		}
	}
	
	Ogre::Vector2 LightAreaComponent::getAttenuationRadius(void) const
	{
		return this->attenuationRadius->getVector2();
	}
	
	void LightAreaComponent::setRectSize(const Ogre::Vector2& rectSize)
	{
		Ogre::Vector2 tempRectSize = rectSize;
		if (tempRectSize.x > 5.0f)
			tempRectSize.x = 5.0f;
		if (tempRectSize.y > 5.0f)
			tempRectSize.y = 5.0f;
		if (tempRectSize.x < 0.1f)
			tempRectSize.x = 0.1f;
		if (tempRectSize.y < 0.1f)
			tempRectSize.y = 0.1f;

		this->rectSize->setValue(tempRectSize);
		if (nullptr != this->light)
		{
			this->light->setRectSize(tempRectSize);
			this->gameObjectPtr->getSceneNode()->setScale(rectSize.x, rectSize.y, 1.0f);
		}
	}
	
	Ogre::Vector2 LightAreaComponent::getRectSize(void) const
	{
		return this->rectSize->getVector2();
	}
	
	void LightAreaComponent::setTextureName(const Ogre::String& textureName)
	{
		this->textureName->setValue(textureName);
		if (nullptr != this->light)
		{
			if (true == textureName.empty())
			{
				if (nullptr != this->light)
				{
					// When the array index is 0xFFFF, the light will not use a texture.
					this->light->mTextureLightMaskIdx = std::numeric_limits<Ogre::uint16>::max();
					if (nullptr != this->datablock)
					{
						this->datablock->setTexture(0, nullptr);
						this->light->setTexture(nullptr);
					}
				}
			}
			else
			{
				Ogre::Hlms* hlmsPbs = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
				assert(dynamic_cast<Ogre::HlmsPbs*>(hlmsPbs));
				Ogre::HlmsPbs* pbs = static_cast<Ogre::HlmsPbs*>(hlmsPbs);

				//Set the array index of the light mask in mAreaMaskTex
				this->light->mTextureLightMaskIdx = 1u;

				Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
				Ogre::TextureGpu* areaTexture = textureManager->findTextureNoThrow(this->textureName->getString());
				if (nullptr != areaTexture)
					textureManager->destroyTexture(areaTexture);

				//We know beforehand that floor_bump.PNG & co are 512x512. This is important!!!
				//(because it must match the resolution of the texture created via reservePoolId)
				/*const char *textureNames[4] = { "floor_bump.PNG", "grassWalpha.tga",
												"MtlPlat2.jpg", "Panels_Normal_Obj.png" };*/
												// Attention: How does it work with the pools (c_areaLightsPoolId) ??
				areaTexture = textureManager->createOrRetrieveTexture(this->textureName->getString(), this->textureName->getString(),
					Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::Diffuse, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, this->light->mTextureLightMaskIdx);

				//Set the array index of the light mask in mAreaMaskTex
				this->light->mTextureLightMaskIdx = 1u;

				if (nullptr == areaTexture)
				{
					return;
				}

				// areaTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				// areaTexture->waitForData();
				textureManager->waitForStreamingCompletion();

				this->light->setTexture(areaTexture);
				// Attention: How to control this?
				pbs->setAreaLightMasks(areaTexture);
				pbs->setAreaLightForwardSettings(2u, 2u);

				if (nullptr != areaTexture)
				{
					if (nullptr != this->datablock)
						this->datablock->setTexture(Ogre::PbsTextureTypes::PBSM_DIFFUSE, areaTexture);
// Attention: is this necessary?
					if (nullptr != this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>())
						this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->setDatablock(this->datablock);
					
					if (nullptr != this->light)
						this->light->setTexture(areaTexture);
				}
			}
		}
	}
	
	Ogre::String LightAreaComponent::getTextureName(void) const
	{
		return this->textureName->getString();
	}
	
	void LightAreaComponent::setDiffuseMipStart(Ogre::Real diffuseMipStart)
	{
		this->diffuseMipStart->setValue(diffuseMipStart);
		if (nullptr != this->light)
		{
			this->light->mTexLightMaskDiffuseMipStart = (Ogre::uint16)(diffuseMipStart * 65535); // (Ogre::uint16)(0.95f * 65535);
		}
	}

	Ogre::Real LightAreaComponent::getDiffuseMipStart(void) const
	{
		return this->diffuseMipStart->getReal();
	}
	
	void LightAreaComponent::setDoubleSided(bool doubleSided)
	{
		this->doubleSided->setValue(doubleSided);
		if (nullptr != this->light)
		{
			this->light->setDoubleSided(doubleSided);
		}
	}
	
    bool LightAreaComponent::getDoubleSided(void) const
	{
		return this->doubleSided->getBool();
	}
	
	void LightAreaComponent::setAreaLightType(const Ogre::String& areaLightType)
	{
		this->areaLightType->setListSelectedValue(areaLightType);
		if (nullptr != this->light)
		{
			if ("Area Approximation" == areaLightType)
			{
				this->light->setType(Ogre::Light::LT_AREA_APPROX);
			}
			else
			{
				this->light->setType(Ogre::Light::LT_AREA_LTC);
				this->setTextureName(""); // Is not supported by accurate area light
			}
                                            
		}
	}
	
	Ogre::String LightAreaComponent::getAreaLightType(void) const
	{
		return this->areaLightType->getListSelectedValue();
	}
	
	Ogre::Light* LightAreaComponent::getOgreLight(void) const
	{
		return this->light;
	}

}; // namespace end