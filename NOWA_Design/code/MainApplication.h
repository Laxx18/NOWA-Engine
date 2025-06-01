#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#undef NOWA_CLIENT_BUILD

//Main application, that starts all states
#include "OgreString.h"
#include "OgreException.h"

class MainApplication
{
public:
	MainApplication();

	~MainApplication();

	void startSimulation(const Ogre::String& configName = "");
private:
	Ogre::String configName;
};

#endif
