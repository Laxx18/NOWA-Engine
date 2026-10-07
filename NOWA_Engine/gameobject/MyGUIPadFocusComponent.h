#ifndef MYGUI_PAD_FOCUS_COMPONENT_H
#define MYGUI_PAD_FOCUS_COMPONENT_H

#include "GameObjectComponent.h"
#include "gameobject/GameObjectComponent.h"

namespace NOWA
{
    /**
     * @class 	MyGUIPadFocusComponent
     * @brief 	Gamepad and keyboard navigation for MyGUI widgets, without any pointer emulation.
     *
     *			Instead of moving a free cursor, the focus jumps from widget to widget and the mouse is snapped onto the
     *			center of the focused widget (InputDeviceCore::setMousePosition). Everything else follows for free:
     *			- MyGUI draws its own hover state, so no highlight code is necessary
     *			- A confirm press is injected as a real MyGUI mouse press plus release, so each widget's existing
     *			  'mouseClickEventName' / reactOnMouseButtonClick closure is called, exactly as with a real mouse
     *			- An inventory (MyGUIItemBoxComponent) contributes one focus target per slot, so item boxes behave
     *			  like a grid of buttons, including tool tips and the slot's select/active visuals
     *			- Mouse and gamepad never fight each other, because there is only one cursor. Moving the real mouse
     *			  simply takes over
     *
     *			Which widgets take part is decided by the MyGUI components themselves: everything whose isFocusable()
     *			returns true, that is a MyGUIButtonComponent or a MyGUIItemBoxComponent with its 'Focusable' property
     *			switched on. The game object does not matter, so a whole menu may sit as a pile of components on one
     *			single global game object.
     *
     *			The component is a mode: it is switched on and off from lua (setActivated), e.g. when the player stands
     *			at a point of interest or opens the inventory. While it is active, the mapped UP / DOWN / LEFT / RIGHT
     *			actions step the focus, which is why the player movement should be locked by the script in the meantime.
     */
    class EXPORTED MyGUIPadFocusComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<MyGUIPadFocusComponent> MyGUIPadFocusCompPtr;

    public:
        MyGUIPadFocusComponent();

        virtual ~MyGUIPadFocusComponent();

        /////////////////////////////////////////////////////////////////////////////

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
         * @see		GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

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

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("MyGUIPadFocusComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "MyGUIPadFocusComponent";
        }

        /**
         * @see  GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Gamepad and keyboard navigation for MyGUI widgets. Every MyGUI component with its 'Focusable' property switched on becomes a "
                   "focus target, no matter on which game object it sits. The mapped UP / DOWN / LEFT / RIGHT actions move the focus to the nearest widget "
                   "in that direction, the mouse is snapped onto it, and the confirm action injects a real MyGUI click, so the widget's own click closure "
                   "is called. A MyGUIItemBoxComponent contributes one target per inventory slot.\n"
                   "Note: This component is a mode. Activate it from lua (setActivated(true)) and lock the player movement while it is active, because "
                   "it uses the same UP / DOWN / LEFT / RIGHT actions as the player controller.";
        }

        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief	Switches the pad navigation on or off. On activation the focus targets are collected again and the
         *			focus is placed on the first one.
         */
        virtual void setActivated(bool activated) override;

        virtual bool isActivated(void) const override;

        void setNavigationDelay(Ogre::Real navigationDelay);

        Ogre::Real getNavigationDelay(void) const;

        void setConfirmAction(const Ogre::String& confirmAction);

        Ogre::String getConfirmAction(void) const;

        void setShowPointer(bool showPointer);

        bool getShowPointer(void) const;

        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief	Collects the focus targets again. Necessary whenever widgets appeared, vanished or moved, e.g. after
         *			the inventory received its first item.
         */
        void refreshFocusTargets(void);

        /**
         * @brief	Steps the focus to the next target in collection order. Meant for a shoulder button, which cycles
         *			through everything without caring about the geometry.
         */
        void focusNext(void);

        /**
         * @brief	Steps the focus to the previous target in collection order.
         */
        void focusPrevious(void);

        /**
         * @brief	Injects a MyGUI mouse press plus release on the focused widget, which makes it a real click.
         */
        void confirm(void);

        /**
         * @brief	Gets the id of the game object the focused widget belongs to, or 0, if nothing is focused.
         */
        unsigned long getFocusedGameObjectId(void) const;

        /**
         * @brief	Gets the inventory slot index of the focused target, or -1, if the focused widget is not an
         *			inventory slot.
         */
        int getFocusedSlotIndex(void) const;

        /**
         * @brief	Lua closure function gets called each time the focus moved to another widget. It receives the game
         *			object id of the widget (as string) and the inventory slot index (-1 if it is no inventory slot).
         * @param[in] closureFunction The closure function set.
         */
        void reactOnFocusChanged(luabind::object closureFunction);

        /////////////////////////////////////////////////////////////////////////////

        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrNavigationDelay(void)
        {
            return "Navigation Delay";
        }
        static Ogre::String AttrConfirmAction(void)
        {
            return "Confirm Action";
        }
        static Ogre::String AttrShowPointer(void)
        {
            return "Show Pointer";
        }

    private:
        /**
         * @brief	One focusable thing on the screen. A plain widget, or one slot of an inventory.
         */
        struct FocusTarget
        {
            unsigned long gameObjectId;
            int slotIndex;
            int centerX;
            int centerY;
        };

    private:
        /**
         * @brief	Collects every MyGUI component of the whole scene whose isFocusable() returns true. The widget
         *			coordinates are read inside a single render command, because MyGUI lives on the render thread.
         */
        void buildFocusTargets(void);

        /**
         * @brief	Places the focus on the given target index and snaps the mouse onto it.
         */
        void applyFocus(int targetIndex);

        /**
         * @brief	Finds the nearest target in the given screen direction, or -1, if there is none. Only targets inside
         *			a 45 degree cone around the direction are considered, so a row of buttons cannot be left by accident.
         */
        int findNeighbourTarget(int directionX, int directionY) const;

        /**
         * @brief	Reads the mapped UP / DOWN / LEFT / RIGHT actions of the keyboard and of every gamepad.
         * @return	true, if any direction is currently pressed.
         */
        bool readDirection(int& directionX, int& directionY);

        /**
         * @brief	Reads the configured confirm action of the keyboard and of every gamepad.
         */
        bool readConfirm(void);

        /**
         * @brief	Maps the 'Confirm Action' list value to the InputDeviceModule action, returned as int, so that this
         *			header does not need to pull in InputDeviceModule.h.
         */
        int getConfirmActionValue(void) const;

    private:
        Variant* activated;
        Variant* navigationDelay;
        Variant* confirmAction;
        Variant* showPointer;

        std::vector<FocusTarget> focusTargets;
        int focusedIndex;
        Ogre::Real navigationTimer;
        bool confirmWasDown;
        bool needsRefresh;
        bool isSimulating;

        luabind::object focusChangedClosureFunction;
    };

}; // namespace end

#endif