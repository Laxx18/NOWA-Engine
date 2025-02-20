#ifndef LUA_SCRIPT_COMPONENT_H
#define LUA_SCRIPT_COMPONENT_H

#include "GameObjectComponent.h"
#include "modules/LuaScript.h"
#include "main/Events.h"

namespace NOWA
{
	/**
	 * @class 	LuaScriptComponent
	 * @brief 	This component can be used to attach a lua script to the given owner game object in order develop custom behavior in Lua during runtime without the need to re-compile something.
	 *			This opens a new way of development, as die NOWA-Engine will remain unattached, but developers, designers may add custom behavior
	 *			Info: Only one component can be attached to the owner game object.
	 *			Example: Control a ball via script.
	 *			Requirements: None
	 * 			Lua Interface Functions when LuaScriptComponent is involved: 
	 *				connect(ownerGameObject)
	 *				disconnect(ownerGameObject)
	 *				update(dt)
	 */
	class EXPORTED LuaScriptComponent : public GameObjectComponent
	{
	public:
		enum eScriptAction
		{
			NEW = 0,
			LOAD = 1,
			CLONE = 2,
			RENAME = 3,
			SET = 4,
			WRITE_XML = 5
		};

		friend class GameObjectController;

	public:

		typedef boost::shared_ptr<NOWA::LuaScriptComponent> LuaScriptCompPtr;
	public:

		LuaScriptComponent();

		virtual ~LuaScriptComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;
		
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
			return NOWA::getIdFromName("LuaScriptComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LuaScriptComponent";
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
			return "Usage: Creates a lua script in the project folder for this game object for powerful script manipulation, game mechanics etc."
					"Several game object may use the same script, if the script name for the lua script components is the same. This is usefull if they should behave the same. This also increases the performance";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		void lateUpdate(Ogre::Real dt, bool notSimulating);

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @brief Sets whether the lua script is activated or not
		* @param[in] if set to true, the lua script will be compiled and executed when @connect is called.
		*/
		virtual void setActivated(bool activated) override;
		
		/**
		* @brief Gets whether the lua script  is activated or not.
		* @return activated True, if the lua script is activated, else false
		*/
		virtual bool isActivated(void) const override;

		/**
		 * @brief Sets the lua script file for storage.
		 * @param[in] scriptFile The lua script file to set
		 * @param[in] scriptAction The script action to use (new, load, clone, rename)
		 * @note		The script is created in the same directory as the current scene.
		 */
		void setScriptFile(const Ogre::String& scriptFile, eScriptAction scriptAction);

		/**
		 * @brief Gets the currently used lua script file name.
		 * @return scriptFile The lua script file name to get
		 */
		Ogre::String getScriptFile(void) const;

		/**
		* @brief Sets whether also to clone the lua script, if this component gets cloned.
		* @param[in]	cloneLuaScript		Whether to clone the lua script or not.
		* @note			Cloning a lua script may be dangerous, because its content of this original component will also be cloned and executed.
		*/
		void setCloneScript(bool cloneScript);

		/**
		* @brief Gets whether the lua script will also be cloned and executed.
		* @return cloneScript true, if lua script will be cloned, else false
		*/
		bool getCloneScript(void) const;

		/**
		* @brief Sets whether this lua script component has a common script. That is, when the game object is cloned, the lua script components are cloned too, but referencing the original lua script file.
		* @param[in]	commonScript		Whether this lua script component has a common script.
		* @note			Several game object may use the same script, if the script name for the lua script components is the same. This is usefull if they should behave the same. This also increases the performance.
		*/
		void setHasCommonScript(bool commonScript);

		/**
		* @brief Gets whether this lua script component has a common script. That is, when the game object is cloned, the lua script components are cloned too, but referencing the original lua script file.
		* @return	True, if this lua script component has a common script, else false.
		*/
		bool getHasCommonScript(void) const;

		/**
		* @brief Gets the order index, at which this lua script is executed.
		* @return	the order index to get.
		*/
		int getOrderIndex(void) const;

		/**
		 * @brief Gets the lua script pointer to work directly with.
		 * @return luaScript The lua script pointer to get
		 */
		LuaScript* getLuaScript(void) const;

		/**
		* @brief Calls a lua method after a delay in seconds.
		* @param[in]	closureFunction	The closure function to call.
		* @param[in]	delaySec		The delay in seconds.
		*/
		void callDelayedMethod(luabind::object closureFunction, Ogre::Real delaySec);

		// void triggerEvent(const Ogre::String& eventName, )

		/**
		 * @brief Compiles the script
		 * @return success true, if the script has no compile errors, else false
		 */
		bool compileScript(void);

		/**
		 * @brief Tells the component, that its being cloned and shall not create the script. (Getting cloned script).
		 */
		void setComponentCloned(bool componentCloned);

	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrScriptFile(void) { return "Lua Script Name"; }
		static const Ogre::String AttrCloneScript(void) { return "Clone Script"; }
		static const Ogre::String AttrHasCommonScript(void) { return "Has Common Script"; }
		static const Ogre::String AttrOrderIndex(void) { return "Order Index"; }
	private:
		void maybeCopyScriptFromGroups(void);

		void setOrderIndex(int orderIndex);
	private:
		void handleGroupLoaded(NOWA::EventDataPtr eventData);
	private:
		Variant* activated;
		Variant* scriptFile;
		Variant* cloneScript;
		Variant* commonScript;
		Variant* orderIndex;
		LuaScript* luaScript;
		bool hasUpdateFunction;
		bool hasLateUpdateFunction;
		bool componentCloned;
		Ogre::String differentScriptNameForXML;
	};

}; //namespace end

#endif