#include "NOWAPrecompiled.h"
#include "OceanEffectComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "gameobject/OceanComponent.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	OceanEffectComponent::OceanEffectComponent()
		: GameObjectComponent(),
		name("OceanEffectComponent"),
		presetName(new Variant(OceanEffectComponent::AttrPresetName(),
			{
				"Calm Lake",
				"Gentle Ocean",
				"Choppy Water",
				"Storm Sea",
				"Big Swell",
				"Fantasy Water",
				"Custom"
			}, this->attributes)),
		deepColour(new Variant(OceanEffectComponent::AttrDeepColour(), Ogre::Vector3(0.02f, 0.10f, 0.18f), this->attributes)),
		shallowColour(new Variant(OceanEffectComponent::AttrShallowColour(), Ogre::Vector3(0.06f, 0.25f, 0.30f), this->attributes)),
		brdf(new Variant(OceanEffectComponent::AttrBrdf(), Ogre::String("Default"), this->attributes)),

		shaderWavesScale(new Variant(OceanEffectComponent::AttrShaderWavesScale(), 1.0f, this->attributes)),
		wavesIntensity(new Variant(OceanEffectComponent::AttrWavesIntensity(), 0.55f, this->attributes)),
		oceanWavesScale(new Variant(OceanEffectComponent::AttrOceanWavesScale(), 1.0f, this->attributes)),
		useSkirts(new Variant(OceanEffectComponent::AttrUseSkirts(), true, this->attributes)),

		waveTimeScale(new Variant(OceanEffectComponent::AttrWaveTimeScale(), 1.0f, this->attributes)),
		waveFrequencyScale(new Variant(OceanEffectComponent::AttrWaveFrequencyScale(), 1.0f, this->attributes)),
		waveChaos(new Variant(OceanEffectComponent::AttrWaveChaos(), 0.10f, this->attributes)),

		oceanSize(new Variant(OceanEffectComponent::AttrOceanSize(), Ogre::Vector2(4000.0f, 4000.0f), this->attributes)),
		oceanCenter(new Variant(OceanEffectComponent::AttrOceanCenter(), Ogre::Vector3(0.0f, 0.0f, 0.0f), this->attributes)),

		reflectionStrength(new Variant(OceanEffectComponent::AttrReflectionStrength(), 1.0f, this->attributes)),
		baseRoughness(new Variant(OceanEffectComponent::AttrBaseRoughness(), 0.06f, this->attributes)),
		foamRoughness(new Variant(OceanEffectComponent::AttrFoamRoughness(), 0.35f, this->attributes)),
		ambientReduction(new Variant(OceanEffectComponent::AttrAmbientReduction(), 0.25f, this->attributes)),
		diffuseScale(new Variant(OceanEffectComponent::AttrDiffuseScale(), 1.0f, this->attributes)),
		foamIntensity(new Variant(OceanEffectComponent::AttrFoamIntensity(), 1.0f, this->attributes)),

		oceanComponent(nullptr),
		applyingPreset(false)
	{
		this->presetName->addUserData(GameObject::AttrActionNeedRefresh());

		this->deepColour->addUserData(GameObject::AttrActionColorDialog());
		this->shallowColour->addUserData(GameObject::AttrActionColorDialog());

		this->shaderWavesScale->setConstraints(0.01f, 10.0f);
		this->wavesIntensity->setConstraints(0.0f, 10.0f);
		this->oceanWavesScale->setConstraints(0.01f, 10.0f);

		this->waveTimeScale->setConstraints(0.0f, 10.0f);
		this->waveFrequencyScale->setConstraints(0.0f, 10.0f);
		this->waveChaos->setConstraints(0.0f, 5.0f);

		this->reflectionStrength->setConstraints(0.0f, 5.0f);
		this->baseRoughness->setConstraints(0.0f, 1.0f);
		this->foamRoughness->setConstraints(0.0f, 1.0f);
		this->ambientReduction->setConstraints(0.0f, 5.0f);
		this->diffuseScale->setConstraints(0.0f, 5.0f);
		this->foamIntensity->setConstraints(0.0f, 10.0f);

		this->presetName->setDescription("Select a wave preset. If any parameter is manipulated manually, the preset name is set to 'Custom'.");
		this->brdf->setDescription("BRDF string forwarded to OceanComponent::setBrdf().");
		this->useSkirts->setDescription("Skirts hide LOD cracks. Typically requires ocean rebuild to take effect.");
	}

	OceanEffectComponent::~OceanEffectComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
			"[OceanEffectComponent] Destructor ocean effect component for game object: " + this->gameObjectPtr->getName());

		this->oceanComponent = nullptr;
	}

	void OceanEffectComponent::initialise()
	{
	}

	const Ogre::String& OceanEffectComponent::getName() const
	{
		return this->name;
	}

	void OceanEffectComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<OceanEffectComponent>(
			OceanEffectComponent::getStaticClassId(), OceanEffectComponent::getStaticClassName());
	}

	void OceanEffectComponent::shutdown()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void OceanEffectComponent::uninstall()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void OceanEffectComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool OceanEffectComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PresetName")
		{
			this->presetName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DeepColour")
		{
			this->deepColour->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShallowColour")
		{
			this->shallowColour->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Brdf")
		{
			this->brdf->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShaderWavesScale")
		{
			this->shaderWavesScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WavesIntensity")
		{
			this->wavesIntensity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OceanWavesScale")
		{
			this->oceanWavesScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseSkirts")
		{
			this->useSkirts->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaveTimeScale")
		{
			this->waveTimeScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaveFrequencyScale")
		{
			this->waveFrequencyScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaveChaos")
		{
			this->waveChaos->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OceanSize")
		{
			this->oceanSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OceanCenter")
		{
			this->oceanCenter->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReflectionStrength")
		{
			this->reflectionStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BaseRoughness")
		{
			this->baseRoughness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FoamRoughness")
		{
			this->foamRoughness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AmbientReduction")
		{
			this->ambientReduction->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseScale")
		{
			this->diffuseScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FoamIntensity")
		{
			this->foamIntensity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr OceanEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool OceanEffectComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
			"[OceanEffectComponent] Init ocean effect component for game object: " + this->gameObjectPtr->getName());

		auto& oceanCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<OceanComponent>());
		if (nullptr != oceanCompPtr)
		{
			this->oceanComponent = oceanCompPtr.get();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[OceanEffectComponent] Could not find OceanComponent! Affected game object: " + this->gameObjectPtr->getName());
			return false;
		}

		// Apply loaded preset/values (but do not mark custom while applying the preset)
		this->setPresetName(this->presetName->getListSelectedValue());
		this->applyAllSettingsToOcean();

		return true;
	}

	bool OceanEffectComponent::connect(void)
	{
		GameObjectComponent::connect();

		auto& oceanCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<OceanComponent>());
		if (nullptr != oceanCompPtr)
		{
			this->oceanComponent = oceanCompPtr.get();

			// Apply loaded preset and values
			this->setPresetName(this->presetName->getListSelectedValue());
			this->applyAllSettingsToOcean();
		}
		return true;
	}

	bool OceanEffectComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();
		return true;
	}

	void OceanEffectComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
	}

	void OceanEffectComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& oceanCompPtr = boost::dynamic_pointer_cast<OceanComponent>(gameObjectCompPtr);
			if (nullptr != oceanCompPtr)
			{
				this->oceanComponent = nullptr;
			}
		}
	}

	void OceanEffectComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (OceanEffectComponent::AttrPresetName() == attribute->getName())
		{
			this->setPresetName(attribute->getListSelectedValue());
			// after preset, push everything
			this->applyAllSettingsToOcean();
		}
		else if (OceanEffectComponent::AttrDeepColour() == attribute->getName())
		{
			this->setDeepColour(attribute->getVector3());
		}
		else if (OceanEffectComponent::AttrShallowColour() == attribute->getName())
		{
			this->setShallowColour(attribute->getVector3());
		}
		else if (OceanEffectComponent::AttrBrdf() == attribute->getName())
		{
			this->setBrdf(attribute->getString());
		}
		else if (OceanEffectComponent::AttrShaderWavesScale() == attribute->getName())
		{
			this->setShaderWavesScale(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrWavesIntensity() == attribute->getName())
		{
			this->setWavesIntensity(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrOceanWavesScale() == attribute->getName())
		{
			this->setOceanWavesScale(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrUseSkirts() == attribute->getName())
		{
			this->setUseSkirts(attribute->getBool());
		}
		else if (OceanEffectComponent::AttrWaveTimeScale() == attribute->getName())
		{
			this->setWaveTimeScale(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrWaveFrequencyScale() == attribute->getName())
		{
			this->setWaveFrequencyScale(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrWaveChaos() == attribute->getName())
		{
			this->setWaveChaos(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrOceanSize() == attribute->getName())
		{
			this->setOceanSize(attribute->getVector2());
		}
		else if (OceanEffectComponent::AttrOceanCenter() == attribute->getName())
		{
			this->setOceanCenter(attribute->getVector3());
		}
		else if (OceanEffectComponent::AttrReflectionStrength() == attribute->getName())
		{
			this->setReflectionStrength(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrBaseRoughness() == attribute->getName())
		{
			this->setBaseRoughness(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrFoamRoughness() == attribute->getName())
		{
			this->setFoamRoughness(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrAmbientReduction() == attribute->getName())
		{
			this->setAmbientReduction(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrDiffuseScale() == attribute->getName())
		{
			this->setDiffuseScale(attribute->getReal());
		}
		else if (OceanEffectComponent::AttrFoamIntensity() == attribute->getName())
		{
			this->setFoamIntensity(attribute->getReal());
		}
	}

	void OceanEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

		auto writeB = [&](const char* name, bool b)
			{
				xml_node<>* n = doc.allocate_node(node_element, "property");
				n->append_attribute(doc.allocate_attribute("type", "12"));
				n->append_attribute(doc.allocate_attribute("name", name));
				n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, b)));
				propertiesXML->append_node(n);
			};

		auto writeS = [&](const char* name, const Ogre::String& s)
			{
				xml_node<>* n = doc.allocate_node(node_element, "property");
				n->append_attribute(doc.allocate_attribute("type", "7"));
				n->append_attribute(doc.allocate_attribute("name", name));
				n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, s)));
				propertiesXML->append_node(n);
			};

		writeV3("DeepColour", this->deepColour->getVector3());
		writeV3("ShallowColour", this->shallowColour->getVector3());
		writeS("Brdf", this->brdf->getString());

		writeR("ShaderWavesScale", this->shaderWavesScale->getReal());
		writeR("WavesIntensity", this->wavesIntensity->getReal());
		writeR("OceanWavesScale", this->oceanWavesScale->getReal());
		writeB("UseSkirts", this->useSkirts->getBool());

		writeR("WaveTimeScale", this->waveTimeScale->getReal());
		writeR("WaveFrequencyScale", this->waveFrequencyScale->getReal());
		writeR("WaveChaos", this->waveChaos->getReal());

		writeV2("OceanSize", this->oceanSize->getVector2());
		writeV3("OceanCenter", this->oceanCenter->getVector3());

		writeR("ReflectionStrength", this->reflectionStrength->getReal());
		writeR("BaseRoughness", this->baseRoughness->getReal());
		writeR("FoamRoughness", this->foamRoughness->getReal());
		writeR("AmbientReduction", this->ambientReduction->getReal());
		writeR("DiffuseScale", this->diffuseScale->getReal());
		writeR("FoamIntensity", this->foamIntensity->getReal());
	}

	Ogre::String OceanEffectComponent::getClassName(void) const
	{
		return "OceanEffectComponent";
	}

	Ogre::String OceanEffectComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void OceanEffectComponent::markCustom(void)
	{
		if (true == this->applyingPreset)
		{
			return;
		}
		this->presetName->setListSelectedValue("Custom");
	}

	void OceanEffectComponent::applyAllSettingsToOcean(void)
	{
		if (nullptr == this->oceanComponent)
		{
			return;
		}

		// Forward EVERYTHING to the OceanComponent
		this->oceanComponent->setDeepColour(this->deepColour->getVector3());
		this->oceanComponent->setShallowColour(this->shallowColour->getVector3());
		this->oceanComponent->setBrdf(this->brdf->getString());

		this->oceanComponent->setShaderWavesScale(this->shaderWavesScale->getReal());
		this->oceanComponent->setWavesIntensity(this->wavesIntensity->getReal());
		this->oceanComponent->setOceanWavesScale(this->oceanWavesScale->getReal());
		this->oceanComponent->setUseSkirts(this->useSkirts->getBool());

		this->oceanComponent->setWaveTimeScale(this->waveTimeScale->getReal());
		this->oceanComponent->setWaveFrequencyScale(this->waveFrequencyScale->getReal());
		this->oceanComponent->setWaveChaos(this->waveChaos->getReal());

		this->oceanComponent->setOceanSize(this->oceanSize->getVector2());
		this->oceanComponent->setOceanCenter(this->oceanCenter->getVector3());

		this->oceanComponent->setReflectionStrength(this->reflectionStrength->getReal());
		this->oceanComponent->setBaseRoughness(this->baseRoughness->getReal());
		this->oceanComponent->setFoamRoughness(this->foamRoughness->getReal());
		this->oceanComponent->setAmbientReduction(this->ambientReduction->getReal());
		this->oceanComponent->setDiffuseScale(this->diffuseScale->getReal());
		this->oceanComponent->setFoamIntensity(this->foamIntensity->getReal());
	}

	void OceanEffectComponent::setPresetName(const Ogre::String& presetName)
	{
		this->presetName->setListSelectedValue(presetName);

		if ("Custom" == presetName)
		{
			return;
		}

		// Apply only the wave-related preset example values (as requested).
		this->applyingPreset = true;

		if ("Calm Lake" == presetName)
		{
			this->wavesIntensity->setValue(0.2f);
			this->oceanWavesScale->setValue(1.0f);
			this->shaderWavesScale->setValue(1.0f);
			this->waveTimeScale->setValue(0.55f);
			this->waveFrequencyScale->setValue(0.65f);
			this->waveChaos->setValue(0.03f);
		}
		else if ("Gentle Ocean" == presetName)
		{
			this->wavesIntensity->setValue(0.55f);
			this->oceanWavesScale->setValue(1.0f);
			this->shaderWavesScale->setValue(1.0f);
			this->waveTimeScale->setValue(1.0f);
			this->waveFrequencyScale->setValue(1.0f);
			this->waveChaos->setValue(0.10f);
		}
		else if ("Choppy Water" == presetName)
		{
			this->wavesIntensity->setValue(0.75f);
			this->oceanWavesScale->setValue(0.85f);
			this->shaderWavesScale->setValue(1.15f);
			this->waveTimeScale->setValue(1.25f);
			this->waveFrequencyScale->setValue(1.45f);
			this->waveChaos->setValue(0.30f);
		}
		else if ("Storm Sea" == presetName)
		{
			this->wavesIntensity->setValue(1.35f);
			this->oceanWavesScale->setValue(1.15f);
			this->shaderWavesScale->setValue(1.10f);
			this->waveTimeScale->setValue(1.35f);
			this->waveFrequencyScale->setValue(0.95f);
			this->waveChaos->setValue(0.45f);
		}
		else if ("Big Swell" == presetName)
		{
			this->wavesIntensity->setValue(1.10f);
			this->oceanWavesScale->setValue(1.60f);
			this->shaderWavesScale->setValue(0.85f);
			this->waveTimeScale->setValue(0.75f);
			this->waveFrequencyScale->setValue(0.60f);
			this->waveChaos->setValue(0.12f);
		}
		else if ("Fantasy Water" == presetName)
		{
			this->wavesIntensity->setValue(1.60f);
			this->oceanWavesScale->setValue(1.10f);
			this->shaderWavesScale->setValue(1.30f);
			this->waveTimeScale->setValue(2.10f);
			this->waveFrequencyScale->setValue(1.35f);
			this->waveChaos->setValue(0.65f);
		}

		this->applyingPreset = false;
	}

	Ogre::String OceanEffectComponent::getPresetName(void) const
	{
		return this->presetName->getListSelectedValue();
	}

	// ----- Setters/ getters: each marks Custom and applies to OceanComponent -----

	void OceanEffectComponent::setDeepColour(const Ogre::Vector3& v)
	{
		this->deepColour->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setDeepColour(v);
	}

	Ogre::Vector3 OceanEffectComponent::getDeepColour(void) const
	{
		return this->deepColour->getVector3();
	}

	void OceanEffectComponent::setShallowColour(const Ogre::Vector3& v)
	{
		this->shallowColour->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setShallowColour(v);
	}

	Ogre::Vector3 OceanEffectComponent::getShallowColour(void) const
	{
		return this->shallowColour->getVector3();
	}

	void OceanEffectComponent::setBrdf(const Ogre::String& v)
	{
		this->brdf->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setBrdf(v);
	}

	Ogre::String OceanEffectComponent::getBrdf(void) const
	{
		return this->brdf->getString();
	}

	void OceanEffectComponent::setShaderWavesScale(Ogre::Real v)
	{
		this->shaderWavesScale->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setShaderWavesScale(v);
	}

	Ogre::Real OceanEffectComponent::getShaderWavesScale(void) const
	{
		return this->shaderWavesScale->getReal();
	}

	void OceanEffectComponent::setWavesIntensity(Ogre::Real v)
	{
		this->wavesIntensity->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setWavesIntensity(v);
	}

	Ogre::Real OceanEffectComponent::getWavesIntensity(void) const
	{
		return this->wavesIntensity->getReal();
	}

	void OceanEffectComponent::setOceanWavesScale(Ogre::Real v)
	{
		this->oceanWavesScale->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setOceanWavesScale(v);
	}

	Ogre::Real OceanEffectComponent::getOceanWavesScale(void) const
	{
		return this->oceanWavesScale->getReal();
	}

	void OceanEffectComponent::setUseSkirts(bool v)
	{
		this->useSkirts->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setUseSkirts(v);
	}

	bool OceanEffectComponent::getUseSkirts(void) const
	{
		return this->useSkirts->getBool();
	}

	void OceanEffectComponent::setWaveTimeScale(Ogre::Real v)
	{
		this->waveTimeScale->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setWaveTimeScale(v);
	}

	Ogre::Real OceanEffectComponent::getWaveTimeScale(void) const
	{
		return this->waveTimeScale->getReal();
	}

	void OceanEffectComponent::setWaveFrequencyScale(Ogre::Real v)
	{
		this->waveFrequencyScale->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setWaveFrequencyScale(v);
	}

	Ogre::Real OceanEffectComponent::getWaveFrequencyScale(void) const
	{
		return this->waveFrequencyScale->getReal();
	}

	void OceanEffectComponent::setWaveChaos(Ogre::Real v)
	{
		this->waveChaos->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setWaveChaos(v);
	}

	Ogre::Real OceanEffectComponent::getWaveChaos(void) const
	{
		return this->waveChaos->getReal();
	}

	void OceanEffectComponent::setOceanSize(const Ogre::Vector2& v)
	{
		this->oceanSize->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setOceanSize(v);
	}

	Ogre::Vector2 OceanEffectComponent::getOceanSize(void) const
	{
		return this->oceanSize->getVector2();
	}

	void OceanEffectComponent::setOceanCenter(const Ogre::Vector3& v)
	{
		this->oceanCenter->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setOceanCenter(v);
	}

	Ogre::Vector3 OceanEffectComponent::getOceanCenter(void) const
	{
		return this->oceanCenter->getVector3();
	}

	void OceanEffectComponent::setReflectionStrength(Ogre::Real v)
	{
		this->reflectionStrength->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setReflectionStrength(v);
	}

	Ogre::Real OceanEffectComponent::getReflectionStrength(void) const
	{
		return this->reflectionStrength->getReal();
	}

	void OceanEffectComponent::setBaseRoughness(Ogre::Real v)
	{
		this->baseRoughness->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setBaseRoughness(v);
	}

	Ogre::Real OceanEffectComponent::getBaseRoughness(void) const
	{
		return this->baseRoughness->getReal();
	}

	void OceanEffectComponent::setFoamRoughness(Ogre::Real v)
	{
		this->foamRoughness->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setFoamRoughness(v);
	}

	Ogre::Real OceanEffectComponent::getFoamRoughness(void) const
	{
		return this->foamRoughness->getReal();
	}

	void OceanEffectComponent::setAmbientReduction(Ogre::Real v)
	{
		this->ambientReduction->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setAmbientReduction(v);
	}

	Ogre::Real OceanEffectComponent::getAmbientReduction(void) const
	{
		return this->ambientReduction->getReal();
	}

	void OceanEffectComponent::setDiffuseScale(Ogre::Real v)
	{
		this->diffuseScale->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setDiffuseScale(v);
	}

	Ogre::Real OceanEffectComponent::getDiffuseScale(void) const
	{
		return this->diffuseScale->getReal();
	}

	void OceanEffectComponent::setFoamIntensity(Ogre::Real v)
	{
		this->foamIntensity->setValue(v);
		this->markCustom();
		if (nullptr != this->oceanComponent) this->oceanComponent->setFoamIntensity(v);
	}

	Ogre::Real OceanEffectComponent::getFoamIntensity(void) const
	{
		return this->foamIntensity->getReal();
	}

	bool OceanEffectComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto oceanEffectCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<OceanEffectComponent>());
		auto oceanCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<OceanComponent>());

		// Constraints: Can only be placed on an object with OceanComponent and only once
		if (nullptr != oceanCompPtr && nullptr == oceanEffectCompPtr)
		{
			return true;
		}
		return false;
	}

	// Lua registration part

	OceanEffectComponent* getOceanEffectComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<OceanEffectComponent>(gameObject->getComponentWithOccurrence<OceanEffectComponent>(occurrenceIndex)).get();
	}

	OceanEffectComponent* getOceanEffectComponent(GameObject* gameObject)
	{
		return makeStrongPtr<OceanEffectComponent>(gameObject->getComponent<OceanEffectComponent>()).get();
	}

	OceanEffectComponent* getOceanEffectComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<OceanEffectComponent>(gameObject->getComponentFromName<OceanEffectComponent>(name)).get();
	}

	void OceanEffectComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<OceanEffectComponent, GameObjectComponent>("OceanEffectComponent")
			.def("setPresetName", &OceanEffectComponent::setPresetName)
			.def("getPresetName", &OceanEffectComponent::getPresetName)

			.def("setDeepColour", &OceanEffectComponent::setDeepColour)
			.def("getDeepColour", &OceanEffectComponent::getDeepColour)
			.def("setShallowColour", &OceanEffectComponent::setShallowColour)
			.def("getShallowColour", &OceanEffectComponent::getShallowColour)
			.def("setBrdf", &OceanEffectComponent::setBrdf)
			.def("getBrdf", &OceanEffectComponent::getBrdf)

			.def("setShaderWavesScale", &OceanEffectComponent::setShaderWavesScale)
			.def("getShaderWavesScale", &OceanEffectComponent::getShaderWavesScale)
			.def("setWavesIntensity", &OceanEffectComponent::setWavesIntensity)
			.def("getWavesIntensity", &OceanEffectComponent::getWavesIntensity)
			.def("setOceanWavesScale", &OceanEffectComponent::setOceanWavesScale)
			.def("getOceanWavesScale", &OceanEffectComponent::getOceanWavesScale)
			.def("setUseSkirts", &OceanEffectComponent::setUseSkirts)
			.def("getUseSkirts", &OceanEffectComponent::getUseSkirts)

			.def("setWaveTimeScale", &OceanEffectComponent::setWaveTimeScale)
			.def("getWaveTimeScale", &OceanEffectComponent::getWaveTimeScale)
			.def("setWaveFrequencyScale", &OceanEffectComponent::setWaveFrequencyScale)
			.def("getWaveFrequencyScale", &OceanEffectComponent::getWaveFrequencyScale)
			.def("setWaveChaos", &OceanEffectComponent::setWaveChaos)
			.def("getWaveChaos", &OceanEffectComponent::getWaveChaos)

			.def("setOceanSize", &OceanEffectComponent::setOceanSize)
			.def("getOceanSize", &OceanEffectComponent::getOceanSize)
			.def("setOceanCenter", &OceanEffectComponent::setOceanCenter)
			.def("getOceanCenter", &OceanEffectComponent::getOceanCenter)

			.def("setReflectionStrength", &OceanEffectComponent::setReflectionStrength)
			.def("getReflectionStrength", &OceanEffectComponent::getReflectionStrength)
			.def("setBaseRoughness", &OceanEffectComponent::setBaseRoughness)
			.def("getBaseRoughness", &OceanEffectComponent::getBaseRoughness)
			.def("setFoamRoughness", &OceanEffectComponent::setFoamRoughness)
			.def("getFoamRoughness", &OceanEffectComponent::getFoamRoughness)
			.def("setAmbientReduction", &OceanEffectComponent::setAmbientReduction)
			.def("getAmbientReduction", &OceanEffectComponent::getAmbientReduction)
			.def("setDiffuseScale", &OceanEffectComponent::setDiffuseScale)
			.def("getDiffuseScale", &OceanEffectComponent::getDiffuseScale)
			.def("setFoamIntensity", &OceanEffectComponent::setFoamIntensity)
			.def("getFoamIntensity", &OceanEffectComponent::getFoamIntensity)
		];

		gameObjectClass.def("getOceanEffectComponentFromName", &getOceanEffectComponentFromName);
		gameObjectClass.def("getOceanEffectComponent", (OceanEffectComponent * (*)(GameObject*)) & getOceanEffectComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "OceanEffectComponent getOceanEffectComponent()", "Gets the component. This can be used if the game object has this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "OceanEffectComponent getOceanEffectComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castOceanEffectComponent", &GameObjectController::cast<OceanEffectComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "OceanEffectComponent castOceanEffectComponent(OceanEffectComponent other)", "Casts an incoming type from function for lua auto completion.");

		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "class inherits GameObjectComponent", OceanEffectComponent::getStaticInfoText());

		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setPresetName(string presetName)",
			"Sets the ocean wave preset name. Possible values are: "
			"'Calm Lake', 'Gentle Ocean', 'Choppy Water', 'Storm Sea', 'Big Swell', 'Fantasy Water', 'Custom'. "
			"Preset examples: "
			"Calm Lake: WavesIntensity=0.2 OceanWavesScale=1.0 ShaderWavesScale=1.0 WaveTimeScale=0.55 WaveFrequencyScale=0.65 WaveChaos=0.03; "
			"Gentle Ocean: WavesIntensity=0.55 OceanWavesScale=1.0 ShaderWavesScale=1.0 WaveTimeScale=1.0 WaveFrequencyScale=1.0 WaveChaos=0.10; "
			"Choppy Water: WavesIntensity=0.75 OceanWavesScale=0.85 ShaderWavesScale=1.15 WaveTimeScale=1.25 WaveFrequencyScale=1.45 WaveChaos=0.30; "
			"Storm Sea: WavesIntensity=1.35 OceanWavesScale=1.15 ShaderWavesScale=1.10 WaveTimeScale=1.35 WaveFrequencyScale=0.95 WaveChaos=0.45; "
			"Big Swell: WavesIntensity=1.10 OceanWavesScale=1.60 ShaderWavesScale=0.85 WaveTimeScale=0.75 WaveFrequencyScale=0.60 WaveChaos=0.12; "
			"Fantasy Water: WavesIntensity=1.60 OceanWavesScale=1.10 ShaderWavesScale=1.30 WaveTimeScale=2.10 WaveFrequencyScale=1.35 WaveChaos=0.65. "
			"Note: If any value is manipulated manually, the preset name will be set to 'Custom'.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "string getPresetName()", "Gets currently set preset name.");

		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setDeepColour(Vector3 deepColour)", "Sets deep colour for the ocean.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "Vector3 getDeepColour()", "Gets deep colour.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setShallowColour(Vector3 shallowColour)", "Sets shallow colour for the ocean.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "Vector3 getShallowColour()", "Gets shallow colour.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setBrdf(string brdf)", "Sets BRDF string forwarded to OceanComponent::setBrdf().");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "string getBrdf()", "Gets BRDF string.");

		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setShaderWavesScale(float wavesScale)", "Sets shader-side waves scale (datablock waves scale).");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getShaderWavesScale()", "Gets shader-side waves scale.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setWavesIntensity(float intensity)", "Sets waves intensity/amplitude.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getWavesIntensity()", "Gets waves intensity.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setOceanWavesScale(float wavesScale)", "Sets ocean waves scale (tiling/wave size).");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getOceanWavesScale()", "Gets ocean waves scale.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setUseSkirts(bool useSkirts)", "Sets whether skirts are used.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "bool getUseSkirts()", "Gets whether skirts are used.");

		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setWaveTimeScale(float timeScale)", "Sets wave time scale.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getWaveTimeScale()", "Gets wave time scale.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setWaveFrequencyScale(float freqScale)", "Sets wave frequency scale.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getWaveFrequencyScale()", "Gets wave frequency scale.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setWaveChaos(float chaos)", "Sets wave chaos.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getWaveChaos()", "Gets wave chaos.");

		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setOceanSize(Vector2 size)", "Sets ocean size.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "Vector2 getOceanSize()", "Gets ocean size.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setOceanCenter(Vector3 center)", "Sets ocean center.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "Vector3 getOceanCenter()", "Gets ocean center.");

		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setReflectionStrength(float v)", "Sets reflection strength.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getReflectionStrength()", "Gets reflection strength.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setBaseRoughness(float v)", "Sets base roughness.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getBaseRoughness()", "Gets base roughness.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setFoamRoughness(float v)", "Sets foam roughness.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getFoamRoughness()", "Gets foam roughness.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setAmbientReduction(float v)", "Sets ambient reduction.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getAmbientReduction()", "Gets ambient reduction.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setDiffuseScale(float v)", "Sets diffuse scale.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getDiffuseScale()", "Gets diffuse scale.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "void setFoamIntensity(float v)", "Sets foam intensity.");
		LuaScriptApi::getInstance()->addClassToCollection("OceanEffectComponent", "float getFoamIntensity()", "Gets foam intensity.");
	}

}; //namespace end