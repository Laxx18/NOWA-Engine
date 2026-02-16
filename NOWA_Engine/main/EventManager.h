// EventManager.h
#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include "utilities/FastDelegate.h"
#include <mutex>
#include <strstream>
// #include "../Debugging/MemoryWatcher.h"
#include "gameobject/GameObjectFactory.h"
#include "utilities/ConcurrentQueue.h"
#include <boost/shared_ptr.hpp>

namespace NOWA
{

    //---------------------------------------------------------------------------------------------------------------------
    // Forward declaration & typedefs
    //---------------------------------------------------------------------------------------------------------------------
    class EventData;

    using namespace fastdelegate;

    typedef unsigned long EventType;
    typedef boost::shared_ptr<EventData> EventDataPtr;
    typedef FastDelegate1<EventDataPtr> EventListenerDelegate;
    typedef moodycamel::ConcurrentQueue<EventDataPtr> ThreadSafeEventQueue;

    //---------------------------------------------------------------------------------------------------------------------
    // Macro for event registration
    //---------------------------------------------------------------------------------------------------------------------
    extern GenericObjectFactory<EventData, EventType> eventFactory;
#define REGISTER_EVENT(eventClass) eventFactory.register<eventClass>(eventClass::EventType)
#define CREATE_EVENT(eventType) eventFactory.create(eventType)

    //---------------------------------------------------------------------------------------------------------------------
    // EventData                               - Chapter 11, page 310
    // Base type for event object hierarchy, may be used itself for simplest event notifications such as those that do
    // not carry additional payload data. If any event needs to propagate with payload data it must be defined separately.
    //---------------------------------------------------------------------------------------------------------------------
    class EXPORTED EventData
    {
    public:
        virtual ~EventData(void)
        {
        }
        virtual const EventType getEventType(void) const = 0;
        virtual float getTimeStamp(void) const = 0;
        virtual void serialize(std::ostrstream& out) const = 0;
        virtual void deserialize(std::istrstream& in) = 0;
        virtual EventDataPtr copy(void) const = 0;
        virtual const char* getName(void) const = 0;

        // GCC_MEMORY_WATCHER_DECLARATION();
    };

    //---------------------------------------------------------------------------------------------------------------------
    // class BaseEventData		- Chapter 11, page 311
    //---------------------------------------------------------------------------------------------------------------------
    class EXPORTED BaseEventData : public EventData
    {
        const float timeStamp;

    public:
        explicit BaseEventData(const float timeStamp = 0.0f) : timeStamp(timeStamp)
        {
        }

        // Returns the type of the event
        virtual const EventType getEventType(void) const = 0;

        float getTimeStamp(void) const
        {
            return this->timeStamp;
        }

        // Serializing for network input / output
        virtual void serialize(std::ostrstream& out) const
        {
        }
        virtual void deserialize(std::istrstream& in)
        {
        }
    };

    //---------------------------------------------------------------------------------------------------------------------
    // IEventManager Description                        Chapter 11, page 314
    //
    // This is the object which maintains the list of registered events and their listeners.
    //
    // This is a many-to-many relationship, as both one listener can be configured to process multiple event types and
    // of course multiple listeners can be registered to each event type.
    //
    // The interface to this construct uses smart pointer wrapped objects, the purpose being to ensure that no object
    // that the registry is referring to is destroyed before it is removed from the registry AND to allow for the registry
    // to be the only place where this list is kept ... the application code does not need to maintain a second list.
    //
    // Simply tearing down the registry (e.g.: destroying it) will automatically clean up all pointed-to objects (so long
    // as there are no other outstanding references, of course).
    //
    // THREAD SAFETY:
    // - queueEvent() is thread-safe and can be called from any thread (render or main)
    // - triggerEvent() should only be called from the main thread (it invokes listeners immediately)
    // - addListener() and removeListener() are thread-safe and can be called from any thread
    // - update() should only be called from the main thread
    //---------------------------------------------------------------------------------------------------------------------

    class EXPORTED EventManager
    {
    public:
        friend class AppState; // Only AppState may create this class

        enum eConstants
        {
            kINFINITE = 0xffffffff
        };

        /**
         * @brief		Registers a delegate function that will get called when the event type is triggered.
         *				Thread-safe: Can be called from any thread.
         * @param[in]	eventDelegate	The event delegate to register
         * @param[in]	type			The event type to register
         * @return		success			true, if successful, false if not.
         */
        bool addListener(const EventListenerDelegate& eventDelegate, const EventType& type);

        /**
         * @brief		Removes a delegate / event type pairing from the internal tables.
         *				Thread-safe: Can be called from any thread.
         * @param[in]	eventDelegate	The event delegate to remove
         * @param[in]	type			The event type to remove
         * @return		success			Returns false if the pairing was not found.
         */
        bool removeListener(const EventListenerDelegate& eventDelegate, const EventType& type);

        /**
         * @brief		Fires off the event immediately or after delay seconds if specified. This bypasses the queue entirely and immediately calls all delegate functions registered.
         *				Should only be called from the main thread (the thread that owns this EventManager).
         * @param[in]	event			The event to fire
         * @param[in]	delaySec		The optional delay in seconds
         * @return		success			If the event could be triggered.
         */
        bool triggerEvent(const EventDataPtr& event, Ogre::Real delaySec = 0.0f);

        /**
         * @brief		Queues the event for processing on the next update() call.
         *				Thread-safe: Can be called from any thread (render or main).
         * @param[in]	event	The event to queue
         * @return		success			If the event could be queued.
         */
        bool queueEvent(const EventDataPtr& event);

        /**
         * @brief		Cleares all events out of the queue.
         *				Should only be called from the main thread.
         */
        void clearEvents(void);

        /**
         * @brief		Find the next-available instance of the named event type and remove it from the processing queue.
         *				This may be done up to the point that it is actively being processed, e.g.: Is safe to happen during event processing itself.
         *				Note: With lock-free queue, this requires dequeueing all events, filtering, and re-enqueueing - less efficient than original implementation.
         *				Should only be called from the main thread.
         * @param[in]	type		The event type to abort
         * @param[in]	allOfType	If allOfType is true, then all events of that type are cleared from the input queue.
         * @return		success		Returns true if the event was found and removed, false otherwise.
         */
        bool abortEvent(const EventType& type, bool allOfType = false);

        /**
         * @brief		Gets whether the event of the given type does exist in the queue.
         *				Note: With lock-free queue, this requires dequeueing all events and re-enqueueing - less efficient than original implementation.
         *				Should only be called from the main thread.
         * @return		success			Returns true if the event was found.
         */
        bool hasEvent(const EventType& type);

        /**
         * @brief		Allow for processing of any queued messages.
         *				Should only be called from the main thread.
         * @param[in]	maxMillis	Optionally specify a processing time limit so that the event processing does not take too long.
         *							Note the danger of using this artificial limiter is that all messages may not in fact get processed.
         * @return		success		Returns true if all messages ready for processing were completed, false otherwise e.g. timeout.
         */
        bool update(unsigned long maxMillis = kINFINITE);

    private:
        EventManager(const Ogre::String& appStateName);
        ~EventManager();

    private:
        typedef std::list<EventListenerDelegate> EventListenerList;
        typedef std::map<EventType, EventListenerList> EventListenerMap;

        Ogre::String appStateName;

        // Thread-safe event queue using lock-free moodycamel queue
        ThreadSafeEventQueue eventQueue;

        // Listener map protected by mutex
        mutable std::mutex listenerMutex;
        EventListenerMap eventListeners;
    };

}; // namespace end

#endif