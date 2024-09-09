#ifndef EVENTS_H
#define EVENTS_H

#include "EventManager.h"
#include "ProjectParameter.h"

#include <luabind/luabind.hpp>

class luabind::object;
class Ogre::Camera;

namespace NOWA
{

	void RegisterEngineScriptEvents(void);


	//---------------------------------------------------------------------------------------------------------------------
	// EventDataNewGameObject - This event is sent out when a game object is created.
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataNewGameObject : public BaseEventData
	{
	public:

		EventDataNewGameObject(void):
			id(0)
		{
		}

		explicit EventDataNewGameObject(const unsigned long id) 
			: id(id)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7c31;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7c31;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> Ogre::StringConverter::toString(this->id);
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataNewGameObject(this->id));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << Ogre::StringConverter::toString(this->id) << " ";
		}


		virtual const char* getName(void) const
		{
			return "EventDataNewGameObject";
		}

		const unsigned long getGameObjectId(void) const
		{
			return this->id;
		}
	private:
		unsigned long id;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataMoveGameObject - sent when game objects are moved
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataMoveGameObject : public BaseEventData
	{
	public:
		

		EventDataMoveGameObject(void)
			: id(0)
		{
		}

		EventDataMoveGameObject(unsigned int id, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
			: id(id),
			position(position),
			orientation(orientation)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xeeaa0a40;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xeeaa0a40;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->id << " ";
			out << this->position.x << " " << this->position.y << " " << this->position.z << " ";
			out << this->orientation.w << " " << this->orientation.x << " " << this->orientation.y << " " << this->orientation.z << " ";
		}

		virtual void deserialize(std::istrstream& in)
		{
			// not sure if this works
			in >> this->id;
			in >> this->position.x >> this->position.y >> this->position.z;
			in >> this->orientation.w >> this->orientation.x >> this->orientation.y >> this->orientation.z;
		}

		virtual EventDataPtr copy() const
		{
			return EventDataPtr(new EventDataMoveGameObject(this->id, this->position, this->orientation));
		}

		virtual const char* getName(void) const
		{
			return "EventDataMoveGameObject";
		}

		unsigned int getId(void) const
		{
			return this->id;
		}

		const Ogre::Vector3& getPosition(void) const
		{
			return this->position;
		}

		const Ogre::Quaternion& getOrientation(void) const
		{
			return this->orientation;
		}
	private:
		unsigned int id;
		Ogre::Vector3 position;
		Ogre::Quaternion orientation;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataParseGameObject - sent when a game object gets parsed from virtual environment
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataParseGameObject : public BaseEventData
	{
	public:

		EventDataParseGameObject(void)
			: controlledByClientID(0)
		{

		}

		EventDataParseGameObject(const Ogre::String& gameObjectName, unsigned int controlledByClientID)
			: gameObjectName(gameObjectName),
			controlledByClientID(controlledByClientID)
		{

		}

		static EventType getStaticEventType(void)
		{
			return 0xeeaa0a41;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xeeaa0a41;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->gameObjectName << " ";
			out << this->controlledByClientID << " ";
		}

		virtual void deserialize(std::istrstream& in)
		{
			// not sure if this works
			in >> this->gameObjectName;
			in >> this->controlledByClientID;
		}

		virtual EventDataPtr copy() const
		{
			return EventDataPtr(new EventDataParseGameObject(this->gameObjectName, this->controlledByClientID));
		}

		virtual const char* getName(void) const
		{
			return "EventDataParseGameObject";
		}

		Ogre::String getGameObjectName(void) const
		{
			return this->gameObjectName;
		}

		unsigned int getControlledByClientID(void) const
		{
			return this->controlledByClientID;
		}

	private:
		Ogre::String gameObjectName;
		unsigned int controlledByClientID;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataDeleteGameObject - This event is sent out when a game object has been deleted (for spawn component)
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataDeleteGameObject : public BaseEventData
	{
	public:

		EventDataDeleteGameObject(void) :
			id(0)
		{
		}

		explicit EventDataDeleteGameObject(const unsigned long id)
			: id(id)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7c44;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7c44;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> Ogre::StringConverter::toString(this->id);
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataDeleteGameObject(this->id));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << Ogre::StringConverter::toString(this->id) << " ";
		}


		virtual const char* getName(void) const
		{
			return "EventDataDeleteGameObject";
		}

		unsigned long getGameObjectId(void) const
		{
			return this->id;
		}
	private:
		unsigned long id;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataNewComponent - This event is sent out when a game object component has been created
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataNewComponent : public BaseEventData
	{
	public:

		EventDataNewComponent(void)
		{
		}

		explicit EventDataNewComponent(const Ogre::String& componentName)
			: componentName(componentName)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7c45;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7c45;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->componentName;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataNewComponent(this->componentName));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->componentName << " ";
		}

		virtual const char* getName(void) const
		{
			return "EventDataNewComponent";
		}

		Ogre::String getComponentName(void) const
		{
			return this->componentName;
		}
	private:
		Ogre::String componentName;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataDeleteComponent - This event is sent out when a game object component has been deleted
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataDeleteComponent : public BaseEventData
	{
	public:

		EventDataDeleteComponent(void)
		{
		}

		explicit EventDataDeleteComponent(const unsigned long gameObjectId, const Ogre::String& componentName, unsigned int componentIndex)
			: gameObjectId(gameObjectId),
			componentName(componentName),
			componentIndex(componentIndex)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe867cc45;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe867cc45;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->gameObjectId;
			in >> this->componentName;
			in >> this->componentIndex;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataDeleteComponent(this->gameObjectId, this->componentName, this->componentIndex));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->gameObjectId << " ";
			out << this->componentName << " ";
			out << this->componentIndex << " ";
		}

		virtual const char* getName(void) const
		{
			return "EventDataDeleteComponent";
		}

		unsigned long getGameObjectId(void) const
		{
			return this->gameObjectId;
		}

		Ogre::String getComponentName(void) const
		{
			return this->componentName;
		}

		unsigned int getComponentIndex(void) const
		{
			return componentIndex;
		}
	private:
		unsigned long gameObjectId;
		Ogre::String componentName;
		unsigned int componentIndex;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataSpringRelease - This event is sent out when a spring is released
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataSpringRelease : public BaseEventData
	{
	public:

		EventDataSpringRelease(void)
		{
		}

		explicit EventDataSpringRelease(const unsigned long id)
			: id(id)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7c46;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7c46;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			// out << this->id << " ";
		} 

		virtual void deserialize(std::istrstream& in)
		{
			// not sure if this works
			// in >> this->id;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataSpringRelease(this->id));
		}

		virtual const char* getName(void) const
		{
			return "EventDataSpringRelease";
		}

		const unsigned long getJointHandlerId(void) const
		{
			return this->id;
		}
	private:
		unsigned long id;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataEditorMode - This event is sent out when the editor manager mode has changed
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataEditorMode : public BaseEventData
	{
	public:

		explicit EventDataEditorMode(unsigned short manipulationMode)
			: manipulationMode(manipulationMode)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7c47;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7c47;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->manipulationMode << " ";
		}

		virtual void deserialize(std::istrstream& in)
		{
			// not sure if this works
			in >> this->manipulationMode;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataEditorMode(this->manipulationMode));
		}

		virtual const char* getName(void) const
		{
			return "EventDataEditorMode";
		}

		unsigned short getManipulationMode(void) const
		{
			return this->manipulationMode;
		}
	private:
		unsigned short manipulationMode;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataGenerateCategories - Sent when a category has changed, so that the categories are generated again (maybe a new has been added)
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataGenerateCategories : public NOWA::BaseEventData
	{
	public:

		EventDataGenerateCategories(void)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7c48;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7c48;
		}

		virtual void serialize(std::ostrstream& out) const
		{

		}

		virtual void deserialize(std::istrstream& in)
		{

		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataGenerateCategories());
		}

		virtual const char* getName(void) const
		{
			return "EventDataGenerateCategories";
		}
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataSwitchCamera - Sent when a camera is switched in camera component
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataSwitchCamera : public NOWA::BaseEventData
	{
	public:

		EventDataSwitchCamera(unsigned long cameraGameObjectId, unsigned int cameraComponentIndex, bool active)
			: cameraGameObjectId(cameraGameObjectId),
			cameraComponentIndex(cameraComponentIndex),
			active(active)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7c49;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7c49;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->cameraGameObjectId << " ";
			out << this->cameraComponentIndex << " ";
			out << this->active << " ";
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->cameraGameObjectId;
			in >> this->cameraComponentIndex;
			in >> this->active;
		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataSwitchCamera(this->cameraGameObjectId, this->cameraComponentIndex, this->active));
		}

		virtual const char* getName(void) const
		{
			return "EventDataSwitchCamera";
		}

		// id, index, active
		std::tuple<unsigned long, unsigned int, bool> getCameraGameObjectData(void)
		{
			return std::make_tuple(this->cameraGameObjectId, this->cameraComponentIndex, this->active);
		}
	private:
		unsigned long cameraGameObjectId;
		unsigned int cameraComponentIndex;
		bool active;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataRemoveCamera - Sent when a camera component is removed 
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataRemoveCamera : public NOWA::BaseEventData
	{
	public:

		EventDataRemoveCamera(bool active, Ogre::Camera* camera)
			: active(active),
			camera(camera)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7d49;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7d49;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->active << " ";
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->active;
		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataRemoveCamera(this->active, this->camera));
		}

		virtual const char* getName(void) const
		{
			return "EventDataRemoveCamera";
		}

		// id, index, active
		bool getCameraWasActive(void)
		{
			return this->active;
		}
		
		Ogre::Camera* getCamera(void) const
		{
			return this->camera;
		}
	private:
		bool active;
		Ogre::Camera* camera;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataSceneModified - Sent when something has been modified in the scene
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataSceneModified : public NOWA::BaseEventData
	{
	public:
		EventDataSceneModified()
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7c50;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7c50;
		}

		virtual void serialize(std::ostrstream& out) const
		{
		
		}

		virtual void deserialize(std::istrstream& in)
		{
			
		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataSceneModified());
		}

		virtual const char* getName(void) const
		{
			return "EventDataSceneModified";
		}
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataFeedback - Sent when something when wrong with feedback text
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataFeedback : public NOWA::BaseEventData
	{
	public:
		EventDataFeedback(bool positive, const Ogre::String& feedbackMessage)
			: positive(positive),
			feedbackMessage(feedbackMessage)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7c51;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7c51;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->positive;
			out << this->feedbackMessage;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->positive;
			in >> this->feedbackMessage;
		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataFeedback(this->positive, this->feedbackMessage));
		}

		virtual const char* getName(void) const
		{
			return "EventDataFeedback";
		}

		bool isPositive(void)
		{
			return this->positive;
		}

		Ogre::String getFeedbackMessage(void) const
		{
			return this->feedbackMessage;
		}
	private:
		bool positive;
		Ogre::String feedbackMessage;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataStopSimulation - Sent when something when wrong in order to stop a simulation
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataStopSimulation : public NOWA::BaseEventData
	{
	public:
		EventDataStopSimulation(const Ogre::String& feedbackMessage)
			: feedbackMessage(feedbackMessage)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7d51;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7d51;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->feedbackMessage;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->feedbackMessage;
		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataStopSimulation(this->feedbackMessage));
		}

		virtual const char* getName(void) const
		{
			return "EventDataStopSimulation";
		}

		Ogre::String getFeedbackMessage(void) const
		{
			return this->feedbackMessage;
		}
	private:
		Ogre::String feedbackMessage;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataActivatePlayerController - Sent when a player controller has been activated or deactivated
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataActivatePlayerController : public NOWA::BaseEventData
	{
	public:
		EventDataActivatePlayerController(bool active)
			: active(active)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7c52;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7c52;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->active << " ";
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->active;
		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataActivatePlayerController(this->active));
		}

		virtual const char* getName(void) const
		{
			return "EventDataActivatePlayerController";
		}

		bool getIsActive(void)
		{
			return this->active;
		}
	private:
		bool active;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataWorldLoaded - Sent when the world has been loaded
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataWorldLoaded : public NOWA::BaseEventData
	{
	public:
		EventDataWorldLoaded(bool worldChanged, const NOWA::ProjectParameter& projectParameter, const NOWA::SceneParameter& sceneParameter)
			: worldChanged(worldChanged),
			projectParameter(projectParameter),
			sceneParameter(sceneParameter)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7c53;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7c53;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			
		}

		virtual void deserialize(std::istrstream& in)
		{
			
		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataWorldLoaded(this->worldChanged, this->projectParameter, this->sceneParameter));
		}

		virtual const char* getName(void) const
		{
			return "EventDataWorldLoaded";
		}
		
		NOWA::ProjectParameter getProjectParameter(void) const
		{
			return this->projectParameter;
		}

		NOWA::SceneParameter getSceneParameter(void) const
		{
			return this->sceneParameter;
		}

		bool getWorldChanged(void) const
		{
			return this->worldChanged;
		}
	private:
		bool worldChanged;
		NOWA::ProjectParameter projectParameter;
		NOWA::SceneParameter sceneParameter;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataSceneParsed - Sent when the scene has been parsed completely
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataSceneParsed : public NOWA::BaseEventData
	{
	public:
		EventDataSceneParsed()
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7a53;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7a53;
		}

		virtual void serialize(std::ostrstream& out) const
		{

		}

		virtual void deserialize(std::istrstream& in)
		{

		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataSceneParsed());
		}

		virtual const char* getName(void) const
		{
			return "EventDataSceneParsed";
		}
	};
	
	//---------------------------------------------------------------------------------------------------------------------
	// EventPhysicsTrigger - Sent when either a physics trigger has been entered or leaved
	//---------------------------------------------------------------------------------------------------------------------
	class EventPhysicsTrigger : public NOWA::BaseEventData
	{
	public:
		EventPhysicsTrigger(unsigned long visitorGameObjectId, bool entered)
			: visitorGameObjectId(visitorGameObjectId),
			entered(entered)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe86c7c54;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe86c7c54;
		}

		virtual void serialize(std::ostrstream& out) const
		{
			
		}

		virtual void deserialize(std::istrstream& in)
		{
			
		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventPhysicsTrigger(this->visitorGameObjectId, this->entered));
		}

		virtual const char* getName(void) const
		{
			return "EventPhysicsTrigger";
		}

		unsigned long getVisitorGameObjectId(void)
		{
			return this->visitorGameObjectId;
		}
		
		bool getHasEntered(void) const
		{
			return this->entered;
		}
	private:
		unsigned long visitorGameObjectId;
		bool entered;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataBoundsUpdated - This event is sent out when the world bounds have been updated (world stored)
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataBoundsUpdated : public BaseEventData
	{
	public:

		EventDataBoundsUpdated(const Ogre::Vector3& mostLeftNearPosition, const Ogre::Vector3& mostRightFarPosition)
			: mostLeftNearPosition(mostLeftNearPosition),
			mostRightFarPosition(mostRightFarPosition)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7c55;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7c55;
		}

		virtual void deserialize(std::istrstream& in)
		{
			
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataBoundsUpdated(this->mostLeftNearPosition, this->mostRightFarPosition));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			
		}

		virtual const char* getName(void) const
		{
			return "EventDataBoundsUpdated";
		}

		std::pair<Ogre::Vector3, Ogre::Vector3> getCalculatedBounds(void) const
		{
			return std::make_pair(this->mostLeftNearPosition, this->mostRightFarPosition);
		}
	private:
		Ogre::Vector3 mostLeftNearPosition;
		Ogre::Vector3 mostRightFarPosition;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataDeleteWorkspaceComponent - This event is sent out when a game object workspace component is about to being deleted
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataDeleteWorkspaceComponent : public BaseEventData
	{
	public:

		EventDataDeleteWorkspaceComponent(void)
			: gameObjectId(0)
		{
		}

		explicit EventDataDeleteWorkspaceComponent(const unsigned long gameObjectId)
			: gameObjectId(gameObjectId)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe867cc46;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe867cc46;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->gameObjectId;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataDeleteWorkspaceComponent(this->gameObjectId));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->gameObjectId << " ";
		}

		virtual const char* getName(void) const
		{
			return "EventDataDeleteWorkspaceComponent";
		}

		unsigned long getGameObjectId(void) const
		{
			return this->gameObjectId;
		}
	private:
		unsigned long gameObjectId;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataGameObjectIsInRagDollingState - This event is sent out when a game object's rag doll state has changed to ragdolling
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataGameObjectIsInRagDollingState : public BaseEventData
	{
	public:

		EventDataGameObjectIsInRagDollingState(void)
			: gameObjectId(0),
			isInRagDollingState(false)
		{
		}

		explicit EventDataGameObjectIsInRagDollingState(const unsigned long gameObjectId, bool isInRagDollingState)
			: gameObjectId(gameObjectId),
			isInRagDollingState(isInRagDollingState)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe867cc47;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe867cc47;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->gameObjectId;
			in >> this->isInRagDollingState;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataGameObjectIsInRagDollingState(this->gameObjectId, this->isInRagDollingState));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->gameObjectId << " ";
			out << this->isInRagDollingState << " ";
		}

		virtual const char* getName(void) const
		{
			return "EventDataGameObjectIsInRagDollingState";
		}

		unsigned long getGameObjectId(void) const
		{
			return this->gameObjectId;
		}

		bool getIsInRagDollingState(void) const
		{
			return this->isInRagDollingState;
		}
	private:
		unsigned long gameObjectId;
		bool isInRagDollingState;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataAnimationChanged - This event is sent out when a game object's rag doll 
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataAnimationChanged : public BaseEventData
	{
	public:

		EventDataAnimationChanged(void)
			: gameObjectId(0),
			newAnimationId(0)
		{
		}

		explicit EventDataAnimationChanged(const unsigned long gameObjectId, short newAnimationId)
			: gameObjectId(gameObjectId),
			newAnimationId(newAnimationId)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe867cc48;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe867cc48;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->gameObjectId;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataAnimationChanged(this->gameObjectId, this->newAnimationId));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->gameObjectId << " ";
			out << this->newAnimationId << " ";
		}

		virtual const char* getName(void) const
		{
			return "EventDataAnimationChanged";
		}

		unsigned long getGameObjectId(void) const
		{
			return this->gameObjectId;
		}

		short getAnimationId(void) const
		{
			return this->newAnimationId;
		}
	private:
		unsigned long gameObjectId;
		short newAnimationId;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataWindowChanged - This event is sent out when the window is changed somehow
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataWindowChanged : public BaseEventData
	{
	public:

		explicit EventDataWindowChanged()
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe867cc49;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe867cc49;
		}

		virtual void deserialize(std::istrstream& in)
		{
			
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataWindowChanged());
		}

		virtual void serialize(std::ostrstream& out) const
		{
		
		}

		virtual const char* getName(void) const
		{
			return "EventDataWindowChanged";
		}
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataProjectEncoded - This event is sent out when the project has been encoded or decoded
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataProjectEncoded : public BaseEventData
	{
	public:

		EventDataProjectEncoded(void)
			: encoded(false)
		{
		}

		explicit EventDataProjectEncoded(bool encoded)
			: encoded(encoded)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe867cc50;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe867cc50;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->encoded;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataProjectEncoded(this->encoded));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->encoded << " ";
		}

		virtual const char* getName(void) const
		{
			return "EventDataProjectEncoded";
		}

		bool getIsEncoded(void) const
		{
			return this->encoded;
		}
	private:
		bool encoded;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataPrintLuaError - This event is sent out when an error from a lua script occurs
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataPrintLuaError: public BaseEventData
	{
	public:

		EventDataPrintLuaError(void)
			: line(0)
		{
		}

		explicit EventDataPrintLuaError(const Ogre::String& scriptName, int line, const Ogre::String& errorMessage)
			: scriptName(scriptName),
			line(line),
			errorMessage(errorMessage)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe867cc51;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe867cc51;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> this->scriptName;
			in >> this->line;
			in >> this->errorMessage;
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataPrintLuaError(this->scriptName, this->line, this->errorMessage));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << this->scriptName << " ";
			out << this->line << " ";
			out << this->errorMessage << " ";
		}

		virtual const char* getName(void) const
		{
			return "EventDataPrintLuaError";
		}

		Ogre::String getScriptName(void) const
		{
			return this->scriptName;
		}

		int getLine(void) const
		{
			return this->line;
		}

		Ogre::String getErrorMessage(void) const
		{
			return this->errorMessage;
		}
	private:
		Ogre::String scriptName;
		int line;
		Ogre::String errorMessage;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataTranslateFinished - Sent when gizmo translation is finished
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataTranslateFinished : public NOWA::BaseEventData
	{
	public:
		EventDataTranslateFinished(unsigned long gameObjectId, const Ogre::Vector3& newPosition)
			: gameObjectId(gameObjectId),
			newPosition(newPosition)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe867cd52;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe867cd52;
		}

		virtual void serialize(std::ostrstream& out) const
		{

		}

		virtual void deserialize(std::istrstream& in)
		{

		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataTranslateFinished(this->gameObjectId, this->newPosition));
		}

		virtual const char* getName(void) const
		{
			return "EventDataTranslateFinished";
		}

		Ogre::Vector3 getNewPosition(void) const
		{
			return this->newPosition;
		}

		unsigned long getGameObjectId(void) const
		{
			return this->gameObjectId;
		}
	private:
		unsigned long gameObjectId;
		Ogre::Vector3 newPosition;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataRotateFinished - Sent when gizmo translation is finished
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataRotateFinished : public NOWA::BaseEventData
	{
	public:
		EventDataRotateFinished(unsigned long gameObjectId, const Ogre::Quaternion& newOrientation)
			: gameObjectId(gameObjectId),
			newOrientation(newOrientation)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe867cd53;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe867cd53;
		}

		virtual void serialize(std::ostrstream& out) const
		{

		}

		virtual void deserialize(std::istrstream& in)
		{

		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataRotateFinished(this->gameObjectId, this->newOrientation));
		}

		virtual const char* getName(void) const
		{
			return "EventDataRotateFinished";
		}

		Ogre::Quaternion getNewOrientation(void) const
		{
			return this->newOrientation;
		}

		unsigned long getGameObjectId(void) const
		{
			return this->gameObjectId;
		}
	private:
		unsigned long gameObjectId;
		Ogre::Quaternion newOrientation;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataDefaultDirectionChanged - Sent when default direction changed
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataDefaultDirectionChanged : public NOWA::BaseEventData
	{
	public:
		EventDataDefaultDirectionChanged(unsigned long gameObjectId, const Ogre::Vector3& defaultDirection)
			: gameObjectId(gameObjectId),
			defaultDirection(defaultDirection)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xe867cd54;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xe867cd54;
		}

		virtual void serialize(std::ostrstream& out) const
		{

		}

		virtual void deserialize(std::istrstream& in)
		{

		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataDefaultDirectionChanged(this->gameObjectId, this->defaultDirection));
		}

		virtual const char* getName(void) const
		{
			return "EventDataDefaultDirectionChanged";
		}

		Ogre::Vector3 getDefaultDirection(void) const
		{
			return this->defaultDirection;
		}

		unsigned long getGameObjectId(void) const
		{
			return this->gameObjectId;
		}
	private:
		unsigned long gameObjectId;
		Ogre::Vector3 defaultDirection;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataRefreshGui - Sent when a property has changed, so that the properties panel can be refreshed with new values
	//---------------------------------------------------------------------------------------------------------------------
	class EventDataRefreshGui : public NOWA::BaseEventData
	{
	public:

		EventDataRefreshGui(void)
		{

		}

		static NOWA::EventType getStaticEventType(void)
		{
			return 0xeeab0002;
		}

		virtual const NOWA::EventType getEventType(void) const
		{
			return 0xeeab0002;
		}

		virtual void serialize(std::ostrstream& out) const
		{

		}

		virtual void deserialize(std::istrstream& in)
		{

		}

		virtual NOWA::EventDataPtr copy() const
		{
			return NOWA::EventDataPtr(new EventDataRefreshGui());
		}

		virtual const char* getName(void) const
		{
			return "EventDataRefreshPropertiesPanel";
		}
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataDeleteJoint - This event is sent out when a joint has been deleted in order to react and delete other joints
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataDeleteJoint : public BaseEventData
	{
	public:

		EventDataDeleteJoint(void) :
			jointId(0)
		{
		}

		explicit EventDataDeleteJoint(const unsigned long jointId)
			: jointId(jointId)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7d46;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7d46;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> Ogre::StringConverter::toString(this->jointId) >> Ogre::String(" ");
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataDeleteJoint(this->jointId));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << Ogre::StringConverter::toString(this->jointId) << " ";
		}


		virtual const char* getName(void) const
		{
			return "EventDataDeleteJoint";
		}

		unsigned long getJointId(void) const
		{
			return this->jointId;
		}

	private:
		unsigned long jointId;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataDeleteBody - This event is sent out when an OgreNewt body has been deleted in order to react and set used body pointer to 0
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataDeleteBody : public BaseEventData
	{
	public:

		EventDataDeleteBody(void) :
			body(nullptr)
		{
		}

		explicit EventDataDeleteBody(OgreNewt::Body* body)
			: body(body)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7d47;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7d47;
		}

		virtual void deserialize(std::istrstream& in)
		{
		
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataDeleteBody(this->body));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			
		}


		virtual const char* getName(void) const
		{
			return "EventDataDeleteBody";
		}

		OgreNewt::Body* getBody(void) const
		{
			return this->body;
		}

	private:
		OgreNewt::Body* body;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataLuaScriptConnected - This event is sent out the lua script has been connected.
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataLuaScriptConnected : public BaseEventData
	{
	public:

		EventDataLuaScriptConnected(void) :
			id(0)
		{
		}

		explicit EventDataLuaScriptConnected(const unsigned long id)
			: id(id)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7d48;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7d48;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> Ogre::StringConverter::toString(this->id);
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataLuaScriptConnected(this->id));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << Ogre::StringConverter::toString(this->id) << " ";
		}


		virtual const char* getName(void) const
		{
			return "EventDataLuaScriptConnected";
		}

		const unsigned long getGameObjectId(void) const
		{
			return this->id;
		}
	private:
		unsigned long id;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataCountdownActive - This event is sent out if a race countdown is active or not.
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataCountdownActive : public BaseEventData
	{
	public:

		EventDataCountdownActive(void)
			: isActive(false)
		{
		}

		explicit EventDataCountdownActive(bool isActive)
			: isActive(isActive)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe86c7d49;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe86c7d49;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> Ogre::StringConverter::toString(this->isActive);
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataCountdownActive(this->isActive));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << Ogre::StringConverter::toString(this->isActive) << " ";
		}


		virtual const char* getName(void) const
		{
			return "EventDataCountdownActive";
		}

		bool getIsActive(void) const
		{
			return this->isActive;
		}
	private:
		bool isActive;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataTerraChanged - This event is sent out if in terra data has changed like heightmap or blendmap
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataTerraChanged : public BaseEventData
	{
	public:

		EventDataTerraChanged(void)
			: heightMapChanged(false),
			blendMapChanged(false)
		{
		}

		explicit EventDataTerraChanged(bool heightMapChanged, bool blendMapChanged)
			: heightMapChanged(heightMapChanged),
			blendMapChanged(blendMapChanged)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe8329A01;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe8329A01;
		}

		virtual void deserialize(std::istrstream& in)
		{
			
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataTerraChanged(this->heightMapChanged, this->blendMapChanged));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			
		}


		virtual const char* getName(void) const
		{
			return "EventDataTerraChanged";
		}

		bool getIsHeightMapChanged(void) const
		{
			return this->heightMapChanged;
		}

		bool getIsBlendMapChanged(void) const
		{
			return this->blendMapChanged;
		}
	private:
		bool heightMapChanged;
		bool blendMapChanged;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataTerraModifyEnd - This event is sent out if modifying terra has ended
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataTerraModifyEnd : public BaseEventData
	{
	public:

		EventDataTerraModifyEnd(void)
		{
		}

		explicit EventDataTerraModifyEnd(const std::vector<uint16_t>& oldHeightData, const std::vector<uint16_t>& newHeightData, unsigned long terraCompId)
			: oldHeightData(oldHeightData),
			newHeightData(newHeightData),
			terraCompId(terraCompId)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe8329A02;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe8329A02;
		}

		virtual void deserialize(std::istrstream& in)
		{

		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataTerraModifyEnd(this->oldHeightData, this->newHeightData, this->terraCompId));
		}

		virtual void serialize(std::ostrstream& out) const
		{

		}


		virtual const char* getName(void) const
		{
			return "EventDataTerraModifyEnd";
		}

		std::vector<uint16_t> getOldHeightData(void) const
		{
			return this->oldHeightData;
		}

		std::vector<uint16_t> getNewHeightData(void) const
		{
			return this->newHeightData;
		}

		unsigned long getTerraCompId(void) const
		{
			return this->terraCompId;
		}
	private:
		std::vector<uint16_t> oldHeightData;
		std::vector<uint16_t> newHeightData;
		unsigned long terraCompId;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataTerraPaintEnd - This event is sent out if painting terra has ended
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataTerraPaintEnd : public BaseEventData
	{
	public:

		EventDataTerraPaintEnd()
		{
		}

		explicit EventDataTerraPaintEnd(const std::vector<uint8_t>& oldDetailBlendData, const std::vector<uint8_t>& newDetailBlendData, unsigned long terraCompId)
			: oldDetailBlendData(oldDetailBlendData),
			newDetailBlendData(newDetailBlendData),
			terraCompId(terraCompId)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe8329A03;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe8329A03;
		}

		virtual void deserialize(std::istrstream& in)
		{

		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataTerraPaintEnd(this->oldDetailBlendData, this->newDetailBlendData, this->terraCompId));
		}

		virtual void serialize(std::ostrstream& out) const
		{

		}


		virtual const char* getName(void) const
		{
			return "EventDataTerraPaintEnd";
		}

		std::vector<uint8_t> getOldDetailBlendData(void) const
		{
			return this->oldDetailBlendData;
		}

		std::vector<uint8_t> getNewDetailBlendData(void) const
		{
			return this->newDetailBlendData;
		}

		unsigned long getTerraCompId(void) const
		{
			return this->terraCompId;
		}
	private:
		std::vector<uint8_t> oldDetailBlendData;
		std::vector<uint8_t> newDetailBlendData;
		unsigned long terraCompId;
	};

	//---------------------------------------------------------------------------------------------------------------------
	// EventDataMyGUIWidgetSelected - This event is sent out if mygui widget has been selected
	//---------------------------------------------------------------------------------------------------------------------
	class EXPORTED EventDataMyGUIWidgetSelected : public BaseEventData
	{
	public:

		EventDataMyGUIWidgetSelected(void) :
			gameObjectId(0),
			gameObjectComponentIndex(0)
		{
		}

		explicit EventDataMyGUIWidgetSelected(const unsigned long gameObjectId, unsigned int gameObjectComponentIndex)
			: gameObjectId(gameObjectId),
			gameObjectComponentIndex(gameObjectComponentIndex)
		{
		}

		static EventType getStaticEventType(void)
		{
			return 0xe8329A05;
		}

		virtual const EventType getEventType(void) const
		{
			return 0xe8329A05;
		}

		virtual void deserialize(std::istrstream& in)
		{
			in >> Ogre::StringConverter::toString(this->gameObjectId) >> Ogre::StringConverter::toString(this->gameObjectComponentIndex);
		}

		virtual EventDataPtr copy(void) const
		{
			return EventDataPtr(new EventDataMyGUIWidgetSelected(this->gameObjectId, this->gameObjectComponentIndex));
		}

		virtual void serialize(std::ostrstream& out) const
		{
			out << Ogre::StringConverter::toString(this->gameObjectId) << " " << Ogre::StringConverter::toString(this->gameObjectComponentIndex);
		}


		virtual const char* getName(void) const
		{
			return "EventDataMyGUIWidgetSelected";
		}

		const unsigned long getGameObjectId(void) const
		{
			return this->gameObjectId;
		}

		const unsigned int getGameObjectComponentIndex(void) const
		{
			return this->gameObjectComponentIndex;
		}
	private:
		unsigned long gameObjectId;
		unsigned int gameObjectComponentIndex;
	};

}; // namespace end

#endif

