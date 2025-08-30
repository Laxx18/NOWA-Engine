#include "NOWAPrecompiled.h"
#include "GpuParticlesComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "GpuParticles/GpuParticleSystemWorld.h"
#include "GpuParticles/GpuParticleEmitter.h"
#include "GpuParticles/Affectors/GpuParticleDepthCollisionAffector.h"
#include "GpuParticles/Affectors/GpuParticleGlobalGravityAffector.h"
#include "GpuParticles/Affectors/GpuParticleSetColourTrackAffector.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	GpuParticlesComponent::GpuParticlesComponent()
		: GameObjectComponent(),
		name("GpuParticlesComponent"),
		gpuParticleSystem(nullptr),
		gpuParticleSystemWorld(nullptr),
		particlePlayTime(10000.0f),
		oldActivated(true),
		particleInstanceId(0),
		globalPosition(Ogre::Math::POS_INFINITY),
		globalOrientation(Ogre::Quaternion(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		activated(new Variant(GpuParticlesComponent::AttrActivated(), true, this->attributes)),
		particleTemplateName(new Variant(GpuParticlesComponent::AttrParticleName(), std::vector<Ogre::String>(), this->attributes)),
		repeat(new Variant(GpuParticlesComponent::AttrRepeat(), false, this->attributes)),
		particleInitialPlayTime(new Variant(GpuParticlesComponent::AttrPlayTime(), 0.0f, this->attributes)),
		particlePlaySpeed(new Variant(GpuParticlesComponent::AttrPlaySpeed(), 1.0f, this->attributes)),
		particleOffsetPosition(new Variant(GpuParticlesComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		particleOffsetOrientation(new Variant(GpuParticlesComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		particleScale(new Variant(GpuParticlesComponent::AttrScale(), Ogre::Vector3::UNIT_SCALE, this->attributes))
	{
		// Activated variable is set to false again, when particle has played and repeat is off, so tell it the gui that it must be refreshed
		this->activated->addUserData(GameObject::AttrActionNeedRefresh());
		this->particleInitialPlayTime->setDescription("The particle play time in milliseconds, if set to 0, the particle effect will not stop automatically.");
	}

	GpuParticlesComponent::~GpuParticlesComponent(void)
	{
		
	}

	const Ogre::String& GpuParticlesComponent::getName() const
	{
		return this->name;
	}

	void GpuParticlesComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<GpuParticlesComponent>(GpuParticlesComponent::getStaticClassId(), GpuParticlesComponent::getStaticClassName());
	}
	
	void GpuParticlesComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool GpuParticlesComponent::init(rapidxml::xml_node<>*& propertyElement)
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

	GameObjectCompPtr GpuParticlesComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		GpuParticlesComponentPtr clonedCompPtr(boost::make_shared<GpuParticlesComponent>());

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

	bool GpuParticlesComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GpuParticlesComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// Get all available particle effects
		const auto particleSystems = NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->getGpuParticleSystemResourceManager()->getGpuParticleSystems();
		std::vector<Ogre::String> strAvailableParticleTemplateNames(particleSystems.size());

		unsigned short i = 0;
		for (auto& it = particleSystems.begin(); it != particleSystems.end(); ++it)
		{
			strAvailableParticleTemplateNames[i++] = it->first.getFriendlyText();
		}

		if (false == strAvailableParticleTemplateNames.empty())
		{
			this->particleTemplateName->setValue(strAvailableParticleTemplateNames);
			this->particleTemplateName->setListSelectedValue(strAvailableParticleTemplateNames[0]);
		}

		if (true == this->particleTemplateName->getString().empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GpuParticlesComponent] Error: Can not use particle effects, because the resources are not loaded!");
			return true;
		}

		// Particle effect is moving, so it must always be dynamic, else it will not be rendered!
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createParticleEffect();

		return true;
	}

	bool GpuParticlesComponent::createParticleEffect(void)
	{
		if (nullptr == this->gameObjectPtr)
			return true;

		this->gpuParticleSystemWorld = NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->getGpuParticleSystemWorld();

		if (this->oldParticleTemplateName != this->particleTemplateName->getListSelectedValue())
		{
			this->destroyParticleEffect();
			this->oldParticleTemplateName = this->particleTemplateName->getListSelectedValue();
		}

		Ogre::String name = this->particleTemplateName->getListSelectedValue() + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

		this->gpuParticleSystem = const_cast<GpuParticleSystem*>(NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->getGpuParticleSystemResourceManager()->getGpuParticleSystem(this->particleTemplateName->getListSelectedValue()));

		if (nullptr == this->gpuParticleSystem)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GpuParticlesComponent] Error: Could not create particle effect: " + this->particleTemplateName->getListSelectedValue());
			return false;
		}

		/*if (nullptr == this->gpuParticleSystem)
		{
			this->gpuParticleSystem = const_cast<GpuParticleSystem*>(NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->getGpuParticleSystemResourceManager()->createParticleSystem(
				this->particleTemplateName->getListSelectedValue(), this->particleTemplateName->getListSelectedValue(), name, "GpuParticles"));
			if (nullptr == this->gpuParticleSystem)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GpuParticlesComponent] Error: Could not create particle effect: " + this->particleTemplateName->getListSelectedValue());
				return false;
			}
		}*/

		// this->gpuParticleSystemWorld->registerEmitterCore(this->gpuParticleSystem);

		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-->create: " + name);

		// Get entry
		const auto particleSystems = NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->getGpuParticleSystemResourceManager()->getGpuParticleSystems();
		const auto found = particleSystems.find(this->particleTemplateName->getListSelectedValue());
		if (found != particleSystems.cend())
		{
			// Tag resource for deployment
			DeployResourceModule::getInstance()->tagResource(this->particleTemplateName->getListSelectedValue(), found->second.srcResourceGroup);
		}

#if 0
		// Cool! how parentNode for start does work
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

#if 0
		this->gpuParticleSystem = NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->getGpuParticleSystemResourceManager()->createParticleSystem("FireManual", "FireManual");

		GpuParticleEmitter* emitterCore = new GpuParticleEmitter();

		//            emitterCore->mDatablockName = "ParticleAlphaBlend";
		emitterCore->mDatablockName = "ParticleAdditive";
		emitterCore->mSpriteMode = GpuParticleEmitter::SpriteMode::SetWithStart;
		emitterCore->mSpriteFlipbookCoords.push_back(HlmsParticleDatablock::SpriteCoord(0, 0));
		emitterCore->mSpriteFlipbookCoords.push_back(HlmsParticleDatablock::SpriteCoord(0, 1));
		emitterCore->mSpriteFlipbookCoords.push_back(HlmsParticleDatablock::SpriteCoord(1, 0));
		emitterCore->mSpriteFlipbookCoords.push_back(HlmsParticleDatablock::SpriteCoord(1, 1));
		emitterCore->mSpriteTimes.push_back(0.0f);
		emitterCore->mSpriteTimes.push_back(0.15f);
		emitterCore->mSpriteTimes.push_back(0.30f);
		emitterCore->mSpriteTimes.push_back(0.45f);
		//            emitterCore->mEmissionRate = 30.0f;
		emitterCore->mEmissionRate = 60.0f;
		//            emitterCore->mParticleLifetime = Geometry::Range(0.8f, 1.0f);
		//            emitterCore->mParticleLifetime = Geometry::Range(0.5f, 0.7f);
		emitterCore->mParticleLifetimeMin = 0.8f;
		emitterCore->mParticleLifetimeMax = 1.0f;
		emitterCore->mColourA = Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f);
		emitterCore->mColourB = Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f);
		emitterCore->mUniformSize = true;
		emitterCore->mSizeMin = 0.2f;
		emitterCore->mSizeMax = 0.5f;
		emitterCore->mDirectionVelocityMin = 0.9f;
		emitterCore->mDirectionVelocityMax = 1.2f;
		//            emitterCore->mDirectionVelocity = Geometry::Range(1.6f, 1.8f);
		emitterCore->mSpotAngleMin = 0.0f;
		emitterCore->mSpotAngleMax = Ogre::Math::PI / 36.0f;

		{
			GpuParticleSetColourTrackAffector* setColourTrackAffector = OGRE_NEW GpuParticleSetColourTrackAffector();
			setColourTrackAffector->mEnabled = true;

			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.0f, Ogre::Vector3(1.000f, 1.000f, 1.000f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(1.0f, Ogre::Vector3(1.000f, 1.000f, 1.000f)));

			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.0f, Ogre::Vector3(1.000f, 0.878f, 0.000f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.3f, Ogre::Vector3(1.000f, 0.486f, 0.000f)));
			//// //            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.3f, Ogre::Vector3(1.000f, 0.486f, 0.184f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(1.0f, Ogre::Vector3(0.192f, 0.020f, 0.000f)));

			setColourTrackAffector->mColourTrack.insert(std::make_pair(0.0f, Ogre::Vector3(0.750f, 0.659f, 0.000f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.3f, Ogre::Vector3(0.750f, 0.365f, 0.184f)));
			setColourTrackAffector->mColourTrack.insert(std::make_pair(0.2f, Ogre::Vector3(0.750f, 0.365f, 0.000f)));
			setColourTrackAffector->mColourTrack.insert(std::make_pair(0.7f, Ogre::Vector3(0.144f, 0.015f, 0.000f)));

			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.0f, Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(1.0f, Ogre::Vector3(1.0f, 0.0f, 0.0f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(2.0f, Ogre::Vector3(1.0f, 1.0f, 0.0f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(3.0f, Ogre::Vector3(0.0f, 1.0f, 0.0f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(4.0f, Ogre::Vector3(0.0f, 1.0f, 1.0f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(5.0f, Ogre::Vector3(0.0f, 0.0f, 1.0f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(6.0f, Ogre::Vector3(1.0f, 0.0f, 1.0f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(7.0f, Ogre::Vector3(1.0f, 1.0f, 1.0f)));
			//            setColourTrackAffector->mColourTrack.insert(std::make_pair(8.0f, Ogre::Vector3(1.0f, 0.0f, 0.0f)));

			emitterCore->addAffector(setColourTrackAffector);
		}


		emitterCore->mFaderMode = GpuParticleEmitter::FaderMode::Enabled;
		//            emitterCore->mFaderMode = GpuParticleEmitter::FaderMode::AlphaOnly;
		emitterCore->mParticleFaderStartTime = 0.1f;
		emitterCore->mParticleFaderEndTime = 0.2f;

		this->gpuParticleSystem->addEmitter(emitterCore);

		this->gpuParticleSystemWorld->registerEmitterCore(this->gpuParticleSystem);
#endif

		return true;
	}

	void GpuParticlesComponent::startParticleEffect(void)
	{
		if (nullptr != this->gpuParticleSystem)
		{
			this->gpuParticleSystemWorld->registerEmitterCore(this->gpuParticleSystem);

			if (this->globalPosition != Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY) || this->globalOrientation != Ogre::Quaternion(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY))
			{
				// Note: Without parent node, pos and orientation is global
				this->particleInstanceId = this->gpuParticleSystemWorld->start(this->gpuParticleSystem, nullptr, this->globalPosition, this->particleScale->getVector3(), this->globalOrientation);
				// TODO: Add particle instance id to list, or global gpu particle module list?
			}
			else
			{
				this->particleInstanceId = this->gpuParticleSystemWorld->start(this->gpuParticleSystem, this->gameObjectPtr->getSceneNode(), this->particleOffsetPosition->getVector3(), this->particleScale->getVector3(),
																			   MathHelper::getInstance()->degreesToQuat(this->particleOffsetOrientation->getVector3()));

				// Strange effects
				/*auto emmiters = this->gpuParticleSystem->getEmitters();
				for (size_t i = 0; i < emmiters.size(); i++)
				{
					emmiters[i]->setLooped(this->particlePlaySpeed->getReal());
				}*/
			}
		}
	}

	void GpuParticlesComponent::destroyParticleEffect(void)
	{
		// if (0 != this->particleInstanceId)
		if (nullptr != this->gpuParticleSystem)
		{
			// true, true, do not change, else a crash occurs, if particle effect is re-started
			this->gpuParticleSystemWorld->stop(this->particleInstanceId, true, true);
			// TODO: Is this correct, how about caching etc., just unregister if particle changes...
			// Must be called twice, else if just stopping the simulation, the particle effect just freezes
			this->gpuParticleSystemWorld->unregisterEmitterCore(this->gpuParticleSystem);
			this->gpuParticleSystemWorld->unregisterEmitterCore(this->gpuParticleSystem);

			// NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->getGpuParticleSystemResourceManager()->destroyParticleSystem(this->gpuParticleSystem);
			// this->gpuParticleSystem = nullptr;

			this->particleInstanceId = 0;
		}
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
	}

	bool GpuParticlesComponent::connect(void)
	{
		GameObjectComponent::connect();

		bool success = this->createParticleEffect();
		if (true == success)
		{
			this->oldActivated = this->activated->getBool();
			if (true == this->activated->getBool())
			{
				// Just add the offset because the node is a child of the game object scene node and therefore relative to the parent position

				this->startParticleEffect();

				this->particlePlayTime = this->particleInitialPlayTime->getReal();
			}
		}
		return success;
	}

	bool GpuParticlesComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (0 != this->particleInstanceId)
		{
			// this->gpuParticleSystemWorld->stopAndRemoveAllImmediately();
			// TODO: Play with those parameters
			// this->gpuParticleSystemWorld->stop(this->particleInstanceId, true, false);
			this->destroyParticleEffect();
		}
		this->activated->setValue(this->oldActivated);
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
		return true;
	}

	bool GpuParticlesComponent::onCloned(void)
	{
		
		return true;
	}

	void GpuParticlesComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
		this->destroyParticleEffect();
	}
	
	void GpuParticlesComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void GpuParticlesComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void GpuParticlesComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && 0 != this->particleInstanceId)
		{
			// Only play activated particle effect
			if (true == this->activated->getBool())
			{
				this->gpuParticleSystemWorld->processTime(dt);

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
					// Set activated to false, so that the particle can be activated at a later time
					if (false == this->repeat->getBool())
					{
						// true, true, do not change, else a crash occurs, if particle effect is re-started
						this->gpuParticleSystemWorld->stop(this->particleInstanceId, true, true);
						this->activated->setValue(false);
					}
					else
					{
						this->startParticleEffect();
						this->particlePlayTime = this->particleInitialPlayTime->getReal();
					}
				}

				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "#######play time: "
				// 												+ Ogre::StringConverter::toString(this->particlePlayTime) + " for: " + this->gameObjectPtr->getName());
			}
		}
	}

	void GpuParticlesComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (GpuParticlesComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (GpuParticlesComponent::AttrParticleName() == attribute->getName())
		{
			this->setParticleTemplateName(attribute->getListSelectedValue());
		}
		else if (GpuParticlesComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (GpuParticlesComponent::AttrPlayTime() == attribute->getName())
		{
			this->setParticlePlayTimeMS(attribute->getReal());
		}
		else if (GpuParticlesComponent::AttrPlaySpeed() == attribute->getName())
		{
			this->setParticlePlaySpeed(attribute->getReal());
		}
		else if (GpuParticlesComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setParticleOffsetPosition(attribute->getVector3());
		}
		else if (GpuParticlesComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setParticleOffsetOrientation(attribute->getVector3());
		}
		else if (GpuParticlesComponent::AttrScale() == attribute->getName())
		{
			this->setParticleScale(attribute->getVector3());
		}
	}

	void GpuParticlesComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

	Ogre::String GpuParticlesComponent::getClassName(void) const
	{
		return "GpuParticlesComponent";
	}

	Ogre::String GpuParticlesComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void GpuParticlesComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (0 != this->particleInstanceId && true == this->bConnected)
		{
			if (false == activated)
			{
				this->gpuParticleSystemWorld->stop(this->particleInstanceId, true, false);
			}
			else
			{
				this->startParticleEffect();
			}
		}
		this->particlePlayTime = this->particleInitialPlayTime->getReal();
	}

	bool GpuParticlesComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void GpuParticlesComponent::setParticleTemplateName(const Ogre::String& particleTemplateName)
	{
		// this->oldParticleTemplateName = this->particleTemplateName->getListSelectedOldValue();
		this->particleTemplateName->setListSelectedValue(particleTemplateName);
		this->createParticleEffect();
	}

	Ogre::String GpuParticlesComponent::getParticleTemplateName(void) const
	{
		return this->particleTemplateName->getListSelectedValue();
	}

	void GpuParticlesComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
		this->destroyParticleEffect();
	}

	bool GpuParticlesComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void GpuParticlesComponent::setParticlePlayTimeMS(Ogre::Real playTime)
	{
		this->particleInitialPlayTime->setValue(playTime);
		this->particlePlayTime = playTime;
		this->destroyParticleEffect();
	}

	Ogre::Real GpuParticlesComponent::getParticlePlayTimeMS(void) const
	{
		return this->particleInitialPlayTime->getReal();
	}

	void GpuParticlesComponent::setParticlePlaySpeed(Ogre::Real playSpeed)
	{
		this->particlePlaySpeed->setValue(playSpeed);
		this->destroyParticleEffect();
	}

	Ogre::Real GpuParticlesComponent::getParticlePlaySpeed(void) const
	{
		return this->particlePlaySpeed->getReal();
	}

	void GpuParticlesComponent::setParticleOffsetPosition(const Ogre::Vector3& particleOffsetPosition)
	{
		this->particleOffsetPosition->setValue(particleOffsetPosition);
	}

	Ogre::Vector3 GpuParticlesComponent::getParticleOffsetPosition(void)
	{
		return this->particleOffsetPosition->getVector3();
	}

	void GpuParticlesComponent::setParticleOffsetOrientation(const Ogre::Vector3& particleOffsetOrientation)
	{
		this->particleOffsetOrientation->setValue(particleOffsetOrientation);
	}

	Ogre::Vector3 GpuParticlesComponent::getParticleOffsetOrientation(void)
	{
		return this->particleOffsetOrientation->getVector3();
	}

	void GpuParticlesComponent::setParticleScale(const Ogre::Vector3& particleScale)
	{
		this->particleScale->setValue(particleScale);
	}

	Ogre::Vector3 GpuParticlesComponent::getParticleScale(void)
	{
		return this->particleScale->getVector3();
	}

	GpuParticleSystem* GpuParticlesComponent::getParticle(void) const
	{
		return this->gpuParticleSystem;
	}

	bool GpuParticlesComponent::isPlaying(void) const
	{
		return this->particlePlayTime > 0.0f;
	}

	void GpuParticlesComponent::setGlobalPosition(const Ogre::Vector3& particlePosition)
	{
		this->globalPosition = particlePosition;
	}

	void GpuParticlesComponent::setGlobalOrientation(const Ogre::Vector3& particleOrientation)
	{
		this->globalOrientation = MathHelper::getInstance()->degreesToQuat(particleOrientation);
	}

	// Lua registration part

	GpuParticlesComponent* getGpuParticlesComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<GpuParticlesComponent>(gameObject->getComponentWithOccurrence<GpuParticlesComponent>(occurrenceIndex)).get();
	}

	GpuParticlesComponent* getGpuParticlesComponent(GameObject* gameObject)
	{
		return makeStrongPtr<GpuParticlesComponent>(gameObject->getComponent<GpuParticlesComponent>()).get();
	}

	GpuParticlesComponent* getGpuParticlesComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<GpuParticlesComponent>(gameObject->getComponentFromName<GpuParticlesComponent>(name)).get();
	}

	void GpuParticlesComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<GpuParticlesComponent, GameObjectComponent>("GpuParticlesComponent")
			.def("setActivated", &GpuParticlesComponent::setActivated)
			.def("isActivated", &GpuParticlesComponent::isActivated)
			.def("setTemplateName", &ParticleUniverseComponent::setParticleTemplateName)
			.def("getTemplateName", &ParticleUniverseComponent::getParticleTemplateName)
			.def("setRepeat", &ParticleUniverseComponent::setRepeat)
			.def("getRepeat", &ParticleUniverseComponent::getRepeat)
			.def("setPlayTimeMS", &ParticleUniverseComponent::setParticlePlayTimeMS)
			.def("getPlayTimeMS", &ParticleUniverseComponent::getParticlePlayTimeMS)
			.def("setOffsetPosition", &ParticleUniverseComponent::setParticleOffsetPosition)
			.def("getOffsetPosition", &ParticleUniverseComponent::getParticleOffsetPosition)
			.def("setOffsetOrientation", &ParticleUniverseComponent::setParticleOffsetOrientation)
			.def("getOffsetOrientation", &ParticleUniverseComponent::getParticleOffsetOrientation)
			.def("setScale", &ParticleUniverseComponent::setParticleScale)
			.def("getScale", &ParticleUniverseComponent::getParticleScale)
			.def("setPlaySpeed", &ParticleUniverseComponent::setParticlePlaySpeed)
			.def("getPlaySpeed", &ParticleUniverseComponent::getParticlePlaySpeed)
			.def("isPlaying", &ParticleUniverseComponent::isPlaying)
			.def("setGlobalPosition", &ParticleUniverseComponent::setGlobalPosition)
			.def("setGlobalOrientation", &ParticleUniverseComponent::setGlobalOrientation)
		];

		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "class inherits GameObjectComponent", GpuParticlesComponent::getStaticInfoText());

		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not (Start the particle effect).");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setTemplateName(string particleTemplateName)", "Sets the particle template name. The name must be recognized by the resource system, else the particle effect cannot be played.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "string getTemplateName()", "Gets currently used particle template name.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setRepeat(bool repeat)", "Sets whether the current particle effect should be repeated when finished.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "bool getRepeat()", "Gets whether the current particle effect will be repeated when finished.");

		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setPlayTimeMS(float particlePlayTimeMS)", "Sets particle play time in milliseconds, how long the particle effect should be played.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "float getPlayTimeMS()", "Gets particle play time in milliseconds.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets offset position of the particle effect at which it should be played away from the game object.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "Vector3 getOffsetPosition()", "Gets offset position of the particle effect.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setOffsetOrientation(Vector3 orientation)", "Sets offset orientation (as vector3(degree, degree, degree)) of the particle effect at which it should be played away from the game object.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "Vector3 getOffsetPosition()", "Gets offset orientation (as vector3(degree, degree, degree)) of the particle effect.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setScale(Vector3 scale)", "Sets the scale (size) of the particle effect.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "Vector3 getScale()", "Gets scale of the particle effect.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setPlaySpeed(float playSpeed)", "Sets particle play speed. E.g. 2 will play the particle at double speed.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "float getPlaySpeed()", "Gets particle play play speed.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "bool isPlaying()", "Gets whether the particle is playing. Note: This affects the value of @PlayTimeMS.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setGlobalPosition(Vector3 globalPosition)", "Sets a global play position for the particle.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setGlobalOrientation(Vector3 globalOrientation)", "Sets a global player orientation (as vector3(degree, degree, degree)) of the particle effect.");

		gameObjectClass.def("getGpuParticlesComponentFromName", &getGpuParticlesComponentFromName);
		gameObjectClass.def("getGpuParticlesComponent", (GpuParticlesComponent * (*)(GameObject*)) & getGpuParticlesComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getGpuParticlesComponentFromIndex", (GpuParticlesComponent * (*)(GameObject*, unsigned int)) & getGpuParticlesComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "GpuParticlesComponent getGpuParticlesComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "GpuParticlesComponent getGpuParticlesComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "GpuParticlesComponent getGpuParticlesComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castGpuParticlesComponent", &GameObjectController::cast<GpuParticlesComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "GpuParticlesComponent castGpuParticlesComponent(GpuParticlesComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool GpuParticlesComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end