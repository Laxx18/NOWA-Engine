#include "NOWAPrecompiled.h"
#include "Rect2DComponent.h"
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

	Rect2DComponent::Rect2DComponent()
		: GameObjectComponent(),
		name("Rect2DComponent"),
		activated(new Variant(Rect2DComponent::AttrActivated(), true, this->attributes)),
		border(new Variant(Rect2DComponent::AttrBorder(), Ogre::Vector4(0.0f, 0.0f, 2.0f, 2.0f), this->attributes)),
		rectangle2D(nullptr)
	{
		this->activated->setDescription("Description.");

		// See: https://forums.ogre3d.org/viewtopic.php?t=97081
	}

	Rect2DComponent::~Rect2DComponent(void)
	{
		
	}

	const Ogre::String& Rect2DComponent::getName() const
	{
		return this->name;
	}

	void Rect2DComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<Rect2DComponent>(Rect2DComponent::getStaticClassId(), Rect2DComponent::getStaticClassName());
	}
	
	void Rect2DComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool Rect2DComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Border")
		{
			this->border->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr Rect2DComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		Rect2DComponentPtr clonedCompPtr(boost::make_shared<Rect2DComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setBorder(this->border->getVector4());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool Rect2DComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Rect2DComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool Rect2DComponent::connect(void)
	{
		if (nullptr != this->rectangle2D)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->rectangle2D);
			this->gameObjectPtr->getSceneManager()->destroyRectangle2D(this->rectangle2D);
			this->rectangle2D = nullptr;
		}

		if (true == this->activated->getBool())
		{
			Ogre::HlmsMacroblock macroBlock;
			macroBlock.mDepthCheck = false;
			macroBlock.mDepthWrite = false;
			macroBlock.mCullMode = Ogre::CULL_NONE; // Necessary

			this->rectangle2D = this->gameObjectPtr->getSceneManager()->createRectangle2D(Ogre::SCENE_DYNAMIC);
			this->rectangle2D->initialize(Ogre::BT_DEFAULT, Ogre::Rectangle2D::GeometryFlagQuad);
			this->rectangle2D->setGeometry(Ogre::Vector2(this->border->getVector4().x - this->gameObjectPtr->getPosition().x, this->border->getVector4().y - this->gameObjectPtr->getPosition().y), Ogre::Vector2(this->border->getVector4().z, this->border->getVector4().w));
			// this->rectangle2D->setRenderQueueGroup(10u);  // Render after most stuff
			this->rectangle2D->update();
			this->rectangle2D->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
			this->rectangle2D->setStatic(!this->gameObjectPtr->isDynamic());
			
			bool datablockFound = false;
			auto& datablockUnlitComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockUnlitComponent>());
			if (nullptr != datablockUnlitComponent)
			{
				Ogre::HlmsUnlitDatablock* datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(datablockUnlitComponent->getDataBlock());
				if (nullptr != datablock)
				{
					datablockFound = true;
					datablock->setMacroblock(macroBlock);
					this->rectangle2D->setDatablock(datablock);
				}
			}
			else
			{
				// Pbs not visible, not working???
				auto& datablockPbsComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockPbsComponent>());
				if (nullptr != datablockPbsComponent)
				{
					Ogre::HlmsPbsDatablock* datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(datablockPbsComponent->getDataBlock());
					if (nullptr != datablock)
					{
						datablockFound = true;
						const Ogre::HlmsSamplerblock* tempSamplerBlock = datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS);
						if (nullptr != tempSamplerBlock)
						{
							Ogre::HlmsSamplerblock samplerblock(*datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS));
							samplerblock.mU = Ogre::TAM_WRAP;
							samplerblock.mV = Ogre::TAM_WRAP;
							samplerblock.mW = Ogre::TAM_WRAP;
							//Set the new samplerblock. The Hlms system will
							//automatically create the API block if necessary
							datablock->setSamplerblock(Ogre::PBSM_ROUGHNESS, samplerblock);
						}
						datablock->setMacroblock(macroBlock);
						this->rectangle2D->setDatablock(datablock);
					}
				}
			}

			if (false == datablockFound)
			{
				Ogre::Hlms* hlms = Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
				Ogre::HlmsUnlitDatablock* datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlms->getDefaultDatablock());
				datablock->setMacroblock(macroBlock);
				this->rectangle2D->setDatablock(datablock);
			}

			this->gameObjectPtr->getSceneNode()->attachObject(this->rectangle2D);
		}
		
		return true;
	}

	bool Rect2DComponent::disconnect(void)
	{
		if (nullptr != this->rectangle2D)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->rectangle2D);
			this->gameObjectPtr->getSceneManager()->destroyRectangle2D(this->rectangle2D);
			this->rectangle2D = nullptr;
		}

		return true;
	}

	bool Rect2DComponent::onCloned(void)
	{
		
		return true;
	}

	void Rect2DComponent::onRemoveComponent(void)
	{
		if (nullptr != this->rectangle2D)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->rectangle2D);
			this->gameObjectPtr->getSceneManager()->destroyRectangle2D(this->rectangle2D);
			this->rectangle2D = nullptr;
		}
	}
	
	void Rect2DComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void Rect2DComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void Rect2DComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			/*if (nullptr != this->rectangle2D)
			{
				this->rectangle2D->update();
			}*/
		}
	}

	void Rect2DComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (Rect2DComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if(Rect2DComponent::AttrBorder() == attribute->getName())
		{
			this->setBorder(attribute->getVector4());
		}
	}

	void Rect2DComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Border"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->border->getVector4())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String Rect2DComponent::getClassName(void) const
	{
		return "Rect2DComponent";
	}

	Ogre::String Rect2DComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void Rect2DComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool Rect2DComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void Rect2DComponent::setBorder(const Ogre::Vector4& border)
	{
		this->border->setValue(border);
	}

	Ogre::Vector4 Rect2DComponent::getBorder(void) const
	{
		return this->border->getVector4();
	}

	// Lua registration part

	Rect2DComponent* getRect2DComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<Rect2DComponent>(gameObject->getComponentWithOccurrence<Rect2DComponent>(occurrenceIndex)).get();
	}

	Rect2DComponent* getRect2DComponent(GameObject* gameObject)
	{
		return makeStrongPtr<Rect2DComponent>(gameObject->getComponent<Rect2DComponent>()).get();
	}

	Rect2DComponent* getRect2DComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<Rect2DComponent>(gameObject->getComponentFromName<Rect2DComponent>(name)).get();
	}

	void Rect2DComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<Rect2DComponent, GameObjectComponent>("Rect2DComponent")
			.def("setActivated", &Rect2DComponent::setActivated)
			.def("isActivated", &Rect2DComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("Rect2DComponent", "class inherits GameObjectComponent", Rect2DComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("Rect2DComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("Rect2DComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getRect2DComponentFromName", &getRect2DComponentFromName);
		gameObjectClass.def("getRect2DComponent", (Rect2DComponent * (*)(GameObject*)) & getRect2DComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getRect2DComponentFromIndex", (Rect2DComponent * (*)(GameObject*, unsigned int)) & getRect2DComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "Rect2DComponent getRect2DComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "Rect2DComponent getRect2DComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "Rect2DComponent getRect2DComponentFromName(String name)", "Gets the component from name.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "Rect2DComponent getRect2DComponentFromIndex(number index)", "Gets the component from occurrence index.");

		gameObjectControllerClass.def("castRect2DComponent", &GameObjectController::cast<Rect2DComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "Rect2DComponent castRect2DComponent(Rect2DComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool Rect2DComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto& datablockUnlitComponent = NOWA::makeStrongPtr(gameObject->getComponent<DatablockUnlitComponent>());
		if (nullptr != datablockUnlitComponent)
		{
			Ogre::HlmsUnlitDatablock* datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(datablockUnlitComponent->getDataBlock());
			if (nullptr != datablock)
			{
				return true;
			}
		}
		return false;
	}

}; //namespace end