/*
Copyright (c) 2022 Lukas Kalinowski

The Massachusetts Institute of Technology ("MIT"), a non-profit institution of higher education, agrees to make the downloadable software and documentation, if any, 
(collectively, the "Software") available to you without charge for demonstrational and non-commercial purposes, subject to the following terms and conditions.

Restrictions: You may modify or create derivative works based on the Software as long as the modified code is also made accessible and for non-commercial usage. 
You may copy the Software. But you may not sublicense, rent, lease, or otherwise transfer rights to the Software. You may not remove any proprietary notices or labels on the Software.

No Other Rights. MIT claims and reserves title to the Software and all rights and benefits afforded under any available United States and international 
laws protecting copyright and other intellectual property rights in the Software.

Disclaimer of Warranty. You accept the Software on an "AS IS" basis.

MIT MAKES NO REPRESENTATIONS OR WARRANTIES CONCERNING THE SOFTWARE, 
AND EXPRESSLY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT 
LIMITATION ANY EXPRESS OR IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS. 

MIT has no obligation to assist in your installation or use of the Software or to provide services or maintenance of any type with respect to the Software.
The entire risk as to the quality and performance of the Software is borne by you. You acknowledge that the Software may contain errors or bugs. 
You must determine whether the Software sufficiently meets your requirements. This disclaimer of warranty constitutes an essential part of this Agreement.
*/

#ifndef JOYSTICKREMAPCOMPONENT_H
#define JOYSTICKREMAPCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{

	/**
	  * @brief		This component can be used as building block in order to have joystick remap functionality. It can be placed as root via the position or using a parent id to be placed as a child in a parent MyGUI window.
	  */
	class EXPORTED JoystickRemapComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::JoyStickListener
	{
	public:
		typedef boost::shared_ptr<JoystickRemapComponent> JoystickRemapComponentPtr;
	public:

		JoystickRemapComponent();

		virtual ~JoystickRemapComponent();

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
			return NOWA::getIdFromName("JoystickRemapComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "JoystickRemapComponent";
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
			return "Usage: This component can be used as building block in order to have joystick remap functionality. It can be placed as root via the position or using a parent id to be placed as a child in a parent MyGUI window. Note: It can only be added under a MainGameObject.";
		}

		/**
			* @see	GameObjectComponent::createStaticApiForLua
			*/
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

		/**
		 * @brief		Gets the device name.
		 * @return		The device name to get.
		 */
		Ogre::String getDeviceName(void) const;

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
		static const Ogre::String AttrDeviceName(void) { return "Device Name"; }
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
		bool bIsInSimulation;

		Variant* activated;
		Variant* deviceName;
		Variant* relativePosition;
		Variant* parentId;
		Variant* okClickEventName;
		Variant* abordClickEventName;
	};

}; // namespace end

#endif

