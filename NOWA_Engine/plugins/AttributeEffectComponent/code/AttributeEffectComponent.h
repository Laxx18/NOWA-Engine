/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef ATTRIBUTE_EFFECT_COMPONENT_H
#define ATTRIBUTE_EFFECT_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include <fparser.hh>

namespace NOWA
{
	class EXPORTED AttributeEffectComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:

		typedef boost::shared_ptr<NOWA::AttributeEffectComponent> AttributeEffectCompPtr;
	public:

		/**
		 * @class IAttributeEffectObserver
		 * @brief This interface can be implemented to react each on specifig progress of the function.
		 */
		class EXPORTED IAttributeEffectObserver
		{
		public:
			/**
			 * @brief		Called the effect for specific progress value
			 * @param[out]	functionResult	The math function result
			 */
			virtual void onEffect(const Ogre::Vector3& functionResult) = 0;

			/**
			 * @brief		Gets whether the reaction should be done just once.
			 * @return		if true, this observer will be called only once.
			 */
			virtual bool shouldReactOneTime(void) const = 0;
		};

		class EXPORTED AttributeEffectObserver : public IAttributeEffectObserver
		{
		public:
			AttributeEffectObserver(luabind::object closureFunction, bool oneTime);

			virtual ~AttributeEffectObserver();

			/**
			 * @brief		Called at the end of the attribute effect with the result value
			 */
			virtual void onEffect(const Ogre::Vector3& functionResult) override;

			/**
			 * @brief		Gets whether the reaction should be done just once.
			 * @return		if true, this observer will be called only once.
			 */
			virtual bool shouldReactOneTime(void) const override;

		private:
			luabind::object closureFunction;
			bool oneTime;
		};
	public:

		AttributeEffectComponent();

		virtual ~AttributeEffectComponent();

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

		void setAttributeName(const Ogre::String& attributeName);

		Ogre::String getAttributeName(void) const;

		/**
		* @brief			Sets the function for the x component of the move function vector
		* @param[in]		xFunction		The mathematical function for the x component of the vector
		* @note				Some example function:
		*					X-Function: cos(x * 0.1); Y-Function: sin(x * 0.1); Z-Function: 0
		*					X-Function: x * 0.01; Y-Function: 0; Z-Function: sin(x * 0.5); Speed: 20
		*/
		void setXFunction(const Ogre::String& xFunction);

		Ogre::String getXFunction(void) const;

		void setYFunction(const Ogre::String& yFunction);

		Ogre::String getYFunction(void) const;

		void setZFunction(const Ogre::String& zFunction);

		Ogre::String getZFunction(void) const;

		void setSpeed(Ogre::Real speed);

		Ogre::Real getSpeed(void) const;

		void setMinLength(const Ogre::String& minLength);

		Ogre::String getMinLength(void) const;

		void setMaxLength(const Ogre::String& maxLength);

		Ogre::String getMaxLength(void) const;

		void setDirectionChange(bool directionChange);

		bool getDirectionChange(void) const;

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		// Only for lua
		void reactOnEndOfEffect(luabind::object closureFunction, Ogre::Real notificationValue, bool oneTime);

		void reactOnEndOfEffect(luabind::object closureFunction, bool oneTime);
	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AttributeEffectComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "AttributeEffectComponent";
		}

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Manipulates any game object or component's attribute by the given mathematical functions. Important: It always just works for the next prior component! So place it beyond the the be manipulated component!";
		}

		/**
		 * @see		GameObjectComponent::canStaticAddComponent
		 */
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrAttributeName(void) { return "Attribute Name"; }
		static const Ogre::String AttrXFunction(void) { return "X-Function"; }
		static const Ogre::String AttrYFunction(void) { return "Y-Function"; }
		static const Ogre::String AttrZFunction(void) { return "Z-Function"; }
		static const Ogre::String AttrMinLength(void) { return "Min Length"; }
		static const Ogre::String AttrMaxLength(void) { return "Max Length"; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
		static const Ogre::String AttrDirectionChange(void) { return "Direction Change"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
	private:
		void parseMathematicalFunction(void);
	private:
		Ogre::String name;
		Variant* activated;
		Variant* attributeName;
		Variant* xFunction;
		Variant* yFunction;
		Variant* zFunction;
		Variant* minLength;
		Variant* maxLength;
		// Here also autoOrientate?
		Variant* speed;
		Variant* directionChange;
		Variant* repeat;
		Ogre::Real oppositeDir;
		Ogre::Real progress;
		bool internalDirectionChange;
		short round;
		FunctionParser functionParser[5];
		Variant* attributeEffect;
		Variant* oldAttributeEffect;
		GameObjectComponent* targetComponent;
		Ogre::Real notificationValue;
		IAttributeEffectObserver* attributeEffectObserver;
	};

}; //namespace end

#endif