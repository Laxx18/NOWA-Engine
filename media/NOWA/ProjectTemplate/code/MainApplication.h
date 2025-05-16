#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#undef NOWA_CLIENT_BUILD

//Main application, that starts all states
#include "OgreString.h"
#include "OgreException.h"

#include <condition_variable>
#include <mutex>
#include <thread>

class MainApplication
{
public:
	MainApplication();
	~MainApplication();
	//Start simulation
	void startSimulation(const Ogre::String& graphicsConfigName = "");
private:
	void renderThreadFunction(void);
private:
	Ogre::String configName;
	std::condition_variable renderInitCondition;
	std::mutex renderInitMutex;
	bool renderInitialized = false;
};

#endif
