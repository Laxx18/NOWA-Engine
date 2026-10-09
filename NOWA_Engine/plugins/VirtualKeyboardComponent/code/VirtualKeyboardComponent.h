/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef VIRTUALKEYBOARDCOMPONENT_H
#define VIRTUALKEYBOARDCOMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "modules/InputDeviceModule.h"

namespace NOWA
{

    /**
     * @brief		On screen keyboard which can be operated with a gamepad (and with the mouse). It writes the typed text live into the text widget
     *				of another MyGUI component (e.g. a MyGUITextComponent, which shall receive a player name), see 'Target Id'.
     *
     *				Gamepad controls (with the default button mapping): D-Pad / left stick moves over the keys, A (JUMP) types the key under the cursor,
     *				B (ACTION) is backspace, X (ATTACK_1) is shift, Y (ATTACK_2) is space, Start (START) accepts and Select (INVENTORY) cancels.
     */
    class EXPORTED VirtualKeyboardComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<VirtualKeyboardComponent> VirtualKeyboardComponentPtr;

        enum KeyType
        {
            KEY_CHARACTER,
            KEY_SHIFT,
            KEY_CAPS,
            KEY_SPACE,
            KEY_BACKSPACE,
            KEY_CANCEL,
            KEY_ACCEPT
        };

        struct KeyDefinition
        {
            KeyType type;
            // UTF-8 captions. For keys other than KEY_CHARACTER only 'lower' is used as caption.
            Ogre::String lower;
            Ogre::String upper;
            // Width in key units, 1 = a normal character key
            Ogre::Real weight;
        };

        struct KeyEntry
        {
            KeyDefinition definition;
            int row;
            // Relative to the keyboard window (0..1)
            Ogre::Real left;
            Ogre::Real top;
            Ogre::Real width;
            Ogre::Real height;
            Ogre::Real centerX;
            // Only touched on the render thread
            MyGUI::Button* button;
        };

    public:
        VirtualKeyboardComponent();

        virtual ~VirtualKeyboardComponent();

        /**
         * @see		Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see		Ogre::Plugin::initialise
         */
        virtual void initialise() override;

        /**
         * @see		Ogre::Plugin::shutdown
         */
        virtual void shutdown() override;

        /**
         * @see		Ogre::Plugin::uninstall
         */
        virtual void uninstall() override;

        /**
         * @see		Ogre::Plugin::getName
         */
        virtual const Ogre::String& getName() const override;

        /**
         * @see		Ogre::Plugin::getAbiCookie
         */
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see		GameObjectComponent::onCloned
         */
        virtual bool onCloned(void) override;

        /**
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see		GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see		GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see		GameObjectComponent::setActivated
         * @note		true opens the keyboard, false closes it and keeps the typed text (same as accept()).
         */
        virtual void setActivated(bool activated) override;

        /**
         * @see		GameObjectComponent::isActivated
         */
        virtual bool isActivated(void) const override;

