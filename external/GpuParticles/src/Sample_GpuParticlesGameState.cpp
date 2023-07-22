
#include "Sample_GpuParticlesGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"
#include "Utils/MeshUtils.h"

#include "OgreSceneManager.h"

#include "OgreRoot.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreCamera.h"
#include "OgreWindow.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"

#include "GpuParticles/GpuParticleSystemWorld.h"
#include <GpuParticles/GpuParticleSystem.h>
#include "GpuParticles/GpuParticleEmitter.h"
#include "GpuParticles/Hlms/HlmsParticle.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"

#include "OgreTextureGpuManager.h"

#include "OgreLwString.h"
#include "OgreGpuProgramManager.h"

#include "OgreItem.h"

#include <OgreHlmsPbsDatablock.h>

#include <GpuParticles/GpuParticleSystemJsonManager.h>
#include <GpuParticles/GpuParticleSystemResourceManager.h>

#include <iostream>
#include <fstream>

#include <GpuParticles/Affectors/GpuParticleDepthCollisionAffector.h>
#include <GpuParticles/Affectors/GpuParticleGlobalGravityAffector.h>
#include <GpuParticles/Affectors/GpuParticleSetColourTrackAffector.h>

using namespace Demo;

namespace Demo
{
    Sample_GpuParticlesGameState::Sample_GpuParticlesGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mGpuParticleSystemWorld( 0 ),
        mFireParticleSystem( 0 ),
        mSparksParticleSystem( 0 ),
        mFireManualParticleSystem ( 0 ),
        mSparksManualParticleSystem( 0 ),
        mDirectionalLight( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    void Sample_GpuParticlesGameState::createScene01(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        mGraphicsSystem->getCamera()->setPosition( Ogre::Vector3( 0, 30, 100 ) );

        Ogre::SceneNode* rootNode = sceneManager->getRootSceneNode( Ogre::SCENE_STATIC );

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 5000.0f, 5000.0f,
                                            1, 1, true, 1, 4.0f, 4.0f, Ogre::Vector3::UNIT_Z,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
                    "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    planeMeshV1.get(), true, true, true );

        {
            Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1, 0 );
            sceneNode->attachObject( item );
        }

        // Init particle system world
        {
            Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
            HlmsParticle* hlmsParticle = static_cast<HlmsParticle*>( hlmsManager->getHlms(Ogre::HLMS_USER0) );
            HlmsParticleListener* hlmsParticleListener = hlmsParticle->getParticleListener();

            std::vector<GpuParticleAffector*> affectors;
            GpuParticleSystemResourceManager& gpuParticleSystemResourceManager = GpuParticleSystemResourceManager::getSingleton();
            const GpuParticleSystemResourceManager::AffectorByIdStringMap& registeredAffectors = gpuParticleSystemResourceManager.getAffectorByPropertyMap();
            for(GpuParticleSystemResourceManager::AffectorByIdStringMap::const_iterator it = registeredAffectors.begin();
                it != registeredAffectors.end(); ++it) {

                 affectors.push_back(it->second->clone());
            }

            bool useDepthTexture = true;

            // Compositor is needed only in case of useDepthTexture == true.
            Ogre::IdString depthTextureCompositorNode = "Sample_GpuParticlesNode";
            Ogre::IdString depthTextureId = "depthTextureCopy";

            mGpuParticleSystemWorld = OGRE_NEW GpuParticleSystemWorld(
                        Ogre::Id::generateNewId<Ogre::MovableObject>(),
                        &sceneManager->_getEntityMemoryManager( Ogre::SCENE_DYNAMIC ),
                        sceneManager, 15, hlmsParticleListener, affectors,
                        useDepthTexture, mGraphicsSystem->getCompositorWorkspace(), depthTextureCompositorNode, depthTextureId);

    //        mGpuParticleSystemWorld->init(4096, 64, 256, 64, 64);
//            mGpuParticleSystemWorld->init(32768, 256, 256, 64, 64);
//            mGpuParticleSystemWorld->init(131072, 1024, 256, 64, 64);
            mGpuParticleSystemWorld->init(524288, 4096, 256, 64, 64);
//            mGpuParticleSystemWorld->init(2097152, 16384, 16, 64, 64);


            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->attachObject(mGpuParticleSystemWorld);


            registerParticleEmitters();


            mEmitterInstancesCount = 256;
            recreateEmitterInstances();
        }

        mDirectionalLight = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( mDirectionalLight );
        mDirectionalLight->setPowerScale( Ogre::Math::PI );
        mDirectionalLight->setType( Ogre::Light::LT_DIRECTIONAL );
        mDirectionalLight->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        sceneManager->setAmbientLight( Ogre::ColourValue( 0.33f, 0.61f, 0.98f ) * 0.01f,
                                       Ogre::ColourValue( 0.02f, 0.53f, 0.96f ) * 0.01f,
                                       Ogre::Vector3::UNIT_Y );

        mCameraController = new CameraController( mGraphicsSystem, false );
        mGraphicsSystem->getCamera()->setFarClipDistance( 100000.0f );
        mGraphicsSystem->getCamera()->setPosition( -10.0f, 10.0f, 10.0f );
        mGraphicsSystem->getCamera()->lookAt(Ogre::Vector3(10.0f, 0.0f, 10.0f));

        TutorialGameState::createScene01();
    }

