#include "NOWAPrecompiled.h"
#include "KeyholeEffectComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmapDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	KeyholeEffectComponent::KeyholeEffectComponent()
		: CompositorEffectBaseComponent(),
		name("KeyholeEffectComponent"),
		shapeTexture(nullptr),
		shapeStagingTexture(nullptr),
		pass(nullptr),
		radius(new Variant(KeyholeEffectComponent::AttrRadius(), 0.5f, this->attributes)),
		center(new Variant(KeyholeEffectComponent::AttrCenter(), Ogre::Vector2(0.5f, 0.5f), this->attributes)),
		grow(new Variant(KeyholeEffectComponent::AttrGrow(), false, this->attributes)),
		shapeMask(new Variant(KeyholeEffectComponent::AttrShapeMask(), "Keyhole_Mask.png", this->attributes)),
		speed(new Variant(KeyholeEffectComponent::AttrSpeed(), 0.5f, this->attributes))

	{
		this->effectName = "Keyhole";
		this->shapeMask->setDescription("Sets a shape mask, which can be created in gimp and added to the MyGui/images folder.");
		this->tempRadius = this->radius->getReal();
		this->tempGrow = this->grow->getBool();

		this->shapeMask->addUserData(GameObject::AttrActionFileOpenDialog(), "Essential");
		this->radius->setDescription("Note: If grow is set, radius should start at a minimum value like 0. If grow is set to false, it should start at an high value e.g. 1.");
	}

	KeyholeEffectComponent::~KeyholeEffectComponent(void)
	{
		
	}

	const Ogre::String& KeyholeEffectComponent::getName() const
	{
		return this->name;
	}

	void KeyholeEffectComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<KeyholeEffectComponent>(KeyholeEffectComponent::getStaticClassId(), KeyholeEffectComponent::getStaticClassName());
	}
	
	void KeyholeEffectComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool KeyholeEffectComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == KeyholeEffectComponent::AttrRadius())
		{
			this->radius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == KeyholeEffectComponent::AttrCenter())
		{
			this->center->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == KeyholeEffectComponent::AttrGrow())
		{
			this->grow->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == KeyholeEffectComponent::AttrShapeMask())
		{
			this->shapeMask->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == KeyholeEffectComponent::AttrSpeed())
		{
			this->speed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr KeyholeEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool KeyholeEffectComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[KeyholeEffectComponent] Init component for game object: " + this->gameObjectPtr->getName());

		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::String materialName0 = "Postprocess/KeyholeEffect";

		this->material = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->material.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[KeyholeEffectComponent] Could not set: " + this->effectName
															+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		Ogre::Material* material0 = this->material.getPointer();
		this->pass = material0->getTechnique(0)->getPass(0);

		this->createKeyholeCompositorEffect();

		// Set intial value, so that the compositor starts with the loaded values
		this->setRadius(this->radius->getReal());
		this->setCenter(this->center->getVector2());
		this->setGrow(this->grow->getBool());
		this->setShapeMask(this->shapeMask->getString());

		if (nullptr != this->pass)
		{
			// this->pass->getFragmentProgramParameters()->setNamedConstant("screenSize", Ogre::Vector2(NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getHeight()));
		}

		return success;
	}

	bool KeyholeEffectComponent::connect(void)
	{
		bool success = CompositorEffectBaseComponent::connect();

		this->tempRadius = this->radius->getReal();
		this->tempGrow = this->grow->getBool();

		return success;
	}

	bool KeyholeEffectComponent::disconnect(void)
	{
		bool success = CompositorEffectBaseComponent::disconnect();

		this->tempRadius = this->radius->getReal();
		this->tempGrow = this->grow->getBool();

		return success;
	}

	bool KeyholeEffectComponent::onCloned(void)
	{
		return true;
	}

	void KeyholeEffectComponent::onRemoveComponent(void)
	{
		this->destroyShape();
	}
	
	void KeyholeEffectComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void KeyholeEffectComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void KeyholeEffectComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Adjust radius based on time and direction (shrink or grow)
			if (true == this->tempGrow)
			{
				this->tempRadius += this->speed->getReal() * dt;  // Increase by 20% per second
				// this->tempRadius += 0.2f * dt;  // Increase by 20% per second
			}
			else
			{
				this->tempRadius -= this->speed->getReal() * dt;  // Decrease by 20% per second
			}

			// Clamp the radius between 0 and 1
			if (this->tempRadius > 1.0f)
			{
				// this->tempGrow = false;
				this->tempRadius = 1.0f;
			}
			else if (this->tempRadius < 0.0f)
			{
				// this->tempGrow = true;
				this->tempRadius = 0.0f;
			}

			if (nullptr != this->pass)
			{
				// Use normalized radius (1.0 = 100% of the screen)
				// this->pass->getFragmentProgramParameters()->setNamedConstant("radius", this->tempRadius);
				// this->pass->getFragmentProgramParameters()->setNamedConstant("direction", this->tempGrow ? 1 : 0);
			}
		}
	}

	void KeyholeEffectComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (KeyholeEffectComponent::AttrRadius() == attribute->getName())
		{
			this->setRadius(attribute->getReal());
		}
		else if (KeyholeEffectComponent::AttrCenter() == attribute->getName())
		{
			this->setCenter(attribute->getVector2());
		}
		else if (KeyholeEffectComponent::AttrGrow() == attribute->getName())
		{
			this->setGrow(attribute->getBool());
		}
		else if (KeyholeEffectComponent::AttrShapeMask() == attribute->getName())
		{
			this->setShapeMask(attribute->getString());
		}
		else if (KeyholeEffectComponent::AttrSpeed() == attribute->getName())
		{
			this->setSpeed(attribute->getReal());
		}
	}

	void KeyholeEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Radius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Center"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->center->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Grow"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->grow->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShapeMask"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shapeMask->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Speed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speed->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String KeyholeEffectComponent::getClassName(void) const
	{
		return "KeyholeEffectComponent";
	}

	Ogre::String KeyholeEffectComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void KeyholeEffectComponent::setRadius(Ogre::Real radius)
	{
		if (radius < 0.0f)
		{
			radius = 0.0f;
		}

		this->radius->setValue(radius);
		this->tempRadius = radius;

		if (nullptr != this->pass)
		{
			// this->pass->getFragmentProgramParameters()->setNamedConstant("radius", radius);
		}
	}

	Ogre::Real KeyholeEffectComponent::getRadius(void) const
	{
		return this->radius->getReal();
	}

	void KeyholeEffectComponent::setCenter(const Ogre::Vector2& center)
	{
		this->center->setValue(center);

		if (nullptr != this->pass)
		{
			Ogre::Vector2 tempCenter(NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getWidth() * center.x, NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getHeight() * center.y);

			// this->pass->getFragmentProgramParameters()->setNamedConstant("center", tempCenter);
		}
	}

	Ogre::Vector2 KeyholeEffectComponent::getCenter(void) const
	{
		return this->center->getVector2();
	}

	void KeyholeEffectComponent::setGrow(bool grow)
	{
		this->grow->setValue(grow);
		this->tempGrow = grow;

		if (nullptr != this->pass)
		{
			// this->pass->getFragmentProgramParameters()->setNamedConstant("direction", grow ? 1 : 0);
		}
	}

	bool KeyholeEffectComponent::getGrow(void) const
	{
		return this->grow->getBool();
	}

	void KeyholeEffectComponent::setShapeMask(const Ogre::String& shapeMask)
	{
		this->shapeMask->setValue(shapeMask);

		if (nullptr != this->pass)
		{
			Ogre::TextureUnitState* texUnitState = this->pass->getTextureUnitState("Keyhole_Mask");
			texUnitState->setTexture(this->shapeTexture);
		}
	}

	Ogre::String KeyholeEffectComponent::getShapeMask(void) const
	{
		return this->shapeMask->getString();
	}

	void KeyholeEffectComponent::createKeyholeCompositorEffect(void)
	{
		Ogre::Image2 shapeImage;
		shapeImage.load(this->shapeMask->getString(), ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

		this->destroyShape();

		//const uint8 numMipmaps = image.getNumMipmaps();

		TextureGpuManager* textureManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();

		Ogre::PixelFormatGpu pixelFormat = shapeImage.getPixelFormat();
		this->shapeTexture = textureManager->createTexture(this->shapeMask->getString() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()),
														   GpuPageOutStrategy::SaveToSystemRam, TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps, TextureTypes::Type2D);
		this->shapeTexture->setResolution(shapeImage.getWidth(), shapeImage.getHeight());
		this->shapeTexture->setPixelFormat(pixelFormat);
		this->shapeTexture->scheduleTransitionTo(GpuResidency::Resident);

		if (nullptr == this->shapeStagingTexture)
		{
			this->shapeStagingTexture = textureManager->getStagingTexture(shapeImage.getWidth(), shapeImage.getHeight(), 1u, 1u, pixelFormat);
		}
		this->shapeStagingTexture->startMapRegion();
		TextureBox texBox = this->shapeStagingTexture->mapRegion(shapeImage.getWidth(), shapeImage.getHeight(), 1u, 1u, pixelFormat);

		//for( uint8 mip=0; mip<numMipmaps; ++mip )
		texBox.copyFrom(shapeImage.getData(0));
		this->shapeStagingTexture->stopMapRegion();
		this->shapeStagingTexture->upload(texBox, this->shapeTexture, 0, 0, 0);

		if (nullptr != this->pass)
		{
			Ogre::TextureUnitState* texUnitState = this->pass->getTextureUnitState("Keyhole_Mask");
			texUnitState->setTexture(this->shapeTexture);
		}

		// Is done in CompositorEffects.compositor

#if 0
		Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();
		//Glass compositor is loaded from script but here is the hard coded equivalent
		if (!compositorManager->hasNodeDefinition("Keyhole"))
		{
			Ogre::CompositorNodeDef* nodeDef = compositorManager->addNodeDefinition("Keyhole");

			// Texture definition
			Ogre::TextureDefinitionBase::TextureDefinition* texDef = nodeDef->addTextureDefinition("Keyhole_Mask");
			texDef->width = 1024;
			texDef->height = 1024;
			texDef->format = Ogre::PFG_RGBA8_UNORM_SRGB;

			Ogre::RenderTargetViewDef* rtv = nodeDef->addRenderTextureView("Keyhole_Mask");
			Ogre::RenderTargetViewEntry attachment;
			attachment.textureName = "Keyhole_Mask";
			rtv->colourAttachments.push_back(attachment);


			//Input channels
			nodeDef->addTextureSourceName("rt_input", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			nodeDef->addTextureSourceName("rt_output", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			nodeDef->mCustomIdentifier = "Ogre/Postprocess";

			nodeDef->setNumTargetPass(1);

			{
				Ogre::CompositorTargetDef* targetDef = nodeDef->addTargetPass("rt_output");

				{
					auto pass = targetDef->addPass(Ogre::PASS_QUAD);
					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Postprocess/KeyholeEffect";
					passQuad->addQuadTextureSource(0, "rt_input");

					passQuad->mProfilingId = "NOWA_Post_Effect_Keyhole_Pass_Quad";

					// this->applySplitScreenModifier(pass);
				}
			}

			// Output channels
			nodeDef->setNumOutputChannels(2);
			nodeDef->mapOutputChannel(0, "rt_output");
			nodeDef->mapOutputChannel(1, "rt_input");
		}
#endif
	}

	void KeyholeEffectComponent::destroyShape(void)
	{
		if (nullptr != this->shapeTexture)
		{
			TextureGpuManager* textureManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();

			textureManager->destroyTexture(this->shapeTexture);
			this->shapeTexture = nullptr;

			if (nullptr != this->shapeStagingTexture)
			{
				textureManager->removeStagingTexture(this->shapeStagingTexture);
				this->shapeStagingTexture = nullptr;
			}
		}
	}

	bool KeyholeEffectComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Is done directly in NOWA-Design
#if 0
		// Can only be added once
		auto keyholeEffectCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<KeyholeEffectComponent>());
		if (nullptr != keyholeEffectCompPtr)
		{
			return false;
		}

		if (keyholeEffectCompPtr != nullptr && keyholeEffectCompPtr->getOwner()->getId() != GameObjectController::MAIN_GAMEOBJECT_ID)
		{
			return false;
		}
#endif
		return true;
	}

	void KeyholeEffectComponent::setSpeed(Ogre::Real speed)
	{
		if (speed < 0.0f)
		{
			speed = 0.0f;
		}

		this->speed->setValue(speed);
	}

	Ogre::Real KeyholeEffectComponent::getSpeed(void) const
	{
		return this->speed->getReal();
	}

	// Lua registration part

	KeyholeEffectComponent* getKeyholeEffectComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<KeyholeEffectComponent>(gameObject->getComponentWithOccurrence<KeyholeEffectComponent>(occurrenceIndex)).get();
	}

	KeyholeEffectComponent* getKeyholeEffectComponent(GameObject* gameObject)
	{
		return makeStrongPtr<KeyholeEffectComponent>(gameObject->getComponent<KeyholeEffectComponent>()).get();
	}

	KeyholeEffectComponent* getKeyholeEffectComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<KeyholeEffectComponent>(gameObject->getComponentFromName<KeyholeEffectComponent>(name)).get();
	}

	void KeyholeEffectComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<KeyholeEffectComponent, GameObjectComponent>("KeyholeEffectComponent")
			.def("setActivated", &KeyholeEffectComponent::setActivated)
			.def("isActivated", &KeyholeEffectComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("KeyholeEffectComponent", "class inherits GameObjectComponent", KeyholeEffectComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("KeyholeEffectComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("KeyholeEffectComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getKeyholeEffectComponentFromName", &getKeyholeEffectComponentFromName);
		gameObjectClass.def("getKeyholeEffectComponent", (KeyholeEffectComponent * (*)(GameObject*)) & getKeyholeEffectComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getKeyholeEffectComponentFromIndex", (KeyholeEffectComponent * (*)(GameObject*, unsigned int)) & getKeyholeEffectComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "KeyholeEffectComponent getKeyholeEffectComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "KeyholeEffectComponent getKeyholeEffectComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "KeyholeEffectComponent getKeyholeEffectComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castKeyholeEffectComponent", &GameObjectController::cast<KeyholeEffectComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "KeyholeEffectComponent castKeyholeEffectComponent(KeyholeEffectComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

}; //namespace end