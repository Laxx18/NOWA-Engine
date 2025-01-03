#include "NOWAPrecompiled.h"
#include "LensFlareComponent.h"
#include "LightDirectionalComponent.h"
#include "CameraComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	LensFlareComponent::LensFlareComponent()
		: GameObjectComponent(),
		light(nullptr),
		camera(nullptr),
		haloSet(nullptr),
		burstSet(nullptr),
		activated(new Variant(LensFlareComponent::AttrActivated(), true, this->attributes)),
		lightId(new Variant(LensFlareComponent::AttrLightId(), static_cast<unsigned long>(0), this->attributes, true)),
		cameraId(new Variant(LensFlareComponent::AttrCameraId(), static_cast<unsigned long>(0), this->attributes, true)),
		haloColor(new Variant(LensFlareComponent::AttrHaloColor(), Ogre::Vector3(1.0f, 1.0f, 0.0f), this->attributes)),
		burstColor(new Variant(LensFlareComponent::AttrBurstColor(), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes))
	{
		this->haloColor->addUserData(GameObject::AttrActionColorDialog());
		this->burstColor->addUserData(GameObject::AttrActionColorDialog());
		
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &LensFlareComponent::handleRemoveCamera), EventDataRemoveCamera::getStaticEventType());
	}

	LensFlareComponent::~LensFlareComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LensFlareComponent] Destructor lens flare component for game object: " + this->gameObjectPtr->getName());
		
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &LensFlareComponent::handleRemoveCamera), EventDataRemoveCamera::getStaticEventType());
		
		if (nullptr != this->haloSet)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->haloSet);
			this->gameObjectPtr->getSceneManager()->destroyBillboardSet(this->haloSet);
			this->haloSet = nullptr;
		}
		if (nullptr != this->burstSet)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->burstSet);
			this->gameObjectPtr->getSceneManager()->destroyBillboardSet(this->burstSet);
			this->burstSet = nullptr;
		}
	}
	
	void LensFlareComponent::handleRemoveCamera(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataRemoveCamera> castEventData = boost::static_pointer_cast<EventDataRemoveCamera>(eventData);
		if (this->camera == castEventData->getCamera())
		{
			// Set camera to null, if it has been removed by the camera component
			this->camera = nullptr;
			this->cameraId->setValue(static_cast<unsigned long>(0));
		}
	}

	bool LensFlareComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LightId")
		{
			this->lightId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraId")
		{
			this->cameraId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HaloColor")
		{
			this->haloColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BurstColor")
		{
			this->burstColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr LensFlareComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Only one lens flare supported? Or for each light??
		return nullptr;
	}

	bool LensFlareComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LensFlareComponent] Init billboard component for game object: " + this->gameObjectPtr->getName());
// Attention: This is done in post init, what if camera or light is not yet available?
		this->setLightId(this->lightId->getULong());
		this->setCameraId(this->lightId->getULong());
		
		this->createLensFlare();
		return true;
	}
	
	bool LensFlareComponent::connect(void)
	{
		this->setLightId(this->lightId->getULong());
		this->setCameraId(this->lightId->getULong());
		
		return true;
	}
	
	void LensFlareComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (nullptr == this->camera || nullptr == this->light)
			return;
		
		Ogre::Vector3 lightPosition = this->light->getParentSceneNode()->_getDerivedPosition();
		
		// If the Light is out of the Ogre::Camera field Of View, the lensflare is hidden.
		if (false == this->camera->isVisible(lightPosition))
		{
			this->haloSet->setVisible(false);
			this->burstSet->setVisible(false);
			return;
		}
		else
		{
			this->haloSet->setVisible(true);
			this->burstSet->setVisible(true);
		}

		Ogre::Real lightDistance = lightPosition.distance(this->camera->getPosition());
		Ogre::Vector3 cameraVector = this->camera->getDirection(); // normalized vector (length 1)

		cameraVector = this->camera->getPosition() + (lightDistance * cameraVector);

		// The LensFlare effect takes place along this vector.
		Ogre::Vector3 lensFlareVector = (cameraVector - lightPosition);
		//lensFlareVector += Ogre::Vector3(-64,-64,0);  // sprite dimension (to be adjusted, but not necessary)

		// The different sprites are placed along this line.
		this->haloSet->getBillboard(0)->setPosition(lensFlareVector * 0.500f);
		this->haloSet->getBillboard(1)->setPosition(lensFlareVector*  0.125f);
		this->haloSet->getBillboard(2)->setPosition(-lensFlareVector * 0.250f);

		this->burstSet->getBillboard(0)->setPosition(lensFlareVector * 0.333f);
		this->burstSet->getBillboard(1)->setPosition(-lensFlareVector * 0.500f);
		this->burstSet->getBillboard(2)->setPosition(-lensFlareVector * 0.180f);

		// We redraw the lensflare (in case it was previouly out of the camera field, and hidden)
// Attention:
		this->setActivated(this->activated->getBool());
	}

	void LensFlareComponent::createLensFlare(void)
	{
		if (nullptr == this->haloSet)
		{
			Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
			Ogre::HlmsUnlit* hlmsUnlit = dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

			// this->datablock = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock("FadeEffect", "FadeEffect", Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));
			this->datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlmsManager->getDatablock("lensflare/halo"));
			
			Ogre::Real scale = 2000.0f;
			
			// https://forums.ogre3d.org/viewtopic.php?t=83421
			this->haloSet = this->gameObjectPtr->getSceneManager()->createBillboardSet();
			
			this->haloSet->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
			this->haloSet->setDatablock(this->datablock);
			this->haloSet->setCullIndividually(true);
			this->haloSet->setQueryFlags(0);	// They should not be detected by rays.
			// Must be set to false, else an ugly depthRange vertex shader error occurs, because an unlit material is used
	// Attention: True or false?
			this->haloSet->setCastShadows(false);

			auto data = DeployResourceModule::getInstance()->getPathAndResourceGroupFromDatablock("lensflare/halo", Ogre::HlmsTypes::HLMS_UNLIT);

			DeployResourceModule::getInstance()->tagResource("lensflare/halo", data.first, data.second);

			this->gameObjectPtr->getSceneNode()->attachObject(this->haloSet);
			
			this->burstSet = this->gameObjectPtr->getSceneManager()->createBillboardSet();
			this->burstSet->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
	// Attention: Where is this material? Is it in the default resource group name?
			this->burstSet->setDatablock(this->datablock);
			this->burstSet->setCullIndividually(true);
			this->burstSet->setQueryFlags(0);
	// Attention: True or false?
			this->burstSet->setCastShadows(false);
			
			this->gameObjectPtr->getSceneNode()->attachObject(this->burstSet);
			
		// -------------------------------
		// Creation of the Halo billboards
		// -------------------------------
		Ogre::v1::Billboard* halo1 = this->haloSet->createBillboard(0, 0, 0);
		halo1->setDimensions(scale * 0.5f, scale * 0.5f);
		Ogre::v1::Billboard* halo2 = this->haloSet->createBillboard(0, 0, 0);
		halo2->setDimensions(scale, scale);
		Ogre::v1::Billboard* halo3 = this->haloSet->createBillboard(0, 0, 0);
		halo3->setDimensions(scale * 0.25f, scale * 0.25f);


		// -------------------------------
		// Creation of the "Burst" billboards
		// -------------------------------
		Ogre::v1::Billboard* burst1 = this->burstSet->createBillboard(0, 0, 0);
		burst1->setDimensions(scale * 0.25f, scale * 0.25f);
		Ogre::v1::Billboard* burst2 = this->burstSet->createBillboard(0, 0, 0);
		burst2->setDimensions(scale * 0.5f, scale * 0.5f);
		Ogre::v1::Billboard* burst3 = this->burstSet->createBillboard(0, 0, 0);
		burst3->setDimensions(scale * 0.25f, scale * 0.25f);
		}
	}

	void LensFlareComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LensFlareComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (LensFlareComponent::AttrLightId() == attribute->getName())
		{
			this->setLightId(attribute->getULong());
		}
		else if (LensFlareComponent::AttrCameraId() == attribute->getName())
		{
			this->setCameraId(attribute->getULong());
		}
		else if (LensFlareComponent::AttrHaloColor() == attribute->getName())
		{
			this->setHaloColor(attribute->getVector3());
		}
		else if (LensFlareComponent::AttrBurstColor() == attribute->getName())
		{
			this->setBurstColor(attribute->getVector3());
		}
	}

	void LensFlareComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LightId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lightId->getULong())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cameraId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HaloColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->haloColor->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BurstColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->burstColor->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	void LensFlareComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (nullptr != this->haloSet)
		{
			this->haloSet->setVisible(activated);
			this->burstSet->setVisible(activated);
		}
		this->hidden = !activated;
	}

	bool LensFlareComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	Ogre::String LensFlareComponent::getClassName(void) const
	{
		return "LensFlareComponent";
	}

	Ogre::String LensFlareComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}
	
	void LensFlareComponent::setLightId(unsigned long lightId)
	{
		this->lightId->setValue(lightId);
		GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(lightId);
		if (nullptr != lightGameObjectPtr)
		{
			auto lightCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());
			if (nullptr != lightCompPtr)
			{
				this->light = lightCompPtr->getOgreLight();
			}
		}
	}

	unsigned long LensFlareComponent::getLightId(void) const
	{
		return this->lightId->getULong();
	}
	
	void LensFlareComponent::setCameraId(unsigned long cameraId)
	{
		this->cameraId->setValue(cameraId);
		GameObjectPtr cameraGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(cameraId);
		if (nullptr != cameraGameObjectPtr)
		{
			auto cameraCompPtr = NOWA::makeStrongPtr(cameraGameObjectPtr->getComponent<CameraComponent>());
			if (nullptr != cameraCompPtr)
			{
				this->camera = cameraCompPtr->getCamera();
			}
		}
	}

	unsigned long LensFlareComponent::getCameraId(void) const
	{
		return this->cameraId->getULong();
	}

	void LensFlareComponent::setHaloColor(const Ogre::Vector3& haloColor)
	{
		this->haloColor->setValue(haloColor);
		if (nullptr != this->burstSet)
		{
			this->haloSet->getBillboard(0)->setColour(Ogre::ColourValue(haloColor.x, haloColor.y, haloColor.z, 1.0f)  * 0.8f);
			this->haloSet->getBillboard(1)->setColour(Ogre::ColourValue(haloColor.x, haloColor.y, haloColor.z, 1.0f) * 0.6f);
			this->haloSet->getBillboard(2)->setColour(Ogre::ColourValue(haloColor.x, haloColor.y, haloColor.z, 1.0f));
		}
	}
	
	Ogre::Vector3 LensFlareComponent::getHaloColor(void) const
	{
		return this->haloColor->getVector3();
	}
	
	void LensFlareComponent::setBurstColor(const Ogre::Vector3& burstColor)
	{
		this->burstColor->setValue(burstColor);
		if (nullptr != this->burstSet)
		{
			this->burstSet->getBillboard(0)->setColour(Ogre::ColourValue(burstColor.x, burstColor.y, burstColor.z, 1.0f));
			this->burstSet->getBillboard(1)->setColour(Ogre::ColourValue(burstColor.x, burstColor.y, burstColor.z, 1.0f) * 0.8f);
			this->burstSet->getBillboard(2)->setColour(Ogre::ColourValue(burstColor.x, burstColor.y, burstColor.z, 1.0f) * 0.6f);
		}
	}

	Ogre::Vector3 LensFlareComponent::getBurstColor(void) const
	{
		return this->burstColor->getVector3();
	}

	Ogre::v1::BillboardSet* LensFlareComponent::getHaloBillboard(void) const
	{
		return this->haloSet;
	}
	
	Ogre::v1::BillboardSet* LensFlareComponent::getBurstBillboard(void) const
	{
		return this->burstSet;
	}

}; // namespace end