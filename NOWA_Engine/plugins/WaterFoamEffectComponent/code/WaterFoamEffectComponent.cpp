#include "NOWAPrecompiled.h"
#include "WaterFoamEffectComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "modules/ParticleFxModule.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	WaterFoamEffectComponent::WaterFoamEffectComponent()
		: GameObjectComponent(),
		name("WaterFoamEffectComponent"),
		oceanComponent(nullptr),
		postInitDone(false),
		effectActive(false),
		oceanId(new Variant(WaterFoamEffectComponent::AttrOceanId(), static_cast<unsigned long>(0), this->attributes, true)),
		particleTemplateName(new Variant(WaterFoamEffectComponent::AttrParticleTemplateName(), Ogre::String("wake"), this->attributes)),
		particleScale(new Variant(WaterFoamEffectComponent::AttrParticleScale(), 0.05f, this->attributes)),
		playTimeMs(new Variant(WaterFoamEffectComponent::AttrPlayTimeMs(), 0.0f, this->attributes)),
		waterContactHeight(new Variant(WaterFoamEffectComponent::AttrWaterContactHeight(), 1.5f, this->attributes)),
		surfaceOffset(new Variant(WaterFoamEffectComponent::AttrSurfaceOffset(), 0.02f, this->attributes)),
		particleOffset(new Variant(WaterFoamEffectComponent::AttrParticleOffset(), Ogre::Vector3(0.0f, 0.0f, 0.0f), this->attributes))
	{
		this->oceanId->setDescription("Id of the GameObject that owns the OceanComponent. What it does: links this effect to a specific ocean. Why: lets the player component query ocean level and decide when to play foam.");
		this->particleTemplateName->setDescription("ParticleUniverse system template name (e.g. 'wake'). What it does: selects which ParticleUniverse script is instantiated. Why: keeps the component reusable for different wake/foam styles.");

		this->particleScale->setConstraints(0.001f, 5.0f);
		this->particleScale->setDescription("Uniform particle system scale. What it does: scales the full wake/foam effect. Why: lets you match effect size to your character/world scale.");

		this->playTimeMs->setConstraints(0.0f, 600000.0f);
		this->playTimeMs->setDescription("How long the particle plays after start(milliseconds). 0 = continuous. What it does: defines whether the system auto-stops or runs until you leave the water. Why: wake/foam usually should run continuously while swimming.");

		this->waterContactHeight->setConstraints(0.0f, 10.0f);
		this->waterContactHeight->setDescription("Activation height above the ocean surface. What it does: effect is active when playerY <= waterY + this value. Why: player origin is often above feet; this makes the wake trigger feel correct.");

		this->surfaceOffset->setConstraints(-1.0f, 1.0f);
		this->surfaceOffset->setDescription("Vertical offset applied to place the effect on the surface. What it does: effectY = waterY + surfaceOffset. Why: small positive offset avoids z-fighting and makes foam sit on top of water.");

		this->particleOffset->setDescription("Local/world offset added to the owner position before snapping to the water surface. What it does: shifts the wake forward/back or to the feet. Why: lets you place the foam exactly where the body meets the water.");
	}

	WaterFoamEffectComponent::~WaterFoamEffectComponent()
	{
		
	}

	void WaterFoamEffectComponent::initialise()
	{

	}

	const Ogre::String& WaterFoamEffectComponent::getName() const
	{
		return this->name;
	}

	void WaterFoamEffectComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<WaterFoamEffectComponent>(WaterFoamEffectComponent::getStaticClassId(), WaterFoamEffectComponent::getStaticClassName());
	}

	void WaterFoamEffectComponent::shutdown()
	{
		// Do nothing here
	}

	void WaterFoamEffectComponent::uninstall()
	{
		// Do nothing here
	}

	void WaterFoamEffectComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool WaterFoamEffectComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OceanId")
		{
			this->oceanId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ParticleTemplateName")
		{
			this->particleTemplateName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ParticleScale")
		{
			this->particleScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.05f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlayTimeMs")
		{
			this->playTimeMs->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaterContactHeight")
		{
			this->waterContactHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SurfaceOffset")
		{
			this->surfaceOffset->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.02f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ParticleOffset")
		{
			this->particleOffset->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr WaterFoamEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		WaterFoamEffectCompPtr clonedCompPtr(boost::make_shared<WaterFoamEffectComponent>());

		clonedCompPtr->setOceanId(this->oceanId->getULong());
		clonedCompPtr->setParticleTemplateName(this->particleTemplateName->getString());
		clonedCompPtr->setParticleScale(this->particleScale->getReal());
		clonedCompPtr->setPlayTimeMs(this->playTimeMs->getReal());
		clonedCompPtr->setWaterContactHeight(this->waterContactHeight->getReal());
		clonedCompPtr->setSurfaceOffset(this->surfaceOffset->getReal());
		clonedCompPtr->setParticleOffset(this->particleOffset->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool WaterFoamEffectComponent::onCloned(void)
	{
		// Remap ocean id if the ocean got cloned too (same pattern as TagChildNodeComponent)
		GameObjectPtr oceanGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->oceanId->getULong());
		if (nullptr != oceanGo)
			this->setOceanId(oceanGo->getId());
		else
			this->setOceanId(0);

		return true;
	}

	bool WaterFoamEffectComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WaterFoamEffectComponent] Init for game object: " + this->gameObjectPtr->getName());

		this->postInitDone = true;
		this->resolveOceanComponent();
		return true;
	}

	bool WaterFoamEffectComponent::connect(void)
	{
		return true;
	}

	bool WaterFoamEffectComponent::disconnect(void)
	{
		this->stopEffect();
		return true;
	}

	void WaterFoamEffectComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WaterFoamEffectComponent] Destructor for game object: " + this->gameObjectPtr->getName());

		this->stopEffect();
		this->oceanComponent = nullptr;
	}

	void WaterFoamEffectComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == this->postInitDone)
		{
			return;
		}

		if (0 == this->oceanId->getULong())
		{
			if (true == this->effectActive)
			{
				this->stopEffect();
			}
			return;
		}

		if (nullptr == this->oceanComponent)
		{
			this->resolveOceanComponent();
		}

		if (nullptr == this->oceanComponent)
		{
			if (true == this->effectActive)
			{
				this->stopEffect();
			}
			return;
		}

		// Ocean surface is constant: Ocean origin Y == OceanCenter.y (see Ocean.cpp)
		const Ogre::Real waterY = this->oceanComponent->getOceanCenter().y;
		const Ogre::Vector3 playerPos = this->gameObjectPtr->getPosition();

		const Ogre::Real contactY = waterY + this->waterContactHeight->getReal();
		const bool touchingWater = (playerPos.y <= contactY);

		if (true == touchingWater && false == this->effectActive)
		{
			this->startEffect();
		}
		else if (false == touchingWater && true == this->effectActive)
		{
			this->stopEffect();
		}

		if (true == this->effectActive)
		{
			this->updateEffectTransform();
		}
	}

	void WaterFoamEffectComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (WaterFoamEffectComponent::AttrOceanId() == attribute->getName())
		{
			this->setOceanId(attribute->getULong());
		}
		else if (WaterFoamEffectComponent::AttrParticleTemplateName() == attribute->getName())
		{
			this->setParticleTemplateName(attribute->getString());
		}
		else if (WaterFoamEffectComponent::AttrParticleScale() == attribute->getName())
		{
			this->setParticleScale(attribute->getReal());
		}
		else if (WaterFoamEffectComponent::AttrPlayTimeMs() == attribute->getName())
		{
			this->setPlayTimeMs(attribute->getReal());
		}
		else if (WaterFoamEffectComponent::AttrWaterContactHeight() == attribute->getName())
		{
			this->setWaterContactHeight(attribute->getReal());
		}
		else if (WaterFoamEffectComponent::AttrSurfaceOffset() == attribute->getName())
		{
			this->setSurfaceOffset(attribute->getReal());
		}
		else if (WaterFoamEffectComponent::AttrParticleOffset() == attribute->getName())
		{
			this->setParticleOffset(attribute->getVector3());
		}
	}

	void WaterFoamEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OceanId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->oceanId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ParticleTemplateName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleTemplateName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ParticleScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleScale->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PlayTimeMs"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->playTimeMs->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaterContactHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waterContactHeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SurfaceOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->surfaceOffset->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ParticleOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleOffset->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String WaterFoamEffectComponent::getClassName(void) const
	{
		return "WaterFoamEffectComponent";
	}

	Ogre::String WaterFoamEffectComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void WaterFoamEffectComponent::setOceanId(unsigned long oceanId)
	{
		this->oceanId->setValue(oceanId);
		this->oceanComponent = nullptr;

		if (true == this->effectActive)
			this->stopEffect();

		this->resolveOceanComponent();
	}

	unsigned long WaterFoamEffectComponent::getOceanId(void) const
	{
		return this->oceanId->getULong();
	}

	void WaterFoamEffectComponent::setParticleTemplateName(const Ogre::String& templateName)
	{
		this->particleTemplateName->setValue(templateName);

		if (true == this->effectActive)
		{
			this->stopEffect();
			this->startEffect();
		}
	}

	Ogre::String WaterFoamEffectComponent::getParticleTemplateName(void) const
	{
		return this->particleTemplateName->getString();
	}

	void WaterFoamEffectComponent::setParticleScale(Ogre::Real scale)
	{
		if (scale < Ogre::Real(0.0f))
		{
			scale = Ogre::Real(0.0f);
		}

		this->particleScale->setValue(scale);
	}

	Ogre::Real WaterFoamEffectComponent::getParticleScale(void) const
	{
		return this->particleScale->getReal();
	}

	void WaterFoamEffectComponent::setPlayTimeMs(Ogre::Real playTimeMs)
	{
		if (playTimeMs < Ogre::Real(0.0f))
		{
			playTimeMs = Ogre::Real(0.0f);
		}

		this->playTimeMs->setValue(playTimeMs);

		// Only affects newly created systems
	}

	Ogre::Real WaterFoamEffectComponent::getPlayTimeMs(void) const
	{
		return this->playTimeMs->getReal();
	}

	void WaterFoamEffectComponent::setWaterContactHeight(Ogre::Real waterContactHeight)
	{
		if (waterContactHeight < Ogre::Real(0.0f))
		{
			waterContactHeight = Ogre::Real(0.0f);
		}

		this->waterContactHeight->setValue(waterContactHeight);
	}

	Ogre::Real WaterFoamEffectComponent::getWaterContactHeight(void) const
	{
		return this->waterContactHeight->getReal();
	}

	void WaterFoamEffectComponent::setSurfaceOffset(Ogre::Real surfaceOffset)
	{
		this->surfaceOffset->setValue(surfaceOffset);
	}

	Ogre::Real WaterFoamEffectComponent::getSurfaceOffset(void) const
	{
		return this->surfaceOffset->getReal();
	}

	void WaterFoamEffectComponent::setParticleOffset(const Ogre::Vector3& offset)
	{
		this->particleOffset->setValue(offset);
	}

	Ogre::Vector3 WaterFoamEffectComponent::getParticleOffset(void) const
	{
		return this->particleOffset->getVector3();
	}

	void WaterFoamEffectComponent::resolveOceanComponent(void)
	{
		this->oceanComponent = nullptr;

		GameObjectPtr oceanGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->oceanId->getULong());
		if (nullptr == oceanGo)
		{
			return;
		}

		auto oceanCompPtr = NOWA::makeStrongPtr(oceanGo->getComponent<OceanComponent>());
		if (nullptr != oceanCompPtr)
		{
			this->oceanComponent = oceanCompPtr.get();
		}
	}

	Ogre::String WaterFoamEffectComponent::getParticleSystemInstanceName(void) const
	{
		return "WaterFoamEffect_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(this->index);
	}

	Ogre::String WaterFoamEffectComponent::getTrackedClosureId(void) const
	{
		return this->gameObjectPtr->getName() + this->getClassName() + "::updateParticle" + Ogre::StringConverter::toString(this->index);
	}

	void WaterFoamEffectComponent::startEffect(void)
	{
		if (true == this->effectActive)
			return;

		ParticleFxModule* particleFxModule = AppStateManager::getSingletonPtr()->getParticleFxModule();
		if (nullptr == particleFxModule)
		{
			return;
		}

		const Ogre::Real waterY = this->oceanComponent ? this->oceanComponent->getOceanCenter().y : this->gameObjectPtr->getPosition().y;
		Ogre::Vector3 pos = this->gameObjectPtr->getPosition() + this->particleOffset->getVector3();
		pos.y = waterY + this->surfaceOffset->getReal();

		const Ogre::String instanceName = this->getParticleSystemInstanceName();
		const Ogre::String templateName = this->particleTemplateName->getString();
		const Ogre::Quaternion q = Ogre::Quaternion::IDENTITY;
		const Ogre::Real scale = this->particleScale->getReal();
		const Ogre::Real tMs = this->playTimeMs->getReal();

		particleFxModule->createParticleSystem(instanceName, templateName, tMs, q, pos, Ogre::Vector2(scale, scale), true, 1.0f, NOWA::ParticleBlendingMethod::AlphaBlending, true, 1000.0f, true, 1000.0f);
		particleFxModule->playParticleSystem(instanceName);

		this->effectActive = true;
	}

	void WaterFoamEffectComponent::stopEffect(void)
	{
		if (false == this->effectActive)
			return;

		Ogre::String id = this->getTrackedClosureId();
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		ParticleFxModule* particleFxModule = AppStateManager::getSingletonPtr()->getParticleFxModule();
		if (nullptr != particleFxModule)
		{
			const Ogre::String instanceName = this->getParticleSystemInstanceName();
			particleFxModule->stopParticleSystem(instanceName);
			particleFxModule->removeParticle(instanceName);
		}

		this->effectActive = false;
	}

	void WaterFoamEffectComponent::updateEffectTransform(void)
	{
		ParticleFxModule* particleFxModule = AppStateManager::getSingletonPtr()->getParticleFxModule();
		if (nullptr == particleFxModule)
		{
			return;
		}

		const Ogre::Real waterY = this->oceanComponent ? this->oceanComponent->getOceanCenter().y : this->gameObjectPtr->getPosition().y;
		Ogre::Vector3 pos = this->gameObjectPtr->getPosition() + this->particleOffset->getVector3();
		pos.y = waterY + this->surfaceOffset->getReal();

		const Ogre::String instanceName = this->getParticleSystemInstanceName();
		const Ogre::Real scale = this->particleScale->getReal();
		const Ogre::String closureId = this->getTrackedClosureId();

		auto closureFunction = [instanceName, pos, scale, particleFxModule](Ogre::Real renderDt)
		{
			auto particleFxData = particleFxModule->getParticle(instanceName);
			if (nullptr != particleFxData && nullptr != particleFxData->particleNode)
			{
				particleFxData->particleNode->setPosition(pos);
				particleFxData->particleScale = Ogre::Vector2(scale, scale);
			}
		};
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(closureId, closureFunction, false);
	}

	// Lua registration part

	WaterFoamEffectComponent* getWaterFoamEffectComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<WaterFoamEffectComponent>(gameObject->getComponentWithOccurrence<WaterFoamEffectComponent>(occurrenceIndex)).get();
	}

	WaterFoamEffectComponent* getWaterFoamEffectComponent(GameObject* gameObject)
	{
		return makeStrongPtr<WaterFoamEffectComponent>(gameObject->getComponent<WaterFoamEffectComponent>()).get();
	}

	WaterFoamEffectComponent* getWaterFoamEffectComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<WaterFoamEffectComponent>(gameObject->getComponentFromName<WaterFoamEffectComponent>(name)).get();
	}

	void WaterFoamEffectComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<WaterFoamEffectComponent, GameObjectComponent>("WaterFoamEffectComponent")
			.def("setActivated", &WaterFoamEffectComponent::setActivated)
			.def("isActivated", &WaterFoamEffectComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("WaterFoamEffectComponent", "class inherits GameObjectComponent", WaterFoamEffectComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("WaterFoamEffectComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterFoamEffectComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterFoamEffectComponent", "GameObject getOwner()", "Gets the owner game object.");

		gameObjectClass.def("getWaterFoamEffectComponentFromName", &getWaterFoamEffectComponentFromName);
		gameObjectClass.def("getWaterFoamEffectComponent", (WaterFoamEffectComponent * (*)(GameObject*)) & getWaterFoamEffectComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getWaterFoamEffectComponentFromIndex", (WaterFoamEffectComponent * (*)(GameObject*, unsigned int)) & getWaterFoamEffectComponentFromIndex);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WaterFoamEffectComponent getWaterFoamEffectComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WaterFoamEffectComponent getWaterFoamEffectComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WaterFoamEffectComponent getWaterFoamEffectComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castWaterFoamEffectComponent", &GameObjectController::cast<WaterFoamEffectComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "WaterFoamEffectComponent castWaterFoamEffectComponent(WaterFoamEffectComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool WaterFoamEffectComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		if (gameObject->getComponentCount<WaterFoamEffectComponent>() < 2)
		{
			return true;
		}
	}

}; //namespace end