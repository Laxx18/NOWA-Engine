#include "NOWAPrecompiled.h"
#include "IESLightComponent.h"
#include "gameobject/LightSpotComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "modules/GraphicsModule.h"

#include "OgreBitwise.h"
#include "OgreAbiUtils.h"
#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreTextureGpuManager.h"
#include "OgreResourceGroupManager.h"

#include "LightProfiles/OgreLightProfiles.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	// Initialize static members
	Ogre::LightProfiles* IESLightComponent::s_lightProfilesManager = nullptr;
	unsigned int IESLightComponent::s_instanceCount = 0;

	IESLightComponent::IESLightComponent()
		: GameObjectComponent(),
		name("IESLightComponent"),
		activated(new Variant(IESLightComponent::AttrActivated(), true, this->attributes)),
		iesProfile(new Variant(IESLightComponent::AttrIESProfile(), std::vector<Ogre::String>{}, this->attributes)),
		lightComponent(nullptr),
		ogreLight(nullptr),
		iesProfileApplied(false)
	{
		this->iesProfile->setDescription("Select an IES profile file. These contain real-world light distribution patterns.");
	}

	IESLightComponent::~IESLightComponent(void)
	{
		
	}

	void IESLightComponent::initialise()
	{
		// Plugin initialisation if needed
	}

	const Ogre::String& IESLightComponent::getName() const
	{
		return this->name;
	}

	void IESLightComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<IESLightComponent>(IESLightComponent::getStaticClassId(), IESLightComponent::getStaticClassName());
	}

	void IESLightComponent::shutdown()
	{
		// Called too late - use onRemoveComponent
	}

	void IESLightComponent::uninstall()
	{
		// Called too late - use onRemoveComponent
	}

	void IESLightComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool IESLightComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IESProfile")
		{
			this->iesProfile->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr IESLightComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		IESLightComponentPtr clonedCompPtr(boost::make_shared<IESLightComponent>());

		clonedCompPtr->setIESProfile(this->iesProfile->getListSelectedValue());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool IESLightComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IESLightComponent] Init component for game object: " + this->gameObjectPtr->getName());

		IESLightComponent::s_instanceCount++;

		// Initialize the global LightProfiles manager
		this->initializeLightProfilesManager();

		// Populate available IES files for the editor
		this->updateAvailableIESFiles();

		return true;
	}

	bool IESLightComponent::connect(void)
	{
		GameObjectComponent::connect();

		auto& lightSpotCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<LightSpotComponent>());

		if (nullptr != lightSpotCompPtr)
		{
			this->lightComponent = lightSpotCompPtr.get();
			this->ogreLight = lightSpotCompPtr->getOgreLight();
		}
		else
		{
			return false;
		}

		// Apply/remove based on current activated state
		this->setActivated(this->activated->getBool());
		return true;
	}


	bool IESLightComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		// Always remove IES when leaving simulation
		this->removeIESProfile();

		return true;
	}

	void IESLightComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		this->removeIESProfile();

		const unsigned int newCount = --IESLightComponent::s_instanceCount;

		if (newCount == 0 && s_lightProfilesManager != nullptr)
		{
			NOWA::GraphicsModule::RenderCommand renderCommand = []()
			{
				OGRE_DELETE IESLightComponent::s_lightProfilesManager;
				IESLightComponent::s_lightProfilesManager = nullptr;
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "IESLightComponent::DestroyLightProfilesManager");
		}
	}


	void IESLightComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Runtime updates if needed
			// For now, IES profiles are static once applied
		}
	}

	void IESLightComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (IESLightComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (IESLightComponent::AttrIESProfile() == attribute->getName())
		{
			this->setIESProfile(attribute->getListSelectedValue());
		}
	}

	void IESLightComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
		propertyXML->append_attribute(doc.allocate_attribute("name", "IESProfile"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->iesProfile->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String IESLightComponent::getClassName(void) const
	{
		return "IESLightComponent";
	}

	Ogre::String IESLightComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void IESLightComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (this->ogreLight == nullptr)
			return;

		if (true == this->activated->getBool() && false == this->iesProfile->getListSelectedValue().empty())
		{
			this->applyIESProfile();
		}
		else
		{
			this->removeIESProfile();
		}
	}

	bool IESLightComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void IESLightComponent::setIESProfile(const Ogre::String& iesFileName)
	{
		this->iesProfile->setListSelectedValue(iesFileName);

		if (this->activated->getBool() && this->ogreLight != nullptr)
		{
			this->applyIESProfile();
		}
		else
		{
			this->removeIESProfile();
		}
	}

	Ogre::String IESLightComponent::getIESProfile(void) const
	{
		return this->iesProfile->getListSelectedValue();
	}

	void IESLightComponent::initializeLightProfilesManager(void)
	{
		if (IESLightComponent::s_lightProfilesManager != nullptr)
		{
			return; // Already initialized
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			Ogre::Root* root = Ogre::Root::getSingletonPtr();
			if (root == nullptr)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[IESLightComponent] Cannot initialize LightProfiles: Ogre::Root not available");
				return;
			}

			Ogre::TextureGpuManager* textureMgr = root->getRenderSystem()->getTextureGpuManager();
			Ogre::Hlms* hlms = root->getHlmsManager()->getHlms(Ogre::HLMS_PBS);

			if (hlms == nullptr)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[IESLightComponent] Cannot initialize LightProfiles: HlmsPbs not available");
				return;
			}

			assert(dynamic_cast<Ogre::HlmsPbs*>(hlms));
			Ogre::HlmsPbs* pbs = static_cast<Ogre::HlmsPbs*>(hlms);

			IESLightComponent::s_lightProfilesManager = OGRE_NEW Ogre::LightProfiles(pbs, textureMgr);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IESLightComponent] LightProfiles manager initialized with LTC matrix loaded");
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "IESLightComponent::initializeLightProfilesManager");
	}

	void IESLightComponent::applyIESProfile(void)
	{
		if (IESLightComponent::s_lightProfilesManager == nullptr || this->ogreLight == nullptr)
		{
			return;
		}

		const Ogre::String profileName = this->iesProfile->getListSelectedValue();
		if (true == profileName.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IESLightComponent] No IES profile selected for game object: " + this->gameObjectPtr->getName());
			return;
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [this, profileName]()
		{
			try
			{
				// Load the IES profile if not already loaded
				// false = Don't skip if already loaded
				IESLightComponent::s_lightProfilesManager->loadIesProfile(profileName, "IES", false);

				// Assign the profile to this light
				IESLightComponent::s_lightProfilesManager->assignProfile(profileName, this->ogreLight);

				// Build/rebuild the profile texture array
				IESLightComponent::s_lightProfilesManager->build();

				this->iesProfileApplied = true;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IESLightComponent] Applied IES profile '" + profileName + "' to game object: " + this->gameObjectPtr->getName());
			}
			catch (const Ogre::Exception& e)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[IESLightComponent] Failed to apply IES profile '" + profileName + "': " + e.getFullDescription());
				this->iesProfileApplied = false;
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "IESLightComponent::applyIESProfile");
	}

	void IESLightComponent::removeIESProfile(void)
	{
		if (false == this->iesProfileApplied || IESLightComponent::s_lightProfilesManager == nullptr || this->ogreLight == nullptr)
		{
			return;
		}

		const Ogre::String profileName = this->iesProfile->getListSelectedValue();

		NOWA::GraphicsModule::RenderCommand renderCommand = [this, profileName]()
		{
			try
			{
				// Remove the profile assignment (set to null/default)
				IESLightComponent::s_lightProfilesManager->assignProfile("", this->ogreLight);

				// Rebuild the profile texture array
				IESLightComponent::s_lightProfilesManager->build();

				this->lightComponent = nullptr;
				this->ogreLight = nullptr;

				this->iesProfileApplied = false;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IESLightComponent] Removed IES profile from game object: " + this->gameObjectPtr->getName());
			}
			catch (const Ogre::Exception& e)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[IESLightComponent] Failed to remove IES profile: " + e.getFullDescription());
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "IESLightComponent::removeIESProfile");
	}

	void IESLightComponent::updateAvailableIESFiles(void)
	{
		// Keep current selection (so the editor doesn't reset it)
		Ogre::String prevSelection = this->iesProfile->getListSelectedValue();
		Ogre::StringUtil::trim(prevSelection);

		std::vector<Ogre::String> profiles;
		profiles.emplace_back(""); // "none" entry

		// Gather resource names on render thread (safe in your architecture)
		NOWA::GraphicsModule::RenderCommand renderCommand = [&profiles]()
		{
			Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

			if (false == rgm.resourceGroupExists("IES"))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[IESLightComponent] Resource group 'IES' does not exist.");
				return;
			}

			// On Windows the filesystem is case-insensitive, so searching "*.ies" and "*.IES"
			// returns duplicates. We de-duplicate to be safe across platforms.
			std::set<Ogre::String> uniqueNames;

			{
				Ogre::StringVectorPtr files = rgm.findResourceNames("IES", "*.ies");
				for (const Ogre::String& f : *files)
				{
					Ogre::String name = f;
					Ogre::StringUtil::trim(name);

					if (false == name.empty())
					{
						uniqueNames.insert(name);
					}
				}
			}

			{
				Ogre::StringVectorPtr files = rgm.findResourceNames("IES", "*.IES");
				for (const Ogre::String& f : *files)
				{
					Ogre::String name = f;
					Ogre::StringUtil::trim(name);

					if (false == name.empty())
					{
						uniqueNames.insert(name);
					}
				}
			}

			for (const Ogre::String& name : uniqueNames)
			{
				profiles.emplace_back(name);
			}
		};

		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "IESLightComponent::updateAvailableIESFiles");

		// Sort (skip the first empty entry)
		if (profiles.size() > 2)
		{
			std::sort(profiles.begin() + 1, profiles.end());
		}

		// Push into Variant list
		this->iesProfile->setValue(profiles);
	}

	bool IESLightComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only add if there's a supported light component (Point or Spot) and no existing IESLightComponent
		auto iesCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<IESLightComponent>());
		auto spotLightCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<LightSpotComponent>());

		if (nullptr == iesCompPtr && nullptr != spotLightCompPtr)
		{
			return true;
		}
		return false;
	}

	// Lua registration

	IESLightComponent* getIESLightComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<IESLightComponent>(gameObject->getComponentWithOccurrence<IESLightComponent>(occurrenceIndex)).get();
	}

	IESLightComponent* getIESLightComponent(GameObject* gameObject)
	{
		return makeStrongPtr<IESLightComponent>(gameObject->getComponent<IESLightComponent>()).get();
	}

	IESLightComponent* getIESLightComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<IESLightComponent>(gameObject->getComponentFromName<IESLightComponent>(name)).get();
	}

	void IESLightComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<IESLightComponent, GameObjectComponent>("IESLightComponent")
			.def("setActivated", &IESLightComponent::setActivated)
			.def("isActivated", &IESLightComponent::isActivated)
			.def("setIESProfile", &IESLightComponent::setIESProfile)
			.def("getIESProfile", &IESLightComponent::getIESProfile)
		];

		LuaScriptApi::getInstance()->addClassToCollection("IESLightComponent", "class inherits GameObjectComponent", IESLightComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("IESLightComponent", "void setActivated(bool activated)", "Sets whether this IES is active.");
		LuaScriptApi::getInstance()->addClassToCollection("IESLightComponent", "bool isActivated()", "Gets whether this IES is active.");
		LuaScriptApi::getInstance()->addClassToCollection("IESLightComponent", "void setIESProfile(string iesFileName)", "Sets the IES profile file to use for photometric lighting.");
		LuaScriptApi::getInstance()->addClassToCollection("IESLightComponent", "string getIESProfile()", "Gets the currently assigned IES profile filename.");

		gameObjectClass.def("getIESLightComponent", (IESLightComponent * (*)(GameObject*)) & getIESLightComponent);
		gameObjectClass.def("getIESLightComponent", (IESLightComponent * (*)(GameObject*, unsigned int)) & getIESLightComponent);
		gameObjectClass.def("getIESLightComponentFromName", (IESLightComponent * (*)(GameObject*, const Ogre::String&)) & getIESLightComponentFromName);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "IESLightComponent getIESLightComponent()", "Gets the IES light component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "IESLightComponent getIESLightComponent(unsigned int occurrenceIndex)", "Gets the IES light component by occurrence index.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "IESLightComponent getIESLightComponentFromName(string name)", "Gets the IES light component by custom name.");
	}

}; // namespace end