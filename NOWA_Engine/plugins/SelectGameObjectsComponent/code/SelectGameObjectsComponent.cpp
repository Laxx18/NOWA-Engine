#include "NOWAPrecompiled.h"
#include "SelectGameObjectsComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "main/InputDeviceCore.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	class SelectionObserver : public NOWA::SelectionManager::ISelectionObserver
	{
	public:
		SelectionObserver()
			: selectionStrategy(new NOWA::DefaultOutLine()) // = new NOWA::RimEffectOutLine();
		{

		}

		virtual ~SelectionObserver()
		{
			if (this->selectionStrategy)
			{
				delete this->selectionStrategy;
				this->selectionStrategy = nullptr;
			}
		}

		virtual void onHandleSelection(NOWA::GameObject* gameObject, bool selected) override
		{
			if (true == selected)
			{
				this->selectionStrategy->highlight(gameObject);
			}
			else
			{
				this->selectionStrategy->unHighlight(gameObject);
			}
		}
	private:
		NOWA::DefaultOutLine* selectionStrategy;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SelectGameObjectsComponent::SelectGameObjectsComponent()
		: GameObjectComponent(),
		name("SelectGameObjectsComponent"),
		bIsInSimulation(false),
		activated(new Variant(SelectGameObjectsComponent::AttrActivated(), true, this->attributes)),
		categories(new Variant(SelectGameObjectsComponent::AttrCategories(), Ogre::String("All"), this->attributes)),
		useMultiSelection(new Variant(SelectGameObjectsComponent::AttrUseMultiSelection(), true, this->attributes)),
		useSelectionRectangle(new Variant(SelectGameObjectsComponent::AttrUseSelectionRectangle(), true, this->attributes))
	{
		this->selectionManager = new SelectionManager();
		// postbuild still necessary?
		// "$(DevEnvDir)devenv" "$(NOWA)/NOWA_Engine/NOWA_Engine.sln" /Build $(configuration) /project "$(NOWA)/NOWA_Engine/NOWA_Engine/NOWA_Engine.vcxproj" 
	}

	SelectGameObjectsComponent::~SelectGameObjectsComponent(void)
	{
		
	}

	void SelectGameObjectsComponent::initialise()
	{

	}

	const Ogre::String& SelectGameObjectsComponent::getName() const
	{
		return this->name;
	}

	void SelectGameObjectsComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<SelectGameObjectsComponent>(SelectGameObjectsComponent::getStaticClassId(), SelectGameObjectsComponent::getStaticClassName());
	}

	void SelectGameObjectsComponent::shutdown()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void SelectGameObjectsComponent::uninstall()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}
	
	void SelectGameObjectsComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool SelectGameObjectsComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
		{
			this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseMultiSelection")
		{
			this->useMultiSelection->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseSelectionRectangle")
		{
			this->useSelectionRectangle->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr SelectGameObjectsComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool SelectGameObjectsComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SelectGameObjectsComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// TODO: What if camera does change?
		this->selectionManager->init(this->gameObjectPtr->getSceneManager(), NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), 
			this->categories->getString(), OIS::MB_Left, new SelectionObserver);


		// If a listener has been added via key/mouse/joystick pressed, a new listener would be inserted during this iteration, which would cause a crash in mouse/key/button release iterator, hence add in next frame
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]() {
			InputDeviceCore::getSingletonPtr()->addKeyListener(this, SelectGameObjectsComponent::getStaticClassName());
			InputDeviceCore::getSingletonPtr()->addMouseListener(this, SelectGameObjectsComponent::getStaticClassName());
			InputDeviceCore::getSingletonPtr()->addJoystickListener(this, SelectGameObjectsComponent::getStaticClassName());
		};
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
		
		return true;
	}

	bool SelectGameObjectsComponent::connect(void)
	{
		GameObjectComponent::connect();

		// In post init not all game objects are known, and so there are maybe no categories yet, so set the categories here
		this->setCategories(this->categories->getString());
		
		return true;
	}

	bool SelectGameObjectsComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		return true;
	}

	bool SelectGameObjectsComponent::onCloned(void)
	{
		
		return true;
	}

	void SelectGameObjectsComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		InputDeviceCore::getSingletonPtr()->removeKeyListener(SelectGameObjectsComponent::getStaticClassName());
		InputDeviceCore::getSingletonPtr()->removeMouseListener(SelectGameObjectsComponent::getStaticClassName());
		InputDeviceCore::getSingletonPtr()->removeJoystickListener(SelectGameObjectsComponent::getStaticClassName());

		if (nullptr != this->selectionManager)
		{
			delete this->selectionManager;
			this->selectionManager = nullptr;
		}
	}
	
	void SelectGameObjectsComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void SelectGameObjectsComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}

	void SelectGameObjectsComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (SelectGameObjectsComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (SelectGameObjectsComponent::AttrCategories() == attribute->getName())
		{
			this->setCategories(attribute->getString());
		}
		else if (SelectGameObjectsComponent::AttrUseMultiSelection() == attribute->getName())
		{
			this->setUseMultiSelection(attribute->getBool());
		}
		else if (SelectGameObjectsComponent::AttrUseSelectionRectangle() == attribute->getName())
		{
			this->setUseSelectionRectangle(attribute->getBool());
		}
	}

	void SelectGameObjectsComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseMultiSelection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useMultiSelection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseSelectionRectangle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useSelectionRectangle->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String SelectGameObjectsComponent::getClassName(void) const
	{
		return "SelectGameObjectsComponent";
	}

	Ogre::String SelectGameObjectsComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void SelectGameObjectsComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool SelectGameObjectsComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void SelectGameObjectsComponent::setCategories(const Ogre::String& categories)
	{
		this->categories->setValue(categories);
		if (nullptr != this->selectionManager)
		{
			this->selectionManager->filterCategories(this->categories->getString());
		}
	}

	Ogre::String SelectGameObjectsComponent::getCategories(void) const
	{
		return this->categories->getString();
	}

	void SelectGameObjectsComponent::setUseMultiSelection(bool useMultiSelection)
	{
		this->useMultiSelection->setValue(useMultiSelection);
	}

	bool SelectGameObjectsComponent::getUseMultiSelection(void) const
	{
		return this->useMultiSelection->getBool();
	}

	void SelectGameObjectsComponent::setUseSelectionRectangle(bool useSelectionRectangle)
	{
		this->useSelectionRectangle->setValue(useSelectionRectangle);
	}

	bool SelectGameObjectsComponent::getUseSelectionRectangle(void) const
	{
		return this->useSelectionRectangle->getBool();
	}

	void SelectGameObjectsComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->bIsInSimulation = !notSimulating;
	}

	bool SelectGameObjectsComponent::keyPressed(const OIS::KeyEvent& keyEventRef)
	{
		if (true == this->bIsInSimulation)
		{
			if (true == this->useMultiSelection->getBool())
			{

				this->selectionManager->handleKeyPress(keyEventRef);
			}
		}
		return true;
	}

	bool SelectGameObjectsComponent::keyReleased(const OIS::KeyEvent& keyEventRef)
	{
		if (true == this->bIsInSimulation)
		{
			if (true == this->useMultiSelection->getBool())
			{
				this->selectionManager->handleKeyRelease(keyEventRef);
			}
		}
		return true;
	}

	bool SelectGameObjectsComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (true == this->bIsInSimulation)
		{
			if (nullptr == MyGUI::InputManager::getInstance().getMouseFocusWidget())
			{
				this->selectionManager->handleMousePress(evt, id);
			}
		}
		return true;
	}

	bool SelectGameObjectsComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (true == this->bIsInSimulation)
		{
			this->selectionManager->handleMouseRelease(evt, id);

			if (nullptr != MyGUI::InputManager::getInstance().getMouseFocusWidget())
			{
				return true;
			}

			if (this->closureFunction.is_valid())
			{
				auto& selectedGameObjects = this->selectionManager->getSelectedGameObjects();

				std::vector<GameObject*> gameObjectList(selectedGameObjects.size());
				size_t i = 0;
				for (auto& selectedGameObject : selectedGameObjects)
				{
					gameObjectList[i] = selectedGameObject.second.gameObject;
					i++;
				}

				try
				{
					luabind::call_function<void>(this->closureFunction, gameObjectList);
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnGameObjectsSelected' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}
		}
		return true;
	}

	bool SelectGameObjectsComponent::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		// TODO: Select via joystick?
		return true;
	}

	bool SelectGameObjectsComponent::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		// TODO: Select via joystick?
		return true;
	}

	bool SelectGameObjectsComponent::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		// TODO: Select via joystick?
		return true;
	}

	bool SelectGameObjectsComponent::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (true == this->bIsInSimulation)
		{
			// this->selectionManager->isSelecting = false;

			if (true == this->useSelectionRectangle->getBool())
			{
				this->selectionManager->handleMouseMove(evt);
			}
		}
		return true;
	}

	void SelectGameObjectsComponent::reactOnGameObjectsSelected(luabind::object closureFunction)
	{
		this->closureFunction = closureFunction;
	}

	void SelectGameObjectsComponent::select(unsigned long gameObjectId, bool bSelect)
	{
		if (nullptr != this->selectionManager)
		{
			this->selectionManager->select(gameObjectId, bSelect);
		}
	}

	// Lua registration part

	SelectGameObjectsComponent* getSelectGameObjectsComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<SelectGameObjectsComponent>(gameObject->getComponentWithOccurrence<SelectGameObjectsComponent>(occurrenceIndex)).get();
	}

	SelectGameObjectsComponent* getSelectGameObjectsComponent(GameObject* gameObject)
	{
		return makeStrongPtr<SelectGameObjectsComponent>(gameObject->getComponent<SelectGameObjectsComponent>()).get();
	}

	SelectGameObjectsComponent* getSelectGameObjectsComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<SelectGameObjectsComponent>(gameObject->getComponentFromName<SelectGameObjectsComponent>(name)).get();
	}

	void internalSelect(SelectGameObjectsComponent* instance, const Ogre::String& gameObjectId, bool bSelect)
	{
		instance->select(Ogre::StringConverter::parseUnsignedLong(gameObjectId), bSelect);
	}

