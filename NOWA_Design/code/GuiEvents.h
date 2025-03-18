#ifndef GUI_EVENTS_H
#define GUI_EVENTS_H

#include "main/Events.h"

//---------------------------------------------------------------------------------------------------------------------
// EventDataGameObjectTransformChanged - Sent when a game object position, scale, orientation has changed to send out to properties panel to change the values in the edit box
//---------------------------------------------------------------------------------------------------------------------
class EventDataGameObjectTransformChanged : public NOWA::BaseEventData
{
public:

	EventDataGameObjectTransformChanged(void)
		: gameObject(nullptr)
	{

	}

	EventDataGameObjectTransformChanged(NOWA::GameObject* gameObject)
		: gameObject(gameObject)
	{

	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0000;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0000;
	}

	virtual void serialize(std::ostrstream& out) const
	{
		
	}

	virtual void deserialize(std::istrstream& in)
	{
		
	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataGameObjectTransformChanged(this->gameObject));
	}

	virtual const char* getName(void) const
	{
		return "EventDataParseGameObject";
	}

	NOWA::GameObject* getGameObject(void) const
	{
		return this->gameObject;
	}

private:
	NOWA::GameObject* gameObject;
};

//---------------------------------------------------------------------------------------------------------------------
// EventDataRefreshPropertiesPanel - Sent when a property has changed, so that the properties panel can be refreshed with new values
//---------------------------------------------------------------------------------------------------------------------
class EventDataRefreshPropertiesPanel : public NOWA::BaseEventData
{
public:

	EventDataRefreshPropertiesPanel(void)
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
		return NOWA::EventDataPtr(new EventDataRefreshPropertiesPanel());
	}

	virtual const char* getName(void) const
	{
		return "EventDataRefreshPropertiesPanel";
	}
};

//---------------------------------------------------------------------------------------------------------------------
// EventDataRefreshResourcesPanel - Sent when a property has changed, so that the resources panel can be refreshed with new values
//---------------------------------------------------------------------------------------------------------------------
class EventDataRefreshResourcesPanel : public NOWA::BaseEventData
{
public:

	EventDataRefreshResourcesPanel(void)
	{
	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0003;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0003;
	}

	virtual void serialize(std::ostrstream& out) const
	{

	}

	virtual void deserialize(std::istrstream& in)
	{

	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataRefreshResourcesPanel());
	}

	virtual const char* getName(void) const
	{
		return "EventDataRefreshResourcesPanel";
	}
};

//---------------------------------------------------------------------------------------------------------------------
// EventDataExit - Sent when any exit button has been clicked
//---------------------------------------------------------------------------------------------------------------------
class EventDataExit : public NOWA::BaseEventData
{
public:

	EventDataExit(void)
	{

	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0004;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0004;
	}

	virtual void serialize(std::ostrstream& out) const
	{

	}

	virtual void deserialize(std::istrstream& in)
	{

	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataExit());
	}

	virtual const char* getName(void) const
	{
		return "EventDataExit";
	}
};

/*
 * @class EventDataProjectManipulation
 * @brief Sent when a new project has been created, or saved or opened.
 * mode = 0: new scene created, mode = 1: scene opened, mode = 2: scene saved
 */
class EventDataProjectManipulation : public NOWA::BaseEventData
{
public:

	EventDataProjectManipulation(unsigned short mode)
		: mode(mode)
	{

	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0005;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0005;
	}

	virtual void serialize(std::ostrstream& out) const
	{

	}

	virtual void deserialize(std::istrstream& in)
	{

	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataProjectManipulation(this->mode));
	}

	virtual const char* getName(void) const
	{
		return "EventDataProjectManipulation";
	}

	/*
	 * @brief Gets the mode of project manipulation
	 * @return mode = 0: new scene created, mode = 1: scene opened, mode = 2: scene saved
	 */
	unsigned short getMode(void) const
	{
		return this->mode;
	}
private:
	unsigned short mode;
};

/*
* @class EventDataSceneValid
* @brief Sent when a new project wants to be created or opened, so that the scene is not valid until the process is finished
*/
class EventDataSceneValid : public NOWA::BaseEventData
{
public:

