#ifndef PLAYER_INPUT_COMPONENT_H
#define PLAYER_INPUT_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED PlayerInputComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::PlayerInputComponent> PlayerInputCompPtr;
	public:

		PlayerInputComponent();

		virtual ~PlayerInputComponent();

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
			return NOWA::getIdFromName("PlayerInputComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PlayerInputComponent";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
		
		void setMoveNorthMapping(const Ogre::String& key, const Ogre::String& horizontalJoyStickAxis, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getMoveNorthMapping(void) const;

		void setMoveSouthMapping(const Ogre::String& key, bool enabled);

		std::tuple<Ogre::String, bool> getMoveSouthMapping(void) const;
		
		void setMoveWestMapping(const Ogre::String& key, const Ogre::String& verticalJoyStickAxis, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getMoveWestMapping(void) const;
		
		void setMoveEastMapping(const Ogre::String& key, bool enabled);

		std::tuple<Ogre::String, bool> getMoveEastMapping(void) const;
		
		void setJumpMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getJumpMapping(void) const;
		
		void setAttack1Mapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getAttack1Mapping(void) const;
		
		void setAttack2Mapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getAttack2Mapping(void) const;
		
		void setActionMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getActionMapping(void) const;
		
		void setDuckMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getDuckMapping(void) const;
		
		void setCrouchMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getCrouchMapping(void) const;
		
		void setInventoryMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getInventoryMapping(void) const;
		
		void setMapMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getMapMapping(void) const;
		
		void setPauseMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getPauseMapping(void) const;
		
		void setMenuMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled);

		std::tuple<Ogre::String, Ogre::String, bool> getMenuMapping(void) const;
	public:
		static const Ogre::String AttrMoveNorthKey(void) { return "Move North Key"; }
		static const Ogre::String AttrMoveHorizontalJoyStick(void) { return "Move Horizontal JoyStick"; }
		static const Ogre::String AttrMoveNorthEnabled(void) { return "Move North Enabled"; }
		
		static const Ogre::String AttrMoveSouthKey(void) { return "Move South Key"; }
		static const Ogre::String AttrMoveSouthEnabled(void) { return "Move South Enabled"; }
		
		static const Ogre::String AttrMoveWestKey(void) { return "Move West Key"; }
		static const Ogre::String AttrMoveVerticalJoyStick(void) { return "Move Vertical JoyStick"; }
		static const Ogre::String AttrMoveWestEnabled(void) { return "Move West Enabled"; }
		
		static const Ogre::String AttrMoveEastKey(void) { return "Move East Key"; }
		static const Ogre::String AttrMoveEastEnabled(void) { return "Move East Enabled"; }
		
		static const Ogre::String AttrJumpKey(void) { return "Jump Key"; }
		static const Ogre::String AttrJumpJoyStick(void) { return "Jump JoyStick"; }
		static const Ogre::String AttrJumpEnabled(void) { return "Jump Enabled"; }
		
		static const Ogre::String AttrAttack1Key(void) { return "Attack1 Key"; }
		static const Ogre::String AttrAttack1JoyStick(void) { return "Attack1 JoyStick"; }
		static const Ogre::String AttrAttack1Enabled(void) { return "Attack1 Enabled"; }
		
		static const Ogre::String AttrAttack2Key(void) { return "Attack2 Key"; }
		static const Ogre::String AttrAttack2JoyStick(void) { return "Attack2 JoyStick"; }
		static const Ogre::String AttrAttack2Enabled(void) { return "Attack2 Enabled"; }
		
		static const Ogre::String AttrActionKey(void) { return "Action Key"; }
		static const Ogre::String AttrActionJoyStick(void) { return "Action JoyStick"; }
		static const Ogre::String AttrActionEnabled(void) { return "Action Enabled"; }
		
		static const Ogre::String AttrDuckKey(void) { return "Duck Key"; }
		static const Ogre::String AttrDuckJoyStick(void) { return "Duck JoyStick"; }
		static const Ogre::String AttrDuckEnabled(void) { return "Duck Enabled"; }
		
		static const Ogre::String AttrCrouchKey(void) { return "Crouch Key"; }
		static const Ogre::String AttrCrouchJoyStick(void) { return "Crouch JoyStick"; }
		static const Ogre::String AttrCrouchEnabled(void) { return "Crouch Enabled"; }
		
		static const Ogre::String AttrInventoryKey(void) { return "Inventory Key"; }
		static const Ogre::String AttrInventoryJoyStick(void) { return "Inventory JoyStick"; }
		static const Ogre::String AttrInventoryEnabled(void) { return "Inventory Enabled"; }
		
		static const Ogre::String AttrMapKey(void) { return "Map Key"; }
		static const Ogre::String AttrMapJoyStick(void) { return "Map JoyStick"; }
		static const Ogre::String AttrMapEnabled(void) { return "Map Enabled"; }
		
		static const Ogre::String AttrPauseKey(void) { return "Pause Key"; }
		static const Ogre::String AttrPauseJoyStick(void) { return "Pause JoyStick"; }
		static const Ogre::String AttrPauseEnabled(void) { return "Pause Enabled"; }
		
		static const Ogre::String AttrMenuKey(void) { return "Menu Key"; }
		static const Ogre::String AttrMenuJoyStick(void) { return "Menu JoyStick"; }
		static const Ogre::String AttrMenuEnabled(void) { return "Menu Enabled"; }
	private:
		// RUN missing?
		Variant* moveNorthKey;
		Variant* moveHorizontalJoyStick;
		Variant* moveNorthEnabled;
		
		Variant* separator1;
		Variant* moveSouthKey;
		Variant* moveSouthEnabled;
		
		Variant* separator2;
		Variant* moveWestKey;
		Variant* moveVerticalJoyStick;
		Variant* moveWestEnabled;
		
		Variant* separator3;
		Variant* moveEastKey;
		Variant* moveEastEnabled;
		
		Variant* separator4;
		Variant* jumpKey;
		Variant* jumpJoyStick;
		Variant* jumpEnabled;
		
		Variant* separator5;
		Variant* attack1Key;
		Variant* attack1JoyStick;
		Variant* attack1Enabled;
		
		Variant* separator6;
		Variant* attack2Key;
		Variant* attack2JoyStick;
		Variant* attack2Enabled;
		
		Variant* separator7;
		Variant* actionKey;
		Variant* actionJoyStick;
		Variant* actionEnabled;
		
		Variant* separator8;
		Variant* duckKey;
		Variant* duckJoyStick;
		Variant* duckEnabled;
		
		Variant* separator9;
		Variant* crouchKey;
		Variant* crouchJoyStick;
		Variant* crouchEnabled;
		
		Variant* separator10;
		Variant* inventoryKey;
		Variant* inventoryJoyStick;
		Variant* inventoryEnabled;
		
		Variant* separator11;
		Variant* mapKey;
		Variant* mapJoyStick;
		Variant* mapEnabled;
		
		Variant* separator12;
		Variant* pauseKey;
		Variant* pauseJoyStick;
		Variant* pauseEnabled;
		
		Variant* separator13;
		Variant* menuKey;
		Variant* menuJoyStick;
		Variant* menuEnabled;
	};

}; //namespace end

#endif