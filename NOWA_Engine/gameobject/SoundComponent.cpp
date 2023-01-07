#include "NOWAPrecompiled.h"
#include "SoundComponent.h"
#include "utilities/XMLConverter.h"
#include "main/AppStateManager.h"
#include "modules/OgreALModule.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	SoundComponent::SoundComponent()
		: GameObjectComponent(),
		sound(nullptr),
		activated(new Variant(SoundComponent::AttrActivated(), false, this->attributes)),
		soundName(new Variant(SoundComponent::AttrSoundName(), "Health.wav", this->attributes)),
		volume(new Variant(SoundComponent::AttrVolume(), 100.0f, this->attributes)),
		loop(new Variant(SoundComponent::AttrLoop(), false, this->attributes)),
		stream(new Variant(SoundComponent::AttrStream(), false, this->attributes)),
		relativeToListener(new Variant(SoundComponent::AttrRelativeToListener(), false, this->attributes)),
		fadeInOutTime(new Variant(SoundComponent::AttrFadeInOutTime(), Ogre::Vector2(-1.0f, -1.0f), this->attributes)),
		innerOuterConeAngle(new Variant(SoundComponent::AttrInnerOuterConeAngle(), Ogre::Vector2(360.0f, 360.0f), this->attributes)),
		outerConeGain(new Variant(SoundComponent::AttrOuterConeGain(), 0.0f, this->attributes)),
		pitch(new Variant(SoundComponent::AttrPitch(), 1.0f, this->attributes)),
		priority(new Variant(SoundComponent::AttrPriority(), static_cast<int>(1), this->attributes)),
		distanceValues(new Variant(SoundComponent::AttrDistanceValues(), Ogre::Vector3(3400.0f, 1.0f, 150.0f), this->attributes)),
		secondOffset(new Variant(SoundComponent::AttrSecondOffset(), 0.0f, this->attributes))
	{
		std::vector<Ogre::String> triggerList = { "Prompt", "OnHit" };
		this->soundTrigger = new Variant(SoundComponent::AttrSoundTrigger(), triggerList, this->attributes);
		// Really important line of code, ogreal must be not null, when using it
		this->soundManager = OgreALModule::getInstance()->getSoundManager();
		this->oldSoundName = soundName->getString();
		this->stream->setDescription("Should be used for bigger sound files, so that they will be streamed instead of loaded completely into buffer (performance issue).");
	}

	SoundComponent::~SoundComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SoundComponent] Destructor sound component for game object: " + this->gameObjectPtr->getName());
		this->destroySound();
	}

	bool SoundComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool stream = false;
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SoundName")
		{
			this->soundName->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Volume")
		{
			this->volume->setValue(XMLConverter::getAttribReal(propertyElement, "data", 100.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Loop")
		{
			this->loop->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Stream")
		{
			this->stream->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RelativeToListener")
		{
			this->relativeToListener->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SoundTrigger")
		{
			this->soundTrigger->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "OnHit"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SoundFadeInOut")
		{
			this->fadeInOutTime->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SoundInnerOuterConeAngle")
		{
			this->innerOuterConeAngle->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(360.0f, 360.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SoundPitch")
		{
			this->pitch->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SoundPriority")
		{
			this->priority->setValue(XMLConverter::getAttribInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SoundDistanceValues")
		{
			this->distanceValues->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(3400.0f, 1.0f, 150.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SoundSecondOffset")
		{
			this->secondOffset->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr SoundComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		SoundCompPtr clonedCompPtr(boost::make_shared<SoundComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setSoundName(this->soundName->getString());
		clonedCompPtr->setVolume(this->volume->getReal());
		clonedCompPtr->setLoop(this->loop->getBool());
		clonedCompPtr->setStream(this->stream->getBool());
		clonedCompPtr->setFadeInOutTime(this->fadeInOutTime->getVector2());
		clonedCompPtr->setInnerOuterConeAngle(this->innerOuterConeAngle->getVector2());
		clonedCompPtr->setOuterConeGain(this->outerConeGain->getReal());
		clonedCompPtr->setPitch(this->pitch->getReal());
		clonedCompPtr->setPriority(this->priority->getInt());
		clonedCompPtr->setDistanceValues(this->distanceValues->getVector3());
		clonedCompPtr->setSecondOffset(this->secondOffset->getReal());
		clonedCompPtr->setRelativeToListener(this->relativeToListener->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool SoundComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SoundComponent] Init sound component for game object: " + this->gameObjectPtr->getName());
		
		this->createSound();

		return true;
	}
	
	bool SoundComponent::createSound(void)
	{
		if (this->oldSoundName != this->soundName->getString())
		{
			this->destroySound();
		}
		if (nullptr == this->sound)
		{
			this->sound = OgreALModule::getInstance()->createSound(this->gameObjectPtr->getSceneManager(), Ogre::StringConverter::toString(NOWA::makeUniqueID()), this->soundName->getString(),
				this->loop->getBool(), this->stream->getBool());
			if (nullptr == this->sound)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SimpleSoundComponent] Error: Could not create sound: " + this->soundName->getString());
				return false;
			}
			this->sound->setGain(this->volume->getReal() * 0.01f);
			this->sound->setRelativeToListener(this->relativeToListener->getBool());
			this->sound->setQueryFlags(0);
			this->gameObjectPtr->getSceneNode()->attachObject(this->sound);

			// hier attach to node? wenn relative zu listener? Beispiele anschauen

			// wenn on hit, dann schauen ob physicscomponent gibt die activephysicscomponent ist, dann in physicscomponent, wo auch materialien erstellt werden, coll function einfuehren,
			// die alles behandelt und dann per delegate verteilt, dann hier die delegate function, um zu schauen, ob diese sound component angesprochen wurde, dann sound abspielen
		}
		this->sound->setLoop(this->loop->getBool());
		this->sound->setGain(this->volume->getReal() * 0.01f);
		this->sound->fadeIn(this->fadeInOutTime->getVector2().x);
		this->sound->fadeOut(this->fadeInOutTime->getVector2().y);
		this->sound->setInnerConeAngle(this->innerOuterConeAngle->getVector2().x);
		this->sound->setOuterConeAngle(this->innerOuterConeAngle->getVector2().y);
		this->sound->setOuterConeGain(this->outerConeGain->getReal());
		this->sound->setPitch(this->pitch->getReal());
		this->sound->setPriority(static_cast<OgreAL::Sound::Priority>(this->priority->getInt()));
		this->sound->setDistanceValues(this->distanceValues->getVector3().x, this->distanceValues->getVector3().y, this->distanceValues->getVector3().z);
		this->sound->setSecondOffset(this->secondOffset->getReal());
		
		this->sound->setRelativeToListener(this->relativeToListener->getBool());
		this->sound->setQueryFlags(0);
		
		return true;
	}

	void SoundComponent::destroySound(void)
	{
		if (nullptr != this->sound)
		{
			if (this->sound->isAttached())
			{
				this->gameObjectPtr->getSceneNode()->detachObject(this->sound);
			}
			OgreALModule::getInstance()->deleteSound(this->gameObjectPtr->getSceneManager(), this->sound);
			this->sound = nullptr;
		}
	}

	bool SoundComponent::connect(void)
	{
		this->createSound();
		if (nullptr != this->sound)
		{
			if (this->activated->getBool())
			{
				if (this->sound->isPlaying())
				{
					this->sound->stop();
				}
				this->sound->play();
			}
			else
			{
				this->sound->stop();
			}
		}
		return true;
	}

	bool SoundComponent::disconnect(void)
	{
		if (nullptr != this->sound)
		{
			this->sound->stop();
		}
		return true;
	}

	void SoundComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (SoundComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (SoundComponent::AttrSoundName() == attribute->getName())
		{
			this->setSoundName(attribute->getString());
		}
		else if (SoundComponent::AttrVolume() == attribute->getName())
		{
			this->setVolume(attribute->getReal());
		}
		else if (SoundComponent::AttrLoop() == attribute->getName())
		{
			this->setLoop(attribute->getBool());
		}
		else if (SoundComponent::AttrStream() == attribute->getName())
		{
			this->setStream(attribute->getBool());
		}
		else if (SoundComponent::AttrFadeInOutTime() == attribute->getName())
		{
			this->setFadeInOutTime(attribute->getVector2());
		}
		else if (SoundComponent::AttrInnerOuterConeAngle() == attribute->getName())
		{
			this->setInnerOuterConeAngle(attribute->getVector2());
		}
		else if (SoundComponent::AttrOuterConeGain() == attribute->getName())
		{
			this->setOuterConeGain(attribute->getReal());
		}
		else if (SoundComponent::AttrPitch() == attribute->getName())
		{
			this->setPitch(attribute->getReal());
		}
		else if (SoundComponent::AttrPriority() == attribute->getName())
		{
			this->setPriority(attribute->getInt());
		}
		else if (SoundComponent::AttrDistanceValues() == attribute->getName())
		{
			this->setDistanceValues(attribute->getVector3());
		}
		else if (SoundComponent::AttrSecondOffset() == attribute->getName())
		{
			this->setSecondOffset(attribute->getReal());
		}
		else if (SoundComponent::AttrRelativeToListener() == attribute->getName())
		{
			this->setRelativeToListener(attribute->getBool());
		}
	}

	void SoundComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// Does not work correctly at the moment, because to less parameters are member functions, like stream etc.
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
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->soundName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Volume"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->volume->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Loop"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->loop->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Stream"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->stream->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RelativeToListener"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->relativeToListener->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundTrigger"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->soundTrigger->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundFadeInOut"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->fadeInOutTime->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundInnerOuterConeAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->innerOuterConeAngle->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundOuterConeGain"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->outerConeGain->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundPitch"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->pitch->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundPriority"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->priority->getInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundDistanceValues"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->distanceValues->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundSecondOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->secondOffset->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String SoundComponent::getClassName(void) const
	{
		return "SoundComponent";
	}

	Ogre::String SoundComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void SoundComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (false == activated)
		{
			if (nullptr != this->sound)
				this->sound->stop();
		}
	}

	bool SoundComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void SoundComponent::setSoundName(const Ogre::String& soundName)
	{
		this->soundName->setValue(soundName);
		this->createSound();
		this->oldSoundName = soundName;
	}

	Ogre::String SoundComponent::getSoundName(void) const
	{
		return this->soundName->getString();
	}

	void SoundComponent::setVolume(Ogre::Real volume)
	{
		this->volume->setValue(volume);
		if (nullptr != this->sound)
		{
			this->sound->setGain(volume * 0.01f);
		}
	}

	Ogre::Real SoundComponent::getVolume(void) const
	{
		return this->volume->getReal();
	}

	void SoundComponent::setLoop(bool loop)
	{
		this->loop->setValue(loop);
		if (nullptr != this->sound)
		{
			this->sound->setLoop(loop);
		}
	}

	bool SoundComponent::getIsLooped(void) const
	{
		return this->loop->getBool();
	}

	void SoundComponent::setStream(bool stream)
	{
		if (stream != this->stream->getBool())
		{
			this->stream->setValue(stream);
			this->createSound();
		}
	}

	bool SoundComponent::getIsStreamed(void) const
	{
		return this->stream->getBool();
	}

	void SoundComponent::setRelativeToListener(bool relativeToListener)
	{
		this->relativeToListener->setValue(relativeToListener);
		if (nullptr != this->sound)
		{
			this->sound->setRelativeToListener(relativeToListener);
		}
	}

	bool SoundComponent::getRelativeToListener(void) const
	{
		return this->relativeToListener->getBool();
	}

	void SoundComponent::setSoundTrigger(const Ogre::String& soundTrigger)
	{
		this->soundTrigger->setListSelectedValue(soundTrigger);
	}

	Ogre::String SoundComponent::getSoundTrigger(void) const
	{
		return this->soundTrigger->getListSelectedValue();
	}

	void SoundComponent::setFadeInOutTime(const Ogre::Vector2& fadeInOutTime)
	{
		this->fadeInOutTime->setValue(fadeInOutTime);
		if (nullptr != this->sound)
		{
			this->sound->fadeIn(this->fadeInOutTime->getVector2().x);
			this->sound->fadeOut(this->fadeInOutTime->getVector2().y);
		}
	}

	Ogre::Vector2 SoundComponent::getFadeInOutTime(void) const
	{
		return this->fadeInOutTime->getVector2();
	}

	void SoundComponent::setInnerOuterConeAngle(const Ogre::Vector2& innerOuterConeAngle)
	{
		this->innerOuterConeAngle->setValue(innerOuterConeAngle);
		if (nullptr != this->sound)
		{
			this->sound->setInnerConeAngle(this->innerOuterConeAngle->getVector2().x);
			this->sound->setOuterConeAngle(this->innerOuterConeAngle->getVector2().y);
		}
	}

	Ogre::Vector2 SoundComponent::getInnerOuterConeAngle(void) const
	{
		return this->innerOuterConeAngle->getVector2();
	}

	void SoundComponent::setOuterConeGain(Ogre::Real outerConeGain)
	{
		this->outerConeGain->setValue(outerConeGain);
		if (nullptr != this->sound)
		{
			this->sound->setOuterConeGain(this->outerConeGain->getReal());
		}
	}

	Ogre::Real SoundComponent::getOuterConeGain(void) const
	{
		return this->outerConeGain->getReal();
	}

	void SoundComponent::setPitch(Ogre::Real pitch)
	{
		this->pitch->setValue(pitch);
		if (nullptr != this->sound)
		{
			this->sound->setPitch(this->pitch->getReal());
		}
	}

	Ogre::Real SoundComponent::getPitch(void) const
	{
		return this->pitch->getReal();
	}

	void SoundComponent::setPriority(int priority)
	{
		this->priority->setValue(priority);
		if (nullptr != this->sound)
		{
			this->sound->setPriority(static_cast<OgreAL::Sound::Priority>(this->priority->getInt()));
		}
	}

	int SoundComponent::getPriority(void) const
	{
		return this->priority->getInt();
	}

	void SoundComponent::setDistanceValues(const Ogre::Vector3& distanceValues)
	{
		this->distanceValues->setValue(distanceValues);
		if (nullptr != this->sound)
		{
			this->sound->setDistanceValues(this->distanceValues->getVector3().x, this->distanceValues->getVector3().y, this->distanceValues->getVector3().z);
		}
	}

	Ogre::Vector3 SoundComponent::getDistanceValues(void) const
	{
		return this->distanceValues->getVector3();
	}

	void SoundComponent::setSecondOffset(Ogre::Real secondOffset)
	{
		this->secondOffset->setValue(secondOffset);
		if (nullptr != this->sound)
		{
			this->sound->setSecondOffset(this->secondOffset->getReal());
		}
	}

	Ogre::Real SoundComponent::getSecondOffset(void) const
	{
		return this->secondOffset->getReal();
	}

	OgreAL::Sound* SoundComponent::getSound(void) const
	{
		return this->sound;
	}

}; // namespace end