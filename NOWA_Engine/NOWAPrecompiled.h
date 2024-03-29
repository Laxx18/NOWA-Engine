/*
OGRE (www.ogre3d.org) is made available under the MIT License.

Copyright (c) 2000-2015 Torus Knot Software Ltd

NOWA (http://www.lukas-kalinowski.com/Homepage/?page_id=1631) is made available under the MIT License.

Copyright (c) 2017 Lukas Kalinowski

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
*/

// https://stackoverflow.com/questions/48882439/how-to-restore-auto-ptr-in-visual-studio-c17
// std::auto_ptr has been deleted, but luabind needs it, hence enable it again
#define _HAS_AUTO_PTR_ETC 1

#include "Ogre.h"
#include "OgreWindow.h"
#include "OgreDepthBuffer.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/Pass/PassClear/OgreCompositorPassClearDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "OgrePlatformInformation.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreTextureGpuManager.h"
#include "OgreTextureFilters.h"
#include "OgreStagingTexture.h"
#include "OgreArchiveManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OIS.h"
#include "OgreNewt.h"
#include "Terra.h"
#include "OgreAtmosphereComponent.h"
#include <boost/make_shared.hpp>

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include <luabind/luabind.hpp>