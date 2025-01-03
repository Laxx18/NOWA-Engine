#include "NOWAPrecompiled.h"

#include "DecalComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	DecalComponent::DecalComponent()
		: GameObjectComponent(),
		decal(nullptr),
		diffuseTextureName(new Variant(DecalComponent::AttrDiffuseTextureName(), Ogre::String("floor_diffuse.PNG"), this->attributes)),
		normalTextureName(new Variant(DecalComponent::AttrNormalTextureName(), Ogre::String("floor_bump.PNG"), this->attributes)),
		emissiveTextureName(new Variant(DecalComponent::AttrEmissiveTextureName(), Ogre::String(""), this->attributes)),
		ignoreAlpha(new Variant(DecalComponent::AttrIgnoreAlpha(), false, this->attributes)),
		metalness(new Variant(DecalComponent::AttrMetalness(), 1.0f, this->attributes)),
		roughness(new Variant(DecalComponent::AttrRoughness(), 1.0f, this->attributes)),
		rectSize(new Variant(DecalComponent::AttrRectSize(), Ogre::Vector3(1.0f, 1.0f, 1.0f), this->attributes))
	{
		this->rectSize->setDescription("Decals are 2D by nature. The depth of the decal indicates its influence on the objects."
			"Decals are like oriented boxes, and everything inside the box will be affected by the decal.");
	}

	DecalComponent::~DecalComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DecalComponent] Destructor decal component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->decal)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->decal);
			this->gameObjectPtr->getSceneManager()->destroyDecal(this->decal);
			this->decal = nullptr;
		}
	}

	bool DecalComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseTextureName")
		{
			this->diffuseTextureName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NormalTextureName")
		{
			this->normalTextureName->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EmissiveTextureName")
		{
			this->emissiveTextureName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IgnoreAlpha")
		{
			this->ignoreAlpha->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Metalness")
		{
			this->metalness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Roughness")
		{
			this->roughness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RectSize")
		{
			this->rectSize->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr DecalComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		DecalCompPtr clonedCompPtr(boost::make_shared<DecalComponent>());

		
		clonedCompPtr->setDiffuseTextureName(this->diffuseTextureName->getString());
		clonedCompPtr->setNormalTextureName(this->normalTextureName->getString());
		clonedCompPtr->setEmissiveTextureName(this->emissiveTextureName->getString());
		clonedCompPtr->setIgnoreAlpha(this->ignoreAlpha->getBool());
		clonedCompPtr->setMetalness(this->metalness->getReal());
		clonedCompPtr->setRoughness(this->roughness->getReal());
		clonedCompPtr->setRectSize(this->rectSize->getVector3());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool DecalComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DecalComponent] Init decal component for game object: " + this->gameObjectPtr->getName());

		this->createDecal();
		return true;
	}

	void DecalComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void DecalComponent::createDecal(void)
	{
		if (nullptr == this->decal)
		{
			// https://forums.ogre3d.org/viewtopic.php?t=83421
			this->decal = this->gameObjectPtr->getSceneManager()->createDecal();
			this->decal->setQueryFlags(0);
// Attention: is this correct?
			// this->billboardSet->setCastShadows(false);
			// this->billboardSet->setQueryFlags(0);

			// auto data = DeployResourceModule::getInstance()->getPathAndResourceGroupFromDatablock(this->datablockName->getListSelectedValue(), Ogre::HlmsTypes::HLMS_UNLIT);

			// DeployResourceModule::getInstance()->tagResource(this->datablockName->getString(), data.first, data.second);

			this->gameObjectPtr->getSceneNode()->attachObject(this->decal);

// Attention:
			this->gameObjectPtr->setDoNotDestroyMovableObject(true);
			
// Attention: size of pictures must be the same, check it! and reject with error message box in mygui
			this->setDiffuseTextureName(this->diffuseTextureName->getString());
			this->setNormalTextureName(this->normalTextureName->getString());
			this->setEmissiveTextureName(this->emissiveTextureName->getString());
			this->setIgnoreAlpha(this->ignoreAlpha->getBool());
			this->setMetalness(this->metalness->getReal());
			this->setRoughness(this->roughness->getReal());
			this->setRectSize(this->rectSize->getVector3());

			this->gameObjectPtr->init(this->decal);

			// Register after the component has been created
			AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
		}
	}

	void DecalComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (DecalComponent::AttrDiffuseTextureName() == attribute->getName())
		{
			this->setDiffuseTextureName(attribute->getString());
		}
		else if (DecalComponent::AttrNormalTextureName() == attribute->getName())
		{
			this->setNormalTextureName(attribute->getString());
		}
		else if (DecalComponent::AttrEmissiveTextureName() == attribute->getName())
		{
			this->setEmissiveTextureName(attribute->getString());
		}
		else if (DecalComponent::AttrIgnoreAlpha() == attribute->getName())
		{
			this->setIgnoreAlpha(attribute->getBool());
		}
		else if (DecalComponent::AttrMetalness() == attribute->getName())
		{
			this->setMetalness(attribute->getReal());
		}
		else if (DecalComponent::AttrRoughness() == attribute->getName())
		{
			this->setRoughness(attribute->getReal());
		}
		else if (DecalComponent::AttrRectSize() == attribute->getName())
		{
			this->setRectSize(attribute->getVector3());
		}
	}

	void DecalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NormalTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->normalTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EmissiveTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->emissiveTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "IgnoreAlpha"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ignoreAlpha->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metalness->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RectSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rectSize->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String DecalComponent::getClassName(void) const
	{
		return "DecalComponent";
	}

	Ogre::String DecalComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void DecalComponent::setDiffuseTextureName(const Ogre::String& diffuseTextureName)
	{
		this->diffuseTextureName->setValue(diffuseTextureName);
		if (nullptr != this->decal)
		{
			if (false == diffuseTextureName.empty())
			{
				Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
				
				const Ogre::uint32 decalDiffuseId = 1;
				//Create them and load them together to encourage loading them in burst in
				//background thread. Hopefully the information may already be fully available
				//by the time we call decal->setXXXTexture

				/*Ogre::TextureGpu* textureDiffuse = textureManager->createOrRetrieveTexture(diffuseTextureName, Ogre::GpuPageOutStrategy::Discard,
					Ogre::CommonTextureTypes::Diffuse, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, decalDiffuseId);*/

				Ogre::TextureGpu* textureDiffuse = textureManager->createOrRetrieveTexture(diffuseTextureName, Ogre::GpuPageOutStrategy::Discard,
					Ogre::CommonTextureTypes::Diffuse, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

				if (nullptr != textureDiffuse)
				{
					textureDiffuse->scheduleTransitionTo(Ogre::GpuResidency::Resident);

					this->decal->setDiffuseTexture(textureDiffuse);
					this->gameObjectPtr->getSceneManager()->setDecalsDiffuse(textureDiffuse);
				}
			}
		}
	}

	Ogre::String DecalComponent::getDiffuseTextureName(void) const
	{
		return this->diffuseTextureName->getString();
	}
	
	void DecalComponent::setNormalTextureName(const Ogre::String& normalTextureName)
	{
		this->normalTextureName->setValue(normalTextureName);
		if (nullptr != this->decal)
		{
			if (false == normalTextureName.empty())
			{
				const Ogre::uint32 decalNormalId = 1;
				Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

				/*Ogre::TextureGpu* textureNormalMap = textureManager->createOrRetrieveTexture(normalTextureName, Ogre::GpuPageOutStrategy::Discard,
					Ogre::CommonTextureTypes::NormalMap, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, decalNormalId);*/

				Ogre::TextureGpu* textureNormalMap = textureManager->createOrRetrieveTexture(normalTextureName, Ogre::GpuPageOutStrategy::Discard,
					Ogre::CommonTextureTypes::NormalMap, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

				if (nullptr != textureNormalMap)
				{
					textureNormalMap->scheduleTransitionTo(Ogre::GpuResidency::Resident);

					this->decal->setNormalTexture(textureNormalMap);
					this->gameObjectPtr->getSceneManager()->setDecalsNormals(textureNormalMap);
				}
			}
		}
	}

	Ogre::String DecalComponent::getNormalTextureName(void) const
	{
		return this->diffuseTextureName->getString();
	}
	
	void DecalComponent::setEmissiveTextureName(const Ogre::String& emissiveTextureName)
	{
		this->emissiveTextureName->setValue(emissiveTextureName);
		if (nullptr != this->decal)
		{
			if (false == emissiveTextureName.empty())
			{
				Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

				const Ogre::uint32 decalDiffuseId = 1;
				//Create them and load them together to encourage loading them in burst in
				//background thread. Hopefully the information may already be fully available
				//by the time we call decal->setXXXTexture

				/*Ogre::TextureGpu* textureEmmissive = textureManager->createOrRetrieveTexture(emissiveTextureName, Ogre::GpuPageOutStrategy::Discard,
					Ogre::CommonTextureTypes::Diffuse, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, decalDiffuseId);*/

				Ogre::TextureGpu* textureEmmissive = textureManager->createOrRetrieveTexture(emissiveTextureName, Ogre::GpuPageOutStrategy::Discard,
					Ogre::CommonTextureTypes::Diffuse, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

				if (nullptr != textureEmmissive)
				{
					textureEmmissive->scheduleTransitionTo(Ogre::GpuResidency::Resident);

					this->decal->setEmissiveTexture(textureEmmissive);
					this->gameObjectPtr->getSceneManager()->setDecalsEmissive(textureEmmissive);
				}
			}
		}
	}

	Ogre::String DecalComponent::getEmissiveTextureName(void) const
	{
		return this->emissiveTextureName->getString();
	}
	
	void DecalComponent::setIgnoreAlpha(bool ignoreAlpha)
	{
		this->ignoreAlpha->setValue(ignoreAlpha);
		if (nullptr != this->decal)
		{
			this->decal->setIgnoreAlphaDiffuse(ignoreAlpha);
		}
	}

	bool DecalComponent::getIgnoreAlpha(void) const
	{
		return this->ignoreAlpha->getBool();
	}

	void DecalComponent::setMetalness(Ogre::Real metalness)
	{
		this->metalness->setValue(metalness);
		if (nullptr != this->decal)
		{
			this->decal->setMetalness(metalness);
		}
	}

	Ogre::Real DecalComponent::getMetalness(void) const
	{
		return this->metalness->getReal();
	}

	void DecalComponent::setRoughness(Ogre::Real roughness)
	{
		this->roughness->setValue(roughness);
		if (nullptr != this->decal)
		{
			this->decal->setRoughness(roughness);
		}
	}

	Ogre::Real DecalComponent::getRoughness(void) const
	{
		return this->roughness->getReal();
	}

	void DecalComponent::setRectSize(const Ogre::Vector3& rectSizeAndDimensions)
	{
		this->rectSize->setValue(rectSizeAndDimensions);
		if (nullptr != this->decal)
		{
			this->decal->setRectSize(Ogre::Vector2(rectSizeAndDimensions.x, rectSizeAndDimensions.y), rectSizeAndDimensions.z);
		}
	}

	Ogre::Vector3 DecalComponent::getRectSize(void) const
	{
		return this->rectSize->getVector3();
	}

}; // namespace end