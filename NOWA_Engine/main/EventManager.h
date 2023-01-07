#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <strstream>
#include "utilities/FastDelegate.h"
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
	typedef ConcurrentQueue<EventDataPtr> ThreadSafeEventQueue;


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
		virtual ~EventData(void) {}
		virtual const EventType getEventType(void) const = 0;
		virtual float getTimeStamp(void) const = 0;
		virtual void serialize(std::ostrstream& out) const = 0;
		virtual void deserialize(std::istrstream& in) = 0;
		virtual EventDataPtr copy(void) const = 0;
		virtual const char* getName(void) const = 0;

		//GCC_MEMORY_WATCHER_DECLARATION();
	};


	//---------------------------------------------------------------------------------------------------------------------
	// class BaseEventData		- Chapter 11, page 311
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED BaseEventData : public EventData
	{
		const float timeStamp;

	public:
		explicit BaseEventData(const float timeStamp = 0.0f) 
			: timeStamp(timeStamp) 
		{ 

		}

		// Returns the type of the event
		virtual const EventType getEventType(void) const = 0;

		float getTimeStamp(void) const { return this->timeStamp; }

		// Serializing for network input / output
		virtual void serialize(std::ostrstream& out) const	{ }
		virtual void deserialize(std::istrstream& in) { }
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
	//---------------------------------------------------------------------------------------------------------------------

	const unsigned int EVENTMANAGER_NUM_QUEUES = 2;

	class EXPORTED EventManager
	{
	public:
		friend class AppState; // Only AppState may create this class

		enum eConstants { kINFINITE = 0xffffffff };

		/**
		 * @brief		Registers a delegate function that will get called when the event type is triggered.
		 * @param[in]	eventDelegate	The event delegate to register
		 * @param[in]	type			The event type to register
		 * @return		success			true, if successful, false if not.
		 */
		bool addListener(const EventListenerDelegate& eventDelegate, const EventType& type);

		/**
		 * @brief		Removes a delegate / event type pairing from the internal tables.  
		 * @param[in]	eventDelegate	The event delegate to remove
		 * @param[in]	type			The event type to remove
		 * @return		success			Returns false if the pairing was not found.
		 */
		bool removeListener(const EventListenerDelegate& eventDelegate, const EventType& type);

		/**
		 * @brief		Fires off the event immediately or after delay seconds if specified. This bypasses the queue entirely and immediately calls all delegate functions registered 
		 * @param[in]	event	The event to fire
		 * @param[in]	delaySec			The optional delay in seconds
		 * @return		success			If the event could be triggered.
		 */
		bool triggerEvent(const EventDataPtr& event, Ogre::Real delaySec = 0.0f) const;

		/**
		 * @brief		Fires off the event. This uses the queue and will call the delegate function on the next call to update(), assuming there's enough time.
		 * @param[in]	event	The event to fire
		 * @return		success			If the event could be queued.
		 */
		bool queueEvent(const EventDataPtr& event);

		/**
		 * @brief		Fires off the event. This uses the queue and will call the delegate function on the next call to update(), assuming there's enough time.
		 *				This function can be called from two different threads, because its thread safe.
		 * @param[in]	event	The event to fire
		 * @return		success			If the event could be queued.
		 */
		bool threadSafeQueueEvent(const EventDataPtr& event);

		/**
		 * @brief		Cleares all events out of the queue.
		 */
		void clearEvents(void);

		/**
		 * @brief		Find the next-available instance of the named event type and remove it from the processing queue.
		 *				This  may be done up to the point that it is actively being processed, e.g.: Is safe to happen during event processing itself.
		 * @param[in]	type		The event type to abord
		 * @param[in]	allOfType	If allOfType is true, then all events of that type are cleared from the input queue.
		 * @return		success		Returns true if the event was found and removed, false otherwise.
		 */
		bool abortEvent(const EventType& type, bool allOfType = false);

		/**
		 * @brief		Gets whether the event of the given type does exist.
		 * @return		success			Returns true if the event was found.
		 */
		bool hasEvent(const EventType& type);

		/**
		 * @brief		Allow for processing of any queued messages.
		 * @param[in]	maxMillis	Optionally specify a processing time limit so that the event processing does not take too long.
		 *							Note the danger of using this artificial limiter is that all messages may not in fact get processed.
		 * @param[in]	type		If allOfType is true, then all events of that type are cleared from the input queue.
		 * @return		success		Returns true if all messages ready for processing were completed, false otherwise e.g. timeout.
		 */
		bool update(unsigned long maxMillis = kINFINITE);
	private:
		EventManager(const Ogre::String& appStateName);
		~EventManager();
	private:
		typedef std::list<EventListenerDelegate> EventListenerList;
		typedef std::map<EventType, EventListenerList> EventListenerMap;
		typedef std::list<EventDataPtr> EventQueue;

		Ogre::String appStateName;

		EventListenerMap eventListeners;
		EventQueue queues[EVENTMANAGER_NUM_QUEUES];
		int activeQueue;  // index of actively processing queue; events enque to the opposing queue

		ThreadSafeEventQueue realtimeEventQueue;
	};

}; // namespace end

#endif