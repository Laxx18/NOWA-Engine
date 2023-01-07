#ifndef OGRE_NEWT_MODULE_H
#define OGRE_NEWT_MODULE_H

#include "defines.h"
#include "OgreNewt.h"

namespace NOWA
{
	class EXPORTED OgreNewtModule
	{
	public:
		friend class AppState; // Only AppState may create this class

		/*
		* @brief Creates the physics.
		* @param[in] name								The name of the physics world.
		* @param[in] solverModel						The solver model to use.
		* @details:										The model of operation n = number of iteration default value is 4. n: the solve will execute a maximum of n iteration per cluster of connected joints and will terminate regardless of the 
		*												of the joint residual acceleration. If it happen that the joints residual acceleration fall below the minimum tolerance 1.0e-5 then the solve will terminate before the number of iteration reach N.
		* @param[in] broadPhaseAlgorithm				The broad phase algorithm to use.
		* @details										For example you have 16000 objects. If the ratio of objects with zero mass (PhysicsArtifactComponents) to dynamics objects (PhysicsActiveComponent) is high, say may few hundred bodies are actually dynamics, 
		*												then your game is spending lot of time check traversing static collision for objects that do not move. Is this some kind of vegetation terrain where many object are static?
		*												If so, you are using the engine is the worse possible way. The engine has tow special ways to deal with that.
		*												0- There is the scene collision, when the zero mass object are all bundle in static compound collision.
		*												you can have many scene collision, basically you divide your world into sectors and for each sector you add the collision of these objects, this reduces your world for 15000, 
		*												to maybe a couple of dozen and you get maybe 100 fold speed up, but you have to do some house keeping. This is my prefect way.
		*												1- If in your scene there are objects that need to be dynamics, you can use the persistence broadphase. On initiation select that options, and the world will be divided 
		*												at the root level into two large nodes, one node for the static bodies and the other the dynamics. Default is 1.
		* @param[in] multithreadSolverOnSingleIsland	Whether to use multi thread solver on single island.
		* @details										Enable/disable multi-threaded constraint resolution for large islands.
		*												Multi threaded mode is not always faster. Among the reasons are
		*												1 - Significant software cost to set up threads, as well as instruction overhead.
		*												2 - Different systems have different cost for running separate threads in a shared memory environment.
		*												3 - Parallel algorithms often have decreased converge rate. This can be as
		*													high as half of the of the sequential version. Consequently, the parallel
		*													solver requires a higher number of interactions to achieve similar convergence.
		*
		*												It is recommended this option is enabled on system with more than two cores,
		*												since the performance gain in a dual core system are marginally better. Your
		*												mileage may vary.
		*
		*												At the very least the application must test the option to verify the performance gains.
		*
		*												This option has no impact on other subsystems of the engine.
		* @param[in] threadCount						The maximum number of threaded is set on initialization to the maximum number.
		* @param[in] updateRate							The physics update rate. Default value are 120 ticks per second.
		* @param[in] defaultLinearDamping				The default linear damping for all dynamic bodies.
		* @param[in] defaultLinearDamping				The default angular damping for all dynamic bodies.
		* @param[in] defaultLinearDamping				The default angular damping for all dynamic bodies. 
		*/
		
		OgreNewt::World* createPhysics(const Ogre::String& name, int solverModel = 1, int broadPhaseAlgorithm = 1, int multithreadSolverOnSingleIsland = 1,
			int threadCount = 1, Ogre::Real updateRate = 120.0f, Ogre::Real defaultLinearDamping = 0.1f, Ogre::Vector3 defaultAngularDamping = Ogre::Vector3(0.1f, 0.1f, 0.1f));

		/*initialise OgreNewt physics (performant, also using 2 threads for simulation) (Info: OgreNewt Objekt gets created on a heap)
		Params: 
		updateRate: how often update OgreNewt per second (60 times is the minimum)
		*/
		OgreNewt::World* createPerformantPhysics(const Ogre::String& name, Ogre::Real updateRate = 120.0f);

		/*initialise OgreNewt physics (quality) (Info: OgreNewt Objekt gets created on a heap)
		*/
		OgreNewt::World* createQualityPhysics(const Ogre::String& name, Ogre::Real updateRate = 120.0f);

		/*
		Initialize a OgreNewt debugger in order to show collision lines during the simulation
		Params: 
		sceneManager: The SceneManager, that should paint the collision lines
		showText: Whether to show debug text infos about the bodies.
		@Note: When there are a lot of bodies, the text is not really readable. When its sufficient to just show the collision hull, showText may be set to false.
		*/
		void enableOgreNewtCollisionLines(Ogre::SceneManager* sceneManager, bool showText = true);

		OgreNewt::World* getOgreNewt(void) const;

		/*
		Show/Unshow the lines
		Params: 
		enabled: (true) --> show the lines
		(false) --> hide the lines
		*/
		void showOgreNewtCollisionLines(bool enabled);

		void destroyContent(void);

		void update(Ogre::Real dt);

		void setGlobalGravity(const Ogre::Vector3& globalGravity);

		Ogre::Vector3 getGlobalGravity(void) const;
	private:
		OgreNewtModule(const Ogre::String& appStateName);
		~OgreNewtModule();
	private:
		Ogre::String appStateName;
		OgreNewt::World* ogreNewt;
		bool showText;
		Ogre::Vector3 globalGravity;
	};

}; //namespace end

#endif
