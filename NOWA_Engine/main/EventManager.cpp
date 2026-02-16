// EventManager.cpp
#include "NOWAPrecompiled.h"
#include "EventManager.h"
#include "AppStateManager.h"
#include "Core.h"
#include "ProcessManager.h"

namespace NOWA
{
    namespace
    {
        class TriggerDelayedProcess : public Process
        {
        public:
            explicit TriggerDelayedProcess(const EventDataPtr& event, const std::map<EventType, std::list<EventListenerDelegate>>& eventListeners, Ogre::Real timeToDelaySec) :
                event(event),
                eventListeners(eventListeners),
                timeToDelaySec(timeToDelaySec),
                timeDelayedSoFar(0)
            {
            }

        protected:
            virtual void onUpdate(float dt) override
            {
                this->timeDelayedSoFar += dt;
                if (this->timeDelayedSoFar >= timeToDelaySec)
                {
                    auto findIt = this->eventListeners.find(event->getEventType());
                    if (findIt != this->eventListeners.end())
                    {
                        const std::list<EventListenerDelegate>& eventListenerList = findIt->second;
                        for (std::list<EventListenerDelegate>::const_iterator it = eventListenerList.cbegin(); it != eventListenerList.cend(); ++it)
                        {
                            EventListenerDelegate listener = (*it);
                            try
                            {
                                listener(event); // call the delegate
                            }
                            catch (const std::exception& e)
                            {
                                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[EventManager] Exception in delayed trigger: " + Ogre::String(e.what()));
                            }
                        }
                    }
                    this->succeed();
                }
            }

        private:
            Ogre::Real timeToDelaySec;
            Ogre::Real timeDelayedSoFar;
            EventDataPtr event;
            std::map<EventType, std::list<EventListenerDelegate>> eventListeners;
        };
    }

    GenericObjectFactory<EventData, EventType> eventFactory;

    EventManager::EventManager(const Ogre::String& appStateName) : appStateName(appStateName)
    {
    }

    EventManager::~EventManager()
    {
    }

    bool EventManager::addListener(const EventListenerDelegate& eventDelegate, const EventType& type)
    {
        std::lock_guard<std::mutex> lock(this->listenerMutex);

        EventListenerList& eventListenerList = this->eventListeners[type]; // this will find or create the entry
        for (auto it = eventListenerList.cbegin(); it != eventListenerList.cend(); ++it)
        {
            if (eventDelegate == (*it))
            {
                // WARNING("Attempting to double-register a delegate");
                return false;
            }
        }

        eventListenerList.push_back(eventDelegate);
        return true;
    }

    bool EventManager::removeListener(const EventListenerDelegate& eventDelegate, const EventType& type)
    {
        std::lock_guard<std::mutex> lock(this->listenerMutex);

        bool success = false;
        auto findIt = this->eventListeners.find(type);
        if (findIt != this->eventListeners.end())
        {
            EventListenerList& listeners = findIt->second;
            for (auto it = listeners.begin(); it != listeners.end(); ++it)
            {
                if (eventDelegate == (*it))
                {
                    listeners.erase(it);
                    success = true;
                    break; // No need continue because it should be impossible for the same delegate function to be registered for the same event more than once
                }
            }
        }

        return success;
    }

    bool EventManager::triggerEvent(const EventDataPtr& event, Ogre::Real delaySec)
    {
        // CRITICAL: triggerEvent must only be called from the main/logic thread
        // because listeners execute on the calling thread
        assert(AppStateManager::getSingletonPtr()->isLogicThread() && "triggerEvent() must be called from the main/logic thread! Use queueEvent() from other threads.");
        

        if (!event)
        {
            return false;
        }

        bool processed = false;

        if (0.0f == delaySec)
        {
            // Copy listeners under lock, then invoke without holding lock
            EventListenerList listenersCopy;
            {
                std::lock_guard<std::mutex> lock(this->listenerMutex);
                auto findIt = this->eventListeners.find(event->getEventType());
                if (findIt != this->eventListeners.end())
                {
                    listenersCopy = findIt->second;
                }
            }

            // Invoke listeners without holding lock to avoid potential deadlocks
            for (EventListenerList::const_iterator it = listenersCopy.cbegin(); it != listenersCopy.cend(); ++it)
            {
                EventListenerDelegate listener = (*it);
                try
                {
                    // Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[EventManager] Sending Event: " + Ogre::String(event->getName()));
                    listener(event); // call the delegate
                    processed = true;
                }
                catch (const std::exception& e)
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[EventManager] Exception in triggerEvent for event '" + Ogre::String(event->getName()) + "': " + Ogre::String(e.what()));
                }
            }
        }
        else
        {
            // Copy listeners for delayed process
            EventListenerMap listenersCopy;
            {
                std::lock_guard<std::mutex> lock(this->listenerMutex);
                listenersCopy = this->eventListeners;
            }

            NOWA::ProcessPtr triggerDelayedProcess(new TriggerDelayedProcess(event, listenersCopy, delaySec));
            NOWA::ProcessManager::getInstance()->attachProcess(triggerDelayedProcess);
            processed = true;
        }