    public:
        /**
         * @see		GameObjectComponent::getStaticClassId
         */
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("VirtualKeyboardComponent");
        }

        /**
         * @see		GameObjectComponent::getStaticClassName
         */
        static Ogre::String getStaticClassName(void)
        {
            return "VirtualKeyboardComponent";
        }

        /**
         * @see		GameObjectComponent::canStaticAddComponent
         */
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @see		GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: On screen keyboard for gamepad mode, e.g. to enter a player name. It writes the typed text live into the text widget of the MyGUI component "
                   "with the 'Target Id' (game object id or MyGUI component id). Open it from lua with setActivated(true) or open(). "
                   "Gamepad: D-Pad moves, JUMP types, ACTION is backspace, ATTACK_1 is shift, ATTACK_2 is space, START accepts, INVENTORY cancels. "
                   "Lock the player movement while it is open, because it uses the same UP / DOWN / LEFT / RIGHT actions.";
        }

        /**
         * @see		GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        /**
         * @brief		Opens the keyboard. The current text of the target widget is the start text.
         */
        void open(void);

        /**
         * @brief		Closes the keyboard and keeps the typed text in the target widget.
         */
        void accept(void);

        /**
         * @brief		Closes the keyboard and restores the text the target widget had when the keyboard was opened.
         */
        void cancel(void);

        /**
         * @brief		Sets the target: The id of the game object or of the MyGUI component whose text widget receives the text.
         * @param[in]	targetId	The id to set (0 = none, the keyboard then only reports the text via lua).
         */
        void setTargetId(unsigned long targetId);

        unsigned long getTargetId(void) const;

        /**
         * @brief		Sets the relative position of the keyboard window (0 = top/left, 1 = bottom/right)
         */
        void setRelativePosition(const Ogre::Vector2& relativePosition);

        Ogre::Vector2 getRelativePosition(void) const;

        /**
         * @brief		Sets the maximum size of the keyboard window (fraction of the screen). The keyboard keeps a fixed key proportion and fits into this box, so it does not get distorted on ultra wide screens.
         */
        void setRelativeSize(const Ogre::Vector2& relativeSize);

        Ogre::Vector2 getRelativeSize(void) const;

        /**
         * @brief		Sets the layout: "QWERTZ" or "QWERTY". Takes effect at the next open().
         */
        void setLayout(const Ogre::String& layout);

        Ogre::String getLayout(void) const;

        void setMaxLength(unsigned long maxLength);

        unsigned long getMaxLength(void) const;

        void setNavigationDelay(Ogre::Real navigationDelay);

        Ogre::Real getNavigationDelay(void) const;

        void setShowUmlauts(bool showUmlauts);

        bool getShowUmlauts(void) const;

        void setAutoCapitalize(bool autoCapitalize);

        bool getAutoCapitalize(void) const;

        void setSuspendPadFocus(bool suspendPadFocus);

        bool getSuspendPadFocus(void) const;

        /**
         * @brief		Gets the text typed so far.
         */
        Ogre::String getText(void) const;

        /**
         * @brief		Sets the text (UTF-8). It is cut to the maximum length.
         */
        void setText(const Ogre::String& text);

        /**
         * @brief		Sets a lua closure which is called whenever the text changed: function(text)
         */
        void reactOnTextChanged(luabind::object closureFunction);

        /**
         * @brief		Sets a lua closure which is called when the keyboard has been closed: function(accepted, text)
         */
        void reactOnClosed(luabind::object closureFunction);

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrTargetId(void)
        {
            return "Target Id";
        }
        static const Ogre::String AttrLayout(void)
        {
            return "Layout";
        }
        static const Ogre::String AttrMaxLength(void)
        {
            return "Max Length";
        }
        static const Ogre::String AttrRelativePosition(void)
        {
            return "Relative Position";
        }
        static const Ogre::String AttrRelativeSize(void)
        {
            return "Relative Size";
        }
        static const Ogre::String AttrNavigationDelay(void)
        {
            return "Navigation Delay";
        }
        static const Ogre::String AttrShowUmlauts(void)
        {
            return "Show Umlauts";
        }
        static const Ogre::String AttrAutoCapitalize(void)
        {
            return "Auto Capitalize";
        }
        static const Ogre::String AttrSuspendPadFocus(void)
        {
            return "Suspend Pad Focus";
        }

    private:
        // ---- Logic thread -------------------------------------------------------------------
        void buildLayout(void);

        void resolveTarget(void);

        void pressKey(int index);

        void moveCursor(int directionX, int directionY);

        void appendText(const Ogre::String& characters);

        void removeLastCharacter(void);

        void applyAutoCapitalize(void);

        bool isUpperCase(void) const;

        void notifyTextChanged(void);

        void closeKeyboard(bool accepted, bool notifyLua);

        void suspendPadFocus(void);

        void resumePadFocus(void);

        bool readDirection(int& directionX, int& directionY) const;

        bool isActionDownAnywhere(InputDeviceModule::Action action) const;

        // Pushes the current state to the widgets (render thread, fire and forget)
        void refreshVisuals(bool writeTarget);

        // ---- Render thread ------------------------------------------------------------------
        void createWidgets(const Ogre::Vector2& position, const Ogre::Vector2& size);

        void destroyWidgets(void);

        void applyVisuals(bool upperCase, bool shift, bool caps, int cursor, const Ogre::String& currentText, bool writeTarget, GameObjectCompPtr target);

        // A default constructed luabind::object has no lua state. Calling luabind::type() on it crashes, so it is checked first.
        static bool isLuaFunction(const luabind::object& closureFunction);

        // MyGUI callback, comes from the render thread
        void notifyKeyClicked(MyGUI::Widget* sender);

    private:
        Ogre::String name;
        bool isSimulating;
        bool widgetsCreated;

        // Render thread only
        MyGUI::Window* window;

        // Layout, built on the logic thread. The 'button' pointers are written on the render thread while open() waits.
        std::vector<KeyEntry> keyEntries;
        std::vector<std::vector<int>> keyRows;

        // Logic thread state
        int cursorIndex;
        bool shiftActive;
        bool capsLock;
        Ogre::String text;
        Ogre::String originalText;
        Ogre::Real navigationTimer;
        bool confirmWasDown;
        bool backspaceWasDown;
        bool shiftWasDown;
        bool spaceWasDown;
        bool acceptWasDown;
        bool cancelWasDown;

        GameObjectCompPtr targetComponent;
        std::vector<unsigned long> suspendedPadFocusGameObjectIds;

        luabind::object textChangedClosureFunction;
        luabind::object closedClosureFunction;

        Variant* activated;
        Variant* targetId;
        Variant* layout;
        Variant* maxLength;
        Variant* relativePosition;
        Variant* relativeSize;
        Variant* navigationDelay;
        Variant* showUmlauts;
        Variant* autoCapitalize;
        Variant* suspendPadFocusVariant;
    };

}; // namespace end

#endif