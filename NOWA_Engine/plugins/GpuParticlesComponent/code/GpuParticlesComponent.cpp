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
		activated(new Variant(GpuParticlesComponent::AttrActivated(), true, this->attributes))
	{
		this->activated->setDescription("Description.");
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

	bool GpuParticlesComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		/*if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShortTimeActivation")
		{
			this->shortTimeActivation->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}*/
		
		return true;
	}

	GameObjectCompPtr GpuParticlesComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		GpuParticlesComponentPtr clonedCompPtr(boost::make_shared<GpuParticlesComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool GpuParticlesComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GpuParticlesComponent] Init component for game object: " + this->gameObjectPtr->getName());

		this->gpuParticleSystemWorld = NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->getGpuParticleSystemWorld();

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

		Ogre::uint64 particleInstanceId = this->gpuParticleSystemWorld->start(this->gpuParticleSystem, nullptr, this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation());

		return true;
	}

	bool GpuParticlesComponent::connect(void)
	{
		
		
		return true;
	}

	bool GpuParticlesComponent::disconnect(void)
	{
		this->gpuParticleSystemWorld->stopAndRemoveAllImmediately();
		return true;
	}

	bool GpuParticlesComponent::onCloned(void)
	{
		
		return true;
	}

	void GpuParticlesComponent::onRemoveComponent(void)
	{
		
	}
	
	void GpuParticlesComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void GpuParticlesComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void GpuParticlesComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			this->gpuParticleSystemWorld->processTime(dt);
		}
	}

	void GpuParticlesComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (GpuParticlesComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		/*else if(GpuParticlesComponent::AttrShortTimeActivation() == attribute->getName())
		{
			this->setShortTimeActivation(attribute->getBool());
		}*/
	}

	void GpuParticlesComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);	

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShortTimeActivation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shortTimeActivation->getBool())));
		propertiesXML->append_node(propertyXML);*/
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
	}

	bool GpuParticlesComponent::isActivated(void) const
	{
		return this->activated->getBool();
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
		];

		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "class inherits GameObjectComponent", GpuParticlesComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("GpuParticlesComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getGpuParticlesComponentFromName", &getGpuParticlesComponentFromName);
		gameObjectClass.def("getGpuParticlesComponent", (GpuParticlesComponent * (*)(GameObject*)) & getGpuParticlesComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getGpuParticlesComponent2", (GpuParticlesComponent * (*)(GameObject*, unsigned int)) & getGpuParticlesComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "GpuParticlesComponent getGpuParticlesComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
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