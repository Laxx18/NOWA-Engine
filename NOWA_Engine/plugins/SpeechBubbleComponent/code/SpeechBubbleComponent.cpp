#include "NOWAPrecompiled.h"
#include "SpeechBubbleComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/GameObjectTitleComponent.h"
#include "gameobject/SimpleSoundComponent.h"

#include "OgreSimpleSpline.h"
#include "OgreAbiUtils.h"

namespace
{
	Ogre::String replaceAll(Ogre::String str, const Ogre::String& from, const Ogre::String& to)
	{
		size_t startPos = 0;
		while ((startPos = str.find(from, startPos)) != Ogre::String::npos)
		{
			str.replace(startPos, from.length(), to);
			startPos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}

	Ogre::String mid(Ogre::String& str, unsigned short pos1, unsigned short pos2)
	{
		unsigned short i;
		Ogre::String temp;
		for (i = pos1; i < pos2; i++)
		{
			temp += str[i];
		}

		return temp;
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	SpeechBubbleComponent::SpeechBubbleComponent()
		: GameObjectComponent(),
		name("SpeechBubbleComponent"),
		lineNode(nullptr),
		manualObject(nullptr),
		gameObjectTitleComponent(nullptr),
		simpleSoundComponent(nullptr),
		indices(0),
		currentCaptionWidth(0.0f),
		currentCaptionHeight(0.0f),
		currentCharIndex(0),
		timeSinceLastRun(0.0f),
		couldDraw(false),
		speechDone(false),
		bIsInSimulation(false),
		activated(new Variant(SpeechBubbleComponent::AttrActivated(), true, this->attributes)),
		caption(new Variant(SpeechBubbleComponent::AttrCaption(), "MyCaption", this->attributes)),
		runSpeech(new Variant(SpeechBubbleComponent::AttrRunSpeech(), false, this->attributes)),
		speechDuration(new Variant(SpeechBubbleComponent::AttrSpeechDuration(), 10.0f, this->attributes)),
		runSpeechSound(new Variant(SpeechBubbleComponent::AttrRunSpeechSound(), false, this->attributes)),
		keepCaption(new Variant(SpeechBubbleComponent::AttrKeepCaption(), false, this->attributes))
	{
		this->runSpeech->setDescription("Sets whether the caption should remain after the speech run.");
		this->speechDuration->setDescription("Sets the speed duration. That is how long the bubble shall remain in seconds.");
		this->runSpeechSound->setDescription("Sets whether to use a sound if the speech is running char by char.");
		this->keepCaption->setDescription("Sets whether to use a sound if the speech is running char by char.");
	}

	SpeechBubbleComponent::~SpeechBubbleComponent(void)
	{

	}

	void SpeechBubbleComponent::initialise()
	{

	}

	const Ogre::String& SpeechBubbleComponent::getName() const
	{
		return this->name;
	}

	void SpeechBubbleComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<SpeechBubbleComponent>(SpeechBubbleComponent::getStaticClassId(), SpeechBubbleComponent::getStaticClassName());
	}

	void SpeechBubbleComponent::shutdown()
	{

	}

	void SpeechBubbleComponent::uninstall()
	{

	}

	void SpeechBubbleComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool SpeechBubbleComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Caption")
		{
			this->caption->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RunSpeech")
		{
			this->runSpeech->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpeechDuration")
		{
			this->speechDuration->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RunSpeechSound")
		{
			this->runSpeechSound->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "KeepCaption")
		{
			this->keepCaption->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr SpeechBubbleComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		SpeechBubbleComponentPtr clonedCompPtr(boost::make_shared<SpeechBubbleComponent>());

		clonedCompPtr->setCaption(this->caption->getString());
		clonedCompPtr->setRunSpeech(this->runSpeech->getBool());
		clonedCompPtr->setSpeechDuration(this->speechDuration->getReal());
		clonedCompPtr->setRunSpeechSound(this->runSpeechSound->getBool());
		clonedCompPtr->setKeepCaption(this->keepCaption->getBool());

		clonedCompPtr->setActivated(this->activated->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool SpeechBubbleComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SpeechBubbleComponent] Init component for game object: " + this->gameObjectPtr->getName());

		auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<GameObjectTitleComponent>());
		if (nullptr != gameObjectTitleCompPtr)
		{
			this->gameObjectTitleComponent = gameObjectTitleCompPtr.get();
			auto captionAttribute = this->gameObjectTitleComponent->getAttribute(SpeechBubbleComponent::AttrCaption());
			if (nullptr != captionAttribute)
			{
				captionAttribute->setVisible(false);
			}
		}

		return true;
	}

	bool SpeechBubbleComponent::connect(void)
	{
		GameObjectComponent::connect();

		bool success = GameObjectComponent::connect();
		if (false == success)
		{
			return success;
		}

		auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<GameObjectTitleComponent>());
		if (nullptr != gameObjectTitleCompPtr)
		{
			this->gameObjectTitleComponent = gameObjectTitleCompPtr.get();
			this->gameObjectTitleComponent->getMovableText()->setVisible(true);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpeechBubbleComponent] This component will not work, as a prior GameObjectTitleComponent is missing for game object: " + this->gameObjectPtr->getName());
		}

		auto simpleSoundCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<SimpleSoundComponent>());
		if (nullptr != simpleSoundCompPtr)
		{
			this->simpleSoundComponent = simpleSoundCompPtr.get();
		}

		this->setCaption(this->caption->getString());

		this->createSpeechBubble();

		this->timeSinceLastChar = 0.0f;
		this->timeSinceLastRun = 0.0f;
		this->speechDone = false;

		this->lineNode->setVisible(true);

		return success;
	}

