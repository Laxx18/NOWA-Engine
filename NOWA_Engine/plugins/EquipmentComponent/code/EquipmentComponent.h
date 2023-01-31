/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef EQUIPMENTCOMPONENT_H
#define EQUIPMENTCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{

	/**
	  * @brief		Usage: This component can be used to give more specific information about the game objects equipment(s). That is, several things can be equipped.
	  *				Hence give each component a proper name. For example, if the player has a sword, which is an own game object, add for the player an EquipmentComponent with the name
	  *				'RightHandSword' and set the target id to the id of the sword game object. In lua e.g. call player:getEquipmentComponentFromName('RightHandSword'):getTargetId() to get the currently equipped sword.
	  *				You can also use another EquipmentComponent e.g. 'Shoes' to store the currently equipped shoes.
	  */
	class EXPORTED EquipmentComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<EquipmentComponent> EquipmentComponentPtr;
	public:

		EquipmentComponent();

		virtual ~EquipmentComponent();

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
		 * @brief Sets target id for the game object, which is equipped.
		 * @param[in] targetId The target id to set
		 * @note	If target id is set to 0, nothing is equipped anymore.
		 */
		void setTargetId(unsigned long targetId);

		/**
		 * @brief Gets the target id for the game object, which is equipped.
		 * @return targetId The target id to get
		 */
		unsigned long getTargetId(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("EquipmentComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "EquipmentComponent";
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
			return "Usage: This component can be used to give more specific information about the game objects equipment(s). That is, several things can be equipped. "
				"Hence give each component a proper name.For example, if the player has a sword, which is an own game object, add for the player an EquipmentComponent with the name "
				"'RightHandSword' and set the target id to the id of the sword game object.In lua e.g.call player : getEquipmentComponentFromName('RightHandSword') : getTargetId() to get the currently equipped sword. "
				"You can also use another EquipmentComponent e.g. 'Shoes' to store the currently equipped shoes.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
	private:
		Ogre::String name;

		Variant* targetId;
	};

}; // namespace end

#endif

