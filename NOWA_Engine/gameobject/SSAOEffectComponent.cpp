#include "NOWAPrecompiled.h"
#include "HdrEffectComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "Modules/WorkspaceModule.h"
#include "LightDirectionalComponent.h"

namespace NOWA
{
	using namespace rapidxml;

	HdrEffectComponent::HdrEffectComponent()
		: GameObjectComponent(),
		effectName(new Variant(HdrEffectComponent::AttrEffectName(), { "Bright, sunny day", "Scary Night", "Dark Night", "Dream Night", "Average, slightly hazy day", "Heavy overcast day", "Gibbous moon night", "JJ Abrams style", "Custom" }, this->attributes)),
		skyColor(new Variant(HdrEffectComponent::AttrSkyColor(), Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f, this->attributes)),
		upperHemisphere(new Variant(HdrEffectComponent::AttrUpperHemisphere(), Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f, this->attributes)),
		lowerHemisphere(new Variant(HdrEffectComponent::AttrLowerHemisphere(), Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f, this->attributes)),
		sunPower(new Variant(HdrEffectComponent::AttrSunPower(), 97.0f, this->attributes)),
		exposure(new Variant(HdrEffectComponent::AttrExposure(), 0.0f , this->attributes)),
		minAutoExposure(new Variant(HdrEffectComponent::AttrMinAutoExposure(), -1.0f, this->attributes)),
		maxAutoExposure(new Variant(HdrEffectComponent::AttrMaxAutoExposure(), 2.5f, this->attributes)),
		bloom(new Variant(HdrEffectComponent::AttrBloom(), 5.0f, this->attributes)),
		envMapScale(new Variant(HdrEffectComponent::AttrEnvMapScale(), 16.0f, this->attributes)),
		lightDirectionalComponent(nullptr)
	{
		this->effectName->setUserData(GameObject::AttrActionNeedRefresh());
		this->skyColor->setUserData(GameObject::AttrActionColorDialog());
		this->upperHemisphere->setUserData(GameObject::AttrActionColorDialog());
		this->lowerHemisphere->setUserData(GameObject::AttrActionColorDialog());
		this->sunPower->setConstraints(0.0f, 100.0f);
		this->bloom->setConstraints(0.0001f, 5.0f);

		this->exposure->setDescription("Modifies the HDR Materials for the new exposure parameters. "
			"By default the HDR implementation will try to auto adjust the exposure based on the scene's average luminance. "
			"If left unbounded, even the darkest scenes can look well lit and the brigthest scenes appear too normal. "
			"These parameters are useful to prevent the auto exposure from jumping too much from one extreme to the otherand provide "
			"a consistent experience within the same lighting conditions. (e.g.you may want to change the params when going from indoors to outdoors)"
		    "The smaller the gap between minAutoExposure & maxAutoExposure, the less the auto exposure tries to auto adjust to the scene's lighting conditions. "
		    "The first value is exposure. Valid range is [-4.9; 4.9]. Low values will make the picture darker. Higher values will make the picture brighter.");

		this->minAutoExposure->setDescription("Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure darkens a bright scene. "
			"To prevent that looking at a very bright object makes the rest of the scene really dark, use higher values.");

		this->maxAutoExposure->setDescription("Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure brightens a dark scene. "
			"To prevent that looking at a very dark object makes the rest of the scene really bright, use lower values.");

		this->bloom->setDescription("Controls the bloom intensity. Scale is in lumens / 1024. Valid range is [0.01; 4.9].");

		this->upperHemisphere->setDescription("Ambient colour when the surface normal is close to hemisphereDir.");
		this->lowerHemisphere->setDescription("Ambient colour when the surface normal is pointing away from hemisphereDir.");
		this->envMapScale->setDescription("Global scale to apply to all environment maps (for relevant Hlms implementations, "
			"like PBS). The value will be stored in upperHemisphere.a. Use 1.0 to disable.");
	}

