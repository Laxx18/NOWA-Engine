#ifndef GOAL_H
#define GOAL_H

#include "NOWAPrecompiled.h"
#include "defines.h"

extern "C"
{
#include <lua.h>
}
#include <luabind/luabind.hpp>

namespace NOWA
{
	namespace KI
	{
		class EXPORTED GoalResult
		{
		public:
			enum LuaGoalState
			{
				ACTIVE,
				INACTIVE,
				COMPLETED,
				FAILED
			};
		public:
			GoalResult()
				: status(INACTIVE)
			{

			}

			void setStatus(LuaGoalState status)
			{
				this->status = status;
			}

			LuaGoalState getStatus(void) const
			{
				return this->status;
			}

		private:
			LuaGoalState status;
		};

		// Note: A LuaGoal cannot have children!
		template <class Owner>
		class EXPORTED LuaGoal
		{
		public:

			//! @note How goals start off in the inactive state
			LuaGoal(Owner* owner, const luabind::object& thisLuaGoal/*, int type*/)
				: owner(owner),
				thisLuaGoal(thisLuaGoal),
				/*type(type),*/
				goalResult(boost::make_shared<GoalResult>())
			{

			}

			virtual ~LuaGoal()
			{
				
			}

			//! @brief Logic to run when the goal is activated.
			virtual void activate(void)
			{
				try
				{
					// Call the entry method of the new state
					auto& goal = this->thisLuaGoal["activate"];
					if (goal)
					{
						goal(this->owner, this->goalResult.get());
					}
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoal] Caught error in 'activate' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			//! @brief Logic to run each update-step
			virtual boost::shared_ptr<GoalResult> process(Ogre::Real dt)
			{
				// // if status is inactive, call Activate()
				// this->activateIfInactive<Owner>();

				// Note: return int in lua not possible, hence use param in/out status to store its state for return
				try
				{
					// Call the entry method of the new state
					auto& goal = this->thisLuaGoal["process"];
					if (goal)
					{
						goal(this->owner, dt, this->goalResult.get());
					}
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoal] Caught error in 'process' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
					this->goalResult->setStatus(GoalResult::FAILED);
					return this->goalResult;
				}

				return this->goalResult;
			}

			//! @brief Logic to run when the goal is satisfied. (typically used to switch off any active steering behaviors)
			virtual void terminate(void)
			{
				try
				{
					// Call the entry method of the new state
					auto& goal = this->thisLuaGoal["terminate"];
					if (goal)
					{
						goal(this->owner, this->goalResult.get());
					}
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoal] Caught error in 'terminate' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			//! @brief Goals can handle messages. Many do not though, so this defines a default behavior
			/*virtual bool handleEvent(const Event& event)
			{
				return false;
			}*/

			//! @brief A Goal is atomic and cannot aggregate subgoals yet we must implement
			//! this method to provide the uniform interface required for the goal
			//! hierarchy.
			virtual void addSubGoal(LuaGoal<Owner>* owner)
			{
				throw std::runtime_error("Cannot add goals to atomic goals");
			}

			bool isComplete(void) const
			{
				return this->goalResult->getStatus() == GoalResult::COMPLETED;
			}

			bool isActive(void) const
			{
				return this->goalResult->getStatus() == GoalResult::ACTIVE;
			}

			bool isInactive(void) const
			{
				return this->goalResult->getStatus() == GoalResult::INACTIVE;
			}

			bool hasFailed(void) const
			{
				return this->goalResult->getStatus() == GoalResult::FAILED;
			}

			//int getType(void) const
			//{
			// 	return this->type;
			//}
		public:
			/* the following methods were created to factor out some of the commonality
			in the implementations of the process method() */

			//! @brief If status = inactive this method sets it to active and calls activate()
			template <class Owner>
			void activateIfInactive()
			{
				if (this->isInactive())
				{
					this->activate();
				}
			}

			//! @brief If status is failed this method sets it to inactive so that the goal
			//! will be reactivated (and therefore re-planned) on the next update-step.
			template <class Owner>
			void reactivateIfFailed()
			{
				if (this->hasFailed())
				{
					this->goalResult->setStatus(GoalResult::INACTIVE);
				}
			}

		protected:

			// An enumerated type specifying the type of goal
			// TODO: int type; see enum goal_wander etc. is this necessary??

			// A pointer to the game object that owns this goal
			Owner* owner;

			luabind::object thisLuaGoal;

			// An enumerated value indicating the goal's status (active, inactive, completed, failed)
			boost::shared_ptr<GoalResult> goalResult;

			// int type;
		};

	}; //end namespace KI

}; //end namespace NOWA

#endif