#include "NOWAPrecompiled.h"
#include "ParticleFxComponent.h"
#include "gameobject/GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/LuaScriptApi.h"
#include "modules/DeployResourceModule.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"
#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreSceneManager.h"
#include "ParticleSystem/OgreParticleSystemManager2.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ParticleFxComponent::ParticleFxComponent()
		: GameObjectComponent(),
		name("ParticleFxComponent"),
		particleSystem(nullptr),
		particleNode(nullptr),
		particlePlayTime(10000.0f),
		oldActivated(true),
		activated(new Variant(ParticleFxComponent::AttrActivated(), true, this->attributes)),
		particleTemplateName(new Variant(ParticleFxComponent::AttrParticleName(), std::vector<Ogre::String>(), this->attributes)),
		repeat(new Variant(ParticleFxComponent::AttrRepeat(), false, this->attributes)),
		particleInitialPlayTime(new Variant(ParticleFxComponent::AttrPlayTime(), 10000.0f, this->attributes)),
		particlePlaySpeed(new Variant(ParticleFxComponent::AttrPlaySpeed(), 1.0f, this->attributes)),
		particleOffsetPosition(new Variant(ParticleFxComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		particleOffsetOrientation(new Variant(ParticleFxComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		particleScale(new Variant(ParticleFxComponent::AttrScale(), Ogre::Vector3::UNIT_SCALE, this->attributes))
	{
		// Activated variable is set to false again, when particle has played and repeat is off, so tell it the gui that it must be refreshed
		this->activated->addUserData(GameObject::AttrActionNeedRefresh());
		this->particleInitialPlayTime->setDescription("The particle play time in milliseconds, if set to 0, the particle effect will not stop automatically.");
	}

	ParticleFxComponent::~ParticleFxComponent(void)
	{

	}

	void ParticleFxComponent::initialise()
	{
		// Plugin initialise - called when plugin is installed
	}

	const Ogre::String& ParticleFxComponent::getName() const
	{
		return this->name;
	}

	void ParticleFxComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ParticleFxComponent>(ParticleFxComponent::getStaticClassId(), ParticleFxComponent::getStaticClassName());
	}

	void ParticleFxComponent::shutdown()
	{
		// Plugin shutdown
	}

	void ParticleFxComponent::uninstall()
	{
		// Plugin uninstall
	}

	void ParticleFxComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	void ParticleFxComponent::gatherParticleNames(void)
	{
		const Ogre::String prevSelected = this->particleTemplateName->getListSelectedValue();

		std::vector<Ogre::String> names;
		names.reserve(64u);
		names.emplace_back(""); // allow "none"

		try
		{
			// Search through all HLMS types for datablocks starting with "Particle/"
			const std::array<Ogre::HlmsTypes, 7> searchHlms =
			{
				Ogre::HLMS_PBS, Ogre::HLMS_TOON, Ogre::HLMS_UNLIT, Ogre::HLMS_USER0,
				Ogre::HLMS_USER1, Ogre::HLMS_USER2, Ogre::HLMS_USER3
			};

			Ogre::HlmsManager* hlmsManager = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager();

			for (auto searchHlmsIt = searchHlms.begin(); searchHlmsIt != searchHlms.end(); ++searchHlmsIt)
			{
				Ogre::Hlms* hlms = hlmsManager->getHlms(*searchHlmsIt);
				if (nullptr != hlms)
				{
					for (auto& it = hlms->getDatablockMap().cbegin(); it != hlms->getDatablockMap().cend(); ++it)
					{
						const Ogre::String& datablockName = it->second.name;

						// Check if the datablock name starts with "Particle/"
						if (Ogre::StringUtil::startsWith(datablockName, "Particle/", false))
						{
							// Extract the name part after "Particle/"
							Ogre::String particleName = datablockName.substr(9); // Length of "Particle/" is 9

							// Check if not already in the list
							if (std::find(names.begin(), names.end(), particleName) == names.end())
							{
								names.emplace_back(particleName);
							}
						}
					}
				}
			}
		}
		catch (const Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxComponent] gatherParticleNames exception: " + e.getFullDescription());
		}

		// Sort alphabetically (keep "" at front)
		if (names.size() > 2u)
			std::sort(names.begin() + 1u, names.end());

		this->particleTemplateName->setValue(names);

		// Restore previous selection if it still exists, otherwise select first valid option
		if (!prevSelected.empty() && std::find(names.begin(), names.end(), prevSelected) != names.end())
		{
			this->particleTemplateName->setListSelectedValue(prevSelected);
		}
		else if (names.size() > 1u)
		{
			this->particleTemplateName->setListSelectedValue(names[1]); // Select first non-empty
		}
	}

	bool ParticleFxComponent::init(rapidxml::xml_node<>*& propertyElement)
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
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlayTime")
		{
			this->particleInitialPlayTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10000.0f));
			this->particlePlayTime = this->particleInitialPlayTime->getReal();
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlaySpeed")
		{
			this->particlePlaySpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
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

	GameObjectCompPtr ParticleFxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ParticleFxComponentPtr clonedCompPtr(boost::make_shared<ParticleFxComponent>());

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

	bool ParticleFxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Init particle fx component for game object: " + this->gameObjectPtr->getName());

		// Refresh the particle names list
		this->gatherParticleNames();

		if (true == this->particleTemplateName->getListSelectedValue().empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Warning: No particle template selected for game object: " + this->gameObjectPtr->getName());
			return true;
		}

		// Particle effect is moving, so it must always be dynamic, else it will not be rendered!
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createParticleEffect();

		return true;
	}

	bool ParticleFxComponent::createParticleEffect(void)
	{
		if (nullptr == this->gameObjectPtr)
			return true;

		const Ogre::String& templateName = this->particleTemplateName->getListSelectedValue();

		if (templateName.empty())
		{
			return true;
		}

		// Check if template changed
		if (this->oldParticleTemplateName != templateName)
		{
			this->destroyParticleEffect();
			this->oldParticleTemplateName = templateName;
		}

		bool success = true;

		if (nullptr == this->particleSystem)
		{
			// Construct the full datablock name (e.g., "Particle/SmokePBS")
			Ogre::String fullDatablockName = "Particle/" + templateName;

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::createParticleEffect", _3(&success, fullDatablockName, templateName),
				{
					try
					{
						Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

						// Create the ParticleSystem2 using the datablock name
						this->particleSystem = sceneManager->createParticleSystem2(fullDatablockName);

						if (nullptr == this->particleSystem)
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
								"[ParticleFxComponent] Error: Could not create particle effect: " + fullDatablockName);
							success = false;
						}

						if (true == success)
						{
							// Tag resource for deployment
							DeployResourceModule::getInstance()->tagResource(templateName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

							// Create child scene node for the particle effect
							this->particleNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();

							// Set offset orientation
							this->particleNode->setOrientation(MathHelper::getInstance()->degreesToQuat(this->particleOffsetOrientation->getVector3()));

							// Set offset position (relative to parent)
							this->particleNode->setPosition(this->particleOffsetPosition->getVector3());

							// Set scale
							this->particleNode->setScale(this->particleScale->getVector3());

							// Particle effect is moving, so it must always be dynamic
							this->particleNode->setStatic(false);

							// Attach the particle system to the node
							this->particleNode->attachObject(this->particleSystem);

							// Configure render settings
							this->particleSystem->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
							this->particleSystem->setCastShadows(false);
						}
					}
					catch (const Ogre::Exception& e)
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
							"[ParticleFxComponent] Exception creating particle effect: " + e.getFullDescription());
						success = false;
					}
				});
		}
		return success;
	}

	void ParticleFxComponent::destroyParticleEffect(void)
	{
		if (nullptr != this->particleSystem)
		{
			GraphicsModule::RenderCommand renderCommand = [this]()
			{
				try
				{
					// Detach from scene node if attached
					if (nullptr != this->particleNode && this->particleSystem->isAttached())
					{
						this->particleNode->detachObject(this->particleSystem);
					}

					// Remove tagged resource
					DeployResourceModule::getInstance()->removeResource(this->particleTemplateName->getListSelectedValue());

					// Destroy the particle system through the scene manager
					Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
					sceneManager->destroyParticleSystem2(this->particleSystem);

					// Remove and destroy the scene node
					if (nullptr != this->particleNode)
					{
						this->gameObjectPtr->getSceneNode()->removeAndDestroyChild(this->particleNode);
					}

					this->particleSystem = nullptr;
					this->particleNode = nullptr;
				}
				catch (const Ogre::Exception& e)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
						"[ParticleFxComponent] Exception destroying particle effect: " + e.getFullDescription());
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ParticleFxComponent::destroyParticleEffect");
		}
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
	}

	bool ParticleFxComponent::connect(void)
	{
		GameObjectComponent::connect();

		bool success = this->createParticleEffect();
		if (true == success)
		{
			this->oldActivated = this->activated->getBool();
			if (true == this->activated->getBool() && nullptr != this->particleSystem)
			{
				this->particlePlayTime = this->particleInitialPlayTime->getReal();

				//ENQUEUE_RENDER_COMMAND("ParticleFxComponent::connect",
				//	{
				//		// ParticleSystem2 doesn't have prepare/start like ParticleUniverse
				//		// It's automatically active when attached to a scene node
				//		// The system is controlled by enabling/disabling emitters
				//	});
			}
		}

		return success;
	}

	bool ParticleFxComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		this->activated->setValue(this->oldActivated);
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
		return true;
	}

	bool ParticleFxComponent::onCloned(void)
	{
		return true;
	}

	void ParticleFxComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Destructor particle fx component for game object: " + this->gameObjectPtr->getName());
		this->destroyParticleEffect();
	}

	void ParticleFxComponent::onOtherComponentRemoved(unsigned int index)
	{
		// Nothing special to do
	}

	void ParticleFxComponent::onOtherComponentAdded(unsigned int index)
	{
		// Nothing special to do
	}

	void ParticleFxComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && nullptr != this->particleSystem)
		{
			// Update camera position for the particle system manager
			// This is important for proper billboard orientation
			Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
			Ogre::ParticleSystemManager2* particleManager = sceneManager->getParticleSystemManager2();
			if (nullptr != particleManager)
			{
				Ogre::Camera* camera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
				if (nullptr != camera)
				{
					auto closureFunction = [this, particleManager, camera](Ogre::Real renderDt)
					{
						particleManager->setCameraPosition(camera->getDerivedPosition());
					};
					Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
					NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
				}
			}

			// Only process activated particle effects
			if (true == this->activated->getBool())
			{
				// Play particle effect forever if repeat is enabled
				if (true == this->repeat->getBool())
					return;

				// Play particle effect forever if play time is 0
				if (0.0f == this->particleInitialPlayTime->getReal())
				{
					return;
				}

				// Control the execution time
				if (this->particlePlayTime > 0.0f)
				{
					this->particlePlayTime -= dt * 1000.0f * this->particlePlaySpeed->getReal();
				}
				else
				{
					// Time is up - deactivate if not repeating
					if (false == this->repeat->getBool())
					{
						this->activated->setValue(false);
					}
					else
					{
						// Reset play time for repeat
						this->particlePlayTime = this->particleInitialPlayTime->getReal();
					}
				}
			}
		}
	}

	void ParticleFxComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ParticleFxComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ParticleFxComponent::AttrParticleName() == attribute->getName())
		{
			this->setParticleTemplateName(attribute->getListSelectedValue());
		}
		else if (ParticleFxComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (ParticleFxComponent::AttrPlayTime() == attribute->getName())
		{
			this->setParticlePlayTimeMS(attribute->getReal());
		}
		else if (ParticleFxComponent::AttrPlaySpeed() == attribute->getName())
		{
			this->setParticlePlaySpeed(attribute->getReal());
		}
		else if (ParticleFxComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setParticleOffsetPosition(attribute->getVector3());
		}
		else if (ParticleFxComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setParticleOffsetOrientation(attribute->getVector3());
		}
		else if (ParticleFxComponent::AttrScale() == attribute->getName())
		{
			this->setParticleScale(attribute->getVector3());
		}
	}

	void ParticleFxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// Type codes:
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleOffsetOrientation->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Scale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleScale->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String ParticleFxComponent::getClassName(void) const
	{
		return "ParticleFxComponent";
	}

	Ogre::String ParticleFxComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void ParticleFxComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (nullptr != this->particleSystem && true == this->bConnected)
		{
			if (false == this->activated->getBool())
			{
				// Hide the particle system by detaching or making invisible
				auto particleNode = this->particleNode;
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::activated false", _1(particleNode),
					{
						if (particleNode)
						{
							particleNode->setVisible(false);
						}
					});
			}
			else
			{
				// Show and reset the particle system
				auto particleNode = this->particleNode;
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::activated true", _1(particleNode),
					{
						if (particleNode)
						{
							particleNode->setVisible(true);
						}
					});
			}
		}
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
	}

	bool ParticleFxComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void ParticleFxComponent::setParticleTemplateName(const Ogre::String& particleTemplateName)
	{
		this->particleTemplateName->setListSelectedValue(particleTemplateName);
		this->createParticleEffect();
	}

	Ogre::String ParticleFxComponent::getParticleTemplateName(void) const
	{
		return this->particleTemplateName->getListSelectedValue();
	}

	void ParticleFxComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool ParticleFxComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void ParticleFxComponent::setParticlePlayTimeMS(Ogre::Real playTime)
	{
		this->particleInitialPlayTime->setValue(playTime);
		this->particlePlayTime = playTime;
	}

	Ogre::Real ParticleFxComponent::getParticlePlayTimeMS(void) const
	{
		return this->particleInitialPlayTime->getReal();
	}

	void ParticleFxComponent::setParticlePlaySpeed(Ogre::Real playSpeed)
	{
		this->particlePlaySpeed->setValue(playSpeed);
	}

	Ogre::Real ParticleFxComponent::getParticlePlaySpeed(void) const
	{
		return this->particlePlaySpeed->getReal();
	}

	void ParticleFxComponent::setParticleOffsetPosition(const Ogre::Vector3& particleOffsetPosition)
	{
		this->particleOffsetPosition->setValue(particleOffsetPosition);

		if (nullptr != this->particleNode)
		{
			NOWA::GraphicsModule::getInstance()->updateNodePosition(this->particleNode, particleOffsetPosition);
		}
	}

	Ogre::Vector3 ParticleFxComponent::getParticleOffsetPosition(void) const
	{
		return this->particleOffsetPosition->getVector3();
	}

	void ParticleFxComponent::setParticleOffsetOrientation(const Ogre::Vector3& particleOffsetOrientation)
	{
		this->particleOffsetOrientation->setValue(particleOffsetOrientation);

		if (nullptr != this->particleNode)
		{
			NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->particleNode, MathHelper::getInstance()->degreesToQuat(particleOffsetOrientation));
		}
	}

	Ogre::Vector3 ParticleFxComponent::getParticleOffsetOrientation(void) const
	{
		return this->particleOffsetOrientation->getVector3();
	}

	void ParticleFxComponent::setParticleScale(const Ogre::Vector3& particleScale)
	{
		this->particleScale->setValue(particleScale);

		if (nullptr != this->particleNode)
		{
			auto node = this->particleNode;
			auto scale = particleScale;
			ENQUEUE_RENDER_COMMAND_MULTI("ParticleFxComponent::setParticleScale", _2(node, scale),
				{
					node->setScale(scale);
				});
		}
	}

	Ogre::Vector3 ParticleFxComponent::getParticleScale(void) const
	{
		return this->particleScale->getVector3();
	}

	Ogre::ParticleSystem2* ParticleFxComponent::getParticleSystem(void) const
	{
		return this->particleSystem;
	}

	bool ParticleFxComponent::isPlaying(void) const
	{
		return this->particlePlayTime > 0.0f && this->activated->getBool();
	}

	void ParticleFxComponent::setGlobalPosition(const Ogre::Vector3& particlePosition)
	{
		if (nullptr != this->particleNode)
		{
			Ogre::Vector3 resultPosition = this->particleNode->convertWorldToLocalPosition(particlePosition);
			NOWA::GraphicsModule::getInstance()->updateNodePosition(this->particleNode, resultPosition);
		}
	}

	void ParticleFxComponent::setGlobalOrientation(const Ogre::Vector3& particleOrientation)
	{
		if (nullptr != this->particleNode)
		{
			Ogre::Quaternion globalOrientation = MathHelper::getInstance()->degreesToQuat(particleOrientation);
			Ogre::Quaternion resultOrientation = this->particleNode->convertWorldToLocalOrientation(globalOrientation);
			NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->particleNode, resultOrientation);
		}
	}

	// Lua registration part

	ParticleFxComponent* getParticleFxComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<ParticleFxComponent>(gameObject->getComponentWithOccurrence<ParticleFxComponent>(occurrenceIndex)).get();
	}

	ParticleFxComponent* getParticleFxComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ParticleFxComponent>(gameObject->getComponent<ParticleFxComponent>()).get();
	}

	ParticleFxComponent* getParticleFxComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ParticleFxComponent>(gameObject->getComponentFromName<ParticleFxComponent>(name)).get();
	}

	void ParticleFxComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
			[
				class_<ParticleFxComponent, GameObjectComponent>("ParticleFxComponent")
					.def("setActivated", &ParticleFxComponent::setActivated)
					.def("isActivated", &ParticleFxComponent::isActivated)
					.def("setParticleTemplateName", &ParticleFxComponent::setParticleTemplateName)
					.def("getParticleTemplateName", &ParticleFxComponent::getParticleTemplateName)
					.def("setRepeat", &ParticleFxComponent::setRepeat)
					.def("getRepeat", &ParticleFxComponent::getRepeat)
					.def("setParticlePlayTimeMS", &ParticleFxComponent::setParticlePlayTimeMS)
					.def("getParticlePlayTimeMS", &ParticleFxComponent::getParticlePlayTimeMS)
					.def("setParticlePlaySpeed", &ParticleFxComponent::setParticlePlaySpeed)
					.def("getParticlePlaySpeed", &ParticleFxComponent::getParticlePlaySpeed)
					.def("setParticleOffsetPosition", &ParticleFxComponent::setParticleOffsetPosition)
					.def("getParticleOffsetPosition", &ParticleFxComponent::getParticleOffsetPosition)
					.def("setParticleOffsetOrientation", &ParticleFxComponent::setParticleOffsetOrientation)
					.def("getParticleOffsetOrientation", &ParticleFxComponent::getParticleOffsetOrientation)
					.def("setParticleScale", &ParticleFxComponent::setParticleScale)
					.def("getParticleScale", &ParticleFxComponent::getParticleScale)
					.def("isPlaying", &ParticleFxComponent::isPlaying)
					.def("setGlobalPosition", &ParticleFxComponent::setGlobalPosition)
					.def("setGlobalOrientation", &ParticleFxComponent::setGlobalOrientation)
			];

		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "class inherits GameObjectComponent", ParticleFxComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleTemplateName(String templateName)", "Sets the particle template name (without 'Particle/' prefix).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "String getParticleTemplateName()", "Gets the particle template name.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setRepeat(bool repeat)", "Sets whether the particle effect should repeat.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool getRepeat()", "Gets whether the particle effect repeats.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticlePlayTimeMS(number playTime)", "Sets the particle play time in milliseconds (0 = infinite).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getParticlePlayTimeMS()", "Gets the particle play time in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticlePlaySpeed(number playSpeed)", "Sets the particle play speed multiplier (1.0 = normal).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getParticlePlaySpeed()", "Gets the particle play speed multiplier.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleOffsetPosition(Vector3 position)", "Sets the offset position relative to the game object.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "Vector3 getParticleOffsetPosition()", "Gets the offset position.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleOffsetOrientation(Vector3 orientation)", "Sets the offset orientation in degrees.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "Vector3 getParticleOffsetOrientation()", "Gets the offset orientation in degrees.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleScale(Vector3 scale)", "Sets the particle scale.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "Vector3 getParticleScale()", "Gets the particle scale.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool isPlaying()", "Checks if the particle effect is currently playing.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setGlobalPosition(Vector3 position)", "Sets the global world position for the particle effect.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setGlobalOrientation(Vector3 orientation)", "Sets the global orientation in degrees.");

		gameObjectClass.def("getParticleFxComponentFromName", &getParticleFxComponentFromName);
		gameObjectClass.def("getParticleFxComponent", (ParticleFxComponent * (*)(GameObject*)) & getParticleFxComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getParticleFxComponentFromIndex", (ParticleFxComponent * (*)(GameObject*, unsigned int)) & getParticleFxComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ParticleFxComponent getParticleFxComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castParticleFxComponent", &GameObjectController::cast<ParticleFxComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ParticleFxComponent castParticleFxComponent(ParticleFxComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool ParticleFxComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end