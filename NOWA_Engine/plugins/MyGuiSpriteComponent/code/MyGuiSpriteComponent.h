/*
Copyright (c) 2024 Lukas Kalinowski

GPL v3
*/

#ifndef MYGUISPRITECOMPONENT_H
#define MYGUISPRITECOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "gameobject/MyGUIComponents.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	/**
	  * @brief		Usage: An image with sprite tiles can be loaded and manipulated, so that a sprite movement is possible.
	  */
	class EXPORTED MyGuiSpriteComponent : public MyGUIComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<MyGuiSpriteComponent> MyGuiSpriteComponentPtr;
	public:

		MyGuiSpriteComponent();

		virtual ~MyGuiSpriteComponent();

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
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		virtual void setActivated(bool activated) override;

		void setImage(const Ogre::String& image); 

		Ogre::String getImage(void) const;

		void setTileSize(const Ogre::Vector2& tileSize); 

		Ogre::Vector2 getTileSize(void) const;

		void setRowsCols(const Ogre::Vector2& rowsCols); 

		Ogre::Vector2 getRowsCols(void) const;

		void setHorizontal(bool horizontal); 

		bool getHorizontal(void) const;

		void setInverseDirection(bool inverseDirection); 

		bool getInverseDirection(void) const;

		void setSpeed(Ogre::Real speed); 

		Ogre::Real getSpeed(void) const;

		void setStartEndIndex(const Ogre::Vector2& startEndIndex); 

		Ogre::Vector2 getStartEndIndex(void) const;

		void setCurrentRow(unsigned int currentRow);

		unsigned int getCurrentRow(void) const;

		void setCurrentCol(unsigned int currentCol);

		unsigned int getCurrentCol(void) const;

		void setRepeat(bool repeat); 

		bool getRepeat(void) const;

		void setUsePickingMask(bool usePickingMask); 

		bool getUsePickingMask(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MyGuiSpriteComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "MyGuiSpriteComponent";
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
			return "Usage: An image with sprite tiles can be loaded and manipulated, so that a sprite movement is possible.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrImage(void) { return "Image"; }
		static const Ogre::String AttrTileSize(void) { return "TileSize"; }
		static const Ogre::String AttrRowsCols(void) { return "RowsCols"; }
		static const Ogre::String AttrHorizontal(void) { return "Horizontal"; }
		static const Ogre::String AttrInverseDirection(void) { return "InverseDirection"; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
		static const Ogre::String AttrStartEndIndex(void) { return "StartEndIndex"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrUsePickingMask(void) { return "UsePickingMask"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;
		virtual void mouseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id) override;
	private:
		void advanceFrame(void);
	
		void updateFrame(void);

		void callMousePressLuaFunction(void);
	private:
		Ogre::String name;
		unsigned int currentRow;
		unsigned int currentCol;
		Ogre::Real timeSinceLastUpdate;
		int currentFrame;
		bool finished;
		CustomPickingMask* pickingMask;

		Variant* image;
		Variant* tileSize;
		Variant* rowsCols;
		Variant* horizontal;
		Variant* inverseDirection;
		Variant* speed;
		Variant* startEndIndex;
		Variant* repeat;
		Variant* usePickingMask;
	};

}; // namespace end

#endif