	HdrEffectComponent::~HdrEffectComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] Destructor hdr effect component for game object: " + this->gameObjectPtr->getName());
		this->lightDirectionalComponent = nullptr;
	}

	bool HdrEffectComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EffectName")
		{
			this->effectName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SkyColor")
		{
			this->skyColor->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UpperHemisphere")
		{
			this->upperHemisphere->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LowerHemisphere")
		{
			this->lowerHemisphere->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SunPower")
		{
			this->sunPower->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Exposure")
		{
			this->exposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinAutoExposure")
		{
			this->minAutoExposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxAutoExposure")
		{
			this->maxAutoExposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Bloom")
		{
			this->bloom->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnvMapScale")
		{
			this->envMapScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr HdrEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool HdrEffectComponent::postInit(void)
	{
		if (nullptr == WorkspaceModules::getInstance()->getPrimaryWorkspaceModule())
			return true;

		if (false == WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->getIsHdrUsed())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent] HDR component is useless, since no HDR is activated!");
			return true;
		}
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] Init hdr effect component for game object: " + this->gameObjectPtr->getName());

		// Get the sun light (directional light for sun power setting)
		GameObjectPtr lightGameObjectPtr = GameObjectController::getInstance()->getGameObjectFromName("SunLight");

		if (nullptr == lightGameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent] Could not 'SunLight' for this component. This component must be set under the DirectionalLightComponent ('Sunlight')! Affected game object: " + this->gameObjectPtr->getName());
		}

		auto& lightDirectionalCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());
		if (nullptr != lightDirectionalCompPtr)
		{
			this->lightDirectionalComponent = lightDirectionalCompPtr.get();
		}

		// Apply loaded effect
		this->setEffectName(this->effectName->getListSelectedValue());
		
		return true;
	}

	void HdrEffectComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (HdrEffectComponent::AttrEffectName() == attribute->getName())
		{
			this->setEffectName(attribute->getListSelectedValue());
		}
		else if (HdrEffectComponent::AttrSkyColor() == attribute->getName())
		{
			this->setSkyColor(attribute->getVector4());
		}
		else if (HdrEffectComponent::AttrUpperHemisphere() == attribute->getName())
		{
			this->setUpperHemisphere(attribute->getVector4());
		}
		else if (HdrEffectComponent::AttrLowerHemisphere() == attribute->getName())
		{
			this->setLowerHemisphere(attribute->getVector4());
		}
		else if (HdrEffectComponent::AttrSunPower() == attribute->getName())
		{
			this->setSunPower(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrExposure() == attribute->getName())
		{
			this->setExposure(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrMinAutoExposure() == attribute->getName())
		{
			this->setMinAutoExposure(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrMaxAutoExposure() == attribute->getName())
		{
			this->setMaxAutoExposure(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrBloom() == attribute->getName())
		{
			this->setBloom(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrEnvMapScale() == attribute->getName())
		{
			this->setEnvMapScale(attribute->getReal());
		}
	}

	void HdrEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EffectName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->effectName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SkyColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->skyColor->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UpperHemisphere"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->upperHemisphere->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LowerHemisphere"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->lowerHemisphere->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SunPower"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sunPower->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Exposure"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->exposure->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinAutoExposure"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->minAutoExposure->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxAutoExposure"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->maxAutoExposure->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Bloom"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->bloom->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnvMapScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->envMapScale->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String HdrEffectComponent::getClassName(void) const
	{
		return "HdrEffectComponent";
	}

	Ogre::String HdrEffectComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void HdrEffectComponent::setEffectName(const Ogre::String& effectName)
	{
		this->effectName->setListSelectedValue(effectName);

		if ("Bright, sunny day" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
			this->sunPower->setValue(97.0f);
			this->exposure->setValue(0.0f);
			this->minAutoExposure->setValue(-1.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(5.0f);
			this->envMapScale->setValue(16.0f);
		}
		else if ("Scary Night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
			this->sunPower->setValue(90.0f);
			this->exposure->setValue(-2.0f);
			this->minAutoExposure->setValue(-3.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(2.674f);
			this->envMapScale->setValue(16.0f);
		}
		else if ("Dark Night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
			this->sunPower->setValue(30.0f);
			this->exposure->setValue(-2.0f);
			this->minAutoExposure->setValue(-2.5f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(0.1f);
			this->envMapScale->setValue(16.0f);
		}
		else if ("Dream Night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
			this->sunPower->setValue(97.0f);
			this->exposure->setValue(0.1f);
			this->minAutoExposure->setValue(-1.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(0.001f);
			this->envMapScale->setValue(16.0f);
		}
		else if ("Average, slightly hazy day" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 32.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 3.15f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.0475f);
			this->sunPower->setValue(48.0f);
			this->exposure->setValue(0.0f);
			this->minAutoExposure->setValue(-2.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(5.0f);
			this->envMapScale->setValue(8.0f);
		}
		else if ("Heavy overcast day" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.4f, 0.4f, 0.4f, 1.0f) * 4.5f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.5f, 1.0f) * 0.4f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.5f, 1.0f) * 0.365625f);
			this->sunPower->setValue(6.0625f);
			this->exposure->setValue(0.0f);
			this->minAutoExposure->setValue(-2.5f);
			this->maxAutoExposure->setValue(1.0f);
			this->bloom->setValue(5.0f);
			this->envMapScale->setValue(0.5f);
		}
		else if ("Gibbous moon night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.27f, 0.3f, 0.6f, 1.0f) * 0.01831072f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.50f, 1.0f) * 0.003f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.5f, 0.65f, 1.0f) * 0.00274222f);
			this->sunPower->setValue(0.4f);
			this->exposure->setValue(0.65f);
			this->minAutoExposure->setValue(-2.5f);
			this->maxAutoExposure->setValue(3.0f);
			this->bloom->setValue(5.0f);
			this->envMapScale->setValue(0.0152587890625f);
		}
		else if ("JJ Abrams style" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 6.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 0.1125f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 0.073125f);
			this->sunPower->setValue(4.0f);
			this->exposure->setValue(0.5f);
			this->minAutoExposure->setValue(1.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(3.0f);
			this->envMapScale->setValue(1.0f);
		}

		// Apply values
		Ogre::ColourValue skyColor(this->skyColor->getVector4().x, this->skyColor->getVector4().y, this->skyColor->getVector4().z, this->skyColor->getVector4().w);
		WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->setHdrSkyColour(skyColor, 1.0f);
		WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->setExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
		WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->setBloomThreshold(Ogre::max(this->bloom->getReal() - 2.0f, 0.0f), Ogre::max(this->bloom->getReal(), 0.01f));

		if (nullptr != this->lightDirectionalComponent)
		{
			this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal());
		}

		Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z, this->lowerHemisphere->getVector4().w);
		Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z, this->upperHemisphere->getVector4().w);

		this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
	}

	Ogre::String HdrEffectComponent::getEffectName(void) const
	{
		return this->effectName->getListSelectedValue();
	}

	void HdrEffectComponent::setSkyColor(const Ogre::Vector4& skyColor)
	{
		this->skyColor->setValue(skyColor);
		this->effectName->setListSelectedValue("Custom");
		Ogre::ColourValue tempSkyColor(this->skyColor->getVector4().x, this->skyColor->getVector4().y, this->skyColor->getVector4().z, this->skyColor->getVector4().w);

		WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->setHdrSkyColour(tempSkyColor, 1.0f);
	}

	Ogre::Vector4 HdrEffectComponent::getSkyColor(void) const
	{
		return this->skyColor->getVector4();
	}

	void HdrEffectComponent::setUpperHemisphere(const Ogre::Vector4& upperHemisphere)
	{
		this->upperHemisphere->setValue(upperHemisphere);
		this->effectName->setListSelectedValue("Custom");

		Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z, this->lowerHemisphere->getVector4().w);
		Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z, this->upperHemisphere->getVector4().w);
		this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
	}

	Ogre::Vector4 HdrEffectComponent::getUpperHemisphere(void) const
	{
		return this->upperHemisphere->getVector4();
	}

	void HdrEffectComponent::setLowerHemisphere(const Ogre::Vector4& lowerHemisphere)
	{
		this->lowerHemisphere->setValue(lowerHemisphere);
		this->effectName->setListSelectedValue("Custom");

		Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z, this->lowerHemisphere->getVector4().w);
		Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z, this->upperHemisphere->getVector4().w);
		this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
	}

	Ogre::Vector4 HdrEffectComponent::getLowerHemisphere(void) const
	{
		return this->lowerHemisphere->getVector4();
	}

	void HdrEffectComponent::setSunPower(Ogre::Real sunPower)
	{
		this->sunPower->setValue(sunPower);
		this->effectName->setListSelectedValue("Custom");

		if (nullptr != this->lightDirectionalComponent)
		{
			this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal());
		}
	}

	Ogre::Real HdrEffectComponent::getSunPower(void) const
	{
		return this->sunPower->getReal();
	}

	void HdrEffectComponent::setExposure(Ogre::Real exposure)
	{
		this->exposure->setValue(exposure);
		this->effectName->setListSelectedValue("Custom");

		WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->setExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
	}

	Ogre::Real HdrEffectComponent::getExposure(void) const
	{
		return this->exposure->getReal();
	}

	void HdrEffectComponent::setMinAutoExposure(Ogre::Real minAutoExposure)
	{
		if (minAutoExposure > this->maxAutoExposure->getReal())
			this->maxAutoExposure->setValue(minAutoExposure);
		
		this->minAutoExposure->setValue(minAutoExposure);
		this->effectName->setListSelectedValue("Custom");

		WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->setExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
	}

	Ogre::Real HdrEffectComponent::getMinAutoExposure(void) const
	{
		return this->minAutoExposure->getReal();
	}

	void HdrEffectComponent::setMaxAutoExposure(Ogre::Real maxAutoExposure)
	{
		if (maxAutoExposure < this->minAutoExposure->getReal())
			this->minAutoExposure->setValue(maxAutoExposure);
		
		this->maxAutoExposure->setValue(maxAutoExposure);
		this->effectName->setListSelectedValue("Custom");

		WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->setExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
	}

	Ogre::Real HdrEffectComponent::getMaxAutoExposure(void) const
	{
		return this->maxAutoExposure->getReal();
	}

	void HdrEffectComponent::setBloom(Ogre::Real bloom)
	{
		this->bloom->setValue(bloom);
		this->effectName->setListSelectedValue("Custom");

		WorkspaceModules::getInstance()->getPrimaryWorkspaceModule()->setBloomThreshold(Ogre::max(this->bloom->getReal() - 2.0f, 0.0f), Ogre::max(this->bloom->getReal(), 0.01f));
	}

	Ogre::Real HdrEffectComponent::getBloom(void) const
	{
		return this->bloom->getReal();
	}

	void HdrEffectComponent::setEnvMapScale(Ogre::Real envMapScale)
	{
		if (envMapScale < 0.0f)
			envMapScale = 0.0f;

		this->envMapScale->setValue(envMapScale);
		this->effectName->setListSelectedValue("Custom");

		Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
		Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);
		this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
	}

	Ogre::Real HdrEffectComponent::getEnvMapScale(void) const
	{
		return this->envMapScale->getReal();
	}

}; // namespace end