#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include "NOWA.h"
#include "OpenSaveFileDialogExtended.h"

class NOWA::EditorManager;

class ProjectManager
{
public:

	enum eProjectMode
	{
		NEW = 0,
		LOAD = 1,
		SAVE = 2,
		NOT_READY = 3
	};
	
	ProjectManager(Ogre::SceneManager* sceneManager);

	~ProjectManager();

	void setEditorManager(NOWA::EditorManager* editorManager);

	NOWA::EditorManager* getEditorManager(void) const;

	void createNewProject(const NOWA::ProjectParameter& projectParameter);

	void applySettings(const NOWA::ProjectParameter& projectParameter);

	bool showFileOpenDialog(const Ogre::String& action, const Ogre::String& fileMask);

	void showFileSaveDialog(const Ogre::String& action, const Ogre::String& fileMask, const Ogre::String& specifiedTargetFolder = Ogre::String());

	void saveProject(const Ogre::String& optionalFileName = Ogre::String());

	void loadProject(const Ogre::String& filePathName);

	void saveGroup(const Ogre::String& filePathName);

	Ogre::String getSceneFileName(void) const;

	void setOgreNewt(OgreNewt::World* ogreNewt);

	OgreNewt::World* getOgreNewt(void) const;

	void setProjectParameter(NOWA::ProjectParameter projectParameter);

	NOWA::ProjectParameter getProjectParameter(void);
private:
	void notifyEndDialog(tools::Dialog* sender, bool result);
	void notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);
	bool checkProjectExists(const Ogre::String& fileName);
	Ogre::Light* createSunLight(void);
	Ogre::Camera* createMainCamera(void);
	void createMainGameObject(void);
	void destroyScene(void);
	void internalApplySettings(void);
	void createDummyCamera(void);
private:
	Ogre::SceneManager* sceneManager;
	Ogre::Light* sunLight;
	NOWA::EditorManager* editorManager;
	NOWA::DotSceneImportModule* dotSceneImportModule;
	NOWA::DotSceneExportModule* dotSceneExportModule;

	OgreNewt::World* ogreNewt;

	OpenSaveFileDialogExtended* openSaveFileDialog;

	NOWA::ProjectParameter projectParameter;

	std::vector<Ogre::String> additionalMeshResources;
};

#endif
