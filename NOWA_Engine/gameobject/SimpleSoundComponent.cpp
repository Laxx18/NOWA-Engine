#include "NOWAPrecompiled.h"
#include "SimpleSoundComponent.h"

#include <boost/make_shared.hpp>
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "modules/OgreALModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	SimpleSoundComponent::SimpleSoundComponent()
		: GameObjectComponent(),
		sound(nullptr),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		activated(new Variant(SimpleSoundComponent::AttrActivated(), false, this->attributes)),
		soundName(new Variant(SimpleSoundComponent::AttrSoundName(), "Health.wav", this->attributes)),
		volume(new Variant(SimpleSoundComponent::AttrVolume(), 100.0f, this->attributes)),
		loop(new Variant(SimpleSoundComponent::AttrLoop(), false, this->attributes)),
		stream(new Variant(SimpleSoundComponent::AttrStream(), false, this->attributes)),
		relativeToListener(new Variant(SimpleSoundComponent::AttrRelativeToListener(), false, this->attributes)),
		playWhenInMotion(new Variant(SimpleSoundComponent::AttrPlayWhenInMotion(), false, this->attributes)),
		onSpectrumAnalysisFunctionName(new Variant(SimpleSoundComponent::AttrSpectrumAnalysisFunctionName(), Ogre::String(""), this->attributes))
	{
		// Really important line of code, ogreal must be not null, when using it
		this->soundManager = OgreALModule::getInstance()->getSoundManager();
		this->oldSoundName = soundName->getString();
		this->soundName->addUserData(GameObject::AttrActionFileOpenDialog(), "Audio");
		this->stream->setDescription("Should be used for bigger sound files, so that they will be streamed instead of loaded completely into buffer (performance issue).");

		this->volume->setDescription("Range [0, 100]");
		this->volume->setConstraints(0.0f, 100.0f);

		this->onSpectrumAnalysisFunctionName->setDescription("Sets the lua function name to react on spectrum as long as the audio is played. Note: Streaming must be activated.");
	}

	SimpleSoundComponent::~SimpleSoundComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SimpleSoundComponent] Destructor simple sound component for game object: " + this->gameObjectPtr->getName());
		this->destroySound();
	}

	bool SimpleSoundComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlayWhenInMotion")
		{
			this->playWhenInMotion->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnSpectrumAnalysisFunctionName")
		{
			this->onSpectrumAnalysisFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			this->onSpectrumAnalysisFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onSpectrumAnalysisFunctionName->getString() + "()");
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr SimpleSoundComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		SimpleSoundCompPtr clonedCompPtr(boost::make_shared<SimpleSoundComponent>());

		
		clonedCompPtr->setSoundName(this->soundName->getString());
		clonedCompPtr->setVolume(this->volume->getReal());
		clonedCompPtr->setLoop(this->loop->getBool());
		clonedCompPtr->setStream(this->stream->getBool());
		clonedCompPtr->setRelativeToListener(this->relativeToListener->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setPlayWhenInMotion(this->playWhenInMotion->getBool());
		clonedCompPtr->setOnSpectrumAnalysisFunctionName(this->onSpectrumAnalysisFunctionName->getString());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool SimpleSoundComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SimpleSoundComponent] Init simple sound component for game object: " + this->gameObjectPtr->getName());
		
		this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

		// ENQUEUE_RENDER_COMMAND_WAIT("",
		// {
			this->createSound();
		// });
		
		return true;
	}

	bool SimpleSoundComponent::createSound(void)
	{
		if (nullptr == this->gameObjectPtr)
			return true;

		if (this->oldSoundName != this->soundName->getString())
		{
			this->destroySound();
			this->oldSoundName = this->soundName->getString();
		}

		bool loop = this->loop->getBool();

		if (true == this->playWhenInMotion->getBool())
			loop = true;

		if (nullptr == this->sound)
		{
			this->sound = OgreALModule::getInstance()->createSound(this->gameObjectPtr->getSceneManager(), Ogre::StringConverter::toString(NOWA::makeUniqueID()), this->soundName->getString(),
				loop, this->stream->getBool());
			if (nullptr == this->sound)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SimpleSoundComponent] Error: Could not create sound: " + this->soundName->getString());
				return false;
			}
			this->sound->setStatic(!this->gameObjectPtr->isDynamic());
			this->sound->setGain(this->volume->getReal() * 0.01f);
			this->sound->setRelativeToListener(this->relativeToListener->getBool());
			this->sound->setQueryFlags(0);
			this->gameObjectPtr->getSceneNode()->attachObject(this->sound);

			// hier attach to node? wenn relative zu listener? Beispiele anschauen

			// wenn on hit, dann schauen ob physicscomponent gibt die activephysicscomponent ist, dann in physicscomponent, wo auch materialien erstellt werden, coll function einfuehren,
			// die alles behandelt und dann per delegate verteilt, dann hier die delegate function, um zu schauen, ob diese sound component angesprochen wurde, dann sound abspielen
		}

		this->sound->setStream(this->stream->getBool());
		this->sound->setLoop(loop);
		this->sound->setGain(this->volume->getReal() * 0.01f);
		this->sound->setRelativeToListener(this->relativeToListener->getBool());
		this->sound->setQueryFlags(0);

		this->setOnSpectrumAnalysisFunctionName(this->onSpectrumAnalysisFunctionName->getString());

		return true;
	}

	void SimpleSoundComponent::destroySound(void)
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

	inline float norm(float v, float max, float dest, float offset)
	{
		return (v * (dest / max)) + offset;
	}

	void SimpleSoundComponent::soundSpectrumFuncPtr(OgreAL::Sound* sound)
	{
		if (false == this->bConnected)
		{
			return;
		}

		OgreAL::Sound::SpectrumParameter* spectrumParameter = sound->getSpectrumParameter();
		if (nullptr != spectrumParameter)
		{
			// Call also function in lua script, if it does exist in the lua script component
			if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->onSpectrumAnalysisFunctionName->getString().empty())
			{
				NOWA::AppStateManager::LogicCommand logicCommand = [this]()
				{
					this->gameObjectPtr->getLuaScript()->callTableFunction(this->onSpectrumAnalysisFunctionName->getString());
				};
				NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
			}

#if 0
			/*for (int i = 0; i < this->sound->getBufferSize() - 1; ++i)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Spectrum: " + Ogre::StringConverter::toString(spectrumParameter->VUpoints[i]));
				
			}

			int level = 0;
			for (int i = 0; i < (this->sound->getBufferSize() - 1) / 2; ++i)
			{
				level = -10 * log(spectrumParameter->amp[i] / 94);
				if (level > 0)
					level = 0;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Level: " + Ogre::StringConverter::toString(level));
			}*/

			const Ogre::Real xoff = 0.02f;
			const Ogre::Real yoff = 0.02f;

		
			if (true == this->lines.empty())
			{
				this->lines.resize((512 - 1) / 2);
				for (size_t i = 0; i < (512 - 1) / 2; i++)
				{
					// this->translationLine = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
					this->lines[i] = this->gameObjectPtr->getSceneManager()->createManualObject(Ogre::SCENE_DYNAMIC);
					// this->sceneManager->createManualObject(Ogre::SCENE_DYNAMIC);
					this->lines[i]->setQueryFlags(0 << 0);
					this->lineNode->attachObject(this->lines[i]);
					this->lines[i]->setCastShadows(false);
					this->lines[i]->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
				}
			}

			/*for (size_t i = 0; i < 10; i++)
			{
				
				this->lines[i]->clear();
				this->lines[i]->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);

				this->lines[i]->position(Ogre::Vector3(i + xoff, norm(spectrumParameter->VUpoints[i], 65536, 10.0f, 2.5f), 0.0f));
				this->lines[i]->index(0);
				this->lines[i]->position(Ogre::Vector3(i + xoff, norm(spectrumParameter->VUpoints[i], -65536, 10.0f, 2.5f), 0.0f));
				this->lines[i]->index(1);
				this->lines[i]->end();
			}*/




			float level = 0;
			for (size_t i = 0; i < (512 - 1) / 2; i++)
			{
				// Ogre::Vector3 previousPosition = Ogre::Vector3(i * 2 * xoff, 0.0f, 0.0f);

				level = spectrumParameter->amp[i];

				/*double regleDe3 = level * 255 / 100;
				int y = static_cast<int>(regleDe3);
				if (y > 255) y = 255;
				if (y < 0) y = 0;*/
			

				/*level = -10 * log(spectrumParameter->amp[i] / 94);
				if (level > 0)
					level = 0;*/

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Spectrum: " + Ogre::StringConverter::toString(level));
				
				this->lines[i]->clear();
				this->lines[i]->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);

				

				this->lines[i]->position(Ogre::Vector3(i * 2 * xoff, 0.0f, 0.0f));
				this->lines[i]->index(0);
				this->lines[i]->position(Ogre::Vector3(i * 2 * xoff, level, 0.0f));
				this->lines[i]->index(1);
				this->lines[i]->end();
			}
