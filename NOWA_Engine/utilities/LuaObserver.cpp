#include "NOWAPrecompiled.h"
#include "LuaObserver.h"

#include "gameObject/GameObject.h"
#include "modules/LuaScript.h"

namespace NOWA
{
	PathGoalObserver::PathGoalObserver()
		: IPathGoalObserver()
	{
		
	}

	PathGoalObserver::~PathGoalObserver()
	{

	}

	void PathGoalObserver::onPathGoalReached(void)
	{
		if (this->scriptCallbackFunction.is_valid())
		{
			try
			{
				luabind::call_function<void>(this->scriptCallbackFunction);
			}
			catch (luabind::error& error)
			{
				luabind::object errorMsg(luabind::from_stack(error.state(), -1));
				std::stringstream msg;
				msg << errorMsg;

				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'onPathGoalReached' Error: " + Ogre::String(error.what())
					+ " details: " + msg.str());
			}
		}
	}

	bool PathGoalObserver::shouldReactOneTime(void) const
	{
		return false;
	}

	void PathGoalObserver::reactOnPathGoalReached(luabind::object scriptCallbackFunction)
	{
		this->scriptCallbackFunction = scriptCallbackFunction;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	AgentStuckObserver::AgentStuckObserver()
		: IAgentStuckObserver()
	{
		
	}

	AgentStuckObserver::~AgentStuckObserver()
	{

	}

	void AgentStuckObserver::onAgentStuck(void)
	{
		if (this->scriptCallbackFunction.is_valid())
		{
			try
			{
				luabind::call_function<void>(this->scriptCallbackFunction);
			}
			catch (luabind::error& error)
			{
				luabind::object errorMsg(luabind::from_stack(error.state(), -1));
				std::stringstream msg;
				msg << errorMsg;

				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'onAgentStuck' Error: " + Ogre::String(error.what())
					+ " details: " + msg.str());
			}
		}
	}

	bool AgentStuckObserver::shouldReactOneTime(void) const
	{
		return false;
	}

	void AgentStuckObserver::reactOnAgentStuck(luabind::object scriptCallbackFunction)
	{
		this->scriptCallbackFunction = scriptCallbackFunction;
	}

}; // namespace end
