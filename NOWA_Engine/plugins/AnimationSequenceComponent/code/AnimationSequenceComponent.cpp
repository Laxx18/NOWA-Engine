#include "NOWAPrecompiled.h"
#include "AnimationSequenceComponent.h"
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

	AnimationSequenceComponent::AnimationSequenceComponent()
		: GameObjectComponent(),
		name("AnimationSequenceComponent"),
		activated(new Variant(AnimationSequenceComponent::AttrActivated(), true, this->attributes)),
		animationRepeat(new Variant(AnimationSequenceComponent::AttrRepeat(), true, this->attributes)),
		showSkeleton(new Variant(AnimationSequenceComponent::AttrShowSkeleton(), false, this->attributes)),
		animationCount(new Variant(AnimationSequenceComponent::AttrAnimationCount(), static_cast<unsigned int>(0), this->attributes)),
		animationBlender(nullptr),
		skeletonVisualizer(nullptr),
		timePosition(0.0f),
		firstTimeRepeat(true),
		currentAnimationIndex(0),
		bIsInSimulation(false)
	{
		// Since when animation count is changed, the whole properties must be refreshed, so that new field may come for animations
		this->animationCount->addUserData(GameObject::AttrActionNeedRefresh());

	}

	AnimationSequenceComponent::~AnimationSequenceComponent(void)
	{

	}

	void AnimationSequenceComponent::initialise()
	{

	}

	const Ogre::String& AnimationSequenceComponent::getName() const
	{
		return this->name;
	}

	void AnimationSequenceComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<AnimationSequenceComponent>(AnimationSequenceComponent::getStaticClassId(), AnimationSequenceComponent::getStaticClassName());
	}

	void AnimationSequenceComponent::shutdown()
	{

	}

	void AnimationSequenceComponent::uninstall()
	{

	}
	
	void AnimationSequenceComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool AnimationSequenceComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->animationRepeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Count")
		{
			this->animationCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// Only create new variant, if fresh loading. If snapshot is done, no new variant
		// must be created! Because the algorithm is working changed flag of each existing variant!
		if (this->animationNames.size() < this->animationCount->getUInt())
		{
			this->animationNames.resize(this->animationCount->getUInt());
			this->animationBlendTransitions.resize(this->animationCount->getUInt());
			this->animationDurations.resize(this->animationCount->getUInt());
			this->animationTimePositions.resize(this->animationCount->getUInt());
			this->animationSpeeds.resize(this->animationCount->getUInt());
		}

		for (size_t i = 0; i < this->animationNames.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationName" + Ogre::StringConverter::toString(i))
			{
				Ogre::String animationName = XMLConverter::getAttrib(propertyElement, "data");
				// List will be filled in postInit, in which the entity is available, but set the selected animation name string now, even the list is yet empty
				if (nullptr == this->animationNames[i])
				{
					this->animationNames[i] = new Variant(AnimationSequenceComponent::AttrAnimationName() + Ogre::StringConverter::toString(i), std::vector<Ogre::String>(), this->attributes);
					this->animationNames[i]->setListSelectedValue(animationName);
				}
				else
				{
					this->animationNames[i]->setListSelectedValue(animationName);
				}
				this->animationNames[i]->addUserData(GameObject::AttrActionNeedRefresh());
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendTransition" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->animationBlendTransitions[i])
				{
					this->animationBlendTransitions[i] = new Variant(AnimationSequenceComponent::AttrBlendTransition() + Ogre::StringConverter::toString(i),
						{ "BlendWhileAnimating", "BlendSwitch", "BlendThenAnimate" }, this->attributes);

					this->animationBlendTransitions[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				else
				{
					this->animationBlendTransitions[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Duration" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->animationDurations[i])
				{
					this->animationDurations[i] = new Variant(AnimationSequenceComponent::AttrDuration() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->animationDurations[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimePosition" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->animationTimePositions[i])
				{
					this->animationTimePositions[i] = new Variant(AnimationSequenceComponent::AttrTimePosition() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->animationTimePositions[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->animationSpeeds[i])
				{
					this->animationSpeeds[i] = new Variant(AnimationSequenceComponent::AttrSpeed() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data", 1.0f), this->attributes);
				}
				else
				{
					this->animationSpeeds[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				this->animationSpeeds[i]->addUserData(GameObject::AttrActionSeparator());
				propertyElement = propertyElement->next_sibling("property");
			}
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowSkeleton")
		{
			this->showSkeleton->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr AnimationSequenceComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AnimationSequenceCompPtr clonedCompPtr(boost::make_shared<AnimationSequenceComponent>());

		clonedCompPtr->setRepeat(this->animationRepeat->getBool());

		for (size_t i = 0; i < this->animationNames.size(); i++)
		{
			clonedCompPtr->setAnimationName(static_cast<unsigned int>(i), this->animationNames[i]->getListSelectedValue());
			clonedCompPtr->setBlendTransition(static_cast<unsigned int>(i), this->animationBlendTransitions[i]->getListSelectedValue());
			clonedCompPtr->setDuration(static_cast<unsigned int>(i), this->animationDurations[i]->getReal());
			clonedCompPtr->setTimePosition(static_cast<unsigned int>(i), this->animationTimePositions[i]->getReal());
			clonedCompPtr->setSpeed(static_cast<unsigned int>(i), this->animationSpeeds[i]->getReal());
		}

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		// Activation after everything is set, because the game object is required
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setShowSkeleton(this->showSkeleton->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AnimationSequenceComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] Init animation component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->generateAnimationList();
		return true;
	}

	void AnimationSequenceComponent::generateAnimationList(void)
	{
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			this->animationBlender = new NOWA::AnimationBlender(entity);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] List all animations for mesh '" + entity->getMesh()->getName() + "':");
			Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
			if (nullptr != set)
			{
				Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
				// list all animations
				while (it.hasMoreElements())
				{
					Ogre::v1::AnimationState* anim = it.getNext();

					this->availableAnimations.emplace_back(anim->getAnimationName());

					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] Animation name: " + anim->getAnimationName()
						+ " length: " + Ogre::StringConverter::toString(anim->getLength()) + " seconds");
				}
			}

			// Animation names must be set after loading, because just in post init the entity with the animations is available and not on init!
			for (size_t i = 0; i < this->animationNames.size(); i++)
			{
				// First get selected animation name
				Ogre::String selectedAnimationName = this->animationNames[i]->getListSelectedValue();
				// Fill list
				this->animationNames[i]->setValue(this->availableAnimations);
				// Reset selected animation name
				this->animationNames[i]->setListSelectedValue(selectedAnimationName);
			}
		}
	}

	bool AnimationSequenceComponent::connect(void)
	{
		GameObjectComponent::connect();

		if (true == this->activated->getBool())
		{
			this->activateAnimation();
		}
		return true;
	}

	bool AnimationSequenceComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		this->resetAnimation();
		return true;
	}

	void AnimationSequenceComponent::onRemoveComponent(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] Destructor animation component for game object: " + this->gameObjectPtr->getName());

		if (nullptr != this->skeletonVisualizer)
		{
			delete this->skeletonVisualizer;
			this->skeletonVisualizer = nullptr;
		}
		if (nullptr != this->animationBlender)
		{
			delete this->animationBlender;
			this->animationBlender = nullptr;
		}
	}

	Ogre::String AnimationSequenceComponent::getClassName(void) const
	{
		return "AnimationSequenceComponent";
	}

	Ogre::String AnimationSequenceComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void AnimationSequenceComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->bIsInSimulation = !notSimulating;

		if (true == this->activated->getBool() && false == notSimulating)
		{
			if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
			{
				this->timePosition += dt;

				// Check whether to play everything
				if (true == this->firstTimeRepeat)
				{
					this->firstTimeRepeat = false;
				}

				// Start at the second animation (if existing), because the first is played when animation is activated
				if (/*true == this->animationBlender->isComplete()*/this->timePosition >= this->animationDurations[this->currentAnimationIndex]->getReal())
				{
					this->currentAnimationIndex++;
					this->timePosition = 0.0f;

					// If repeat, start the current animation index at the beginning
					if (this->currentAnimationIndex > this->animationNames.size() - 1)
					{
						if (true == this->animationRepeat->getBool())
						{
							// this->resetAnimation();
							this->currentAnimationIndex = 0;
						}
						else
						{
							return;
						}
					}
					this->animationBlender->blend(this->animationNames[this->currentAnimationIndex]->getListSelectedValue(),
						this->mapStringToBlendingTransition(this->animationBlendTransitions[this->currentAnimationIndex]->getListSelectedValue()),
						0.2f, true);
				}

				// ATTENTION: Here later: just addTime(dt) and internally calculate the speed and length?
				Ogre::Real deltaTime = dt * this->animationSpeeds[this->currentAnimationIndex]->getReal() / this->animationBlender->getSource()->getLength();
				this->animationBlender->addTime(deltaTime);

				if (true == this->bShowDebugData)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationSequenceComponent] Playing Animation: '"
						+ this->animationNames[this->currentAnimationIndex]->getListSelectedValue()
						+ "', animation index: '" + Ogre::StringConverter::toString(this->currentAnimationIndex)
						+ "', time position: '" + Ogre::StringConverter::toString(this->timePosition)
						+ "', animation duration: '" + Ogre::StringConverter::toString(this->animationDurations[this->currentAnimationIndex]->getReal())
						+ "', add time: '" + Ogre::StringConverter::toString(deltaTime)
						+ "', weight: '" + Ogre::StringConverter::toString(this->animationBlender->getSource()->getWeight())
						+ "', duration: '" + Ogre::StringConverter::toString(this->animationDurations[this->currentAnimationIndex]->getReal()));
				}
			}
		}
		/*else if (true == notSimulating && nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
		{
			this->resetAnimation();
		}*/
		if (true == this->showSkeleton->getBool())
		{
			this->skeletonVisualizer->update(dt);
		}
	}

	void AnimationSequenceComponent::resetAnimation(void)
	{
		this->timePosition = 0.0f;
		this->currentAnimationIndex = 0;
		this->firstTimeRepeat = true;
		if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
		{
			this->animationBlender->getSource()->setEnabled(false);
			this->animationBlender->getSource()->setWeight(0.0f);
			this->animationBlender->getSource()->setTimePosition(0.0f);
			if (nullptr != this->animationBlender->getTarget())
			{
				this->animationBlender->getTarget()->setEnabled(false);
				this->animationBlender->getTarget()->setWeight(0.0f);
				this->animationBlender->getTarget()->setTimePosition(0.0f);
			}
		}
	}

	void AnimationSequenceComponent::actualizeValue(Variant* attribute)
	{
		// Attention: Is this correct?
		if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
		{
			this->animationBlender->getSource()->setTimePosition(0.0f);
		}

		GameObjectComponent::actualizeValue(attribute);

		if (AnimationSequenceComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AnimationSequenceComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (AnimationSequenceComponent::AttrAnimationCount() == attribute->getName())
		{
			this->setAnimationCount(attribute->getUInt());
		}
		else if (AnimationSequenceComponent::AttrShowSkeleton() == attribute->getName())
		{
			this->setShowSkeleton(attribute->getBool());
		}
		else
		{
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationNames.size()); i++)
			{
				if (AnimationSequenceComponent::AttrAnimationName() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setAnimationName(i, attribute->getListSelectedValue());
				}
			}
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationBlendTransitions.size()); i++)
			{
				if (AnimationSequenceComponent::AttrBlendTransition() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setBlendTransition(i, attribute->getListSelectedValue());
				}
			}
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationDurations.size()); i++)
			{
				if (AnimationSequenceComponent::AttrDuration() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setDuration(i, attribute->getReal());
				}
			}
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationTimePositions.size()); i++)
			{
				if (AnimationSequenceComponent::AttrTimePosition() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setTimePosition(i, attribute->getReal());
				}
			}
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationSpeeds.size()); i++)
			{
				if (AnimationSequenceComponent::AttrSpeed() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setSpeed(i, attribute->getReal());
				}
			}
		}
	}

	void AnimationSequenceComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Count"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->animationNames.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "AnimationName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationNames[i]->getListSelectedValue())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "BlendTransition" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationBlendTransitions[i]->getListSelectedValue())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Duration" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationDurations[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TimePosition" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationTimePositions[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Speed" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationSpeeds[i]->getReal())));
			propertiesXML->append_node(propertyXML);
		}

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShowSkeleton"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showSkeleton->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void AnimationSequenceComponent::activateAnimation(void)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		if (true == this->animationNames.empty())
			return;

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			// First deactivate
			this->resetAnimation();
			this->animationBlender->init(this->animationNames[0]->getListSelectedValue(), false);
			this->animationBlender->blend(this->animationNames[0]->getListSelectedValue(), this->mapStringToBlendingTransition(this->animationBlendTransitions[0]->getListSelectedValue()),
				0.2f, true);
		}
	}

	void AnimationSequenceComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		// First deactivate
		if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
		{
			// disable animation
			this->resetAnimation();
		}

		if (true == this->bIsInSimulation && true == activated)
		{
			this->activateAnimation();
		}
	}

	bool AnimationSequenceComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void AnimationSequenceComponent::setRepeat(bool animationRepeat)
	{
		this->animationRepeat->setValue(animationRepeat);
		this->setActivated(this->activated->getBool());
	}

	bool AnimationSequenceComponent::getRepeat(void) const
	{
		return this->animationRepeat->getBool();
	}

	void AnimationSequenceComponent::setShowSkeleton(bool showSkeleton)
	{
		this->showSkeleton->setValue(showSkeleton);
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		// entity->setDisplaySkeleton(this->showSkeleton->getBool());

		/*if (true == showSkeleton)
		{
			this->skeletonVisualizer = new SkeletonVisualizer(entity, this->gameObjectPtr->getSceneManager(), WorkspaceModule::getInstance()->getWorkspace()->getDefaultCamera());
		}*/
	}

	bool AnimationSequenceComponent::getShowSkeleton(void) const
	{
		return this->showSkeleton->getBool();
	}

	void AnimationSequenceComponent::setAnimationCount(unsigned int animationCount)
	{
		this->animationCount->setValue(animationCount);

		size_t oldSize = this->animationNames.size();

		if (animationCount > oldSize)
		{
			// Resize the waypoints array for count
			this->animationNames.resize(animationCount);
			this->animationBlendTransitions.resize(animationCount);
			this->animationDurations.resize(animationCount);
			this->animationTimePositions.resize(animationCount);
			this->animationSpeeds.resize(animationCount);

			for (size_t i = oldSize; i < animationCount; i++)
			{
				this->animationNames[i] = new Variant(AnimationSequenceComponent::AttrAnimationName() + Ogre::StringConverter::toString(i), this->availableAnimations, this->attributes);
				this->animationNames[i]->addUserData(GameObject::AttrActionNeedRefresh());
				this->animationBlendTransitions[i] = new Variant(AnimationSequenceComponent::AttrBlendTransition() + Ogre::StringConverter::toString(i), { "BlendWhileAnimating", "BlendSwitch", "BlendThenAnimate" }, this->attributes);
				// Here get from animation name its duration
				this->animationDurations[i] = new Variant(AnimationSequenceComponent::AttrDuration() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
				this->setAnimationName(i, this->animationNames[i]->getListSelectedValue());

				this->animationTimePositions[i] = new Variant(AnimationSequenceComponent::AttrTimePosition() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
				this->animationSpeeds[i] = new Variant(AnimationSequenceComponent::AttrSpeed() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->animationSpeeds[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (animationCount < oldSize)
		{
			this->eraseVariants(this->animationNames, animationCount);
			this->eraseVariants(this->animationBlendTransitions, animationCount);
			this->eraseVariants(this->animationDurations, animationCount);
			this->eraseVariants(this->animationTimePositions, animationCount);
			this->eraseVariants(this->animationSpeeds, animationCount);
		}
	}

	unsigned int AnimationSequenceComponent::getAnimationCount(void) const
	{
		return this->animationCount->getUInt();
	}

	void AnimationSequenceComponent::setAnimationName(unsigned int index, const Ogre::String& animationName)
	{
		if (index > this->animationNames.size())
			index = static_cast<unsigned int>(this->animationNames.size()) - 1;

		this->animationNames[index]->setListSelectedValue(animationName);

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			this->animationBlender = new NOWA::AnimationBlender(entity);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] List all animations for mesh '" + entity->getMesh()->getName() + "':");
			Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();

			if (true == set->hasAnimationState(animationName))
			{
				Ogre::Real duration = set->getAnimationState(animationName)->getLength();
				this->animationDurations[index]->setValue(duration);
			}
		}
		// Attention: Is this correct?
		this->setActivated(this->activated->getBool());
	}

	Ogre::String AnimationSequenceComponent::getAnimationName(unsigned int index) const
	{
		if (index > this->animationNames.size())
			return "";
		return this->animationNames[index]->getListSelectedValue();
	}

	void AnimationSequenceComponent::setBlendTransition(unsigned int index, const Ogre::String& strBlendTransition)
	{
		if (index > this->animationBlendTransitions.size())
			index = static_cast<unsigned int>(this->animationBlendTransitions.size()) - 1;

		this->animationBlendTransitions[index]->setListSelectedValue(strBlendTransition);
	}

	Ogre::String AnimationSequenceComponent::getBlendTransition(unsigned int index) const
	{
		if (index > this->animationBlendTransitions.size())
			return "";
		return this->animationBlendTransitions[index]->getListSelectedValue();
	}

	void AnimationSequenceComponent::setDuration(unsigned int index, Ogre::Real duration)
	{
		if (index > this->animationDurations.size())
			index = static_cast<unsigned int>(this->animationDurations.size()) - 1;

		this->animationDurations[index]->setValue(duration);
	}

	Ogre::Real AnimationSequenceComponent::getDuration(unsigned int index)
	{
		if (index > this->animationDurations.size())
			return -1.0f;
		return this->animationDurations[index]->getReal();
	}

	void AnimationSequenceComponent::setTimePosition(unsigned int index, Ogre::Real timePosition)
	{
		if (index > this->animationTimePositions.size())
			index = static_cast<unsigned int>(this->animationTimePositions.size()) - 1;

		this->animationTimePositions[index]->setValue(timePosition);
	}

	Ogre::Real AnimationSequenceComponent::getTimePosition(unsigned int index)
	{
		if (index > this->animationTimePositions.size())
			return -1.0f;
		return this->animationTimePositions[index]->getReal();
	}

	void AnimationSequenceComponent::setSpeed(unsigned int index, Ogre::Real animationSpeed)
	{
		if (index > this->animationSpeeds.size())
			index = static_cast<unsigned int>(this->animationSpeeds.size()) - 1;

		this->animationSpeeds[index]->setValue(animationSpeed);
	}

	Ogre::Real AnimationSequenceComponent::getSpeed(unsigned int index) const
	{
		if (index > this->animationSpeeds.size())
			return -1.0f;
		return this->animationSpeeds[index]->getReal();
	}

	AnimationBlender* AnimationSequenceComponent::getAnimationBlender(void) const
	{
		return this->animationBlender;
	}

	Ogre::v1::OldBone* AnimationSequenceComponent::getBone(const Ogre::String& boneName)
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

	Ogre::String AnimationSequenceComponent::mapBlendingTransitionToString(AnimationBlender::BlendingTransition blendingTransition)
	{
		Ogre::String strBlendingTransition = "BlendWhileAnimating";
		switch (blendingTransition)
		{
		case AnimationBlender::BlendSwitch:
			strBlendingTransition = "BlendSwitch";
			break;
		case AnimationBlender::BlendThenAnimate:
			strBlendingTransition = "BlendThenAnimate";
			break;
		}
		return strBlendingTransition;
	}

	AnimationBlender::BlendingTransition AnimationSequenceComponent::mapStringToBlendingTransition(const Ogre::String& strBlendingTransition)
	{
		AnimationBlender::BlendingTransition blendingTransition = AnimationBlender::BlendWhileAnimating;
		if ("BlendSwitch" == strBlendingTransition)
			blendingTransition = AnimationBlender::BlendSwitch;
		else if ("BlendThenAnimate" == strBlendingTransition)
			blendingTransition = AnimationBlender::BlendThenAnimate;
		return blendingTransition;
	}

	// Lua registration part

	AnimationSequenceComponent* getAnimationSequenceComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<AnimationSequenceComponent>(gameObject->getComponentWithOccurrence<AnimationSequenceComponent>(occurrenceIndex)).get();
	}

	AnimationSequenceComponent* getAnimationSequenceComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AnimationSequenceComponent>(gameObject->getComponent<AnimationSequenceComponent>()).get();
	}

	AnimationSequenceComponent* getAnimationSequenceComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AnimationSequenceComponent>(gameObject->getComponentFromName<AnimationSequenceComponent>(name)).get();
	}

	void AnimationSequenceComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<AnimationSequenceComponent, GameObjectComponent>("AnimationSequenceComponent")
			// .def("getClassName", &AnimationSequenceComponent::getClassName)
			.def("getParentClassName", &AnimationSequenceComponent::getParentClassName)
			// .def("clone", &AnimationSequenceComponent::clone)
			// .def("getClassId", &AnimationSequenceComponent::getClassId)
			// .def("getParentClassId", &AnimationSequenceComponent::getParentClassId)
			.def("setActivated", &AnimationSequenceComponent::setActivated)
			.def("isActivated", &AnimationSequenceComponent::isActivated)
			// .def("setAnimationCount", &AnimationSequenceComponent::setAnimationCount)
			.def("getAnimationCount", &AnimationSequenceComponent::getAnimationCount)
			// .def("setAnimationName", &AnimationSequenceComponent::setAnimationName)
			.def("getAnimationName", &AnimationSequenceComponent::getAnimationName)
			.def("setBlendTransition", &AnimationSequenceComponent::setBlendTransition)
			.def("getBlendTransition", &AnimationSequenceComponent::getBlendTransition)
			.def("setDuration", &AnimationSequenceComponent::setDuration)
			.def("getDuration", &AnimationSequenceComponent::getDuration)
			.def("setTimePosition", &AnimationSequenceComponent::setTimePosition)
			.def("getTimePosition", &AnimationSequenceComponent::getTimePosition)
			.def("setSpeed", &AnimationSequenceComponent::setSpeed)
			.def("getSpeed", &AnimationSequenceComponent::getSpeed)
			.def("setRepeat", &AnimationSequenceComponent::setRepeat)
			.def("getRepeat", &AnimationSequenceComponent::getRepeat)
			.def("getAnimationBlender", &AnimationSequenceComponent::getAnimationBlender)
			.def("getBone", &AnimationSequenceComponent::getBone)
		];

		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "class inherits GameObjectComponent", AnimationSequenceComponent::getStaticInfoText());
		// LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "String getClassName()", "Gets the class name of this component as string.");
		// LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "String getParentClassName()", "Gets the parent class name (the one this component is derived from) of this component as string.");
		// LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "number getClassId()", "Gets the class id of this component.");
		// LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "number getParentClassId()", "Gets the parent class id (the one this component is derived from) of this component.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not (Start the animations).");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "int getAnimationCount()", "Gets the number of used animations.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setBlendTransition(int index, String strBlendTransition)", "Sets the blending transition for the given animation by index. Possible values are: 'BlendWhileAnimating', 'BlendSwitch', 'BlendThenAnimate'");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "String getBlendTransition(int index)", "Gets the blending transition for the given animation by index. Possible values are: 'BlendWhileAnimating', 'BlendSwitch', 'BlendThenAnimate'");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setDuration(int index, float duration)", "Sets the duration in seconds for the given animation by index.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "float getDuration(int index)", "Gets the duration in seconds for the given animation by index.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setTimePosition(int index, float timePosition)", "Sets the time position in seconds for the given animation by index.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "float getTimePosition(int index)", "Gets the time position in seconds for the given animation by index.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setSpeed(int index, float speed)", "Sets the speed for the given animation by index.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "float getSpeed(int index)", "Gets the speed for the given animation by index.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the whole sequence, after it has been played.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "bool getRepeat()", "Gets whether the whole sequence is repeated, after it has been played.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "AnmationBlender getAnimationBlender()", "Gets animation blender to manipulate animations directly.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "Bone getBone(String boneName)", "Gets the bone by the given bone name for direct manipulation. Nil is delivered, if the bone name does not exist.");

		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "class inherits GameObjectComponent", AnimationSequenceComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getAnimationSequenceComponentFromName", &getAnimationSequenceComponentFromName);
		gameObjectClass.def("getAnimationSequenceComponent", (AnimationSequenceComponent * (*)(GameObject*)) & getAnimationSequenceComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationSequenceComponent getAnimationSequenceComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationSequenceComponent getAnimationSequenceComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castAnimationSequenceComponent", &GameObjectController::cast<AnimationSequenceComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AnimationSequenceComponent castAnimationSequenceComponent(AnimationSequenceComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool AnimationSequenceComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		auto animationSequenceCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<AnimationSequenceComponent>());
		if (nullptr != animationSequenceCompPtr)
		{
			return false;
		}

		// Check if the entity has at least one animation and no player controller, else animation component is senseless
		auto playerControllerCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlayerControllerComponent>());
		Ogre::v1::Entity* entity = gameObject->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity && nullptr == playerControllerCompPtr)
		{
			Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
			if (nullptr != set)
			{
				Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
				// list all animations
				if (it.hasMoreElements())
				{
					return true;
				}
			}
		}
		return false;
	}

}; //namespace end