#ifndef SCRIPT_EVENT_MANAGER_H
#define SCRIPT_EVENT_MANAGER_H

#include "defines.h"
#include "EventManager.h"
#include "ScriptEvent.h"
#include <strstream>
#include "utilities/FastDelegate.h"
#include <boost/shared_ptr.hpp>
#include <luabind/luabind.hpp>

class luabind::object;

namespace NOWA
{
	class EXPORTED ScriptEventListener
	{
	public:
		explicit ScriptEventListener(const EventType& eventType, luabind::object scriptCallbackFunction);

		~ScriptEventListener(void);

		EventListenerDelegate getDelegate(void);

		void scriptEventDelegate(EventDataPtr eventDataPtr);
	private:
		EventType eventType;
		luabind::object scriptCallbackFunction;
	};

	//////////////////////////////////////////////////////////////

	class EXPORTED ScriptEventManager
	{
	public:
		friend class AppState; // Only AppState may create this class

		/* 
		* @brief Destroy the singleton and all its listener.
		*/
		void destroyContent(void);

		void addListener(ScriptEventListener* listener);

		void destroyListener(ScriptEventListener* listener);

		/**
		 * @brief		Registers the event listener for lua.
		 * @param[in]	eventType			The event type to register to.
		 * @param[in]   callbackFunction	The callback function, that should be called in lue.
		 * @return		handle				The handle id of the listener, in order to destroy the listener.
		 * @note		This function should be called in a lua script.
		 */
		unsigned long registerEventListener(EventType eventType, luabind::object callbackFunction);

		void removeEventListener(unsigned long listenerId);

		bool queueEvent(EventType eventType, luabind::object eventData);

		bool triggerEvent(EventType eventType, luabind::object eventData);

		boost::shared_ptr<ScriptEvent> buildEvent(EventType eventType, luabind::object& eventData);

		void registerEvent(const Ogre::String& eventName);
	private:
		ScriptEventManager(const Ogre::String& appStateName);
		~ScriptEventManager();
	private:
		Ogre::String appStateName;
		std::set<ScriptEventListener*> listeners;
	};

}; // namespace end

#endif