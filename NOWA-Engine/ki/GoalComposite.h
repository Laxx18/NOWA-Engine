#ifndef GOAL_COMPOSITE_H
#define GOAL_COMPOSITE_H

#include <list>
#include "Goal.h"

namespace NOWA
{
	namespace KI
	{
		template <class Owner>
		class EXPORTED GoalComposite : public Goal<Owner>
		{
		public:

			GoalComposite(Owner* owner, int type)
				: Goal<Owner>(owner, type)
			{
			}

			//! @brief When this object is destroyed make sure any subgoals are terminated
			//! and destroyed.
			virtual ~GoalComposite()
			{
				this->removeAllSubGoals();
			}

			//! @brief Logic to run when the goal is activated.
			virtual void activate(void) = 0;

			///! @brief Logic to run each update-step.
			virtual int  process(void) = 0;

			//! @brief Logic to run prior to the goal's destruction
			virtual void terminate(void) = 0;

			//! @brief If a child class of GoalComposite does not define a event handler
			//! the default behavior is to forward the event to the front-most
			//! subgoal
			/*virtual bool handleEvent(const Event& event)
			{
				return forwardEventToFrontMostSubgoal(event)
			}*/

			//! @brief Adds a subgoal to the front of the subgoal list
			//----------------------------- addSubGoal ------------------------------------
			template <class Owner>
			void addSubGoal(Goal<Owner>* owner)
			{
				//add the new goal to the front of the list
				this->subGoals.push_front(owner);
			}

			//! @brief This method iterates through the subgoals and calls each one's terminate
			//! method before deleting the subgoal and removing it from the subgoal list
			template <class Owner>
			void removeAllSubGoals(void)
			{
				for (SubgoalList::iterator it = this->subGoals.begin(); it != subGoals.end(); ++it)
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

			//! @brief Passes the event to the front-most subgoal
			//---------------- forwardEventToFrontMostSubgoal ---------------------------
			//
			//  passes the message to the goal at the front of the queue
			//-----------------------------------------------------------------------------
			//template <class Owner>
			//bool forwardEventToFrontMostSubgoal(const Event& event)
			//{
			//	if (false == this->subGoals.empty())
			//	{
			//		return subGoals.front()->handleEvent(event);
			//	}

			//	//return false if the message has not been handled
			//	return false;
			//}
		protected:
			// Composite goals may have any number of subgoals
			std::list<Goal<Owner>* > subGoals;
		};

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		template <class Owner>
		class EXPORTED LuaGoalComposite : public LuaGoal<Owner>
		{
		public:

			LuaGoalComposite(Owner* owner, const luabind::object& thisLuaGoal/*, int type*/)
				: LuaGoal<Owner>(owner, thisLuaGoal/*, type*/)
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
#if 0
				// See Arbitrate: evaluate, which goal is most suitable?
				try
				{
					// Call the entry method of the new state
					auto& goal = this->thisLuaGoal["activate"];
					if (goal)
						goal(this->owner, this->goalResult.get());
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoalComposite] Caught error in 'activate' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
#endif
			}

