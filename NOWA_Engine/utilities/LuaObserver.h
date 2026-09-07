#ifndef LUA_OBSERVER_H
#define LUA_OBSERVER_H

#include "OgreString.h"
#include "defines.h"

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

        /**
         * @brief		Sets the reaction closure. Calling this again REPLACES the previous
         *				one, so it is safe to call from a per frame script function.
         * @param[in]	scriptCallbackFunction	The closure to call when the goal is reached
         */
        void reactOnPathGoalReached(luabind::object scriptCallbackFunction);

        /**
         * @brief		Same as above, but additionally controls whether the closure fires
         *				only once. Useful on looping paths, where the goal is reached again
         *				on every lap and the reaction is usually meant to happen just once.
         * @param[in]	scriptCallbackFunction	The closure to call when the goal is reached
         * @param[in]	oneTime					If true, the closure fires only on the first goal
         */
        void reactOnPathGoalReached(luabind::object scriptCallbackFunction, bool oneTime);

    private:
        luabind::object scriptCallbackFunction;
        // shouldReactOneTime() used to be hardcoded to false, so the one time facility the
        // interface offers could never actually be used by anyone.
        bool oneTime;
        // Self guard: even if the owner ignores shouldReactOneTime() and keeps the observer
        // registered, a one time reaction must not fire a second time.
        bool alreadyReacted;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class EXPORTED AgentStuckObserver : public IAgentStuckObserver
    {
    public:
        AgentStuckObserver();

        virtual ~AgentStuckObserver();

        virtual void onAgentStuck(void) override;

        virtual bool shouldReactOneTime(void) const override;

        /**
         * @brief		Sets the reaction closure. Calling this again REPLACES the previous one.
         * @param[in]	scriptCallbackFunction	The closure to call when the agent got stuck
         */
        void reactOnAgentStuck(luabind::object scriptCallbackFunction);

        /**
         * @brief		Same as above, but additionally controls whether the closure fires only once.
         * @param[in]	scriptCallbackFunction	The closure to call when the agent got stuck
         * @param[in]	oneTime					If true, the closure fires only the first time
         */
        void reactOnAgentStuck(luabind::object scriptCallbackFunction, bool oneTime);

    private:
        luabind::object scriptCallbackFunction;
        bool oneTime;
        bool alreadyReacted;
    };

}; // namespace end

#endif