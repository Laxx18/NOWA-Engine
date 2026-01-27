#include "NOWAPrecompiled.h"
#include "ParticleFxComponent.h"
#include "gameobject/GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/LuaScriptApi.h"
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

// Available emitter parameters:
/*
"angle : The angle up to which particles may vary in their initial direction from the emitter direction, in degrees.",
"colour : The colour of emitted particles.",
"colour_range_start : The start of a range of colours to be assigned to emitted particles.",
"colour_range_end : The end of a range of colours to be assigned to emitted particles.",
"direction : The base direction of the emitter.",
"up : The up vector of the emitter.",
"direction_position_reference : Reference position used to calculate particle direction based on position, useful for explosions or implosions.",
"emission_rate : The number of particles emitted per second.",
"position : The position of the emitter relative to the particle system center.",
"velocity : The initial velocity assigned to every particle in world units per second.",
"velocity_min : The minimum initial velocity assigned to particles.",
"velocity_max : The maximum initial velocity assigned to particles.",
"time_to_live : The lifetime of each particle in seconds.",
"time_to_live_min : The minimum lifetime of each particle in seconds.",
"time_to_live_max : The maximum lifetime of each particle in seconds.",
"duration : The length of time in seconds which the emitter stays enabled.",
"duration_min : The minimum length of time in seconds which the emitter stays enabled.",
"duration_max : The maximum length of time in seconds which the emitter stays enabled.",
"repeat_delay : Delay in seconds before the emitter is re-enabled after disabling.",
"repeat_delay_min : Minimum delay before the emitter is re-enabled after disabling.",
"repeat_delay_max : Maximum delay before the emitter is re-enabled after disabling.",
"name : The name of the emitter.",
"emit_emitter : If enabled, this emitter emits other emitters instead of visual particles.",
"width : Width of the emitter shape in world coordinates.",
"height : Height of the emitter shape in world coordinates.",
"depth : Depth of the emitter shape in world coordinates."
*/

/*******************************************************************************
 * TESTING CHECKLIST
 *******************************************************************************
 *
 * After implementing all changes, test these scenarios:
 *
 * 1. CONNECT/DISCONNECT:
 *    - Start simulation -> particle plays from beginning
 *    - Stop simulation -> particle stops
 *    - Start again -> particle plays from beginning again
 *    particlePlayTime should reset to initial value each time
 *
 * 2. PLAYTIME:
 *    - Set playtime to 5000ms, playSpeed to 1.0 -> should play for 5 seconds
 *    - Set playtime to 5000ms, playSpeed to 2.0 -> should play for 2.5 seconds
 *    - Set playtime to 5000ms, playSpeed to 0.5 -> should play for 10 seconds
 *    Time countdown must use playSpeed multiplier
 *
 * 3. REPEAT:
 *    - Enable repeat -> particle loops forever
 *    - Disable repeat -> particle plays once then stops
 *    Each loop starts from beginning
 *
 * 4. SCALE CHANGES:
 *    - Change scale during play -> should continue playing at same point
 *    - Change scale multiple times -> no memory leaks
 *    Only ONE clone should exist per GameObject
 *
 * 5. TEMPLATE CHANGES:
 *    - Change template -> old clone destroyed, new clone created
 *    - Change template multiple times -> no zombie clones
 *    Memory should not grow
 *
 * 6. COMPONENT REMOVAL:
 *    - Remove component -> everything cleaned up
 *    - Check memory -> clone should be destroyed
 *    No leaks
 *
 ******************************************************************************/