	EventDataSceneValid(bool sceneValid)
		: sceneValid(sceneValid)
	{

	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0006;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0006;
	}

	virtual void serialize(std::ostrstream& out) const
	{

	}

	virtual void deserialize(std::istrstream& in)
	{

	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataSceneValid(this->sceneValid));
	}

	virtual const char* getName(void) const
	{
		return "EventDataSceneValid";
	}

	bool getSceneValid(void) const
	{
		return this->sceneValid;
	}
private:
	bool sceneValid;
};

//---------------------------------------------------------------------------------------------------------------------
// EventDataShowComponentsPanel - Sent when components panel should be shown
//---------------------------------------------------------------------------------------------------------------------
class EventDataShowComponentsPanel : public NOWA::BaseEventData
{
public:

	EventDataShowComponentsPanel(int index = -1)
		: index(index)
	{

	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0007;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0007;
	}

	virtual void serialize(std::ostrstream& out) const
	{

	}

	virtual void deserialize(std::istrstream& in)
	{

	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataShowComponentsPanel(this->index));
	}

	virtual const char* getName(void) const
	{
		return "EventDataShowComponentsPanel";
	}

	int getIndex(void) const
	{
		return index;
	}
private:
	int index;
};

//---------------------------------------------------------------------------------------------------------------------
// EventDataRefreshMeshResources - Sent when new mesh resources has been loaded
//---------------------------------------------------------------------------------------------------------------------
class EventDataRefreshMeshResources : public NOWA::BaseEventData
{
public:

	EventDataRefreshMeshResources(void)
	{

	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0008;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0008;
	}

	virtual void serialize(std::ostrstream& out) const
	{

	}

	virtual void deserialize(std::istrstream& in)
	{

	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataRefreshMeshResources());
	}

	virtual const char* getName(void) const
	{
		return "EventDataRefreshMeshResources";
	}
};

//---------------------------------------------------------------------------------------------------------------------
// EventDataTestSelectedGameObjects - Sent when editor is in test activated game objects mode
//---------------------------------------------------------------------------------------------------------------------
class EventDataTestSelectedGameObjects : public NOWA::BaseEventData
{
public:

	EventDataTestSelectedGameObjects(bool bActive)
		: bActive(bActive)
	{

	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0009;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0009;
	}

	virtual void serialize(std::ostrstream& out) const
	{

	}

	virtual void deserialize(std::istrstream& in)
	{

	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataTestSelectedGameObjects(this->bActive));
	}

	virtual const char* getName(void) const
	{
		return "EventDataTestSelectedGameObjects";
	}

	bool isActive(void) const
	{
		return bActive;
	}
private:
	bool bActive;
};

//---------------------------------------------------------------------------------------------------------------------
// EventDataSceneInvalid - Sent when a loaded scene is invalid
//---------------------------------------------------------------------------------------------------------------------
class EventDataSceneInvalid : public NOWA::BaseEventData
{
public:

	EventDataSceneInvalid(unsigned char recentFileIndex, const Ogre::String& sceneFilePathName)
		: recentFileIndex(recentFileIndex),
		sceneFilePathName(sceneFilePathName)
	{

	}

	static NOWA::EventType getStaticEventType(void)
	{
		return 0xeeab0010;
	}

	virtual const NOWA::EventType getEventType(void) const
	{
		return 0xeeab0010;
	}

	virtual void serialize(std::ostrstream& out) const
	{

	}

	virtual void deserialize(std::istrstream& in)
	{

	}

	virtual NOWA::EventDataPtr copy() const
	{
		return NOWA::EventDataPtr(new EventDataSceneInvalid(this->recentFileIndex, this->sceneFilePathName));
	}

	virtual const char* getName(void) const
	{
		return "EventDataSceneInvalid";
	}

	unsigned char getRecentFileIndex(void) const
	{
		return this->recentFileIndex;
	}

	Ogre::String getSceneFilePathName(void) const
	{
		this->sceneFilePathName;
	}
private:
	unsigned char recentFileIndex;
	Ogre::String sceneFilePathName;
};

#endif

