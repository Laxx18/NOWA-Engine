#include "NOWAPrecompiled.h"
#include "LuaObserver.h"

#include "gameObject/GameObject.h"
#include "main/AppStateManager.h"
#include "modules/LuaScript.h"

namespace NOWA
{
    PathGoalObserver::PathGoalObserver() : IPathGoalObserver(), oneTime(false), alreadyReacted(false)
    {
    }

    PathGoalObserver::~PathGoalObserver()
    {
    }

    void PathGoalObserver::onPathGoalReached(void)
    {
        if (false == this->scriptCallbackFunction.is_valid())
        {
            return;
        }

        // On a looping path the goal is reached again on every lap. Without this guard a
        // one time reaction would fire once per lap, even though shouldReactOneTime()
        // promises otherwise - the owner may well keep the observer registered.
        if (true == this->oneTime && true == this->alreadyReacted)
        {
            return;
        }
        this->alreadyReacted = true;

        // The closure is COPIED into the command instead of capturing 'this'. The command
        // runs later on the logic thread, and the observer may already have been destroyed
        // by then (disconnect, joint released, scene change) - a raw 'this' capture then
        // dereferences freed memory.
        luabind::object callback = this->scriptCallbackFunction;

        NOWA::AppStateManager::LogicCommand logicCommand = [callback]()
        {
            try
            {
                luabind::call_function<void>(callback);
            }
            catch (luabind::error& error)
            {
                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                std::stringstream msg;
                msg << errorMsg;

                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PathGoalObserver] Caught error in 'onPathGoalReached' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    bool PathGoalObserver::shouldReactOneTime(void) const
    {
        return this->oneTime;
    }

    void PathGoalObserver::reactOnPathGoalReached(luabind::object scriptCallbackFunction)
    {
        this->reactOnPathGoalReached(scriptCallbackFunction, false);
    }

    void PathGoalObserver::reactOnPathGoalReached(luabind::object scriptCallbackFunction, bool oneTime)
    {
        // Replacing, not appending: calling this repeatedly (e.g. from a script function
        // that runs every frame) leaves exactly one reaction registered.
        this->scriptCallbackFunction = scriptCallbackFunction;
        this->oneTime = oneTime;
        // A newly registered closure is allowed to fire again, even if a previous one time
        // reaction had already been consumed.
        this->alreadyReacted = false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////

    AgentStuckObserver::AgentStuckObserver() : IAgentStuckObserver(), oneTime(false), alreadyReacted(false)
    {
    }

    AgentStuckObserver::~AgentStuckObserver()
    {
    }

    void AgentStuckObserver::onAgentStuck(void)
    {
        if (false == this->scriptCallbackFunction.is_valid())
        {
            return;
        }

        if (true == this->oneTime && true == this->alreadyReacted)
        {
            return;
        }
        this->alreadyReacted = true;

        // Bug: this used to call the closure TWICE - once directly here, and once more via
        // the logic command below. The direct call additionally ran Lua on whatever thread
        // happened to notify the observer, which is not the logic thread. Only the queued
        // call remains.
        //
        // The closure is copied rather than captured through 'this', because the command
        // runs later and the observer may be gone by then.
        luabind::object callback = this->scriptCallbackFunction;

        NOWA::AppStateManager::LogicCommand logicCommand = [callback]()
        {
            try
            {
                luabind::call_function<void>(callback);
            }
            catch (luabind::error& error)
            {
                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                std::stringstream msg;
                msg << errorMsg;

                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[AgentStuckObserver] Caught error in 'onAgentStuck' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    bool AgentStuckObserver::shouldReactOneTime(void) const
    {
        return this->oneTime;
    }

    void AgentStuckObserver::reactOnAgentStuck(luabind::object scriptCallbackFunction)
    {
        this->reactOnAgentStuck(scriptCallbackFunction, false);
    }

    void AgentStuckObserver::reactOnAgentStuck(luabind::object scriptCallbackFunction, bool oneTime)
    {
        this->scriptCallbackFunction = scriptCallbackFunction;
        this->oneTime = oneTime;
        this->alreadyReacted = false;
    }

}; // namespace end