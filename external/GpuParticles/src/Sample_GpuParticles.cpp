
#include "GraphicsSystem.h"
#include "Sample_GpuParticlesGameState.h"

#include "OgreRoot.h"
#include "Compositor/OgreCompositorManager2.h"
#include "OgreConfigFile.h"
#include "OgreWindow.h"

#include "GpuParticles/Hlms/HlmsParticle.h"
#include "OgreHlmsManager.h"
#include "OgreArchiveManager.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

#include <GpuParticles/GpuParticleSystemJsonManager.h>
#include <GpuParticles/GpuParticleSystemResourceManager.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    #include "OSX/macUtils.h"
    #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        #include "System/iOS/iOSUtils.h"
    #else
        #include "System/OSX/OSXUtils.h"
    #endif
#endif

namespace Demo
{
    class Sample_GpuParticlesGraphicsSystem : public GraphicsSystem
    {
        Ogre::CompositorWorkspace* setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "Sample_GpuParticlesWorkspace", true );
        }

        void setupResources() override
        {
            GpuParticleSystemResourceManager* gpuParticleSystemResourceManager = new GpuParticleSystemResourceManager();
            gpuParticleSystemResourceManager->registerCommonAffectors();
            GpuParticleSystemJsonManager* gpuParticleSystemJsonManager = new GpuParticleSystemJsonManager();

            GraphicsSystem::setupResources();
        }

        void registerHlms() override
        {
            GraphicsSystem::registerHlms();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            Ogre::String rootHlmsFolder = Ogre::macBundlePath() + '/' +
                                          cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
            Ogre::String rootHlmsFolder = mResourcePath +
                                          cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif
            if( rootHlmsFolder.empty() )
                rootHlmsFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *(rootHlmsFolder.end() - 1) != '/' )
                rootHlmsFolder += "/";

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            Ogre::String rootHlmsFolderParticle = Ogre::macBundlePath() + '/' +
                                          cf.getSetting( "DoNotUseAsResourceParticle", "Hlms", "" );
#else
            Ogre::String rootHlmsFolderParticle = mResourcePath +
                                          cf.getSetting( "DoNotUseAsResourceParticle", "Hlms", "" );
#endif
            if( rootHlmsFolderParticle.empty() )
                rootHlmsFolderParticle = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *(rootHlmsFolderParticle.end() - 1) != '/' )
                rootHlmsFolderParticle += "/";

            HlmsParticle* hlmsParticle = HlmsParticle::registerHlms(rootHlmsFolder, rootHlmsFolderParticle);
        }

    public:
        Sample_GpuParticlesGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState, "../Data/" )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        Sample_GpuParticlesGameState *gfxGameState = new Sample_GpuParticlesGameState(
        "--- !!! NOTE: THIS SAMPLE IS A WORK IN PROGRESS !!! ---\n"
        "This is sample application using GPU particles:\n"
        "   * Own Hlms implementation to render the terrain\n"
        "   * Vertex buffer-less rendering: The particles are generated purely using SV_VertexID\n"
        "   * Hlms customizations to Unlit\n"
        "   * Compute Shaders to create and update particles every frame\n"
        "   * Custom compositor is needed if depth buffer collisions are used for particles\n"
        "This sample depends on the media files:\n"
        "   * media/Compositor/*.*\n"
        "   * media/Materials/scripts/*.*\n"
        "   * media/Materials/textures/*.*\n"
        "   * media/Hlms/Compute/*.*\n"
        "   * media/Hlms/Particle/*.*\n" );

        Sample_GpuParticlesGraphicsSystem *graphicsSystem =
                new Sample_GpuParticlesGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState,
                                          GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState,
                                          LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char* MainEntryPoints::getWindowTitle(void)
    {
        return "Tutorial: GPUParticle";
    }
}

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}
#endif
