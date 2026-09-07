#include "NOWAPrecompiled.h"
#include "ScriptEventManager.h"
#include "AppStateManager.h"
#include "modules/LuaScriptApi.h"

namespace NOWA
{
    ScriptEventListener::ScriptEventListener(const Ogre::String& appStateName, const EventType& eventType, luabind::object scriptCallbackFunction) : appStateName(appStateName), eventType(eventType), scriptCallbackFunction(scriptCallbackFunction)
    {
    }

    ScriptEventListener::~ScriptEventListener(void)
    {
        // Bug: this used to unregister against getEventManager() - the DEFAULT manager -
        // while ScriptEventManager::registerEventListener() adds the listener to
        // getEventManager(appStateName). With more than one app state those are different
        // objects, so the listener was never actually removed and remained registered as a
        // delegate pointing at freed memory.
        AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->removeListener(this->getDelegate(), this->eventType);
    }

    void ScriptEventListener::scriptEventDelegate(EventDataPtr eventDataPtr)
    {
        // Call the Lua function
        boost::shared_ptr<ScriptEvent> scriptEvent = boost::static_pointer_cast<ScriptEvent>(eventDataPtr);
        // LuaPlus::LuaFunction<void> Callback = m_scriptCallbackFunction;
        // Callback(pScriptEvent->GetEventData());
        // Attention: If this does not work, checkout this implementation: https://sourceforge.net/p/luabind/mailman/message/3336165/

        if (false == this->scriptCallbackFunction.is_valid())
        {
            return;
        }

        // Everything the command needs is COPIED here instead of being read through 'this'
        // at execution time. This listener is deleted directly by destroyListener() and by
        // destroyContent(), while commands it already enqueued are still sitting in the
        // logic queue - a lua script calling removeEventListener() from inside its own
        // event handler hits that window essentially every time. With nothing captured by
        // pointer, a deleted listener simply leaves a harmless self contained command
        // behind.
        luabind::object callback = this->scriptCallbackFunction;
        const EventType capturedEventType = this->eventType;

        NOWA::AppStateManager::LogicCommand logicCommand = [callback, capturedEventType, scriptEvent]()
        {
            try
            {
                luabind::call_function<void>(callback, scriptEvent->getEventData());
            }
            catch (luabind::error& error)
            {
                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                std::stringstream msg;
                msg << errorMsg;

                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                    "[ScriptEventListener] Caught error in 'call event function' for function type: '" + Ogre::StringConverter::toString(capturedEventType) + "' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    EventListenerDelegate ScriptEventListener::getDelegate(void)
    {
        return fastdelegate::MakeDelegate(this, &ScriptEventListener::scriptEventDelegate);
    }

    ////////////////////////////////////////////////////////////////////////////////////

    ScriptEventManager::ScriptEventManager(const Ogre::String& appStateName) : appStateName(appStateName)
    {
    }

    ScriptEventManager::~ScriptEventManager(void)
    {
    }

    void ScriptEventManager::destroyContent(void)
    {
        for (auto it = this->listeners.begin(); it != this->listeners.end(); ++it)
        {
            ScriptEventListener* pListener = (*it);
            delete pListener;
        }
        this->listeners.clear();
    }

    void ScriptEventManager::addListener(ScriptEventListener* listener)
    {
        this->listeners.insert(listener);
    }

    void ScriptEventManager::destroyListener(ScriptEventListener* listener)
    {
        auto findIt = this->listeners.find(listener);
        if (findIt != this->listeners.end())
        {
            this->listeners.erase(findIt);
            delete listener;
        }
        else
        {
            // GCC_ERROR("Couldn't find script listener in set; this will probably cause a memory leak");
        }
    }

    unsigned long ScriptEventManager::registerEventListener(EventType eventType, luabind::object callbackFunction)
    {
        if (luabind::type(callbackFunction) == LUA_TFUNCTION)
        {
            // Create the C++ listener proxy and set it to listen for the event
            ScriptEventListener* listener = new ScriptEventListener(this->appStateName, eventType, callbackFunction);
            this->addListener(listener);
            AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->addListener(listener->getDelegate(), eventType);

            // Convert the pointer to an unsigned long to use as the handle
            unsigned long handle = reinterpret_cast<unsigned long>(listener);
            return handle;
        }

        // ERROR("Attempting to register script event listener with invalid callback function");
        return 0;
    }

    void ScriptEventManager::removeEventListener(unsigned long listenerId)
    {
        if (0 == listenerId)
        {
            return;
        }

        // Convert the listenerId back into a pointer
        ScriptEventListener* listener = reinterpret_cast<ScriptEventListener*>(listenerId);

        // destroyListener() already checks membership in the set, which is what makes a
        // double removeEventListener() from lua safe: the second call finds nothing and
        // does NOT delete again. Without going through the set, reinterpret_cast on an
        // arbitrary number followed by delete would be unbounded.
        this->destroyListener(listener);
    }

    bool ScriptEventManager::queueEvent(EventType eventType, luabind::object eventData)
    {
        boost::shared_ptr<ScriptEvent> eventPtr(this->buildEvent(eventType, eventData));
        if (eventPtr)
        {
            AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->queueEvent(eventPtr);
            return true;
        }
        return false;
    }

    bool ScriptEventManager::triggerEvent(EventType eventType, luabind::object eventData)
    {
        boost::shared_ptr<ScriptEvent> eventPtr(this->buildEvent(eventType, eventData));
        if (eventPtr)
        {
            AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(eventPtr);
            return true;
        }
        return false;
    }

    boost::shared_ptr<ScriptEvent> ScriptEventManager::buildEvent(EventType eventType, luabind::object& eventData)
    {
        // Create the event from the event type
        boost::shared_ptr<ScriptEvent> eventPtr(ScriptEvent::createEventFromScript(eventType));
        if (nullptr == eventPtr)
        {
            return boost::shared_ptr<ScriptEvent>();
        }

        // Set the event data that was passed in
        if (false == eventPtr->setEventData(eventData))
        {
            return boost::shared_ptr<ScriptEvent>();
        }

        return eventPtr;
    }

    void ScriptEventManager::registerEvent(const Ogre::String& eventName)
    {
        EventType eventType = NOWA::getIdFromName(eventName);
        ScriptEvent::registerEventTypeWithScript(eventName.data(), eventType);
        ScriptEvent::addCreationFunction(eventType, &ScriptEvent::instantiateScript);
    }

}; // namespace end