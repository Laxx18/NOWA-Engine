#ifndef PARTICLE_UNIVERSE_MODULE_H_
#define PARTICLE_UNIVERSE_MODULE_H_

#include "defines.h"

#ifdef WIN32

#include "ParticleUniverseSystem.h"

namespace NOWA
{
	struct EXPORTED ParticleUniverseData
	{
		ParticleUniverseData()
			: particle(nullptr),
			particleNode(nullptr),
			particlePlayTime(0.0f),
			particleInitialPlayTime(0.0f),
			particleOffsetPosition(Ogre::Vector3::ZERO),
			particleOffsetOrientation(Ogre::Quaternion::IDENTITY),
			particleScale(Ogre::Vector3(1.0f, 1.0f, 1.0f)),
			activated(false)
		{

		}
		ParticleUniverse::ParticleSystem* particle;
		Ogre::SceneNode* particleNode;
		Ogre::String particleTemplateName;
		Ogre::Real particlePlayTime;
		Ogre::Real particleInitialPlayTime;
		Ogre::Vector3 particleOffsetPosition;
		Ogre::Quaternion particleOffsetOrientation;
		Ogre::Vector3 particleScale;
		bool activated;
	};

	class EXPORTED ParticleUniverseModule
	{
	public:
		friend class AppState; // Only AppState may create this class

		//"PUMediaPack/Snow"
		//"PUMediaPack/Bubbles"
		//explosionSystem
		//electricBeamSystem!
		//PUMediaPack/Fireplace_01
		//flameSystem (nicht so gut)
		//fireSystem evtl. für vulkan
		//windyFireSystem
		//softParticlesSystem 
		//tornadoSystem
		//example_032 : forcefield
		//example_022: RibbonTrailRenderer
		//example_023:
		// - Exclude emitters
		// - PathFollower
		//example_027: Flocking Fish
		//PUMediaPack/Teleport
		//PUMediaPack/Atomicity
		//PUMediaPack/LineStreak
		//PUMediaPack/SpiralStars
		//PUMediaPack/TimeShift
		//PUMediaPack/Leaves
		//PUMediaPack/FlareShield
		//PUMediaPack/LightningBolt
		//PUMediaPack/PsychoBurn
		//PUMediaPack/Hypno
		//PUMediaPack/Smoke
		//PUMediaPack/CanOfWorms
		//PUMediaPack/Propulsion
		//PUMediaPack/BlackHole
		//PUMediaPack/ColouredStreaks
		//PUMediaPack/Woosh
		//PUMediaPack/ElectroShield
		//PUMediaPack/Fireplace_02
		//Water/Wake
		//example_028 für Vulkan! zu PerformanceLastig alle Examples!

		void init(Ogre::SceneManager* sceneManager);

		void createParticleSystem(const Ogre::String& name, const Ogre::String& templateName, Ogre::Real playTimeMS,
			Ogre::Quaternion orientation, Ogre::Vector3 position, Ogre::Real scale = 0.01f);

		void playParticleSystem(const Ogre::String& name);

		void playAndStopFadeParticleSystem(const Ogre::String& name, Ogre::Real stopTimeMS);

		void stopParticleSystem(const Ogre::String& name);

		void pauseParticleSystem(const Ogre::String& name, Ogre::Real pauseTimeMS = 0.0f);

		void resumeParticleSystem(const Ogre::String& name);

		void update(Ogre::Real dt);

		void destroyContent(void);

		ParticleUniverseData* getParticle(const Ogre::String& name);

		void removeParticle(const Ogre::String& name);

	private:
		ParticleUniverseModule(const Ogre::String& appStateName);
		~ParticleUniverseModule();
	private:
		Ogre::String appStateName;
		ParticleUniverse::ParticleSystem* particleUniverseSystem;
		Ogre::SceneManager* sceneManager;
		std::map<Ogre::String, ParticleUniverseData> particles;
	};


}; // namespace end

#endif

#endif /* PARTICLEUNIVERSEMODULE_H_ */

