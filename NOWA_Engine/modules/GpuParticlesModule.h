#ifndef GPU_PARTICLES_MODULE_H
#define GPU_PARTICLES_MODULE_H

#include "defines.h"

#include "GpuParticles/GpuParticleSystemWorld.h"
#include "GpuParticles/GpuParticleSystemResourceManager.h"
#include "GpuParticles/GpuParticleSystemJsonManager.h"

namespace NOWA
{
	class EXPORTED GpuParticlesModule
	{
	public:
		friend class AppState; // Only AppState may create this class

		/**
		 * @brief Real maximum number of particles will be aligned to bucket size.
		 *  @param bucketSize - number of particles per bucket. 64 because this is default number of threads per group.
		 *  @param maxEmitterCores - all emitter cores (recipes) are stored in uav buffer. This param is necessary to calculate size of this buffer. All systems handled by GpuParticleSystemWorld must share the same datablock.
		 */
		void init(Ogre::SceneManager* sceneManager, Ogre::uint32 maxParticles = 4096, Ogre::uint16 maxEmitterInstances = 1024, Ogre::uint16 maxEmitterCores = 256,
				  Ogre::uint16 bucketSize = 64, Ogre::uint16 gpuThreadsPerGroup = 64);

		void destroyContent(void);

		GpuParticleSystemWorld* getGpuParticleSystemWorld(void) const;

		GpuParticleSystemResourceManager* getGpuParticleSystemResourceManager(void) const;

	private:
		GpuParticlesModule(const Ogre::String& appStateName);
		~GpuParticlesModule();
	private:
		Ogre::String appStateName;
		Ogre::SceneManager* sceneManager;
		Ogre::SceneNode* sceneNode;
		GpuParticleSystemWorld* gpuParticleSystemWorld;
		GpuParticleSystemResourceManager* gpuParticleSystemResourceManager;
		GpuParticleSystemJsonManager* gpuParticleSystemJsonManager;
	};

}; //namespace end

#endif
