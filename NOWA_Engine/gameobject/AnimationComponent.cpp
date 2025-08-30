#include "NOWAPrecompiled.h"
#include "AnimationComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "PlayerControllerComponents.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AnimationComponent::AnimationBlenderObserver::AnimationBlenderObserver(luabind::object closureFunction, bool oneTime)
		: AnimationBlender::IAnimationBlenderObserver(),
		closureFunction(closureFunction),
		oneTime(oneTime)
	{

	}

	AnimationComponent::AnimationBlenderObserver::~AnimationBlenderObserver()
	{

	}

	void AnimationComponent::AnimationBlenderObserver::onAnimationFinished(void)
	{
		if (this->closureFunction.is_valid())
		{
			NOWA::AppStateManager::LogicCommand logicCommand = [this]()
				{
					try
					{
						luabind::call_function<void>(this->closureFunction);
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[AnimationComponent::AnimationBlenderObserver] Caught error in 'reactOnAnimationFinished' Error: " + Ogre::String(error.what())
							+ " details: " + msg.str());
					}
				};
			NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		}
	}

	bool AnimationComponent::AnimationBlenderObserver::shouldReactOneTime(void) const
	{
		return this->oneTime;
	}

	void AnimationComponent::AnimationBlenderObserver::setNewFunctionName(luabind::object closureFunction, bool oneTime)
	{
		this->closureFunction = closureFunction;
		this->oneTime = oneTime;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	AnimationComponent::AnimationComponent()
		: GameObjectComponent(),
		activated(new Variant(AnimationComponent::AttrActivated(), true, this->attributes)),
		animationName(new Variant(AnimationComponent::AttrName(), std::vector<Ogre::String>(), this->attributes)),
		animationSpeed(new Variant(AnimationComponent::AttrSpeed(), 1.0f, this->attributes)),
		animationRepeat(new Variant(AnimationComponent::AttrRepeat(), true, this->attributes)),
		showSkeleton(new Variant(AnimationComponent::AttrShowSkeleton(), false, this->attributes)),
		animationBlender(nullptr),
		skeletonVisualizer(nullptr)
	{
		
	}

	AnimationComponent::~AnimationComponent()
	{
		
	}

	bool AnimationComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		ENQUEUE_RENDER_COMMAND("AnimationComponent::createAnimationBlender",
		{
			Ogre::v1::Entity * entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
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
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationComponent] It seems, that this game object: '" + this->gameObjectPtr->getName()
					+ "' is using the wrong animation component type, as it has no v1::entity but item. Please use AnimationComponent! So no animations will work so far.");
			}
		});
	}

	bool AnimationComponent::connect(void)
	{
		GameObjectComponent::connect();
		if (true == this->activated->getBool())
		{
			this->activateAnimation();
		}

		if (nullptr != this->animationBlender)
		{
			this->animationBlender->setDebugLog(this->bShowDebugData);
		}

		return true;
	}

	bool AnimationComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();
		this->resetAnimation();

		return true;
	}

	void AnimationComponent::onRemoveComponent(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Destructor animation component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->animationBlender)
		{
			this->animationBlender->deleteAllObservers();
			delete this->animationBlender;
			this->animationBlender = nullptr;
		}
		if (nullptr != this->skeletonVisualizer)
		{
			delete this->skeletonVisualizer;
			this->skeletonVisualizer = nullptr;
		}
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
		if (true == this->activated->getBool() && false == notSimulating)
		{
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationComponent] weight: " + Ogre::StringConverter::toString(this->animationState->getWeight()));
			if (nullptr != this->animationBlender)
			{
				this->animationBlender->addTime(dt * this->animationSpeed->getReal() / this->animationBlender->getSource()->getLength());
			}
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
			ENQUEUE_RENDER_COMMAND("AnimationComponent::resetAnimation",
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
			});
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

	void AnimationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationActivated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShowSkeleton"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showSkeleton->getBool())));
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
					// this->resetAnimation();
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
			// this->resetAnimation();
		}

		if (true == this->bConnected && true == activated)
		{
			this->activateAnimation();
		}
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

	void AnimationComponent::reactOnAnimationFinished(luabind::object closureFunction, bool oneTime)
	{
		if (nullptr == this->animationBlender)
		{
			return;
		}

		AnimationBlenderObserver* newObserver = new AnimationBlenderObserver(closureFunction, oneTime);
		this->animationBlender->addAnimationBlenderObserver(newObserver);
	}

	// Lua registration part

	AnimationComponent* getAnimationComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<AnimationComponent>(gameObject->getComponentWithOccurrence<AnimationComponent>(occurrenceIndex)).get();
	}

	AnimationComponent* getAnimationComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AnimationComponent>(gameObject->getComponent<AnimationComponent>()).get();
	}

	AnimationComponent* getAnimationComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AnimationComponent>(gameObject->getComponentFromName<AnimationComponent>(name)).get();
	}

	void AnimationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<AnimationComponent, GameObjectComponent>("AnimationComponent")
			// .def("getClassName", &AnimationComponent::getClassName)
			.def("getParentClassName", &AnimationComponent::getParentClassName)
			// .def("clone", &AnimationComponent::clone)
			// .def("getClassId", &AnimationComponent::getClassId)
			// .def("getParentClassId", &AnimationComponent::getParentClassId)
			.def("setActivated", &AnimationComponent::setActivated)
			.def("isActivated", &AnimationComponent::isActivated)
			.def("setAnimationName", &AnimationComponent::setAnimationName)
			.def("getAnimationName", &AnimationComponent::getAnimationName)
			.def("setSpeed", &AnimationComponent::setSpeed)
			.def("getSpeed", &AnimationComponent::getSpeed)
			.def("setRepeat", &AnimationComponent::setRepeat)
			.def("getRepeat", &AnimationComponent::getRepeat)
			.def("isComplete", &AnimationComponent::isComplete)
			.def("getAnimationBlender", &AnimationComponent::getAnimationBlender)
			.def("getBone", &AnimationComponent::getBone)
			.def("setTimePosition", &AnimationComponent::setTimePosition)
			.def("getTimePosition", &AnimationComponent::getTimePosition)
			.def("getLength", &AnimationComponent::getLength)
			.def("setWeight", &AnimationComponent::setWeight)
			.def("getWeight", &AnimationComponent::getWeight)
			.def("getLocalToWorldPosition", &AnimationComponent::getLocalToWorldPosition)
			.def("getLocalToWorldOrientation", &AnimationComponent::getLocalToWorldOrientation)
			.def("reactOnAnimationFinished", &AnimationComponent::reactOnAnimationFinished)
		];

		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "class inherits GameObjectComponent", AnimationComponent::getStaticInfoText());
		// LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "String getClassName()", "Gets the class name of this component as string.");
		// LuaScriptApi::getInstance()->addClassToCollection(("AnimationComponent", "String getParentClassName()", "Gets the parent class name (the one this component is derived from) of this component as string.");
		// LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "number getClassId()", "Gets the class id of this component.");
		// LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "number getParentClassId()", "Gets the parent class id (the one this component is derived from) of this component.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not (Start the animations).");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "void setAnimationName(String animationName)", "Sets the to be played animation name. If it does not exist, the animation cannot be played later.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "String getAnimationName()", "Gets currently used animation name.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "void setSpeed(float speed)", "Sets the animation speed for the current animation.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "float getSpeed()", "Gets the animation speed for currently used animation.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "void setRepeat(bool repeat)", "Sets whether the current animation should be repeated when finished.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "bool getRepeat()", "Gets whether the current animation will be repeated when finished.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "bool isComplete()", "Gets whether the current animation has finished.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "AnmationBlender getAnimationBlender()", "Gets animation blender to manipulate animations directly.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "Bone getBone(String boneName)", "Gets the bone by the given bone name for direct manipulation. Nil is delivered, if the bone name does not exist.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "void setTimePosition(float timePosition)", "Sets the time position for the animation.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "float getTimePosition()", "Gets the current animation time position.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "float getLength()", "Gets the animation length.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "void setWeight(float weight)", "Sets the animation weight. The more less the weight the more less all bones are moved.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "float getWeight()", "Gets the current animation weight.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponent", "void reactOnAnimationFinished(func closureFunction, bool oneTime)",
			"Sets whether to react when the given animation has finished.");

		gameObjectClass.def("getAnimationComponentFromName", &getAnimationComponentFromName);
		gameObjectClass.def("getAnimationComponent", (AnimationComponent * (*)(GameObject*)) & getAnimationComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getAnimationComponent2", (AnimationComponent * (*)(GameObject*, unsigned int)) & getAnimationComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponent getAnimationComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponent getAnimationComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponent getAnimationComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castAnimationComponent", &GameObjectController::cast<AnimationComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AnimationComponent castAnimationComponent(AnimationComponent other)", "Casts an incoming type from function for lua auto completion.");
	}
	
}; // namespace end