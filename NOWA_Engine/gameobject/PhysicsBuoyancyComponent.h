#ifndef PHYSICS_BUOYANCY_COMPONENT_H
#define PHYSICS_BUOYANCY_COMPONENT_H

#include "PhysicsComponent.h"
#include "OgreNewt_BuoyancyBody.h"

namespace NOWA
{
	class LuaScript;

	class EXPORTED PhysicsBuoyancyComponent : public PhysicsComponent
	{
	public:
		class PhysicsBuoyancyTriggerCallback : public OgreNewt::BuoyancyForceTriggerCallback
		{
		public:
			PhysicsBuoyancyTriggerCallback(GameObject* owner, Ogre::Plane& fluidPlane, Ogre::Real waterToSolidVolumeRatio, Ogre::Real viscosity, 
				const Ogre::Vector3& gravity, Ogre::Real offsetHeight, LuaScript* luaScript, 
										   luabind::object& enterClosureFunction, luabind::object& insideClosureFunction, luabind::object& leaveClosureFunction);

			virtual ~PhysicsBuoyancyTriggerCallback();

			virtual void OnEnter(const OgreNewt::Body* visitor);

			virtual void OnInside(const OgreNewt::Body* visitor);

			virtual void OnExit(const OgreNewt::Body* visitor);

			void setLuaScript(LuaScript* luaScript);

			void setTriggerFunctions(luabind::object& enterClosureFunction, luabind::object& insideClosureFunction, luabind::object& leaveClosureFunction);
		private:
			GameObject* owner;
			LuaScript* luaScript;
			bool onInsideFunctionAvailable;
			luabind::object enterClosureFunction;
			luabind::object insideClosureFunction;
			luabind::object leaveClosureFunction;
		};
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<PhysicsBuoyancyComponent> PhysicsBuoyancyCompPtr;
	public:
	
		PhysicsBuoyancyComponent();

		virtual ~PhysicsBuoyancyComponent();

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
		* @see		GameObjectComponent::showDebugData
		*/
		virtual void showDebugData(void) override;

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
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsBuoyancyComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsBuoyancyComponent";
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
			return "Usage: This Component is used for fluid simulation of physics active components. "
				   "Requirements: A PhysicsArtifactComponent, which acts as the fluid container.";
		}

		static Ogre::String getStaticRequiredCategory(void)
		{
			return "Buoyancy";
		}

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;
		
		/**
		 * @see		PhysicsComponent::isMovable
		 */
		virtual bool isMovable(void) const override
		{
			return false;
		}

		virtual void reCreateCollision(void) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		void setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio);

		Ogre::Real getWaterToSolidVolumeRatio(void) const;

		void setViscosity(Ogre::Real viscosity);

		Ogre::Real getViscosity(void) const;

		void setBuoyancyGravity(const Ogre::Vector3& buoyancyGravity);

		const Ogre::Vector3 getBuoyancyGravity(void) const;

		void setOffsetHeight(Ogre::Real offsetHeight);

		Ogre::Real getOffsetHeight(void) const;

		/**
		 * @brief Sets the lua function name, to react when a game object enters the buoyancy area.
		 * @param[in]	onEnterFunctionName		The function name to set
		 */
		void setOnEnterFunctionName(const Ogre::String& onEnterFunctionName);

		/**
		 * @brief Gets the lua function name.
		 * @return lua function name to get
		 */
		Ogre::String getOnEnterFunctionName(void) const;

		/**
		 * @brief Sets the lua function name, to react when a game object is inside the buoyancy area.
		 * @param[in]	onInsideFunctionName		The function name to set
		 */
		void setOnInsideFunctionName(const Ogre::String& onInsideFunctionName);

		/**
		 * @brief Lua closure function gets called in order to react when a game object enters the trigger area.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnEnter(luabind::object closureFunction);

		/**
		 * @brief Lua closure function gets called in order to react when a game object is inside the trigger area.
		 * @param[in] closureFunction The closure function set.
		 * @note: This function should only be used if really necessary, because this function gets triggered permanently.
		 */
		void reactOnInside(luabind::object closureFunction);

		/**
		 * @brief Lua closure function gets called in order to react when a game object leaves the trigger area.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnLeave(luabind::object closureFunction);
	public:
		static const Ogre::String AttrWaterToSolidVolumeRatio(void) { return "Water To Solid Volume Ratio"; }
		static const Ogre::String AttrViscosity(void) { return "Viscosity"; }
		static const Ogre::String AttrBuoyancyGravity(void) { return "Buoyancy Gravity"; }
		static const Ogre::String AttrOffsetHeight(void) { return "OffsetHeight"; }
	private:
		bool createStaticBody(void);
	protected:
		Variant* waterToSolidVolumeRatio;
		Variant* viscosity;
		Variant* buoyancyGravity;
		Variant* offsetHeight;
		PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback* physicsBuoyancyTriggerCallback;
		bool newleyCreated;
		luabind::object enterClosureFunction;
		luabind::object insideClosureFunction;
		luabind::object leaveClosureFunction;
	};

}; //namespace end

#endif