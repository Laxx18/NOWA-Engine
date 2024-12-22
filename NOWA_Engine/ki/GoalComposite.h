#ifndef GOAL_COMPOSITE_H
#define GOAL_COMPOSITE_H

#include <list>
#include "Goal.h"

namespace NOWA
{
	namespace KI
	{
		template <class Owner>
		class EXPORTED LuaGoalComposite : public LuaGoal<Owner>
		{
		public:

			LuaGoalComposite(Owner* owner, const luabind::object& thisLuaGoal)
				: LuaGoal<Owner>(owner, thisLuaGoal)
			{
			}

			//! @brief When this object is destroyed make sure any subgoals are terminated
			//! and destroyed.
			virtual ~LuaGoalComposite()
			{
				this->removeAllSubGoals<Owner>();
			}

			//! @brief Logic to run when the goal is activated.
			virtual void activate(void)
			{
				// See Arbitrate: evaluate, which goal is most suitable?
				try
				{
					if (this->thisLuaGoal.is_valid())
					{
						// Call the entry method of the new state
						auto& goal = this->thisLuaGoal["activate"];
						if (goal)
						{
							goal(this->owner, this->goalResult.get());
						}
					}
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoalComposite] Caught error in 'activate' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			//! @brief Processes any subgoals that may be present
			//-------------------------- processSubGoals ----------------------------------
			//
			//  this method first removes any completed goals from the front of the
			//  subgoal list. It then processes the next goal in the list (if there is one)
			//-----------------------------------------------------------------------------
			template <class Owner>
			int processSubGoals(void)
			{
				//remove all completed and failed goals from the front of the subgoal list
				while (!this->subGoals.empty() && (this->subGoals.front()->isComplete() || subGoals.front()->hasFailed()))
				{
					this->subGoals.front()->terminate();
					delete this->subGoals.front();
					this->subGoals.pop_front();
				}

				//if any subgoals remain, process the one at the front of the list
				if (false == this->subGoals.empty())
				{
					//grab the status of the front-most subgoal
					int statusOfSubGoals = this->subGoals.front()->process();

					//we have to test for the special case where the front-most subgoal
					//reports 'completed' *and* the subgoal list contains additional goals.When
					//this is the case, to ensure the parent keeps processing its subgoal list
					//we must return the 'active' status.
					if (statusOfSubGoals == COMPLETED && this->subGoals.size() > 1)
					{
						return ACTIVE;
					}

					return statusOfSubGoals;
				}

				//no more subgoals to process - return 'completed'
				else
				{
					return COMPLETED;
				}
			}

			///! @brief Logic to run each update-step.
			virtual boost::shared_ptr<GoalResult> process(Ogre::Real dt)
			{
				// if status is inactive, call Activate()
				// this->activateIfInactive<Owner>();

				//process the subgoals
				this->goalResult = this->processSubGoals<Owner>(dt);

				try
				{
					if (this->thisLuaGoal.is_valid())
					{
						// Call the entry method of the new state
						auto& goal = this->thisLuaGoal["process"];

						if (goal)
						{
							goal(this->owner, dt, this->goalResult.get());
						}
					}
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoalComposite] Caught error in 'activate' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}

				this->reactivateIfFailed<Owner>();
				return this->goalResult;
			}

			//! @brief Logic to run prior to the goal's destruction
			virtual void terminate(void)
			{
				try
				{
					if (this->thisLuaGoal.is_valid())
					{
						// Call the entry method of the new state
						auto& goal = this->thisLuaGoal["terminate"];
						if (goal)
						{
							goal(this->owner, this->goalResult.get());
						}
					}
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoalComposite] Caught error in 'terminate' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			//! @brief Adds a subgoal to the front of the subgoal list
			//----------------------------- addSubGoal ------------------------------------
			template <class Owner>
			void addSubGoal(luabind::object luaGoalTable)
			{
				// Adds the new goal to the front of the list
				if (!luabind::object_cast<bool>(luaGoalTable["activate"]))
				{
					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoalComposite] Caught error in 'addSubGoal' Error: LuaGoal must define an 'activate' function.");
					return;
				}

				this->subGoals.push_front(new LuaGoal<Owner>(this->owner, luaGoalTable));
			}

			//! @brief This method iterates through the subgoals and calls each one's terminate
			//! method before deleting the subgoal and removing it from the subgoal list
			template <class Owner>
			void removeAllSubGoals(void)
			{
				for (auto it = this->subGoals.begin(); it != this->subGoals.end(); ++it)
				{
					(*it)->terminate();
					delete* it;
				}
				this->subGoals.clear();
			}
		private:
			//! @brief Processes any subgoals that may be present
			//-------------------------- processSubGoals ----------------------------------
			//
			//  this method first removes any completed goals from the front of the
			//  subgoal list. It then processes the next goal in the list (if there is one)
			//-----------------------------------------------------------------------------
			template <class Owner>
			boost::shared_ptr<GoalResult> processSubGoals(Ogre::Real dt)
			{
				//remove all completed and failed goals from the front of the subgoal list
				while (!this->subGoals.empty() && (this->subGoals.front()->isComplete() || subGoals.front()->hasFailed()))
				{
					this->subGoals.front()->terminate();
					delete this->subGoals.front();
					this->subGoals.pop_front();
				}

				//if any subgoals remain, process the one at the front of the list
				if (false == this->subGoals.empty())
				{
					//grab the status of the front-most subgoal
					this->goalResult = this->subGoals.front()->process(dt);
					// boost::shared_ptr<GoalResult> subGoalResult(boost::make_shared<GoalResult>(new GoalResult());


					//we have to test for the special case where the front-most subgoal
					//reports 'completed' *and* the subgoal list contains additional goals.When
					//this is the case, to ensure the parent keeps processing its subgoal list
					//we must return the 'active' status.
					if (this->goalResult->getStatus() == GoalResult::COMPLETED && this->subGoals.size() > 1)
					{
						this->goalResult->setStatus(GoalResult::ACTIVE);
						return this->goalResult;
					}

					return this->goalResult;
				}

				//no more subgoals to process - return 'completed'
				else
				{
					this->goalResult->setStatus(GoalResult::COMPLETED);
					return this->goalResult;
				}
			}

			template <class Owner>
			int getType(void) const
			{
				return this->type;
			}

		protected:
			// Composite goals may have any number of subgoals
			std::list<LuaGoal<Owner>*> subGoals;
		};

	}; //end namespace KI

}; //end namespace NOWA

#endif

