#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "NOWAPrecompiled.h"
#include "defines.h"
#include <map>

extern "C"
{
#include <lua.h>
}
#include <luabind/luabind.hpp>

namespace NOWA
{

	namespace KI
	{
		template<class Owner>
		class IState;

		template <class Owner>
		class EXPORTED StateMachine
		{
		public:

			StateMachine(Owner* owner)
				: owner(owner),
				currentState(nullptr),
				previousState(nullptr),
				childState(nullptr)
			{

			}

			~StateMachine()
			{
				auto& it = this->states.begin();
				while (it != this->states.end())
				{
					delete it->second;
					it->second = nullptr;
					it = this->states.erase(it);
				}
			}

			//Use this methods to initialise the FSM
			void setCurrentState(const Ogre::String& name)
			{
				auto it = this->states.find(name);
				assert(it != this->states.end() && "[stateMachine::setCurrentState] Illegal state for name!");
				this->currentState = it->second;

				this->currentState->enter(this->owner);
			}

			void setPreviousState(const Ogre::String& name)
			{
				auto it = this->states.find(name);
				assert(it != this->states.end() && "[stateMachine::setPreviousState] Illegal state for name!");
				this->previousState = it->second;
			}

			void setChildState(const Ogre::String& name)
			{
				auto it = this->states.find(name);
				assert(it != this->states.end() && "[stateMachine::setchildState] Illegal state for name!");
				this->childState = it->second;

				this->childState->enter(this->owner);
			}

			//Call this to update the FSM
			void update(Ogre::Real dt)
			{
				//If a global state exists, call its update method;
				if (this->childState)
				{
					this->childState->update(this->owner, dt);
				}
				//Same for the current state
				if (this->currentState)
				{
					this->currentState->update(this->owner, dt);
				}
			}

			void handleKeyInput(int keyCode, bool down)
			{
				//If a global state exists, call its handle key input method;
				if (this->childState)
				{
					this->childState->handleKeyInput(this->owner, keyCode, down);
				}
				//Same for the current state
				if (this->currentState)
				{
					this->currentState->handleKeyInput(this->owner, keyCode, down);
				}
			}

			void handleMouseInput(int mouseButton, bool down, int x, int y, int z = 0)
			{
				//If a global state exists, call its handle mouse input method;
				if (this->childState)
				{
					this->childState->handleMouseInput(this->owner, mouseButton, down, x, y, z);
				}
				//Same for the current state
				if (this->currentState)
				{
					this->currentState->handleMouseInput(this->owner, mouseButton, down, x, y, z);
				}
			}

			//Change to a new state
			void changeState(const Ogre::String& name, int keyCode, bool down)
			{
				auto it = this->states.find(name);
				assert(it != this->states.end() && "[stateMachine::changeState] Illegal state for name!");

				//Keep a record of the previous state
				this->previousState = this->currentState;

				//Call the exit method of the existing state
				this->currentState->exit(this->owner);

				//change state to the new state
				this->currentState = it->second;

				//call the entry method of the new state
				this->currentState->enter(this->owner);
				if (keyCode != -1 && down)
				{
					this->currentState->handleKeyInput(this->owner, keyCode, down);
				}
			}

			//Change to a new state
			void changeState(const Ogre::String& name, int mouseButton, bool down, int x, int y, int z = 0)
			{
				auto it = this->states.find(name);
				assert(it != this->states.end() && "[stateMachine::changeState] Illegal state for name!");
				//Keep a record of the previous state
				this->previousState = this->currentState;

				//Call the exit method of the existing state
				this->currentState->exit(this->owner);

				//change state to the new state
				this->currentState = it->second;

				//call the entry method of the new state
				this->currentState->enter(this->owner);
				if (mouseButton != -1 && down)
				{
					this->currentState->handleMouseInput(this->owner, mouseButton, down, x, y, z);
				}
			}

			void changeState(IState<Owner>* newState)
			{
				//Keep a record of the previous state
				this->previousState = this->currentState;

				//Call the exit method of the existing state
				this->currentState->exit(this->owner);

				//change state to the new state
				this->currentState = newState;

				//call the entry method of the new state
				this->currentState->enter(this->owner);
			}

			void changeState(const Ogre::String& name)
			{
				auto it = this->states.find(name);
				assert(it != this->states.end() && "[stateMachine::changeState] Illegal state for name!");

				//Keep a record of the previous state
				this->previousState = this->currentState;

				//Call the exit method of the existing state
				this->currentState->exit(this->owner);

				//change state to the new state
				this->currentState = it->second;

				//call the entry method of the new state
				this->currentState->enter(this->owner);
			}

			void endChildState(void)
			{
				//Keep a record of the previous state
				this->previousState = this->currentState;

				//Call the exit method of the existing state
				this->childState->exit(this->owner);

				//change state to the new state
				this->childState = nullptr;
			}

			//Change state back to the previous state
			void revertToPreviousState(void)
			{
				this->changeState(this->previousState);
			}

