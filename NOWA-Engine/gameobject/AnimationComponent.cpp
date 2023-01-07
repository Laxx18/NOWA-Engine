#include "NOWAPrecompiled.h"
#include "AnimationComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/WorkspaceModule.h"
#include "LookAfterComponent.h"
#include "PlayerControllerComponents.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AnimationComponent::AnimationComponent()
		: GameObjectComponent(),
		activated(new Variant(AnimationComponent::AttrActivated(), true, this->attributes)),
		animationName(new Variant(AnimationComponent::AttrName(), std::vector<Ogre::String>(), this->attributes)),
		animationSpeed(new Variant(AnimationComponent::AttrSpeed(), 1.0f, this->attributes)),
		animationRepeat(new Variant(AnimationComponent::AttrRepeat(), true, this->attributes)),
		showSkeleton(new Variant(AnimationComponent::AttrShowSkeleton(), false, this->attributes)),
		animationBlender(nullptr),
		skeletonVisualizer(nullptr),
		isInSimulation(false)
	{
		
	}

	AnimationComponent::~AnimationComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Destructor animation component for game object: " + this->gameObjectPtr->getName());

		if (this->animationBlender)
		{
			delete this->animationBlender;
			this->animationBlender = nullptr;
		}
		if (nullptr != this->skeletonVisualizer)
		{
			delete this->skeletonVisualizer;
			this->skeletonVisualizer = nullptr;
		}
	}

	bool AnimationComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationActivated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		// collision type is mandantory
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationName")
		{
			this->animationName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationSpeed")
		{
			this->animationSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationRepeat")
		{
			this->animationRepeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowSkeleton")
		{
			this->showSkeleton->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr AnimationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AnimationCompPtr clonedCompPtr(boost::make_shared<AnimationComponent>());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->createAnimationBlender();
		clonedCompPtr->setAnimationName(this->animationName->getListSelectedValue());
		clonedCompPtr->setSpeed(this->animationSpeed->getReal());
		clonedCompPtr->setRepeat(this->animationRepeat->getBool());

		// Activation after everything is set, because the game object is required
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setShowSkeleton(this->showSkeleton->getBool());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AnimationComponent::postInit(void)
	{
		// Do not set things a second time after cloning
		if (nullptr != this->animationBlender)
		{
			return true;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Init animation component for game object: " + this->gameObjectPtr->getName());
		
		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createAnimationBlender();
		return true;
	}

	void AnimationComponent::createAnimationBlender(void)
	{
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] List all animations for mesh '" + entity->getMesh()->getName() + "':");
			std::vector<Ogre::String> animationNames;
			Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
			if (nullptr != set)
			{
				Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
				// list all animations
				while (it.hasMoreElements())
				{
					Ogre::v1::AnimationState* anim = it.getNext();
					animationNames.emplace_back(anim->getAnimationName());
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Animation name: " + anim->getAnimationName()
						+ " length: " + Ogre::StringConverter::toString(anim->getLength()) + " seconds");
				}
				// Add all available animation names to list
				this->animationName->setValue(animationNames);
			}

			this->animationBlender = new NOWA::AnimationBlender(entity);
			this->animationBlender->init(this->animationName->getListSelectedValue(), this->animationRepeat->getBool());
		}
	}

	bool AnimationComponent::connect(void)
	{
		if (true == this->activated->getBool())
		{
			this->activateAnimation();
		}
		return true;
	}

	bool AnimationComponent::disconnect(void)
	{
		this->resetAnimation();
		return true;
	}

	Ogre::String AnimationComponent::getClassName(void) const
	{
		return "AnimationComponent";
	}

	Ogre::String AnimationComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void AnimationComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->isInSimulation = !notSimulating;
		
		if (true == this->activated->getBool() && false == notSimulating)
		{
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationComponent] weight: " + Ogre::StringConverter::toString(this->animationState->getWeight()));
			if (nullptr != this->animationBlender)
				this->animationBlender->addTime(dt * this->animationSpeed->getReal() / this->animationBlender->getSource()->getLength());
		}
		/*else if (true == notSimulating && nullptr != this->animationBlender)
		{
			this->resetAnimation();
		}*/
		if (true == showSkeleton->getBool())
		{
			this->skeletonVisualizer->update(dt);
		}
	}
	
	void AnimationComponent::resetAnimation(void)
	{
		if (nullptr != this->animationBlender)
		{
			if (nullptr != this->animationBlender->getSource())
			{
				this->animationBlender->getSource()->setEnabled(false);
				this->animationBlender->getSource()->setWeight(0.0f);
				this->animationBlender->getSource()->setTimePosition(0.0f);
			}
			if (nullptr != this->animationBlender->getTarget())
			{
				this->animationBlender->getTarget()->setEnabled(false);
				this->animationBlender->getTarget()->setWeight(0.0f);
				this->animationBlender->getTarget()->setTimePosition(0.0f);
			}
		}
	}

	void AnimationComponent::actualizeValue(Variant* attribute)
	{
		this->resetAnimation();

		GameObjectComponent::actualizeValue(attribute);
		
		if (AnimationComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AnimationComponent::AttrName() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue());
		}
		else if (AnimationComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (AnimationComponent::AttrSpeed() == attribute->getName())
		{
			this->animationSpeed->setValue(attribute->getReal());
		}
		else if (AnimationComponent::AttrShowSkeleton() == attribute->getName())
		{
			this->setShowSkeleton(attribute->getBool());
		}
	}

	void AnimationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationActivated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->animationName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->animationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->animationRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShowSkeleton"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->showSkeleton->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void AnimationComponent::activateAnimation(void)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return;
		}
		
		if (false == this->animationName->getListSelectedValue().empty())
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				Ogre::v1::AnimationState* animationState = entity->getAnimationState(this->animationName->getListSelectedValue());
				if (nullptr != animationState)
				{
					this->resetAnimation();
					// this->animationState->setEnabled(true);
					// this->animationState->setWeight(5.0f);
					// this->animationState->setTimePosition(0.0f);
					// this->animationBlender->init(this->animationName->getListSelectedValue(), this->animationRepeat->getBool());
					this->animationBlender->blend(this->animationName->getListSelectedValue(), NOWA::AnimationBlender::BlendThenAnimate, 0.2f, this->animationRepeat->getBool());
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Error: The entity: " + entity->getName() 
						+ " has no animation: " + this->animationName->getListSelectedValue());
					Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
					if (nullptr != set)
					{
						Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
						// list all animations
						while (it.hasMoreElements())
						{
							Ogre::v1::AnimationState* anim = it.getNext();
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent]: Available animations:");
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Animation name: " + anim->getAnimationName()
								+ " length: " + Ogre::StringConverter::toString(anim->getLength()) + " seconds");
						}
					}
				}
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Warning: The given animation name is empty.");
		}
	}

	bool AnimationComponent::isComplete(void) const
	{
		if (nullptr == this->animationBlender)
			return true;
		return this->animationBlender->isComplete();
	}

	void AnimationComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		// First deactivate
		if (nullptr != this->animationBlender)
		{
			// disable animation
			this->resetAnimation();
		}

		if (true == this->isInSimulation && true == activated)
			this->activateAnimation();
	}

	bool AnimationComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	bool AnimationComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Check if the entity has at least one animation and no player controller, else animation component is senseless
		auto playerControllerCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlayerControllerComponent>());
		Ogre::v1::Entity* entity = gameObject->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity && nullptr == playerControllerCompPtr)
		{
			Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
			if (nullptr != set)
			{
				Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
				if (it.hasMoreElements())
				{
					return true;
				}
			}
		}
		return false;
	}

	void AnimationComponent::setAnimationName(const Ogre::String& animationName)
	{
		if (nullptr != this->animationBlender)
		{
			// if (this->animationBlender->getSource()->getAnimationName() != animationName)
			{
				this->animationName->setListSelectedValue(animationName);
				this->setActivated(this->activated->getBool());
			}
		}
	}

	const Ogre::String AnimationComponent::getAnimationName(void) const
	{
		return this->animationName->getListSelectedValue();
	}

	void AnimationComponent::setSpeed(Ogre::Real animationSpeed)
	{
		this->animationSpeed->setValue(animationSpeed);
	}

	Ogre::Real AnimationComponent::getSpeed(void) const
	{
		return this->animationSpeed->getReal();
	}

	void AnimationComponent::setRepeat(bool animationRepeat)
	{
		if (nullptr != this->animationBlender)
		{
			// if (this->animationBlender->getSource()->getLoop() != animationRepeat)
			{
				this->animationRepeat->setValue(animationRepeat);
				this->setActivated(this->activated->getBool());
			}
		}
	}

	bool AnimationComponent::getRepeat(void) const
	{
		return this->animationRepeat->getBool();
	}

	void AnimationComponent::setShowSkeleton(bool showSkeleton)
	{
		this->showSkeleton->setValue(showSkeleton);
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		// entity->setDisplaySkeleton(this->showSkeleton->getBool());

		/*if (true == showSkeleton)
		{
			this->skeletonVisualizer = new SkeletonVisualizer(entity, this->gameObjectPtr->getSceneManager(), WorkspaceModule::getInstance()->getWorkspace()->getDefaultCamera());
		}*/
	}

	bool AnimationComponent::getShowSkeleton(void) const
	{
		return this->showSkeleton->getBool();
	}
	
	AnimationBlender* AnimationComponent::getAnimationBlender(void) const
	{
		return this->animationBlender;
	}
	
	Ogre::v1::OldBone* AnimationComponent::getBone(const Ogre::String& boneName)
	{
		Ogre::v1::OldBone* bone = nullptr;
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			Ogre::v1::Skeleton* skeleton = entity->getSkeleton();
			if (nullptr != skeleton)
			{
				if (true == skeleton->hasBone(boneName))
				{
					bone = skeleton->getBone(boneName);
					return bone;
				}
			}
		}
		return bone;
	}

	Ogre::Vector3 AnimationComponent::getLocalToWorldPosition(Ogre::v1::OldBone* bone)
	{
		return this->animationBlender->getLocalToWorldPosition(bone);
	}

	Ogre::Quaternion AnimationComponent::getLocalToWorldOrientation(Ogre::v1::OldBone* bone)
	{
		return this->animationBlender->getLocalToWorldOrientation(bone);
	}
	
	void AnimationComponent::setTimePosition(Ogre::Real timePosition)
	{
		if (nullptr != this->animationBlender)
		{
			this->animationBlender->getSource()->setTimePosition(timePosition);
		}
	}
	
	Ogre::Real AnimationComponent::getTimePosition(void) const
	{
		if (nullptr != this->animationBlender)
		{
			return this->animationBlender->getSource()->getTimePosition();
		}
		return 0.0f;
	}
	
	Ogre::Real AnimationComponent::getLength(void) const
	{
		if (nullptr != this->animationBlender)
		{
			return this->animationBlender->getSource()->getLength();
		}
		return 0.0f;
	}
	
	void AnimationComponent::setWeight(Ogre::Real weight)
	{
		if (nullptr != this->animationBlender)
		{
			this->animationBlender->getSource()->setWeight(weight);
		}
	}
	
	Ogre::Real AnimationComponent::getWeight(void) const
	{
		if (nullptr != this->animationBlender)
		{
			return this->animationBlender->getSource()->getWeight();
		}
		return 0.0f;
	}
	
}; // namespace end