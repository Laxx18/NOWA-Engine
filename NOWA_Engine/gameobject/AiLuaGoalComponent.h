#ifndef AI_LUA_GOAL_COMPONENT_H
#define AI_LUA_GOAL_COMPONENT_H

#include "GameObjectComponent.h"
#include "ki/GoalComposite.h"
#include "ki/MovingBehavior.h"
#include "modules/LuaScript.h"
#include "utilities/LuaObserver.h"
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED AiLuaGoalComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<AiLuaGoalComponent> AiLuaGoalCompPtr;
	public:
		AiLuaGoalComponent();

		virtual ~AiLuaGoalComponent();

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
			return NOWA::getIdFromName("AiLuaGoalComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AiLuaGoalComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
		
		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Define lua script tables with the corresponding goals, composite goals and logic. Requirements: A kind of physics active component must exist and a LuaScriptComponent, which references the script file.";
		}

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		void setRootGoalName(const Ogre::String& rootGoalName);

		Ogre::String getRootGoalName(void) const;

		void setRotationSpeed(Ogre::Real rotationSpeed);

		Ogre::Real getRotationSpeed(void) const;

		void setFlyMode(bool flyMode);

		bool getIsInFlyMode(void) const;

		NOWA::KI::LuaGoalComposite<GameObject>* getGoalMachine(void) const;

		NOWA::KI::MovingBehavior* getMovingBehavior(void) const;

		void setRootGoal(const luabind::object& rootGoal);

		void addSubGoal(const luabind::object& subGoal);

		void terminate(void);

		void removeAllSubGoals(void);
#if 0
		void setCurrentState(const luabind::object& currentState);

		void setGlobalState(const luabind::object& globalState);

		void setPreviousState(const luabind::object& previousState);

		void changeState(const luabind::object& newState);

		void exitGlobalState(void);

		void revertToPreviousState(void);

		const luabind::object& getCurrentState(void) const;

		const luabind::object& getPreviousState(void) const;
		
		const luabind::object& getGlobalState(void) const;
#endif

		void reactOnPathGoalReached(luabind::object closureFunction);

		void reactOnAgentStuck(luabind::object closureFunction);
			
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrRotationSpeed(void) { return "Rotation Speed"; }
		static const Ogre::String AttrFlyMode(void) { return "Fly Mode"; }
		static const Ogre::String AttrRootGoalName(void) { return "Root Goal Name"; }
	private:
		void handleLuaScriptConnected(NOWA::EventDataPtr eventData);
	protected:
		Variant* activated;
		Variant* rotationSpeed;
		Variant* flyMode;
		Variant* rootGoalName;
		// Will work with the game object for this component, which is a better base for ai scripting
		NOWA::KI::LuaGoalComposite<GameObject>* luaGoalMachine;
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