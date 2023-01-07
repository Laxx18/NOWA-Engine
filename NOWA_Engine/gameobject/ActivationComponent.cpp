#include "NOWAPrecompiled.h"
#include "ActivationComponent.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ActivationComponent::ActivationComponent()
		: GameObjectComponent()
	{

	}

	ActivationComponent::~ActivationComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ActivationComponent] Destructor activation component for game object: " + this->gameObjectPtr->getName());
	}

	GameObjectCompPtr ActivationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ActivationCompPtr clonedCompPtr(boost::make_shared<ActivationComponent>());

				
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ActivationComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ActivationComponent] Init activation component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool ActivationComponent::connect(void)
	{
		// No ray execution does work, if not visible
		// this->gameObjectPtr->setVisible(false);
		return true;
	}

	bool ActivationComponent::disconnect(void)
	{
		// this->gameObjectPtr->setVisible(true);
		return true;
	}

	Ogre::String ActivationComponent::getClassName(void) const
	{
		return "ActivationComponent";
	}

	Ogre::String ActivationComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void ActivationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// Note string is parsed for Component*, hence ComponentAiLua, but the real class name is ActivationComponent
		
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);
	}

	void ActivationComponent::setActivated(bool activated)
	{
		// this->gameObjectPtr->setVisible(activated);
		// Set activated state of all components, besides this one (recursion prevention)
		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i)).get();
			if (component != this)
			{
				component->setActivated(activated);
			}
		}
	}

}; // namespace end