        return processed;
    }

    bool EventManager::queueEvent(const EventDataPtr& event)
    {
        // Make sure the event is valid
        if (nullptr == event)
        {
            // ERROR("Invalid event in queueEvent()");
            return false;
        }

        // Thread-safe enqueue - can be called from any thread (render or main)
        this->eventQueue.enqueue(event);
        // Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[EventManager] Successfully queued event: " + Ogre::String(event->getName()));
        return true;
    }

    bool EventManager::abortEvent(const EventType& inType, bool allOfType)
    {
        // Note: With lock-free queue, we must dequeue all events, filter, and re-enqueue
        // This is less efficient than the original double-buffer approach, but maintains API compatibility

        bool success = false;
        std::vector<EventDataPtr> tempEvents;

        // Dequeue all events
        EventDataPtr event;
        while (this->eventQueue.try_dequeue(event))
        {
            if (event->getEventType() == inType)
            {
                success = true;
                if (!allOfType)
                {
                    // Found one, re-enqueue the rest
                    break;
                }
                // If allOfType, don't re-enqueue this event (effectively removing it)
            }
            else
            {
                // Keep this event
                tempEvents.push_back(event);
            }
        }

        // Continue dequeueing remaining events if we broke early
        if (success && !allOfType)
        {
            while (this->eventQueue.try_dequeue(event))
            {
                tempEvents.push_back(event);
            }
        }

        // Re-enqueue all kept events
        for (const auto& evt : tempEvents)
        {
            this->eventQueue.enqueue(evt);
        }

        return success;
    }

    bool EventManager::hasEvent(const EventType& type)
    {
        // Note: With lock-free queue, we must dequeue all events and re-enqueue them
        // This is less efficient than the original approach, but maintains API compatibility

        bool found = false;
        std::vector<EventDataPtr> tempEvents;

        // Dequeue all events, checking for the type
        EventDataPtr event;
        while (this->eventQueue.try_dequeue(event))
        {
            tempEvents.push_back(event);
            if (event->getEventType() == type)
            {
                found = true;
            }
        }

        // Re-enqueue all events
        for (const auto& evt : tempEvents)
        {
            this->eventQueue.enqueue(evt);
        }

        return found;
    }

    bool EventManager::update(unsigned long maxMillis)
    {
        unsigned long currMs = Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
        unsigned long maxMs = ((maxMillis == EventManager::kINFINITE) ? (EventManager::kINFINITE) : (currMs + maxMillis));

        EventDataPtr event;
        bool processedAll = true;

        // Process events from the lock-free queue
        while (this->eventQueue.try_dequeue(event))
        {
            if (!event)
            {
                continue;
            }

            const EventType& eventType = event->getEventType();

            // Copy listeners under lock
            EventListenerList listenersCopy;
            {
                std::lock_guard<std::mutex> lock(this->listenerMutex);
                auto findIt = this->eventListeners.find(eventType);
                if (findIt != this->eventListeners.end())
                {
                    listenersCopy = findIt->second;
                }
            }

            // Call delegates without holding the lock
            if (false == listenersCopy.empty())
            {
                for (auto it = listenersCopy.begin(); it != listenersCopy.end(); ++it)
                {
                    EventListenerDelegate listener = (*it);
                    try
                    {
                        listener(event);
                    }
                    catch (const std::exception& e)
                    {
                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[EventManager] Exception in update for event '" + Ogre::String(event->getName()) + "': " + Ogre::String(e.what()));
                    }
                }
            }

            // Check to see if time ran out
            currMs = Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
            if (maxMillis != EventManager::kINFINITE && currMs >= maxMs)
            {
                // Time limit reached - there may be more events in the queue
                processedAll = false;
                // Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[EventManager] Aborting event processing; time ran out");
                break;
            }
        }

        return processedAll;
    }

    void EventManager::clearEvents(void)
    {
        // Drain the lock-free queue
        EventDataPtr dummy;
        while (this->eventQueue.try_dequeue(dummy))
        {
            // Just dequeue and discard
        }
    }

}; // namespace end