#ifndef MYGUI_COMPONENTS_H
#define MYGUI_COMPONENTS_H

#include "GameObjectComponent.h"
#include "main/Events.h"
#include "MyGUI.h"
#include "MessageBox/MessageBox.h"

namespace NOWA
{
	class EXPORTED CustomPickingMask
	{
	public:
		CustomPickingMask();

		bool loadFromAlpha(const Ogre::String& filename, int resizeWidth = -1, int resizeHeight = -1);

		// Check if the given point is clickable
		bool pick(const MyGUI::IntPoint& point) const;

		bool empty() const;

	private:
		int width;
		int height;
		Ogre::String newImageFileName;
		Ogre::String oldImageFileName;
		std::vector<bool> mask;  // A 1-bit per pixel mask to track clickable pixels
	};

	/////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<MyGUIComponent> MyGUICompPtr;
	public:

		MyGUIComponent();

		virtual ~MyGUIComponent();

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
		* @see		GameObjectComponent::pause
		*/
		virtual void pause(void) override;

		/**
		* @see		GameObjectComponent::resume
		*/
		virtual void resume(void) override;

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
		* @see		GameObjectComponent::onReordered
		*/
		virtual void onReordered(unsigned int index) override;

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
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) = 0;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MyGUIComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A base MyGUI UI component. Note: Each MyGUI UI component can use the 'mouseClickEventName' in order to specify a function name for being executed in lua script, if the corresponding widget has been clicked.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @brief	Gets called when a skin is changed for a widget
		*/
		virtual void onChangeSkin(void) { };

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		void setRealPosition(const Ogre::Vector2& position);

		Ogre::Vector2 getRealPosition(void) const;

		void setRealSize(const Ogre::Vector2& size);

		Ogre::Vector2 getRealSize(void) const;
		
		void setAlign(const Ogre::String& align);

		Ogre::String getAlign(void) const;
		
		void setLayer(const Ogre::String& layer);

		Ogre::String getLayer(void) const;
		
		void setColor(const Ogre::Vector4& color);

		Ogre::Vector4 getColor(void) const;
		
		void setEnabled(bool enabled);

		bool getEnabled(void) const;

		/**
		* @brief	Gets the unique id of this MyGUI component instance
		* @return	id		The id of this MyGUI component instance
		*/
		unsigned long getId(void) const;
		
		void setParentId(unsigned long parentId);

		unsigned long getParentId(void) const;
		
		void setSkin(const Ogre::String& skin);
		
		Ogre::String getSkin(void) const;
		
		MyGUI::Widget* getWidget(void) const;

		MyGUI::Align mapStringToAlign(const Ogre::String& strAlign);

		unsigned long getPriorId(void) const;

		/**
		 * @brief Lua closure function gets called if mouse button has been clicked.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnMouseButtonClick(luabind::object closureFunction);

		/**
		 * @brief Lua closure function gets called if mouse has entered the given widget (hover start).
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnMouseEnter(luabind::object closureFunction);

		/**
		 * @brief Lua closure function gets called if mouse has left the given widget (hover end).
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnMouseLeave(luabind::object closureFunction);

		void refreshTransform(void);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrPosition(void) { return "Position"; }
		static const Ogre::String AttrSize(void) { return "Size"; }
		static const Ogre::String AttrAlign(void) { return "Align"; }
		static const Ogre::String AttrLayer(void) { return "Layer"; }
		static const Ogre::String AttrColor(void) { return "Color"; }
		static const Ogre::String AttrEnabled(void) { return "Enabled"; }
		static const Ogre::String AttrId(void) { return "Id"; }
		static const Ogre::String AttrParentId(void) { return "Parent Id"; }
		static const Ogre::String AttrSkin(void) { return "Skin"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) = 0;

		virtual void mouseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);

		virtual void mouseButtonDoubleClick(MyGUI::Widget* sender);

		virtual void rootMouseChangeFocus(MyGUI::Widget* sender, bool focus);

		virtual void changeCoord(MyGUI::Widget* sender);
		
		Ogre::String mapAlignToString(MyGUI::Align align);

		void internalSetPriorId(unsigned long priorId);

		MyGUI::Widget* findWidgetAtPosition(MyGUI::Widget* root, const MyGUI::IntPoint& position);

		void onWidgetSelected(MyGUI::Widget* sender);
	private:
		void baseMouseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
		void baseMouseButtonDoubleClick(MyGUI::Widget* sender);
	private:
		void handleWindowChangedDelegate(NOWA::EventDataPtr eventData);
	
	protected:
		bool hasParent;
		bool createWidgetInParent;
		bool oldActivated;
		bool isSimulating;
		unsigned long priorId;
		Ogre::Vector4 oldCoordinate;
		luabind::object mouseButtonClickClosureFunction;
		luabind::object mouseEnterClosureFunction;
		luabind::object mouseLeaveClosureFunction;

		Variant* activated;
		Variant* position;
		Variant* size;
		Variant* align;
		Variant* layer;
		Variant* color;
		Variant* enabled;
		Variant* id;
		Variant* parentId;
		Variant* skin;
		MyGUI::Widget* widget;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIWindowComponent : public MyGUIComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIWindowComponent> MyGUIWindowCompPtr;
	public:

		MyGUIWindowComponent();

		virtual ~MyGUIWindowComponent();

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
			return NOWA::getIdFromName("MyGUIWindowComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIWindowComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI window component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		MyGUIComponent::onChangeSkin
		*/
		virtual void onChangeSkin(void) override;

		void setMovable(bool movable);

		bool getMovable(void) const;

		void setWindowCaption(const Ogre::String& windowCaption);

		Ogre::String getWindowCaption(void) const;
	public:
		static const Ogre::String AttrMovable(void) { return "Movable"; }
		static const Ogre::String AttrWindowCaption(void) { return "WindowCaption"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;
	protected:
		Variant* movable;
		Variant* windowCaption;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUITextComponent : public MyGUIComponent
	{
	public:

		typedef boost::shared_ptr<MyGUITextComponent> MyGUITextCompPtr;
	public:

		MyGUITextComponent();

		virtual ~MyGUITextComponent();

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
			return NOWA::getIdFromName("MyGUITextComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUITextComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI text component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		MyGUIComponent::onChangeSkin
		*/
		virtual void onChangeSkin(void) override;
		
		void setCaption(const Ogre::String& caption);
		
		Ogre::String getCaption(void) const;

		void setFontHeight(unsigned int fontHeight);

		unsigned int getFontHeight(void) const;

		void setTextAlign(const Ogre::String& align);

		Ogre::String getTextAlign(void) const;

		void setTextOffset(const Ogre::Vector2& textOffset);

		Ogre::Vector2 getTextOffset(void) const;
		
		void setTextColor(const Ogre::Vector4& color);

		Ogre::Vector4 getTextColor(void) const;

		void setReadOnly(bool readOnly);

		bool getReadOnly(void) const;

		void setMultiLine(bool multiLine);

		bool getMultiLine(void) const;

		void setWordWrap(bool wordWrap);

		bool getWordWrap(void) const;

		void reactOnEditTextChanged(luabind::object closureFunction);

		void reactOnEditAccepted(luabind::object closureFunction);

	public:
		static const Ogre::String AttrCaption(void) { return "Caption"; }
		static const Ogre::String AttrFontHeight(void) { return "Font Height"; }
		static const Ogre::String AttrTextAlign(void) { return "Text Align"; }
		static const Ogre::String AttrTextOffset(void) { return "Text Offset"; }
		static const Ogre::String AttrTextColor(void) { return "Text Color"; }
		static const Ogre::String AttrReadOnly(void) { return "Read Only"; }
		static const Ogre::String AttrMultiLine(void) { return "Multiline"; }
		static const Ogre::String AttrWordWrap(void) { return "Word wrap"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;

		void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c);

		void onEditTextChanged(MyGUI::EditBox* sender);
		
		void onEditAccepted(MyGUI::EditBox* sender);
	private:
		void initTextAttributes(void);
	protected:
		Variant* caption;
		Variant* fontHeight;
		Variant* textAlign;
		Variant* textOffset;
		Variant* textColor;
		Variant* readOnly;
		Variant* multiLine;
		Variant* wordWrap;

		luabind::object editTextChangedClosureFunction;
		luabind::object editAcceptedClosureFunction;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIButtonComponent : public MyGUIComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIButtonComponent> MyGUIButtonCompPtr;
	public:

		MyGUIButtonComponent();

		virtual ~MyGUIButtonComponent();

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
			return NOWA::getIdFromName("MyGUIButtonComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIButtonComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI button component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		MyGUIComponent::onChangeSkin
		*/
		virtual void onChangeSkin(void) override;
		
		void setCaption(const Ogre::String& caption);
		
		Ogre::String getCaption(void) const;

		void setFontHeight(unsigned int fontHeight);
		
		unsigned int getFontHeight(void) const;

		void setTextAlign(const Ogre::String& align);

		Ogre::String getTextAlign(void) const;

		void setTextOffset(const Ogre::Vector2& textOffset);

		Ogre::Vector2 getTextOffset(void) const;
		
		void setTextColor(const Ogre::Vector4& color);

		Ogre::Vector4 getTextColor(void) const;

	public:
		static const Ogre::String AttrCaption(void) { return "Caption"; }
		static const Ogre::String AttrFontHeight(void) { return "Font Height"; }
		static const Ogre::String AttrTextAlign(void) { return "Text Align"; }
		static const Ogre::String AttrTextOffset(void) { return "Text Offset"; }
		static const Ogre::String AttrTextColor(void) { return "Text Color"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;
	protected:
		Variant* caption;
		Variant* fontHeight;
		Variant* textAlign;
		Variant* textOffset;
		Variant* textColor;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUICheckBoxComponent : public MyGUIComponent
	{
	public:

		typedef boost::shared_ptr<MyGUICheckBoxComponent> MyGUICheckBoxCompPtr;
	public:

		MyGUICheckBoxComponent();

		virtual ~MyGUICheckBoxComponent();

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
			return NOWA::getIdFromName("MyGUICheckBoxComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUICheckBoxComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI check box component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		MyGUIComponent::onChangeSkin
		*/
		virtual void onChangeSkin(void) override;
		
		void setCaption(const Ogre::String& caption);
		
		Ogre::String getCaption(void) const;

		void setChecked(bool checked);

		bool getChecked(void) const;

		void setFontHeight(unsigned int fontHeight);
		
		unsigned int getFontHeight(void) const;

		void setTextAlign(const Ogre::String& align);

		Ogre::String getTextAlign(void) const;

		void setTextOffset(const Ogre::Vector2& textOffset);

		Ogre::Vector2 getTextOffset(void) const;
		
		void setTextColor(const Ogre::Vector4& color);

		Ogre::Vector4 getTextColor(void) const;
	public:
		static const Ogre::String AttrCaption(void) { return "Caption"; }
		static const Ogre::String AttrChecked(void) { return "Checked"; }
		static const Ogre::String AttrFontHeight(void) { return "Font Height"; }
		static const Ogre::String AttrTextAlign(void) { return "Text Align"; }
		static const Ogre::String AttrTextOffset(void) { return "Text Offset"; }
		static const Ogre::String AttrTextColor(void) { return "Text Color"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;
	protected:
		Variant* caption;
		Variant* checked;
		Variant* fontHeight;
		Variant* textAlign;
		Variant* textOffset;
		Variant* textColor;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIImageBoxComponent : public MyGUIComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIImageBoxComponent> MyGUIImageBoxCompPtr;
	public:

		MyGUIImageBoxComponent();

		virtual ~MyGUIImageBoxComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MyGUIImageBoxComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIImageBoxComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI image box component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		MyGUIComponent::onChangeSkin
		*/
		virtual void onChangeSkin(void) override;
		
		void setImageFileName(const Ogre::String& imageFileName);
		
		Ogre::String getImageFileName(void) const;
		
		void setCenter(const Ogre::Vector2& center);
		
		Ogre::Vector2 getCenter(void) const;
		
		void setAngle(Ogre::Real angle);
		
		Ogre::Real getAngle(void) const;
		
		void setRotationSpeed(Ogre::Real rotationSpeed);
		
		Ogre::Real getRotationSpeed(void) const;

		void setUsePickingMask(bool usePickingMask);

		bool getUsePickingMask(void) const;

		MyGUI::RotatingSkin* getRotatingSkin(void) const;

	public:
		static const Ogre::String AttrImageFileName(void) { return "Image Filename"; }
		static const Ogre::String AttrCenter(void) { return "Rotation Center x, y"; }
		static const Ogre::String AttrAngle(void) { return "Angle (Degree)"; }
		static const Ogre::String AttrRotationSpeed(void) { return "Rotation Speed"; }
		static const Ogre::String AttrUsePickingMask(void) { return "UsePickingMask"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;
		virtual void mouseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id) override;

		void callMousePressLuaFunction(void);
	protected:
		MyGUI::RotatingSkin* rotatingSkin;
		CustomPickingMask* pickingMask;

		Variant* imageFileName;
		Variant* center;
		Variant* angle;
		Variant* rotationSpeed;
		Variant* usePickingMask;

	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIProgressBarComponent : public MyGUIComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIProgressBarComponent> MyGUIProgressBarCompPtr;
	public:

		MyGUIProgressBarComponent();

		virtual ~MyGUIProgressBarComponent();

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
			return NOWA::getIdFromName("MyGUIProgressBarComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIProgressBarComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI progress bar component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		MyGUIComponent::onChangeSkin
		*/
		virtual void onChangeSkin(void) override;
		
		void setValue(unsigned int value);
		
		unsigned int getValue(void) const;

		void setRange(unsigned int range);

		unsigned int getRange(void) const;
		
		void setFlowDirection(const Ogre::String& direction);
		
		Ogre::String getFlowDirection(void) const;
	public:
		static const Ogre::String AttrValue(void) { return "Value"; }
		static const Ogre::String AttrRange(void) { return "Range (Max)"; }
		static const Ogre::String AttrFlowDirection(void) { return "Flow Direction"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;
	protected:
		Variant* value;
		Variant* range;
		Variant* flowDirection;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIListBoxComponent : public MyGUIComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIListBoxComponent> MyGUIListBoxCompPtr;
	public:

		MyGUIListBoxComponent();

		virtual ~MyGUIListBoxComponent();

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
			return NOWA::getIdFromName("MyGUIListBoxComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIListBoxComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI list box component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		MyGUIComponent::onChangeSkin
		*/
		virtual void onChangeSkin(void) override;

		void setItemHeight(unsigned int itemHeight);

		unsigned int getItemHeight(void) const;

		void setFontHeight(unsigned int fontHeight);

		unsigned int getFontHeight(void) const;

		void setTextAlign(const Ogre::String& align);

		Ogre::String getTextAlign(void) const;

		void setTextOffset(const Ogre::Vector2& textOffset);

		Ogre::Vector2 getTextOffset(void) const;
		
		void setTextColor(const Ogre::Vector4& color);

		Ogre::Vector4 getTextColor(void) const;

		/**
		 * @brief Lua closure function gets called if a list item has been selected by the given index. 
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnSelected(luabind::object closureFunction);

		/**
		 * @brief Lua closure function gets called if an item has been accepted either via mouse double click or by pressing the enter button by the given index.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnAccept(luabind::object closureFunction);
		
		void setItemCount(unsigned int itemCount);
		
		unsigned int getItemCount(void) const;
		
		void setItemText(unsigned int index, const Ogre::String& itemText);

		Ogre::String getItemText(unsigned int index) const;

		void addItem(const Ogre::String& itemText);

		void insertItem(int index, const Ogre::String& itemText);

		void removeItem(unsigned int index);

		int getSelectedIndex(void) const;

	public:
		static const Ogre::String AttrItemHeight(void) { return "Item Height"; }
		static const Ogre::String AttrFontHeight(void) { return "Font Height"; }
		static const Ogre::String AttrTextAlign(void) { return "Text Align"; }
		static const Ogre::String AttrTextOffset(void) { return "Text Offset"; }
		static const Ogre::String AttrTextColor(void) { return "Text Color"; }
		static const Ogre::String AttrItemCount(void) { return "Item Count"; }
		static const Ogre::String AttrItem(void) { return "Item "; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;

		void listSelectAccept(MyGUI::ListBox* sender, size_t index);
		// Double click or enter pressed
		void listAccept(MyGUI::ListBox* sender, size_t index);
	protected:
		Variant* itemHeight;
		Variant* fontHeight;
		Variant* textAlign;
		Variant* textOffset;
		Variant* textColor;
		Variant* itemCount;
		std::vector<Variant*> items;
		luabind::object selectedClosureFunction;
		luabind::object acceptClosureFunction;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIComboBoxComponent : public MyGUIComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIComboBoxComponent> MyGUIComboBoxCompPtr;
	public:

		MyGUIComboBoxComponent();

		virtual ~MyGUIComboBoxComponent();

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
			return NOWA::getIdFromName("MyGUIComboBoxComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIComboBoxComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI combo box component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		MyGUIComponent::onChangeSkin
		*/
		virtual void onChangeSkin(void) override;

		void setItemHeight(unsigned int itemHeight);

		unsigned int getItemHeight(void) const;

		void setFontHeight(unsigned int fontHeight);

		unsigned int getFontHeight(void) const;

		void setTextAlign(const Ogre::String& align);

		Ogre::String getTextAlign(void) const;

		void setTextOffset(const Ogre::Vector2& textOffset);

		Ogre::Vector2 getTextOffset(void) const;
		
		void setTextColor(const Ogre::Vector4& color);

		Ogre::Vector4 getTextColor(void) const;

		void setModeDrop(bool comboModeDrop);

		bool getModeDrop(void) const;

		void setSmooth(bool smoothShow);
		
		bool getSmooth(void) const;

		void setFlowDirection(const Ogre::String& flowDirection);
		
		Ogre::String getFlowDirection(void) const;

		void setCaption(const Ogre::String& caption);
		
		void setItemCount(unsigned int itemCount);
		
		unsigned int getItemCount(void) const;
		
		void setItemText(unsigned int index, const Ogre::String& itemText);

		Ogre::String getItemText(unsigned int index) const;

		void addItem(const Ogre::String& itemText);

		void insertItem(int index, const Ogre::String& itemText);

		void removeItem(unsigned int index);

		int getSelectedIndex(void) const;

		/**
		 * @brief Lua closure function gets called if a list item has been selected by the given index.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnSelected(luabind::object closureFunction);

	public:
		static const Ogre::String AttrItemHeight(void) { return "Item Height"; }
		static const Ogre::String AttrFontHeight(void) { return "Font Height"; }
		static const Ogre::String AttrTextAlign(void) { return "Text Align"; }
		static const Ogre::String AttrTextOffset(void) { return "Text Offset"; }
		static const Ogre::String AttrTextColor(void) { return "Text Color"; }
		static const Ogre::String AttrItemCount(void) { return "Item Count"; }
		static const Ogre::String AttrItem(void) { return "Item "; }
		static const Ogre::String AttrModeDrop(void) { return "Mode Drop"; }
		static const Ogre::String AttrSmooth(void) { return "Smooth"; }
		static const Ogre::String AttrFlowDirection(void) { return "Flow Direction"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;

		void comboAccept(MyGUI::ComboBox* sender, size_t index);
	protected:
		Variant* itemHeight;
		Variant* fontHeight;
		Variant* textAlign;
		Variant* textOffset;
		Variant* textColor;
		Variant* itemCount;
		Variant* modeDrop;
		Variant* smooth;
		Variant* flowDirection;

		std::vector<Variant*> items;
		luabind::object selectedClosureFunction;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIMessageBoxComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIMessageBoxComponent> MyGUIMessageBoxCompPtr;
	public:

		MyGUIMessageBoxComponent();

		virtual ~MyGUIMessageBoxComponent();

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
			return NOWA::getIdFromName("MyGUIMessageBoxComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIMessageBoxComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI message box component.";
		}

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

		virtual bool isActivated(void) const override;
		
		void setTitle(const Ogre::String& title);

		Ogre::String getTitle(void) const;

		void setMessage(const Ogre::String& message);

		Ogre::String getMessage(void) const;

		void setResultEventName(const Ogre::String& resultEventName);
		
		Ogre::String getResultEventName(void) const;
		
		void setStylesCount(unsigned int stylesCount);
		
		unsigned int getStylesCount(void) const;
		
		void setStyle(unsigned int index, const Ogre::String& style);

		Ogre::String getStyle(unsigned int index) const;

	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTitle(void) { return "Title"; }
		static const Ogre::String AttrMessage(void) { return "Message "; }
		static const Ogre::String AttrResultEventName(void) { return "Result Event Name"; }
		static const Ogre::String AttrStylesCount(void) { return "Styles Count"; }
		static const Ogre::String AttrStyle(void) { return "Style "; }
	protected:
		void notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);
	protected:
		Variant* activated;
		Variant* title;
		Variant* message;
		Variant* resultEventName;
		Variant* style;
		Variant* stylesCount;
		std::vector<Variant*> styles;

		MyGUI::Message* messageBox;
		bool isSimulating;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUITrackComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<MyGUITrackComponent> MyGUITrackCompPtr;
	public:

		MyGUITrackComponent();

		virtual ~MyGUITrackComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
		
		/**
		 * @see		GameObjectComponent::onCloned
		 */
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::pause
		*/
		virtual void pause(void) override;

		/**
		* @see		GameObjectComponent::resume
		*/
		virtual void resume(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MyGUITrackComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUITrackComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI track component. Which makes it possible to track a game object and still remain in overlay space.\n"
				"Note: It must be placed under the game object, which should be tracked by a MyGUI widget. The MyGUI widget's must also be placed under the to be tracked game object.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @brief Activates the tracking and shows the widget.
		 * @param[in] activated If set to true, the game object will be tracked and the widget visible.
		 */
		virtual void setActivated(bool activated);

		/**
		* @brief Gets whether the component behaviour is activated or not.
		* @return activated True, if the components behaviour is activated, else false
		*/
		virtual bool isActivated(void) const override;

		void setCameraId(unsigned long cameraId);

		unsigned long setCameraId(void) const;

		void setWidgetId(unsigned long widgetId);

		unsigned long getWidgetId(void) const;

		void setOffsetPosition(const Ogre::Vector2& offsetPosition);

		Ogre::Vector2 getOffsetPosition(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrCameraId(void) { return "Camera Id"; }
		static const Ogre::String AttrWidgetId(void) { return "Widget Id"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position (x, y)"; }
	protected:
		Variant* activated;
		Variant* cameraId;
		Variant* widgetId;
		Variant* offsetPosition;
		bool oldActivated;
		MyGUI::Widget* widget;
		Ogre::Camera* camera;
		bool isSimulating;
	};

}; //namespace end

#endif