/*
	Approach:
	Start:     0 clones
	Scale:     1 clone (reused)
	Scale:     1 clone (reused)
	Scale:     1 clone (reused)
	Template:  0 clones (destroyed), then 1 clone (new template)
	Template:  0 clones (destroyed), then 1 clone (new template)

	Remove:    0 clones (properly destroyed)
*/

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
		fadeState(ParticleFadeState::None),
		fadeProgress(0.0f),
		pendingDrainTimeMs(0.0f),
		pendingRestartAfterDrain(false),
		activated(new Variant(ParticleFxComponent::AttrActivated(), true, this->attributes)),
		particleTemplateName(new Variant(ParticleFxComponent::AttrParticleName(), std::vector<Ogre::String>(), this->attributes)),
		repeat(new Variant(ParticleFxComponent::AttrRepeat(), false, this->attributes)),
		particleInitialPlayTime(new Variant(ParticleFxComponent::AttrPlayTime(), 10000.0f, this->attributes)),
		particlePlaySpeed(new Variant(ParticleFxComponent::AttrPlaySpeed(), 1.0f, this->attributes)),
		particleOffsetPosition(new Variant(ParticleFxComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		particleOffsetOrientation(new Variant(ParticleFxComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		particleScale(new Variant(ParticleFxComponent::AttrScale(), Ogre::Vector2::UNIT_SCALE, this->attributes)),
		blendingMethod(new Variant(ParticleFxComponent::AttrBlendingMethod(), { "Alpha Hashing", "Alpha Hashing + A2C", "Alpha Blending" }, this->attributes)),
		fadeIn(new Variant(ParticleFxComponent::AttrFadeIn(), false, this->attributes)),
		fadeInTime(new Variant(ParticleFxComponent::AttrFadeInTime(), 1000.0f, this->attributes)),
		fadeOut(new Variant(ParticleFxComponent::AttrFadeOut(), false, this->attributes)),
		fadeOutTime(new Variant(ParticleFxComponent::AttrFadeOutTime(), 1000.0f, this->attributes))
	{
		// Activated variable is set to false again, when particle has played and repeat is off, so tell it the gui that it must be refreshed
		this->activated->addUserData(GameObject::AttrActionNeedRefresh());
		this->particleInitialPlayTime->setDescription("The particle play time in milliseconds, if set to 0, the particle effect will not stop automatically.");
		this->particlePlaySpeed->setDescription("The particle flow speed multiplier. Values > 1.0 make particles flow faster, < 1.0 slower. Also affects play time countdown.");
		this->blendingMethod->setListSelectedValue("Alpha Blending");
		this->blendingMethod->setDescription("Alpha Hashing: Fast, no sorting needed. Alpha Hashing + A2C: Best quality with MSAA. Alpha Blending: Traditional transparency.");
		this->fadeIn->setDescription("If enabled, the particle effect will gradually increase emission rate from 0 to max when starting.");
		this->fadeInTime->setDescription("The fade in duration in milliseconds.");
		this->fadeOut->setDescription("If enabled, the particle effect will gradually decrease emission rate to 0 before stopping.");
		this->fadeOutTime->setDescription("The fade out duration in milliseconds.");
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
		names.emplace_back("");

		Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
		Ogre::ParticleSystemManager2* particleManager = sceneManager->getParticleSystemManager2();

		if (nullptr == particleManager)
		{
			return;
		}

		Ogre::ResourceGroupManager& resourceGroupManager = Ogre::ResourceGroupManager::getSingleton();

		if (false == resourceGroupManager.resourceGroupExists("ParticleFX2"))
		{
			return;
		}

		Ogre::StringVectorPtr scripts = resourceGroupManager.findResourceNames("ParticleFX2", "*.particle2");

		for (const Ogre::String& scriptName : *scripts)
		{
			try
			{
				Ogre::DataStreamPtr stream = resourceGroupManager.openResource(scriptName, "ParticleFX2");
				if (!stream)
				{
					continue;
				}

				Ogre::String origin = scriptName;

				std::istringstream iss(stream->getAsString());
				std::string line;

				while (std::getline(iss, line))
				{
					size_t start = line.find_first_not_of(" \t");
					if (start == std::string::npos)
					{
						continue;
					}

					std::string trimmed = line.substr(start);

					if (!Ogre::StringUtil::startsWith(trimmed, "particle_system", false))
					{
						continue;
					}

					size_t nameStart = trimmed.find_first_not_of(" \t", 15);
					if (nameStart == std::string::npos)
					{
						continue;
					}

					std::string systemName = trimmed.substr(nameStart);
					size_t inheritPos = systemName.find(':');
					if (inheritPos != std::string::npos)
					{
						systemName = systemName.substr(0, inheritPos);
					}

					Ogre::StringUtil::trim(systemName);

					if (!particleManager->hasParticleSystemDef(systemName))
					{
						continue;
					}

					Ogre::ParticleSystemDef* particleSystemDef = particleManager->getParticleSystemDef(systemName);

					if (particleSystemDef->getOrigin() != origin)
					{
						continue;
					}

					if (std::find(names.begin(), names.end(), systemName) == names.end())
					{
						names.emplace_back(systemName);
					}
				}
			}
			catch (...)
			{
			}
		}

		if (names.size() > 2u)
		{
			std::sort(names.begin() + 1u, names.end());
		}

		this->particleTemplateName->setValue(names);

		if (!prevSelected.empty() && std::find(names.begin(), names.end(), prevSelected) != names.end())
		{
			this->particleTemplateName->setListSelectedValue(prevSelected);
		}
		else if (names.size() > 1u)
		{
			this->particleTemplateName->setListSelectedValue(names[1]);
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
			this->particleScale->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendingMethod")
		{
			this->blendingMethod->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Alpha Hashing + A2C"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeIn")
		{
			this->fadeIn->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeInTime")
		{
			this->fadeInTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1000.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeOut")
		{
			this->fadeOut->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeOutTime")
		{
			this->fadeOutTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1000.0f));
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
		clonedCompPtr->setParticleScale(this->particleScale->getVector2());
		clonedCompPtr->setBlendingMethod(this->getBlendingMethod());
		clonedCompPtr->setFadeIn(this->fadeIn->getBool());
		clonedCompPtr->setFadeInTimeMS(this->fadeInTime->getReal());
		clonedCompPtr->setFadeOut(this->fadeOut->getBool());
		clonedCompPtr->setFadeOutTimeMS(this->fadeOutTime->getReal());

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
		particleManager->setHighestPossibleQuota(512, 0);

		if (true == this->particleTemplateName->getListSelectedValue().empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Warning: No particle template selected for game object: " + this->gameObjectPtr->getName());
			return true;
		}

		// Particle effect is moving, so it must always be dynamic, else it will not be rendered!
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool ParticleFxComponent::createParticleEffect(void)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return true;
		}

		const Ogre::String& templateName = this->particleTemplateName->getListSelectedValue();
		if (true == templateName.empty())
		{
			return true;
		}

		// Clone must be unique per (GameObject, Template)
		const Ogre::String cloneName = "ParticleFX_NOWA_" + templateName + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

		// Template changed? Destroy EVERYTHING and start fresh
		if (false == this->baseParticleTemplateName.empty() && this->baseParticleTemplateName != templateName)
		{
			this->destroyEverything();
		}

		// Already created?
		if (nullptr != this->particleSystem)
		{
			return true;
		}

		GraphicsModule::RenderCommand renderCommand = [this, templateName, cloneName]()
		{
			Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
			Ogre::ParticleSystemManager2* particleManager = sceneManager->getParticleSystemManager2();

			Ogre::ParticleSystemDef* particleSystemDefInstance = nullptr;

			// If our clone exists, reuse it
			if (particleManager && true == particleManager->hasParticleSystemDef(cloneName, false))
			{
				particleSystemDefInstance = particleManager->getParticleSystemDef(cloneName, false);
				this->resetClone(particleSystemDefInstance);
			}
			else
			{
				Ogre::ParticleSystemDef* baseDef = particleManager->getParticleSystemDef(templateName);

				try
				{
					particleSystemDefInstance = baseDef->clone(cloneName, particleManager);
				}
				catch (...)
				{
					particleSystemDefInstance = particleManager->getParticleSystemDef(cloneName, false);
				}

				this->resetClone(particleSystemDefInstance);
			}

			if (particleSystemDefInstance && false == particleSystemDefInstance->isInitialized())
			{
				Ogre::VaoManager* vaoManager = Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->getVaoManager();
				particleSystemDefInstance->init(vaoManager);
			}

			this->particleSystem = sceneManager->createParticleSystem2(cloneName);
			if (nullptr == this->particleSystem)
			{
				return;
			}

			this->particleNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();
			this->particleNode->setOrientation(MathHelper::getInstance()->degreesToQuat(this->particleOffsetOrientation->getVector3()));
			this->particleNode->setPosition(this->particleOffsetPosition->getVector3());
			this->particleNode->attachObject(this->particleSystem);

			this->particleSystem->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
			this->particleSystem->setCastShadows(false);

			this->applyBlendingMethod();
			this->storeOriginalEmissionRates();

			Ogre::Real speed = this->particlePlaySpeed->getReal();
			if (speed != 1.0f)
			{
				this->applyVelocitySpeedFactor(speed);
			}

			if (this->fadeIn->getBool())
			{
				this->fadeState = ParticleFadeState::FadingIn;
				this->fadeProgress = 0.0f;
				this->setEmissionRateFactor(0.0f);
			}
			else
			{
				this->fadeState = ParticleFadeState::None;
				this->fadeProgress = 1.0f;
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ParticleFxComponent::createParticleEffect");

		this->clonedDefName = cloneName;
		this->baseParticleTemplateName = templateName;

		return (this->particleSystem != nullptr);
	}

	void ParticleFxComponent::destroyParticleEffect(void)
	{
		if (nullptr == this->particleSystem && nullptr == this->particleNode)
		{
			return;
		}

		GraphicsModule::RenderCommand renderCommand = [this]()
		{
			try
			{
				Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

				// IMPORTANT: Restore clone defaults so next play is not stuck at emissionRate=0 after fade out
				this->restoreOriginalEmissionRates();

				if (this->particleNode && this->particleSystem && this->particleSystem->isAttached())
				{
					this->particleNode->detachObject(this->particleSystem);
				}

				if (this->particleSystem)
				{
					sceneManager->destroyParticleSystem2(this->particleSystem);
					this->particleSystem = nullptr;
				}

				if (this->particleNode)
				{
					Ogre::SceneNode* parent = this->particleNode->getParentSceneNode();
					if (parent)
					{
						parent->removeAndDestroyChild(this->particleNode);
					}
					else
					{
						sceneManager->destroySceneNode(this->particleNode);
					}
					this->particleNode = nullptr;
				}
			}
			catch (const Ogre::Exception& e)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxComponent] Exception destroying particle effect: " + e.getFullDescription());
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ParticleFxComponent::destroyParticleEffect");

		this->isEmitting = false;
		this->fadeState = ParticleFadeState::None;
		this->fadeProgress = 0.0f;
		this->originalEmissionRates.clear();
		this->originalMinVelocities.clear();
		this->originalMaxVelocities.clear();
	}

	void ParticleFxComponent::destroyEverything(void)
	{
		// First destroy instance and node
		this->destroyParticleEffect();

		// Now destroy the clone if it exists
		if (false == this->clonedDefName.empty())
		{
			GraphicsModule::RenderCommand renderCommand = [this]()
			{
				try
				{
					Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
					Ogre::ParticleSystemManager2* particleManager = sceneManager->getParticleSystemManager2();

					if (particleManager && particleManager->hasParticleSystemDef(this->clonedDefName, false))
					{
						Ogre::ParticleSystemDef* clone = particleManager->getParticleSystemDef(this->clonedDefName, false);

						// CRITICAL: Must destroy all instances first
						clone->_destroyAllParticleSystems();

						// Now destroy the def itself
						particleManager->destroyAllParticleSystems();
					}
				}
				catch (const Ogre::Exception& e)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxComponent] Exception destroying clone: " + e.getFullDescription());
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ParticleFxComponent::destroyEverything");

			this->clonedDefName.clear();
			this->baseParticleTemplateName.clear();
		}
	}

	void ParticleFxComponent::resetClone(Ogre::ParticleSystemDef* particleSystemDefInstance)
	{
		if (nullptr == particleSystemDefInstance)
		{
			return;
		}

		const Ogre::Vector2 scale = this->particleScale->getVector2();
		const Ogre::Real uniformScale = (scale.x + scale.y) * 0.5f;

		Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
		Ogre::ParticleSystemManager2* particleManager = sceneManager->getParticleSystemManager2();
		Ogre::ParticleSystemDef* baseDef = particleManager->getParticleSystemDef(this->particleTemplateName->getListSelectedValue());

		const auto& baseEmitters = baseDef->getEmitters();
		auto& cloneEmitters = particleSystemDefInstance->getEmitters();

		for (size_t i = 0; i < cloneEmitters.size() && i < baseEmitters.size(); ++i)
		{
			Ogre::EmitterDefData* cloneEmitter = cloneEmitters[i];
			const Ogre::EmitterDefData* baseEmitter = baseEmitters[i];

			Ogre::ParticleEmitter* cloneEmitterWrite = cloneEmitter->asParticleEmitter();
			const Ogre::ParticleEmitter* baseEmitterRead = baseEmitter->asParticleEmitter();

			// --- RESTORE BASE DEFAULTS FIRST (prevents "plays once" + prevents accumulating edits) ---

			cloneEmitterWrite->setAngle(baseEmitterRead->getAngle());
			cloneEmitterWrite->setDirection(baseEmitterRead->getDirection());
			cloneEmitterWrite->setUp(baseEmitterRead->getUp());

			cloneEmitterWrite->setColourRangeStart(baseEmitterRead->getColourRangeStart());
			cloneEmitterWrite->setColourRangeEnd(baseEmitterRead->getColourRangeEnd());

			cloneEmitterWrite->setMinTimeToLive(baseEmitterRead->getMinTimeToLive());
			cloneEmitterWrite->setMaxTimeToLive(baseEmitterRead->getMaxTimeToLive());

			cloneEmitterWrite->setEmissionRate(baseEmitterRead->getEmissionRate());

			cloneEmitterWrite->setDuration(baseEmitterRead->getMinDuration(), baseEmitterRead->getMaxDuration());
			cloneEmitterWrite->setRepeatDelay(baseEmitterRead->getMinRepeatDelay(), baseEmitterRead->getMaxRepeatDelay());

			// CRITICAL: setStartTime(0) DISABLES the emitter in OGRE (see ParticleEmitter::setStartTime)
			// So only apply base startTime, and ensure enabled state is correct afterwards.
			const Ogre::Real baseStartTime = baseEmitterRead->getStartTime();
			const bool baseEnabled = baseEmitterRead->getEnabled();

			if (baseStartTime > 0.0f)
			{
				cloneEmitterWrite->setStartTime(baseStartTime);
			}
			else
			{
				cloneEmitterWrite->setEnabled(baseEnabled);
			}

			cloneEmitterWrite->resetDimensions();

			Ogre::Vector2 baseDim = baseEmitter->getInitialDimensions();
			Ogre::Vector2 scaledDim(baseDim.x * scale.x, baseDim.y * scale.y);
			cloneEmitter->setInitialDimensions(scaledDim);

			Ogre::Vector3 basePos = baseEmitterRead->getPosition();
			cloneEmitterWrite->setPosition(basePos * uniformScale);

			Ogre::Real baseMinVel = baseEmitterRead->getMinParticleVelocity();
			Ogre::Real baseMaxVel = baseEmitterRead->getMaxParticleVelocity();
			cloneEmitterWrite->setParticleVelocity(baseMinVel * uniformScale, baseMaxVel * uniformScale);

			// Scale emitter box dimensions (guard: parameter may not exist for all emitter types)
			const Ogre::String baseWidthStr = baseEmitterRead->getParameter("width");
			if (false == baseWidthStr.empty())
			{
				Ogre::Real baseWidth = Ogre::StringConverter::parseReal(baseWidthStr);
				cloneEmitterWrite->setParameter("width", Ogre::StringConverter::toString(baseWidth * uniformScale));
			}

			const Ogre::String baseHeightStr = baseEmitterRead->getParameter("height");
			if (false == baseHeightStr.empty())
			{
				Ogre::Real baseHeight = Ogre::StringConverter::parseReal(baseHeightStr);
				cloneEmitterWrite->setParameter("height", Ogre::StringConverter::toString(baseHeight * uniformScale));
			}

			const Ogre::String baseDepthStr = baseEmitterRead->getParameter("depth");
			if (false == baseDepthStr.empty())
			{
				Ogre::Real baseDepth = Ogre::StringConverter::parseReal(baseDepthStr);
				cloneEmitterWrite->setParameter("depth", Ogre::StringConverter::toString(baseDepth * uniformScale));
			}

			// Final sanity: if baseStartTime == 0, ensure emitter is enabled (setStartTime(0) would have disabled it)
			if (baseStartTime <= 0.0f && true == baseEnabled)
			{
				cloneEmitterWrite->setEnabled(true);
			}
		}
	}

	void ParticleFxComponent::restoreOriginalEmissionRates(void)
	{
		if (nullptr == this->particleSystem)
		{
			return;
		}

		Ogre::ParticleSystemDef* particleSystemDef = const_cast<Ogre::ParticleSystemDef*>(this->particleSystem->getParticleSystemDef());
		if (nullptr == particleSystemDef)
		{
			return;
		}

		auto& emitters = particleSystemDef->getEmitters();

		for (size_t i = 0; i < emitters.size() && i < this->originalEmissionRates.size() && i < this->originalMinVelocities.size() && i < this->originalMaxVelocities.size(); ++i)
		{
			Ogre::ParticleEmitter* particleEmitterWrite = emitters[i]->asParticleEmitter();
			particleEmitterWrite->setEmissionRate(this->originalEmissionRates[i]);
			particleEmitterWrite->setParticleVelocity(this->originalMinVelocities[i], this->originalMaxVelocities[i]);
		}
	}

	Ogre::Real ParticleFxComponent::getMaxTtlSecondsFromDef(const Ogre::ParticleSystemDef* def) const
	{
		if (nullptr == def)
		{
			return 0.0f;
		}

		Ogre::Real maxTtl = 0.0f;

		const auto& emitters = def->getEmitters();
		for (const Ogre::EmitterDefData* emitters : emitters)
		{
			const Ogre::ParticleEmitter* emitter = emitters->asParticleEmitter();
			if (emitter)
			{
				maxTtl = std::max(maxTtl, emitter->getMaxTimeToLive());
			}
		}

		return maxTtl;
	}

	/*
	connect()
	  ├─> particlePlayTime = initial
	  ├─> fadeState = None
	  └─> if activated:
		  └─> startParticleEffect()
			  ├─> destroyParticleEffect() (cleanup old)
			  ├─> createParticleEffect()
			  │   ├─> cloneName = "ParticleFX_NOWA_" + gameObjectID
			  │   ├─> if clone exists:
			  │   │   ├─> Reuse it
			  │   │   └─> resetClone()
			  │   └─> else:
			  │       ├─> baseDef->clone(cloneName)
			  │       └─> resetClone()
			  │   
			  │   ├─> Initialize clone if needed
			  │   ├─> createParticleSystem2(cloneName)
			  │   ├─> Create node
			  │   ├─> Attach instance to node
			  │   └─> Store: clonedDefName, baseTemplateName
			  │   
			  └─> isEmitting = true
	*/
	bool ParticleFxComponent::connect(void)
	{
		GameObjectComponent::connect();

		this->oldActivated = this->activated->getBool();

		this->particlePlayTime = this->particleInitialPlayTime->getReal();

		this->pendingDrainTimeMs = 0.0f;
		this->pendingRestartAfterDrain = false;

		this->fadeState = ParticleFadeState::None;
		this->fadeProgress = 0.0f;

		if (true == this->activated->getBool())
		{
			this->startParticleEffect();
		}

		return true;
	}

	/*
		disconnect()
		  ├─> destroyParticleEffect()
		  │   ├─> Detach instance
		  │   ├─> destroyParticleSystem2()
		  │   ├─> Destroy node
		  │   └─> isEmitting = false
		  ├─> activated = oldActivated
		  └─> particlePlayTime = initial
	*/
	bool ParticleFxComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		// HARD RESET: destroy instance + node + clone def
		this->destroyEverything();

		// Restore editor state
		this->activated->setValue(this->oldActivated);

		// Reset runtime state
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
		this->pendingDrainTimeMs = 0.0f;
		this->pendingRestartAfterDrain = false;
		this->fadeState = ParticleFadeState::None;
		this->fadeProgress = 0.0f;
		this->isEmitting = false;

		return true;
	}

	bool ParticleFxComponent::onCloned(void)
	{
		return true;
	}

	/*
		onRemoveComponent()
		  └─> destroyEverything()
			  ├─> destroyParticleEffect()
			  └─> Destroy clone (see above)
	*/
	void ParticleFxComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Destructor particle fx component for game object: " + this->gameObjectPtr->getName());

		// Destroy EVERYTHING including the clone
		this->destroyEverything();
	}

	void ParticleFxComponent::onOtherComponentRemoved(unsigned int index)
	{
		// Nothing special to do
	}

	void ParticleFxComponent::onOtherComponentAdded(unsigned int index)
	{
		// Nothing special to do
	}

	/*
	 update(dt)
	  ├─> Update camera position
	  ├─> Handle fade states
	  └─> if activated && playTime > 0:
		  ├─> particlePlayTime -= dt * 1000 * playSpeed
		  └─> if time expired:
			  ├─> if repeat:
			  │   ├─> particlePlayTime = initial
			  │   ├─> destroyParticleEffect()
			  │   └─> createParticleEffect()
			  └─> else:
				  └─> Stop (with optional fade out)
	*/
	void ParticleFxComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (notSimulating || nullptr == this->particleSystem)
		{
			return;
		}

		Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
		Ogre::ParticleSystemManager2* particleManager = sceneManager->getParticleSystemManager2();
		if (particleManager)
		{
			Ogre::Camera* camera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();

			if (nullptr != camera)
			{
				auto closureFunction = [particleManager, camera](Ogre::Real renderDt)
					{
						particleManager->setCameraPosition(camera->getDerivedPosition());
					};
				Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
				NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
			}
		}

		// --- Handle fade states ---
		if (this->fadeState == ParticleFadeState::FadingIn)
		{
			this->updateFadeIn(dt);
		}
		else if (this->fadeState == ParticleFadeState::FadingOut)
		{
			this->updateFadeOut(dt);
			// Do NOT return here if drain gate is handled in update()
			// return;
		}

		// Drain gate (prevents leftovers showing on restart)
		if (this->pendingDrainTimeMs > 0.0f)
		{
			this->pendingDrainTimeMs -= dt * 1000.0f;
			if (this->pendingDrainTimeMs > 0.0f)
			{
				return;
			}

			this->pendingDrainTimeMs = 0.0f;

			if (this->pendingRestartAfterDrain)
			{
				this->pendingRestartAfterDrain = false;

				this->destroyParticleEffect();
				this->createParticleEffect();
				this->particlePlayTime = this->particleInitialPlayTime->getReal();
				this->isEmitting = true;
			}
			else
			{
				this->stopParticleEffect();
				this->isEmitting = false;
				this->activated->setValue(false);
			}
			return;
		}

		// Particle lifetime control
		if (false == this->activated->getBool())
		{
			return;
		}

		const Ogre::Real initialPlayTime = this->particleInitialPlayTime->getReal();

		// Infinite playback
		if (initialPlayTime <= 0.0f)
		{
			return;
		}

		this->particlePlayTime -= dt * 1000.0f * this->particlePlaySpeed->getReal();

		if (this->particlePlayTime > 0.0f)
		{
			return;
		}

		// Time expired
		if (this->repeat->getBool())
		{
			this->particlePlayTime = initialPlayTime;

			if (this->fadeOut->getBool())
			{
				this->pendingRestartAfterDrain = true;
				this->beginFadeOut();
				return;
			}

			this->destroyParticleEffect();
			this->createParticleEffect();
			return;
		}
		else
		{
			if (this->fadeOut->getBool() && this->fadeState != ParticleFadeState::FadingOut)
			{
				this->pendingRestartAfterDrain = false;
				this->beginFadeOut();
			}
			else if (!this->fadeOut->getBool())
			{
				this->stopParticleEffect();
				this->isEmitting = false;
				this->activated->setValue(false);
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
			this->setParticleScale(attribute->getVector2());
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
		else if (ParticleFxComponent::AttrFadeIn() == attribute->getName())
		{
			this->setFadeIn(attribute->getBool());
		}
		else if (ParticleFxComponent::AttrFadeInTime() == attribute->getName())
		{
			this->setFadeInTimeMS(attribute->getReal());
		}
		else if (ParticleFxComponent::AttrFadeOut() == attribute->getName())
		{
			this->setFadeOut(attribute->getBool());
		}
		else if (ParticleFxComponent::AttrFadeOutTime() == attribute->getName())
		{
			this->setFadeOutTimeMS(attribute->getReal());
		}
	}

	void ParticleFxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Scale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->particleScale->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlendingMethod"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blendingMethod->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeIn"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeIn->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeInTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeInTime->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeOut"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeOut->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeOutTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeOutTime->getReal())));
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

		if (false == this->bConnected)
		{
			this->particlePlayTime = this->particleInitialPlayTime->getReal();
			return;
		}

		if (false == activated)
		{
			this->pendingRestartAfterDrain = false;

			if (this->fadeOut->getBool() && this->fadeState != ParticleFadeState::FadingOut && nullptr != this->particleSystem)
			{
				this->beginFadeOut();
			}
			else
			{
				this->stopParticleEffect();
				this->isEmitting = false;
			}

			this->particlePlayTime = this->particleInitialPlayTime->getReal();
			return;
		}

		// Activating: cancel fade/drain and start clean
		this->pendingDrainTimeMs = 0.0f;
		this->pendingRestartAfterDrain = false;
		this->fadeState = ParticleFadeState::None;
		this->fadeProgress = 1.0f;

		this->startParticleEffect();

		this->particlePlayTime = this->particleInitialPlayTime->getReal();
	}

	bool ParticleFxComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	/*
		setParticleTemplateName(newTemplate)
		  ├─> if template != current:
		  │   ├─> Store: wasPlaying
		  │   ├─> destroyEverything() <- Destroys clone!
		  │   │   ├─> destroyParticleEffect()
		  │   │   └─> if clone exists:
		  │   │       ├─> clone->_destroyAllParticleSystems()
		  │   │       └─> manager->destroyAllParticleSystems()
		  │   ├─> createParticleEffect() <- Creates new clone
		  │   └─> isEmitting = wasPlaying
		  └─> else: do nothing
	*/
	void ParticleFxComponent::setParticleTemplateName(const Ogre::String& particleTemplateName)
	{
		this->particleTemplateName->setListSelectedValue(particleTemplateName);

		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		if (false == this->bConnected)
		{
			this->baseParticleTemplateName = particleTemplateName;
			return;
		}

		if (false == this->baseParticleTemplateName.empty() && this->baseParticleTemplateName != particleTemplateName)
		{
			const bool wasPlaying = this->activated->getBool();

			this->destroyEverything();
			this->createParticleEffect();

			if (true == wasPlaying)
			{
				this->startParticleEffect();
			}
		}
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
		this->applyVelocitySpeedFactor(playSpeed);
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

	/*
		setParticleScale(newScale)
		  ├─> Store: wasPlaying, remainingTime
		  ├─> destroyParticleEffect() (instance only)
		  ├─> createParticleEffect()
		  │   └─> resetClone() <- Modifies existing clone
		  ├─> isEmitting = wasPlaying
		  └─> particlePlayTime = remainingTime
	*/
	void ParticleFxComponent::setParticleScale(const Ogre::Vector2& scale)
	{
		this->particleScale->setValue(scale);

		if (nullptr == this->particleSystem)
		{
			return;
		}

		// Store state to preserve it
		bool wasPlaying = this->isEmitting && this->activated->getBool();
		Ogre::Real remainingTime = this->particlePlayTime;

		// Destroy instance (but not clone)
		this->destroyParticleEffect();

		// Recreate with new scale (reuses existing clone via resetClone)
		this->createParticleEffect();

		// Restore state
		if (true == wasPlaying)
		{
			this->isEmitting = true;
		}
		this->particlePlayTime = remainingTime;
	}

	Ogre::Vector2 ParticleFxComponent::getParticleScale(void) const
	{
		return this->particleScale->getVector2();
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
		const Ogre::ParticleSystemDef* particleSystemDef = this->getParticleSystemDef();
		if (nullptr != particleSystemDef)
		{
			return particleSystemDef->getNumSimdActiveParticles();
		}
		return 0;
	}

	Ogre::uint32 ParticleFxComponent::getParticleQuota(void) const
	{
		const Ogre::ParticleSystemDef* particleSystemDef = this->getParticleSystemDef();
		if (nullptr != particleSystemDef)
		{
			return particleSystemDef->getQuota();
		}
		return 0;
	}

	size_t ParticleFxComponent::getNumEmitters(void) const
	{
		const Ogre::ParticleSystemDef* particleSystemDef = this->getParticleSystemDef();
		if (nullptr != particleSystemDef)
		{
			return particleSystemDef->getNumEmitters();
		}
		return 0;
	}

	size_t ParticleFxComponent::getNumAffectors(void) const
	{
		const Ogre::ParticleSystemDef* particleSystemDef = this->getParticleSystemDef();
		if (nullptr != particleSystemDef)
		{
			return particleSystemDef->getNumAffectors();
		}
		return 0;
	}

	void ParticleFxComponent::startParticleEffect(void)
	{
		// Destroy old instance
		this->destroyParticleEffect();

		// Create new instance (reuses existing clone)
		this->createParticleEffect();

		this->particlePlayTime = this->particleInitialPlayTime->getReal();
		this->isEmitting = true;
	}

	void ParticleFxComponent::stopParticleEffect(void)
	{
		this->destroyParticleEffect();
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
		{
			return ParticleBlendingMethod::AlphaHashing;
		}
		else if (selectedValue == "Alpha Hashing + A2C")
		{
			return ParticleBlendingMethod::AlphaHashingA2C;
		}
		else
		{
			return ParticleBlendingMethod::AlphaBlending;
		}
	}

	void ParticleFxComponent::setFadeIn(bool fadeIn)
	{
		this->fadeIn->setValue(fadeIn);
	}

	bool ParticleFxComponent::getFadeIn(void) const
	{
		return this->fadeIn->getBool();
	}

	void ParticleFxComponent::setFadeInTimeMS(Ogre::Real fadeInTimeMS)
	{
		this->fadeInTime->setValue(fadeInTimeMS);
	}

	Ogre::Real ParticleFxComponent::getFadeInTimeMS(void) const
	{
		return this->fadeInTime->getReal();
	}

	void ParticleFxComponent::setFadeOut(bool fadeOut)
	{
		this->fadeOut->setValue(fadeOut);
	}

	bool ParticleFxComponent::getFadeOut(void) const
	{
		return this->fadeOut->getBool();
	}

	void ParticleFxComponent::setFadeOutTimeMS(Ogre::Real fadeOutTimeMS)
	{
		this->fadeOutTime->setValue(fadeOutTimeMS);
	}

	Ogre::Real ParticleFxComponent::getFadeOutTimeMS(void) const
	{
		return this->fadeOutTime->getReal();
	}

	ParticleFadeState::ParticleFadeState ParticleFxComponent::getFadeState(void) const
	{
		return this->fadeState;
	}

	void ParticleFxComponent::storeOriginalEmissionRates(void)
	{
		this->originalEmissionRates.clear();
		this->originalMinVelocities.clear();
		this->originalMaxVelocities.clear();

		if (nullptr == this->particleSystem)
			return;

		const Ogre::ParticleSystemDef* particleSystemDef = this->particleSystem->getParticleSystemDef();
		if (nullptr == particleSystemDef)
		{
			return;
		}

		const auto& emitters = particleSystemDef->getEmitters();
		this->originalEmissionRates.reserve(emitters.size());
		this->originalMinVelocities.reserve(emitters.size());
		this->originalMaxVelocities.reserve(emitters.size());

		for (const Ogre::EmitterDefData* emitter : emitters)
		{
			const Ogre::ParticleEmitter* particleEmitterRead = emitter->asParticleEmitter();
			this->originalEmissionRates.push_back(particleEmitterRead->getEmissionRate());
			this->originalMinVelocities.push_back(particleEmitterRead->getMinParticleVelocity());
			this->originalMaxVelocities.push_back(particleEmitterRead->getMaxParticleVelocity());
		}
	}

	void ParticleFxComponent::setEmissionRateFactor(Ogre::Real factor)
	{
		if (nullptr == this->particleSystem)
		{
			return;
		}

		Ogre::ParticleSystemDef* particleSystemDef = const_cast<Ogre::ParticleSystemDef*>(this->particleSystem->getParticleSystemDef());
		if (nullptr == particleSystemDef)
		{
			return;
		}

		auto& emitters = particleSystemDef->getEmitters();

		for (size_t i = 0; i < emitters.size() && i < this->originalEmissionRates.size(); ++i)
		{
			Ogre::ParticleEmitter* particleEmitterWrite = emitters[i]->asParticleEmitter();
			Ogre::Real newRate = this->originalEmissionRates[i] * factor;
			particleEmitterWrite->setEmissionRate(newRate);
		}
	}

	void ParticleFxComponent::applyVelocitySpeedFactor(Ogre::Real speedFactor)
	{
		if (nullptr == this->particleSystem)
		{
			return;
		}

		Ogre::ParticleSystemDef* particleSystemDef = const_cast<Ogre::ParticleSystemDef*>(this->particleSystem->getParticleSystemDef());
		if (nullptr == particleSystemDef)
		{
			return;
		}

		auto& emitters = particleSystemDef->getEmitters();

		for (size_t i = 0; i < emitters.size() && i < this->originalMinVelocities.size(); ++i)
		{
			Ogre::ParticleEmitter* particleEmitterWrite = emitters[i]->asParticleEmitter();
			Ogre::Real newMinVel = this->originalMinVelocities[i] * speedFactor;
			Ogre::Real newMaxVel = this->originalMaxVelocities[i] * speedFactor;
			particleEmitterWrite->setParticleVelocity(newMinVel, newMaxVel);
		}
	}

	void ParticleFxComponent::updateFadeIn(Ogre::Real dt)
	{
		if (this->fadeState != ParticleFadeState::FadingIn)
		{
			return;
		}

		const Ogre::Real fadeInTimeSeconds = this->fadeInTime->getReal() / 1000.0f;

		if (fadeInTimeSeconds <= 0.0f)
		{
			// Instant fade in
			this->fadeProgress = 1.0f;
			this->fadeState = ParticleFadeState::None;
			this->setEmissionRateFactor(1.0f);
			return;
		}

		// Advance fade progress
		this->fadeProgress += (dt * this->particlePlaySpeed->getReal()) / fadeInTimeSeconds;

		if (this->fadeProgress >= 1.0f)
		{
			this->fadeProgress = 1.0f;
			this->fadeState = ParticleFadeState::None;
		}

		// Apply smooth easing (ease-in-out)
		Ogre::Real easedProgress = this->fadeProgress * this->fadeProgress * (3.0f - 2.0f * this->fadeProgress);

		this->setEmissionRateFactor(easedProgress);
	}

	void ParticleFxComponent::updateFadeOut(Ogre::Real dt)
	{
		const Ogre::Real fadeOutMs = this->fadeOutTime->getReal(); // whatever you use
		if (fadeOutMs <= 0.0f)
		{
			this->fadeProgress = 0.0f;
			this->setEmissionRateFactor(0.0f);
		}
		else
		{
			this->fadeProgress -= (dt * 1000.0f) / fadeOutMs;
			if (this->fadeProgress < 0.0f)
				this->fadeProgress = 0.0f;

			this->setEmissionRateFactor(this->fadeProgress);
		}

		// Once fully faded, start drain exactly once
		if (this->fadeProgress <= 0.0f && this->pendingDrainTimeMs <= 0.0f)
		{
			const Ogre::ParticleSystemDef* def = nullptr;
			if (this->particleSystem)
				def = this->particleSystem->getParticleSystemDef();

			const Ogre::Real maxTtlSec = this->getMaxTtlSecondsFromDef(def);
			this->pendingDrainTimeMs = (maxTtlSec * 1000.0f) + 50.0f;
		}
	}

	void ParticleFxComponent::beginFadeOut(void)
	{
		if (this->fadeState == ParticleFadeState::FadingOut)
		{
			return;
		}

		this->fadeState = ParticleFadeState::FadingOut;

		if (this->fadeProgress <= 0.0f)
		{
			this->fadeProgress = 1.0f;
		}

		// Drain will be armed by updateFadeOut once fadeProgress reaches 0
		this->pendingDrainTimeMs = 0.0f;
	}

	void ParticleFxComponent::applyBlendingMethod(void)
	{
		if (nullptr == this->particleSystem)
		{
			return;
		}

		Ogre::ParticleSystem2* particleSystem2 = this->particleSystem;

		auto particleSystemDef = particleSystem2->getParticleSystemDef();
		if (nullptr == particleSystemDef)
		{
			return;
		}

		const Ogre::String& materialName = particleSystemDef->getMaterialName();
		if (true == materialName.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxComponent] Particle system has no material assigned.");
			return;
		}

		Ogre::HlmsManager* hlmsManager = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager();

		Ogre::HlmsDatablock* datablock = hlmsManager->getDatablock(materialName);

		if (nullptr == datablock)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxComponent] Could not find datablock for particle material: " + materialName);
			return;
		}

		const ParticleBlendingMethod::ParticleBlendingMethod currentMethod = this->getBlendingMethod();

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::applyBlendingMethod", _2(datablock, currentMethod),
			{
				if (currentMethod == ParticleBlendingMethod::AlphaHashing || currentMethod == ParticleBlendingMethod::AlphaHashingA2C)
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
				else // Alpha blending
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
					.def("setFadeIn", &ParticleFxComponent::setFadeIn)
					.def("getFadeIn", &ParticleFxComponent::getFadeIn)
					.def("setFadeInTimeMS", &ParticleFxComponent::setFadeInTimeMS)
					.def("getFadeInTimeMS", &ParticleFxComponent::getFadeInTimeMS)
					.def("setFadeOut", &ParticleFxComponent::setFadeOut)
					.def("getFadeOut", &ParticleFxComponent::getFadeOut)
					.def("setFadeOutTimeMS", &ParticleFxComponent::setFadeOutTimeMS)
					.def("getFadeOutTimeMS", &ParticleFxComponent::getFadeOutTimeMS)
					.def("getFadeState", &ParticleFxComponent::getFadeState)

					.enum_("ParticleBlendingMethod")
					[
						luabind::value("AlphaHashing", ParticleBlendingMethod::AlphaHashing),
						luabind::value("AlphaHashingA2C", ParticleBlendingMethod::AlphaHashingA2C),
						luabind::value("AlphaBlending", ParticleBlendingMethod::AlphaBlending)
					]

					.enum_("ParticleFadeState")
					[
						luabind::value("None", ParticleFadeState::None),
						luabind::value("FadingIn", ParticleFadeState::FadingIn),
						luabind::value("FadingOut", ParticleFadeState::FadingOut)
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
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setParticleScale(Vector2 scale)", "Sets the particle scale.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "Vector2 getParticleScale()", "Gets the particle scale.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool isPlaying()", "Checks if the particle effect is currently playing/emitting.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setGlobalPosition(Vector3 position)", "Sets the global world position for the particle effect.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setGlobalOrientation(Vector3 orientation)", "Sets the global orientation in degrees.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumActiveParticles()", "Gets the current number of active particles.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getParticleQuota()", "Gets the particle quota (maximum particles allowed).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumEmitters()", "Gets the number of emitters in the particle system.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getNumAffectors()", "Gets the number of affectors in the particle system.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setBlendingMethod(ParticleBlendingMethod method)", "Sets the blending method (AlphaHashing=0, AlphaHashingA2C=1, AlphaBlending=2).");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "ParticleBlendingMethod getBlendingMethod()", "Gets the current blending method.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setFadeIn(bool fadeIn)", "Sets whether the particle effect should fade in when starting.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool getFadeIn()", "Gets whether the particle effect fades in when starting.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setFadeInTimeMS(number fadeInTime)", "Sets the fade in duration in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getFadeInTimeMS()", "Gets the fade in duration in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setFadeOut(bool fadeOut)", "Sets whether the particle effect should fade out when stopping.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "bool getFadeOut()", "Gets whether the particle effect fades out when stopping.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "void setFadeOutTimeMS(number fadeOutTime)", "Sets the fade out duration in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "number getFadeOutTimeMS()", "Gets the fade out duration in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("ParticleFxComponent", "ParticleFadeState getFadeState()", "Gets the current fade state (None=0, FadingIn=1, FadingOut=2).");

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