    void Sample_GpuParticlesGameState::registerParticleEmitters()
    {
        {
            mFireManualParticleSystem = GpuParticleSystemResourceManager::getSingleton().createParticleSystem("FireManual", "FireManual");

            GpuParticleEmitter* emitterCore = new GpuParticleEmitter();

//            emitterCore->mDatablockName = "ParticleAlphaBlend";
            emitterCore->mDatablockName = "ParticleAdditive";
            emitterCore->mSpriteMode = GpuParticleEmitter::SpriteMode::SetWithStart;
            emitterCore->mSpriteFlipbookCoords.push_back(HlmsParticleDatablock::SpriteCoord(0, 0));
            emitterCore->mSpriteFlipbookCoords.push_back(HlmsParticleDatablock::SpriteCoord(0, 1));
            emitterCore->mSpriteFlipbookCoords.push_back(HlmsParticleDatablock::SpriteCoord(1, 0));
            emitterCore->mSpriteFlipbookCoords.push_back(HlmsParticleDatablock::SpriteCoord(1, 1));
            emitterCore->mSpriteTimes.push_back(0.0f);
            emitterCore->mSpriteTimes.push_back(0.15f);
            emitterCore->mSpriteTimes.push_back(0.30f);
            emitterCore->mSpriteTimes.push_back(0.45f);
//            emitterCore->mEmissionRate = 30.0f;
            emitterCore->mEmissionRate = 60.0f;
//            emitterCore->mParticleLifetime = Geometry::Range(0.8f, 1.0f);
//            emitterCore->mParticleLifetime = Geometry::Range(0.5f, 0.7f);
            emitterCore->mParticleLifetimeMin = 0.8f;
            emitterCore->mParticleLifetimeMax = 1.0f;
            emitterCore->mColourA = Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f);
            emitterCore->mColourB = Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f);
            emitterCore->mUniformSize = true;
            emitterCore->mSizeMin = 0.2f;
            emitterCore->mSizeMax = 0.5f;
            emitterCore->mDirectionVelocityMin = 0.9f;
            emitterCore->mDirectionVelocityMax = 1.2f;
//            emitterCore->mDirectionVelocity = Geometry::Range(1.6f, 1.8f);
            emitterCore->mSpotAngleMin = 0.0f;
            emitterCore->mSpotAngleMax = (float)M_PI/36.0f;

            {
                GpuParticleSetColourTrackAffector* setColourTrackAffector = OGRE_NEW GpuParticleSetColourTrackAffector();
                setColourTrackAffector->mEnabled = true;

    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.0f, Ogre::Vector3(1.000f, 1.000f, 1.000f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(1.0f, Ogre::Vector3(1.000f, 1.000f, 1.000f)));

    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.0f, Ogre::Vector3(1.000f, 0.878f, 0.000f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.3f, Ogre::Vector3(1.000f, 0.486f, 0.000f)));
    //// //            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.3f, Ogre::Vector3(1.000f, 0.486f, 0.184f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(1.0f, Ogre::Vector3(0.192f, 0.020f, 0.000f)));

