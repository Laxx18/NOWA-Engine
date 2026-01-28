/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ParticleFxComponent.h"
#include "gameobject/GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"
#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"
#include "OgreSceneManager.h"
#include "OgreResourceGroupManager.h"
#include "ParticleSystem/OgreParticleSystemManager2.h"

#include <fstream>
#include <sstream>

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ParticleFxComponent::ParticleFxComponent()
		: GameObjectComponent(),
		name("ParticleFxComponent"),
		uniqueParticleName(""),
		particleFxModule(nullptr),
		oldActivated(true),
		activated(new Variant(ParticleFxComponent::AttrActivated(), true, this->attributes)),
		particleTemplateName(new Variant(ParticleFxComponent::AttrParticleName(), std::vector<Ogre::String>(), this->attributes)),
		repeat(new Variant(ParticleFxComponent::AttrRepeat(), false, this->attributes)),
		particleInitialPlayTime(new Variant(ParticleFxComponent::AttrPlayTime(), 10000.0f, this->attributes)),
		particlePlaySpeed(new Variant(ParticleFxComponent::AttrPlaySpeed(), 1.0f, this->attributes)),
		particleOffsetPosition(new Variant(ParticleFxComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		particleOffsetOrientation(new Variant(ParticleFxComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		particleScale(new Variant(ParticleFxComponent::AttrScale(), Ogre::Vector2::UNIT_SCALE, this->attributes)),
		blendingMethod(new Variant(ParticleFxComponent::AttrBlendingMethod(), { "Alpha Hashing", "Alpha Hashing + A2C", "Alpha Blending", "Alpha Transparent", "Transparent Colour", "From Material"}, this->attributes)),
		fadeIn(new Variant(ParticleFxComponent::AttrFadeIn(), false, this->attributes)),
		fadeInTime(new Variant(ParticleFxComponent::AttrFadeInTime(), 1000.0f, this->attributes)),
		fadeOut(new Variant(ParticleFxComponent::AttrFadeOut(), false, this->attributes)),
		fadeOutTime(new Variant(ParticleFxComponent::AttrFadeOutTime(), 1000.0f, this->attributes))
	{
		// Activated variable is set to false again, when particle has played and repeat is off
		this->activated->addUserData(GameObject::AttrActionNeedRefresh());
		this->particleInitialPlayTime->setDescription("The particle play time in milliseconds, if set to 0, the particle effect will not stop automatically.");
		this->particlePlaySpeed->setDescription("The particle flow speed multiplier. Values > 1.0 make particles flow faster, < 1.0 slower. Also affects play time countdown.");
		this->blendingMethod->setListSelectedValue("From Material");
		this->blendingMethod->setDescription("Alpha Hashing: Fast, no sorting needed. Alpha Hashing + A2C: Best quality with MSAA. Alpha Blending: Traditional transparency. From Material: A lookup in the corresponding is made for the configured blending method.");
		this->fadeIn->setDescription("If enabled, the particle effect will gradually increase emission rate from 0 to max when starting.");
		this->fadeInTime->setDescription("The fade in duration in milliseconds.");
		this->fadeOut->setDescription("If enabled, the particle effect will gradually decrease emission rate to 0 before stopping.");
		this->fadeOutTime->setDescription("The fade out duration in milliseconds.");
	}

	ParticleFxComponent::~ParticleFxComponent(void)
	{
	}

	void ParticleFxComponent::initialise()
	{
		// Plugin initialise - called when plugin is installed
	}

	const Ogre::String& ParticleFxComponent::getName() const
	{
		return this->name;
	}

	void ParticleFxComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ParticleFxComponent>(ParticleFxComponent::getStaticClassId(), ParticleFxComponent::getStaticClassName());
	}

	void ParticleFxComponent::shutdown()
	{
		// Plugin shutdown
	}

	void ParticleFxComponent::uninstall()
	{
		// Plugin uninstall
	}

	void ParticleFxComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	void ParticleFxComponent::gatherParticleNames(void)
	{
		const Ogre::String prevSelected = this->particleTemplateName->getListSelectedValue();

		std::vector<Ogre::String> names;
		names.reserve(64u);
		names.emplace_back("");

		Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
		Ogre::ParticleSystemManager2* particleManager = sceneManager->getParticleSystemManager2();

		if (nullptr == particleManager)
		{
			return;
		}

		Ogre::ResourceGroupManager& resourceGroupManager = Ogre::ResourceGroupManager::getSingleton();

		if (false == resourceGroupManager.resourceGroupExists("ParticleFX2"))
		{
			return;
		}

		Ogre::StringVectorPtr scripts = resourceGroupManager.findResourceNames("ParticleFX2", "*.particle2");

		for (const Ogre::String& scriptName : *scripts)
		{
			try
			{
				Ogre::DataStreamPtr stream = resourceGroupManager.openResource(scriptName, "ParticleFX2");
				if (!stream)
				{
					continue;
				}

				Ogre::String origin = scriptName;

				std::istringstream iss(stream->getAsString());
				std::string line;

				while (std::getline(iss, line))
				{
					size_t start = line.find_first_not_of(" \t");
					if (start == std::string::npos)
					{
						continue;
					}

					std::string trimmed = line.substr(start);

					if (!Ogre::StringUtil::startsWith(trimmed, "particle_system", false))
					{
						continue;
					}

					size_t nameStart = trimmed.find_first_not_of(" \t", 15);
					if (nameStart == std::string::npos)
					{
						continue;
					}

					std::string systemName = trimmed.substr(nameStart);
					size_t inheritPos = systemName.find(':');
					if (inheritPos != std::string::npos)
					{
						systemName = systemName.substr(0, inheritPos);
					}

					Ogre::StringUtil::trim(systemName);

					if (!particleManager->hasParticleSystemDef(systemName))
					{
						continue;
					}

					Ogre::ParticleSystemDef* particleSystemDef = particleManager->getParticleSystemDef(systemName);

					if (particleSystemDef->getOrigin() != origin)
					{
						continue;
					}

					if (std::find(names.begin(), names.end(), systemName) == names.end())
					{
						names.emplace_back(systemName);
					}
				}
			}
			catch (...)
			{
			}
		}

		if (names.size() > 2u)
		{
			std::sort(names.begin() + 1u, names.end());
		}

		this->particleTemplateName->setValue(names);

		if (!prevSelected.empty() && std::find(names.begin(), names.end(), prevSelected) != names.end())
		{
			this->particleTemplateName->setListSelectedValue(prevSelected);
		}
		else if (names.size() > 1u)
		{
			this->particleTemplateName->setListSelectedValue(names[1]);
		}
	}

	Ogre::String ParticleFxComponent::getUniqueParticleName(void) const
	{
		return "ParticleFxComp_" + this->gameObjectPtr->getName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(this->index);
	}

	bool ParticleFxComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TemplateName")
		{
			this->particleTemplateName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlayTime")
		{
			this->particleInitialPlayTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10000.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlaySpeed")
		{
			this->particlePlaySpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->particleOffsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
		{
			this->particleOffsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Scale")
		{
			this->particleScale->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendingMethod")
		{
			this->blendingMethod->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Alpha Hashing + A2C"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeIn")
		{
			this->fadeIn->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeInTime")
		{
			this->fadeInTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1000.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeOut")
		{
			this->fadeOut->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeOutTime")
		{
			this->fadeOutTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1000.0f));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr ParticleFxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ParticleFxComponentPtr clonedCompPtr(boost::make_shared<ParticleFxComponent>());

		clonedCompPtr->setParticleTemplateName(this->particleTemplateName->getListSelectedValue());
		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setParticlePlayTimeMS(this->particleInitialPlayTime->getReal());
		clonedCompPtr->setParticlePlaySpeed(this->particlePlaySpeed->getReal());
		clonedCompPtr->setParticleOffsetPosition(this->particleOffsetPosition->getVector3());
		clonedCompPtr->setParticleOffsetOrientation(this->particleOffsetOrientation->getVector3());
		clonedCompPtr->setParticleScale(this->particleScale->getVector2());
		clonedCompPtr->setBlendingMethod(this->getBlendingMethod());
		clonedCompPtr->setFadeIn(this->fadeIn->getBool());
		clonedCompPtr->setFadeInTimeMS(this->fadeInTime->getReal());
		clonedCompPtr->setFadeOut(this->fadeOut->getBool());
		clonedCompPtr->setFadeOutTimeMS(this->fadeOutTime->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ParticleFxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Init particle fx component for game object: " + this->gameObjectPtr->getName());

		// Get the module from AppStateManager (assuming it's available there)
		// You'll need to implement getParticleFxModule() in your AppState
		this->particleFxModule = AppStateManager::getSingletonPtr()->getParticleFxModule();

		if (nullptr == this->particleFxModule)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxComponent] Error: ParticleFxModule not available!");
			return false;
		}

		// Refresh the particle names list
		this->gatherParticleNames();

		if (true == this->particleTemplateName->getListSelectedValue().empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Warning: No particle template selected for game object: " + this->gameObjectPtr->getName());
			return true;
		}

		// Particle effect is moving, so it must always be dynamic
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		// Generate unique name for this particle instance
		this->uniqueParticleName = this->getUniqueParticleName();

		return true;
	}

	bool ParticleFxComponent::connect(void)
	{
		GameObjectComponent::connect();

		this->oldActivated = this->activated->getBool();

		if (false == this->particleTemplateName->getListSelectedValue().empty())
		{
			// Create the particle in the module
			this->recreateParticle();

			if (true == this->activated->getBool())
			{
				this->particleFxModule->playParticleSystem(this->uniqueParticleName);
			}
		}

		return true;
	}

	bool ParticleFxComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		// Remove the particle from the module
		if (false == this->uniqueParticleName.empty() && nullptr != this->particleFxModule)
		{
			this->particleFxModule->removeParticle(this->uniqueParticleName);
		}

		// Restore editor state
		this->activated->setValue(this->oldActivated);

		return true;
	}

	bool ParticleFxComponent::onCloned(void)
	{
		// Generate a new unique name for the cloned component
		this->uniqueParticleName = this->getUniqueParticleName();
		return true;
	}

	void ParticleFxComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Removing particle fx component for game object: " + this->gameObjectPtr->getName());

		// Remove the particle from the module
		if (false == this->uniqueParticleName.empty() && nullptr != this->particleFxModule)
		{
			this->particleFxModule->removeParticle(this->uniqueParticleName);
		}
	}

	void ParticleFxComponent::onOtherComponentRemoved(unsigned int index)
	{
		// Nothing special to do
	}

	void ParticleFxComponent::onOtherComponentAdded(unsigned int index)
	{
		// Nothing special to do
	}

	void ParticleFxComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (notSimulating || nullptr == this->particleFxModule)
		{
			return;
		}

		// Update the particle node position to follow the GameObject
		this->updateParticleTransform();

		// Check if particle has finished playing (when not repeating)
		if (false == this->repeat->getBool())
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (nullptr != particleData && false == particleData->activated && this->activated->getBool())
			{
				// Particle has finished, update our activated state
				this->activated->setValue(false);
			}
		}
	}

	void ParticleFxComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ParticleFxComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ParticleFxComponent::AttrParticleName() == attribute->getName())
		{
			this->setParticleTemplateName(attribute->getListSelectedValue());
		}
		else if (ParticleFxComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (ParticleFxComponent::AttrPlayTime() == attribute->getName())
		{
			this->setParticlePlayTimeMS(attribute->getReal());
		}
		else if (ParticleFxComponent::AttrPlaySpeed() == attribute->getName())
		{
			this->setParticlePlaySpeed(attribute->getReal());
		}
		else if (ParticleFxComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setParticleOffsetPosition(attribute->getVector3());
		}
		else if (ParticleFxComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setParticleOffsetOrientation(attribute->getVector3());
		}
		else if (ParticleFxComponent::AttrScale() == attribute->getName())
		{
			this->setParticleScale(attribute->getVector2());
		}
		else if (ParticleFxComponent::AttrBlendingMethod() == attribute->getName())
		{
			Ogre::String selectedValue = attribute->getListSelectedValue();
			if (selectedValue == "Alpha Hashing")
				this->setBlendingMethod(ParticleBlendingMethod::AlphaHashing);
			else if (selectedValue == "Alpha Hashing + A2C")
				this->setBlendingMethod(ParticleBlendingMethod::AlphaHashingA2C);
			else if (selectedValue == "Alpha Blending")
				this->setBlendingMethod(ParticleBlendingMethod::AlphaBlending);
			else if (selectedValue == "Alpha Transparent")
				this->setBlendingMethod(ParticleBlendingMethod::AlphaTransparent);
			else if (selectedValue == "Transparent Colour")
				this->setBlendingMethod(ParticleBlendingMethod::TransparentColour);
			else if (selectedValue == "From Material")
				this->setBlendingMethod(ParticleBlendingMethod::FromMaterial);
		}
		else if (ParticleFxComponent::AttrFadeIn() == attribute->getName())
		{
			this->setFadeIn(attribute->getBool());
		}
		else if (ParticleFxComponent::AttrFadeInTime() == attribute->getName())
		{
			this->setFadeInTimeMS(attribute->getReal());
		}
		else if (ParticleFxComponent::AttrFadeOut() == attribute->getName())
		{
			this->setFadeOut(attribute->getBool());
		}
		else if (ParticleFxComponent::AttrFadeOutTime() == attribute->getName())
		{
			this->setFadeOutTimeMS(attribute->getReal());
		}
	}

	void ParticleFxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TemplateName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleTemplateName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PlayTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleInitialPlayTime->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PlaySpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particlePlaySpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleOffsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleOffsetOrientation->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Scale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleScale->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendingMethod"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blendingMethod->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeIn"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeIn->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeInTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeInTime->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeOut"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeOut->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeOutTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeOutTime->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String ParticleFxComponent::getClassName(void) const
	{
		return "ParticleFxComponent";
	}

	Ogre::String ParticleFxComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void ParticleFxComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (false == this->bConnected || nullptr == this->particleFxModule)
		{
			return;
		}

		if (activated)
		{
			this->particleFxModule->playParticleSystem(this->uniqueParticleName);
		}
		else
		{
			this->particleFxModule->stopParticleSystem(this->uniqueParticleName);
		}
	}

	bool ParticleFxComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void ParticleFxComponent::setParticleTemplateName(const Ogre::String& particleTemplateName)
	{
		this->particleTemplateName->setListSelectedValue(particleTemplateName);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			this->recreateParticle();
		}
	}

	Ogre::String ParticleFxComponent::getParticleTemplateName(void) const
	{
		return this->particleTemplateName->getListSelectedValue();
	}

	void ParticleFxComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				particleData->repeat = repeat;
			}
		}
	}

	bool ParticleFxComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void ParticleFxComponent::setParticlePlayTimeMS(Ogre::Real playTime)
	{
		this->particleInitialPlayTime->setValue(playTime);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				particleData->particleInitialPlayTime = playTime;
				particleData->particlePlayTime = playTime;
			}
		}
	}

	Ogre::Real ParticleFxComponent::getParticlePlayTimeMS(void) const
	{
		return this->particleInitialPlayTime->getReal();
	}

	void ParticleFxComponent::setParticlePlaySpeed(Ogre::Real playSpeed)
	{
		this->particlePlaySpeed->setValue(playSpeed);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			this->particleFxModule->setPlaySpeed(this->uniqueParticleName, playSpeed);
		}
	}

	Ogre::Real ParticleFxComponent::getParticlePlaySpeed(void) const
	{
		return this->particlePlaySpeed->getReal();
	}

	void ParticleFxComponent::setParticleOffsetPosition(const Ogre::Vector3& particleOffsetPosition)
	{
		this->particleOffsetPosition->setValue(particleOffsetPosition);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			this->updateParticleTransform();
		}
	}

	Ogre::Vector3 ParticleFxComponent::getParticleOffsetPosition(void) const
	{
		return this->particleOffsetPosition->getVector3();
	}

	void ParticleFxComponent::setParticleOffsetOrientation(const Ogre::Vector3& particleOffsetOrientation)
	{
		this->particleOffsetOrientation->setValue(particleOffsetOrientation);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			this->updateParticleTransform();
		}
	}

	Ogre::Vector3 ParticleFxComponent::getParticleOffsetOrientation(void) const
	{
		return this->particleOffsetOrientation->getVector3();
	}

	void ParticleFxComponent::setParticleScale(const Ogre::Vector2& scale)
	{
		this->particleScale->setValue(scale);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			this->particleFxModule->setScale(this->uniqueParticleName, scale);
		}
	}

	Ogre::Vector2 ParticleFxComponent::getParticleScale(void) const
	{
		return this->particleScale->getVector2();
	}

	Ogre::ParticleSystem2* ParticleFxComponent::getParticleSystem(void) const
	{
		if (nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				return particleData->particleSystem;
			}
		}
		return nullptr;
	}

	bool ParticleFxComponent::isPlaying(void) const
	{
		if (nullptr != this->particleFxModule)
		{
			return this->particleFxModule->isPlaying(this->uniqueParticleName);
		}
		return false;
	}

	void ParticleFxComponent::setGlobalPosition(const Ogre::Vector3& particlePosition)
	{
		if (nullptr != this->particleFxModule)
		{
			this->particleFxModule->setGlobalPosition(this->uniqueParticleName, particlePosition);
		}
	}

	void ParticleFxComponent::setGlobalOrientation(const Ogre::Vector3& particleOrientation)
	{
		if (nullptr != this->particleFxModule)
		{
			Ogre::Quaternion quat = MathHelper::getInstance()->degreesToQuat(particleOrientation);
			this->particleFxModule->setGlobalOrientation(this->uniqueParticleName, quat);
		}
	}

	const Ogre::ParticleSystemDef* ParticleFxComponent::getParticleSystemDef(void) const
	{
		Ogre::ParticleSystem2* ps = this->getParticleSystem();
		if (nullptr != ps)
		{
			return ps->getParticleSystemDef();
		}
		return nullptr;
	}

	size_t ParticleFxComponent::getNumActiveParticles(void) const
	{
		if (nullptr != this->particleFxModule)
		{
			return this->particleFxModule->getNumActiveParticles(this->uniqueParticleName);
		}
		return 0;
	}

	Ogre::uint32 ParticleFxComponent::getParticleQuota(void) const
	{
		const Ogre::ParticleSystemDef* particleSystemDef = this->getParticleSystemDef();
		if (nullptr != particleSystemDef)
		{
			return particleSystemDef->getQuota();
		}
		return 0;
	}

	size_t ParticleFxComponent::getNumEmitters(void) const
	{
		const Ogre::ParticleSystemDef* particleSystemDef = this->getParticleSystemDef();
		if (nullptr != particleSystemDef)
		{
			return particleSystemDef->getNumEmitters();
		}
		return 0;
	}

	size_t ParticleFxComponent::getNumAffectors(void) const
	{
		const Ogre::ParticleSystemDef* particleSystemDef = this->getParticleSystemDef();
		if (nullptr != particleSystemDef)
		{
			return particleSystemDef->getNumAffectors();
		}
		return 0;
	}

	void ParticleFxComponent::setBlendingMethod(ParticleBlendingMethod::ParticleBlendingMethod blendingMethod)
	{
		switch (blendingMethod)
		{
		case ParticleBlendingMethod::AlphaHashing:
			this->blendingMethod->setListSelectedValue("Alpha Hashing");
			break;
		case ParticleBlendingMethod::AlphaHashingA2C:
			this->blendingMethod->setListSelectedValue("Alpha Hashing + A2C");
			break;
		case ParticleBlendingMethod::AlphaBlending:
			this->blendingMethod->setListSelectedValue("Alpha Blending");
			break;
		case ParticleBlendingMethod::AlphaTransparent:
			this->blendingMethod->setListSelectedValue("Alpha Transparent");
			break;
		case ParticleBlendingMethod::TransparentColour:
			this->blendingMethod->setListSelectedValue("Transparent Colour");
			break;
		case ParticleBlendingMethod::FromMaterial:
			this->blendingMethod->setListSelectedValue("From Material");
			break;
		}

		// is done in createPartcleSystem
		/*if (this->bConnected && nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				particleData->blendingMethod = blendingMethod;
				this->particleFxModule->applyBlendingMethod(*particleData);
			}
		}*/
	}

	ParticleBlendingMethod::ParticleBlendingMethod ParticleFxComponent::getBlendingMethod(void) const
	{
		Ogre::String selectedValue = this->blendingMethod->getListSelectedValue();
		if (selectedValue == "Alpha Hashing")
		{
			return ParticleBlendingMethod::AlphaHashing;
		}
		else if (selectedValue == "Alpha Hashing + A2C")
		{
			return ParticleBlendingMethod::AlphaHashingA2C;
		}
		else if (selectedValue == "Alpha Blending")
		{
			return ParticleBlendingMethod::AlphaBlending;
		}
		else if (selectedValue == "Alpha Transparent")
		{
			return ParticleBlendingMethod::AlphaTransparent;
		}
		else if (selectedValue == "Transparent Colour")
		{
			return ParticleBlendingMethod::TransparentColour;
		}
		else if (selectedValue == "From Material")
		{
			return ParticleBlendingMethod::FromMaterial;
		}
		else
		{
			return ParticleBlendingMethod::AlphaBlending;  // Default
		}
	}

	void ParticleFxComponent::setFadeIn(bool fadeIn)
	{
		this->fadeIn->setValue(fadeIn);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				particleData->fadeIn = fadeIn;
			}
		}
	}

	bool ParticleFxComponent::getFadeIn(void) const
	{
		return this->fadeIn->getBool();
	}

	void ParticleFxComponent::setFadeInTimeMS(Ogre::Real fadeInTimeMS)
	{
		this->fadeInTime->setValue(fadeInTimeMS);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				particleData->fadeInTimeMS = fadeInTimeMS;
			}
		}
	}

	Ogre::Real ParticleFxComponent::getFadeInTimeMS(void) const
	{
		return this->fadeInTime->getReal();
	}

	void ParticleFxComponent::setFadeOut(bool fadeOut)
	{
		this->fadeOut->setValue(fadeOut);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				particleData->fadeOut = fadeOut;
			}
		}
	}

	bool ParticleFxComponent::getFadeOut(void) const
	{
		return this->fadeOut->getBool();
	}

	void ParticleFxComponent::setFadeOutTimeMS(Ogre::Real fadeOutTimeMS)
	{
		this->fadeOutTime->setValue(fadeOutTimeMS);

		if (this->bConnected && nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				particleData->fadeOutTimeMS = fadeOutTimeMS;
			}
		}
	}

	Ogre::Real ParticleFxComponent::getFadeOutTimeMS(void) const
	{
		return this->fadeOutTime->getReal();
	}

	ParticleFadeState::ParticleFadeState ParticleFxComponent::getFadeState(void) const
	{
		if (nullptr != this->particleFxModule)
		{
			ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
			if (particleData)
			{
				return particleData->fadeState;
			}
		}
		return ParticleFadeState::None;
	}

	void ParticleFxComponent::updateParticleTransform(void)
	{
		if (nullptr == this->particleFxModule || nullptr == this->gameObjectPtr)
		{
			return;
		}

		ParticleFxData* particleData = this->particleFxModule->getParticle(this->uniqueParticleName);
		if (nullptr == particleData || nullptr == particleData->particleNode)
		{
			return;
		}

		// Calculate world position
		Ogre::Vector3 worldPos = this->gameObjectPtr->getSceneNode()->_getDerivedPosition() + this->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * this->particleOffsetPosition->getVector3();

		// Calculate world orientation
		Ogre::Quaternion worldOrient = this->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * MathHelper::getInstance()->degreesToQuat(this->particleOffsetOrientation->getVector3());

		this->particleFxModule->setGlobalPosition(this->uniqueParticleName, worldPos);
		this->particleFxModule->setGlobalOrientation(this->uniqueParticleName, worldOrient);
	}

	void ParticleFxComponent::recreateParticle(void)
	{
		if (nullptr == this->particleFxModule)
		{
			return;
		}

		const Ogre::String& templateName = this->particleTemplateName->getListSelectedValue();
		if (true == templateName.empty())
		{
			return;
		}

		// Remember if it was playing
		bool wasPlaying = this->activated->getBool();

		// Remove old particle if exists
		this->particleFxModule->removeParticle(this->uniqueParticleName);

		// Create new particle with current settings
		Ogre::Quaternion orientation = MathHelper::getInstance()->degreesToQuat(this->particleOffsetOrientation->getVector3());

		this->particleFxModule->createParticleSystem(
			this->uniqueParticleName,
			templateName,
			this->particleInitialPlayTime->getReal(),
			orientation,
			this->particleOffsetPosition->getVector3(),
			this->particleScale->getVector2(),
			this->repeat->getBool(),
			this->particlePlaySpeed->getReal(),
			this->getBlendingMethod(),
			this->fadeIn->getBool(),
			this->fadeInTime->getReal(),
			this->fadeOut->getBool(),
			this->fadeOutTime->getReal()
		);

		// Update transform to match GameObject
		this->updateParticleTransform();

		// Restore playing state
		if (wasPlaying)
		{
			this->particleFxModule->playParticleSystem(this->uniqueParticleName);
		}
	}

	// ========== LUA API ==========

	ParticleFxComponent* getParticleFxComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<ParticleFxComponent>(gameObject->getComponentWithOccurrence<ParticleFxComponent>(occurrenceIndex)).get();
	}

	ParticleFxComponent* getParticleFxComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ParticleFxComponent>(gameObject->getComponent<ParticleFxComponent>()).get();
	}

	ParticleFxComponent* getParticleFxComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ParticleFxComponent>(gameObject->getComponentFromName<ParticleFxComponent>(name)).get();
	}

	void ParticleFxComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
			[
				class_<ParticleFxComponent, GameObjectComponent>("ParticleFxComponent")
					.def("setActivated", &ParticleFxComponent::setActivated)
					.def("isActivated", &ParticleFxComponent::isActivated)
					.def("setParticleTemplateName", &ParticleFxComponent::setParticleTemplateName)
					.def("getParticleTemplateName", &ParticleFxComponent::getParticleTemplateName)
					.def("setRepeat", &ParticleFxComponent::setRepeat)
					.def("getRepeat", &ParticleFxComponent::getRepeat)
					.def("setParticlePlayTimeMS", &ParticleFxComponent::setParticlePlayTimeMS)
					.def("getParticlePlayTimeMS", &ParticleFxComponent::getParticlePlayTimeMS)
					.def("setParticlePlaySpeed", &ParticleFxComponent::setParticlePlaySpeed)
					.def("getParticlePlaySpeed", &ParticleFxComponent::getParticlePlaySpeed)
					.def("setParticleOffsetPosition", &ParticleFxComponent::setParticleOffsetPosition)
					.def("getParticleOffsetPosition", &ParticleFxComponent::getParticleOffsetPosition)
					.def("setParticleOffsetOrientation", &ParticleFxComponent::setParticleOffsetOrientation)
					.def("getParticleOffsetOrientation", &ParticleFxComponent::getParticleOffsetOrientation)
					.def("setParticleScale", &ParticleFxComponent::setParticleScale)
					.def("getParticleScale", &ParticleFxComponent::getParticleScale)
					.def("isPlaying", &ParticleFxComponent::isPlaying)
					.def("setGlobalPosition", &ParticleFxComponent::setGlobalPosition)
					.def("setGlobalOrientation", &ParticleFxComponent::setGlobalOrientation)
					.def("getNumActiveParticles", &ParticleFxComponent::getNumActiveParticles)
					.def("getParticleQuota", &ParticleFxComponent::getParticleQuota)
					.def("getNumEmitters", &ParticleFxComponent::getNumEmitters)
					.def("getNumAffectors", &ParticleFxComponent::getNumAffectors)
					.def("setBlendingMethod", &ParticleFxComponent::setBlendingMethod)
					.def("getBlendingMethod", &ParticleFxComponent::getBlendingMethod)
					.def("setFadeIn", &ParticleFxComponent::setFadeIn)
					.def("getFadeIn", &ParticleFxComponent::getFadeIn)
					.def("setFadeInTimeMS", &ParticleFxComponent::setFadeInTimeMS)
					.def("getFadeInTimeMS", &ParticleFxComponent::getFadeInTimeMS)
					.def("setFadeOut", &ParticleFxComponent::setFadeOut)
					.def("getFadeOut", &ParticleFxComponent::getFadeOut)
					.def("setFadeOutTimeMS", &ParticleFxComponent::setFadeOutTimeMS)
					.def("getFadeOutTimeMS", &ParticleFxComponent::getFadeOutTimeMS)
					.def("getFadeState", &ParticleFxComponent::getFadeState)

					.enum_("ParticleBlendingMethod")
					[
						luabind::value("AlphaHashing", ParticleBlendingMethod::AlphaHashing),
						luabind::value("AlphaHashingA2C", ParticleBlendingMethod::AlphaHashingA2C),
						luabind::value("AlphaBlending", ParticleBlendingMethod::AlphaBlending),
						luabind::value("AlphaTransparent", ParticleBlendingMethod::AlphaTransparent),
						luabind::value("FromMaterial", ParticleBlendingMethod::FromMaterial)
					]
			];

		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "class inherits GameObjectComponent", ParticleFxComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleTemplateName(String templateName)", "Sets the particle template name (without 'Particle/' prefix).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "String getParticleTemplateName()", "Gets the particle template name.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setRepeat(bool repeat)", "Sets whether the particle effect should repeat.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool getRepeat()", "Gets whether the particle effect repeats.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticlePlayTimeMS(number playTime)", "Sets the particle play time in milliseconds (0 = infinite).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getParticlePlayTimeMS()", "Gets the particle play time in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticlePlaySpeed(number playSpeed)", "Sets the particle play speed multiplier (1.0 = normal).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getParticlePlaySpeed()", "Gets the particle play speed multiplier.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleOffsetPosition(Vector3 position)", "Sets the offset position relative to the game object.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "Vector3 getParticleOffsetPosition()", "Gets the offset position.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleOffsetOrientation(Vector3 orientation)", "Sets the offset orientation in degrees.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "Vector3 getParticleOffsetOrientation()", "Gets the offset orientation in degrees.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleScale(Vector2 scale)", "Sets the particle scale.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "Vector2 getParticleScale()", "Gets the particle scale.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool isPlaying()", "Checks if the particle effect is currently playing/emitting.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setGlobalPosition(Vector3 position)", "Sets the global world position for the particle effect.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setGlobalOrientation(Vector3 orientation)", "Sets the global orientation in degrees.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumActiveParticles()", "Gets the current number of active particles.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getParticleQuota()", "Gets the particle quota (maximum particles allowed).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumEmitters()", "Gets the number of emitters in the particle system.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumAffectors()", "Gets the number of affectors in the particle system.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setBlendingMethod(ParticleBlendingMethod method)", "Sets the blending method (AlphaHashing=0, AlphaHashingA2C=1, AlphaBlending=2, AlphaTransparent=3, FromMaterial=4).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "ParticleBlendingMethod getBlendingMethod()", "Gets the current blending method.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setFadeIn(bool fadeIn)", "Sets whether the particle effect should fade in when starting.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool getFadeIn()", "Gets whether the particle effect fades in when starting.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setFadeInTimeMS(number fadeInTime)", "Sets the fade in duration in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getFadeInTimeMS()", "Gets the fade in duration in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setFadeOut(bool fadeOut)", "Sets whether the particle effect should fade out when stopping.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool getFadeOut()", "Gets whether the particle effect fades out when stopping.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setFadeOutTimeMS(number fadeOutTime)", "Sets the fade out duration in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getFadeOutTimeMS()", "Gets the fade out duration in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "ParticleFadeState getFadeState()", "Gets the current fade state (None=0, FadingIn=1, FadingOut=2).");

		gameObjectClass.def("getParticleFxComponentFromName", &getParticleFxComponentFromName);
		gameObjectClass.def("getParticleFxComponent", (ParticleFxComponent * (*)(GameObject*)) & getParticleFxComponent);
		gameObjectClass.def("getParticleFxComponentFromIndex", (ParticleFxComponent * (*)(GameObject*, unsigned int)) & getParticleFxComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castParticleFxComponent", &GameObjectController::cast<ParticleFxComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ParticleFxComponent castParticleFxComponent(ParticleFxComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool ParticleFxComponent::canStaticAddComponent(GameObject* gameObject)
	{
		return true;
	}

}; // namespace end
