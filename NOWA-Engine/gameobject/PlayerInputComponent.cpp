#include "NOWAPrecompiled.h"
#include "PlayerInputComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "main/KeyConfigurator.h"

namespace NOWA
{
	using namespace rapidxml;

	PlayerInputComponent::PlayerInputComponent()
		: GameObjectComponent()
	{
		this->moveNorthKey = new Variant(PlayerInputComponent::AttrMoveNorthKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->moveNorthKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_UP));
		this->moveHorizontalJoyStick = new Variant(PlayerInputComponent::AttrMoveHorizontalJoyStick(), "Left-Horizontal-Axis", this->attributes)); // left, right
		this->moveNorthEnabled = new Variant(PlayerInputComponent::AttrMoveNorthEnabled(), true, this->attributes));
		this->separator1 = new Variant("Separator1", true, this->attributes));
		this->separator1->setUserData("Separator");
		
		this->moveSouthKey = new Variant(PlayerInputComponent::AttrMoveSouthKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->moveSouthKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_DOWN));
		this->moveSouthEnabled = new Variant(PlayerInputComponent::AttrMoveSouthEnabled(), true, this->attributes));
		this->separator2 = new Variant("Separator2", true, this->attributes));
		this->separator2->setUserData("Separator");
		
		this->moveWestKey = new Variant(PlayerInputComponent::AttrMoveWestKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->moveWestKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_LEFT));
		this->moveVerticalJoyStick = new Variant(PlayerInputComponent::AttrMoveVerticalJoyStick(), "Left-Vertical-Axis", this->attributes)); // up, down
		this->moveWestEnabled = new Variant(PlayerInputComponent::AttrMoveWestEnabled(), true, this->attributes));
		this->separator3 = new Variant("Separator3", true, this->attributes));
		this->separator3->setUserData("Separator");
		
		this->moveEastKey = new Variant(PlayerInputComponent::AttrMoveEastKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->moveEastKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_RIGHT));
		this->moveEastEnabled = new Variant(PlayerInputComponent::AttrMoveEastEnabled(), true, this->attributes));
		this->separator4 = new Variant("Separator4", true, this->attributes));
		this->separator4->setUserData("Separator");
		
		this->jumpKey = new Variant(PlayerInputComponent::AttrJumpKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->jumpKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_JUMP));
		this->jumpJoyStick = new Variant(PlayerInputComponent::AttrJumpJoyStick(), "Button1", this->attributes));
		this->jumpEnabled = new Variant(PlayerInputComponent::AttrJumpEnabled(), true, this->attributes));
		this->separator5 = new Variant("Separator5", true, this->attributes));
		this->separator5->setUserData("Separator");
		
		this->attack1Key = new Variant(PlayerInputComponent::AttrAttack1Key(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->attack1Key->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_ATTACK_1));
		this->attack1JoyStick = new Variant(PlayerInputComponent::AttrAttack1JoyStick(), "Button2", this->attributes));
		this->attack1Enabled = new Variant(PlayerInputComponent::AttrAttack1Enabled(), true, this->attributes));
		this->separator6 = new Variant("Separator6", true, this->attributes));
		this->separator6->setUserData("Separator");
		
		this->attack2Key = new Variant(PlayerInputComponent::AttrAttack2Key(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->attack2Key->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_ATTACK_2));
		this->attack2JoyStick = new Variant(PlayerInputComponent::AttrAttack2JoyStick(), "Button3", this->attributes));
		this->attack2Enabled = new Variant(PlayerInputComponent::AttrAttack2Enabled(), true, this->attributes));
		this->separator7 = new Variant("Separator7", true, this->attributes));
		this->separator7->setUserData("Separator");
		
		this->actionKey = new Variant(PlayerInputComponent::AttrActionKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->actionKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_ACTION));
		this->actionJoyStick = new Variant(PlayerInputComponent::AttrActionJoyStick(), "Button4", this->attributes));
		this->actionEnabled = new Variant(PlayerInputComponent::AttrActionEnabled(), true, this->attributes));
		this->separator8 = new Variant("Separator8", true, this->attributes));
		this->separator8->setUserData("Separator");
		
		this->duckKey = new Variant(PlayerInputComponent::AttrDuckKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->duckKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_DUCK));
		this->duckJoyStick = new Variant(PlayerInputComponent::AttrDuckJoyStick(), "Button5", this->attributes));
		this->duckEnabled = new Variant(PlayerInputComponent::AttrDuckEnabled(), true, this->attributes));
		this->separator9 = new Variant("Separator9", true, this->attributes));
		this->separator9->setUserData("Separator");
		
		this->crouchKey = new Variant(PlayerInputComponent::AttrCrouchKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->crouchKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_CROUCH));
		this->crouchJoyStick = new Variant(PlayerInputComponent::AttrCrouchJoyStick(), "Button6", this->attributes));
		this->crouchEnabled = new Variant(PlayerInputComponent::AttrCrouchEnabled(), true, this->attributes));
		this->separator10 = new Variant("Separator10", true, this->attributes));
		this->separator10->setUserData("Separator");
		
		this->inventoryKey = new Variant(PlayerInputComponent::AttrInventoryKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->inventoryKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_INVENTORY));
		this->inventoryJoyStick = new Variant(PlayerInputComponent::AttrInventoryJoyStick(), "Button7", this->attributes));
		this->inventoryEnabled = new Variant(PlayerInputComponent::AttrInventoryEnabled(), true, this->attributes));
		this->separator11 = new Variant("Separator11", true, this->attributes));
		this->separator11->setUserData("Separator");
		
		this->mapKey = new Variant(PlayerInputComponent::AttrMapKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->mapKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_MAP));
		this->mapJoyStick = new Variant(PlayerInputComponent::AttrMapJoyStick(), "Button7", this->attributes));
		this->mapEnabled = new Variant(PlayerInputComponent::AttrMapEnabled(), true, this->attributes));
		this->separator12 = new Variant("Separator12", true, this->attributes));
		this->separator12->setUserData("Separator");
		
		this->pauseKey = new Variant(PlayerInputComponent::AttrPauseKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->pauseKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_PAUSE));
		this->pauseJoyStick = new Variant(PlayerInputComponent::AttrPauseJoyStick(), "Button8", this->attributes));
		this->pauseEnabled = new Variant(PlayerInputComponent::AttrPauseEnabled(), true, this->attributes));
		this->separator13 = new Variant("Separator13", true, this->attributes));
		this->separator13->setUserData("Separator");
		
		this->menuKey = new Variant(PlayerInputComponent::AttrMenuKey(), KeyConfigurator::getInstance()->getAllKeyStrings(), this->attributes));
		this->menuKey->setListSelectedValue(KeyConfigurator::getInstance()->getStringFromKey(NOWA_K_MENU));
		this->menuJoyStick = new Variant(PlayerInputComponent::AttrMenuJoyStick(), "Button9", this->attributes));
		this->menuEnabled = new Variant(PlayerInputComponent::AttrMenuEnabled(), true, this->attributes));
		this->separator14 = new Variant("Separator14", true, this->attributes));
		this->separator14->setUserData("Separator");
		
		// see: https://unity3d.com/de/learn/tutorials/projects/2d-game-kit/ellen
	}

	PlayerInputComponent::~PlayerInputComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerInputComponent] Destructor player input component for game object: " + this->gameObjectPtr->getName());
		
	}

	bool PlayerInputComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		propertyElement = propertyElement->next_sibling("property");
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveNorthKey")
		{
			this->moveNorthKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::UP, KeyConfigurator::getInstance()->getMappedKeyFromString(this->moveNorthKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveHorizontalJoyStick")
		{
			this->moveHorizontalJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveNorthEnabled")
		{
			this->moveNorthEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveSouthKey")
		{
			this->moveSouthKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::DOWN, KeyConfigurator::getInstance()->getMappedKeyFromString(this->moveSouthKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveSouthEnabled")
		{
			this->moveSouthEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveWestKey")
		{
			this->moveWestKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::LEFT, KeyConfigurator::getInstance()->getMappedKeyFromString(this->moveWestKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveVerticalJoyStick")
		{
			this->moveVerticalJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveWestEnabled")
		{
			this->moveWestEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveEastKey")
		{
			this->moveEastKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::RIGHT, KeyConfigurator::getInstance()->getMappedKeyFromString(this->moveEastKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveEastEnabled")
		{
			this->moveEastEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JumpKey")
		{
			this->jumpKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::JUMP, KeyConfigurator::getInstance()->getMappedKeyFromString(this->jumpKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JumpJoyStick")
		{
			this->jumpJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JumpEnabled")
		{
			this->jumpEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Attack1Key")
		{
			this->attack1Key->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::ATTACK_1, KeyConfigurator::getInstance()->getMappedKeyFromString(this->attack1Key->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Attack1JoyStick")
		{
			this->attack1JoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Attack1Enabled")
		{
			this->attack1Enabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Attack2Key")
		{
			this->attack2Key->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::ATTACK_2, KeyConfigurator::getInstance()->getMappedKeyFromString(this->attack2Key->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Attack2JoyStick")
		{
			this->attack2JoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Attack2Enabled")
		{
			this->attack2Enabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ActionKey")
		{
			this->actionKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::ACTION, KeyConfigurator::getInstance()->getMappedKeyFromString(this->actionKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ActionJoyStick")
		{
			this->actionJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ActionEnabled")
		{
			this->actionEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DuckKey")
		{
			this->duckKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::DUCK, KeyConfigurator::getInstance()->getMappedKeyFromString(this->duckKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DuckJoyStick")
		{
			this->duckJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DuckEnabled")
		{
			this->duckEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CrouchKey")
		{
			this->crouchKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::CROUCH, KeyConfigurator::getInstance()->getMappedKeyFromString(this->crouchKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CrouchJoyStick")
		{
			this->crouchJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CrouchEnabled")
		{
			this->crouchEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InventoryKey")
		{
			this->inventoryKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::INVENTORY, KeyConfigurator::getInstance()->getMappedKeyFromString(this->inventoryKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InventoryJoyStick")
		{
			this->inventoryJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InventoryEnabled")
		{
			this->inventoryEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MapKey")
		{
			this->mapKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::MAP, KeyConfigurator::getInstance()->getMappedKeyFromString(this->map->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MapJoyStick")
		{
			this->mapJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MapEnabled")
		{
			this->mapEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PauseKey")
		{
			this->pauseKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::PAUSE, KeyConfigurator::getInstance()->getMappedKeyFromString(this->pauseKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PauseJoyStick")
		{
			this->pauseJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PauseEnabled")
		{
			this->pauseEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MenuKey")
		{
			this->menuKey->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			KeyConfigurator::getInstance()->remapKey(KeyboardAction::MENU, KeyConfigurator::getInstance()->getMappedKeyFromString(this->menuKey->getListSelectedValue());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MenuJoyStick")
		{
			this->menuJoyStick->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MenuEnabled")
		{
			this->menuEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr PlayerInputComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool PlayerInputComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerInputComponent] Init move math function component for game object: " + this->gameObjectPtr->getName());

		return true;
	}
	
	bool PlayerInputComponent::connect(void)
	{
		
		return true;
	}

	void PlayerInputComponent::actualizeValue(Variant* attribute)
	{
		if (PlayerInputComponent::AttrMoveNorthKey() == attribute->getName())
		{
			this->setMoveNorthMapping(attribute->getListSelectedValue(), this->moveHorizontalJoyStick->getListSelectedValue(), this->moveNorthEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMoveHorizontalJoyStick() == attribute->getName())
		{
			this->setMoveNorthMapping(this->moveNorthKey->getListSelectedValue(), attribute->getListSelectedValue(), this->moveNorthEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMoveNorthEnabled() == attribute->getName())
		{
			this->setMoveNorthMapping(this->moveNorthKey->getListSelectedValue(), this->moveHorizontalJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		if (PlayerInputComponent::AttrMoveSouthKey() == attribute->getName())
		{
			this->setMoveSouthMapping(attribute->getListSelectedValue(), this->moveSouthEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMoveSouthEnabled() == attribute->getName())
		{
			this->setMoveSouthMapping(this->moveSouthKey->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrMoveWestKey() == attribute->getName())
		{
			this->setMoveWestMapping(attribute->getListSelectedValue(), this->moveVerticalJoyStick->getListSelectedValue(), this->moveWestEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMoveVerticalJoyStick() == attribute->getName())
		{
			this->setMoveWestMapping(this->moveWestKey->getListSelectedValue(), attribute->getListSelectedValue(), this->moveWestEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMoveWestEnabled() == attribute->getName())
		{
			this->setMoveWestMapping(this->moveWestKey->getListSelectedValue(), this->moveVerticalJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrMoveEastKey() == attribute->getName())
		{
			this->setMoveEastMapping(attribute->getListSelectedValue(), this->moveEastEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMoveEastEnabled() == attribute->getName())
		{
			this->setMoveEastMapping(this->moveEastKey->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrJumpKey() == attribute->getName())
		{
			this->setJumpMapping(attribute->getListSelectedValue(), this->jumpJoyStick->getListSelectedValue(), this->jumpEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrJumpJoyStick() == attribute->getName())
		{
			this->setMoveWestMapping(this->jumpKey->getListSelectedValue(), attribute->getListSelectedValue(), this->jumpEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrJumpEnabled() == attribute->getName())
		{
			this->setJumpMapping(this->jumpKey->getListSelectedValue(), this->jumpJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrAttack1Key() == attribute->getName())
		{
			this->setAttack1Mapping(attribute->getListSelectedValue(), this->attack1JoyStick->getListSelectedValue(), this->attack1Enabled->getBool());
		}
		else if (PlayerInputComponent::AttrAttack1JoyStick() == attribute->getName())
		{
			this->setAttack1Mapping(this->attack1Key->getListSelectedValue(), attribute->getListSelectedValue(), this->attack1Enabled->getBool());
		}
		else if (PlayerInputComponent::AttrAttack1Enabled() == attribute->getName())
		{
			this->setAttack1Mapping(this->attack1Key->getListSelectedValue(), this->attack1JoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrAttack2Key() == attribute->getName())
		{
			this->setAttack2Mapping(attribute->getListSelectedValue(), this->attack2JoyStick->getListSelectedValue(), this->attack2Enabled->getBool());
		}
		else if (PlayerInputComponent::AttrAttack2JoyStick() == attribute->getName())
		{
			this->setAttack2Mapping(this->attack1Key->getListSelectedValue(), attribute->getListSelectedValue(), this->attack2Enabled->getBool());
		}
		else if (PlayerInputComponent::AttrAttack2Enabled() == attribute->getName())
		{
			this->setAttack2Mapping(this->attack2Key->getListSelectedValue(), this->attack2JoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrActionKey() == attribute->getName())
		{
			this->setActionMapping(attribute->getListSelectedValue(), this->actionJoyStick->getListSelectedValue(), this->actionEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrActionJoyStick() == attribute->getName())
		{
			this->setActionMapping(this->actionKey->getListSelectedValue(), attribute->getListSelectedValue(), this->actionEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrActionEnabled() == attribute->getName())
		{
			this->setActionMapping(this->actionKey->getListSelectedValue(), this->actionJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrDuckKey() == attribute->getName())
		{
			this->setDugMapping(attribute->getListSelectedValue(), this->duckJoyStick->getListSelectedValue(), this->duckEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrDuckJoyStick() == attribute->getName())
		{
			this->setDuckMapping(this->duckKey->getListSelectedValue(), attribute->getListSelectedValue(), this->duckEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrDuckEnabled() == attribute->getName())
		{
			this->setDugMapping(this->duckKey->getListSelectedValue(), this->duckJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrCrouchKey() == attribute->getName())
		{
			this->setCrouchMapping(attribute->getListSelectedValue(), this->crouchJoyStick->getListSelectedValue(), this->crouchEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrCrouchJoyStick() == attribute->getName())
		{
			this->setCrouchMapping(this->crouchKey->getListSelectedValue(), attribute->getListSelectedValue(), this->crouchEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrCrouchEnabled() == attribute->getName())
		{
			this->setCrouchMapping(this->crouchKey->getListSelectedValue(), this->crouchJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrInventoryKey() == attribute->getName())
		{
			this->setInventoryMapping(attribute->getListSelectedValue(), this->inventoryJoyStick->getListSelectedValue(), this->inventoryEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrInventoryJoyStick() == attribute->getName())
		{
			this->setInventoryMapping(this->inventoryKey->getListSelectedValue(), attribute->getListSelectedValue(), this->inventoryEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrInventoryEnabled() == attribute->getName())
		{
			this->setInventoryMapping(this->inventoryKey->getListSelectedValue(), this->inventoryJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrMapKey() == attribute->getName())
		{
			this->setMapMapping(attribute->getListSelectedValue(), this->mapJoyStick->getListSelectedValue(), this->mapEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMapJoyStick() == attribute->getName())
		{
			this->setMapMapping(this->mapKey->getListSelectedValue(), attribute->getListSelectedValue(), this->mapEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMapEnabled() == attribute->getName())
		{
			this->setMapMapping(this->mapKey->getListSelectedValue(), this->mapJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrPauseKey() == attribute->getName())
		{
			this->setPauseMapping(attribute->getListSelectedValue(), this->pauseJoyStick->getListSelectedValue(), this->pauseEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrPauseJoyStick() == attribute->getName())
		{
			this->setPauseMapping(this->pauseKey->getListSelectedValue(), attribute->getListSelectedValue(), this->pauseEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrPauseEnabled() == attribute->getName())
		{
			this->setPauseMapping(this->pauseKey->getListSelectedValue(), this->pauseJoyStick->getListSelectedValue(), attribute->getBool());
		}
		
		else if (PlayerInputComponent::AttrMenuKey() == attribute->getName())
		{
			this->setMenuMapping(attribute->getListSelectedValue(), this->menuJoyStick->getListSelectedValue(), this->menuEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMenuJoyStick() == attribute->getName())
		{
			this->setMenuMapping(this->menuKey->getListSelectedValue(), attribute->getListSelectedValue(), this->menuEnabled->getBool());
		}
		else if (PlayerInputComponent::AttrMenuEnabled() == attribute->getName())
		{
			this->setMenuMapping(this->menuKey->getListSelectedValue(), this->menuJoyStick->getListSelectedValue(), attribute->getBool());
		}
	}

	void PlayerInputComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ComponentPlayerInput"));
		propertyXML->append_attribute(doc.allocate_attribute("data", "PlayerInputComponent"));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveNorthKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveNorthKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveHorizontalJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveHorizontalJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveNorthEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveNorthEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveSouthKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveSouthKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveSouthEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveSouthEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveWestKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveWestKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveVerticalJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveVerticalJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveWestEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveWestEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JumpKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->jumpKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JumpJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->jumpJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JumpEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->jumpEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Attack1Key"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->attack1Key->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Attack1JoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->attack1JoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Attack1Enabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->attack1Enabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Attack2Key"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->attack2Key->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Attack2JoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->attack2JoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Attack2Enabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->attack2Enabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ActionKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->actionKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ActionJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->actionJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ActionEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->actionEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DugKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->dugKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DugJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->dugJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DugEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->dugEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CrouchKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->crouchKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CrouchJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->crouchJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CrouchEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->crouchEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InventoryKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->inventoryKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InventoryJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->inventoryJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InventoryEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->inventoryEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MapKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->mapKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MapJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->mapJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MapEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->mapEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PauseKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->pauseKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PauseJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->pauseJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PauseEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->pauseEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MenuKKey"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->menuKey->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MenuJoyStick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->menuJoyStick->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MenuEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->mapEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PlayerInputComponent::getClassName(void) const
	{
		return "PlayerInputComponent";
	}

	Ogre::String PlayerInputComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PlayerInputComponent::setMoveNorthMapping(const Ogre::String& key, const Ogre::String& horizontalJoyStickAxis, bool enabled)
	{
		this->moveNorthKey->setListSelectedValue(key);
		this->moveHorizontalJoyStick->setListSelectedValue(horizontalJoyStickAxis);
		this->moveNorthEnabled->setValue(enabled);
		
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::UP, KeyConfigurator::getInstance()->getMappedKeyFromString(this->moveNorthKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getMoveNorthMapping(void) const
	{
		return std::make_tuple(this->moveNorthKey->getListSelectedValue(), this->moveHorizontalJoyStick->getListSelectedValue(), this->moveNorthEnabled->getBool());
	}

	void PlayerInputComponent::setMoveSouthMapping(const Ogre::String& key, bool enabled)
	{
		this->moveSouthKey->setListSelectedValue(key);
		this->moveSouthEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::DOWN, KeyConfigurator::getInstance()->getMappedKeyFromString(this->moveSouthKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, bool> PlayerInputComponent::getMoveSouthMapping(void) const
	{
		return std::make_tuple(this->moveSouthKey->getListSelectedValue(), this->moveSouthEnabled->getBool());
	}
	
	void PlayerInputComponent::setMoveWestMapping(const Ogre::String& key, const Ogre::String& verticalJoyStickAxis, bool enabled)
	{
		this->moveWestKey->setListSelectedValue(key);
		this->moveVerticalJoyStick->setListSelectedValue(horizontalJoyStickAxis);
		this->moveWestEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::LEFT, KeyConfigurator::getInstance()->getMappedKeyFromString(this->moveWestKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getMoveWestMapping(void) const
	{
		return std::make_tuple(this->moveWestKey->getListSelectedValue(), this->moveVerticalJoyStick->getListSelectedValue(), this->moveWestEnabled->getBool());
	}
	
	void PlayerInputComponent::setMoveEastMapping(const Ogre::String& key, bool enabled)
	{
		this->moveEastKey->setListSelectedValue(key);
		this->moveEastEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::RIGHT, KeyConfigurator::getInstance()->getMappedKeyFromString(this->moveEastKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, bool> PlayerInputComponent::getMoveEastMapping(void) const
	{
		return std::make_tuple(this->moveEastKey->getListSelectedValue(), this->moveEastEnabled->getBool());
	}
	
	void PlayerInputComponent::setJumpMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->jumpKey->setListSelectedValue(key);
		this->jumpJoyStick->setListSelectedValue(joyStickKey);
		this->jumpEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::JUMP, KeyConfigurator::getInstance()->getMappedKeyFromString(this->jumpKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getJumpMapping(void) const
	{
		return std::make_tuple(this->jumpKey->getListSelectedValue(), this->jumpJoyStick->getListSelectedValue(), this->jumpEnabled->getBool());
	}
	
	void PlayerInputComponent::setAttack1Mapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->attack1Key->setListSelectedValue(key);
		this->attack1JoyStick->setListSelectedValue(joyStickKey);
		this->attack1Enabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::ATTACK_1, KeyConfigurator::getInstance()->getMappedKeyFromString(this->attack1Key->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> gPlayerInputComponent::etAttack1Mapping(void) const
	{
		return std::make_tuple(this->attack1Key->getListSelectedValue(), this->attack1JoyStick->getListSelectedValue(), this->attack1Enabled->getBool());
	}
	
	void PlayerInputComponent::setAttack2Mapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->attack2Key->setListSelectedValue(key);
		this->attack2JoyStick->setListSelectedValue(joyStickKey);
		this->attack2Enabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::ATTACK_2, KeyConfigurator::getInstance()->getMappedKeyFromString(this->attack2Key->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getAttack2Mapping(void) const
	{
		return std::make_tuple(this->attack2Key->getListSelectedValue(), this->attack2JoyStick->getListSelectedValue(), this->attack2Enabled->getBool());
	}
	
	void PlayerInputComponent::setActionMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->actionKey->setListSelectedValue(key);
		this->actionJoyStick->setListSelectedValue(joyStickKey);
		this->actionEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::ACTION, KeyConfigurator::getInstance()->getMappedKeyFromString(this->actionKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getActionMapping(void) const
	{
		return std::make_tuple(this->actionKey->getListSelectedValue(), this->actionJoyStick->getListSelectedValue(), this->actionEnabled->getBool());
	}
	
	void PlayerInputComponent::setDuckMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->duckKey->setListSelectedValue(key);
		this->duckJoyStick->setListSelectedValue(joyStickKey);
		this->duckEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::DUCK, KeyConfigurator::getInstance()->getMappedKeyFromString(this->duckKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getDuckMapping(void) const
	{
		return std::make_tuple(this->duckKey->getListSelectedValue(), this->duckJoyStick->getListSelectedValue(), this->duckEnabled->getBool());
	}
	
	void PlayerInputComponent::setCrouchMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->crouchKey->setListSelectedValue(key);
		this->crouchJoyStick->setListSelectedValue(joyStickKey);
		this->crouchEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::CROUCH, KeyConfigurator::getInstance()->getMappedKeyFromString(this->crouchKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getCrouchMapping(void) const
	{
		return std::make_tuple(this->crouchKey->getListSelectedValue(), this->crouchJoyStick->getListSelectedValue(), this->crouchEnabled->getBool());
	}
	
	void PlayerInputComponent::setInventoryMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->inventoryKey->setListSelectedValue(key);
		this->inventoryJoyStick->setListSelectedValue(joyStickKey);
		this->inventoryEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::INVENTORY, KeyConfigurator::getInstance()->getMappedKeyFromString(this->inventoryKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getInventoryMapping(void) const
	{
		return std::make_tuple(this->inventoryKey->getListSelectedValue(), this->inventoryJoyStick->getListSelectedValue(), this->inventoryEnabled->getBool());
	}
	
	void PlayerInputComponent::setMapMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->mapKey->setListSelectedValue(key);
		this->mapJoyStick->setListSelectedValue(joyStickKey);
		this->mapEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::MAP, KeyConfigurator::getInstance()->getMappedKeyFromString(this->mapKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getMapMapping(void) const
	{
		return std::make_tuple(this->mapKey->getListSelectedValue(), this->mapJoyStick->getListSelectedValue(), this->mapEnabled->getBool());
	}
	
	void PlayerInputComponent::setPauseMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->pauseKey->setListSelectedValue(key);
		this->pauseJoyStick->setListSelectedValue(joyStickKey);
		this->pauseEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::PAUSE, KeyConfigurator::getInstance()->getMappedKeyFromString(this->pauseKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getPauseMapping(void) const
	{
		return std::make_tuple(this->pauseKey->getListSelectedValue(), this->pauseJoyStick->getListSelectedValue(), this->pauseEnabled->getBool());
	}
	
	void PlayerInputComponent::setMenuMapping(const Ogre::String& key, const Ogre::String& joyStickKey, bool enabled)
	{
		this->menuKey->setListSelectedValue(key);
		this->menuJoyStick->setListSelectedValue(joyStickKey);
		this->menuEnabled->setValue(enabled);
		KeyConfigurator::getInstance()->remapKey(KeyboardAction::MENU, KeyConfigurator::getInstance()->getMappedKeyFromString(this->menuKey->getListSelectedValue());
	}

	std::tuple<Ogre::String, Ogre::String, bool> PlayerInputComponent::getMenuMapping(void) const
	{
		return std::make_tuple(this->menuKey->getListSelectedValue(), this->menuJoyStick->getListSelectedValue(), this->menuEnabled->getBool());
	}

}; // namespace end