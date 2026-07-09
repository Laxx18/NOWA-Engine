/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef INPUTDEVICECOMPONENT_H
#define INPUTDEVICECOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{

    class InputDeviceModule;

    /**
     * @brief		This component can be used in order to control a player with an available device like keyboard or a joystick.
     *				E.g. using Splitscreen scenario, each player has a player controller and then an InputDeviceComponent, in order to control the player.
     */
    class EXPORTED InputDeviceComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<InputDeviceComponent> InputDeviceComponentPtr;

    public:
        InputDeviceComponent();

        virtual ~InputDeviceComponent();

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
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see		GameObjectComponent::onOtherComponentAdded
         */
        virtual void onOtherComponentAdded(unsigned int index) override;

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
         */
        virtual void setActivated(bool activated) override;

        /**
         * @see		GameObjectComponent::isActivated
         */
        virtual bool isActivated(void) const override;

        void setDeviceName(const Ogre::String& deviceName);

        Ogre::String getDeviceName(void) const;

        void setIsExcluse(bool isExclusive);

        bool getIsExclusive(void) const;

        std::vector<Ogre::String> getActualizedDeviceList(void);

        luabind::object getLuaActualizedDeviceList();

        InputDeviceModule* getInputDeviceModule(void);

        bool checkDevice(const Ogre::String& deviceName);

        bool hasValidDevice(void) const;

        void lockDevice(bool bLockDevice);

        bool isDeviceLocked(void) const;

        /////////////////////////////////////////////////////////////////////////////
        // Forwarding functions to the internal InputDeviceModule, so that lua scripts
        // only ever need to work with InputDeviceComponent and never call
        // getInputDeviceModule() directly. All of these are safe to call even if no
        // device has been assigned yet (they return neutral/default values in that case).
        /////////////////////////////////////////////////////////////////////////////

        bool isKeyboardDevice(void) const;

        int getMappedKey(int action);

        Ogre::String getStringFromMappedKey(int keyCode);

        int getMappedKeyFromString(const Ogre::String& key);

        int getMappedButton(int action);

        Ogre::String getStringFromMappedButton(int joyStickButton);

        void setJoyStickDeadZone(Ogre::Real deadZone);

        bool hasActiveJoyStick(void) const;

        /**
         * @brief		Gets the strength of the left stick horizontal moving
         * @return		strength, if 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].
         */
        Ogre::Real getLeftStickHorizontalMovingStrength(void) const;

        /**
         * @brief		Gets the strength of the left stick vertical moving
         * @return		strength, if 0 vertical stick is not moved. When moved up values are in range (0, 1]. When moved down values are in range (0, -1].
         */
        Ogre::Real getLeftStickVerticalMovingStrength(void) const;

        /**
         * @brief		Gets the strength of the right stick horizontal moving
         * @return		strength, if 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].
         */
        Ogre::Real getRightStickHorizontalMovingStrength(void) const;

        /**
         * @brief		Gets the strength of the right stick vertical moving
         * @return		strength, if 0 vertical stick is not moved. When moved up values are in range (0, 1]. When moved down values are in range (0, -1].
         */
        Ogre::Real getRightStickVerticalMovingStrength(void) const;

        bool isKeyDown(int keyCode) const;

        bool isButtonDown(int button) const;

        bool isActionDown(int action);

        /**
         * @brief		Gets whether a specific action is down, but max. for the action duration time.
         */
        bool isActionDownAmount(int action, Ogre::Real dt, Ogre::Real actionDuration = 0.2f);

        /**
         * @brief		Gets whether a specific action is pressed.
         */
        bool isActionDownPressed(int action, Ogre::Real dt, Ogre::Real durationBetweenTheAction = 0.2f);

        bool areButtonsDown2(int button1, int button2) const;

        bool areButtonsDown3(int button1, int button2, int button3) const;

        bool areButtonsDown4(int button1, int button2, int button3, int button4) const;

        int getPressedButton(void) const;

        luabind::object getLuaPressedButtons(void);

        void setAnalogActionThreshold(Ogre::Real t);

        Ogre::Real getAnalogActionThreshold(void) const;

        Ogre::Real getSteerAxis(void); // [-1..+1]
    public:
        /**
         * @see		GameObjectComponent::getStaticClassId
         */
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("InputDeviceComponent");
        }

        /**
         * @see		GameObjectComponent::getStaticClassName
         */
        static Ogre::String getStaticClassName(void)
        {
            return "InputDeviceComponent";
        }

        /**
         * @see		GameObjectComponent::canStaticAddComponent
         */
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: This component can be used in order to control a player with an available device like keyboard or a joystick. "
                   "E.g. using Splitscreen scenario, each player has a player controller and then an InputDeviceComponent, in order to control the player.";
        }

        /**
         * @see	GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrDeviceName(void)
        {
            return "DeviceName";
        }
        static const Ogre::String AttrIsExclusive(void)
        {
            return "Is Exclusive";
        }

    private:
        void handleInputDeviceOccupied(NOWA::EventDataPtr eventData);

    private:
        InputDeviceModule* inputDeviceModule;
        bool bValidDevice;

        Variant* activated;
        Variant* deviceName;
        Variant* isExclusive;
    };

}; // namespace end

#endif