#endif
		}
	}

	bool SimpleSoundComponent::connect(void)
	{
		GameObjectComponent::connect();

		// ENQUEUE_RENDER_COMMAND_WAIT("",
		// {
			this->setupSound();
		// });
		
		return true;
	}

	bool SimpleSoundComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		this->oldPosition = this->gameObjectPtr->getPosition();
		this->oldOrientation = this->gameObjectPtr->getOrientation();

		if (nullptr != this->sound)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("SimpleSoundComponent::enableSpectrumAnalysis",
			{
				this->sound->stop();
				this->sound->enableSpectrumAnalysis(false, 1, 1, OgreAL::MathWindows::BARLETT, OgreAL::AudioProcessor::SpectrumPreparationType::LINEAR, 0.0f);
			});
			
		}
		return true;
	}

	void SimpleSoundComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->playWhenInMotion->getBool())
		{
			bool canPlay = false;

			// Check if position or orientation changed
			if (false == this->oldPosition.positionEquals(this->gameObjectPtr->getPosition(), 0.001f))
			{
				canPlay = true;
			}
			else if (false == this->gameObjectPtr->getOrientation().equals(this->oldOrientation, Ogre::Degree(0.001f)))
			{
				canPlay = true;
			}

			this->oldPosition = this->gameObjectPtr->getPosition();
			this->oldOrientation = this->gameObjectPtr->getOrientation();

			if (true == canPlay)
			{
				if (nullptr != this->sound)
				{
					// Do not interrupt sound, when it is currently playing
					if (this->sound->isPlaying())
					{
						return;
					}
				}
				else
				{
					this->createSound();
					this->sound->play();
				}
			}
			else
			{
				if (nullptr != this->sound)
				{
					this->sound->fadeOut(0.2f);
				}
			}
		}
	}

	void SimpleSoundComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (SimpleSoundComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (SimpleSoundComponent::AttrSoundName() == attribute->getName())
		{
			this->setSoundName(attribute->getString());
		}
		else if (SimpleSoundComponent::AttrVolume() == attribute->getName())
		{
			this->setVolume(attribute->getReal());
		}
		else if (SimpleSoundComponent::AttrLoop() == attribute->getName())
		{
			this->setLoop(attribute->getBool());
		}
		else if (SimpleSoundComponent::AttrStream() == attribute->getName())
		{
			this->setStream(attribute->getBool());
		}
		else if (SimpleSoundComponent::AttrRelativeToListener() == attribute->getName())
		{
			this->setRelativeToListener(attribute->getBool());
		}
		else if (SimpleSoundComponent::AttrPlayWhenInMotion() == attribute->getName())
		{
			this->setPlayWhenInMotion(attribute->getBool());
		}
		else if (SimpleSoundComponent::AttrSpectrumAnalysisFunctionName() == attribute->getName())
		{
			this->setOnSpectrumAnalysisFunctionName(attribute->getString());
		}
	}

	void SimpleSoundComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SoundName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->soundName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Volume"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->volume->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Loop"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->loop->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Stream"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->stream->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RelativeToListener"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->relativeToListener->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PlayWhenInMotion"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->playWhenInMotion->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnSpectrumAnalysisFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onSpectrumAnalysisFunctionName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String SimpleSoundComponent::getClassName(void) const
	{
		return "SimpleSoundComponent";
	}

	Ogre::String SimpleSoundComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void SimpleSoundComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (false == activated)
		{
			if (nullptr != this->sound)
				this->sound->stop();
		}
		else
		{
			if (true == this->bConnected)
			{
				// ENQUEUE_RENDER_COMMAND_WAIT("",
				// {
					this->setupSound();
				// });
			}
		}
	}

	bool SimpleSoundComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void SimpleSoundComponent::setSoundName(const Ogre::String& soundName)
	{
		this->soundName->setValue(soundName);
		// ENQUEUE_RENDER_COMMAND_WAIT("",
		// {
			this->createSound();
		// });
		this->oldSoundName = soundName;
	}

	Ogre::String SimpleSoundComponent::getSoundName(void) const
	{
		return this->soundName->getString();
	}

	void SimpleSoundComponent::setVolume(Ogre::Real volume)
	{
		this->volume->setValue(volume);
		if (nullptr != this->sound)
		{
			this->sound->setGain(volume * 0.01f);
		}
	}

	Ogre::Real SimpleSoundComponent::getVolume(void) const
	{
		return this->volume->getReal();
	}

	void SimpleSoundComponent::setLoop(bool loop)
	{
		this->loop->setValue(loop);
		if (nullptr != this->sound)
		{
			this->sound->setLoop(loop);
		}
	}

	bool SimpleSoundComponent::isLooped(void) const
	{
		return this->loop->getBool();
	}

	void SimpleSoundComponent::setStream(bool stream)
	{
		if (stream != this->stream->getBool())
		{
			this->stream->setValue(stream);
			// ENQUEUE_RENDER_COMMAND_WAIT("",
			// {
				this->createSound();
			// });
		}
	}

	bool SimpleSoundComponent::isStreamed(void) const
	{
		return this->stream->getBool();
	}

	void SimpleSoundComponent::setRelativeToListener(bool relativeToListener)
	{
		this->relativeToListener->setValue(relativeToListener);
		if (nullptr != this->sound)
		{
			this->sound->setRelativeToListener(relativeToListener);
		}
	}

	bool SimpleSoundComponent::isRelativeToListener(void) const
	{
		return this->relativeToListener->getBool();
	}

	void SimpleSoundComponent::setPlayWhenInMotion(bool playWhenInMotion)
	{
		this->playWhenInMotion->setValue(playWhenInMotion);
	}

	bool SimpleSoundComponent::getPlayWhenInMotion(void) const
	{
		return this->playWhenInMotion->getBool();
	}

	void SimpleSoundComponent::enableSpectrumAnalysis(bool enable, int processingSize, int numberOfBands, OgreAL::MathWindows::WindowType windowType, 
		OgreAL::AudioProcessor::SpectrumPreparationType spectrumPreparationType, Ogre::Real smoothFactor)
	{
		if (nullptr != this->sound)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("SimpleSoundComponent::enableSpectrumAnalysis", _6(enable, processingSize, numberOfBands, windowType, spectrumPreparationType, smoothFactor),
			{
				this->sound->enableSpectrumAnalysis(enable, processingSize, numberOfBands, windowType, spectrumPreparationType, smoothFactor);
			});

			/*NOWA::AppStateManager::LogicCommand logicCommand = [this, enable, processingSize, numberOfBands, windowType, spectrumPreparationType, smoothFactor]()
				{
					this->sound->enableSpectrumAnalysis(enable, processingSize, numberOfBands, windowType, spectrumPreparationType, smoothFactor);
				};
			NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));*/
		}
	}

	void SimpleSoundComponent::setOnSpectrumAnalysisFunctionName(const Ogre::String& onSpectrumAnalysisFunctionName)
	{
		if (nullptr == this->sound)
			return;

		this->onSpectrumAnalysisFunctionName->setValue(onSpectrumAnalysisFunctionName);
		this->onSpectrumAnalysisFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onSpectrumAnalysisFunctionName + "()");
		if (false == onSpectrumAnalysisFunctionName.empty())
		{
			this->sound->addSoundSpectrumHandler(this, &SimpleSoundComponent::soundSpectrumFuncPtr);
		}
		else
		{
			this->sound->removeSoundSpectrumHandler();
		}
	}

	Ogre::String SimpleSoundComponent::getOnSpectrumAnalysisFunctionName(void) const
	{
		return this->onSpectrumAnalysisFunctionName->getString();
	}

	Ogre::Real SimpleSoundComponent::getCurrentSpectrumTimePosSec(void) const
	{
		if (nullptr != this->sound)
		{
			return this->sound->getCurrentSpectrumTimePosSec();
		}
		return 0;
	}

	int SimpleSoundComponent::getActualSpectrumSize(void) const
	{
		if (nullptr != this->sound && this->sound->getSpectrumParameter())
		{
			return this->sound->getSpectrumParameter()->actualSpectrumSize;
		}
		return 0;
	}

	int SimpleSoundComponent::getFrequency(void) const
	{
		if (nullptr != this->sound)
		{
			return this->sound->getFrequency();
		}
		return 0;
	}

	bool SimpleSoundComponent::isSpectrumArea(OgreAL::AudioProcessor::SpectrumArea spectrumArea) const
	{
		if (nullptr != this->sound && this->sound->getAudioProcessor())
		{
			return this->sound->getAudioProcessor()->isSpectrumArea(spectrumArea);
		}
		return false;
	}

	void SimpleSoundComponent::setFadeInOutTime(const Ogre::Vector2& fadeInOutTime)
	{
		if (nullptr != this->sound)
		{
			this->sound->fadeIn(fadeInOutTime.x);
			this->sound->fadeOut(fadeInOutTime.y);
		}
	}

	void SimpleSoundComponent::setInnerOuterConeAngle(const Ogre::Vector2& innerOuterConeAngle)
	{
		if (nullptr != this->sound)
		{
			this->sound->setInnerConeAngle(innerOuterConeAngle.x);
			this->sound->setOuterConeAngle(innerOuterConeAngle.y);
		}
	}

	void SimpleSoundComponent::setOuterConeGain(Ogre::Real outerConeGain)
	{
		if (nullptr != this->sound)
		{
			this->sound->setOuterConeGain(outerConeGain);
		}
	}

	Ogre::Real SimpleSoundComponent::getOuterConeGain(void) const
	{
		if (nullptr != this->sound)
		{
			return this->sound->getOuterConeGain();
		}
		return 0.0f;
	}

	void SimpleSoundComponent::setPitch(Ogre::Real pitch)
	{
		if (nullptr != this->sound)
		{
			this->sound->setPitch(pitch);
		}
	}

	Ogre::Real SimpleSoundComponent::getPitch(void) const
	{
		if (nullptr != this->sound)
		{
			return this->sound->getPitch();
		}
		return 0.0f;
	}

	void SimpleSoundComponent::setPriority(int priority)
	{
		if (nullptr != this->sound)
		{
			this->sound->setPriority(static_cast<OgreAL::Sound::Priority>(priority));
		}
	}

	int SimpleSoundComponent::getPriority(void) const
	{
		if (nullptr != this->sound)
		{
			return this->sound->getPriority();
		}
		return 0;
	}

	void SimpleSoundComponent::setDistanceValues(const Ogre::Vector3& distanceValues)
	{
		if (nullptr != this->sound)
		{
			this->sound->setDistanceValues(distanceValues.x, distanceValues.y, distanceValues.z);
		}
	}

	/*Ogre::Vector3 SimpleSoundComponent::getDistanceValues(void) const
	{
		return this->distanceValues->getVector3();
	}*/

	void SimpleSoundComponent::setSecondOffset(Ogre::Real secondOffset)
	{
		if (nullptr != this->sound)
		{
			this->sound->setSecondOffset(secondOffset);
		}
	}

	Ogre::Real SimpleSoundComponent::getSecondOffset(void) const
	{
		if (nullptr != this->sound)
		{
			return this->sound->getSecondOffset();
		}
		return 0.0f;
	}
	
	void SimpleSoundComponent::setVelocity(const Ogre::Vector3& velocity)
	{
		if (nullptr != this->sound)
		{
			this->sound->setVelocity(velocity);
		}
	}
		
	Ogre::Vector3 SimpleSoundComponent::getVelocity(void) const
	{
		if (nullptr != this->sound)
		{
			return this->sound->getVelocity();
		}
		return Ogre::Vector3::ZERO;
	}

	void SimpleSoundComponent::setDirection(const Ogre::Vector3& direction)
	{
		if (nullptr != this->sound)
		{
			this->sound->setDirection(direction);
		}
	}

	Ogre::Vector3 SimpleSoundComponent::getDirection(void) const
	{
		if (nullptr != this->sound)
		{
			return this->sound->getDirection();
		}
		return Ogre::Vector3::NEGATIVE_UNIT_Z;
	}

	void SimpleSoundComponent::setupSound(void)
	{
		this->oldPosition = this->gameObjectPtr->getPosition();
		this->oldOrientation = this->gameObjectPtr->getOrientation();

		if (true == this->activated->getBool())
		{
			if (nullptr != this->sound)
			{
				// Do not interrupt sound, when it is currently playing
				if (this->sound->isPlaying())
				{
					return;
				}
			}
		}
		this->createSound();
		if (nullptr != this->sound)
		{
			if (true == this->activated->getBool())
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
				this->destroySound();
			}
		}
	}
		
	OgreAL::Sound* SimpleSoundComponent::getSound(void) const
	{
		return this->sound;
	}

}; // namespace end