                setColourTrackAffector->mColourTrack.insert(std::make_pair(0.0f, Ogre::Vector3(0.750f, 0.659f, 0.000f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.3f, Ogre::Vector3(0.750f, 0.365f, 0.184f)));
                setColourTrackAffector->mColourTrack.insert(std::make_pair(0.2f, Ogre::Vector3(0.750f, 0.365f, 0.000f)));
                setColourTrackAffector->mColourTrack.insert(std::make_pair(0.7f, Ogre::Vector3(0.144f, 0.015f, 0.000f)));

    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(0.0f, Ogre::Vector3(0.0f, 0.0f, 0.0f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(1.0f, Ogre::Vector3(1.0f, 0.0f, 0.0f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(2.0f, Ogre::Vector3(1.0f, 1.0f, 0.0f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(3.0f, Ogre::Vector3(0.0f, 1.0f, 0.0f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(4.0f, Ogre::Vector3(0.0f, 1.0f, 1.0f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(5.0f, Ogre::Vector3(0.0f, 0.0f, 1.0f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(6.0f, Ogre::Vector3(1.0f, 0.0f, 1.0f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(7.0f, Ogre::Vector3(1.0f, 1.0f, 1.0f)));
    //            setColourTrackAffector->mColourTrack.insert(std::make_pair(8.0f, Ogre::Vector3(1.0f, 0.0f, 0.0f)));

                emitterCore->addAffector(setColourTrackAffector);
            }


            emitterCore->mFaderMode = GpuParticleEmitter::FaderMode::Enabled;
//            emitterCore->mFaderMode = GpuParticleEmitter::FaderMode::AlphaOnly;
            emitterCore->mParticleFaderStartTime = 0.1f;
            emitterCore->mParticleFaderEndTime = 0.2f;

            mFireManualParticleSystem->addEmitter(emitterCore);
        }
        mGpuParticleSystemWorld->registerEmitterCore(mFireManualParticleSystem);

        {
            mSparksManualParticleSystem = GpuParticleSystemResourceManager::getSingleton().createParticleSystem("SparksManual", "SparksManual");

            GpuParticleEmitter* emitterCore = new GpuParticleEmitter();

            emitterCore->setBurst(250, 0.2f);
//            emitterCore->mEmissionRate = 60.0f;
            emitterCore->mDatablockName = "Examples/SparkParticle";
            emitterCore->mParticleLifetimeMin = 3.0f;
            emitterCore->mParticleLifetimeMax = 4.0f;
            emitterCore->mColourA = Ogre::ColourValue(0.925f, 0.82f, 0.412f, 1.0f);
            emitterCore->mColourB = Ogre::ColourValue(0.925f, 0.82f, 0.412f, 1.0f);
            emitterCore->mSizeMin = 0.02f;
            emitterCore->mSizeMax = 0.02f;
            emitterCore->mSizeYMin = 0.16f;
            emitterCore->mSizeYMax = 0.16f;
            emitterCore->mDirectionVelocityMin = 3.0f;
            emitterCore->mDirectionVelocityMax = 3.5f;
            emitterCore->mSpotAngleMin = 0.0f;
            emitterCore->mSpotAngleMax = (float)M_PI/6.0f;
            emitterCore->mUniformSize = false;
            emitterCore->mBillboardType = Ogre::v1::BBT_ORIENTED_SELF;

            GpuParticleDepthCollisionAffector* depthCollisionAffector = OGRE_NEW GpuParticleDepthCollisionAffector();
            depthCollisionAffector->mEnabled = true;
            emitterCore->addAffector(depthCollisionAffector);

            GpuParticleGlobalGravityAffector* globalGravityAffector = OGRE_NEW GpuParticleGlobalGravityAffector();
            globalGravityAffector->mGravity = Ogre::Vector3(0.0f, -1.0f, 0.0f);
            emitterCore->addAffector(globalGravityAffector);

            mSparksManualParticleSystem->addEmitter(emitterCore);
        }
        mGpuParticleSystemWorld->registerEmitterCore(mSparksManualParticleSystem);


        {
            mFireParticleSystem = GpuParticleSystemResourceManager::getSingleton().getGpuParticleSystem("Fire");
            mGpuParticleSystemWorld->registerEmitterCore(mFireParticleSystem);
        }

        {
            mSparksParticleSystem = GpuParticleSystemResourceManager::getSingleton().getGpuParticleSystem("Sparks");
            mGpuParticleSystemWorld->registerEmitterCore(mSparksParticleSystem);
        }

//        // Save particle systems to files.
//        {
//            Ogre::String outText;
//            GpuParticleSystemJsonManager::getSingleton().saveGpuParticleSystem(mFireParticleSystem, outText);

//            std::ofstream myfile;
//            myfile.open("fire.gpuparticle.json");
//            myfile << outText.c_str();
//            myfile.close();
//        }

//        {
//            Ogre::String outText;
//            GpuParticleSystemJsonManager::getSingleton().saveGpuParticleSystem(mSparksParticleSystem, outText);

//            std::ofstream myfile;
//            myfile.open("sparks.gpuparticle.json");
//            myfile << outText.c_str();
//            myfile.close();
//        }

//        {
//            Ogre::String outText;
//            GpuParticleSystemJsonManager::getSingleton().saveGpuParticleSystem(mFireManualParticleSystem, outText);

//            std::ofstream myfile;
//            myfile.open("fire_manual.gpuparticle.json");
//            myfile << outText.c_str();
//            myfile.close();
//        }

//        {
//            Ogre::String outText;
//            GpuParticleSystemJsonManager::getSingleton().saveGpuParticleSystem(mSparksManualParticleSystem, outText);

//            std::ofstream myfile;
//            myfile.open("sparks_manual.gpuparticle.json");
//            myfile << outText.c_str();
//            myfile.close();
//        }
    }

    //-----------------------------------------------------------------------------------
    void Sample_GpuParticlesGameState::destroyScene(void)
    {
        delete mGpuParticleSystemWorld;
        mGpuParticleSystemWorld = 0;

        TutorialGameState::destroyScene();
    }

    void Sample_GpuParticlesGameState::recreateEmitterInstances()
    {
        mFireParticleInstances.clear();
        mGpuParticleSystemWorld->stopAndRemoveAllImmediately();

        int sizeX = 1;
        int sizeZ = 1;

        int count = 1;
        while(count < mEmitterInstancesCount) {
            count *= 2;
            sizeX *= 2;
            if(count < mEmitterInstancesCount) {
                count *= 2;
                sizeZ *= 2;
            }
        }
        assert(sizeX * sizeZ == mEmitterInstancesCount);

        for (int iz = 0; iz < sizeZ; ++iz) {
            for (int ix = 0; ix < sizeX; ++ix) {
                Ogre::Vector3 p(ix*2.0f, 1.0f, iz*2.0f);
                Ogre::uint64 particleInstanceId = mGpuParticleSystemWorld->start(mFireParticleSystem, nullptr, p, Ogre::Quaternion());
                mFireParticleInstances.push_back(particleInstanceId);
            }
        }
    }

    //-----------------------------------------------------------------------------------
    void Sample_GpuParticlesGameState::update( float timeSinceLast )
    {
        //Do not call update() while invisible, as it will cause an assert because the frames
        //are not advancing, but we're still mapping the same GPU region over and over.
        if( mGraphicsSystem->getRenderWindow()->isVisible() ) {
            mGpuParticleSystemWorld->processTime(timeSinceLast);
        }

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void Sample_GpuParticlesGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        if( mDisplayHelpMode == 0 )
        {
            outText += "\nCtrl+F4 will reload Particles's shaders.";
        }
        else if( mDisplayHelpMode == 1 )
        {
            char tmp[128];
            Ogre::LwString str( Ogre::LwString::FromEmptyPointer(tmp, sizeof(tmp)) );
            Ogre::Vector3 camPos = mGraphicsSystem->getCamera()->getPosition();

            GpuParticleSystemWorld::Info info = mGpuParticleSystemWorld->getInfo();

            using namespace Ogre;

            outText += "\n+/- to change particle instances. [";
            outText += StringConverter::toString( mEmitterInstancesCount ) + "]";
            outText += "\n6 to burst particles.";
            outText += "\n\nCamera: ";
            str.a( "[", LwString::Float( camPos.x, 2, 2 ), ", ",
                        LwString::Float( camPos.y, 2, 2 ), ", ",
                        LwString::Float( camPos.z, 2, 2 ), "]" );
            outText += str.c_str();
            outText += "\nParticles alive: ";
            outText += StringConverter::toString(info.mAliveParticles).c_str();
            outText += "\nParticles in used buckets capacity: ";
            outText += StringConverter::toString(info.mUsedBucketsParticleCapacity).c_str();
            outText += "\nParticles created (alive or created ahead of time): ";
            outText += StringConverter::toString(info.mParticlesCreated).c_str();
            outText += "\nParticles added this frame: ";
            outText += StringConverter::toString(info.mParticlesAddedThisFrame).c_str();
        }
    }
    //-----------------------------------------------------------------------------------
    void Sample_GpuParticlesGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( arg.keysym.sym == SDLK_F4 && (arg.keysym.mod & (KMOD_LCTRL|KMOD_RCTRL)) )
        {
            //Hot reload of Particle shaders.
            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

            Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_USER0 );
            Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
            hlms->reloadFrom( hlms->getDataFolder() );
        }
        else if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.scancode == SDL_SCANCODE_KP_PLUS )
        {
            if(mEmitterInstancesCount*2 <= (int)mGpuParticleSystemWorld->getMaxEmitterInstances()) {
                mEmitterInstancesCount = mEmitterInstancesCount * 2;
                recreateEmitterInstances();
            }
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_MINUS ||
                 arg.keysym.scancode == SDL_SCANCODE_KP_MINUS )
        {
            if(mEmitterInstancesCount > 1) {
                mEmitterInstancesCount = mEmitterInstancesCount / 2;
                recreateEmitterInstances();
            }
        }

        if( arg.keysym.scancode == SDL_SCANCODE_KP_6 )
        {
            Ogre::Vector3 p(5.0f, 1.0f, 5.0f);
            Ogre::uint64 particleInstanceId = mGpuParticleSystemWorld->start(mSparksParticleSystem, nullptr, p, Ogre::Quaternion(Ogre::Radian((float)M_PI*0.75f), Ogre::Vector3::UNIT_X));
        }

        TutorialGameState::keyReleased( arg );
    }
}
