/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef JOYSTICCONFIGURATIONCOMPONENT_H
#define JOYSTICCONFIGURATIONCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{

	/**
	  * @brief		This component can be used as building block in order to have joystick remap functionality. It can be placed as root via the position or using a parent id to be placed as a child in a parent MyGUI window.
	  */
	class EXPORTED JoystickConfigurationComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::JoyStickListener
	{
	public:
		typedef boost::shared_ptr<JoystickConfigurationComponent> JoystickConfigurationComponentPtr;
	public:

		JoystickConfigurationComponent();

		virtual ~JoystickConfigurationComponent();

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
			return NOWA::getIdFromName("JoystickConfigurationComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "JoystickConfigurationComponent";
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
			return "Usage: This component can be used as building block in order to have joystick remap functionality. "
				"It can be placed as root via the position or using a parent id to be placed as a child in a parent MyGUI window. " 
				"Note: It can only be added a game object with already existing InputDeviceComponent.";
		}

		/**
			* @see	GameObjectComponent::createStaticApiForLua
			*/
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

		/**
		 * @brief		Sets the position of the remap config window (0 = top/left, 1 = bottom/right)
		 * @param[in]	relativePosition	The relative position to set.
		 */
		void setRelativePosition(const Ogre::Vector2& relativePosition);

		/**
		 * @brief		Sets the relative position of the remap config window (0 = top/left, 1 = bottom/right)
		 * @return		The current relative position.
		 */
		Ogre::Vector2 getRelativePosition(void) const;

		/**
		 * @brief		Sets the parent id (another MyGUI window) under which this building block component can be placed.
		 * @param[in]	parentId	The parent id to set. May be left of (0), when this building block should be placed as root.
		 */
		void setParentId(unsigned long parentId);

		/**
		 * @brief		Gets the parent MyGUI window id.
		 * @return		The parent MyGUI window id to get or 0, if does not exist.
		 */
		unsigned long getParentId(void) const;

		void setOkClickEventName(const Ogre::String& okClickEventName);

		Ogre::String getOkClickEventName(void) const;

		void setAbordClickEventName(const Ogre::String& abordClickEventName);

		Ogre::String getAbordClickEventName(void) const;

		MyGUI::Window* getWindow(void) const;

		virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;

		virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;

		virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;

		virtual bool sliderMoved(const OIS::JoyStickEvent& evt, int index) override;

		virtual bool povMoved(const OIS::JoyStickEvent& evt, int pov) override;

		virtual bool vector3Moved(const OIS::JoyStickEvent& evt, int index) override;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrRelativePosition(void) { return "Relative Position"; }
		static const Ogre::String AttrParentId(void) { return "Parent Id"; }
		static const Ogre::String AttrOkClickEventName(void) { return "Ok Click Event Name"; }
		static const Ogre::String AttrAbordClickEventName(void) { return "Abord Click Event Name"; }
	private:
		void createMyGuiWidgets(void);

		void destroyMyGUIWidgets(void);

		void notifyMouseSetFocus(MyGUI::Widget* sender, MyGUI::Widget* old);
		void buttonHit(MyGUI::Widget* sender);
	private:
		Ogre::String name;
		MyGUI::VectorWidgetPtr widgets;
		std::vector<bool> textboxActive;
		MyGUI::Window* widget;
		MyGUI::EditBox* messageLabel;
		std::vector<MyGUI::EditBox*> keyConfigLabels;
		std::vector<MyGUI::EditBox*> keyConfigTextboxes;
		std::vector<Ogre::String> oldKeyValue;
		std::vector<Ogre::String> newKeyValue;
		MyGUI::Button* okButton;
		MyGUI::Button* abordButton;
		bool hasParent;
		InputDeviceModule::JoyStickButton lastButton;

		// Action, Keycode
		std::map<unsigned short, int> keyCodes;

		Variant* activated;
		Variant* relativePosition;
		Variant* parentId;
		Variant* okClickEventName;
		Variant* abordClickEventName;
	};

}; // namespace end

#endif

