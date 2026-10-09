#include "NOWAPrecompiled.h"
#include "VirtualKeyboardComponent.h"
#include "gameobject/MyGUIPadFocusComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/MyGUIComponents.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "modules/InputDeviceModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"

namespace
{
    // Number of UTF-8 characters (not bytes). Continuation bytes (10xxxxxx) do not start a character.
    size_t getUtf8Length(const Ogre::String& text)
    {
        size_t count = 0;
        for (size_t i = 0; i < text.size(); i++)
        {
            if (0x80 != (static_cast<unsigned char>(text[i]) & 0xC0))
            {
                count++;
            }
        }
        return count;
    }

    // Cuts the text to at most maxLength UTF-8 characters, never in the middle of a multi byte character.
    Ogre::String truncateUtf8(const Ogre::String& text, size_t maxLength)
    {
        size_t count = 0;
        for (size_t i = 0; i < text.size(); i++)
        {
            if (0x80 != (static_cast<unsigned char>(text[i]) & 0xC0))
            {
                if (count == maxLength)
                {
                    return text.substr(0, i);
                }
                count++;
            }
        }
        return text;
    }

    void removeLastUtf8Character(Ogre::String& text)
    {
        if (true == text.empty())
        {
            return;
        }

        size_t position = text.size() - 1;
        while (position > 0 && 0x80 == (static_cast<unsigned char>(text[position]) & 0xC0))
        {
            position--;
        }
        text.erase(position);
    }

    NOWA::VirtualKeyboardComponent::KeyDefinition makeKey(NOWA::VirtualKeyboardComponent::KeyType type, const Ogre::String& lower, const Ogre::String& upper, Ogre::Real weight)
    {
        NOWA::VirtualKeyboardComponent::KeyDefinition keyDefinition;
        keyDefinition.type = type;
        keyDefinition.lower = lower;
        keyDefinition.upper = upper;
        keyDefinition.weight = weight;
        return keyDefinition;
    }

