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
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"
#include "OgreSceneManager.h"
#include "OgreResourceGroupManager.h"
#include "ParticleSystem/OgreParticleSystemManager2.h"

#include <fstream>
#include <sstream>

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
		isEmitting(false),
		activated(new Variant(ParticleFxComponent::AttrActivated(), true, this->attributes)),
		particleTemplateName(new Variant(ParticleFxComponent::AttrParticleName(), std::vector<Ogre::String>(), this->attributes)),
		repeat(new Variant(ParticleFxComponent::AttrRepeat(), false, this->attributes)),
		particleInitialPlayTime(new Variant(ParticleFxComponent::AttrPlayTime(), 10000.0f, this->attributes)),
		particlePlaySpeed(new Variant(ParticleFxComponent::AttrPlaySpeed(), 1.0f, this->attributes)),
		particleOffsetPosition(new Variant(ParticleFxComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		particleOffsetOrientation(new Variant(ParticleFxComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		particleScale(new Variant(ParticleFxComponent::AttrScale(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		blendingMethod(new Variant(ParticleFxComponent::AttrBlendingMethod(), { "Alpha Hashing", "Alpha Hashing + A2C", "Alpha Blending" }, this->attributes))
	{
		// Activated variable is set to false again, when particle has played and repeat is off, so tell it the gui that it must be refreshed
		this->activated->addUserData(GameObject::AttrActionNeedRefresh());
		this->particleInitialPlayTime->setDescription("The particle play time in milliseconds, if set to 0, the particle effect will not stop automatically.");
		this->blendingMethod->setListSelectedValue("Alpha Blending");
		this->blendingMethod->setDescription("Alpha Hashing: Fast, no sorting needed. Alpha Hashing + A2C: Best quality with MSAA. Alpha Blending: Traditional transparency.");
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
			// Get all .particle2 files from the ParticleFX2 resource group
			std::vector<Ogre::String> particle2Files = Core::getSingletonPtr()->getFilePathNames(
				"ParticleFX2", "", "*.particle2", false);

			// Parse each .particle2 file to extract particle system names
			for (const auto& filePath : particle2Files)
			{
				std::ifstream file(filePath);
				if (file.is_open())
				{
					std::string line;
					while (std::getline(file, line))
					{
						// Trim leading whitespace
						size_t start = line.find_first_not_of(" \t");
						if (start == std::string::npos)
							continue;

						std::string trimmedLine = line.substr(start);

						// Look for lines starting with "particle_system"
						if (Ogre::StringUtil::startsWith(trimmedLine, "particle_system", false))
						{
							// Extract the particle system name after "particle_system"
							// Format: particle_system Particle/Rain or particle_system "Particle/Rain"
							size_t nameStart = trimmedLine.find_first_not_of(" \t", 15); // 15 = length of "particle_system"
							if (nameStart != std::string::npos)
							{
								std::string particleName = trimmedLine.substr(nameStart);

								// Remove trailing whitespace and any trailing comments
								size_t commentPos = particleName.find("//");
								if (commentPos != std::string::npos)
									particleName = particleName.substr(0, commentPos);

								// Trim trailing whitespace
								size_t end = particleName.find_last_not_of(" \t\r\n");
								if (end != std::string::npos)
									particleName = particleName.substr(0, end + 1);

								// Remove quotes if present
								if (!particleName.empty() && particleName.front() == '"')
									particleName = particleName.substr(1);
								if (!particleName.empty() && particleName.back() == '"')
									particleName = particleName.substr(0, particleName.length() - 1);

								// Check if not already in the list and not empty
								if (!particleName.empty() &&
									std::find(names.begin(), names.end(), particleName) == names.end())
								{
									names.emplace_back(particleName);
								}
							}
						}
					}
					file.close();
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
						"[ParticleFxComponent] Could not open particle2 file: " + filePath);
				}
			}

			// If no files found in default location, also try searching via Ogre resource system
			if (names.size() <= 1)
			{
				Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

				// Check if the resource group exists
				if (rgm.resourceGroupExists("ParticleFX2"))
				{
					Ogre::StringVectorPtr resources = rgm.findResourceNames("ParticleFX2", "*.particle2");

					for (const auto& resourceName : *resources)
					{
						try
						{
							Ogre::DataStreamPtr stream = rgm.openResource(resourceName, "ParticleFX2");
							if (stream)
							{
								Ogre::String content = stream->getAsString();
								std::istringstream iss(content);
								std::string line;

								while (std::getline(iss, line))
								{
									// Trim leading whitespace
									size_t start = line.find_first_not_of(" \t");
									if (start == std::string::npos)
										continue;

									std::string trimmedLine = line.substr(start);

									// Look for lines starting with "particle_system"
									if (Ogre::StringUtil::startsWith(trimmedLine, "particle_system", false))
									{
										// Extract the particle system name
										size_t nameStart = trimmedLine.find_first_not_of(" \t", 15);
										if (nameStart != std::string::npos)
										{
											std::string particleName = trimmedLine.substr(nameStart);

											// Remove trailing whitespace and comments
											size_t commentPos = particleName.find("//");
											if (commentPos != std::string::npos)
												particleName = particleName.substr(0, commentPos);

											size_t end = particleName.find_last_not_of(" \t\r\n");
											if (end != std::string::npos)
												particleName = particleName.substr(0, end + 1);

											// Remove quotes if present
											if (!particleName.empty() && particleName.front() == '"')
												particleName = particleName.substr(1);
											if (!particleName.empty() && particleName.back() == '"')
												particleName = particleName.substr(0, particleName.length() - 1);

											if (!particleName.empty() &&
												std::find(names.begin(), names.end(), particleName) == names.end())
											{
												names.emplace_back(particleName);
											}
										}
									}
								}
								stream->close();
							}
						}
						catch (const Ogre::Exception& e)
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
								"[ParticleFxComponent] Could not read resource: " + resourceName + " - " + e.getDescription());
						}
					}
				}
			}
		}
		catch (const Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[ParticleFxComponent] gatherParticleNames exception: " + e.getFullDescription());
		}
		catch (const std::exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[ParticleFxComponent] gatherParticleNames std exception: " + Ogre::String(e.what()));
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendingMethod")
		{
			this->blendingMethod->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Alpha Hashing + A2C"));
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
		clonedCompPtr->setBlendingMethod(this->getBlendingMethod());

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

		Ogre::ParticleSystemManager2* particleManager = this->gameObjectPtr->getSceneManager()->getParticleSystemManager2();

		// Prevents crash with to many particles:
		// primStart + primCount must be <= 288
		
		// Max particles per system (NOT total systems!)
		particleManager->setHighestPossibleQuota(
			512,    // 16-bit index buffer quota
			0       // 32-bit not needed unless you go crazy
		);

		if (true == this->particleTemplateName->getListSelectedValue().empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Warning: No particle template selected for game object: " + this->gameObjectPtr->getName());
			return true;
		}

		// Particle effect is moving, so it must always be dynamic, else it will not be rendered!
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		// this->createParticleEffect();

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
			// The templateName is the full particle system name from .particle2 files (e.g., "Particle/SmokePBS")
			Ogre::String fullParticleSystemName = templateName;

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::createParticleEffect", _2(&success, fullParticleSystemName),
				{
					try
					{
						Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

						// Create the ParticleSystem2 using the particle system name from .particle2 file
						this->particleSystem = sceneManager->createParticleSystem2(fullParticleSystemName);

						if (nullptr == this->particleSystem)
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
								"[ParticleFxComponent] Error: Could not create particle effect: " + fullParticleSystemName);
							success = false;
						}

						if (true == success)
						{
							// Tag resource for deployment
							DeployResourceModule::getInstance()->tagResource(fullParticleSystemName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

							// Create child scene node for the particle effect (relative to game object)
							this->particleNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();

							// Set offset orientation (relative to parent)
							this->particleNode->setOrientation(MathHelper::getInstance()->degreesToQuat(this->particleOffsetOrientation->getVector3()));

							// Set offset position (relative to parent)
							this->particleNode->setPosition(this->particleOffsetPosition->getVector3());

							// Set scale
							this->particleNode->setScale(this->particleScale->getVector3());

							// Particle effect is moving, so it must always be dynamic
							// Set both the node AND the particle system to non-static
							this->particleSystem->setStatic(false);
							this->particleNode->setStatic(false);

							// Attach the particle system to the node
							this->particleNode->attachObject(this->particleSystem);

							// Configure render settings
							this->particleSystem->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
							this->particleSystem->setCastShadows(false);

							// IMPORTANT: Hide the particle node initially!
							// It will be shown when connect() is called with activated=true
							this->particleNode->setVisible(false);
						}
					}
					catch (const Ogre::Exception& e)
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
							"[ParticleFxComponent] Exception creating particle effect: " + e.getFullDescription());
						success = false;
					}
				});

			// Apply blending method after particle system is created
			if (true == success && nullptr != this->particleSystem)
			{
				this->applyBlendingMethod();
			}
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
				this->startParticleEffect();
			}
			else if (false == this->activated->getBool() && nullptr != this->particleNode)
			{
				// If not activated, make sure particle is hidden/stopped
				this->stopParticleEffect();
			}
		}

		return success;
	}

	bool ParticleFxComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		// Stop the particle effect when disconnecting (like ParticleUniverseComponent)
		// this->stopParticleEffect();
		this->destroyParticleEffect();

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
					// Time is up - stop the particle effect
					this->stopParticleEffect();

					// Set activated to false, so that the particle can be activated at a later time
					if (false == this->repeat->getBool())
					{
						this->isEmitting = false;
						this->activated->setValue(false);
					}
					else
					{
						// Reset play time for repeat and restart
						this->particlePlayTime = this->particleInitialPlayTime->getReal();
						this->startParticleEffect();
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
		else if (ParticleFxComponent::AttrBlendingMethod() == attribute->getName())
		{
			Ogre::String selectedValue = attribute->getListSelectedValue();
			if (selectedValue == "Alpha Hashing")
				this->setBlendingMethod(ParticleBlendingMethod::AlphaHashing);
			else if (selectedValue == "Alpha Hashing + A2C")
				this->setBlendingMethod(ParticleBlendingMethod::AlphaHashingA2C);
			else if (selectedValue == "Alpha Blending")
				this->setBlendingMethod(ParticleBlendingMethod::AlphaBlending);
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendingMethod"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blendingMethod->getListSelectedValue())));
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
				this->stopParticleEffect();
			}
			else
			{
				this->startParticleEffect();
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
			NOWA::GraphicsModule::getInstance()->updateNodeScale(this->particleNode, particleScale);
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
		return this->isEmitting && this->activated->getBool();
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

	const Ogre::ParticleSystemDef* ParticleFxComponent::getParticleSystemDef(void) const
	{
		if (nullptr != this->particleSystem)
		{
			return this->particleSystem->getParticleSystemDef();
		}
		return nullptr;
	}

	size_t ParticleFxComponent::getNumActiveParticles(void) const
	{
		const Ogre::ParticleSystemDef* def = this->getParticleSystemDef();
		if (nullptr != def)
		{
			return def->getNumSimdActiveParticles();
		}
		return 0;
	}

	Ogre::uint32 ParticleFxComponent::getParticleQuota(void) const
	{
		const Ogre::ParticleSystemDef* def = this->getParticleSystemDef();
		if (nullptr != def)
		{
			return def->getQuota();
		}
		return 0;
	}

	size_t ParticleFxComponent::getNumEmitters(void) const
	{
		const Ogre::ParticleSystemDef* def = this->getParticleSystemDef();
		if (nullptr != def)
		{
			return def->getNumEmitters();
		}
		return 0;
	}

	size_t ParticleFxComponent::getNumAffectors(void) const
	{
		const Ogre::ParticleSystemDef* def = this->getParticleSystemDef();
		if (nullptr != def)
		{
			return def->getNumAffectors();
		}
		return 0;
	}

	void ParticleFxComponent::startParticleEffect(void)
	{
		if (nullptr == this->particleSystem || nullptr == this->particleNode)
		{
			return;
		}

		auto particleNode = this->particleNode;
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::startParticleEffect", _1(particleNode),
			{
				if (particleNode)
				{
					particleNode->setVisible(true);
				}
			});

		this->isEmitting = true;
	}

	void ParticleFxComponent::stopParticleEffect(void)
	{
		if (nullptr == this->particleNode)
		{
			return;
		}

		auto particleNode = this->particleNode;
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::stopParticleEffect", _1(particleNode),
			{
				if (particleNode)
				{
					particleNode->setVisible(false);
				}
			});

		this->isEmitting = false;
	}

	void ParticleFxComponent::setBlendingMethod(ParticleBlendingMethod::ParticleBlendingMethod blendingMethodValue)
	{
		switch (blendingMethodValue)
		{
		case ParticleBlendingMethod::AlphaHashing:
			this->blendingMethod->setListSelectedValue("Alpha Hashing");
			break;
		case ParticleBlendingMethod::AlphaHashingA2C:
			this->blendingMethod->setListSelectedValue("Alpha Hashing + A2C");
			break;
		case ParticleBlendingMethod::AlphaBlending:
			this->blendingMethod->setListSelectedValue("Alpha Blending");
			break;
		}
		this->applyBlendingMethod();
	}

	ParticleBlendingMethod::ParticleBlendingMethod ParticleFxComponent::getBlendingMethod(void) const
	{
		Ogre::String selectedValue = this->blendingMethod->getListSelectedValue();
		if (selectedValue == "Alpha Hashing")
			return ParticleBlendingMethod::AlphaHashing;
		else if (selectedValue == "Alpha Hashing + A2C")
			return ParticleBlendingMethod::AlphaHashingA2C;
		else
			return ParticleBlendingMethod::AlphaBlending;
	}

	void ParticleFxComponent::applyBlendingMethod(void)
	{
		if (nullptr == this->particleSystem)
		{
			return;
		}

		const Ogre::String& particleSystemName = this->particleTemplateName->getListSelectedValue();
		if (particleSystemName.empty())
		{
			return;
		}

		// The particle system name is the same as the datablock name (e.g., "Particle/SmokePBS")
		// This is a convention in ParticleFX2 - the datablock name matches the particle_system name

		try
		{
			Ogre::HlmsManager* hlmsManager = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager();
			Ogre::HlmsDatablock* datablock = hlmsManager->getDatablock(particleSystemName);

			if (nullptr == datablock)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
					"[ParticleFxComponent] Could not find datablock for blending: " + particleSystemName + " (this may be normal if the material uses a different name)");
				return;
			}

			ParticleBlendingMethod::ParticleBlendingMethod currentMethod = this->getBlendingMethod();

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::applyBlendingMethod", _2(datablock, currentMethod),
				{
					if (currentMethod == ParticleBlendingMethod::AlphaHashing ||
						currentMethod == ParticleBlendingMethod::AlphaHashingA2C)
					{
						Ogre::HlmsBlendblock blendblock = *datablock->getBlendblock();
						blendblock.setBlendType(Ogre::SBT_REPLACE);
						blendblock.mAlphaToCoverage = (currentMethod == ParticleBlendingMethod::AlphaHashingA2C)
							? Ogre::HlmsBlendblock::A2cEnabledMsaaOnly
							: Ogre::HlmsBlendblock::A2cDisabled;
						datablock->setBlendblock(blendblock);

						Ogre::HlmsMacroblock macroblock = *datablock->getMacroblock();
						macroblock.mDepthWrite = true;
						datablock->setMacroblock(macroblock);
						datablock->setAlphaHashing(true);
					}
					else // AlphaBlending
					{
						Ogre::HlmsBlendblock blendblock = *datablock->getBlendblock();
						blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);
						blendblock.mAlphaToCoverage = Ogre::HlmsBlendblock::A2cDisabled;
						datablock->setBlendblock(blendblock);

						Ogre::HlmsMacroblock macroblock = *datablock->getMacroblock();
						macroblock.mDepthWrite = false;
						datablock->setMacroblock(macroblock);
						datablock->setAlphaHashing(false);
					}
				});
		}
		catch (const Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[ParticleFxComponent] Exception applying blending method: " + e.getFullDescription());
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
			.def("getNumActiveParticles", &ParticleFxComponent::getNumActiveParticles)
			.def("getParticleQuota", &ParticleFxComponent::getParticleQuota)
			.def("getNumEmitters", &ParticleFxComponent::getNumEmitters)
			.def("getNumAffectors", &ParticleFxComponent::getNumAffectors)
			.def("setBlendingMethod", &ParticleFxComponent::setBlendingMethod)
			.def("getBlendingMethod", &ParticleFxComponent::getBlendingMethod)

			.enum_("ParticleBlendingMethod")
			[
				luabind::value("AlphaHashing", ParticleBlendingMethod::AlphaHashing),
				luabind::value("AlphaHashingA2C", ParticleBlendingMethod::AlphaHashingA2C),
				luabind::value("AlphaBlending", ParticleBlendingMethod::AlphaBlending)
			]
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
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool isPlaying()", "Checks if the particle effect is currently playing/emitting.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setGlobalPosition(Vector3 position)", "Sets the global world position for the particle effect.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setGlobalOrientation(Vector3 orientation)", "Sets the global orientation in degrees.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumActiveParticles()", "Gets the current number of active particles.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getParticleQuota()", "Gets the particle quota (maximum particles allowed).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumEmitters()", "Gets the number of emitters in the particle system.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumAffectors()", "Gets the number of affectors in the particle system.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setBlendingMethod(ParticleBlendingMethod method)", "Sets the blending method (AlphaHashing=0, AlphaHashingA2C=1, AlphaBlending=2).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "ParticleBlendingMethod getBlendingMethod()", "Gets the current blending method.");

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