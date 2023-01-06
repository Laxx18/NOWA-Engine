#include "BaseApplication.h"

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreConfigFile.h>
#include <OIS.h>
#include <OgreArchiveManager.h>
#include <OgreHlmsManager.h>
#include <Hlms\Unlit\OgreHlmsUnlit.h>

using namespace Ogre;

BaseApplication::BaseApplication() :
	mWindow(0), mOISMouse(0), mOISKeyboard(0), mShutdown(false)
{

}

BaseApplication::~BaseApplication()
{
	
}

void BaseApplication::run()
{	
	// Construct Ogre::Root
	mRoot = new Root(
        /* plugins.cfg file*/	"plugins.cfg",
        /* config file */ 		"ogre.cfg",
		/* log file */ 			"ogre.log"
	);

#if _DEBUG
	mRoot->loadPlugin(("./RenderSystem_GL3Plus_d"));
#else
	mRoot->loadPlugin(("./RenderSystem_GL3Plus"));
#endif
	mRoot->loadPlugin("./Plugin_ParticleUniverse");

    // Fire up Ogre::Root
    if (!mRoot->restoreConfig())
        mRoot->showConfigDialog();

    mWindow = mRoot->initialise(true);

	Ogre::ConfigFile cf;
	cf.load("resources.cfg");
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
	Ogre::String sec, type, arch;

	//register HLMS units
	Ogre::String dataFolder = cf.getSetting("FileSystem", "HLMS", "");

	if (dataFolder.empty())
		dataFolder = "./";
	else if (*(dataFolder.end() - 1) != '/')
		dataFolder += "/";

    Ogre::Archive *archiveCommon = Ogre::ArchiveManager::getSingletonPtr()->load(
        dataFolder + "Common/GLSL",
        "FileSystem", true);

    Ogre::ArchiveVec libraryCommon;
    libraryCommon.push_back(archiveCommon);

	Ogre::Archive *archiveUnlit = Ogre::ArchiveManager::getSingletonPtr()->load(
		dataFolder + "Unlit/GLSL",
		"FileSystem", true);

	Ogre::HlmsUnlit *hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, &libraryCommon);
	Ogre::Root::getSingleton().getHlmsManager()->registerHlms(hlmsUnlit);

	Ogre::Root::getSingleton().getHlmsManager()->useDefaultDatablockFrom(Ogre::HLMS_UNLIT);

	// go through all specified resource groups
	while (seci.hasMoreElements())
	{
		sec = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap* settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;

		//dont register HLMS specific shaders in normal resource pool
		if (sec != "HLMS")
		{
			// go through all resource paths
			for (i = settings->begin(); i != settings->end(); i++)
			{
				type = i->first;
				arch = i->second;
				Ogre::ResourceGroupManager::getSingleton().addResourceLocation(arch, type, sec);
			}
		}
	}

	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();




	WindowEventUtilities::addWindowEventListener(mWindow, this);
	
	size_t windowID;
	mWindow->getCustomAttribute("WINDOW", &windowID);

	// Create input system
	OIS::ParamList pl;
	std::ostringstream windowHndStr;
	windowHndStr << windowID;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
    #ifdef OIS_LINUX_PLATFORM
    pl.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));
    #endif
	mOIS = OIS::InputManager::createInputSystem( pl );
	// Create devices
	if (mOIS->getNumberOfDevices(OIS::OISKeyboard) > 0)
	{
		mOISKeyboard = static_cast<OIS::Keyboard*>(mOIS->createInputObject(OIS::OISKeyboard, true));
		mOISKeyboard->setEventCallback(this);
	}
	if (mOIS->getNumberOfDevices(OIS::OISMouse) > 0)
	{
		mOISMouse = static_cast<OIS::Mouse*>(mOIS->createInputObject(OIS::OISMouse, true));
		mOISMouse->setEventCallback(this);
	}
		
	// Start the rendering loop
	createScene();
	mRoot->addFrameListener(this);
	mRoot->startRendering();
	destroyScene();
	//Ogre::ResourceManager:: 
	
	
	// Shutdown
	if (mOISMouse)
		mOIS->destroyInputObject(mOISMouse);
	if (mOISKeyboard)
		mOIS->destroyInputObject(mOISKeyboard);
	mOIS->destroyInputSystem(mOIS);
	mRoot->removeFrameListener(this);
	delete mRoot;
}

void BaseApplication::windowResized(RenderWindow* rw)
{
	
}

void BaseApplication::windowClosed(RenderWindow* rw)
{
	mShutdown = true;
}
