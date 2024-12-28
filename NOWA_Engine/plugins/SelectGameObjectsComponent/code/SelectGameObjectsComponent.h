/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef SELECTGAMEOBJECTSCOMPONENT_H
#define SELECTGAMEOBJECTSCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"
#include "editor/SelectionManager.h"

namespace NOWA
{

	/**
	  * @brief		This component can be used for more complex game object selection behavior, like selection rectangle, multi selection etc. It must be placed under a main game object.
	  */
	class EXPORTED SelectGameObjectsComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::KeyListener, public OIS::MouseListener, public OIS::JoyStickListener
	{
	public:
		typedef boost::shared_ptr<SelectGameObjectsComponent> SelectGameObjectsComponentPtr;
	public:

		SelectGameObjectsComponent();

		virtual ~SelectGameObjectsComponent();

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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setCategories(const Ogre::String& categories);

		Ogre::String getCategories(void) const;

		void setUseMultiSelection(bool useMultiSelection);

		bool getUseMultiSelection(void) const;

		void setUseSelectionRectangle(bool useSelectionRectangle);

		bool getUseSelectionRectangle(void) const;

		/**
		 * @brief Lua closure function gets called one or more game objects are selected.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnGameObjectsSelected(luabind::object closureFunction);

		void select(unsigned long gameObjectId, bool bSelect);

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("SelectGameObjectsComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "SelectGameObjectsComponent";
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
			return "Usage: This component can be used for more complex game object selection behavior, like selection rectangle, multi selection etc. It must be placed under a main game object.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrCategories(void) { return "Categories"; }
		static const Ogre::String AttrUseMultiSelection(void) { return "Use Multi Selection"; }
		static const Ogre::String AttrUseSelectionRectangle(void) { return "Use Selection Rectangle"; }
	protected:
		/**
		 * @brief		Actions on key pressed event.
		 * @see			OIS::KeyListener::keyPressed()
		 * @param[in]	keyEventRef		The key event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;

		/**
		 * @brief		Actions on key released event.
		 * @see			OIS::KeyListener::keyReleased()
		 * @param[in]	keyEventRef		The key event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

		/**
		 * @brief		Actions on mouse moved event.
		 * @see			OIS::MouseListener::mouseMoved()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

		/**
		 * @brief		Actions on mouse pressed event.
		 * @see			OIS::MouseListener::mousePressed()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

		/**
		 * @brief		Actions on mouse released event.
		 * @see			OIS::MouseListener::mouseReleased()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

		/**
		 * @brief		Actions on joyStick axis moved event.
		 * @see			OIS::JoyStickListener::axisMoved()
		 * @param[in]	evt		The joyStick event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;

		/**
		 * @brief		Actions on joyStick button pressed event.
		 * @see			OIS::JoyStickListener::buttonPressed()
		 * @param[in]	evt		The joyStick event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;

		/**
		 * @brief		Actions on joyStick button released event.
		 * @see			OIS::JoyStickListener::buttonReleased()
		 * @param[in]	evt		The joyStick event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;
	private:
		Ogre::String name;
		SelectionManager* selectionManager;
		luabind::object closureFunction;

		Variant* activated;
		Variant* categories;
		Variant* useMultiSelection;
		Variant* useSelectionRectangle;

	};

}; // namespace end

#endif

