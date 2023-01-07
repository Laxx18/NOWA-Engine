#ifndef SCRIPT_EVENT_H
#define SCRIPT_EVENT_H

#include "EventManager.h"
#include <luabind/luabind.hpp>

class luabind::object;

namespace NOWA
{
	class ScriptEvent;
	// Function ptr typedef to create a script event
	typedef ScriptEvent* (*CreateEventForScriptFunctionType)(EventType);

	//---------------------------------------------------------------------------------------------------------------------
	// These macros implement exporting events to script.
	//---------------------------------------------------------------------------------------------------------------------
#define REGISTER_SCRIPT_EVENT(eventClass, eventType) \
		ScriptEvent::registerEventTypeWithScript(#eventClass, eventType); \
		ScriptEvent::addCreationFunction(eventType, &eventClass::createEventForScriptMakro)

#define EXPORT_FOR_SCRIPT_EVENT(eventClass, eventType) \
		public: \
			static ScriptEvent* createEventForScriptMakro(void) \
			{ \
				return new eventClass(eventType); \
			}

	//---------------------------------------------------------------------------------------------------------------------
	// Script event base class.  This class is meant to be subclassed by any event that can be sent or received by the
	// script.  Note that these events are not limited to script and can be received just fine by C++ listeners.
	// Furthermore, since the Script data isn't built unless being received by a script listener, there's no worry about 
	// performance.
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED ScriptEvent : public BaseEventData
	{
	public:
		friend class ScriptEventManager;

		ScriptEvent(EventType eventType)
			: eventType(eventType),
			eventDataIsValid(false)
		{

		}

		/**
		 * @brief		Called when event is sent from C++ to script.
		 * @return		eventData	The lua event data to get.
		 */
		luabind::object getEventData(void);  

		/**
		 * @brief		Sets the event data for lua.
		 * @param[in]	eventData	The event data to set.
		 * @return		success		True if the data could be set.
		 */
		bool setEventData(luabind::object eventData); 

		/**
		 * @brief		Static helper function for registering events with the script. 
		 *				You should call the REGISTER_SCRIPT_EVENT() macro instead of calling this function directly.
		 *				Any class that needs to be exported also needs to call the EXPORT_FOR_SCRIPT_EVENT() inside the class declaration.
		 * @note:        This function is called to register an event type with the script to link them.  The simplest way to do this is 
		 *              with the REGISTER_SCRIPT_EVENT() macro, which calls this function.
		 * @param[in]	key			The class name as string
		 * @param[in]	type		The event type
		 */
		static void registerEventTypeWithScript(const char* key, EventType eventType);

		/**
		 * @brief		Static helper function for the event instance creation.
		 *				You should call the REGISTER_SCRIPT_EVENT() macro instead of calling this function directly.
		 *				Any class that needs to be exported also needs to call the EXPORT_FOR_SCRIPT_EVENT() inside the class declaration.
		 * @note		This function is called to map an event creation function pointer with the event type.  This allows an event to be
	     *				created by only knowing its type.  This is required to allow scripts to trigger the instantiation and queueing of events.
		 * @param[in]	type				The event type
		 * @param[in]	creationFunctionPtr	The creation function pointer
		 */
		static void addCreationFunction(EventType type, CreateEventForScriptFunctionType creationFunctionPtr);

		static EventType getStaticEventType(void)
		{
			return -1;
		}

		virtual const EventType getEventType(void) const
		{
			return this->eventType;
		}

		virtual void deserialize(std::istrstream& in)
		{
			
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr();
		}

		virtual void serialize(std::ostrstream& out) const
		{
			
		}

		virtual const char* getName(void) const
		{
			return "ScriptEvent";
		}

	protected:
		/**
		 * @brief		This function must be overridden if you want to fire this event from C++ and have it received by the script.
		 *				If you only fire the event from the script side, this function will never be called.  It's purpose is to 
		 *				fill in the eventData member, which is then passed to the script callback function in the listener.
		 *				This is only called the first time getEventData() is called.  If the event is script-only, this function does not 
	     *				created by only knowing its type.  This is required to allow scripts to trigger the instantiation and queueing of events.
		 *				need to be overridden.
		 */
		virtual void buildEventDataFromCppToLuaScript(void);

		/**
		 * @brief		This function must be overridden if you want to fire this event from Script and have it received by C++.
		 *				If you only fire this event from script and have it received by the script, it doesn't matter since eventData
		 *				will just be passed straight through.  Its purpose is to fill in any C++ member variables using the data in 
		 *				eventData (which is valid at the time of the call).  It is called when the event is fired from the script.
		 * @return		success		Return false if the data is invalid in some way, which will keep the event from actually firing.
		 */
		virtual bool buildEventFromLuaScriptToCpp(void)
		{
			return true;
		}
	protected:
		luabind::object eventData;
	private:
		typedef std::map<EventType, CreateEventForScriptFunctionType> CreationFunctions;
		static CreationFunctions s_creationFunctions;

		static ScriptEvent* instantiateScript(EventType eventType);

		/**
		 * @brief		Static helper function for the event instance creation.
		 *				You should call the REGISTER_SCRIPT_EVENT() macro instead of calling this function directly.
		 *				Any class that needs to be exported also needs to call the EXPORT_FOR_SCRIPT_EVENT() inside the class declaration.
		 * @param[in]	type				The event type
		 * @param[in]	creationFunctionPtr	The creation function pointer
		 */
		static ScriptEvent* createEventFromScript(EventType eventType);
	private:
		bool eventDataIsValid;
		EventType eventType;
	};

}; // namespace end

#endif

