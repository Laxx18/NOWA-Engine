#ifndef LUA_OBSERVER_H
#define LUA_OBSERVER_H

#include "defines.h"
#include "OgreString.h"

namespace NOWA
{
	class GameObject;
	class LuaScript;

	/**
	  * @class IPathGoalObserver
	  * @brief This interface can be implemented to react when an agent reached a path goal.
	  */
	class EXPORTED IPathGoalObserver
	{
	public:
		/**
		* @brief		Called path goal has been reached
		*/
		virtual void onPathGoalReached(void) = 0;

		/**
		 * @brief		Gets whether the reaction should be done just once.
		 * @return		if true, this observer will be called only once.
		 */
		virtual bool shouldReactOneTime(void) const = 0;
	};

	/**
	* @class IAgentStuckObserver
	* @brief This interface can be implemented to react when an agent got stuck.
	*/
	class EXPORTED IAgentStuckObserver
	{
	public:

		/**
		* @brief		Called when agent got stuck
		*/
		virtual void onAgentStuck(void) = 0;

		/**
		 * @brief		Gets whether the reaction should be done just once.
		 * @return		if true, this observer will be called only once.
		 */
		virtual bool shouldReactOneTime(void) const = 0;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED PathGoalObserver : public IPathGoalObserver
	{
	public:
		PathGoalObserver();

		virtual ~PathGoalObserver();

		virtual void onPathGoalReached(void) override;

		virtual bool shouldReactOneTime(void) const override;

		void reactOnPathGoalReached(luabind::object scriptCallbackFunction);

	private:
		luabind::object scriptCallbackFunction;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED AgentStuckObserver : public IAgentStuckObserver
	{
	public:
		AgentStuckObserver();

		virtual ~AgentStuckObserver();

		virtual void onAgentStuck(void) override;

		virtual bool shouldReactOneTime(void) const override;

		void reactOnAgentStuck(luabind::object scriptCallbackFunction);

	private:
		luabind::object scriptCallbackFunction;
	};

}; //namespace end

#endif