	bool SpeechBubbleComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		this->currentCharIndex = 0;
		this->timeSinceLastChar = 0.0f;
		this->timeSinceLastRun = 0.0f;
		this->speechDone = false;

		if (nullptr != this->simpleSoundComponent)
		{
			this->simpleSoundComponent->setActivated(false);
		}

		this->destroySpeechBubble();

		this->gameObjectTitleComponent = nullptr;
		this->simpleSoundComponent = nullptr;

		return true;
	}

	void SpeechBubbleComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->bIsInSimulation = !notSimulating;

		if (false == notSimulating && true == this->activated->getBool())
		{
			if (nullptr == this->manualObject)
			{
				return;
			}

			this->indices = 0;
			if (this->manualObject->getNumSections() > 0)
			{
				// Ogre will crash or throw exceptions if empty manual object is processed
				if (true == this->couldDraw)
				{
					this->manualObject->beginUpdate(0);
				}
			}
			else
			{
				this->manualObject->clear();
				this->manualObject->begin("WhiteNoLightingBackground", Ogre::OT_TRIANGLE_LIST);
			}

			this->drawSpeechBubble(dt);

			// Ogre will crash or throw exceptions if empty manual object is processed
			if (true == this->couldDraw)
			{
				// Realllllllyyyyy important! Else the rectangle is a whole mess!
				this->manualObject->index(0);
				this->manualObject->end();
			}
			else
			{
				this->manualObject->clear();
			}
		}
	}

	void SpeechBubbleComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ValueBarComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (SpeechBubbleComponent::AttrCaption() == attribute->getName())
		{
			this->setCaption(attribute->getString());
		}
		else if (SpeechBubbleComponent::AttrRunSpeech() == attribute->getName())
		{
			this->setRunSpeech(attribute->getBool());
		}
		else if (SpeechBubbleComponent::AttrSpeechDuration() == attribute->getName())
		{
			this->setSpeechDuration(attribute->getReal());
		}
		else if (SpeechBubbleComponent::AttrRunSpeechSound() == attribute->getName())
		{
			this->setRunSpeechSound(attribute->getBool());
		}
		else if (SpeechBubbleComponent::AttrKeepCaption() == attribute->getName())
		{
			this->setKeepCaption(attribute->getBool());
		}
	}

	void SpeechBubbleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Caption"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->caption->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RunSpeech"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->runSpeech->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpeechDuration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speechDuration->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RunSpeechSound"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->runSpeechSound->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "KeepCaption"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->keepCaption->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void SpeechBubbleComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		if (nullptr != this->gameObjectTitleComponent)
		{
			auto captionAttribute = this->gameObjectTitleComponent->getAttribute(SpeechBubbleComponent::AttrCaption());
			if (nullptr != captionAttribute)
			{
				captionAttribute->setVisible(true);
			}
		}
	}

	Ogre::String SpeechBubbleComponent::getClassName(void) const
	{
		return "SpeechBubbleComponent";
	}

	Ogre::String SpeechBubbleComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void SpeechBubbleComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (true == this->bIsInSimulation)
		{
			if (false == activated)
			{
				this->destroySpeechBubble();
				if (nullptr != this->gameObjectTitleComponent)
				{
					this->gameObjectTitleComponent->getMovableText()->setVisible(false);
				}
			}
			else
			{
				if (nullptr != this->gameObjectTitleComponent)
				{
					this->gameObjectTitleComponent->getMovableText()->setVisible(true);
				}
				this->createSpeechBubble();
			}
		}
	}

	bool SpeechBubbleComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void SpeechBubbleComponent::setCaption(const Ogre::String& caption)
	{
		Ogre::String tempCaption = replaceAll(caption, "\\n", "\n");
		this->caption->setValue(tempCaption);
		if (nullptr != this->gameObjectTitleComponent)
		{
			this->gameObjectTitleComponent->setCaption(tempCaption);
			// Must be called, in order to calculate bounding box for currentCaptionWidth, currentCaptionHeight
			this->gameObjectTitleComponent->getMovableText()->forceUpdate();

			this->gameObjectTitleComponent->getMovableText()->setVisible(true);

			this->currentCharIndex = 0;
			this->timeSinceLastRun = 0.0f;

			this->currentCaptionWidth = this->gameObjectTitleComponent->getMovableText()->getLocalAabb().getMaximum().x * 0.5f + 0.1f;
			this->currentCaptionHeight = this->gameObjectTitleComponent->getMovableText()->getLocalAabb().getMaximum().y * 0.5f;

			// Only set directly the whole caption to be rendered, if run speech is set to false. Else set caption char by char
			if (true == this->runSpeech->getBool())
			{
				this->gameObjectTitleComponent->setCaption("");
			}
		}

		if (nullptr != this->lineNode)
		{
			this->lineNode->setVisible(true);
		}
	}

	Ogre::String SpeechBubbleComponent::getCaption(void) const
	{
		return this->caption->getString();
	}

	void SpeechBubbleComponent::setRunSpeech(bool runSpeech)
	{
		this->runSpeech->setValue(runSpeech);
	}

	bool SpeechBubbleComponent::getRunSpeech(void) const
	{
		return this->runSpeech->getBool();
	}

	void SpeechBubbleComponent::setSpeechDuration(Ogre::Real speechDurationSec)
	{
		if (speechDurationSec < 0.0f)
		{
			speechDurationSec = 0.0f;
		}
		this->speechDuration->setValue(speechDurationSec);
	}

	Ogre::Real SpeechBubbleComponent::getSpeechDuration(void) const
	{
		return this->speechDuration->getReal();
	}

	void SpeechBubbleComponent::setRunSpeechSound(bool runSpeechSound)
	{
		if (true == runSpeechSound)
		{
			if (nullptr == this->simpleSoundComponent)
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[SpeechBubbleComponent] Could not set run speech sound, because there is no simple sound component. Add one first! For game object: " + this->gameObjectPtr->getName());
				return;
			}
		}

		this->runSpeechSound->setValue(runSpeechSound);
	}

	bool SpeechBubbleComponent::getRunSpeechSound(void) const
	{
		return this->runSpeechSound->getBool();
	}

	void SpeechBubbleComponent::setKeepCaption(bool keepCaption)
	{
	}

	bool SpeechBubbleComponent::getKeepCaption(void) const
	{
		return false;
	}

	void SpeechBubbleComponent::reactOnSpeechDone(luabind::object closureFunction)
	{
		this->closureFunction = closureFunction;
	}

	void SpeechBubbleComponent::drawSpeechBubble(Ogre::Real dt)
	{
		this->couldDraw = false;

		if (nullptr != this->gameObjectTitleComponent)
		{
			if (this->caption->getString().empty() || this->currentCaptionWidth == 0.0f)
			{
				return;
			}

			if (true == this->runSpeech->getBool())
			{
				size_t totalCharacters = this->caption->getString().length();
				if (totalCharacters > 0)
				{
					Ogre::Real timePerCharacter = this->speechDuration->getReal() / static_cast<Ogre::Real>(totalCharacters);

					if (this->timeSinceLastChar >= timePerCharacter && this->currentCharIndex < totalCharacters)
					{
						this->timeSinceLastChar = 0.0f;
						this->currentCharIndex++;

						Ogre::String captionSoFar = mid(this->caption->getString(), 0, this->currentCharIndex);
						this->gameObjectTitleComponent->setCaption(captionSoFar);

						if (this->runSpeechSound->getBool() && this->simpleSoundComponent)
						{
							Ogre::Real rndPitch = static_cast<Ogre::Real>(MathHelper::getInstance()->getRandomNumber(3, 10)) * 0.1f;
							this->simpleSoundComponent->setPitch(rndPitch);
							this->simpleSoundComponent->setActivated(true);
						}
					}
				}

				if (this->gameObjectTitleComponent->getCaption() == this->caption->getString() &&
					this->timeSinceLastRun >= this->speechDuration->getReal() + 0.2f)
				{
					this->speechDone = true;
					if (this->simpleSoundComponent)
					{
						this->simpleSoundComponent->setActivated(false);
					}
				}

				if (this->speechDone)
				{
					this->timeSinceLastChar = 0.0f;
					this->timeSinceLastRun = 0.0f;
					if (false == this->keepCaption->getBool())
					{
						this->gameObjectTitleComponent->getMovableText()->setVisible(false);
						this->lineNode->setVisible(false);
					}
					this->speechDone = false;

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

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnSpeechDone' Error: "
								+ Ogre::String(error.what()) + " details: " + msg.str());
						}
					}
				}

				this->timeSinceLastChar += dt;
				this->timeSinceLastRun += dt;
			}

			if (this->gameObjectTitleComponent->getCaption().empty())
			{
				return;
			}

			// Ogre::Vector3 p = this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_getDerivedPosition() + (Ogre::Vector3(0.0f, this->currentCaptionHeight, 0.0f) * 0.5f);
			Ogre::Vector3 p = this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_getDerivedPosition() + Ogre::Vector3(0.0f, this->currentCaptionHeight * 1.2f, 0.0f);
			Ogre::Quaternion o = this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_getDerivedOrientation();
			Ogre::Vector3 sp = Ogre::Vector3::ZERO;
			Ogre::Quaternion so = Ogre::Quaternion::IDENTITY;

			// Ogre::Vector3  p = this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_getDerivedPosition() * Ogre::Vector3(this->gameObjectTitleComponent->getAlignment().x, this->gameObjectTitleComponent->getAlignment().y, 1.0f);
			 // 	- Ogre::Vector3(0.0f, this->currentCaptionHeight * 0.5f, 0.0f);


			// https://stackoverflow.com/questions/3477988/how-can-a-programmatically-draw-a-scalable-aethetically-pleasing-curved-comic

			Ogre::Real accuracy = 17.0f;

			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-this->currentCaptionWidth, 0.0f, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue::White);
			this->manualObject->index(this->indices + 0);

			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-this->currentCaptionWidth + 0.4f, 0.0f, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue::White);
			this->manualObject->index(this->indices + 1);

			// Play with 0.2 y
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-this->currentCaptionWidth + 0.1f, 0.2f, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue::White);
			this->manualObject->index(this->indices + 2);
			this->indices += 3;

			std::vector<Ogre::Vector3> points(3);
			for (Ogre::Real theta = 0; theta <= 2 * Math::PI; theta += Math::PI / accuracy)
			{
				points[0] = Ogre::Vector3(0.0f, 0.0f, 0.0f);
				points[1] = Ogre::Vector3(this->currentCaptionWidth * Ogre::Math::Cos(theta - Math::PI / accuracy), (this->currentCaptionHeight * 0.5f) + this->currentCaptionHeight * 0.7f * Ogre::Math::Sin(theta - Math::PI / accuracy), 0.0f);
				points[2] = Ogre::Vector3(this->currentCaptionWidth * Ogre::Math::Cos(theta), (this->currentCaptionHeight * 0.5f) + this->currentCaptionHeight * 0.7f * Ogre::Math::Sin(theta), 0.0f);

				this->manualObject->position(p + (o * (so * (sp + points[0]))));
				this->manualObject->colour(Ogre::ColourValue::White);
				this->manualObject->index(this->indices + 0);

				this->manualObject->position(p + (o * (so * (sp + points[1]))));
				this->manualObject->colour(Ogre::ColourValue::White);
				this->manualObject->index(this->indices + 1);

				this->manualObject->position(p + (o * (so * (sp + points[2]))));
				this->manualObject->colour(Ogre::ColourValue::White);
				this->manualObject->index(this->indices + 2);
				this->indices += 3;
			}

			this->couldDraw = true;
		}
	}

	void SpeechBubbleComponent::createSpeechBubble(void)
	{
		if (nullptr == this->manualObject)
		{
			if (nullptr == this->lineNode)
			{
				this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
			}
			this->manualObject = this->gameObjectPtr->getSceneManager()->createManualObject();
			this->manualObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
			this->manualObject->setName("SpeechBubble_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(index));
			this->manualObject->setQueryFlags(0 << 0);
			this->lineNode->attachObject(this->manualObject);
			this->manualObject->setCastShadows(false);
		}
	}

	void SpeechBubbleComponent::destroySpeechBubble(void)
	{
		if (this->lineNode != nullptr)
		{
			this->lineNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroyManualObject(this->manualObject);
			this->manualObject = nullptr;
			this->lineNode->getParentSceneNode()->removeAndDestroyChild(this->lineNode);
			this->lineNode = nullptr;
		}
	}

	// Lua registration part

	SpeechBubbleComponent* getSpeechBubbleComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponentWithOccurrence<SpeechBubbleComponent>(occurrenceIndex)).get();
	}

	SpeechBubbleComponent* getSpeechBubbleComponent(GameObject* gameObject)
	{
		return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponent<SpeechBubbleComponent>()).get();
	}

	SpeechBubbleComponent* getSpeechBubbleComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponentFromName<SpeechBubbleComponent>(name)).get();
	}

	void SpeechBubbleComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<SpeechBubbleComponent, GameObjectComponent>("SpeechBubbleComponent")
			.def("setActivated", &SpeechBubbleComponent::setActivated)
			.def("isActivated", &SpeechBubbleComponent::isActivated)
			.def("setRunSpeech", &SpeechBubbleComponent::setRunSpeech)
			.def("getRunSpeech", &SpeechBubbleComponent::getRunSpeech)
			.def("setCaption", &SpeechBubbleComponent::setCaption)
			.def("getCaption", &SpeechBubbleComponent::getCaption)
			.def("setSpeechDuration", &SpeechBubbleComponent::setSpeechDuration)
			.def("getSpeechDuration", &SpeechBubbleComponent::getSpeechDuration)
			.def("setRunSpeechSound", &SpeechBubbleComponent::setRunSpeechSound)
			.def("getRunSpeechSound", &SpeechBubbleComponent::getRunSpeechSound)
			.def("setKeepCaption", &SpeechBubbleComponent::setKeepCaption)
			.def("getKeepCaption", &SpeechBubbleComponent::getKeepCaption)
			.def("reactOnSpeechDone", &SpeechBubbleComponent::reactOnSpeechDone)
		];

		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "class inherits GameObjectComponent", SpeechBubbleComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setTextColor(Vector3 color)", "Sets the color for the text.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "Vector3 getTextColor()", "Gets the text color.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setRunSpeech(bool runSpeech)", "Sets whether the speech text shall appear char by char running.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getRunSpeech()", "Gets whether the speech text shall appear char by char running.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setCaption(string caption)", "Sets the caption text to be displayed.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "string getCaption()", "Gets the caption text.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setSpeechDuration(float speechDuration)", "Sets the speed duration. That is how long the bubble shall remain in seconds.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getSpeechDuration()", "Gets the speed duration. That is how long the bubble shall remain in seconds.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setKeepCaption(bool keepCaption)", "Sets whether to use a sound if the speech is running char by char.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getKeepCaption()", "Gets whether to use a sound if the speech is running char by char.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setRunSpeechSound(bool runSpeechSound)", "Sets whether the caption should remain after the speech run.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getRunSpeechSound()", "Gets whether the caption is remained after the speech run.");
		LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void reactOnSpeechDone(func closureFunction)", "Sets whether to react if a speech is done.");


		gameObjectClass.def("getSpeechBubbleComponentFromName", &getSpeechBubbleComponentFromName);
		gameObjectClass.def("getSpeechBubbleComponent", (SpeechBubbleComponent * (*)(GameObject*)) & getSpeechBubbleComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SpeechBubbleComponent getSpeechBubbleComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SpeechBubbleComponent getSpeechBubbleComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castSpeechBubbleComponent", &GameObjectController::cast<SpeechBubbleComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "SpeechBubbleComponent castSpeechBubbleComponent(SpeechBubbleComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool SpeechBubbleComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<GameObjectTitleComponent>());
		if (nullptr == gameObjectTitleCompPtr)
		{
			return false;
		}

		if (gameObject->getComponentCount<SpeechBubbleComponent>() < 2)
		{
			return true;
		}

		return true;
	}

}; //namespace end