			///! @brief Logic to run each update-step.
			virtual boost::shared_ptr<GoalResult> process(Ogre::Real dt)
			{
#if 1
				// if status is inactive, call Activate()
				this->activateIfInactive<Owner>();

				//process the subgoals
				this->goalResult = this->processSubGoals<Owner>(dt);

				try
				{
					// Call the entry method of the new state
					auto& goal = this->thisLuaGoal["process"];
					if (goal)
						goal(this->owner, dt, this->goalResult.get());
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
#endif
				return this->goalResult;
			}

			//! @brief Logic to run prior to the goal's destruction
			virtual void terminate(void)
			{
#if 0
				try
				{
					// Call the entry method of the new state
					auto& goal = this->thisLuaGoal["terminate"];
					if (goal)
						goal(this->owner, this->goalResult.get());
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoalComposite] Caught error in 'terminate' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
#endif
			}

			//! @brief If a child class of GoalComposite does not define a event handler
			//! the default behavior is to forward the event to the front-most
			//! subgoal
			/*virtual bool handleEvent(const Event& event)
			{
				return forwardEventToFrontMostSubgoal(event)
			}*/

			virtual void setStartGoalComposite(const luabind::object& startCompositeGoal)
			{
				auto goal = new LuaGoal<Owner>(this->owner, startCompositeGoal);
				this->subGoals.push_front(goal);
				goal->activate();
			}

			//! @brief Adds a subgoal to the front of the subgoal list
			//----------------------------- addSubGoal ------------------------------------
			template <class Owner>
			void addSubGoal(luabind::object luaGoalTable)
			{
				//add the new goal to the front of the list
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

			//! @brief Passes the event to the front-most subgoal
			//---------------- forwardEventToFrontMostSubgoal ---------------------------
			//
			//  passes the message to the goal at the front of the queue
			//-----------------------------------------------------------------------------
			//template <class Owner>
			//bool forwardEventToFrontMostSubgoal(const Event& event)
			//{
			//	if (false == this->subGoals.empty())
			//	{
			//		return subGoals.front()->handleEvent(event);
			//	}

			//	//return false if the message has not been handled
			//	return false;
			//}
		protected:
			// Composite goals may have any number of subgoals
			std::list<LuaGoal<Owner>*> subGoals;
		};

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 1
		// TODO: Remove if not necessary
		// TODO: Is this necessary, or can i work directly with LuaGoalComposite????
		// First try directly with LuaGoalComposite!!!
		// In AiLuaGoalComponent: Create one new LuaGoalComposite, and work on that object and other lua goal composites and loa goals will be
		// created in lua scripts!
		template <class Owner>
		class LuaGoalMachine : public LuaGoalComposite<Owner>
		{
		public:
			LuaGoalMachine(Owner* owner, const luabind::object& thisLuaGoal)
				: LuaGoalComposite(owner, thisLuaGoal)
			{

			}

			~LuaGoalMachine()
			{
				auto it = this->goals.begin();
				for (it; it != this->goals.end(); ++it)
				{
					delete *it;
				}
			}

			// TODO: Is this necessary?
			void setStartGoal(const luabind::object& startGoal)
			{
				this->goals.push_back(startGoal);

				try
				{
					// Call the entry method of the new state
					auto& goal = startGoal["activate"];
					if (goal)
						goal(this->owner);
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoalMachine] Caught error in 'setStartGoal' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}
			}

			virtual void setStartGoalComposite(const luabind::object& startCompositeGoal) override
			{
				this->goals.push_back(startCompositeGoal);

				/*try
				{
					// Call the entry method of the new state
					auto& goal = startCompositeGoal["activate"];
					if (goal)
						goal(this->owner);
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaGoalMachine] Caught error in 'setStartCompositeGoal' Error: " + Ogre::String(error.what())
						+ " details: " + msg.str());
				}*/
			}

			void addSubGoal(const luabind::object& subGoal)
			{
				this->goals.push_back(subGoal);
			}

			void addSubCompositeGoal(const luabind::object& subCompositeGoal)
			{
				this->goals.push_back(subCompositeGoal);
			}

			// Call this to update the GSM
			void update(Ogre::Real dt)
			{
				this->activateIfInactive();

				LuaGoal<Owner>::ELuaGoalState subgoalStatus = this->processSubgoals(dt);

				if (subgoalStatus == COMPLETED || subgoalStatus == FAILED)
				{
					// if (!m_pOwner->isPossessed())
					{
						this->status = INACTIVE;
					}
				}

				return this->status;
			}

		private:
			// A pointer to the object that owns this instance
			Owner* owner;

			std::vector<luabind::object> goals;
		};

#endif


		// TODO: Usage later:
		/*
		  // but in lua
		  addSubGoal(new GetHealthGoal_Evaluator(HealthBias));
		  addSubCompositeGoal(new ExploreGoal_Evaluator(ExploreBias));
		  goals.push_back(new AttackTargetGoal_Evaluator(AttackBias));
		  goals.push_back(new GetWeaponGoal_Evaluator(ShotgunBias,
															 type_shotgun));
		  goals.push_back(new GetWeaponGoal_Evaluator(RailgunBias,
															 type_rail_gun));
		  goals.push_back(new GetWeaponGoal_Evaluator(RocketLauncherBias,
															 type_rocket_launcher));
		*/

	}; //end namespace KI

}; //end namespace NOWA

#endif