    // One character key per ASCII letter of the given string
    void addLetterKeys(std::vector<NOWA::VirtualKeyboardComponent::KeyDefinition>& row, const Ogre::String& letters)
    {
        for (size_t i = 0; i < letters.size(); i++)
        {
            const Ogre::String lower(1, letters[i]);
            const Ogre::String upper(1, static_cast<char>(std::toupper(static_cast<unsigned char>(letters[i]))));
            row.push_back(makeKey(NOWA::VirtualKeyboardComponent::KEY_CHARACTER, lower, upper, 1.0f));
        }
    }
} // namespace

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    VirtualKeyboardComponent::VirtualKeyboardComponent() :
        GameObjectComponent(),
        name("VirtualKeyboardComponent"),
        isSimulating(false),
        widgetsCreated(false),
        window(nullptr),
        cursorIndex(0),
        shiftActive(false),
        capsLock(false),
        navigationTimer(0.0f),
        confirmWasDown(false),
        backspaceWasDown(false),
        shiftWasDown(false),
        spaceWasDown(false),
        acceptWasDown(false),
        cancelWasDown(false),
        activated(new Variant(VirtualKeyboardComponent::AttrActivated(), false, this->attributes)),
        // Same flag as the parent id of the JoystickConfigurationComponent, so that the editor offers the id selection
        targetId(new Variant(VirtualKeyboardComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
        layout(new Variant(VirtualKeyboardComponent::AttrLayout(), std::vector<Ogre::String>{"QWERTZ", "QWERTY"}, this->attributes)),
        maxLength(new Variant(VirtualKeyboardComponent::AttrMaxLength(), static_cast<unsigned long>(16), this->attributes)),
        relativePosition(new Variant(VirtualKeyboardComponent::AttrRelativePosition(), Ogre::Vector2(0.22f, 0.54f), this->attributes)),
        relativeSize(new Variant(VirtualKeyboardComponent::AttrRelativeSize(), Ogre::Vector2(0.56f, 0.42f), this->attributes)),
        navigationDelay(new Variant(VirtualKeyboardComponent::AttrNavigationDelay(), 0.2f, this->attributes)),
        showUmlauts(new Variant(VirtualKeyboardComponent::AttrShowUmlauts(), true, this->attributes)),
        autoCapitalize(new Variant(VirtualKeyboardComponent::AttrAutoCapitalize(), true, this->attributes)),
        suspendPadFocusVariant(new Variant(VirtualKeyboardComponent::AttrSuspendPadFocus(), true, this->attributes))
    {
        this->activated->setDescription("Opens the keyboard. It is a mode and therefore switched off by default. Open it from lua, e.g. with setActivated(true), "
                                        "and lock the player movement while it is open.");
        this->targetId->setDescription("The id of the game object or of the MyGUI component whose text widget receives the typed text, e.g. of a MyGUITextComponent.");
        this->layout->setDescription("The key layout: QWERTZ (german) or QWERTY. Takes effect at the next open.");
        this->maxLength->setDescription("The maximum count of characters, e.g. 16 for a player name.");
        this->relativePosition->setDescription("The position of the keyboard window (0 = top/left, 1 = bottom/right).");
        this->relativeSize->setDescription("The maximum size of the keyboard window as fraction of the screen. The keyboard fits into this box with a fixed key proportion, so it is never distorted or cut off.");
        this->navigationDelay->setDescription("Seconds between two cursor steps while a direction is held down.");
        this->showUmlauts->setDescription("Adds a row with the keys ae, oe, ue and sz (umlauts). Takes effect at the next open.");
        this->autoCapitalize->setDescription("Switches shift on automatically at the start of the text and after a space, e.g. for names.");
        this->suspendPadFocusVariant->setDescription("Switches active MyGUIPadFocusComponents off while the keyboard is open, because both would react on the same directions "
                                                     "and the same confirm button. They are switched on again when the keyboard has been closed.");

        this->layout->setListSelectedValue("QWERTZ");
    }

    bool VirtualKeyboardComponent::isLuaFunction(const luabind::object& closureFunction)
    {
        // Attention: A luabind::object which was never assigned has no interpreter (m_interpreter == nullptr). luabind::type() would push it onto a null lua state and crash.
        if (false == closureFunction.is_valid())
        {
            return false;
        }
        return LUA_TFUNCTION == luabind::type(closureFunction);
    }

    VirtualKeyboardComponent::~VirtualKeyboardComponent()
    {
        if (nullptr != this->window)
        {
            MyGUI::Gui::getInstancePtr()->destroyWidget(this->window);
            this->window = nullptr;
        }
    }

    void VirtualKeyboardComponent::initialise()
    {
    }

    const Ogre::String& VirtualKeyboardComponent::getName() const
    {
        return this->name;
    }

    void VirtualKeyboardComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<VirtualKeyboardComponent>(VirtualKeyboardComponent::getStaticClassId(), VirtualKeyboardComponent::getStaticClassName());
    }

    void VirtualKeyboardComponent::shutdown()
    {
    }

    void VirtualKeyboardComponent::uninstall()
    {
    }

    void VirtualKeyboardComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool VirtualKeyboardComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
        {
            this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Layout")
        {
            this->layout->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxLength")
        {
            this->maxLength->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RelativePosition")
        {
            this->relativePosition->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RelativeSize")
        {
            this->relativeSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NavigationDelay")
        {
            this->navigationDelay->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowUmlauts")
        {
            this->showUmlauts->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoCapitalize")
        {
            this->autoCapitalize->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SuspendPadFocus")
        {
            this->suspendPadFocusVariant->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr VirtualKeyboardComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        VirtualKeyboardComponentPtr clonedCompPtr(boost::make_shared<VirtualKeyboardComponent>());

        clonedCompPtr->setTargetId(this->targetId->getULong());
        clonedCompPtr->setLayout(this->layout->getListSelectedValue());
        clonedCompPtr->setMaxLength(this->maxLength->getULong());
        clonedCompPtr->setRelativePosition(this->relativePosition->getVector2());
        clonedCompPtr->setRelativeSize(this->relativeSize->getVector2());
        clonedCompPtr->setNavigationDelay(this->navigationDelay->getReal());
        clonedCompPtr->setShowUmlauts(this->showUmlauts->getBool());
        clonedCompPtr->setAutoCapitalize(this->autoCapitalize->getBool());
        clonedCompPtr->setSuspendPadFocus(this->suspendPadFocusVariant->getBool());
        // Never cloned as open: the keyboard is a mode, which a script switches on
        clonedCompPtr->activated->setValue(false);

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool VirtualKeyboardComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VirtualKeyboardComponent] Init component for game object: " + this->gameObjectPtr->getName());

        return true;
    }

    bool VirtualKeyboardComponent::connect(void)
    {
        GameObjectComponent::connect();

        this->isSimulating = true;

        // Widgets of other MyGUI components may not exist yet, depending on the connect order, so the target is resolved in open() and not earlier.
        if (true == this->activated->getBool())
        {
            this->open();
        }

        return true;
    }

    bool VirtualKeyboardComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        // The value set in the editor must survive the simulation, closeKeyboard() resets it
        const bool wasActivated = this->activated->getBool();
        this->closeKeyboard(false, false);
        this->activated->setValue(wasActivated);
        this->isSimulating = false;

        // The lua closures belong to the lua state of this simulation run. Keeping them would leave dangling references, when the state is destroyed.
        this->textChangedClosureFunction = luabind::object();
        this->closedClosureFunction = luabind::object();

        return true;
    }

    bool VirtualKeyboardComponent::onCloned(void)
    {
        return true;
    }

    void VirtualKeyboardComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        this->closeKeyboard(false, false);

        this->textChangedClosureFunction = luabind::object();
        this->closedClosureFunction = luabind::object();
    }

    Ogre::String VirtualKeyboardComponent::getClassName(void) const
    {
        return "VirtualKeyboardComponent";
    }

    Ogre::String VirtualKeyboardComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void VirtualKeyboardComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (VirtualKeyboardComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (VirtualKeyboardComponent::AttrTargetId() == attribute->getName())
        {
            this->setTargetId(attribute->getULong());
        }
        else if (VirtualKeyboardComponent::AttrLayout() == attribute->getName())
        {
            this->setLayout(attribute->getListSelectedValue());
        }
        else if (VirtualKeyboardComponent::AttrMaxLength() == attribute->getName())
        {
            this->setMaxLength(attribute->getULong());
        }
        else if (VirtualKeyboardComponent::AttrRelativePosition() == attribute->getName())
        {
            this->setRelativePosition(attribute->getVector2());
        }
        else if (VirtualKeyboardComponent::AttrRelativeSize() == attribute->getName())
        {
            this->setRelativeSize(attribute->getVector2());
        }
        else if (VirtualKeyboardComponent::AttrNavigationDelay() == attribute->getName())
        {
            this->setNavigationDelay(attribute->getReal());
        }
        else if (VirtualKeyboardComponent::AttrShowUmlauts() == attribute->getName())
        {
            this->setShowUmlauts(attribute->getBool());
        }
        else if (VirtualKeyboardComponent::AttrAutoCapitalize() == attribute->getName())
        {
            this->setAutoCapitalize(attribute->getBool());
        }
        else if (VirtualKeyboardComponent::AttrSuspendPadFocus() == attribute->getName())
        {
            this->setSuspendPadFocus(attribute->getBool());
        }
    }

    void VirtualKeyboardComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int
        // 6 = real
        // 7 = string
        // 8 = vector2
        // 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Layout"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->layout->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MaxLength"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxLength->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RelativePosition"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->relativePosition->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RelativeSize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->relativeSize->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "NavigationDelay"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->navigationDelay->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowUmlauts"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showUmlauts->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "AutoCapitalize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoCapitalize->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SuspendPadFocus"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->suspendPadFocusVariant->getBool())));
        propertiesXML->append_node(propertyXML);
    }

    // =============================================================================
    // Open / close
    // =============================================================================

    void VirtualKeyboardComponent::setActivated(bool activated)
    {
        if (true == activated)
        {
            this->open();
        }
        else
        {
            this->accept();
        }
    }

    bool VirtualKeyboardComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void VirtualKeyboardComponent::open(void)
    {
        this->activated->setValue(true);

        // In the editor (not simulating) the value is only stored, connect() opens the keyboard when the simulation starts.
        if (false == this->isSimulating || true == this->widgetsCreated)
        {
            return;
        }

        this->resolveTarget();
        this->buildLayout();

        this->shiftActive = false;
        this->capsLock = false;
        this->cursorIndex = 0;

        Ogre::String initialText;
        const GameObjectCompPtr target = this->targetComponent;
        const Ogre::Vector2 position = this->relativePosition->getVector2();
        const Ogre::Vector2 size = this->relativeSize->getVector2();

        // Attention: This runs on the logic thread, so waiting for the render thread is fine. The MyGUI click callback (render thread) must never wait,
        // therefore it only enqueues a logic command, see notifyKeyClicked.
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, target, position, size, &initialText]()
        {
            this->createWidgets(position, size);

            // The text which the target widget shows right now is the start text, so that e.g. a default name can be edited
            if (nullptr != target)
            {
                MyGUIComponent* myGuiComponent = dynamic_cast<MyGUIComponent*>(target.get());
                if (nullptr != myGuiComponent && nullptr != myGuiComponent->getWidget())
                {
                    MyGUI::TextBox* textBox = myGuiComponent->getWidget()->castType<MyGUI::TextBox>(false);
                    if (nullptr != textBox)
                    {
                        initialText = textBox->getCaption().asUTF8();
                    }
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "VirtualKeyboardComponent::open");

        this->widgetsCreated = true;

        this->originalText = initialText;
        this->text = truncateUtf8(initialText, static_cast<size_t>(this->maxLength->getULong()));

        // The press which opened the keyboard (e.g. the same button) must not be taken as the first key press
        this->confirmWasDown = this->isActionDownAnywhere(InputDeviceModule::JUMP);
        this->backspaceWasDown = this->isActionDownAnywhere(InputDeviceModule::ACTION);
        this->shiftWasDown = this->isActionDownAnywhere(InputDeviceModule::ATTACK_1);
        this->spaceWasDown = this->isActionDownAnywhere(InputDeviceModule::ATTACK_2);
        this->acceptWasDown = this->isActionDownAnywhere(InputDeviceModule::START);
        this->cancelWasDown = this->isActionDownAnywhere(InputDeviceModule::INVENTORY);
        this->navigationTimer = this->navigationDelay->getReal();

        this->applyAutoCapitalize();

        if (true == this->suspendPadFocusVariant->getBool())
        {
            this->suspendPadFocus();
        }

        this->refreshVisuals(false);
    }

    void VirtualKeyboardComponent::accept(void)
    {
        this->closeKeyboard(true, true);
    }

    void VirtualKeyboardComponent::cancel(void)
    {
        this->closeKeyboard(false, true);
    }

    void VirtualKeyboardComponent::closeKeyboard(bool accepted, bool notifyLua)
    {
        this->activated->setValue(false);

        if (false == this->widgetsCreated)
        {
            return;
        }

        if (false == accepted)
        {
            // Restores the text of the target. Enqueued before the destroy command below, so it is processed first.
            this->text = this->originalText;
            this->refreshVisuals(true);
        }

        const Ogre::String finalText = this->text;

        this->widgetsCreated = false;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->destroyWidgets();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "VirtualKeyboardComponent::close");

        this->keyEntries.clear();
        this->keyRows.clear();
        this->targetComponent.reset();

        this->resumePadFocus();

        if (false == notifyLua || false == VirtualKeyboardComponent::isLuaFunction(this->closedClosureFunction))
        {
            return;
        }

        try
        {
            luabind::call_function<void>(this->closedClosureFunction, accepted, finalText);
        }
        catch (luabind::error& error)
        {
            luabind::object errorMsg(luabind::from_stack(error.state(), -1));
            std::stringstream msg;
            msg << errorMsg;
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[VirtualKeyboardComponent] Caught error in 'reactOnClosed' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
        }
    }

    // =============================================================================
    // Target and pad focus
    // =============================================================================

    void VirtualKeyboardComponent::resolveTarget(void)
    {
        this->targetComponent.reset();

        const unsigned long id = this->targetId->getULong();
        if (0 == id)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VirtualKeyboardComponent] Warning: No 'Target Id' set, the typed text is only reported via lua.");
            return;
        }

        const std::vector<GameObjectPtr>& gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsList();

        // An exact MyGUI component id wins. A game object id is the fallback and takes the first MyGUI component of that game object.
        GameObjectCompPtr fallbackComponent;

        for (size_t i = 0; i < gameObjects.size(); i++)
        {
            GameObjectPtr gameObjectPtr = gameObjects[i];
            if (nullptr == gameObjectPtr)
            {
                continue;
            }

            GameObjectComponents* gameObjectComponents = gameObjectPtr->getComponents();

            for (auto it = gameObjectComponents->cbegin(); it != gameObjectComponents->cend(); ++it)
            {
                GameObjectCompPtr gameObjectCompPtr = std::get<COMPONENT>(*it);

                MyGUIComponent* myGuiComponent = dynamic_cast<MyGUIComponent*>(gameObjectCompPtr.get());
                if (nullptr == myGuiComponent)
                {
                    continue;
                }

                if (id == myGuiComponent->getId())
                {
                    this->targetComponent = gameObjectCompPtr;
                    return;
                }

                if (id == gameObjectPtr->getId() && nullptr == fallbackComponent)
                {
                    fallbackComponent = gameObjectCompPtr;
                }
            }
        }

        this->targetComponent = fallbackComponent;

        if (nullptr == this->targetComponent)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VirtualKeyboardComponent] Warning: No MyGUI component found for 'Target Id' " + Ogre::StringConverter::toString(id) + ".");
        }
    }

    void VirtualKeyboardComponent::suspendPadFocus(void)
    {
        this->suspendedPadFocusGameObjectIds.clear();

        const std::vector<GameObjectPtr>& gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsList();

        for (size_t i = 0; i < gameObjects.size(); i++)
        {
            GameObjectPtr gameObjectPtr = gameObjects[i];
            if (nullptr == gameObjectPtr)
            {
                continue;
            }

            MyGUIPadFocusComponent* padFocusComponent = makeStrongPtr<MyGUIPadFocusComponent>(gameObjectPtr->getComponent<MyGUIPadFocusComponent>()).get();
            if (nullptr != padFocusComponent && true == padFocusComponent->isActivated())
            {
                padFocusComponent->setActivated(false);
                this->suspendedPadFocusGameObjectIds.push_back(gameObjectPtr->getId());
            }
        }
    }

    void VirtualKeyboardComponent::resumePadFocus(void)
    {
        if (true == this->suspendedPadFocusGameObjectIds.empty())
        {
            return;
        }

        const std::vector<GameObjectPtr>& gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsList();

        for (size_t i = 0; i < gameObjects.size(); i++)
        {
            GameObjectPtr gameObjectPtr = gameObjects[i];
            if (nullptr == gameObjectPtr)
            {
                continue;
            }

            if (this->suspendedPadFocusGameObjectIds.end() == std::find(this->suspendedPadFocusGameObjectIds.begin(), this->suspendedPadFocusGameObjectIds.end(), gameObjectPtr->getId()))
            {
                continue;
            }

            MyGUIPadFocusComponent* padFocusComponent = makeStrongPtr<MyGUIPadFocusComponent>(gameObjectPtr->getComponent<MyGUIPadFocusComponent>()).get();
            if (nullptr != padFocusComponent)
            {
                padFocusComponent->setActivated(true);
            }
        }

        this->suspendedPadFocusGameObjectIds.clear();
    }

    // =============================================================================
    // Layout
    // =============================================================================

    void VirtualKeyboardComponent::buildLayout(void)
    {
        this->keyEntries.clear();
        this->keyRows.clear();

        const bool isQwertz = ("QWERTZ" == this->layout->getListSelectedValue());

        std::vector<std::vector<KeyDefinition>> rows;

        // Row 0: digits. Upper case is the same digit.
        std::vector<KeyDefinition> digitRow;
        const Ogre::String digits = "1234567890";
        for (size_t i = 0; i < digits.size(); i++)
        {
            const Ogre::String digit(1, digits[i]);
            digitRow.push_back(makeKey(KEY_CHARACTER, digit, digit, 1.0f));
        }
        rows.push_back(digitRow);

        // Row 1 to 3: letters. QWERTZ and QWERTY differ only in the position of y and z.
        std::vector<KeyDefinition> letterRow1;
        addLetterKeys(letterRow1, true == isQwertz ? "qwertzuiop" : "qwertyuiop");
        rows.push_back(letterRow1);

        std::vector<KeyDefinition> letterRow2;
        addLetterKeys(letterRow2, "asdfghjkl");
        rows.push_back(letterRow2);

        std::vector<KeyDefinition> letterRow3;
        addLetterKeys(letterRow3, true == isQwertz ? "yxcvbnm" : "zxcvbnm");
        letterRow3.push_back(makeKey(KEY_CHARACTER, ".", ".", 1.0f));
        letterRow3.push_back(makeKey(KEY_CHARACTER, "-", "_", 1.0f));
        rows.push_back(letterRow3);

        // Umlauts as UTF-8 byte sequences, so that the result does not depend on the source file encoding of the compiler.
        if (true == this->showUmlauts->getBool())
        {
            std::vector<KeyDefinition> umlautRow;
            umlautRow.push_back(makeKey(KEY_CHARACTER, "\xC3\xA4", "\xC3\x84", 1.0f)); // ae
            umlautRow.push_back(makeKey(KEY_CHARACTER, "\xC3\xB6", "\xC3\x96", 1.0f)); // oe
            umlautRow.push_back(makeKey(KEY_CHARACTER, "\xC3\xBC", "\xC3\x9C", 1.0f)); // ue
            umlautRow.push_back(makeKey(KEY_CHARACTER, "\xC3\x9F", "\xC3\x9F", 1.0f)); // sz
            rows.push_back(umlautRow);
        }

        // Last row: the weights sum up to the 10 units of the digit row
        std::vector<KeyDefinition> specialRow;
        specialRow.push_back(makeKey(KEY_SHIFT, "Shift", "Shift", 1.5f));
        specialRow.push_back(makeKey(KEY_CAPS, "Caps", "Caps", 1.5f));
        specialRow.push_back(makeKey(KEY_SPACE, "Space", "Space", 3.0f));
        specialRow.push_back(makeKey(KEY_BACKSPACE, "Back", "Back", 1.5f));
        specialRow.push_back(makeKey(KEY_CANCEL, "Cancel", "Cancel", 1.25f));
        specialRow.push_back(makeKey(KEY_ACCEPT, "OK", "OK", 1.25f));
        rows.push_back(specialRow);

        // Geometry, relative to the client area of the keyboard window (below the caption). There is no text field: the text goes into the target widget.
        const Ogre::Real areaLeft = 0.02f;
        const Ogre::Real areaWidth = 0.96f;
        const Ogre::Real areaTop = 0.03f;
        const Ogre::Real areaBottom = 0.97f;
        const Ogre::Real gapX = 0.008f;
        const Ogre::Real gapY = 0.018f;

        const Ogre::Real rowCount = static_cast<Ogre::Real>(rows.size());
        const Ogre::Real keyHeight = ((areaBottom - areaTop) - gapY * (rowCount - 1.0f)) / rowCount;
        // The digit row has 10 units and fills the whole width, all other rows are centered with the same unit width
        const Ogre::Real unitWidth = (areaWidth - gapX * 9.0f) / 10.0f;

        for (size_t rowIndex = 0; rowIndex < rows.size(); rowIndex++)
        {
            const std::vector<KeyDefinition>& row = rows[rowIndex];

            Ogre::Real rowWidth = gapX * static_cast<Ogre::Real>(row.size() - 1);
            for (size_t i = 0; i < row.size(); i++)
            {
                rowWidth += row[i].weight * unitWidth + (row[i].weight - 1.0f) * gapX;
            }

            Ogre::Real left = areaLeft + (areaWidth - rowWidth) * 0.5f;
            const Ogre::Real top = areaTop + static_cast<Ogre::Real>(rowIndex) * (keyHeight + gapY);

            std::vector<int> rowIndices;

            for (size_t i = 0; i < row.size(); i++)
            {
                KeyEntry keyEntry;
                keyEntry.definition = row[i];
                keyEntry.row = static_cast<int>(rowIndex);
                keyEntry.left = left;
                keyEntry.top = top;
                keyEntry.width = row[i].weight * unitWidth + (row[i].weight - 1.0f) * gapX;
                keyEntry.height = keyHeight;
                keyEntry.centerX = left + keyEntry.width * 0.5f;
                keyEntry.button = nullptr;

                rowIndices.push_back(static_cast<int>(this->keyEntries.size()));
                this->keyEntries.push_back(keyEntry);

                left += keyEntry.width + gapX;
            }

            this->keyRows.push_back(rowIndices);
        }
    }

    // =============================================================================
    // Render thread: widgets
    // =============================================================================

    void VirtualKeyboardComponent::createWidgets(const Ogre::Vector2& position, const Ogre::Vector2& size)
    {
        // Layers: "Wallpaper", "ToolTip", "Info", "FadeMiddle", "Popup", "Main", "Modal", "Middle", "Overlapped", "Back", "DragAndDrop", "FadeBusy", "Pointer", "Fade", "Statistic"
        const MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        // 'position' and 'size' describe a box in screen fractions. The keyboard is NOT stretched to it, because relative sizes distort on ultra wide screens
        // (flat keys, a window which is higher than the box). Instead the keys keep a fixed proportion and the window is fitted into the box.
        const int boxLeft = static_cast<int>(position.x * static_cast<Ogre::Real>(viewSize.width));
        const int boxTop = static_cast<int>(position.y * static_cast<Ogre::Real>(viewSize.height));
        const int boxWidth = static_cast<int>(size.x * static_cast<Ogre::Real>(viewSize.width));
        const int boxHeight = static_cast<int>(size.y * static_cast<Ogre::Real>(viewSize.height));

        // Provisional window, only to learn how much the caption and the border take away from the client area
        this->window = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::Window>("Window", MyGUI::IntCoord(boxLeft, boxTop, boxWidth, boxHeight), MyGUI::Align::Default, "Popup");
        this->window->setCaption("Virtual Keyboard");
        this->window->setMovable(false);

        const int overheadWidth = this->window->getWidth() - this->window->getClientCoord().width;
        const int overheadHeight = this->window->getHeight() - this->window->getClientCoord().height;

        // Client height per client width, so that a key is about 1.5 times as wide as high
        Ogre::Real heightPerWidth = 0.55f;
        if (false == this->keyEntries.empty() && this->keyEntries[0].height > 0.0f)
        {
            const Ogre::Real keyProportion = 1.5f;
            heightPerWidth = this->keyEntries[0].width / (keyProportion * this->keyEntries[0].height);
        }

        int clientWidth = boxWidth - overheadWidth;
        int clientHeight = static_cast<int>(static_cast<Ogre::Real>(clientWidth) * heightPerWidth);

        if (clientHeight > boxHeight - overheadHeight)
        {
            clientHeight = boxHeight - overheadHeight;
            clientWidth = static_cast<int>(static_cast<Ogre::Real>(clientHeight) / heightPerWidth);
        }

        int windowWidth = clientWidth + overheadWidth;
        int windowHeight = clientHeight + overheadHeight;

        // Centered in the box and never outside of the screen
        int windowLeft = boxLeft + (boxWidth - windowWidth) / 2;
        int windowTop = boxTop + (boxHeight - windowHeight) / 2;

        if (windowLeft + windowWidth > viewSize.width)
        {
            windowLeft = viewSize.width - windowWidth;
        }
        if (windowTop + windowHeight > viewSize.height)
        {
            windowTop = viewSize.height - windowHeight;
        }
        if (windowLeft < 0)
        {
            windowLeft = 0;
        }
        if (windowTop < 0)
        {
            windowTop = 0;
        }

        this->window->setCoord(windowLeft, windowTop, windowWidth, windowHeight);

        for (size_t i = 0; i < this->keyEntries.size(); i++)
        {
            KeyEntry& keyEntry = this->keyEntries[i];

            keyEntry.button = this->window->createWidgetReal<MyGUI::Button>("Button", keyEntry.left, keyEntry.top, keyEntry.width, keyEntry.height, MyGUI::Align::Left | MyGUI::Align::Top);
            keyEntry.button->setCaption(keyEntry.definition.lower);
            keyEntry.button->setTextColour(MyGUI::Colour(0.85f, 0.85f, 0.85f, 1.0f));
            keyEntry.button->eventMouseButtonClick += MyGUI::newDelegate(this, &VirtualKeyboardComponent::notifyKeyClicked);
        }
    }

    void VirtualKeyboardComponent::destroyWidgets(void)
    {
        for (size_t i = 0; i < this->keyEntries.size(); i++)
        {
            if (nullptr != this->keyEntries[i].button)
            {
                this->keyEntries[i].button->eventMouseButtonClick -= MyGUI::newDelegate(this, &VirtualKeyboardComponent::notifyKeyClicked);
                this->keyEntries[i].button = nullptr;
            }
        }

        // The children are destroyed together with the window
        if (nullptr != this->window)
        {
            MyGUI::Gui::getInstancePtr()->destroyWidget(this->window);
            this->window = nullptr;
        }
    }

    void VirtualKeyboardComponent::applyVisuals(bool upperCase, bool shift, bool caps, int cursor, const Ogre::String& currentText, bool writeTarget, GameObjectCompPtr target)
    {
        if (nullptr != this->window)
        {
            const MyGUI::Colour normalColour(0.85f, 0.85f, 0.85f, 1.0f);
            const MyGUI::Colour activeColour(1.0f, 0.85f, 0.3f, 1.0f);

            for (size_t i = 0; i < this->keyEntries.size(); i++)
            {
                MyGUI::Button* button = this->keyEntries[i].button;
                if (nullptr == button)
                {
                    continue;
                }

                const KeyDefinition& keyDefinition = this->keyEntries[i].definition;

                if (KEY_CHARACTER == keyDefinition.type)
                {
                    button->setCaption(true == upperCase ? keyDefinition.upper : keyDefinition.lower);
                }
                else if (KEY_SHIFT == keyDefinition.type)
                {
                    button->setTextColour(true == shift ? activeColour : normalColour);
                }
                else if (KEY_CAPS == keyDefinition.type)
                {
                    button->setTextColour(true == caps ? activeColour : normalColour);
                }

                // The key under the gamepad cursor shows the pressed look
                button->setStateSelected(static_cast<int>(i) == cursor);
            }
        }

        if (true == writeTarget && nullptr != target)
        {
            MyGUIComponent* myGuiComponent = dynamic_cast<MyGUIComponent*>(target.get());
            if (nullptr != myGuiComponent && nullptr != myGuiComponent->getWidget())
            {
                // Written to the widget directly. If the MyGUI text component keeps its own copy of the caption (e.g. a variant saved in the scene),
                // this is the one line to change, so that it is updated as well.
                MyGUI::TextBox* textBox = myGuiComponent->getWidget()->castType<MyGUI::TextBox>(false);
                if (nullptr != textBox)
                {
                    textBox->setCaption(currentText);
                }
            }
        }
    }

    void VirtualKeyboardComponent::refreshVisuals(bool writeTarget)
    {
        if (false == this->widgetsCreated)
        {
            return;
        }

        const bool upperCase = this->isUpperCase();
        const bool shift = this->shiftActive;
        const bool caps = this->capsLock;
        const int cursor = this->cursorIndex;
        const Ogre::String currentText = this->text;
        const GameObjectCompPtr target = this->targetComponent;

        // Fire and forget, only copies are captured. The weak pointer protects against a component which is destroyed before the command runs.
        boost::weak_ptr<GameObjectComponent> weakThis = this->shared_from_this();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, weakThis, upperCase, shift, caps, cursor, currentText, writeTarget, target]()
        {
            boost::shared_ptr<GameObjectComponent> strongThis = weakThis.lock();
            if (nullptr == strongThis)
            {
                return;
            }

            this->applyVisuals(upperCase, shift, caps, cursor, currentText, writeTarget, target);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "VirtualKeyboardComponent::refreshVisuals");
    }

    void VirtualKeyboardComponent::notifyKeyClicked(MyGUI::Widget* sender)
    {
        int index = -1;
        for (size_t i = 0; i < this->keyEntries.size(); i++)
        {
            if (this->keyEntries[i].button == sender)
            {
                index = static_cast<int>(i);
                break;
            }
        }

        if (-1 == index)
        {
            return;
        }

        // This callback comes from MyGUI on the render thread. All keyboard state lives on the logic thread, so the press is handed over.
        boost::weak_ptr<GameObjectComponent> weakThis = this->shared_from_this();

        NOWA::AppStateManager::LogicCommand logicCommand = [this, weakThis, index]()
        {
            boost::shared_ptr<GameObjectComponent> strongThis = weakThis.lock();
            if (nullptr == strongThis)
            {
                return;
            }

            this->pressKey(index);
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    // =============================================================================
    // Logic thread: typing
    // =============================================================================

    bool VirtualKeyboardComponent::isUpperCase(void) const
    {
        // Shift inverts caps lock, like on a real keyboard
        return this->shiftActive != this->capsLock;
    }

    void VirtualKeyboardComponent::appendText(const Ogre::String& characters)
    {
        if (getUtf8Length(this->text) >= static_cast<size_t>(this->maxLength->getULong()))
        {
            return;
        }

        this->text += characters;
    }

    void VirtualKeyboardComponent::removeLastCharacter(void)
    {
        removeLastUtf8Character(this->text);
    }

    void VirtualKeyboardComponent::applyAutoCapitalize(void)
    {
        if (false == this->autoCapitalize->getBool())
        {
            return;
        }

        if (true == this->text.empty() || ' ' == this->text[this->text.size() - 1])
        {
            this->shiftActive = true;
        }
    }

    void VirtualKeyboardComponent::notifyTextChanged(void)
    {
        if (false == VirtualKeyboardComponent::isLuaFunction(this->textChangedClosureFunction))
        {
            return;
        }

        try
        {
            luabind::call_function<void>(this->textChangedClosureFunction, this->text);
        }
        catch (luabind::error& error)
        {
            luabind::object errorMsg(luabind::from_stack(error.state(), -1));
            std::stringstream msg;
            msg << errorMsg;
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[VirtualKeyboardComponent] Caught error in 'reactOnTextChanged' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
        }
    }

    void VirtualKeyboardComponent::pressKey(int index)
    {
        if (false == this->widgetsCreated || 0 > index || index >= static_cast<int>(this->keyEntries.size()))
        {
            return;
        }

        // A mouse click also moves the cursor, so that mouse and gamepad stay in sync
        this->cursorIndex = index;

        const KeyDefinition keyDefinition = this->keyEntries[index].definition;
        const Ogre::String oldText = this->text;
        bool textModifyingKey = false;

        switch (keyDefinition.type)
        {
        case KEY_CHARACTER:
        {
            this->appendText(true == this->isUpperCase() ? keyDefinition.upper : keyDefinition.lower);
            // Shift is a one shot: it applies to the next character only
            this->shiftActive = false;
            textModifyingKey = true;
            break;
        }
        case KEY_SPACE:
        {
            this->appendText(" ");
            textModifyingKey = true;
            break;
        }
        case KEY_BACKSPACE:
        {
            this->removeLastCharacter();
            textModifyingKey = true;
            break;
        }
        case KEY_SHIFT:
        {
            this->shiftActive = (false == this->shiftActive);
            break;
        }
        case KEY_CAPS:
        {
            this->capsLock = (false == this->capsLock);
            break;
        }
        case KEY_CANCEL:
        {
            this->cancel();
            return;
        }
        case KEY_ACCEPT:
        {
            this->accept();
            return;
        }
        }

        // Not done after shift / caps, else the player could never switch the automatic capital letter off
        if (true == textModifyingKey)
        {
            this->applyAutoCapitalize();
        }

        const bool textChanged = (oldText != this->text);

        this->refreshVisuals(textChanged);

        if (true == textChanged)
        {
            this->notifyTextChanged();
        }
    }

    void VirtualKeyboardComponent::moveCursor(int directionX, int directionY)
    {
        if (true == this->keyEntries.empty() || 0 > this->cursorIndex || this->cursorIndex >= static_cast<int>(this->keyEntries.size()))
        {
            return;
        }

        const KeyEntry& currentEntry = this->keyEntries[this->cursorIndex];

        if (0 != directionX)
        {
            // Along the row, wrapping around at the ends
            const std::vector<int>& rowIndices = this->keyRows[currentEntry.row];

            int position = 0;
            for (size_t i = 0; i < rowIndices.size(); i++)
            {
                if (rowIndices[i] == this->cursorIndex)
                {
                    position = static_cast<int>(i);
                    break;
                }
            }

            const int count = static_cast<int>(rowIndices.size());
            position = (position + directionX + count) % count;
            this->cursorIndex = rowIndices[position];
        }
        else if (0 != directionY)
        {
            // To the neighbour row, wrapping around at the ends. The key whose center is nearest to the current one is taken, because the rows differ in key count.
            const int rowCount = static_cast<int>(this->keyRows.size());
            const int newRow = (currentEntry.row + directionY + rowCount) % rowCount;

            const std::vector<int>& rowIndices = this->keyRows[newRow];

            int bestIndex = rowIndices[0];
            Ogre::Real bestDistance = std::numeric_limits<Ogre::Real>::max();

            for (size_t i = 0; i < rowIndices.size(); i++)
            {
                const Ogre::Real distance = std::abs(this->keyEntries[rowIndices[i]].centerX - currentEntry.centerX);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestIndex = rowIndices[i];
                }
            }

            this->cursorIndex = bestIndex;
        }

        this->refreshVisuals(false);
    }

    // =============================================================================
    // Logic thread: input
    // =============================================================================

    bool VirtualKeyboardComponent::readDirection(int& directionX, int& directionY) const
    {
        directionX = 0;
        directionY = 0;

        // Attention: Only the state getters are used, never isActionPressed(). The timed variants rely on InputDeviceModule::update(dt), which is called for
        // the main keyboard module only. The repeat rate is therefore handled here, see 'Navigation Delay'.
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

            // Screen coordinates: up is the smaller y
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

    bool VirtualKeyboardComponent::isActionDownAnywhere(InputDeviceModule::Action action) const
    {
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

    void VirtualKeyboardComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating || false == this->isSimulating || false == this->activated->getBool() || false == this->widgetsCreated)
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
                this->moveCursor(directionX, directionY);
                this->navigationTimer = this->navigationDelay->getReal();
            }
        }
        else
        {
            // Released: the next press steps immediately instead of waiting for the delay
            this->navigationTimer = 0.0f;
        }

        // Only the flank counts, otherwise a held button would type a key every frame.
        // All states are read first, so that none of them is stuck, if an earlier one closes the keyboard.
        const bool confirmIsDown = this->isActionDownAnywhere(InputDeviceModule::JUMP);
        const bool backspaceIsDown = this->isActionDownAnywhere(InputDeviceModule::ACTION);
        const bool shiftIsDown = this->isActionDownAnywhere(InputDeviceModule::ATTACK_1);
        const bool spaceIsDown = this->isActionDownAnywhere(InputDeviceModule::ATTACK_2);
        const bool acceptIsDown = this->isActionDownAnywhere(InputDeviceModule::START);
        const bool cancelIsDown = this->isActionDownAnywhere(InputDeviceModule::INVENTORY);

        const bool confirmPressed = (true == confirmIsDown && false == this->confirmWasDown);
        const bool backspacePressed = (true == backspaceIsDown && false == this->backspaceWasDown);
        const bool shiftPressed = (true == shiftIsDown && false == this->shiftWasDown);
        const bool spacePressed = (true == spaceIsDown && false == this->spaceWasDown);
        const bool acceptPressed = (true == acceptIsDown && false == this->acceptWasDown);
        const bool cancelPressed = (true == cancelIsDown && false == this->cancelWasDown);

        this->confirmWasDown = confirmIsDown;
        this->backspaceWasDown = backspaceIsDown;
        this->shiftWasDown = shiftIsDown;
        this->spaceWasDown = spaceIsDown;
        this->acceptWasDown = acceptIsDown;
        this->cancelWasDown = cancelIsDown;

        // Closing first, so that a simultaneous press of a key does not change a text which is going away
        if (true == cancelPressed)
        {
            this->cancel();
            return;
        }
        if (true == acceptPressed)
        {
            this->accept();
            return;
        }

        if (true == confirmPressed)
        {
            this->pressKey(this->cursorIndex);
            // The key may have closed the keyboard (Cancel / OK)
            if (false == this->widgetsCreated)
            {
                return;
            }
        }

        // The shortcut buttons press the matching on screen key, but the cursor of the player must stay where it is.
        // pressKey() moves the cursor to the pressed key (needed for mouse clicks), so it is restored below.
        const int savedCursorIndex = this->cursorIndex;

        if (true == backspacePressed)
        {
            for (size_t i = 0; i < this->keyEntries.size(); i++)
            {
                if (KEY_BACKSPACE == this->keyEntries[i].definition.type)
                {
                    this->pressKey(static_cast<int>(i));
                    break;
                }
            }
        }
        if (true == shiftPressed)
        {
            for (size_t i = 0; i < this->keyEntries.size(); i++)
            {
                if (KEY_SHIFT == this->keyEntries[i].definition.type)
                {
                    this->pressKey(static_cast<int>(i));
                    break;
                }
            }
        }
        if (true == spacePressed)
        {
            for (size_t i = 0; i < this->keyEntries.size(); i++)
            {
                if (KEY_SPACE == this->keyEntries[i].definition.type)
                {
                    this->pressKey(static_cast<int>(i));
                    break;
                }
            }
        }

        if (true == this->widgetsCreated && this->cursorIndex != savedCursorIndex)
        {
            this->cursorIndex = savedCursorIndex;
            this->refreshVisuals(false);
        }
    }

    // =============================================================================
    // Properties
    // =============================================================================

    void VirtualKeyboardComponent::setTargetId(unsigned long targetId)
    {
        this->targetId->setValue(targetId);
    }

    unsigned long VirtualKeyboardComponent::getTargetId(void) const
    {
        return this->targetId->getULong();
    }

    void VirtualKeyboardComponent::setRelativePosition(const Ogre::Vector2& relativePosition)
    {
        this->relativePosition->setValue(relativePosition);

        if (true == this->widgetsCreated)
        {
            boost::weak_ptr<GameObjectComponent> weakThis = this->shared_from_this();

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, weakThis, relativePosition]()
            {
                boost::shared_ptr<GameObjectComponent> strongThis = weakThis.lock();
                if (nullptr == strongThis)
                {
                    return;
                }

                if (nullptr != this->window)
                {
                    this->window->setRealPosition(relativePosition.x, relativePosition.y);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "VirtualKeyboardComponent::setRelativePosition");
        }
    }

    Ogre::Vector2 VirtualKeyboardComponent::getRelativePosition(void) const
    {
        return this->relativePosition->getVector2();
    }

    void VirtualKeyboardComponent::setRelativeSize(const Ogre::Vector2& relativeSize)
    {
        // Applied at the next open(), because the children are sized from the window
        this->relativeSize->setValue(relativeSize);
    }

    Ogre::Vector2 VirtualKeyboardComponent::getRelativeSize(void) const
    {
        return this->relativeSize->getVector2();
    }

    void VirtualKeyboardComponent::setLayout(const Ogre::String& layout)
    {
        this->layout->setListSelectedValue(layout);
    }

    Ogre::String VirtualKeyboardComponent::getLayout(void) const
    {
        return this->layout->getListSelectedValue();
    }

    void VirtualKeyboardComponent::setMaxLength(unsigned long maxLength)
    {
        this->maxLength->setValue(maxLength);
    }

    unsigned long VirtualKeyboardComponent::getMaxLength(void) const
    {
        return this->maxLength->getULong();
    }

    void VirtualKeyboardComponent::setNavigationDelay(Ogre::Real navigationDelay)
    {
        this->navigationDelay->setValue(navigationDelay);
    }

    Ogre::Real VirtualKeyboardComponent::getNavigationDelay(void) const
    {
        return this->navigationDelay->getReal();
    }

    void VirtualKeyboardComponent::setShowUmlauts(bool showUmlauts)
    {
        this->showUmlauts->setValue(showUmlauts);
    }

    bool VirtualKeyboardComponent::getShowUmlauts(void) const
    {
        return this->showUmlauts->getBool();
    }

    void VirtualKeyboardComponent::setAutoCapitalize(bool autoCapitalize)
    {
        this->autoCapitalize->setValue(autoCapitalize);
    }

    bool VirtualKeyboardComponent::getAutoCapitalize(void) const
    {
        return this->autoCapitalize->getBool();
    }

    void VirtualKeyboardComponent::setSuspendPadFocus(bool suspendPadFocus)
    {
        this->suspendPadFocusVariant->setValue(suspendPadFocus);
    }

    bool VirtualKeyboardComponent::getSuspendPadFocus(void) const
    {
        return this->suspendPadFocusVariant->getBool();
    }

    Ogre::String VirtualKeyboardComponent::getText(void) const
    {
        return this->text;
    }

    void VirtualKeyboardComponent::setText(const Ogre::String& text)
    {
        this->text = truncateUtf8(text, static_cast<size_t>(this->maxLength->getULong()));

        if (true == this->widgetsCreated)
        {
            this->applyAutoCapitalize();
            this->refreshVisuals(true);
            this->notifyTextChanged();
        }
    }

    void VirtualKeyboardComponent::reactOnTextChanged(luabind::object closureFunction)
    {
        if (false == VirtualKeyboardComponent::isLuaFunction(closureFunction))
        {
            return;
        }

        this->textChangedClosureFunction = closureFunction;
    }

    void VirtualKeyboardComponent::reactOnClosed(luabind::object closureFunction)
    {
        if (false == VirtualKeyboardComponent::isLuaFunction(closureFunction))
        {
            return;
        }

        this->closedClosureFunction = closureFunction;
    }

    bool VirtualKeyboardComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // One keyboard per game object is enough, it is re-used for every text input
        return 0 == gameObject->getComponentCount<VirtualKeyboardComponent>();
    }

    // =============================================================================
    // Lua registration part
    // =============================================================================

    VirtualKeyboardComponent* getVirtualKeyboardComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<VirtualKeyboardComponent>(gameObject->getComponentWithOccurrence<VirtualKeyboardComponent>(occurrenceIndex)).get();
    }

    VirtualKeyboardComponent* getVirtualKeyboardComponent(GameObject* gameObject)
    {
        return makeStrongPtr<VirtualKeyboardComponent>(gameObject->getComponent<VirtualKeyboardComponent>()).get();
    }

    VirtualKeyboardComponent* getVirtualKeyboardComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<VirtualKeyboardComponent>(gameObject->getComponentFromName<VirtualKeyboardComponent>(name)).get();
    }

    // Ids are passed as strings between lua and c++, because an unsigned long does not survive the lua number type
    void setVirtualKeyboardTargetIdString(VirtualKeyboardComponent* instance, const Ogre::String& targetId)
    {
        instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(targetId));
    }

    Ogre::String getVirtualKeyboardTargetIdString(VirtualKeyboardComponent* instance)
    {
        return Ogre::StringConverter::toString(instance->getTargetId());
    }

    void VirtualKeyboardComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<VirtualKeyboardComponent, GameObjectComponent>("VirtualKeyboardComponent")
                .def("setActivated", &VirtualKeyboardComponent::setActivated)
                .def("isActivated", &VirtualKeyboardComponent::isActivated)
                .def("open", &VirtualKeyboardComponent::open)
                .def("accept", &VirtualKeyboardComponent::accept)
                .def("cancel", &VirtualKeyboardComponent::cancel)
                .def("setTargetId", &setVirtualKeyboardTargetIdString)
                .def("getTargetId", &getVirtualKeyboardTargetIdString)
                .def("setMaxLength", &VirtualKeyboardComponent::setMaxLength)
                .def("getMaxLength", &VirtualKeyboardComponent::getMaxLength)
                .def("setText", &VirtualKeyboardComponent::setText)
                .def("getText", &VirtualKeyboardComponent::getText)
                .def("reactOnTextChanged", &VirtualKeyboardComponent::reactOnTextChanged)
                .def("reactOnClosed", &VirtualKeyboardComponent::reactOnClosed)];

        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "class inherits GameObjectComponent", VirtualKeyboardComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void setActivated(bool activated)",
            "Opens (true) or closes (false, keeps the text) the keyboard. Lock the player movement while it is open, because it uses the same UP / DOWN / LEFT / RIGHT actions.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "bool isActivated()", "Gets whether the keyboard is open.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void open()", "Opens the keyboard. The current text of the target widget is the start text.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void accept()", "Closes the keyboard and keeps the typed text.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void cancel()", "Closes the keyboard and restores the text from the time it was opened.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void setTargetId(string id)", "Sets the id of the game object or MyGUI component whose text widget receives the text.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "string getTargetId()", "Gets the target id.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void setMaxLength(unsigned long maxLength)", "Sets the maximum count of characters.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "unsigned long getMaxLength()", "Gets the maximum count of characters.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void setText(string text)", "Sets the text, cut to the maximum length.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "string getText()", "Gets the text typed so far.");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void reactOnTextChanged(func closureFunction, string text)",
            "Sets whether to react if the text changed. E.g. getVirtualKeyboardComponent():reactOnTextChanged(function(text) ... end)");
        LuaScriptApi::getInstance()->addClassToCollection("VirtualKeyboardComponent", "void reactOnClosed(func closureFunction, bool accepted, string text)",
            "Sets whether to react if the keyboard has been closed. E.g. getVirtualKeyboardComponent():reactOnClosed(function(accepted, text) ... end)");

        gameObjectClass.def("getVirtualKeyboardComponentFromName", &getVirtualKeyboardComponentFromName);
        gameObjectClass.def("getVirtualKeyboardComponent", (VirtualKeyboardComponent * (*)(GameObject*)) & getVirtualKeyboardComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "VirtualKeyboardComponent getVirtualKeyboardComponent()", "Gets the virtual keyboard component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "VirtualKeyboardComponent getVirtualKeyboardComponentFromName(string name)", "Gets the virtual keyboard component by name.");

        gameObjectControllerClass.def("castVirtualKeyboardComponent", &GameObjectController::cast<VirtualKeyboardComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "VirtualKeyboardComponent castVirtualKeyboardComponent(VirtualKeyboardComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end