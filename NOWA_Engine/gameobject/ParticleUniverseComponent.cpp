#include "NOWAPrecompiled.h"
#include "ParticleUniverseComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/DeployResourceModule.h"
#include "ParticleUniverseSystemManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ParticleUniverseComponent::ParticleUniverseComponent()
		: GameObjectComponent(),
		particle(nullptr),
		particleNode(nullptr),
		particlePlayTime(10000.0f),
		oldActivated(true),
		bIsInSimulation(false),
		activated(new Variant(ParticleUniverseComponent::AttrActivated(), true, this->attributes)),
		particleTemplateName(new Variant(ParticleUniverseComponent::AttrParticleName(), std::vector<Ogre::String>(), this->attributes)),
		repeat(new Variant(ParticleUniverseComponent::AttrRepeat(), false, this->attributes)),
		particleInitialPlayTime(new Variant(ParticleUniverseComponent::AttrPlayTime(), 10000.0f, this->attributes)),
		particlePlaySpeed(new Variant(ParticleUniverseComponent::AttrPlaySpeed(), 1.0f, this->attributes)),
		particleOffsetPosition(new Variant(ParticleUniverseComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		particleOffsetOrientation(new Variant(ParticleUniverseComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		particleScale(new Variant(ParticleUniverseComponent::AttrScale(), Ogre::Vector3::UNIT_SCALE, this->attributes))
	{
		//this->particleTemplateName->setValue(
		//{
		//	"PUMediaPack/Snow",
		//	"PUMediaPack/Bubbles",
		//	"explosionSystem",
		//	"electricBeamSystem",
		//	"PUMediaPack/Fireplace_01",
		//	"flameSystem",
		//	"fireSystem",
		//	"windyFireSystem",
		//	"softParticlesSystem ",
		//	"tornadoSystem",
		//	"example_032",  // forcefield
		//	"example_022",  // RibbonTrailRenderer
		//	"example_023",
		//	"example_027", // Flocking Fish
		//	"PUMediaPack/Teleport",
		//	"PUMediaPack/Atomicity",
		//	"PUMediaPack/LineStreak",
		//	"PUMediaPack/SpiralStars",
		//	"PUMediaPack/TimeShift",
		//	"PUMediaPack/Leaves",
		//	"PUMediaPack/FlareShield",
		//	"PUMediaPack/LightningBolt",
		//	"PUMediaPack/PsychoBurn",
		//	"PUMediaPack/Hypno",
		//	"PUMediaPack/Smoke",
		//	"PUMediaPack/CanOfWorms",
		//	"PUMediaPack/Propulsion",
		//	"PUMediaPack/BlackHole",
		//	"PUMediaPack/ColouredStreaks",
		//	"APUMediaPack/Woosh",
		//	"PUMediaPack/ElectroShield",
		//	"PUMediaPack/Fireplace_02",
		//	"example_028"
		//});
		// Get all available particle effects
		std::vector<ParticleUniverse::String> availableParticleTemplateNames;
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

		// Activated variable is set to false again, when particle has played and repeat is off, so tell it the gui that it must be refreshed
		this->activated->addUserData(GameObject::AttrActionNeedRefresh());
		this->particleInitialPlayTime->setDescription("The particle play time in milliseconds, if set to 0, the particle effect will not stop automatically.");
	}

	ParticleUniverseComponent::~ParticleUniverseComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleUniverseComponent] Destructor particle universe component for game object: " + this->gameObjectPtr->getName());
		this->destroyParticleEffect();
	}

	bool ParticleUniverseComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TemplateName")
		{
			this->particleTemplateName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlayTime")
		{
			this->particleInitialPlayTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			this->particlePlayTime = this->particleInitialPlayTime->getReal();
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlaySpeed")
		{
			this->particlePlaySpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->particleOffsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
		{
			this->particleOffsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Scale")
		{
			this->particleScale->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool ParticleUniverseComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleUniverseComponent] Init particle universe component for game object: " + this->gameObjectPtr->getName());

		if (true == this->particleTemplateName->getString().empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleUniverseComponent] Error: Can not use particle effects, because the resources are not loaded!");
			return true;
		}

		// Particle effect is moving, so it must always be dynamic, else it will not be rendered!
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createParticleEffect();

		return true;
	}

	GameObjectCompPtr ParticleUniverseComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ParticleUniverseCompPtr clonedCompPtr(boost::make_shared<ParticleUniverseComponent>());

		clonedCompPtr->setParticleTemplateName(this->particleTemplateName->getListSelectedValue());
		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setParticlePlayTimeMS(this->particleInitialPlayTime->getReal());
		clonedCompPtr->setParticlePlaySpeed(this->particlePlaySpeed->getReal());
		clonedCompPtr->setParticleOffsetPosition(this->particleOffsetPosition->getVector3());
		clonedCompPtr->setParticleOffsetOrientation(this->particleOffsetOrientation->getVector3());
		clonedCompPtr->setParticleScale(this->particleScale->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ParticleUniverseComponent::createParticleEffect(void)
	{
		if (nullptr == this->gameObjectPtr)
			return true;
		
		if (this->oldParticleTemplateName != this->particleTemplateName->getListSelectedValue())
		{
			this->destroyParticleEffect();
			this->oldParticleTemplateName = this->particleTemplateName->getListSelectedValue();
		}
		if (nullptr == this->particle)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ParticleUniverseComponent::createParticleEffect",
			{
				Ogre::String name = this->particleTemplateName->getListSelectedValue() + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
				this->particle = ParticleUniverse::ParticleSystemManager::getSingletonPtr()->getParticleSystem(name);
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-->create: " + name);
				if (nullptr != this->particle)
				{
					this->particleNode = this->particle->getParentSceneNode();
					this->particle->prepare();
					this->particle->start();
					this->particle->stop();
				}
				else
				{
					this->particle = ParticleUniverse::ParticleSystemManager::getSingletonPtr()->createParticleSystem(name,
						this->particleTemplateName->getListSelectedValue(), this->gameObjectPtr->getSceneManager());

					if (nullptr == this->particle)
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleUniverseComponent] Error: Could not create particle effect: " + this->particleTemplateName->getListSelectedValue());
						return false;
					}

					// Tag resource for deployment
					DeployResourceModule::getInstance()->tagResource(this->particleTemplateName->getListSelectedValue(), this->particle->getResourceGroupName());

					this->particleNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();
					// this->particleNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(); // 
					this->particleNode->setOrientation(MathHelper::getInstance()->degreesToQuat(this->particleOffsetOrientation->getVector3()));
					// Just add the offset because the node is a child of the game object scene node and therefore relative to the parent position
					this->particleNode->setPosition(this->particleOffsetPosition->getVector3());
					// Particle effect is moving, so it must always be dynamic, else it will not be rendered!
					this->particle->setStatic(false);
					this->particleNode->setStatic(false);
					this->particleNode->attachObject(this->particle);
					this->particle->setDefaultQueryFlags(0 << 0);
					this->particle->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
					this->particle->setCastShadows(false);
					this->particle->setScale(this->particleScale->getVector3());
					this->particle->setScaleVelocity(this->particleScale->getVector3().x);
					this->particle->setScaleTime(this->particlePlaySpeed->getReal());
					// Hack according to http://forums.ogre3d.org/viewtopic.php?f=25&t=82012&sid=05ff08d7c249d71d7f78ead03ce082d3&start=25
					// Because else a crash may occur, when particle is just prepared and played in another frame
					this->particle->prepare();
					this->particle->start();
					this->particle->stop();
				}
			});
		}
		return true;
	}

	void ParticleUniverseComponent::destroyParticleEffect(void)
	{
		if (nullptr != this->particle)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ParticleUniverseComponent::destroyParticleEffect",
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-->destroy: " + this->particle->getName());
				this->particle->stop();
				if (true == this->particle->isAttached())
				{
					this->particleNode->detachObject(particle);
				}
				// ATTENTION: Remove resource when stopping particle effect??? Really?
				DeployResourceModule::getInstance()->removeResource(this->particle->getTemplateName());
				ParticleUniverse::ParticleSystemManager::getSingletonPtr()->destroyParticleSystem(this->particle, this->gameObjectPtr->getSceneManager());
				this->gameObjectPtr->getSceneNode()->removeAndDestroyChild(this->particleNode);
				// this->gameObjectPtr->getSceneManager()->getRootSceneNode()->removeAndDestroyChild(this->particleNode);
				this->particle = nullptr;
				this->particleNode = nullptr;
			});
		}
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
	}

	bool ParticleUniverseComponent::connect(void)
	{
		GameObjectComponent::connect();

		bool success = this->createParticleEffect();
		if (true == success)
		{
			this->oldActivated = this->activated->getBool();
			if (true == this->activated->getBool() && nullptr != this->particle)
			{
				this->particlePlayTime = this->particleInitialPlayTime->getReal();
				ENQUEUE_RENDER_COMMAND_WAIT("ParticleUniverseComponent::connect",
				{
					this->particle->prepare();
					this->particle->start();
				});
			}
		}
		return success;
	}

	bool ParticleUniverseComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();
		// this->destroyParticleEffect();
		if (nullptr != this->particle)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ParticleUniverseComponent::disconnect",
			{
				this->particle->stop();
			});
		}
		this->activated->setValue(this->oldActivated);
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
		return true;
	}

	Ogre::String ParticleUniverseComponent::getClassName(void) const
	{
		return "ParticleUniverseComponent";
	}

	Ogre::String ParticleUniverseComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void ParticleUniverseComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->bIsInSimulation = !notSimulating;

		if (false == notSimulating && nullptr != this->particle)
		{
			// Only play activated particle effect
			if (true == this->activated->getBool())
			{
				// play particle effect forever
				if (true == this->repeat->getBool())
					return;
				
				// play particle effect forever
				if (0.0f == this->particleInitialPlayTime->getReal())
				{
					// So when is it over? When to stop, when to deactivate?
					return;
				}
				// control the execution time
				if (this->particlePlayTime > 0.0f)
				{
					this->particlePlayTime -= dt * 1000.0f;
				}
				else
				{
					// this->particle->stopFade();
					ENQUEUE_RENDER_COMMAND_WAIT("ParticleUniverseComponent::update stop",
					{
						this->particle->stop();
					});
					// Set activated to false, so that the particle can be activated at a later time
					if (false == this->repeat->getBool())
					{
						this->activated->setValue(false);
					}
					else
					{
						this->particlePlayTime = this->particleInitialPlayTime->getReal();
					}
				}

				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "#######play time: "
				// 												+ Ogre::StringConverter::toString(this->particlePlayTime) + " for: " + this->gameObjectPtr->getName());
			}
		}
	}

	void ParticleUniverseComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ParticleUniverseComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ParticleUniverseComponent::AttrParticleName() == attribute->getName())
		{
			this->setParticleTemplateName(attribute->getListSelectedValue());
		}
		else if (ParticleUniverseComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (ParticleUniverseComponent::AttrPlayTime() == attribute->getName())
		{
			this->setParticlePlayTimeMS(attribute->getReal());
		}
		else if (ParticleUniverseComponent::AttrPlaySpeed() == attribute->getName())
		{
			this->setParticlePlaySpeed(attribute->getReal());
		}
		else if (ParticleUniverseComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setParticleOffsetPosition(attribute->getVector3());
		}
		else if (ParticleUniverseComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setParticleOffsetOrientation(attribute->getVector3());
		}
		else if (ParticleUniverseComponent::AttrScale() == attribute->getName())
		{
			this->setParticleScale(attribute->getVector3());
		}
	}

	void ParticleUniverseComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "TemplateName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleTemplateName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PlayTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleInitialPlayTime->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PlaySpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particlePlaySpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleOffsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleOffsetOrientation->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Scale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleScale->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	void ParticleUniverseComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (nullptr != this->particle && true == this->bIsInSimulation)
		{
			if (false == activated)
			{
				ENQUEUE_RENDER_COMMAND_WAIT("ParticleUniverseComponent::activated false",
				{
					this->particle->stopFade();
				});
			}
			else
			{
				ENQUEUE_RENDER_COMMAND_WAIT("ParticleUniverseComponent::activated true",
				{
					this->particle->start();
				});
			}
		}
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
		// this->createParticleEffect();
	}

	bool ParticleUniverseComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void ParticleUniverseComponent::setParticleTemplateName(const Ogre::String& particleTemplateName)
	{
		// this->oldParticleTemplateName = this->particleTemplateName->getListSelectedOldValue();
		this->particleTemplateName->setListSelectedValue(particleTemplateName);
		this->createParticleEffect();
	}

	Ogre::String ParticleUniverseComponent::getParticleTemplateName(void) const
	{
		return this->particleTemplateName->getListSelectedValue();
	}

	void ParticleUniverseComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
		this->destroyParticleEffect();
	}

	bool ParticleUniverseComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void ParticleUniverseComponent::setParticlePlayTimeMS(Ogre::Real playTime)
	{
		this->particleInitialPlayTime->setValue(playTime);
		this->particlePlayTime = playTime;
		this->destroyParticleEffect();
	}

	Ogre::Real ParticleUniverseComponent::getParticlePlayTimeMS(void) const
	{
		return this->particleInitialPlayTime->getReal();
	}

	void ParticleUniverseComponent::setParticlePlaySpeed(Ogre::Real playSpeed)
	{
		this->particlePlaySpeed->setValue(playSpeed);
		this->destroyParticleEffect();
	}

	Ogre::Real ParticleUniverseComponent::getParticlePlaySpeed(void) const
	{
		return this->particlePlaySpeed->getReal();
	}

	void ParticleUniverseComponent::setParticleOffsetPosition(const Ogre::Vector3& particleOffsetPosition)
	{
		this->particleOffsetPosition->setValue(particleOffsetPosition);

		if (nullptr != this->particleNode)
		{
			// this->particleNode->setPosition(this->particleOffsetPosition->getVector3());
			NOWA::RenderCommandQueueModule::getInstance()->updateNodePosition(this->particleNode, this->particleOffsetPosition->getVector3());
		}
	}

	Ogre::Vector3 ParticleUniverseComponent::getParticleOffsetPosition(void)
	{
		return this->particleOffsetPosition->getVector3();
	}

	void ParticleUniverseComponent::setParticleOffsetOrientation(const Ogre::Vector3& particleOffsetOrientation)
	{
		this->particleOffsetOrientation->setValue(particleOffsetOrientation);

		if (nullptr != this->particleNode)
		{
			// this->particleNode->setOrientation(MathHelper::getInstance()->degreesToQuat(particleOffsetOrientation));
			NOWA::RenderCommandQueueModule::getInstance()->updateNodeOrientation(this->particleNode, MathHelper::getInstance()->degreesToQuat(particleOffsetOrientation));
		}
	}

	Ogre::Vector3 ParticleUniverseComponent::ParticleUniverseComponent::getParticleOffsetOrientation(void)
	{
		return this->particleOffsetOrientation->getVector3();
	}

	void ParticleUniverseComponent::setParticleScale(const Ogre::Vector3& particleScale)
	{
		this->particleScale->setValue(particleScale);

		if (nullptr != this->particleNode)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("ParticleUniverseComponent::setParticleScale", _1(particleScale),
			{
				this->particle->setScale(this->particleScale->getVector3());
				this->particle->setScaleVelocity(this->particleScale->getVector3().x);
			});
		}
	}

	Ogre::Vector3 ParticleUniverseComponent::getParticleScale(void)
	{
		return this->particleScale->getVector3();
	}

	ParticleUniverse::ParticleSystem* ParticleUniverseComponent::getParticle(void) const
	{
		return this->particle;
	}

	bool ParticleUniverseComponent::isPlaying(void) const
	{
		return this->particlePlayTime > 0.0f;
	}

	void ParticleUniverseComponent::setGlobalPosition(const Ogre::Vector3& particlePosition)
	{
		if (nullptr != this->particleNode)
		{
			Ogre::Vector3 resultPosition = this->particleNode->convertWorldToLocalPosition(particlePosition);
			// this->particleNode->setPosition(resultPosition);
			NOWA::RenderCommandQueueModule::getInstance()->updateNodePosition(this->particleNode, particlePosition);

			// Test this:
#if 0
			Ogre::Matrix4 mat;
			if (!emitter.mNode)
			{
				mat.makeTransform(emitter.mPos, Ogre::Vector3::UNIT_SCALE, emitter.mRot);
			}
			else
			{
				Ogre::Matrix4 matNode;
				matNode = emitter.mNode->_getFullTransformUpdated();
				Ogre::Matrix4 matOffset;
				matOffset.makeTransform(emitter.mPos, Ogre::Vector3::UNIT_SCALE, emitter.mRot);

				mat = matNode * matOffset;
			}
		}
#endif
		}
	}

	void ParticleUniverseComponent::setGlobalOrientation(const Ogre::Vector3& particleOrientation)
	{
		if (nullptr != this->particleNode)
		{
			Ogre::Quaternion globalOrientation = MathHelper::getInstance()->degreesToQuat(particleOrientation);
			Ogre::Quaternion resultOrientation = this->particleNode->convertWorldToLocalOrientation(globalOrientation);
			// this->particleNode->setOrientation(resultOrientation);
			NOWA::RenderCommandQueueModule::getInstance()->updateNodeOrientation(this->particleNode, resultOrientation);

			// Test this:
#if 0
			Ogre::Matrix4 mat;
			if (!emitter.mNode)
			{
				mat.makeTransform(emitter.mPos, Ogre::Vector3::UNIT_SCALE, emitter.mRot);
			}
			else
			{
				Ogre::Matrix4 matNode;
				matNode = emitter.mNode->_getFullTransformUpdated();
				Ogre::Matrix4 matOffset;
				matOffset.makeTransform(emitter.mPos, Ogre::Vector3::UNIT_SCALE, emitter.mRot);

				mat = matNode * matOffset;
			}
#endif
		}
	}

}; // namespace end