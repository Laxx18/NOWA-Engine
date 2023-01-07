#ifndef AI_LUA_COMPONENT_H
#define AI_LUA_COMPONENT_H

#include "GameObjectComponent.h"
#include "ki/StateMachine.h"
#include "ki/MovingBehavior.h"
#include "modules/LuaScript.h"
#include "utilities/LuaObserver.h"

namespace NOWA
{
	class EXPORTED AiLuaComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<AiLuaComponent> AiLuaCompPtr;
	public:
		AiLuaComponent();

		virtual ~AiLuaComponent();

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
		virtual void onRemoveComponent(void) override;

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AiLuaComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiLuaComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Define lua script tables with the corresponding state and logic. Requirements: A kind of physics active component must exist and a LuaScriptComponent, which references the script file.";
		}

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		void setStartStateName(const Ogre::String& startStateName);

		Ogre::String getStartStateName(void) const;

		void setRotationSpeed(Ogre::Real rotationSpeed);

		Ogre::Real getRotationSpeed(void) const;

		void setFlyMode(bool flyMode);

		bool getIsInFlyMode(void) const;

		NOWA::KI::LuaStateMachine<GameObject>* getStateMachine(void) const;

		NOWA::KI::MovingBehavior* getMovingBehavior(void) const;

		void setCurrentState(const luabind::object& currentState);

		void setGlobalState(const luabind::object& globalState);

		void setPreviousState(const luabind::object& previousState);

		void changeState(const luabind::object& newState);

		void exitGlobalState(void);

		void revertToPreviousState(void);

		const luabind::object& getCurrentState(void) const;

		const luabind::object& getPreviousState(void) const;
		
		const luabind::object& getGlobalState(void) const;

		void reactOnPathGoalReached(luabind::object closureFunction);

		void reactOnAgentStuck(luabind::object closureFunction);
			
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrRotationSpeed(void) { return "Rotation Speed"; }
		static const Ogre::String AttrFlyMode(void) { return "Fly Mode"; }
		static const Ogre::String AttrStartStateName(void) { return "Start State Name"; }
	protected:
		Variant* activated;
		Variant* rotationSpeed;
		Variant* flyMode;
		Variant* startStateName;
		// Will work with the game object for this component, which is a better base for ai scripting
		NOWA::KI::LuaStateMachine<GameObject>* luaStateMachine;
		boost::shared_ptr<NOWA::KI::MovingBehavior> movingBehaviorPtr;
		GameObjectPtr targetGameObject;
		PhysicsActiveComponent* physicsActiveComponent;
		IPathGoalObserver* pathGoalObserver;
		IAgentStuckObserver* agentStuckObserver;
		bool ready;
		bool componentCloned;
	};

}; //namespace end

#endif