#include "NOWAPrecompiled.h"
#include "AnimationComponentV2.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PlayerControllerComponents.h"

#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AnimationComponentV2::AnimationBlenderObserver::AnimationBlenderObserver(luabind::object closureFunction, bool oneTime)
		: AnimationBlenderV2::IAnimationBlenderObserver(),
		closureFunction(closureFunction),
		oneTime(oneTime)
	{

	}

	AnimationComponentV2::AnimationBlenderObserver::~AnimationBlenderObserver()
	{

	}

	void AnimationComponentV2::AnimationBlenderObserver::onAnimationFinished(void)
	{
		if (this->closureFunction.is_valid())
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

				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnAnimationFinished' Error: " + Ogre::String(error.what())
															+ " details: " + msg.str());
			}
		}
	}

	bool AnimationComponentV2::AnimationBlenderObserver::shouldReactOneTime(void) const
	{
		return this->oneTime;
	}

	void AnimationComponentV2::AnimationBlenderObserver::setNewFunctionName(luabind::object closureFunction, bool oneTime)
	{
		this->closureFunction = closureFunction;
		this->oneTime = oneTime;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	AnimationComponentV2::AnimationComponentV2()
		: GameObjectComponent(),
		name("AnimationComponentV2"),
		skeleton(nullptr),
		isInSimulation(false),
		animationBlender(nullptr),
		activated(new Variant(AnimationComponentV2::AttrActivated(), true, this->attributes)),
		animationName(new Variant(AnimationComponentV2::AttrName(), std::vector<Ogre::String>(), this->attributes)),
		animationSpeed(new Variant(AnimationComponentV2::AttrSpeed(), 1.0f, this->attributes)),
		animationRepeat(new Variant(AnimationComponentV2::AttrRepeat(), true, this->attributes)),
		animationBlenderObserver(nullptr)
	{
		
	}

	AnimationComponentV2::~AnimationComponentV2(void)
	{
		
	}

	void AnimationComponentV2::initialise()
	{

	}

	const Ogre::String& AnimationComponentV2::getName() const
	{
		return this->name;
	}

	void AnimationComponentV2::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<AnimationComponentV2>(AnimationComponentV2::getStaticClassId(), AnimationComponentV2::getStaticClassName());
	}

	void AnimationComponentV2::shutdown()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void AnimationComponentV2::uninstall()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}
	
	void AnimationComponentV2::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool AnimationComponentV2::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
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
		
		return true;
	}

	GameObjectCompPtr AnimationComponentV2::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AnimationComponentV2Ptr clonedCompPtr(boost::make_shared<AnimationComponentV2>());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->createAnimationBlender();
		clonedCompPtr->setAnimationName(this->animationName->getListSelectedValue());
		clonedCompPtr->setSpeed(this->animationSpeed->getReal());
		clonedCompPtr->setRepeat(this->animationRepeat->getBool());

		// Activation after everything is set, because the game object is required
		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AnimationComponentV2::postInit(void)
	{
		// Do not set things a second time after cloning
		if (nullptr != this->animationBlender)
		{
			return true;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponentV2] Init animation component v2 for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createAnimationBlender();
		return true;
	}

	void AnimationComponentV2::createAnimationBlender(void)
	{
		Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
		if (nullptr != item)
		{
			std::vector<Ogre::String> animationNames;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationBlenderV2] List all animations for mesh '" + item->getMesh()->getName() + "':");

			this->skeleton = item->getSkeletonInstance();
			if (nullptr == this->skeleton)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Animation Blender V2] Cannot initialize animation blender, because the skeleton resource for item: "
																+ item->getName() + " is missing!");
				return;
			}
			else
			{
				for (auto& anim : this->skeleton->getAnimationsNonConst())
				{
					anim.setEnabled(false);
					anim.mWeight = 0.0f;
					anim.setTime(0.0f);
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponentV2] Animation name: " + anim.getName().getFriendlyText()
																	+ " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
					animationNames.emplace_back(anim.getName().getFriendlyText());
				}

				// Add all available animation names to list
				this->animationName->setValue(animationNames);

				this->animationBlender = new AnimationBlenderV2(item);
				this->animationBlender->init(this->animationName->getListSelectedValue(), this->animationRepeat->getBool());
			}
		}
	}

	bool AnimationComponentV2::connect(void)
	{
		if (true == this->activated->getBool())
		{
			this->activateAnimation();
		}
		return true;
	}

	bool AnimationComponentV2::disconnect(void)
	{
		this->resetAnimation();
		return true;
	}

	bool AnimationComponentV2::onCloned(void)
	{
		
		return true;
	}

	void AnimationComponentV2::onRemoveComponent(void)
	{
		if (nullptr != this->animationBlenderObserver)
		{
			delete this->animationBlenderObserver;
			this->animationBlenderObserver = nullptr;
		}
		if (nullptr != this->animationBlender)
		{
			delete this->animationBlender;
			this->animationBlender = nullptr;
		}
	}
	
	void AnimationComponentV2::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void AnimationComponentV2::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void AnimationComponentV2::update(Ogre::Real dt, bool notSimulating)
	{
		this->isInSimulation = !notSimulating;

		if (true == this->activated->getBool() && false == notSimulating)
		{
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationComponentV2] weight: " + Ogre::StringConverter::toString(this->animationState->getWeight()));
			if (nullptr != this->animationBlender)
			{
				this->animationBlender->addTime(dt * this->animationSpeed->getReal() / this->animationBlender->getSource()->getDuration());
			}
		}
		/*else if (true == notSimulating && nullptr != this->animationBlender)
		{
			this->resetAnimation();
		}*/
	}

	void AnimationComponentV2::resetAnimation(void)
	{
		if (nullptr != this->animationBlender)
		{
			if (nullptr != this->animationBlender->getSource())
			{
				this->animationBlender->getSource()->setEnabled(false);
				this->animationBlender->getSource()->mWeight = 0.0f;
				this->animationBlender->getSource()->setTime(0.0f);
			}
			if (nullptr != this->animationBlender->getTarget())
			{
				this->animationBlender->getTarget()->setEnabled(false);
				this->animationBlender->getTarget()->mWeight = 0.0f;
				this->animationBlender->getTarget()->setTime(0.0f);
			}
		}
	}

	void AnimationComponentV2::actualizeValue(Variant* attribute)
	{
		this->resetAnimation();

		GameObjectComponent::actualizeValue(attribute);

		if (AnimationComponentV2::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AnimationComponentV2::AttrName() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue());
		}
		else if (AnimationComponentV2::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (AnimationComponentV2::AttrSpeed() == attribute->getName())
		{
			this->animationSpeed->setValue(attribute->getReal());
		}
	}

	void AnimationComponentV2::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
	}

	Ogre::String AnimationComponentV2::getClassName(void) const
	{
		return "AnimationComponentV2";
	}

	Ogre::String AnimationComponentV2::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void AnimationComponentV2::activateAnimation(void)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		if (false == this->animationName->getListSelectedValue().empty())
		{

			if (nullptr != this->skeleton)
			{
				Ogre::SkeletonAnimation* animationState = this->skeleton->getAnimation(this->animationName->getListSelectedValue());
				if (nullptr != animationState)
				{
					this->resetAnimation();
					// this->animationState->setEnabled(true);
					// this->animationState->setWeight(5.0f);
					// this->animationState->setTimePosition(0.0f);
					// this->animationBlender->init(this->animationName->getListSelectedValue(), this->animationRepeat->getBool());
					this->animationBlender->blend(this->animationName->getListSelectedValue(), NOWA::AnimationBlenderV2::BlendThenAnimate, 0.2f, this->animationRepeat->getBool());
				}
				else
				{
					Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
					if (nullptr != item)
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationComponentV2] Error: The item: " + item->getName()
																		+ " has no animation: " + this->animationName->getListSelectedValue());
						for (auto& anim : this->skeleton->getAnimationsNonConst())
						{
							anim.setEnabled(false);
							anim.mWeight = 0.0f;
							anim.setTime(0.0f);
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationComponentV2] Available Animations: ");
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationComponentV2] Animation name: " + anim.getName().getFriendlyText()
																			+ " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
						}
					}
					else
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationComponentV2] Error: The game object does not possess an item for animations.");
					}
				}
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponentV2] Warning: The given animation name is empty.");
		}
	}

	bool AnimationComponentV2::isComplete(void) const
	{
		if (nullptr == this->animationBlender)
			return true;
		return this->animationBlender->isComplete();
	}

	void AnimationComponentV2::setActivated(bool activated)
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

	bool AnimationComponentV2::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void AnimationComponentV2::setAnimationName(const Ogre::String& animationName)
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

	const Ogre::String AnimationComponentV2::getAnimationName(void) const
	{
		return this->animationName->getListSelectedValue();
	}

	void AnimationComponentV2::setSpeed(Ogre::Real animationSpeed)
	{
		this->animationSpeed->setValue(animationSpeed);
	}

	Ogre::Real AnimationComponentV2::getSpeed(void) const
	{
		return this->animationSpeed->getReal();
	}

	void AnimationComponentV2::setRepeat(bool animationRepeat)
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

	bool AnimationComponentV2::getRepeat(void) const
	{
		return this->animationRepeat->getBool();
	}

	AnimationBlenderV2* AnimationComponentV2::getAnimationBlender(void) const
	{
		return this->animationBlender;
	}

	Ogre::Bone* AnimationComponentV2::getBone(const Ogre::String& boneName)
	{
		if (nullptr != this->skeleton)
		{
			if (true == skeleton->hasBone(boneName))
			{
				return skeleton->getBone(boneName);
			}
		}
		return nullptr;
	}

	Ogre::Vector3 AnimationComponentV2::getLocalToWorldPosition(Ogre::Bone* bone)
	{
		return this->animationBlender->getLocalToWorldPosition(bone);
	}

	Ogre::Quaternion AnimationComponentV2::getLocalToWorldOrientation(Ogre::Bone* bone)
	{
		return this->animationBlender->getLocalToWorldOrientation(bone);
	}

	void AnimationComponentV2::setTimePosition(Ogre::Real timePosition)
	{
		if (nullptr != this->animationBlender)
		{
			this->animationBlender->getSource()->setTime(timePosition);
		}
	}

	Ogre::Real AnimationComponentV2::getTimePosition(void) const
	{
		if (nullptr != this->animationBlender)
		{
			return this->animationBlender->getSource()->getCurrentTime();
		}
		return 0.0f;
	}

	Ogre::Real AnimationComponentV2::getLength(void) const
	{
		if (nullptr != this->animationBlender)
		{
			return this->animationBlender->getSource()->getDuration();
		}
		return 0.0f;
	}

	void AnimationComponentV2::setWeight(Ogre::Real weight)
	{
		if (nullptr != this->animationBlender)
		{
			this->animationBlender->getSource()->mWeight = weight;
		}
	}

	Ogre::Real AnimationComponentV2::getWeight(void) const
	{
		if (nullptr != this->animationBlender)
		{
			return this->animationBlender->getSource()->mWeight;
		}
		return 0.0f;
	}

	// Lua registration part

	AnimationComponentV2* getAnimationComponentV2(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<AnimationComponentV2>(gameObject->getComponentWithOccurrence<AnimationComponentV2>(occurrenceIndex)).get();
	}

	AnimationComponentV2* getAnimationComponentV2(GameObject* gameObject)
	{
		return makeStrongPtr<AnimationComponentV2>(gameObject->getComponent<AnimationComponentV2>()).get();
	}

	AnimationComponentV2* getAnimationComponentV2FromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AnimationComponentV2>(gameObject->getComponentFromName<AnimationComponentV2>(name)).get();
	}

	void AnimationComponentV2::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<AnimationComponentV2, GameObjectComponent>("AnimationComponentV2")
			.def("setActivated", &AnimationComponentV2::setActivated)
			.def("isActivated", &AnimationComponentV2::isActivated)
			.def("reactOnAnimationFinished", &AnimationComponentV2::reactOnAnimationFinished)
		];

		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponentV2", "class inherits GameObjectComponent", AnimationComponentV2::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponentV2", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponentV2", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("AnimationComponentV2", "void reactOnAnimationFinished(func closureFunction, bool oneTime)",
							 "Sets whether to react when the given animation has finished.");

		gameObjectClass.def("getAnimationComponentV2FromName", &getAnimationComponentV2FromName);
		gameObjectClass.def("getAnimationComponentV2", (AnimationComponentV2 * (*)(GameObject*)) & getAnimationComponentV2);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getAnimationComponentV22", (AnimationComponentV2 * (*)(GameObject*, unsigned int)) & getAnimationComponentV2);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponentV2 getAnimationComponentV22(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponentV2 getAnimationComponentV2()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponentV2 getAnimationComponentV2FromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castAnimationComponentV2", &GameObjectController::cast<AnimationComponentV2>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AnimationComponentV2 castAnimationComponentV2(AnimationComponentV2 other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool AnimationComponentV2::canStaticAddComponent(GameObject* gameObject)
	{
		// Check if the entity has at least one animation and no player controller, else animation component is senseless
		auto playerControllerCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlayerControllerComponent>());
		Ogre::Item* item = gameObject->getMovableObject<Ogre::Item>();
		if (nullptr != item && nullptr == playerControllerCompPtr)
		{
			Ogre::SkeletonInstance* skeleton = item->getSkeletonInstance();
			if (nullptr != skeleton)
			{
				if (false == skeleton->getAnimations().empty())
				{
					return true;
				}
			}
		}
		return false;
	}

	void AnimationComponentV2::reactOnAnimationFinished(luabind::object closureFunction, bool oneTime)
	{
		if (nullptr == this->animationBlender)
		{
			return;
		}

		if (nullptr == this->animationBlenderObserver)
		{
			this->animationBlenderObserver = new AnimationBlenderObserver(closureFunction, oneTime);
		}
		else
		{
			static_cast<AnimationBlenderObserver*>(this->animationBlenderObserver)->setNewFunctionName(closureFunction, oneTime);
		}
		// Note: animation blender will delete observer automatically
		this->animationBlender->setAnimationBlenderObserver(this->animationBlenderObserver);
	}

}; //namespace end