#if 0
	void reactOnGameObjectsSelected(SelectGameObjectsComponent* instance, luabind::object gameObjects)
	{
		std::vector<GameObject*> gameObjects;
		for (luabind::iterator it(gameObjects), end; it != end; ++it)
		{
			luabind::object val = *it;

			if (luabind::type(val) == LUA_TSTRING)
			{
				gameObjects.emplace_back(Ogre::StringConverter::parseUnsignedLong(luabind::object_cast<Ogre::String>(val)));
			}
		}

		instance->snapshotGameObjectsWithUndo(gameObjectIdsULong);
	}
#endif

	void SelectGameObjectsComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<SelectGameObjectsComponent, GameObjectComponent>("SelectGameObjectsComponent")
			.def("setActivated", &SelectGameObjectsComponent::setActivated)
			.def("isActivated", &SelectGameObjectsComponent::isActivated)
			.def("setCategories", &SelectGameObjectsComponent::setCategories)
			.def("getCategories", &SelectGameObjectsComponent::getCategories)
			.def("setUseMultiSelection", &SelectGameObjectsComponent::setUseMultiSelection)
			.def("getUseMultiSelection", &SelectGameObjectsComponent::getUseMultiSelection)
			.def("setUseSelectionRectangle", &SelectGameObjectsComponent::setUseSelectionRectangle)
			.def("getUseSelectionRectangle", &SelectGameObjectsComponent::getUseSelectionRectangle)
			.def("reactOnGameObjectsSelected", &SelectGameObjectsComponent::reactOnGameObjectsSelected)
			.def("select", &internalSelect)
		];

		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "class inherits GameObjectComponent", SelectGameObjectsComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "void setCategories(string categories)", "Sets the categories which shall be selectable. Default is 'All', its also possible to mix categories like: 'Player+Enemy'.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "string getCategories()", "Gets selectable categories.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "void setUseMultiSelection(bool useMultiSelection)", "Sets whether multiple game objects can be selected.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "bool getUseMultiSelection()", "Gets whether multiple game objects can be selected.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "void setUseSelectionRectangle(bool useSelectionRectangle)", "Sets whether use a selection rectangle for multiple game objects selection.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "bool getUseSelectionRectangle()", "Gets whether use a selection rectangle for multiple game objects selection.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "void reactOnGameObjectsSelected(func closureFunction, table[GameObject])",
			"Sets whether to react if one or more game objects are selected.");
		LuaScriptApi::getInstance()->addClassToCollection("SelectGameObjectsComponent", "void select(string gameObjectId, bool bSelect)", "Selects or un-selects the given game object by id.");

		gameObjectClass.def("getSelectGameObjectsComponentFromName", &getSelectGameObjectsComponentFromName);
		gameObjectClass.def("getSelectGameObjectsComponent", (SelectGameObjectsComponent * (*)(GameObject*)) & getSelectGameObjectsComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SelectGameObjectsComponent getSelectGameObjectsComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SelectGameObjectsComponent getSelectGameObjectsComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castSelectGameObjectsComponent", &GameObjectController::cast<SelectGameObjectsComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "SelectGameObjectsComponent castSelectGameObjectsComponent(SelectGameObjectsComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool SelectGameObjectsComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto selectGameObjectsCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<SelectGameObjectsComponent>());
		// Constraints: Can only be placed under a main game object and only once
		if (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID && nullptr == selectGameObjectsCompPtr)
		{
			return true;
		}
		return false;
	}

}; //namespace end