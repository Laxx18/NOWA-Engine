/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef AREA_OF_INTEREST_COMPONENT_PLUGIN_H
#define AREA_OF_INTEREST_COMPONENT_PLUGIN_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class LuaScriptComponent;

	/*
	* @note: This component has a hard dependency to ActivationComponent and must always be delivered with it!
	*/

	class EXPORTED AreaOfInterestComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<AreaOfInterestComponent> AreaOfInterestCompPtr;
	public:
		/**
		* @class ITriggerSphereQueryObserver
		* @brief This interface can be implemented to react each time a game object enters the sphere area and leaves.
		*/
		class EXPORTED ITriggerSphereQueryObserver
		{
	public:
			/**
			* @brief		Called when a game object enters the sphere area.
			*				Note that this function will be call as long as the game object is in the area at the given update frequency specified in the @initAreaForActiveObjects.
			* @param[in]	gameObject	The game object to manipulate
			*/
			virtual void onEnter(NOWA::GameObject* gameObject) = 0;

			/**
			* @brief		Called when a game object leaves the sphere area.
			*				Note that this function will be called just once.
			* @param[in]	gameObject	The game object to manipulate
			*/
			virtual void onLeave(NOWA::GameObject* gameObject) = 0;
		};
	public:

		AreaOfInterestComponent();

		virtual ~AreaOfInterestComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AreaOfInterestComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AreaOfInterestComponent";
		}

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

		void setRadius(Ogre::Real radius);

		Ogre::Real getRadius(void) const;

		void setCategories(const Ogre::String& categories);

		Ogre::String getCategories(void) const;

		void setUpdateThreshold(Ogre::Real updateThreshold);

		Ogre::Real getUpdateThreshold(void) const;

		/**
		 * @brief Lua closure function gets called in order to react, when a game object enters the area.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnEnter(luabind::object closureFunction);

		/**
		 * @brief Lua closure function gets called in order to react, when a game object leaves the area.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnLeave(luabind::object closureFunction);

		void setShortTimeActivation(bool shortTimeActivation);

		bool getShortTimeActivation(void) const;

		void setTriggerPermanentely(bool triggerPermanentely);

		bool getTriggerPermanentely(void) const;
	public:
		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Triggers when GameObjects from the given category (Can also be combined categories, or all) come into the radius of this GameObject. " 
					"Note: This component can also be used in conjunction with a LuaScriptComponent, in which the corresponding methods: onEnter(gameObject) and onLeave(gameObject) are triggered. "
					"Requirements: A kind of physics active component must exist and a LuaScriptComponent, which references the script file.";
		}

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrShortTimeActivation(void) { return "Short Time Activation"; }
		static const Ogre::String AttrRadius(void) { return "Radius"; }
		static const Ogre::String AttrUpdateThreshold(void) { return "Update Threshold"; }
		static const Ogre::String AttrCategories(void) { return "Categories"; }
		static const Ogre::String AttrTriggerPermanentely(void) { return "Trigger Permanentely"; }
	private:

		/**
		* @brief		Attaches an observer, that is called when a game object enters the area or leaves the area
		* @param[in]	triggerSphereQueryObserver		The trigger sphere query observer to get notified when a game object is in range or get called when a game object leaves the range.
		* @Note											The trigger sphere query observer must be created on a heap, but not destroyed, since it will be destroyed automatically.
		*/
		void attachTriggerObserver(ITriggerSphereQueryObserver* triggerSphereQueryObserver);

		/**
		* @brief		Checks via sphere scene query active game objects in range
		* @param[in]	dt				The delta time
		*/
		void checkAreaForActiveObjects(Ogre::Real dt);

		void deleteGameObjectDelegate(EventDataPtr eventData);

		void logLuaError(const Ogre::String& context, const luabind::error& error);

		void callEnterFunction(GameObject* gameObject);

		void callLeaveFunction(GameObject* gameObject);
	private:
		Ogre::String name;

		Variant* activated;
		Variant* radius;
		Variant* categories;
		Variant* updateThreshold;
		Variant* shortTimeActivation;
		Variant* triggerPermanentely;

		Ogre::SphereSceneQuery* sphereSceneQuery;
		ITriggerSphereQueryObserver* triggerSphereQueryObserver;
		std::unordered_map<unsigned long, std::pair<GameObject*, bool>> triggeredGameObjects;

		Ogre::Real triggerUpdateTimer;
		unsigned int categoriesId;
		LuaScriptComponent* luaScriptComponent;

		luabind::object enterClosureFunction;
		luabind::object leaveClosureFunction;
	};

}; // namespace end

#endif

