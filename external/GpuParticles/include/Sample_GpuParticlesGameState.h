
#ifndef _Demo_Sample_GpuParticlesGameState_H_
#define _Demo_Sample_GpuParticlesGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

class GpuParticleSystem;
class GpuParticleSystemWorld;

namespace Demo
{
    class Sample_GpuParticlesGameState : public TutorialGameState
    {
        int mEmitterInstancesCount;
        GpuParticleSystemWorld* mGpuParticleSystemWorld;
        const GpuParticleSystem* mFireParticleSystem;
        const GpuParticleSystem* mSparksParticleSystem;
        GpuParticleSystem* mFireManualParticleSystem;
        GpuParticleSystem* mSparksManualParticleSystem;
        std::vector<Ogre::uint64> mFireParticleInstances;
Ogre::uint64 particleInstanceId;
        Ogre::Node* mNode = nullptr;
        Ogre::Light *mDirectionalLight;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        Sample_GpuParticlesGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
        void registerParticleEmitters();
        virtual void destroyScene(void);

        void recreateEmitterInstances();

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
