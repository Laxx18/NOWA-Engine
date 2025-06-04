#include "NOWAPrecompiled.h"
#include "DistortionComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	DistortionComponent::DistortionComponent()
		: GameObjectComponent(),
		name("DistortionComponent"),
		distortionPass(nullptr),
		oldRenderQueueIndex(0),
		distortionDatablock(nullptr),
		workspaceBaseComponent(nullptr),
		activated(new Variant(DistortionComponent::AttrActivated(), true, this->attributes)),
		strength(new Variant(DistortionComponent::AttrStrength(), 1.0f, this->attributes))
	{
		this->activated->setDescription("Activated the distortion effect.");
		this->strength->setConstraints(0.5f, 10.0f);
	}

	DistortionComponent::~DistortionComponent(void)
	{
		
	}

	void DistortionComponent::initialise()
	{

	}

	const Ogre::String& DistortionComponent::getName() const
	{
		return this->name;
	}

	void DistortionComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<DistortionComponent>(DistortionComponent::getStaticClassId(), DistortionComponent::getStaticClassName());
	}

	void DistortionComponent::shutdown()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void DistortionComponent::uninstall()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}
	
	void DistortionComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool DistortionComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Strength")
		{
			this->strength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr DistortionComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		DistortionComponentPtr clonedCompPtr(boost::make_shared<DistortionComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setStrength(this->strength->getReal());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool DistortionComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistortionComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// For now it will only work for main camera
		auto mainCameraGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(GameObjectController::MAIN_CAMERA_ID);
		if (nullptr != mainCameraGameObject)
		{
			auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(mainCameraGameObject->getComponent<WorkspaceBaseComponent>());
			if (nullptr != workspaceBaseCompPtr)
			{
				this->workspaceBaseComponent = workspaceBaseCompPtr.get();
				if (false == this->workspaceBaseComponent->getUseDistortion())
				{
					this->workspaceBaseComponent->setUseDistortion(true);
				}
			}
		}

		return true;
	}

	bool DistortionComponent::connect(void)
	{
		this->setActivated(this->activated->getBool());
		
		return true;
	}

	bool DistortionComponent::disconnect(void)
	{
		this->destroyDistoration();
		return true;
	}

	bool DistortionComponent::onCloned(void)
	{
		
		return true;
	}

	void DistortionComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
		this->destroyDistoration();
	}
	
	void DistortionComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void DistortionComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}

	void DistortionComponent::createDistortion(void)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND("DistortionComponent::createDistortion",
		{
			if (nullptr == this->distortionDatablock)
			{
				// Store current datablock name
				auto datablockNames = this->gameObjectPtr->getDatablockNames();
				if (false == datablockNames.empty())
				{
					this->oldDatablockName = this->gameObjectPtr->getDatablockNames()[0];
					this->oldRenderQueueIndex = this->gameObjectPtr->getRenderQueueIndex();
				}

				Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
				assert(dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT)));
				Ogre::HlmsUnlit* hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

				/*
				  Setup materials and items:
				  - Use unlit material with displacement texture.
				  - Distortion objects will be rendered in their own render queue to a separate texture.
				  - See distortion compositor and shaders for more information
				*/

				// Create proper blend block for distortion objects
				Ogre::HlmsBlendblock blendBlock = Ogre::HlmsBlendblock();
				blendBlock.mIsTransparent = true;
				blendBlock.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
				blendBlock.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;

				// Create macro block to disable depth write
				Ogre::HlmsMacroblock macroBlock = Ogre::HlmsMacroblock();
				macroBlock.mDepthWrite = false;
				macroBlock.mDepthCheck = true;


				Ogre::String datablockName = "DistortionMaterial_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
				this->distortionDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock(datablockName, datablockName, macroBlock, blendBlock, Ogre::HlmsParamVec()));

				Ogre::TextureGpuManager* textureMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
				// Use non-color data as texture type because distortion is stored as x,y vectors to texture.
				Ogre::TextureGpu* texture = textureMgr->createOrRetrieveTexture("distort_deriv.png", Ogre::GpuPageOutStrategy::Discard, 0, Ogre::TextureTypes::Type2D,
																				Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

				this->distortionDatablock->setTexture(0, texture);

				// Set material to use vertex colors. Vertex colors are used to control distortion intensity (alpha value)
				this->distortionDatablock->setUseColour(true);
				// Random alpha value for objects. Alpha value is multiplier for distortion strenght
				this->distortionDatablock->setColour(Ogre::ColourValue(1.0f, 1.0f, 1.0f, Ogre::Math::RangeRandom(0.25f, 0.5f)));

			}

			if (GameObject::ITEM == this->gameObjectPtr->getType())
			{
				Ogre::Item* item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
				if (nullptr != item)
				{
					item->setDatablock(this->distortionDatablock);
					// Set item to be rendered in distortion queue pass (ID 16)
					this->gameObjectPtr->setRenderQueueIndex(16);
				}
			}
			else
			{
				Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>();
				if (nullptr != entity)
				{
					entity->setDatablock(this->distortionDatablock);
					// Set item to be rendered in distortion queue pass (ID 16)
					this->gameObjectPtr->setRenderQueueIndex(16);
				}
			}

			// Receive distortion material and set strenght uniform
			Ogre::MaterialPtr materialDistortion = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("Distortion/Quad", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

			Ogre::Pass* passDist = materialDistortion->getTechnique(0)->getPass(0);
			this->distortionPass = passDist;
			Ogre::GpuProgramParametersSharedPtr psParams = passDist->getFragmentProgramParameters();
			psParams->setNamedConstant("u_DistortionStrenght", this->strength->getReal());
		});
	}

	void DistortionComponent::destroyDistoration(void)
	{
		ENQUEUE_RENDER_COMMAND("DistortionComponent::destroyDistoration",
		{
			if (0 != this->oldRenderQueueIndex)
			{
				this->gameObjectPtr->setRenderQueueIndex(this->oldRenderQueueIndex);
			}

			this->distortionPass = nullptr;

			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				if (false != this->oldDatablockName.empty())
				{
					// Set back the default datablock
					entity->setDatablock(this->oldDatablockName);
				}
			}
			else
			{
				Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
				if (false == this->oldDatablockName.empty())
				{
					// Set back the default datablock
					item->setDatablock(this->oldDatablockName);
				}
			}

			if (nullptr != this->distortionDatablock)
			{
				auto& linkedRenderabled = this->distortionDatablock->getLinkedRenderables();

				// Only destroy if the datablock is not used else where
				if (true == linkedRenderabled.empty())
				{
					this->distortionDatablock->getCreator()->destroyDatablock(this->distortionDatablock->getName());
					this->distortionDatablock = nullptr;
				}
			}
		});
	}
	
	void DistortionComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			auto distortionPass = this->distortionPass;
			Ogre::Real strength = this->strength->getReal();
			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("DistortionComponent::update", _2(distortionPass, strength),
			{
				// Update distortion uniform
				Ogre::GpuProgramParametersSharedPtr psParams = distortionPass->getFragmentProgramParameters();
				psParams->setNamedConstant("u_DistortionStrenght", strength);
			});

			// this->gameObjectPtr->getSceneNode()->yaw(Ogre::Radian(dt * 0.125f));
		}
	}

	void DistortionComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (DistortionComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if(DistortionComponent::AttrStrength() == attribute->getName())
		{
			this->setStrength(attribute->getReal());
		}
	}

	void DistortionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Strength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->strength->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String DistortionComponent::getClassName(void) const
	{
		return "DistortionComponent";
	}

	Ogre::String DistortionComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void DistortionComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == activated)
		{
			this->createDistortion();
		}
		else
		{
			this->destroyDistoration();
		}
	}

	bool DistortionComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void DistortionComponent::setStrength(Ogre::Real strength)
	{
		if (strength < 0.5f)
		{
			strength = 0.5f;
		}
		else if (strength > 10.0f)
		{
			strength = 10.0f;
		}
		this->strength->setValue(strength);
	}

	Ogre::Real DistortionComponent::getStrength(void) const
	{
		return this->strength->getReal();
	}

	// Lua registration part

	DistortionComponent* getDistortionComponent(GameObject* gameObject)
	{
		return makeStrongPtr<DistortionComponent>(gameObject->getComponent<DistortionComponent>()).get();
	}

	DistortionComponent* getDistortionComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<DistortionComponent>(gameObject->getComponentFromName<DistortionComponent>(name)).get();
	}

	void DistortionComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<DistortionComponent, GameObjectComponent>("DistortionComponent")
			.def("setActivated", &DistortionComponent::setActivated)
			.def("isActivated", &DistortionComponent::isActivated)
			.def("setStrength", &DistortionComponent::setStrength)
			.def("getStrength", &DistortionComponent::getStrength)
		];

		LuaScriptApi::getInstance()->addClassToCollection("DistortionComponent", "class inherits GameObjectComponent", DistortionComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("DistortionComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("DistortionComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("DistortionComponent", "void setStrength(float strength)", "Sets the distortion strength. Valid values are from 0.5 to 3.0. Default is 0.5.");
		LuaScriptApi::getInstance()->addClassToCollection("DistortionComponent", "float getStrength()", "Gets the distortion strength.");

		gameObjectClass.def("getDistortionComponentFromName", &getDistortionComponentFromName);
		gameObjectClass.def("getDistortionComponent", (DistortionComponent * (*)(GameObject*)) & getDistortionComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "DistortionComponent getDistortionComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "DistortionComponent getDistortionComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castDistortionComponent", &GameObjectController::cast<DistortionComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "DistortionComponent castDistortionComponent(DistortionComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool DistortionComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto distortionCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<DistortionComponent>());
		if (nullptr == distortionCompPtr)
		{
			return true;
		}

		return false;
	}

}; //namespace end