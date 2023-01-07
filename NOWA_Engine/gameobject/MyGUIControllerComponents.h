#ifndef MYGUI_CONTROLLER_COMPONENTS_H
#define MYGUI_CONTROLLER_COMPONENTS_H

#include "GameObjectComponent.h"
#include "MyGUI.h"

namespace NOWA
{
	class MyGUIComponent;
	
	class EXPORTED MyGUIControllerComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIControllerComponent> MyGUIControllerCompPtr;
	public:

		MyGUIControllerComponent();

		virtual ~MyGUIControllerComponent();

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
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

		/**
		* @see		GameObjectComponent::onOtherComponentRemoved
		*/
		void onOtherComponentRemoved(unsigned int index) override;

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
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MyGUIControllerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIControllerComponent";
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
			return "Usage: A base MyGUI UI controller component.";
		}

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
		void setActivated(bool activated);

		virtual bool isActivated(void) const override;
		
		/**
		 * @brief Sets source id for the component of this game object that should use this controller.
		 * @param[in] sourceId The sourceId to set
		 */
		void setSourceId(unsigned long sourceId);

		/**
		 * @brief Gets the source id for the component of this game object that uses this controller
		 * @return sourceId The sourceId to get
		 */
		unsigned long getSourceId(void) const;
		
		MyGUI::ControllerItem* getControllerItem(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrSourceId(void) { return "Source Id"; }
	protected:
		virtual void controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller) { };

		void activateController(bool bActivate);
	protected:
		Variant* activated;
		Variant* sourceId;
		MyGUI::ControllerItem* controllerItem;
		MyGUI::Widget* sourceWidget;
		MyGUIComponent* sourceMyGUIComponent;
		bool isInSimulation;
		Ogre::Real elapsedTime;
		Ogre::Vector4 oldCoordinate;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIPositionControllerComponent : public MyGUIControllerComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIPositionControllerComponent> MyGUIPositionControllerCompPtr;
	public:

		MyGUIPositionControllerComponent();

		virtual ~MyGUIPositionControllerComponent();

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
			return NOWA::getIdFromName("MyGUIPositionControllerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIPositionControllerComponent";
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
			return "Usage: A MyGUI UI position controller component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		void setCoordinate(const Ogre::Vector4& coordinate);
		
		Ogre::Vector4 getCoordinate(void) const;

		void setFunction(const Ogre::String& function);

		Ogre::String getFunction(void) const;
		
		void setDurationSec(Ogre::Real durationSec);
		
		Ogre::Real getDurationSec(void) const;
	public:
		static const Ogre::String AttrCoordinate(void) { return "Coordinate Rel. (x, y, w, h)"; }
		static const Ogre::String AttrFunction(void) { return "Function"; }
		static const Ogre::String AttrDurationSec(void) { return "Duration (Sec)"; }
	protected:
		virtual void controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller) override;
	private:
		Variant* coordinate;
		Variant* function;
		Variant* durationSec;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIFadeAlphaControllerComponent : public MyGUIControllerComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIFadeAlphaControllerComponent> MyGUIFadeAlphaControllerCompPtr;
	public:

		MyGUIFadeAlphaControllerComponent();

		virtual ~MyGUIFadeAlphaControllerComponent();

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
			return NOWA::getIdFromName("MyGUIFadeAlphaControllerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIFadeAlphaControllerComponent";
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
			return "Usage: A MyGUI UI fade alpha controller component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		void setAlpha(Ogre::Real alpha);
		
		Ogre::Real getAlpha(void) const;

		void setCoefficient(Ogre::Real coefficient);

		Ogre::Real getCoefficient(void) const;
	public:
		static const Ogre::String AttrAlpha(void) { return "Alpha"; }
		static const Ogre::String AttrCoefficient(void) { return "Coefficient"; }
	protected:
		void controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller);
	private:
		Variant* alpha;
		Variant* coefficient;
		Ogre::Real oldAlpha;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIScrollingMessageControllerComponent : public MyGUIControllerComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIScrollingMessageControllerComponent> MyGUIScrollingMessageControllerCompPtr;
	public:

		MyGUIScrollingMessageControllerComponent();

		virtual ~MyGUIScrollingMessageControllerComponent();

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
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override;
		
		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

		/**
		* @see		GameObjectComponent::onOtherComponentRemoved
		*/
		void onOtherComponentRemoved(unsigned int index) override;

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
			return NOWA::getIdFromName("MyGUIScrollingMessageControllerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIScrollingMessageControllerComponent";
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
			return "Usage: A MyGUI UI scrolling message controller component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @brief Sets the message count (how many message should be scrolled and faded).
		 * @param[in] messageCount The message count to set
		 */
		void setMessageCount(unsigned int messageCount);

		/**
		 * @brief Gets the message count
		 * @return messageCount The message count
		 */
		unsigned int getMessageCount(void) const;
		
		void setMessage(unsigned int index, const Ogre::String& message);

		Ogre::String getMessage(unsigned int index) const;
		
		void setDurationSec(Ogre::Real durationSec);
		
		Ogre::Real getDurationSec(void) const;
	public:
		static const Ogre::String AttrMessageCount(void) { return "Message Count"; }
		static const Ogre::String AttrMessage(void) { return "Message "; }
		static const Ogre::String AttrDurationSec(void) { return "Duration (Sec)"; }
	protected:
		virtual void controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller) override { };
	private:
		Variant* messageCount;
		std::vector<Variant*> messages;
		Variant* durationSec;
		Ogre::Real timeToGo;
		bool justAdded;
		std::vector<MyGUI::EditBox*> editBoxes;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIEdgeHideControllerComponent : public MyGUIControllerComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIEdgeHideControllerComponent> MyGUIEdgeHideControllerCompPtr;
	public:

		MyGUIEdgeHideControllerComponent();

		virtual ~MyGUIEdgeHideControllerComponent();

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
			return NOWA::getIdFromName("MyGUIEdgeHideControllerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIEdgeHideControllerComponent";
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
			return "Usage: This controller is used for hiding widgets near screen edges. Widget will start hiding(move out of screen) if it's near border and it and it's childrens don't have any focus. "
				"Hiding till only small part of widget be visible. Widget will move inside screen if it have any focus.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		void setTimeSec(Ogre::Real time);
		
		Ogre::Real getTimeSec(void) const;

		void setRemainPixels(unsigned int remainPixels);

		unsigned int getRemainPixels(void) const;
		
		void setShadowSize(unsigned int shadowSize);

		unsigned int getShadowSize(void) const;
	public:
		static const Ogre::String AttrTime(void) { return "Time (Sec)"; }
		static const Ogre::String AttrRemainPixels(void) { return "Remain Pixels"; }
		static const Ogre::String AttrShadowSize(void) { return "Shadow Size"; }
	protected:
		virtual void controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller) override;
	private:
		Variant* time;
		Variant* remainPixels;
		Variant* shadowSize;
	};
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED MyGUIRepeatClickControllerComponent : public MyGUIControllerComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIRepeatClickControllerComponent> MyGUIRepeatClickControllerCompPtr;
	public:

		MyGUIRepeatClickControllerComponent();

		virtual ~MyGUIRepeatClickControllerComponent();

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
			return NOWA::getIdFromName("MyGUIRepeatClickControllerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIRepeatClickControllerComponent";
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
			return "Usage: A MyGUI UI repeat click controller component.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		void setTimeLeftSec(Ogre::Real timeLeft);
		
		Ogre::Real getTimeLeftSec(void) const;

		void setStep(Ogre::Real step);

		Ogre::Real getStep(void) const;
	public:
		static const Ogre::String AttrTimeLeft(void) { return "Time Left (Sec)"; }
		static const Ogre::String AttrStep(void) { return "Step"; }
	protected:
		virtual void controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller) override;
	private:
		Variant* timeLeft;
		Variant* step;
	};

}; //namespace end

#endif