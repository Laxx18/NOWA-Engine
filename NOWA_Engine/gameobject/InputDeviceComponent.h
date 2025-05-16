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
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrDeviceName(void) { return "DeviceName"; }
		static const Ogre::String AttrIsExclusive(void) { return "Is Exclusive"; }
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

