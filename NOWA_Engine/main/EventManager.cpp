#include "NOWAPrecompiled.h"
#include "EventManager.h"
#include "ProcessManager.h"
#include "Core.h"
#include "AppStateManager.h"

namespace NOWA
{
	namespace
	{
		class TriggerDelayedProcess : public Process
		{
		public:
			explicit TriggerDelayedProcess(const EventDataPtr& event, const std::map<EventType, std::list<EventListenerDelegate>>& eventListeners, Ogre::Real timeToDelaySec)
				: event(event),
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
							// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[EventManager] Sending Event: " + Ogre::String(event->getName()));
							listener(event);  // call the delegate
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

	EventManager::EventManager(const Ogre::String& appStateName)
		: appStateName(appStateName),
		activeQueue(0)
	{
	
	}

	EventManager::~EventManager()
	{

	}

	bool EventManager::addListener(const EventListenerDelegate& eventDelegate, const EventType& type)
	{
		EventListenerList& eventListenerList = this->eventListeners[type];  // this will find or create the entry
		for (auto& it = eventListenerList.cbegin(); it != eventListenerList.cend(); ++it)
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
		bool success = false;

		auto findIt = this->eventListeners.find(type);
		if (findIt != this->eventListeners.end())
		{
			EventListenerList& listeners = findIt->second;
			for (auto& it = listeners.cbegin(); it != listeners.cend(); ++it)
			{
				if (eventDelegate == (*it))
				{
					listeners.erase(it);
					success = true;
					break;  // No need continue because it should be impossible for the same delegate function to be registered for the same event more than once
				}
			}
		}

		return success;
	}

	bool EventManager::triggerEvent(const EventDataPtr& event, Ogre::Real delaySec) const
	{
		bool processed = false;

		if (0.0f == delaySec)
		{
			auto findIt = this->eventListeners.find(event->getEventType());
			if (findIt != this->eventListeners.end())
			{
				const EventListenerList& eventListenerList = findIt->second;
				for (EventListenerList::const_iterator it = eventListenerList.cbegin(); it != eventListenerList.cend(); ++it)
				{
					EventListenerDelegate listener = (*it);
					// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[EventManager] Sending Event: " + Ogre::String(event->getName()));
					listener(event);  // call the delegate
					processed = true;
				}
			}
		}
		else
		{
			NOWA::ProcessPtr triggerDelayedProcess(new TriggerDelayedProcess(event, eventListeners, delaySec));
			NOWA::ProcessManager::getInstance()->attachProcess(triggerDelayedProcess);
			processed = true;
		}
		return processed;
	}

	bool EventManager::queueEvent(const EventDataPtr& event)
	{
		assert(this->activeQueue >= 0);
		assert(this->activeQueue < EVENTMANAGER_NUM_QUEUES);

		// Make sure the event is valid
		if (nullptr == event)
		{
			// ERROR("Invalid event in QueueEvent()");
			return false;
		}

		auto findIt = this->eventListeners.find(event->getEventType());
		if (findIt != this->eventListeners.end())
		{
			this->queues[this->activeQueue].push_back(event);
			// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[EventManager] Successfully queued event: " + Ogre::String(event->getName()));
			return true;
		}
		else
		{
			return false;
		}
	}

	bool EventManager::threadSafeQueueEvent(const EventDataPtr& event)
	{
		this->realtimeEventQueue.enqueue(event);
		return true;
	}

	bool EventManager::abortEvent(const EventType& inType, bool allOfType)
	{
		assert(this->activeQueue >= 0);
		assert(this->activeQueue < EVENTMANAGER_NUM_QUEUES);

		bool success = false;
		EventListenerMap::iterator findIt = this->eventListeners.find(inType);

		if (findIt != this->eventListeners.end())
		{
			EventQueue& eventQueue = this->queues[this->activeQueue];
			auto it = eventQueue.begin();
			while (it != eventQueue.end())
			{
				// Removing an item from the queue will invalidate the iterator, so have it point to the next member.  All
				// work inside this loop will be done using thisIt.
				auto thisIt = it;
				++it;

				if ((*thisIt)->getEventType() == inType)
				{
					eventQueue.erase(thisIt);
					success = true;
					if (!allOfType)
					{
						break;
					}
				}
			}
		}

		return success;
	}

	bool EventManager::hasEvent(const EventType& type)
	{
		EventListenerMap::iterator findIt = this->eventListeners.find(type);
		if (findIt != this->eventListeners.end())
		{
			EventQueue& eventQueue = this->queues[this->activeQueue];
			auto it = eventQueue.begin();
			while (it != eventQueue.end())
			{
				if ((*it)->getEventType() == type)
				{
					return true;
				}
				++it;
			}
		}
		return false;
	}

	bool EventManager::update(unsigned long maxMillis)
	{
		unsigned long currMs = Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
		unsigned long maxMs = ((maxMillis == EventManager::kINFINITE) ? (EventManager::kINFINITE) : (currMs + maxMillis));

		// This section added to handle events from other threads.  Check out Chapter 20.
		EventDataPtr realtimeEvent;
		while (this->realtimeEventQueue.try_dequeue(realtimeEvent))
		{
			this->queueEvent(realtimeEvent);

			currMs = Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
			if (maxMillis != EventManager::kINFINITE)
			{
				if (currMs >= maxMs)
				{
					// ERROR("A realtime process is spamming the event manager!");
				}
			}
		}

		// swap active queues and clear the new queue after the swap
		int queueToProcess = this->activeQueue;
		this->activeQueue = (this->activeQueue + 1) % EVENTMANAGER_NUM_QUEUES;
		this->queues[this->activeQueue].clear();

		// Process the queue
		while (!this->queues[queueToProcess].empty())
		{
			// pop the front of the queue
			EventDataPtr pEvent = this->queues[queueToProcess].front();
			this->queues[queueToProcess].pop_front();

			const EventType& eventType = pEvent->getEventType();

			// find all the delegate functions registered for this event
			auto findIt = this->eventListeners.find(eventType);
			if (findIt != this->eventListeners.end())
			{
				const EventListenerList& eventListeners = findIt->second;

				if (false == eventListeners.empty())
				{
					// call each listener
					for (auto it = eventListeners.begin(); it != eventListeners.end(); ++it)
					{
						EventListenerDelegate listener = (*it);
						try
						{
							listener(pEvent);
						}
						catch (const std::exception& e)
						{
							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[EventManager] Error using listener for event: " + Ogre::String(pEvent->getName()) + " Message: " + e.what());
						}
						
					}
				}
			}

			// Check to see if time ran out
			currMs = Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
			if (maxMillis != EventManager::kINFINITE && currMs >= maxMs)
			{
				// LOG("EventLoop", "Aborting event processing; time ran out");
				break;
			}
		}

		// If not all events could be processed, push the remaining events to the new active queue.
		// Note: To preserve sequencing, go back-to-front, inserting them at the head of the active queue
		bool queueFlushed = (this->queues[queueToProcess].empty());
		if (!queueFlushed)
		{
			while (!this->queues[queueToProcess].empty())
			{
				EventDataPtr pEvent = this->queues[queueToProcess].back();
				this->queues[queueToProcess].pop_back();
				this->queues[this->activeQueue].push_front(pEvent);
			}
		}

		return queueFlushed;
	}

	void EventManager::clearEvents(void)
	{
		this->queues[0].clear();
		this->queues[1].clear();
	}

}; // namespace end

