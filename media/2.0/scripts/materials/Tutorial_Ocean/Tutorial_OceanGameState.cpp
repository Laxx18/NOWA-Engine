/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2025 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "Tutorial_OceanGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreRoot.h"
#include "OgreCamera.h"
#include "OgreWindow.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreHlmsManager.h"
#include "Ocean/Hlms/OgreHlmsOcean.h"
#include "Ocean/Hlms/OgreHlmsOceanDatablock.h"
#include "Ocean/Ocean.h"

#include "OgreItem.h"

#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
#    include "OgreAtmosphereNpr.h"
#endif

using namespace Demo;

namespace Demo
{
    Tutorial_OceanGameState::Tutorial_OceanGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mUnderwaterMode( false ),
        mUseSkirts( true ),
        mTimeOfDay( Ogre::Math::PI * 0.3f ),
        mAzimuth( 0 ),
        mWaveTimeAccumulator( 0.0f ),
        mOcean( 0 ),
        mSunLight( 0 ),
        mOceanDatablock( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_OceanGameState::createScene01()
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::Camera *camera = mGraphicsSystem->getCamera();

        // Create ocean
        mOcean = new Ogre::Ocean(
            Ogre::Id::generateNewId<Ogre::MovableObject>(),
            &sceneManager->_getEntityMemoryManager( Ogre::SCENE_DYNAMIC ),
            sceneManager,
            0,
            root->getCompositorManager2(),
            camera );
        
        mOcean->setCastShadows( false );
        mOcean->setName( "Tutorial_Ocean" );
        
        // Create ocean with center position and size
        Ogre::Vector3 oceanCenter( 0.0f, 0.0f, 0.0f );
        Ogre::Vector2 oceanSize( 2048.0f, 2048.0f );
        mOcean->create( oceanCenter, oceanSize );
        
        // Runtime wave parameters
        mOcean->setWavesIntensity( 1.0f );
        mOcean->setWavesScale( 1.0f );
        mOcean->setWaveTimeScale( 1.0f );
        mOcean->setWaveFrequencyScale( 1.0f );
        mOcean->setWaveChaos( 0.5f );

        // Attach ocean to scene node
        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC );
        Ogre::SceneNode *oceanNode = rootNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        oceanNode->attachObject( mOcean );

        // Setup HlmsOcean and datablock
        Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
        Ogre::HlmsOcean *hlmsOcean = static_cast<Ogre::HlmsOcean*>(
            hlmsManager->getHlms( Ogre::HLMS_USER1 ) );
        
        // Set ocean data textures
        hlmsOcean->setOceanDataTextureName( "oceanData.dds" );
        hlmsOcean->setWeightTextureName( "weight.dds" );
        
        // Create ocean datablock
        Ogre::String datablockName = "OceanMaterial";
        mOceanDatablock = static_cast<Ogre::HlmsOceanDatablock*>(
            hlmsOcean->createDatablock( datablockName, datablockName,
                Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );
        
        mOcean->setDatablock( mOceanDatablock );
        
        // Configure ocean colors
        mOceanDatablock->setDeepColour( Ogre::Vector3( 0.0f, 0.05f, 0.2f ) );
        mOceanDatablock->setShallowColour( Ogre::Vector3( 0.0f, 0.5f, 0.7f ) );
        
        // Shader-side detail scale
        mOceanDatablock->setWavesScale( 1.0f );
        
        mOcean->setStatic( false );
        mOcean->update( 0.0f );

        // Setup sun light
        mSunLight = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( mSunLight );
        mSunLight->setPowerScale( Ogre::Math::PI );
        mSunLight->setType( Ogre::Light::LT_DIRECTIONAL );
        mSunLight->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.2f,
                                       Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.05f,
                                       -mSunLight->getDirection() );

#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
        mGraphicsSystem->createAtmosphere( mSunLight );
        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::AtmosphereNpr *>( sceneManager->getAtmosphere() ) );
        Ogre::AtmosphereNpr *atmosphere =
            static_cast<Ogre::AtmosphereNpr *>( sceneManager->getAtmosphere() );
        Ogre::AtmosphereNpr::Preset p = atmosphere->getPreset();
        p.fogDensity = 0.00002f;
        p.fogBreakMinBrightness = 0.1f;
        atmosphere->setPreset( p );
