#include "NOWAPrecompiled.h"
#include "MyGUIPadFocusComponent.h"
#include "MyGUIComponents.h"
#include "MyGUIItemBoxComponent.h"
#include "MyGUI_InputManager.h"
#include "gameobject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "modules/InputDeviceModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    MyGUIPadFocusComponent::MyGUIPadFocusComponent() :
        GameObjectComponent(),
        activated(new Variant(MyGUIPadFocusComponent::AttrActivated(), false, this->attributes)),
        navigationDelay(new Variant(MyGUIPadFocusComponent::AttrNavigationDelay(), 0.25f, this->attributes)),
        confirmAction(new Variant(MyGUIPadFocusComponent::AttrConfirmAction(), std::vector<Ogre::String>{"ACTION", "ATTACK_1", "JUMP", "SELECT", "START"}, this->attributes)),
        showPointer(new Variant(MyGUIPadFocusComponent::AttrShowPointer(), true, this->attributes)),
        focusedIndex(-1),
        navigationTimer(0.0f),
        confirmWasDown(false),
        needsRefresh(true),
        isSimulating(false)
    {
        this->activated->setDescription("The pad navigation is a mode and therefore switched off by default. Activate it from lua, e.g. when the player "
                                        "stands at a point of interest, and lock the player movement while it is active.");
        this->navigationDelay->setDescription("Seconds between two focus steps while a direction is held down.");
        this->confirmAction->setDescription("The mapped action which triggers a click on the focused widget.");
        this->showPointer->setDescription("Shows the MyGUI mouse pointer while the pad navigation is active, so that the player sees where the focus is.");

        this->confirmAction->setListSelectedValue("ACTION");
    }

    MyGUIPadFocusComponent::~MyGUIPadFocusComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIPadFocusComponent] Destructor pad focus component for game object: " + this->gameObjectPtr->getName());
    }

    bool MyGUIPadFocusComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NavigationDelay")
        {
            this->navigationDelay->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ConfirmAction")
        {
            this->confirmAction->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowPointer")
        {
            this->showPointer->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    bool MyGUIPadFocusComponent::postInit(void)
    {
        return true;
    }

    bool MyGUIPadFocusComponent::connect(void)
    {
        this->isSimulating = true;

        // The targets are not collected here on purpose. The widgets of the other MyGUI components may not exist yet,
        // depending on the order in which the components are connected, so the collection happens lazily in update().
        this->needsRefresh = true;
        this->focusedIndex = -1;
        this->navigationTimer = 0.0f;
        this->confirmWasDown = false;

        if (true == this->activated->getBool() && true == this->showPointer->getBool())
        {
            Core::getSingletonPtr()->setMyGuiPointerVisible(true);
        }

        return true;
    }

    bool MyGUIPadFocusComponent::disconnect(void)
    {
        this->isSimulating = false;

        this->focusTargets.clear();
        this->focusedIndex = -1;
        this->needsRefresh = true;
        this->confirmWasDown = false;

        return true;
    }

    bool MyGUIPadFocusComponent::onCloned(void)
    {
        return true;
    }

    void MyGUIPadFocusComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating || false == this->isSimulating)
        {
            return;
        }

        if (false == this->activated->getBool())
        {
            return;
        }

        if (true == this->needsRefresh)
        {
            this->needsRefresh = false;
            this->buildFocusTargets();

            if (false == this->focusTargets.empty())
            {
                this->applyFocus(0);
            }
        }

        if (true == this->focusTargets.empty())
        {
            return;
        }

        int directionX = 0;
        int directionY = 0;

        if (true == this->readDirection(directionX, directionY))
        {
            this->navigationTimer -= dt;

            if (this->navigationTimer <= 0.0f)
            {
                const int neighbourIndex = this->findNeighbourTarget(directionX, directionY);
                if (-1 != neighbourIndex)
                {
                    this->applyFocus(neighbourIndex);
                }

                this->navigationTimer = this->navigationDelay->getReal();
            }
        }
        else
        {
            // Released: the next press steps immediately instead of waiting for the delay.
            this->navigationTimer = 0.0f;
        }

        const bool confirmIsDown = this->readConfirm();

        // Only the flank counts, otherwise a held button would fire a click every frame.
        if (true == confirmIsDown && false == this->confirmWasDown)
        {
            this->confirm();
        }

        this->confirmWasDown = confirmIsDown;
    }

    Ogre::String MyGUIPadFocusComponent::getClassName(void) const
    {
        return "MyGUIPadFocusComponent";
    }

    Ogre::String MyGUIPadFocusComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void MyGUIPadFocusComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (MyGUIPadFocusComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (MyGUIPadFocusComponent::AttrNavigationDelay() == attribute->getName())
        {
            this->setNavigationDelay(attribute->getReal());
        }
        else if (MyGUIPadFocusComponent::AttrConfirmAction() == attribute->getName())
        {
            this->setConfirmAction(attribute->getListSelectedValue());
        }
        else if (MyGUIPadFocusComponent::AttrShowPointer() == attribute->getName())
        {
            this->setShowPointer(attribute->getBool());
        }
    }

    void MyGUIPadFocusComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int
        // 6 = real
        // 7 = string
        // 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "NavigationDelay"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->navigationDelay->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ConfirmAction"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->confirmAction->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowPointer"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showPointer->getBool())));
        propertiesXML->append_node(propertyXML);
    }

    GameObjectCompPtr MyGUIPadFocusComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        MyGUIPadFocusCompPtr clonedCompPtr(boost::make_shared<MyGUIPadFocusComponent>());

        clonedCompPtr->setNavigationDelay(this->navigationDelay->getReal());
        clonedCompPtr->setConfirmAction(this->confirmAction->getListSelectedValue());
        clonedCompPtr->setShowPointer(this->showPointer->getBool());
        clonedCompPtr->setActivated(this->activated->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    void MyGUIPadFocusComponent::setActivated(bool activated)
    {
        const bool wasActivated = this->activated->getBool();
        this->activated->setValue(activated);

        if (false == this->isSimulating)
        {
            return;
        }

        if (wasActivated == activated)
        {
            return;
        }

        if (true == activated)
        {
            // Collected in the next update(), because a widget which was just shown by the script may not have its
            // final coordinates yet in this very moment.
            this->needsRefresh = true;
            this->navigationTimer = 0.0f;
            this->confirmWasDown = false;

            if (true == this->showPointer->getBool())
            {
                Core::getSingletonPtr()->setMyGuiPointerVisible(true);
            }
            return;
        }

        this->focusedIndex = -1;
        this->focusTargets.clear();

        if (true == this->showPointer->getBool())
        {
            Core::getSingletonPtr()->setMyGuiPointerVisible(false);
        }
    }

    bool MyGUIPadFocusComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void MyGUIPadFocusComponent::setNavigationDelay(Ogre::Real navigationDelay)
    {
        this->navigationDelay->setValue(navigationDelay);
    }

    Ogre::Real MyGUIPadFocusComponent::getNavigationDelay(void) const
    {
        return this->navigationDelay->getReal();
    }

    void MyGUIPadFocusComponent::setConfirmAction(const Ogre::String& confirmAction)
    {
        this->confirmAction->setListSelectedValue(confirmAction);
    }

    Ogre::String MyGUIPadFocusComponent::getConfirmAction(void) const
    {
        return this->confirmAction->getListSelectedValue();
    }

    void MyGUIPadFocusComponent::setShowPointer(bool showPointer)
    {
        this->showPointer->setValue(showPointer);
    }

    bool MyGUIPadFocusComponent::getShowPointer(void) const
    {
        return this->showPointer->getBool();
    }

    void MyGUIPadFocusComponent::refreshFocusTargets(void)
    {
        this->needsRefresh = true;
    }

    void MyGUIPadFocusComponent::buildFocusTargets(void)
    {
        this->focusTargets.clear();
        this->focusedIndex = -1;

        const std::vector<GameObjectPtr>& gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsList();

        // Resolved on the logic thread, so that the render command below only has to read MyGUI coordinates. The raw
        // pointers are valid for the duration of the enqueueAndWait, because the game objects are alive in this scope.
        std::vector<std::pair<unsigned long, MyGUIComponent*>> widgetComponents;
        std::vector<std::pair<unsigned long, MyGUIItemBoxComponent*>> itemBoxComponents;

        for (size_t i = 0; i < gameObjects.size(); i++)
        {
            GameObjectPtr gameObjectPtr = gameObjects[i];
            if (nullptr == gameObjectPtr)
            {
                continue;
            }

            const unsigned long gameObjectId = gameObjectPtr->getId();

            // Attention: the components are walked directly instead of getComponent<MyGUIComponent>(), because that
            // lookup matches the concrete class id - a MyGUIButtonComponent would not be found as MyGUIComponent.
            // Walking them also catches the case that a whole menu sits as a pile of components on ONE game object,
            // which is why the game object itself plays no role in the selection: only isFocusable() does.
            GameObjectComponents* gameObjectComponents = gameObjectPtr->getComponents();

            for (auto it = gameObjectComponents->cbegin(); it != gameObjectComponents->cend(); ++it)
            {
                GameObjectCompPtr gameObjectCompPtr = std::get<COMPONENT>(*it);

                MyGUIComponent* myGuiComponent = dynamic_cast<MyGUIComponent*>(gameObjectCompPtr.get());
                if (nullptr == myGuiComponent)
                {
                    continue;
                }

                if (false == myGuiComponent->isFocusable())
                {
                    continue;
                }

                // An item box is not one target but one per slot, so it is kept apart.
                MyGUIItemBoxComponent* itemBoxComponent = dynamic_cast<MyGUIItemBoxComponent*>(myGuiComponent);
                if (nullptr != itemBoxComponent)
                {
                    itemBoxComponents.emplace_back(gameObjectId, itemBoxComponent);
                    continue;
                }

                widgetComponents.emplace_back(gameObjectId, myGuiComponent);
            }
        }

        if (true == widgetComponents.empty() && true == itemBoxComponents.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIPadFocusComponent] Warning: No focus target found. Switch the 'Focusable' "
                                                                                "property on for the MyGUI components which shall be reachable with the gamepad.");
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, widgetComponents, itemBoxComponents]()
        {
            for (size_t i = 0; i < widgetComponents.size(); i++)
            {
                MyGUI::Widget* widget = widgetComponents[i].second->getWidget();
                if (nullptr == widget)
                {
                    continue;
                }

                // A hidden widget must not swallow the focus. getInheritedVisible also covers a hidden parent window.
                if (false == widget->getInheritedVisible())
                {
                    continue;
                }

                const MyGUI::IntCoord coord = widget->getAbsoluteCoord();

                FocusTarget focusTarget;
                focusTarget.gameObjectId = widgetComponents[i].first;
                focusTarget.slotIndex = -1;
                focusTarget.centerX = coord.left + coord.width / 2;
                focusTarget.centerY = coord.top + coord.height / 2;

                this->focusTargets.emplace_back(focusTarget);
            }

            for (size_t i = 0; i < itemBoxComponents.size(); i++)
            {
                MyGUI::ItemBox* itemBox = itemBoxComponents[i].second->getItemBoxWidget();
                if (nullptr == itemBox)
                {
                    continue;
                }

                if (false == itemBox->getInheritedVisible())
                {
                    continue;
                }

                const size_t itemCount = itemBox->getItemCount();

                for (size_t slot = 0; slot < itemCount; slot++)
                {
                    MyGUI::Widget* cellWidget = itemBox->getWidgetByIndex(slot);
                    if (nullptr == cellWidget)
                    {
                        continue;
                    }

                    const MyGUI::IntCoord coord = cellWidget->getAbsoluteCoord();

                    FocusTarget focusTarget;
                    focusTarget.gameObjectId = itemBoxComponents[i].first;
                    focusTarget.slotIndex = static_cast<int>(slot);
                    focusTarget.centerX = coord.left + coord.width / 2;
                    focusTarget.centerY = coord.top + coord.height / 2;

                    this->focusTargets.emplace_back(focusTarget);
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIPadFocusComponent::buildFocusTargets");
    }

    void MyGUIPadFocusComponent::applyFocus(int targetIndex)
    {
        if (0 > targetIndex || targetIndex >= static_cast<int>(this->focusTargets.size()))
        {
            return;
        }

        this->focusedIndex = targetIndex;

        const FocusTarget& focusTarget = this->focusTargets[this->focusedIndex];

        // This is the whole trick: the mouse is snapped onto the widget. MyGUI then draws its hover state itself and a
        // confirm press becomes a real click at this position. setMousePosition moves the OS cursor, synchronizes the
        // OIS absolute position and injects the MyGUI mouse move on the render thread.
        InputDeviceCore::getSingletonPtr()->setMousePosition(focusTarget.centerX, focusTarget.centerY);

        if (luabind::type(this->focusChangedClosureFunction) != LUA_TFUNCTION)
        {
            return;
        }

        try
        {
            luabind::call_function<void>(this->focusChangedClosureFunction, Ogre::StringConverter::toString(focusTarget.gameObjectId), focusTarget.slotIndex);
        }
        catch (luabind::error& error)
        {
            luabind::object errorMsg(luabind::from_stack(error.state(), -1));
            std::stringstream msg;
            msg << errorMsg;
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIPadFocusComponent] Caught error in 'reactOnFocusChanged' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
        }
    }

    int MyGUIPadFocusComponent::findNeighbourTarget(int directionX, int directionY) const
    {
        if (true == this->focusTargets.empty())
        {
            return -1;
        }

        if (-1 == this->focusedIndex)
        {
            return 0;
        }

        const FocusTarget& currentTarget = this->focusTargets[this->focusedIndex];

        int bestIndex = -1;
        int bestScore = 0;

        for (size_t i = 0; i < this->focusTargets.size(); i++)
        {
            if (static_cast<int>(i) == this->focusedIndex)
            {
                continue;
            }

            const int deltaX = this->focusTargets[i].centerX - currentTarget.centerX;
            const int deltaY = this->focusTargets[i].centerY - currentTarget.centerY;

            // Distance along the pressed direction and sideways to it. Screen coordinates, so y grows downwards.
            const int along = deltaX * directionX + deltaY * directionY;
            const int sideways = std::abs(deltaX * directionY - deltaY * directionX);

            if (0 >= along)
            {
                continue;
            }

            // 45 degree cone: a widget which lies more to the side than ahead belongs to another row or column.
            if (sideways > along)
            {
                continue;
            }

            // Sideways offsets weigh double, so the straight neighbour wins against a slightly nearer diagonal one.
            const int score = along + 2 * sideways;

            if (-1 == bestIndex || score < bestScore)
            {
                bestIndex = static_cast<int>(i);
                bestScore = score;
            }
        }

        return bestIndex;
    }

    bool MyGUIPadFocusComponent::readDirection(int& directionX, int& directionY)
    {
        directionX = 0;
        directionY = 0;

        // Attention: only the state getters are used here, never isActionPressed(). The timed variants rely on
        // InputDeviceModule::update(dt), and that is called for the main keyboard module only. The repeat rate is
        // therefore handled by this component itself, see 'Navigation Delay'.
        InputDeviceModule* keyboardModule = InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule();
        if (nullptr != keyboardModule)
        {
            if (true == keyboardModule->isActionDown(InputDeviceModule::LEFT))
            {
                directionX = -1;
            }
            else if (true == keyboardModule->isActionDown(InputDeviceModule::RIGHT))
            {
                directionX = 1;
            }

            // Screen coordinates: up is the smaller y.
            if (true == keyboardModule->isActionDown(InputDeviceModule::UP))
            {
                directionY = -1;
            }
            else if (true == keyboardModule->isActionDown(InputDeviceModule::DOWN))
            {
                directionY = 1;
            }
        }

        std::vector<InputDeviceModule*> joystickModules = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModules();

        for (size_t i = 0; i < joystickModules.size(); i++)
        {
            InputDeviceModule* joystickModule = joystickModules[i];
            if (nullptr == joystickModule)
            {
                continue;
            }

            if (0 == directionX)
            {
                if (true == joystickModule->isActionDown(InputDeviceModule::LEFT))
                {
                    directionX = -1;
                }
                else if (true == joystickModule->isActionDown(InputDeviceModule::RIGHT))
                {
                    directionX = 1;
                }
            }

            if (0 == directionY)
            {
                if (true == joystickModule->isActionDown(InputDeviceModule::UP))
                {
                    directionY = -1;
                }
                else if (true == joystickModule->isActionDown(InputDeviceModule::DOWN))
                {
                    directionY = 1;
                }
            }
        }

        return (0 != directionX || 0 != directionY);
    }

    bool MyGUIPadFocusComponent::readConfirm(void)
    {
        const InputDeviceModule::Action action = static_cast<InputDeviceModule::Action>(this->getConfirmActionValue());

        InputDeviceModule* keyboardModule = InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule();
        if (nullptr != keyboardModule && true == keyboardModule->isActionDown(action))
        {
            return true;
        }

        std::vector<InputDeviceModule*> joystickModules = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModules();

        for (size_t i = 0; i < joystickModules.size(); i++)
        {
            if (nullptr == joystickModules[i])
            {
                continue;
            }

            if (true == joystickModules[i]->isActionDown(action))
            {
                return true;
            }
        }

        return false;
    }

    int MyGUIPadFocusComponent::getConfirmActionValue(void) const
    {
        const Ogre::String selectedAction = this->confirmAction->getListSelectedValue();

        if ("ATTACK_1" == selectedAction)
        {
            return static_cast<int>(InputDeviceModule::ATTACK_1);
        }
        if ("JUMP" == selectedAction)
        {
            return static_cast<int>(InputDeviceModule::JUMP);
        }
        if ("SELECT" == selectedAction)
        {
            return static_cast<int>(InputDeviceModule::SELECT);
        }
        if ("START" == selectedAction)
        {
            return static_cast<int>(InputDeviceModule::START);
        }

        return static_cast<int>(InputDeviceModule::ACTION);
    }

    void MyGUIPadFocusComponent::focusNext(void)
    {
        if (true == this->focusTargets.empty())
        {
            return;
        }

        int nextIndex = this->focusedIndex + 1;
        if (nextIndex >= static_cast<int>(this->focusTargets.size()))
        {
            nextIndex = 0;
        }

        this->applyFocus(nextIndex);
    }

    void MyGUIPadFocusComponent::focusPrevious(void)
    {
        if (true == this->focusTargets.empty())
        {
            return;
        }

        int previousIndex = this->focusedIndex - 1;
        if (0 > previousIndex)
        {
            previousIndex = static_cast<int>(this->focusTargets.size()) - 1;
        }

        this->applyFocus(previousIndex);
    }

    void MyGUIPadFocusComponent::confirm(void)
    {
        if (-1 == this->focusedIndex || this->focusedIndex >= static_cast<int>(this->focusTargets.size()))
        {
            return;
        }

        const int positionX = this->focusTargets[this->focusedIndex].centerX;
        const int positionY = this->focusTargets[this->focusedIndex].centerY;

        // Fire and forget, and only plain values are captured. The mouse move is repeated here, so that MyGUI surely
        // has its mouse focus on this widget, even if the snap of applyFocus was processed in an earlier frame.
        NOWA::GraphicsModule::RenderCommand renderCommand = [positionX, positionY]()
        {
            if (nullptr == MyGUI::InputManager::getInstancePtr())
            {
                return;
            }

            MyGUI::InputManager::getInstancePtr()->injectMouseMove(positionX, positionY, 0);
            MyGUI::InputManager::getInstancePtr()->injectMousePress(positionX, positionY, MyGUI::MouseButton::Left);
            MyGUI::InputManager::getInstancePtr()->injectMouseRelease(positionX, positionY, MyGUI::MouseButton::Left);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MyGUIPadFocusComponent::confirm");
    }

    unsigned long MyGUIPadFocusComponent::getFocusedGameObjectId(void) const
    {
        if (-1 == this->focusedIndex || this->focusedIndex >= static_cast<int>(this->focusTargets.size()))
        {
            return 0;
        }

        return this->focusTargets[this->focusedIndex].gameObjectId;
    }

    int MyGUIPadFocusComponent::getFocusedSlotIndex(void) const
    {
        if (-1 == this->focusedIndex || this->focusedIndex >= static_cast<int>(this->focusTargets.size()))
        {
            return -1;
        }

        return this->focusTargets[this->focusedIndex].slotIndex;
    }

    void MyGUIPadFocusComponent::reactOnFocusChanged(luabind::object closureFunction)
    {
        if (luabind::type(closureFunction) != LUA_TFUNCTION)
        {
            return;
        }

        this->focusChangedClosureFunction = closureFunction;
    }

    MyGUIPadFocusComponent* getMyGUIPadFocusComponentComponent(GameObject* gameObject)
    {
        return makeStrongPtr<MyGUIPadFocusComponent>(gameObject->getComponent<MyGUIPadFocusComponent>()).get();
    }

    MyGUIPadFocusComponent* getMyGUIPadFocusComponentComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<MyGUIPadFocusComponent>(gameObject->getComponentFromName<MyGUIPadFocusComponent>(name)).get();
    }

    Ogre::String getFocusedGameObjectIdString(MyGUIPadFocusComponent* instance)
    {
        return Ogre::StringConverter::toString(instance->getFocusedGameObjectId());
    }

    void MyGUIPadFocusComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<MyGUIPadFocusComponent, GameObjectComponent>("MyGUIPadFocusComponent")
                .def("setActivated", &MyGUIPadFocusComponent::setActivated)
                .def("isActivated", &MyGUIPadFocusComponent::isActivated)
                .def("refreshFocusTargets", &MyGUIPadFocusComponent::refreshFocusTargets)
                .def("focusNext", &MyGUIPadFocusComponent::focusNext)
                .def("focusPrevious", &MyGUIPadFocusComponent::focusPrevious)
                .def("confirm", &MyGUIPadFocusComponent::confirm)
                .def("getFocusedGameObjectId", &getFocusedGameObjectIdString)
                .def("getFocusedSlotIndex", &MyGUIPadFocusComponent::getFocusedSlotIndex)
                .def("reactOnFocusChanged", &MyGUIPadFocusComponent::reactOnFocusChanged)];

        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "class inherits GameObjectComponent", MyGUIPadFocusComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "void setActivated(bool activated)",
            "Switches the pad navigation on or off. Lock the player movement while it is on, because it uses the same UP / DOWN / LEFT / RIGHT actions.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "bool isActivated()", "Gets whether the pad navigation is active.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "void refreshFocusTargets()",
            "Collects the focus targets again. Call it whenever widgets appeared, vanished or moved, e.g. after the inventory received its first item.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "void focusNext()", "Steps the focus to the next target, e.g. for a shoulder button.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "void focusPrevious()", "Steps the focus to the previous target, e.g. for a shoulder button.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "void confirm()", "Clicks the focused widget, exactly as a real mouse click would.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "string getFocusedGameObjectId()", "Gets the id of the game object the focused widget belongs to, or '0', if nothing is focused.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "int getFocusedSlotIndex()", "Gets the inventory slot index of the focused target, or -1, if the focused widget is no inventory slot.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIPadFocusComponent", "void reactOnFocusChanged(func closureFunction, string gameObjectId, int slotIndex)",
            "Sets whether to react if the focus moved to another widget. E.g. getMyGUIPadFocusComponent():reactOnFocusChanged(function(gameObjectId, slotIndex) ... end)");

        gameObjectClass.def("getMyGUIPadFocusComponent", &getMyGUIPadFocusComponentComponent);
        gameObjectClass.def("getMyGUIPadFocusComponentFromName", &getMyGUIPadFocusComponentComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MyGUIPadFocusComponent getMyGUIPadFocusComponent()", "Gets the MyGUI pad focus component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MyGUIPadFocusComponent getMyGUIPadFocusComponentFromName(string name)", "Gets the MyGUI pad focus component by name.");

        gameObjectControllerClass.def("castMyGUIPadFocusComponent", &GameObjectController::cast<MyGUIPadFocusComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MyGUIPadFocusComponent castMyGUIPadFocusComponent(MyGUIPadFocusComponent other)", "Casts for Lua auto completion.");
    }

}; // namespace end