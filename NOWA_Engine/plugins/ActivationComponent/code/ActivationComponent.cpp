#include "NOWAPrecompiled.h"
#include "ActivationComponent.h"
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

	ActivationComponent::ActivationComponent()
		: GameObjectComponent(),
		name("ActivationComponent")
	{

	}

	ActivationComponent::~ActivationComponent(void)
	{
		
	}

	const Ogre::String& ActivationComponent::getName() const
	{
		return this->name;
	}

	void ActivationComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ActivationComponent>(ActivationComponent::getStaticClassId(), ActivationComponent::getStaticClassName());
	}
	
	void ActivationComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool ActivationComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);
		
		return true;
	}

	GameObjectCompPtr ActivationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ActivationComponentPtr clonedCompPtr(boost::make_shared<ActivationComponent>());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ActivationComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ActivationComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool ActivationComponent::connect(void)
	{
		GameObjectComponent::connect();

		return true;
	}

	bool ActivationComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();
		return true;
	}

	bool ActivationComponent::onCloned(void)
	{
		
		return true;
	}

	void ActivationComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
	}
	
	void ActivationComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void ActivationComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void ActivationComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void ActivationComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);
	}

	void ActivationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);
	}

	void ActivationComponent::setActivated(bool activated)
	{
		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i)).get();
			if (component != this)
			{
				component->setActivated(activated);
			}
		}
	}

	Ogre::String ActivationComponent::getClassName(void) const
	{
		return "ActivationComponent";
	}

	Ogre::String ActivationComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	// Lua registration part

	ActivationComponent* getActivationComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<ActivationComponent>(gameObject->getComponentWithOccurrence<ActivationComponent>(occurrenceIndex)).get();
	}

	ActivationComponent* getActivationComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ActivationComponent>(gameObject->getComponent<ActivationComponent>()).get();
	}

	ActivationComponent* getActivationComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ActivationComponent>(gameObject->getComponentFromName<ActivationComponent>(name)).get();
	}

	void ActivationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		
	}

	bool ActivationComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (gameObject->getComponentCount<ActivationComponent>() < 2)
		{
			return true;
		}
	}

}; //namespace end