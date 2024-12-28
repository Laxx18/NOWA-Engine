/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef PICKERCOMPONENT_H
#define PICKERCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"
#include "editor/Picker.h"

namespace NOWA
{

	/**
	  * @brief		This component can be used to pick a game objects with physics component via mouse and manipulate the transform in a spring manner.
	  */
	class EXPORTED PickerComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::MouseListener, public OIS::JoyStickListener
	{
	public:
		typedef boost::shared_ptr<PickerComponent> PickerComponentPtr;
	public:

		PickerComponent();

		virtual ~PickerComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		* @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
		*/
		virtual void initialise() override {};

		/**
		* @see		Ogre::Plugin::shutdown
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void shutdown() override {};

		/**
		* @see		Ogre::Plugin::uninstall
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void uninstall() override {};

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

		/**
		 * @brief Sets target id for the game object, which should be picked and transformed.
		 * @param[in] targetId The target id to set
		 */
		void setTargetId(unsigned long targetId);

		/**
		 * @brief Gets the target id, which is picked and transformed.
		 * @return targetId The target id to get
		 */
		unsigned long getTargetId(void) const;

		/**
		 * @brief Sets instead a joint id to get the physics body for dragging. Useful if a ragdoll with joint rag bones is involved.
		 * @param[in] jointId The joint id to set.
		 */
		void setTargetJointId(unsigned long jointId);

		/**
		 * @brief Sets the physics body pointer for dragging. Useful if a ragdoll with joint rag bones is involved.
		 * @param[in] body The body pointer to set.
		 */
		void setTargetBody(OgreNewt::Body* body);

		/**
		 * @brief Sets an offset position at which the source game object should be picked.
		 * @param[in] offsetPosition The offset position to set
		 */
		void setOffsetPosition(const Ogre::Vector3& offsetPosition);

		/**
		 * @brief Gets the offset position at which the source game object is picked.
		 * @return offsetPosition The offset position to get
		 */
		Ogre::Vector3 getOffsetPosition(void) const;

		/**
		 * @brief Sets the pick strength (spring strength).
		 * @param[in] pickStrength The pick strength to set
		 */
		void setSpringStrength(Ogre::Real springStrength);

		/**
		 * @brief Gets the pick strength (spring strength).
		 * @return pickStrength The pick strength to get
		 */
		Ogre::Real getSpringStrength(void) const;

		/**
		 * @brief Sets the drag affect distance in meters.
		 * @param[in] dragAffectDistance The drag affect distance.
		 */
		void setDragAffectDistance(Ogre::Real dragAffectDistance);

		/**
		 * @brief Gets the drag affect distance in meters.
		 * @return dragAffectDistance The drag affect distance to get
		 */
		Ogre::Real getDragAffectDistance(void) const;

		/**
		 * @brief Sets whether to draw the spring line.
		 * @param[in] drawLine If set to true, a spring line is drawn.
		 */
		void setDrawLine(bool drawLine);

		/**
		 * @brief Gets whether a spring line is drawn.
		 * @return drawLine If true, a spring line is drawn, else false.
		 */
		bool getDrawLine(void) const;

		/**
		 * @brief Sets the mouse button, which shall active the mouse picking.
		 * @param[in] mouseButtonPickId The mouse pick id to set.
		 * @note: InputDeviceCore is involved and id can also be set via lua script.
		 */
		void setMouseButtonPickId(const Ogre::String& mouseButtonPickId);

		/**
		 * @brief Gets the mouse button, which shall active the mouse picking.
		 * @return mouseButtonPickId The mouse pick id to get.
		 */
		Ogre::String getMouseButtonPickId(void) const;

		/**
		 * @brief Sets the joystick button, which shall active the mouse picking.
		 * @param[in] mouseButtonPickId The mouse pick id to set.
		 * @note: InputDeviceCore is involved and id can also be set via lua script.
		 */
		void setJoystickButtonPickId(const Ogre::String& joystickButtonPickId);

		/**
		 * @brief Gets the joystick button, which shall active the mouse picking.
		 * @return joystickButtonPickId The mouse pick id to get.
		 */
		Ogre::String getJoystickButtonPickId(void) const;

		/**
		 * @brief Lua closure function gets called in order to react, when the dragging has started.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnDraggingStart(luabind::object closureFunction);

		/**
		 * @brief Lua closure function gets called in order to react, when the dragging has ended.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnDraggingEnd(luabind::object closureFunction);

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PickerComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "PickerComponent";
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
			return "Usage: This component can be used to pick a game objects with physics component via mouse and manipulate the transform in a spring manner.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position"; }
		static const Ogre::String AttrSpringStrength(void) { return "Spring Strength"; }
		static const Ogre::String AttrDragAffectDistance(void) { return "Drag Affect Distance"; }
		static const Ogre::String AttrDrawLine(void) { return "Draw Line"; }
		static const Ogre::String AttrMouseButtonPickId(void) { return "Mouse Button Pick Id"; }
		static const Ogre::String AttrJoystickButtonPickId(void) { return "Joystick Button Pick Id"; }
	protected:
		protected:
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
			void deleteJointDelegate(EventDataPtr eventData);
			void deleteBodyDelegate(EventDataPtr eventData);
	private:
		Ogre::String name;
		GameObjectPicker* picker;
		OIS::MouseButtonID mouseButtonId;
		int joystickButtonId;
		bool mouseIdPressed;
		bool joystickIdPressed;
		unsigned long jointId;
		OgreNewt::Body* body;

		Variant* activated;
		Variant* targetId;
		Variant* offsetPosition;
		Variant* springStrength;
		Variant* dragAffectDistance;
		Variant* drawLine;
		Variant* mouseButtonPickId;
		Variant* joystickButtonPickId;

		luabind::object startClosureFunction;
		luabind::object endClosureFunction;
		bool draggingStartedFirstTime;
		bool draggingEndedFirstTime;
	};

}; // namespace end

#endif