#endif

        // Setup camera
        mCameraController = new CameraController( mGraphicsSystem, false );
        mCameraController->mCameraBaseSpeed = 50.0f;
        mCameraController->mCameraSpeedBoost = 5.0f;
        camera->setPosition( 0.0f, 20.0f, 100.0f );
        camera->lookAt( 0.0f, 0.0f, 0.0f );
        camera->setFarClipDistance( 50000.0f );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_OceanGameState::destroyScene()
    {
        delete mOcean;
        mOcean = 0;
        mOceanDatablock = 0;

        TutorialGameState::destroyScene();
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_OceanGameState::switchWorkspace()
    {
        Ogre::CompositorManager2 *compositorManager = 
            mGraphicsSystem->getRoot()->getCompositorManager2();
        
        if( mGraphicsSystem->getWorkspace() )
        {
            compositorManager->removeWorkspace( mGraphicsSystem->getWorkspace() );
        }

        const char *workspaceName = mUnderwaterMode ? 
            "Tutorial_OceanUnderwaterWorkspace" : "Tutorial_OceanWorkspace";
        
        Ogre::CompositorWorkspace *workspace = compositorManager->addWorkspace(
            mGraphicsSystem->getSceneManager(),
            mGraphicsSystem->getRenderWindow()->getTexture(),
            mGraphicsSystem->getCamera(),
            workspaceName,
            true );
        
        mGraphicsSystem->setWorkspace( workspace );
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_OceanGameState::update( float timeSinceLast )
    {
        // Update sun direction
        mSunLight->setDirection(
            Ogre::Quaternion( Ogre::Radian( mAzimuth ), Ogre::Vector3::UNIT_Y ) *
            Ogre::Vector3( cosf( mTimeOfDay ), -sinf( mTimeOfDay ), 0.0 ).normalisedCopy() );

#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        if( sceneManager->getAtmosphere() )
        {
            Ogre::AtmosphereNpr *atmosphere =
                static_cast<Ogre::AtmosphereNpr *>( sceneManager->getAtmosphere() );
            atmosphere->setSunDir( mSunLight->getDirection(), mTimeOfDay / Ogre::Math::PI );
        }
#endif

        // Update ocean waves
        if( mGraphicsSystem->getRenderWindow()->isVisible() )
        {
            mWaveTimeAccumulator += timeSinceLast;
            mOcean->update( mWaveTimeAccumulator );
        }

        // Check if camera went underwater and switch workspace if needed
        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        Ogre::Vector3 camPos = camera->getPosition();
        bool isUnderwater = mOcean->isCameraUnderwater( camPos );
        
        if( isUnderwater != mUnderwaterMode )
        {
            mUnderwaterMode = isUnderwater;
            switchWorkspace();
        }

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_OceanGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        if( mDisplayHelpMode == 0 )
        {
            outText += "\nCtrl+F4 will reload Ocean's shaders.";
        }
        else if( mDisplayHelpMode == 1 )
        {
            using namespace Ogre;

            Ogre::Vector3 camPos = mGraphicsSystem->getCamera()->getPosition();

            outText += "\nUnderwater Mode: [";
            outText += mUnderwaterMode ? "Yes]" : "No]";
            outText += "\nUse Skirts: [";
            outText += mUseSkirts ? "Yes]" : "No]";
            outText += "\n+/- to change time of day. [";
            outText += StringConverter::toString( mTimeOfDay * 180.0f / Math::PI ) + "]";
            outText += "\n9/6 to change azimuth. [";
            outText += StringConverter::toString( mAzimuth * 180.0f / Math::PI ) + "]";
            outText += "\n\nCamera: [";
            outText += StringConverter::toString( camPos.x, 2 ) + ", ";
            outText += StringConverter::toString( camPos.y, 2 ) + ", ";
            outText += StringConverter::toString( camPos.z, 2 ) + "]";
            outText += "\nWave Time: ";
            outText += StringConverter::toString( mWaveTimeAccumulator, 2 );
        }
    }
    //-----------------------------------------------------------------------------------
    void Tutorial_OceanGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( arg.keysym.sym == SDLK_F4 && ( arg.keysym.mod & ( KMOD_LCTRL | KMOD_RCTRL ) ) )
        {
            // Hot reload of Ocean shaders
            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

            Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_USER1 );
            Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
            hlms->reloadFrom( hlms->getDataFolder() );
        }
        else if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS ) ) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.scancode == SDL_SCANCODE_KP_PLUS )
        {
            mTimeOfDay += 0.1f;
            mTimeOfDay = std::min( mTimeOfDay, (float)Ogre::Math::PI );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_MINUS ||
                 arg.keysym.scancode == SDL_SCANCODE_KP_MINUS )
        {
            mTimeOfDay -= 0.1f;
            mTimeOfDay = std::max( mTimeOfDay, 0.0f );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_KP_9 )
        {
            mAzimuth += 0.1f;
            mAzimuth = fmodf( mAzimuth, Ogre::Math::TWO_PI );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_KP_6 )
        {
            mAzimuth -= 0.1f;
            mAzimuth = fmodf( mAzimuth, Ogre::Math::TWO_PI );
            if( mAzimuth < 0 )
                mAzimuth = Ogre::Math::TWO_PI + mAzimuth;
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo