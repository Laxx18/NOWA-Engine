#include "NOWAPrecompiled.h"
#include "ParticleFxComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ParticleFxComponent::ParticleFxComponent()
		: GameObjectComponent(),
		name("ParticleFxComponent"),
		particleSystem(nullptr),
		particleNode(nullptr),
		particlePlayTime(10000.0f),
		oldActivated(true),
		activated(new Variant(ParticleFxComponent::AttrActivated(), true, this->attributes)),
		particleTemplateName(new Variant(ParticleFxComponent::AttrParticleName(), std::vector<Ogre::String>(), this->attributes)),
		repeat(new Variant(ParticleFxComponent::AttrRepeat(), false, this->attributes)),
		particleInitialPlayTime(new Variant(ParticleFxComponent::AttrPlayTime(), 10000.0f, this->attributes)),
		particlePlaySpeed(new Variant(ParticleFxComponent::AttrPlaySpeed(), 1.0f, this->attributes)),
		particleOffsetPosition(new Variant(ParticleFxComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		particleOffsetOrientation(new Variant(ParticleFxComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		particleScale(new Variant(ParticleFxComponent::AttrScale(), Ogre::Vector3::UNIT_SCALE, this->attributes))
	{
		std::vector<Ogre::String> availableParticleTemplateNames;
		// Get all available particle effects from resources

#if 0
		ParticleUniverse::ParticleSystemManager::getSingletonPtr()->particleSystemTemplateNames(availableParticleTemplateNames);
		std::vector<Ogre::String> strAvailableParticleTemplateNames(availableParticleTemplateNames.size());

		for (size_t i = 0; i < availableParticleTemplateNames.size(); i++)
		{
			strAvailableParticleTemplateNames[i] = availableParticleTemplateNames[i];
		}

		if (false == strAvailableParticleTemplateNames.empty())
		{
			this->particleTemplateName->setValue(strAvailableParticleTemplateNames);
			this->particleTemplateName->setListSelectedValue(strAvailableParticleTemplateNames[0]);
		}
#endif

		// Activated variable is set to false again, when particle has played and repeat is off, so tell it the gui that it must be refreshed
		this->activated->addUserData(GameObject::AttrActionNeedRefresh());
		this->particleInitialPlayTime->setDescription("The particle play time, if set to 0, the particle effect will not stop automatically.");
	}

	ParticleFxComponent::~ParticleFxComponent(void)
	{
		
	}

	void ParticleFxComponent::initialise()
	{

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

	}

	void ParticleFxComponent::uninstall()
	{

	}
	
	void ParticleFxComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool ParticleFxComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		/*if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShortTimeActivation")
		{
			this->shortTimeActivation->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}*/
		
		return true;
	}

	GameObjectCompPtr ParticleFxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ParticleFxComponentPtr clonedCompPtr(boost::make_shared<ParticleFxComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ParticleFxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool ParticleFxComponent::connect(void)
	{
		// Hello World test :)
		auto physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{
			physicsActiveCompPtr->applyRequiredForceForVelocity(Ogre::Vector3(0.0f, 10.0f, 0.0f));
		}
		else
		{
			this->gameObjectPtr->getSceneNode()->translate(Ogre::Vector3(0.0f, 10.0f, 0.0f));
		}
		
		
		return true;
	}

	bool ParticleFxComponent::disconnect(void)
	{
		
		return true;
	}

	bool ParticleFxComponent::onCloned(void)
	{
		
		return true;
	}

	void ParticleFxComponent::onRemoveComponent(void)
	{
		
	}
	
	void ParticleFxComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void ParticleFxComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void ParticleFxComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void ParticleFxComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ParticleFxComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		/*else if(ParticleFxComponent::AttrShortTimeActivation() == attribute->getName())
		{
			this->setShortTimeActivation(attribute->getBool());
		}*/
	}

	void ParticleFxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);	

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShortTimeActivation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shortTimeActivation->getBool())));
		propertiesXML->append_node(propertyXML);*/
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
	}

	bool ParticleFxComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	// Lua registration part

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
		];

		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "class inherits GameObjectComponent", ParticleFxComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getParticleFxComponentFromName", &getParticleFxComponentFromName);
		gameObjectClass.def("getParticleFxComponent", (ParticleFxComponent * (*)(GameObject*)) & getParticleFxComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getParticleFxComponentFromIndex", (ParticleFxComponent * (*)(GameObject*, unsigned int)) & getParticleFxComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castParticleFxComponent", &GameObjectController::cast<ParticleFxComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ParticleFxComponent castParticleFxComponent(ParticleFxComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool ParticleFxComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end