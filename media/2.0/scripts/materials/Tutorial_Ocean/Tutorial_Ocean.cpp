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

#include "GraphicsSystem.h"
#include "Tutorial_OceanGameState.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreWindow.h"

#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreArchiveManager.h"
#include "OgreHlmsManager.h"
#include "Ocean/Hlms/OgreHlmsOcean.h"

#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#    include "OSX/macUtils.h"
#    if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#        include "System/iOS/iOSUtils.h"
#    else
#        include "System/OSX/OSXUtils.h"
#    endif
#endif

namespace Demo
{
    class Tutorial_OceanGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            using namespace Ogre;
            CompositorManager2 *compositorManager = mRoot->getCompositorManager2();

            // Start with normal workspace (above water)
            CompositorWorkspace *workspace = compositorManager->addWorkspace(
                mSceneManager, mRenderWindow->getTexture(), mCamera, 
                "Tutorial_OceanWorkspace", true );

            return workspace;
        }

        void setupResources() override
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

            Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( originalDataFolder.empty() )
                originalDataFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( originalDataFolder.end() - 1 ) != '/' )
                originalDataFolder += "/";

            const char *c_locations[5] = { 
                "2.0/scripts/materials/Tutorial_Ocean",
                "2.0/scripts/materials/Tutorial_Ocean/GLSL",
                "2.0/scripts/materials/Tutorial_Ocean/HLSL",
                "2.0/scripts/materials/Tutorial_Ocean/Metal",
                "2.0/scripts/materials/Postprocessing/SceneAssets"
            };

            for( size_t i = 0; i < 5; ++i )
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
            }
        }

        void registerHlms() override
        {
            GraphicsSystem::registerHlms();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            Ogre::String rootHlmsFolder =
                Ogre::macBundlePath() + '/' + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
            Ogre::String rootHlmsFolder =
                mResourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif
            if( rootHlmsFolder.empty() )
                rootHlmsFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( rootHlmsFolder.end() - 1 ) != '/' )
                rootHlmsFolder += "/";

            Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();

            Ogre::String shaderSyntax = "GLSL";
            if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
                shaderSyntax = "HLSL";
            else if( renderSystem->getName() == "Metal Rendering Subsystem" )
                shaderSyntax = "Metal";

            Ogre::String mainFolderPath;
            Ogre::StringVector libraryFoldersPaths;
            Ogre::StringVector::const_iterator libraryFolderPathIt;
            Ogre::StringVector::const_iterator libraryFolderPathEn;

            Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
            Ogre::HlmsManager *hlmsManager = mRoot->getHlmsManager();

            // Create & Register HlmsOcean
            {
                Ogre::HlmsOcean::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
                Ogre::Archive *archiveOcean = archiveManager.load( 
                    rootHlmsFolder + mainFolderPath,
                    getMediaReadArchiveType(), true );
                
                Ogre::ArchiveVec archiveOceanLibraryFolders;
                libraryFolderPathIt = libraryFoldersPaths.begin();
                libraryFolderPathEn = libraryFoldersPaths.end();
                while( libraryFolderPathIt != libraryFolderPathEn )
                {
                    Ogre::Archive *archiveLibrary = archiveManager.load(
                        rootHlmsFolder + *libraryFolderPathIt, 
                        getMediaReadArchiveType(), true );
                    archiveOceanLibraryFolders.push_back( archiveLibrary );
                    ++libraryFolderPathIt;
                }

                // Create and register the Ocean Hlms
                Ogre::HlmsOcean *hlmsOcean = OGRE_NEW Ogre::HlmsOcean( 
                    archiveOcean, &archiveOceanLibraryFolders );
                hlmsManager->registerHlms( hlmsOcean );
            }
        }

    public:
        Tutorial_OceanGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState, 
                                         LogicSystem **outLogicSystem )
    {
        Tutorial_OceanGameState *gfxGameState = new Tutorial_OceanGameState(
            "This tutorial demonstrates the Ocean rendering system for OGRE-Next.\n"
            "The Ocean system features:\n"
            "   * Custom Hlms implementation for realistic ocean rendering\n"
            "   * GPU-based wave simulation with configurable parameters\n"
            "   * Vertex buffer-less rendering using heightmap and wave functions\n"
            "   * Dynamic underwater post-processing effects\n"
            "   * Automatic workspace switching when camera crosses water surface\n"
            "   * Integration with AtmosphereNpr for realistic sky and lighting\n"
            "   * Configurable ocean colors, wave intensity, scale, and chaos\n\n"
            "Like Terra, the Ocean system is designed as a component that can be easily\n"
            "integrated into your own projects. The implementation is located in the Ocean\n"
            "folder for easy copy-pasting and customization.\n\n"
            "This sample depends on the media files:\n"
            "   * Samples/Media/2.0/scripts/Compositors/Tutorial_Ocean.compositor\n"
            "   * Samples/Media/2.0/materials/Tutorial_Ocean/*.*\n"
            "   * Samples/Media/2.0/materials/Postprocessing/UnderwaterPost.*\n"
            "   * Samples/Media/Hlms/Ocean/*.*\n\n"
            "Controls:\n"
            "   * Move camera above/below water surface to see underwater effect\n"
            "   * +/- : Adjust time of day\n"
            "   * 9/6 : Adjust sun azimuth\n"
            "   * Ctrl+F4 : Hot reload ocean shaders\n" );

        Tutorial_OceanGraphicsSystem *graphicsSystem =
            new Tutorial_OceanGraphicsSystem( gfxGameState );

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

    const char *MainEntryPoints::getWindowTitle() 
    { 
        return "Tutorial: Ocean Rendering"; 
    }
}  // namespace Demo

#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, 
                       LPSTR strCmdLine, INT nCmdShow )
#    else
int mainApp( int argc, const char *argv[] )
#    endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}
#endif