#include "NOWAPrecompiled.h"
#include "UnderwaterEffectComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/CameraComponent.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	UnderwaterEffectComponent::UnderwaterEffectComponent()
		: GameObjectComponent(),
		name("UnderwaterEffectComponent"),
		usedCamera(nullptr),
		cameraId(new Variant(UnderwaterEffectComponent::AttrCameraId(), static_cast<unsigned long>(0), this->attributes, true)),
		presetName(new Variant(UnderwaterEffectComponent::AttrPresetName(),
			{
				"Balanced Default",
				"Clear Tropical Water",
				"Murky / Turbid Water",
				"Deep Ocean",
				"Shallow Reef",
				"Stormy / Rough Water",
				"Custom"
			},
			this->attributes)),
		waterTint(new Variant(UnderwaterEffectComponent::AttrWaterTint(), Ogre::Vector3(0.04f, 0.22f, 0.32f), this->attributes)),
		deepWaterTint(new Variant(UnderwaterEffectComponent::AttrDeepWaterTint(), Ogre::Vector3(0.01f, 0.08f, 0.15f), this->attributes)),
		fogDensity(new Variant(UnderwaterEffectComponent::AttrFogDensity(), 0.015f, this->attributes)),
		fogStart(new Variant(UnderwaterEffectComponent::AttrFogStart(), 1.0f, this->attributes)),
		maxFogDepth(new Variant(UnderwaterEffectComponent::AttrMaxFogDepth(), 100.0f, this->attributes)),
		absorptionScale(new Variant(UnderwaterEffectComponent::AttrAbsorptionScale(), 0.02f, this->attributes)),
		godRayStrength(new Variant(UnderwaterEffectComponent::AttrGodRayStrength(), 0.4f, this->attributes)),
		godRayDensity(new Variant(UnderwaterEffectComponent::AttrGodRayDensity(), 1.2f, this->attributes)),
		maxGodRayDepth(new Variant(UnderwaterEffectComponent::AttrMaxGodRayDepth(), 50.0f, this->attributes)),
		sunScreenPos(new Variant(UnderwaterEffectComponent::AttrSunScreenPos(), Ogre::Vector2(0.5f, 0.1f), this->attributes)),
		causticStrength(new Variant(UnderwaterEffectComponent::AttrCausticStrength(), 0.25f, this->attributes)),
		maxCausticDepth(new Variant(UnderwaterEffectComponent::AttrMaxCausticDepth(), 30.0f, this->attributes)),
		particleStrength(new Variant(UnderwaterEffectComponent::AttrParticleStrength(), 0.35f, this->attributes)),
		maxParticleDepth(new Variant(UnderwaterEffectComponent::AttrMaxParticleDepth(), 40.0f, this->attributes)),
		bubbleStrength(new Variant(UnderwaterEffectComponent::AttrBubbleStrength(), 0.4f, this->attributes)),
		scatterDensity(new Variant(UnderwaterEffectComponent::AttrScatterDensity(), 0.008f, this->attributes)),
		scatterColor(new Variant(UnderwaterEffectComponent::AttrScatterColor(), Ogre::Vector3(0.1f, 0.2f, 0.3f), this->attributes)),
		distortion(new Variant(UnderwaterEffectComponent::AttrDistortion(), 0.01f, this->attributes)),
		contrast(new Variant(UnderwaterEffectComponent::AttrContrast(), 1.15f, this->attributes)),
		saturation(new Variant(UnderwaterEffectComponent::AttrSaturation(), 0.7f, this->attributes)),
		vignette(new Variant(UnderwaterEffectComponent::AttrVignette(), 1.1f, this->attributes)),
		chromaticAberration(new Variant(UnderwaterEffectComponent::AttrChromaticAberration(), 0.002f, this->attributes))
	{
		this->cameraId->setDescription("The optional camera game object id which can be set. E.g. useful if the MinimapComponent is involved, to set the minimap camera, so that ocean is painted correctly on minimap. Can be left of, default is the main active camera.");

		this->presetName->addUserData(GameObject::AttrActionNeedRefresh());

		this->waterTint->addUserData(GameObject::AttrActionColorDialog());
		this->deepWaterTint->addUserData(GameObject::AttrActionColorDialog());
		this->scatterColor->addUserData(GameObject::AttrActionColorDialog());

		this->fogDensity->setConstraints(0.0f, 1.0f);
		this->fogStart->setConstraints(0.0f, 1000.0f);
		this->maxFogDepth->setConstraints(0.01f, 100000.0f);

		this->absorptionScale->setConstraints(0.0f, 10.0f);

		this->godRayStrength->setConstraints(0.0f, 5.0f);
		this->godRayDensity->setConstraints(0.0f, 10.0f);
		this->maxGodRayDepth->setConstraints(0.0f, 100000.0f);

		this->causticStrength->setConstraints(0.0f, 5.0f);
		this->maxCausticDepth->setConstraints(0.0f, 20.0f);

		this->particleStrength->setConstraints(0.0f, 5.0f);
		this->maxParticleDepth->setConstraints(0.0f, 100000.0f);

		this->bubbleStrength->setConstraints(0.0f, 5.0f);

		this->scatterDensity->setConstraints(0.0f, 10.0f);
		this->distortion->setConstraints(0.0f, 1.0f);
		this->contrast->setConstraints(0.0f, 5.0f);
		this->saturation->setConstraints(0.0f, 5.0f);
		this->vignette->setConstraints(0.0f, 10.0f);
		this->chromaticAberration->setConstraints(0.0f, 0.1f);

		this->presetName->setDescription("Select an underwater preset. If any parameter is changed manually, the preset switches to 'Custom'.");
		this->fogDensity->setDescription("Fog density of the underwater depth fog.");
		this->maxFogDepth->setDescription("Maximum distance/depth for fog to reach full strength.");
		this->absorptionScale->setDescription("Light absorption scale (how quickly light is absorbed with depth).");
		this->godRayStrength->setDescription("Strength of volumetric god rays.");
		this->causticStrength->setDescription("Strength of caustics (usually visible on shallow surfaces).");
		this->particleStrength->setDescription("Strength of underwater particles/debris.");
		this->bubbleStrength->setDescription("Strength of underwater bubbles (rising).");
		this->distortion->setDescription("Distortion amount for underwater post.");
		this->contrast->setDescription("Contrast adjustment for underwater post.");
		this->saturation->setDescription("Saturation adjustment for underwater post.");
		this->vignette->setDescription("Vignette intensity.");
		this->chromaticAberration->setDescription("Chromatic aberration amount.");
	}

	UnderwaterEffectComponent::~UnderwaterEffectComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UnderwaterEffectComponent] Destructor underwater effect component for game object: " + this->gameObjectPtr->getName());
	}

	const Ogre::String& UnderwaterEffectComponent::getName() const
	{
		return this->name;
	}

	void UnderwaterEffectComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<UnderwaterEffectComponent>(UnderwaterEffectComponent::getStaticClassId(), UnderwaterEffectComponent::getStaticClassName());
	}

	void UnderwaterEffectComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool UnderwaterEffectComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &UnderwaterEffectComponent::handleSwitchCamera), NOWA::EventDataSwitchCamera::getStaticEventType());

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraId")
		{
			this->cameraId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PresetName")
		{
			this->presetName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaterTint")
		{
			this->waterTint->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DeepWaterTint")
		{
			this->deepWaterTint->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FogDensity")
		{
			this->fogDensity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FogStart")
		{
			this->fogStart->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxFogDepth")
		{
			this->maxFogDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AbsorptionScale")
		{
			this->absorptionScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GodRayStrength")
		{
			this->godRayStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GodRayDensity")
		{
			this->godRayDensity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxGodRayDepth")
		{
			this->maxGodRayDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SunScreenPos")
		{
			this->sunScreenPos->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CausticStrength")
		{
			this->causticStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxCausticDepth")
		{
			this->maxCausticDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ParticleStrength")
		{
			this->particleStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxParticleDepth")
		{
			this->maxParticleDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BubbleStrength")
		{
			this->bubbleStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScatterDensity")
		{
			this->scatterDensity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScatterColor")
		{
			this->scatterColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Distortion")
		{
			this->distortion->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Contrast")
		{
			this->contrast->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Saturation")
		{
			this->saturation->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Vignette")
		{
			this->vignette->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ChromaticAberration")
		{
			this->chromaticAberration->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr UnderwaterEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool UnderwaterEffectComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UnderwaterEffectComponent] Init underwater effect component for game object: " + this->gameObjectPtr->getName());

		// Apply loaded preset/values
		this->setPresetName(this->presetName->getListSelectedValue());

		return true;
	}

	bool UnderwaterEffectComponent::connect(void)
	{
		GameObjectComponent::connect();

		this->applyUnderwaterParamsToMaterial();

		return true;
	}

	bool UnderwaterEffectComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();
		return true;
	}

	void UnderwaterEffectComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &UnderwaterEffectComponent::handleSwitchCamera), NOWA::EventDataSwitchCamera::getStaticEventType());
	}

	void UnderwaterEffectComponent::onOtherComponentRemoved(unsigned int index)
	{

	}

	void UnderwaterEffectComponent::handleSwitchCamera(EventDataPtr eventData)
	{
		// When camera changed event must be triggered, to set the new camera for the ocean
		WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
		if (nullptr != workspaceBaseComponent)
		{
			boost::shared_ptr<EventDataSwitchCamera> castEventData = boost::static_pointer_cast<EventDataSwitchCamera>(eventData);
			unsigned long id = std::get<0>(castEventData->getCameraGameObjectData());
			unsigned int index = std::get<1>(castEventData->getCameraGameObjectData());
			bool active = std::get<2>(castEventData->getCameraGameObjectData());

			if (true == active)
			{
				auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
				for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
				{
					GameObject* gameObject = it->second.get();
					if (id != gameObject->getId())
					{
						auto cameraComponent = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
						if (nullptr != cameraComponent)
						{
							Ogre::String s1 = cameraComponent->getCamera()->getName();
							Ogre::String s2 = this->usedCamera ? this->usedCamera->getName() : "null";

							if (true == cameraComponent->isActivated() && cameraComponent->getCamera() != this->usedCamera)
							{
								this->applyUnderwaterParamsToMaterial();
							}
						}
					}
				}
			}
		}
	}

	void UnderwaterEffectComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (UnderwaterEffectComponent::AttrCameraId() == attribute->getName())
		{
			this->setCameraId(attribute->getULong());
		}
		else if (UnderwaterEffectComponent::AttrPresetName() == attribute->getName())
		{
			this->setPresetName(attribute->getListSelectedValue());
		}
		else if (UnderwaterEffectComponent::AttrWaterTint() == attribute->getName())
		{
			this->setWaterTint(attribute->getVector3());
		}
		else if (UnderwaterEffectComponent::AttrDeepWaterTint() == attribute->getName())
		{
			this->setDeepWaterTint(attribute->getVector3());
		}
		else if (UnderwaterEffectComponent::AttrFogDensity() == attribute->getName())
		{
			this->setFogDensity(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrFogStart() == attribute->getName())
		{
			this->setFogStart(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrMaxFogDepth() == attribute->getName())
		{
			this->setMaxFogDepth(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrAbsorptionScale() == attribute->getName())
		{
			this->setAbsorptionScale(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrGodRayStrength() == attribute->getName())
		{
			this->setGodRayStrength(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrGodRayDensity() == attribute->getName())
		{
			this->setGodRayDensity(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrMaxGodRayDepth() == attribute->getName())
		{
			this->setMaxGodRayDepth(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrSunScreenPos() == attribute->getName())
		{
			this->setSunScreenPos(attribute->getVector2());
		}
		else if (UnderwaterEffectComponent::AttrCausticStrength() == attribute->getName())
		{
			this->setCausticStrength(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrMaxCausticDepth() == attribute->getName())
		{
			this->setMaxCausticDepth(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrParticleStrength() == attribute->getName())
		{
			this->setParticleStrength(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrMaxParticleDepth() == attribute->getName())
		{
			this->setMaxParticleDepth(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrBubbleStrength() == attribute->getName())
		{
			this->setBubbleStrength(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrScatterDensity() == attribute->getName())
		{
			this->setScatterDensity(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrScatterColor() == attribute->getName())
		{
			this->setScatterColor(attribute->getVector3());
		}
		else if (UnderwaterEffectComponent::AttrDistortion() == attribute->getName())
		{
			this->setDistortion(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrContrast() == attribute->getName())
		{
			this->setContrast(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrSaturation() == attribute->getName())
		{
			this->setSaturation(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrVignette() == attribute->getName())
		{
			this->setVignette(attribute->getReal());
		}
		else if (UnderwaterEffectComponent::AttrChromaticAberration() == attribute->getName())
		{
			this->setChromaticAberration(attribute->getReal());
		}
	}

	void UnderwaterEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cameraId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PresetName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->presetName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		auto writeV3 = [&](const char* name, const Ogre::Vector3& v)
			{
				xml_node<>* n = doc.allocate_node(node_element, "property");
				n->append_attribute(doc.allocate_attribute("type", "9"));
				n->append_attribute(doc.allocate_attribute("name", name));
				n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
				propertiesXML->append_node(n);
			};

		auto writeV2 = [&](const char* name, const Ogre::Vector2& v)
			{
				xml_node<>* n = doc.allocate_node(node_element, "property");
				n->append_attribute(doc.allocate_attribute("type", "8"));
				n->append_attribute(doc.allocate_attribute("name", name));
				n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
				propertiesXML->append_node(n);
			};

		auto writeR = [&](const char* name, Ogre::Real r)
			{
				xml_node<>* n = doc.allocate_node(node_element, "property");
				n->append_attribute(doc.allocate_attribute("type", "6"));
				n->append_attribute(doc.allocate_attribute("name", name));
				n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, r)));
				propertiesXML->append_node(n);
			};

		writeV3("WaterTint", this->waterTint->getVector3());
		writeV3("DeepWaterTint", this->deepWaterTint->getVector3());

		writeR("FogDensity", this->fogDensity->getReal());
		writeR("FogStart", this->fogStart->getReal());
		writeR("MaxFogDepth", this->maxFogDepth->getReal());

		writeR("AbsorptionScale", this->absorptionScale->getReal());

		writeR("GodRayStrength", this->godRayStrength->getReal());
		writeR("GodRayDensity", this->godRayDensity->getReal());
		writeR("MaxGodRayDepth", this->maxGodRayDepth->getReal());
		writeV2("SunScreenPos", this->sunScreenPos->getVector2());

		writeR("CausticStrength", this->causticStrength->getReal());
		writeR("MaxCausticDepth", this->maxCausticDepth->getReal());

		writeR("ParticleStrength", this->particleStrength->getReal());
		writeR("MaxParticleDepth", this->maxParticleDepth->getReal());

		writeR("BubbleStrength", this->bubbleStrength->getReal());

		writeR("ScatterDensity", this->scatterDensity->getReal());
		writeV3("ScatterColor", this->scatterColor->getVector3());

		writeR("Distortion", this->distortion->getReal());
		writeR("Contrast", this->contrast->getReal());
		writeR("Saturation", this->saturation->getReal());
		writeR("Vignette", this->vignette->getReal());
		writeR("ChromaticAberration", this->chromaticAberration->getReal());
	}

	Ogre::String UnderwaterEffectComponent::getClassName(void) const
	{
		return "UnderwaterEffectComponent";
	}

	Ogre::String UnderwaterEffectComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	UnderwaterEffectComponent::UnderwaterParams UnderwaterEffectComponent::gatherParams(void) const
	{
		UnderwaterParams p;
		p.waterTint = this->waterTint->getVector3();
		p.deepWaterTint = this->deepWaterTint->getVector3();
		p.fogDensity = this->fogDensity->getReal();
		p.fogStart = this->fogStart->getReal();
		p.maxFogDepth = this->maxFogDepth->getReal();
		p.absorptionScale = this->absorptionScale->getReal();
		p.godRayStrength = this->godRayStrength->getReal();
		p.godRayDensity = this->godRayDensity->getReal();
		p.maxGodRayDepth = this->maxGodRayDepth->getReal();
		p.sunScreenPos = this->sunScreenPos->getVector2();
		p.causticStrength = this->causticStrength->getReal();
		p.maxCausticDepth = this->maxCausticDepth->getReal();
		p.particleStrength = this->particleStrength->getReal();
		p.maxParticleDepth = this->maxParticleDepth->getReal();
		p.bubbleStrength = this->bubbleStrength->getReal();
		p.scatterDensity = this->scatterDensity->getReal();
		p.scatterColor = this->scatterColor->getVector3();
		p.distortion = this->distortion->getReal();
		p.contrast = this->contrast->getReal();
		p.saturation = this->saturation->getReal();
		p.vignette = this->vignette->getReal();
		p.chromaticAberration = this->chromaticAberration->getReal();
		return p;
	}

	void UnderwaterEffectComponent::applyUnderwaterParamsToMaterial(void)
	{
		if (this->cameraId->getULong() != 0)
		{
			GameObjectPtr cameraGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->cameraId->getULong());
			if (nullptr != cameraGameObjectPtr)
			{
				auto& cameraCompPtr = NOWA::makeStrongPtr(cameraGameObjectPtr->getComponent<CameraComponent>());
				if (nullptr != cameraCompPtr)
				{
					this->usedCamera = cameraCompPtr->getCamera();
				}
			}
		}
		else
		{
			this->usedCamera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
		}

		UnderwaterParams params = this->gatherParams();

		ENQUEUE_RENDER_COMMAND_MULTI("UnderwaterEffectComponent::applyUnderwaterParamsToMaterial", _1(params),
		{
			Ogre::MaterialPtr material =
				Ogre::MaterialManager::getSingleton().load("Ocean/UnderwaterPost",
					Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

			if (material.isNull())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
					"[UnderwaterEffectComponent] Material 'Ocean/UnderwaterPost' not found!");
				return;
			}

			Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
			Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

			// Helpers: only set if constant exists (avoids crash if shader variant doesn't define it)
			auto setFloatIfExists = [&](const Ogre::String& n, Ogre::Real v)
			{
				if (nullptr != psParams->_findNamedConstantDefinition(n, false))
				{
					psParams->setNamedConstant(n, v);
				}
			};

			auto setV2IfExists = [&](const Ogre::String& n, const Ogre::Vector2& v)
			{
				if (nullptr != psParams->_findNamedConstantDefinition(n, false))
				{
					psParams->setNamedConstant(n, v);
				}
			};

			auto setV3IfExists = [&](const Ogre::String& n, const Ogre::Vector3& v)
			{
				if (nullptr != psParams->_findNamedConstantDefinition(n, false))
				{
					psParams->setNamedConstant(n, v);
				}
			};

			// Core params
			setV3IfExists("waterTint", params.waterTint);
			setV3IfExists("deepWaterTint", params.deepWaterTint);

			setFloatIfExists("fogDensity", params.fogDensity);
			setFloatIfExists("fogStart", params.fogStart);
			setFloatIfExists("maxFogDepth", params.maxFogDepth);

			setFloatIfExists("absorptionScale", params.absorptionScale);

			setFloatIfExists("godRayStrength", params.godRayStrength);
			setFloatIfExists("godRayDensity", params.godRayDensity);
			setFloatIfExists("maxGodRayDepth", params.maxGodRayDepth);
			setV2IfExists("sunScreenPos", params.sunScreenPos);

			setFloatIfExists("causticStrength", params.causticStrength);
			setFloatIfExists("maxCausticDepth", params.maxCausticDepth);

			setFloatIfExists("particleStrength", params.particleStrength);
			setFloatIfExists("maxParticleDepth", params.maxParticleDepth);

			setFloatIfExists("bubbleStrength", params.bubbleStrength);

			setFloatIfExists("scatterDensity", params.scatterDensity);
			setV3IfExists("scatterColor", params.scatterColor);

			setFloatIfExists("distortion", params.distortion);
			setFloatIfExists("contrast", params.contrast);
			setFloatIfExists("saturation", params.saturation);
			setFloatIfExists("vignette", params.vignette);
			setFloatIfExists("chromaticAberration", params.chromaticAberration);

			// Camera projection params
			if (nullptr != this->usedCamera)
			{
				Ogre::Vector2 projectionAB = this->usedCamera->getProjectionParamsAB();
				projectionAB.y /= this->usedCamera->getFarClipDistance();

				if (nullptr != psParams->_findNamedConstantDefinition("projectionParams", false))
				{
					psParams->setNamedConstant("projectionParams", projectionAB);
				}
			}
		});
	}

	void UnderwaterEffectComponent::markCustomAndApply(void)
	{
		this->presetName->setListSelectedValue("Custom");
		this->applyUnderwaterParamsToMaterial();
	}

	void UnderwaterEffectComponent::setCameraId(unsigned long cameraId)
	{
		this->cameraId->setValue(cameraId);

		this->applyUnderwaterParamsToMaterial();
	}

	unsigned int UnderwaterEffectComponent::geCameraId(void) const
	{
		return this->cameraId->getULong();
	}

	void UnderwaterEffectComponent::setPresetName(const Ogre::String& presetName)
	{
		this->presetName->setListSelectedValue(presetName);

		// If Custom: keep current values, just apply them
		if ("Custom" == presetName)
		{
			this->applyUnderwaterParamsToMaterial();
			return;
		}

		// ============================================================
		// PRESET: BALANCED DEFAULT (your active preset)
		// ============================================================
		if ("Balanced Default" == presetName)
		{
			this->waterTint->setValue(Ogre::Vector3(0.04f, 0.22f, 0.32f));
			this->deepWaterTint->setValue(Ogre::Vector3(0.01f, 0.08f, 0.15f));
			this->fogDensity->setValue(0.15f);
			this->fogStart->setValue(1.0f);
			this->maxFogDepth->setValue(100.0f);
			this->absorptionScale->setValue(0.02f);
			this->godRayStrength->setValue(0.4f);
			this->godRayDensity->setValue(1.2f);
			this->maxGodRayDepth->setValue(50.0f);
			this->causticStrength->setValue(0.25f);
			this->maxCausticDepth->setValue(30.0f);
			this->particleStrength->setValue(0.35f);
			this->maxParticleDepth->setValue(40.0f);
			this->bubbleStrength->setValue(0.4f);
			this->scatterDensity->setValue(0.008f);
			this->scatterColor->setValue(Ogre::Vector3(0.1f, 0.2f, 0.3f));
			this->distortion->setValue(0.01f);
			this->contrast->setValue(1.15f);
			this->saturation->setValue(0.7f);
			this->vignette->setValue(1.1f);
			this->chromaticAberration->setValue(0.002f);
			// keep sunScreenPos as set by user (default ok)
		}
		// ============================================================
		// PRESET: CLEAR TROPICAL WATER
		// ============================================================
		else if ("Clear Tropical Water" == presetName)
		{
			this->waterTint->setValue(Ogre::Vector3(0.08f, 0.35f, 0.50f));
			this->deepWaterTint->setValue(Ogre::Vector3(0.03f, 0.20f, 0.35f));
			this->fogDensity->setValue(0.08f);
			this->fogStart->setValue(0.0f);
			this->maxFogDepth->setValue(150.0f);
			this->absorptionScale->setValue(0.012f);
			this->godRayStrength->setValue(0.6f);
			this->godRayDensity->setValue(1.2f);
			this->maxGodRayDepth->setValue(80.0f);
			this->causticStrength->setValue(0.4f);
			this->maxCausticDepth->setValue(50.0f);
			this->particleStrength->setValue(0.15f);
			this->maxParticleDepth->setValue(50.0f);
			this->bubbleStrength->setValue(0.25f);
			this->scatterDensity->setValue(0.004f);
			this->scatterColor->setValue(Ogre::Vector3(0.10f, 0.22f, 0.30f));
			this->distortion->setValue(0.010f);
			this->contrast->setValue(1.05f);
			this->saturation->setValue(0.85f);
			this->vignette->setValue(1.0f);
			this->chromaticAberration->setValue(0.0015f);
		}
		// ============================================================
		// PRESET: MURKY / TURBID WATER
		// ============================================================
		else if ("Murky / Turbid Water" == presetName)
		{
			this->waterTint->setValue(Ogre::Vector3(0.06f, 0.15f, 0.20f));
			this->deepWaterTint->setValue(Ogre::Vector3(0.02f, 0.08f, 0.12f));
			this->fogDensity->setValue(1.0f);
			this->fogStart->setValue(0.5f);
			this->maxFogDepth->setValue(30.0f);
			this->absorptionScale->setValue(0.05f);
			this->godRayStrength->setValue(0.1f);
			this->godRayDensity->setValue(1.5f);
			this->maxGodRayDepth->setValue(15.0f);
			this->causticStrength->setValue(0.0f);
			this->maxCausticDepth->setValue(10.0f);
			this->particleStrength->setValue(0.6f);
			this->maxParticleDepth->setValue(20.0f);
			this->bubbleStrength->setValue(0.15f);
			this->scatterDensity->setValue(0.03f);
			this->scatterColor->setValue(Ogre::Vector3(0.12f, 0.14f, 0.15f));
			this->distortion->setValue(0.012f);
			this->contrast->setValue(1.10f);
			this->saturation->setValue(0.5f);
			this->vignette->setValue(1.25f);
			this->chromaticAberration->setValue(0.0025f);
		}
		// ============================================================
		// PRESET: DEEP OCEAN
		// ============================================================
		else if ("Deep Ocean" == presetName)
		{
			this->waterTint->setValue(Ogre::Vector3(0.02f, 0.10f, 0.20f));
			this->deepWaterTint->setValue(Ogre::Vector3(0.0f, 0.03f, 0.08f));
			this->fogDensity->setValue(0.2f);
			this->fogStart->setValue(0.0f);
			this->maxFogDepth->setValue(80.0f);
			this->absorptionScale->setValue(0.03f);
			this->godRayStrength->setValue(0.2f);
			this->godRayDensity->setValue(1.0f);
			this->maxGodRayDepth->setValue(40.0f);
			this->causticStrength->setValue(0.05f);
			this->maxCausticDepth->setValue(15.0f);
			this->particleStrength->setValue(0.4f);
			this->maxParticleDepth->setValue(40.0f);
			this->bubbleStrength->setValue(0.2f);
			this->scatterDensity->setValue(0.015f);
			this->scatterColor->setValue(Ogre::Vector3(0.08f, 0.12f, 0.18f));
			this->distortion->setValue(0.012f);
			this->contrast->setValue(1.25f);
			this->saturation->setValue(0.55f);
			this->vignette->setValue(1.4f);
			this->chromaticAberration->setValue(0.002f);
		}
		// ============================================================
		// PRESET: SHALLOW REEF
		// ============================================================
		else if ("Shallow Reef" == presetName)
		{
			this->waterTint->setValue(Ogre::Vector3(0.10f, 0.40f, 0.55f));
			this->deepWaterTint->setValue(Ogre::Vector3(0.05f, 0.25f, 0.40f));
			this->fogDensity->setValue(0.06f);
			this->fogStart->setValue(0.0f);
			this->maxFogDepth->setValue(200.0f);
			this->absorptionScale->setValue(0.008f);
			this->godRayStrength->setValue(0.7f);
			this->godRayDensity->setValue(1.1f);
			this->maxGodRayDepth->setValue(100.0f);
			this->causticStrength->setValue(0.5f);
			this->maxCausticDepth->setValue(80.0f);
			this->particleStrength->setValue(0.2f);
			this->maxParticleDepth->setValue(80.0f);
			this->bubbleStrength->setValue(0.25f);
			this->scatterDensity->setValue(0.003f);
			this->scatterColor->setValue(Ogre::Vector3(0.10f, 0.22f, 0.30f));
			this->distortion->setValue(0.010f);
			this->contrast->setValue(1.05f);
			this->saturation->setValue(0.95f);
			this->vignette->setValue(1.0f);
			this->chromaticAberration->setValue(0.0015f);
		}
		// ============================================================
		// PRESET: STORMY / ROUGH WATER
		// ============================================================
		else if ("Stormy / Rough Water" == presetName)
		{
			this->waterTint->setValue(Ogre::Vector3(0.04f, 0.18f, 0.28f));
			this->deepWaterTint->setValue(Ogre::Vector3(0.01f, 0.08f, 0.15f));
			this->fogDensity->setValue(0.5f);
			this->fogStart->setValue(0.0f);
			this->maxFogDepth->setValue(60.0f);
			this->absorptionScale->setValue(0.025f);
			this->godRayStrength->setValue(0.15f);
			this->godRayDensity->setValue(1.5f);
			this->maxGodRayDepth->setValue(40.0f);
			this->causticStrength->setValue(0.1f);
			this->maxCausticDepth->setValue(20.0f);
			this->particleStrength->setValue(0.5f);
			this->maxParticleDepth->setValue(40.0f);
			this->bubbleStrength->setValue(0.7f);
			this->scatterDensity->setValue(0.02f);
			this->scatterColor->setValue(Ogre::Vector3(0.10f, 0.16f, 0.22f));
			this->distortion->setValue(0.025f);
			this->contrast->setValue(1.20f);
			this->saturation->setValue(0.6f);
			this->vignette->setValue(1.2f);
			this->chromaticAberration->setValue(0.004f);
		}

		this->applyUnderwaterParamsToMaterial();
	}

	Ogre::String UnderwaterEffectComponent::getPresetName(void) const
	{
		return this->presetName->getListSelectedValue();
	}

	void UnderwaterEffectComponent::setWaterTint(const Ogre::Vector3& v)
	{
		this->waterTint->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Vector3 UnderwaterEffectComponent::getWaterTint(void) const
	{
		return this->waterTint->getVector3();
	}

	void UnderwaterEffectComponent::setDeepWaterTint(const Ogre::Vector3& v)
	{
		this->deepWaterTint->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Vector3 UnderwaterEffectComponent::getDeepWaterTint(void) const
	{
		return this->deepWaterTint->getVector3();
	}

	void UnderwaterEffectComponent::setFogDensity(Ogre::Real v)
	{
		this->fogDensity->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getFogDensity(void) const
	{
		return this->fogDensity->getReal();
	}

	void UnderwaterEffectComponent::setFogStart(Ogre::Real v)
	{
		this->fogStart->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getFogStart(void) const
	{
		return this->fogStart->getReal();
	}

	void UnderwaterEffectComponent::setMaxFogDepth(Ogre::Real v)
	{
		this->maxFogDepth->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getMaxFogDepth(void) const
	{
		return this->maxFogDepth->getReal();
	}

	void UnderwaterEffectComponent::setAbsorptionScale(Ogre::Real v)
	{
		this->absorptionScale->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getAbsorptionScale(void) const
	{
		return this->absorptionScale->getReal();
	}

	void UnderwaterEffectComponent::setGodRayStrength(Ogre::Real v)
	{
		this->godRayStrength->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getGodRayStrength(void) const
	{
		return this->godRayStrength->getReal();
	}

	void UnderwaterEffectComponent::setGodRayDensity(Ogre::Real v)
	{
		this->godRayDensity->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getGodRayDensity(void) const
	{
		return this->godRayDensity->getReal();
	}

	void UnderwaterEffectComponent::setMaxGodRayDepth(Ogre::Real v)
	{
		this->maxGodRayDepth->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getMaxGodRayDepth(void) const
	{
		return this->maxGodRayDepth->getReal();
	}

	void UnderwaterEffectComponent::setSunScreenPos(const Ogre::Vector2& v)
	{
		this->sunScreenPos->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Vector2 UnderwaterEffectComponent::getSunScreenPos(void) const
	{
		return this->sunScreenPos->getVector2();
	}

	void UnderwaterEffectComponent::setCausticStrength(Ogre::Real v)
	{
		this->causticStrength->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getCausticStrength(void) const
	{
		return this->causticStrength->getReal();
	}

	void UnderwaterEffectComponent::setMaxCausticDepth(Ogre::Real v)
	{
		this->maxCausticDepth->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getMaxCausticDepth(void) const
	{
		return this->maxCausticDepth->getReal();
	}

	void UnderwaterEffectComponent::setParticleStrength(Ogre::Real v)
	{
		this->particleStrength->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getParticleStrength(void) const
	{
		return this->particleStrength->getReal();
	}

	void UnderwaterEffectComponent::setMaxParticleDepth(Ogre::Real v)
	{
		this->maxParticleDepth->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getMaxParticleDepth(void) const
	{
		return this->maxParticleDepth->getReal();
	}

	void UnderwaterEffectComponent::setBubbleStrength(Ogre::Real v)
	{
		this->bubbleStrength->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getBubbleStrength(void) const
	{
		return this->bubbleStrength->getReal();
	}

	void UnderwaterEffectComponent::setScatterDensity(Ogre::Real v)
	{
		this->scatterDensity->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getScatterDensity(void) const
	{
		return this->scatterDensity->getReal();
	}

	void UnderwaterEffectComponent::setScatterColor(const Ogre::Vector3& v)
	{
		this->scatterColor->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Vector3 UnderwaterEffectComponent::getScatterColor(void) const
	{
		return this->scatterColor->getVector3();
	}

	void UnderwaterEffectComponent::setDistortion(Ogre::Real v)
	{
		this->distortion->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getDistortion(void) const
	{
		return this->distortion->getReal();
	}

	void UnderwaterEffectComponent::setContrast(Ogre::Real v)
	{
		this->contrast->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getContrast(void) const
	{
		return this->contrast->getReal();
	}

	void UnderwaterEffectComponent::setSaturation(Ogre::Real v)
	{
		this->saturation->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getSaturation(void) const
	{
		return this->saturation->getReal();
	}

	void UnderwaterEffectComponent::setVignette(Ogre::Real v)
	{
		this->vignette->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getVignette(void) const
	{
		return this->vignette->getReal();
	}

	void UnderwaterEffectComponent::setChromaticAberration(Ogre::Real v)
	{
		this->chromaticAberration->setValue(v);
		this->markCustomAndApply();
	}
	Ogre::Real UnderwaterEffectComponent::getChromaticAberration(void) const
	{
		return this->chromaticAberration->getReal();
	}

	bool UnderwaterEffectComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto underwaterCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<UnderwaterEffectComponent>());
		auto oceanCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<OceanComponent>());

		// Constraints: Must have OceanComponent and only once
		if (nullptr != oceanCompPtr && nullptr == underwaterCompPtr)
		{
			return true;
		}
		return false;
	}

	// -------------------- Lua registration part --------------------

	UnderwaterEffectComponent* getUnderwaterEffectComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<UnderwaterEffectComponent>(gameObject->getComponentWithOccurrence<UnderwaterEffectComponent>(occurrenceIndex)).get();
	}

	UnderwaterEffectComponent* getUnderwaterEffectComponent(GameObject* gameObject)
	{
		return makeStrongPtr<UnderwaterEffectComponent>(gameObject->getComponent<UnderwaterEffectComponent>()).get();
	}

	UnderwaterEffectComponent* getUnderwaterEffectComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<UnderwaterEffectComponent>(gameObject->getComponentFromName<UnderwaterEffectComponent>(name)).get();
	}

	void UnderwaterEffectComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<UnderwaterEffectComponent, GameObjectComponent>("UnderwaterEffectComponent")
			.def("setCameraId", &UnderwaterEffectComponent::setCameraId)
			.def("geCameraId", &UnderwaterEffectComponent::geCameraId)

			.def("setPresetName", &UnderwaterEffectComponent::setPresetName)
			.def("getPresetName", &UnderwaterEffectComponent::getPresetName)

			.def("setWaterTint", &UnderwaterEffectComponent::setWaterTint)
			.def("getWaterTint", &UnderwaterEffectComponent::getWaterTint)
			.def("setDeepWaterTint", &UnderwaterEffectComponent::setDeepWaterTint)
			.def("getDeepWaterTint", &UnderwaterEffectComponent::getDeepWaterTint)

			.def("setFogDensity", &UnderwaterEffectComponent::setFogDensity)
			.def("getFogDensity", &UnderwaterEffectComponent::getFogDensity)
			.def("setFogStart", &UnderwaterEffectComponent::setFogStart)
			.def("getFogStart", &UnderwaterEffectComponent::getFogStart)
			.def("setMaxFogDepth", &UnderwaterEffectComponent::setMaxFogDepth)
			.def("getMaxFogDepth", &UnderwaterEffectComponent::getMaxFogDepth)

			.def("setAbsorptionScale", &UnderwaterEffectComponent::setAbsorptionScale)
			.def("getAbsorptionScale", &UnderwaterEffectComponent::getAbsorptionScale)

			.def("setGodRayStrength", &UnderwaterEffectComponent::setGodRayStrength)
			.def("getGodRayStrength", &UnderwaterEffectComponent::getGodRayStrength)
			.def("setGodRayDensity", &UnderwaterEffectComponent::setGodRayDensity)
			.def("getGodRayDensity", &UnderwaterEffectComponent::getGodRayDensity)
			.def("setMaxGodRayDepth", &UnderwaterEffectComponent::setMaxGodRayDepth)
			.def("getMaxGodRayDepth", &UnderwaterEffectComponent::getMaxGodRayDepth)
			.def("setSunScreenPos", &UnderwaterEffectComponent::setSunScreenPos)
			.def("getSunScreenPos", &UnderwaterEffectComponent::getSunScreenPos)

			.def("setCausticStrength", &UnderwaterEffectComponent::setCausticStrength)
			.def("getCausticStrength", &UnderwaterEffectComponent::getCausticStrength)
			.def("setMaxCausticDepth", &UnderwaterEffectComponent::setMaxCausticDepth)
			.def("getMaxCausticDepth", &UnderwaterEffectComponent::getMaxCausticDepth)

			.def("setParticleStrength", &UnderwaterEffectComponent::setParticleStrength)
			.def("getParticleStrength", &UnderwaterEffectComponent::getParticleStrength)
			.def("setMaxParticleDepth", &UnderwaterEffectComponent::setMaxParticleDepth)
			.def("getMaxParticleDepth", &UnderwaterEffectComponent::getMaxParticleDepth)

			.def("setBubbleStrength", &UnderwaterEffectComponent::setBubbleStrength)
			.def("getBubbleStrength", &UnderwaterEffectComponent::getBubbleStrength)

			.def("setScatterDensity", &UnderwaterEffectComponent::setScatterDensity)
			.def("getScatterDensity", &UnderwaterEffectComponent::getScatterDensity)
			.def("setScatterColor", &UnderwaterEffectComponent::setScatterColor)
			.def("getScatterColor", &UnderwaterEffectComponent::getScatterColor)

			.def("setDistortion", &UnderwaterEffectComponent::setDistortion)
			.def("getDistortion", &UnderwaterEffectComponent::getDistortion)
			.def("setContrast", &UnderwaterEffectComponent::setContrast)
			.def("getContrast", &UnderwaterEffectComponent::getContrast)
			.def("setSaturation", &UnderwaterEffectComponent::setSaturation)
			.def("getSaturation", &UnderwaterEffectComponent::getSaturation)
			.def("setVignette", &UnderwaterEffectComponent::setVignette)
			.def("getVignette", &UnderwaterEffectComponent::getVignette)
			.def("setChromaticAberration", &UnderwaterEffectComponent::setChromaticAberration)
			.def("getChromaticAberration", &UnderwaterEffectComponent::getChromaticAberration)
		];

		gameObjectClass.def("getUnderwaterEffectComponentFromName", &getUnderwaterEffectComponentFromName);
		gameObjectClass.def("getUnderwaterEffectComponent", (UnderwaterEffectComponent * (*)(GameObject*)) & getUnderwaterEffectComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "UnderwaterEffectComponent getUnderwaterEffectComponent()", "Gets the component. This can be used if the game object has this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "UnderwaterEffectComponent getUnderwaterEffectComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castUnderwaterEffectComponent", &GameObjectController::cast<UnderwaterEffectComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "UnderwaterEffectComponent castUnderwaterEffectComponent(UnderwaterEffectComponent other)", "Casts an incoming type from function for lua auto completion.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "class inherits GameObjectComponent", UnderwaterEffectComponent::getStaticInfoText());

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setPresetName(string presetName)",
			"Sets the underwater preset name. Possible values are: "
			"'Balanced Default', 'Clear Tropical Water', 'Murky / Turbid Water', 'Deep Ocean', 'Shallow Reef', 'Stormy / Rough Water', 'Custom'. "
			"Note: If any value is manipulated manually, the preset name will be set to 'Custom'.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "string getPresetName()", "Gets the currently set preset name.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setWaterTint(Vector3 waterTint)", "Sets water tint color.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "Vector3 getWaterTint()", "Gets water tint color.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setDeepWaterTint(Vector3 deepWaterTint)", "Sets deep water tint color.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "Vector3 getDeepWaterTint()", "Gets deep water tint color.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setFogDensity(float fogDensity)", "Sets fog density.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getFogDensity()", "Gets fog density.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setFogStart(float fogStart)", "Sets fog start distance (if supported by shader).");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getFogStart()", "Gets fog start.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setMaxFogDepth(float maxFogDepth)", "Sets max fog depth.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getMaxFogDepth()", "Gets max fog depth.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setAbsorptionScale(float absorptionScale)", "Sets absorption scale.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getAbsorptionScale()", "Gets absorption scale.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setGodRayStrength(float godRayStrength)", "Sets god ray strength.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getGodRayStrength()", "Gets god ray strength.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setGodRayDensity(float godRayDensity)", "Sets god ray density (if supported by shader).");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getGodRayDensity()", "Gets god ray density.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setMaxGodRayDepth(float maxGodRayDepth)", "Sets max god ray depth (if supported by shader).");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getMaxGodRayDepth()", "Gets max god ray depth.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setSunScreenPos(Vector2 sunScreenPos)", "Sets sun screen position for god rays.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "Vector2 getSunScreenPos()", "Gets sun screen position.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setCausticStrength(float causticStrength)", "Sets caustic strength.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getCausticStrength()", "Gets caustic strength.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setMaxCausticDepth(float maxCausticDepth)", "Sets max caustic depth.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getMaxCausticDepth()", "Gets max caustic depth.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setParticleStrength(float particleStrength)", "Sets particle strength.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getParticleStrength()", "Gets particle strength.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setMaxParticleDepth(float maxParticleDepth)", "Sets max particle depth.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getMaxParticleDepth()", "Gets max particle depth.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setBubbleStrength(float bubbleStrength)", "Sets bubble strength.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getBubbleStrength()", "Gets bubble strength.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setScatterDensity(float scatterDensity)", "Sets scatter density (if supported by shader).");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getScatterDensity()", "Gets scatter density.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setScatterColor(Vector3 scatterColor)", "Sets scatter color (if supported by shader).");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "Vector3 getScatterColor()", "Gets scatter color.");

		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setDistortion(float distortion)", "Sets distortion.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getDistortion()", "Gets distortion.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setContrast(float contrast)", "Sets contrast.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getContrast()", "Gets contrast.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setSaturation(float saturation)", "Sets saturation.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getSaturation()", "Gets saturation.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setVignette(float vignette)", "Sets vignette.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getVignette()", "Gets vignette.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "void setChromaticAberration(float chromaticAberration)", "Sets chromatic aberration.");
		LuaScriptApi::getInstance()->addClassToCollection("UnderwaterEffectComponent", "float getChromaticAberration()", "Gets chromatic aberration.");
	}

}; // namespace end