			//Returns true if the current state's type is equal to the type of the
			//class passed as a parameter. For example if the SleepState is equal the WalkState class
			bool isInState(const Ogre::String& name) const
			{
				auto it = this->states.find(name);
				assert(it != this->states.end() && "[stateMachine::changeState] Illegal state for name!");
				if (this->currentState == it->second)
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			IState<Owner>* getCurrentState(void)  const
			{
				return this->currentState;
			}

			IState<Owner>* getchildState(void)   const
			{
				return this->childState;
			}

			IState<Owner>* getPreviousState(void) const
			{
				return this->previousState;
			}

			//Only ever used during debugging to grab the name of the current state
			Ogre::String getNameOfCurrentState(void) const
			{
				Ogre::String s(typeid(*this->currentState).name());

				//remove the 'class ' part from the front of the string
				if (s.size() > 5)
				{
					s.erase(0, 6);
				}

				return s;
			}

			template <class Instance>
			void registerState(Ogre::String stateName)
			{
				auto it = this->states.find(stateName);
				if (it == this->states.cend())
				{
					IState<Owner>* actionState = new Instance;
					this->states.insert(std::make_pair(stateName, actionState));
				}
			}

			void unregisterState(Ogre::String stateName)
			{
				auto it = this->states.find(stateName);
				assert(it != this->states.end() && "[StateMachine::unrgisterState] Illegal state name!");
				this->states.erase(it);
			}
		private:
			//A pointer to the object that owns this instance
			Owner* owner;
			//The current state in the state machine
			IState<Owner>* currentState;
			//A record of the last state the agent was in
			IState<Owner>* previousState;
			//This state logic is called every time the FSM is updated
			IState<Owner>* childState;

			std::map<Ogre::String, IState<Owner>*> states;
		};

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		template <class Owner>
		class EXPORTED LuaStateMachine
		{
		public:

			LuaStateMachine(Owner* owner)
				: owner(owner)
			{

			}

			~LuaStateMachine()
			{
				
			}

			void setCurrentState(const luabind::object& currentState)
			{
				this->currentState = currentState;

				try
				{
					// Call the entry method of the new state
					auto& state = this->currentState["enter"];
					if (state)
						state(this->owner);
				}
				catch(luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaStateMachine] Caught error in 'setCurrentState' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			void setGlobalState(const luabind::object& globalState)
			{
				this->globalState = globalState;

				try
				{
					// Call the entry method of the new state
					auto& state = this->globalState["enter"];
					if (state)
						state(this->owner);
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaStateMachine] Caught error in 'setGlobalState' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			void setPreviousState(const luabind::object& previousState)
			{
				this->previousState = previousState;
			}

			const luabind::object& getCurrentState(void) const
			{
				return this->currentState;
			}

			const luabind::object& getPreviousState(void) const
			{
				return this->previousState;
			}

			const luabind::object& getGlobalState(void) const
			{
				return this->globalState;
			}

			// Call this to update the FSM
			void update(Ogre::Real dt)
			{
				try
				{
					// Make sure the state is valid before calling its execute 'method'
					if (this->currentState.is_valid())
					{
						auto& state = this->currentState["execute"];
						if (state)
							state(this->owner, dt);
					}
					
					if (this->globalState.is_valid())
					{
						auto& state = this->globalState["execute"];
						if (state)
							state(this->owner, dt);
					}
				}
				catch (luabind::error& error)
				{
					auto& found = errorMessages.find("execute");
					if (found == errorMessages.cend())
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaStateMachine] Caught error in 'execute' Error: " + Ogre::String(error.what())
							+ " details: " + msg.str());

						errorMessages.emplace("execute");
					}
				}
			}

			void changeState(const luabind::object& newState)
			{
				this->previousState = this->currentState;
				try
				{
					// Call the exit method of the existing state
					auto& state = this->currentState["exit"];
					if (state)
						state(this->owner);

					// Change state to the new state
					this->currentState = newState;

					if (this->currentState.is_valid())
					{
						// Call the entry method of the new state
						auto& state = this->currentState["enter"];
						if (state)
							state(this->owner);
					}
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaStateMachine] Caught error in 'changeState' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			void exitGlobalState(void)
			{
				try
				{
					if (this->globalState.is_valid())
					{
						// Call the exit method of the existing state
						auto& state = this->globalState["exit"];
						if (state)
							state(this->owner);
					}
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaStateMachine] Caught error in 'exitGlobalState' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			//Change state back to the previous state
			void revertToPreviousState(void)
			{
				this->changeState(this->previousState);
			}

			void resetStates(void)
			{
				this->currentState = this->originState;
				this->globalState = this->originState;
				this->previousState = this->originState;
				this->errorMessages.clear();
			}
		private:
			// A pointer to the object that owns this instance
			Owner* owner;
			// The current state is a lua table of lua functions. A table may be
			// represented in C++ using a luabind::object
			luabind::object currentState;
			// A record of the last state the owner was in
			luabind::object previousState;
			luabind::object globalState;

			luabind::object originState;

			std::set<Ogre::String> errorMessages;
		};

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		template <class Owner>
		class EXPORTED IState
		{
		public:
			virtual ~IState()
			{
			};

			virtual void enter(Owner*) = 0;

			virtual void update(Owner*, Ogre::Real) = 0;

			virtual void handleKeyInput(Owner*, int, bool)
			{

			}

			virtual void handleMouseInput(Owner*, int, bool, int, int, int = 0)
			{

			}

			virtual void exit(Owner*) = 0;
		};

	}; //end namespace KI

}; //end namespace NOWA

#endif