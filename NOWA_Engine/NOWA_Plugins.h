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

NOWAPrecompiled.h -- Minimal stable precompiled header for NOWA plugin components.

RULES:
  1. Only external, third-party headers go here. They almost never change.
  2. NO engine component headers (PhysicsActiveComponent.h etc.) ever go here.
  3. Plugin .h files  -- include only GameObjectComponent.h + OgrePlugin.h.
  4. Plugin .cpp files -- include this PCH, then ONLY the engine headers they actually use.

*/

#ifndef NOWA_PLUGINS_PRECOMPILED_H
#define NOWA_PLUGINS_PRECOMPILED_H

// ── Windows (MUST come first on Windows builds) ──────────────────────────────
// Without this, HANDLE, DWORD, BOOL, GUID and hundreds of other Windows types
// are undefined, which cascades into broken Boost, COM, luabind, and Ogre headers.
#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN   // exclude rarely-used Windows headers
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX              // prevent Windows min/max macro pollution
#   endif
#   include <windows.h>
#endif
 

 
// ── Lua (external, stable) ───────────────────────────────────────────────────
extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <luabind/luabind.hpp>

// ── Standard library (before Boost — Boost depends on these) ─────────────────
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <array>
#include <functional>
#include <algorithm>
#include <memory>
#include <sstream>
#include <cassert>
#include <tuple>
#include <thread>
#include <mutex>

// ── Boost (external, stable) ─────────────────────────────────────────────────
// NOTE: boost/static_pointer_cast.hpp and boost/dynamic_pointer_cast.hpp do NOT
// exist as standalone files — these casts are part of boost/shared_ptr.hpp.
#include <boost/shared_ptr.hpp>      // includes static_pointer_cast / dynamic_pointer_cast
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
 
// ── OIS input (external, stable) ─────────────────────────────────────────────
#include "OISEvents.h"
#include "OISInputManager.h"
#include "OISKeyboard.h"
#include "OISMouse.h"
#include "OISJoyStick.h"
 
// ── Ogre-Next (external, stable public API) ──────────────────────────────────
#include "Ogre.h"
#include "OgrePlugin.h"
#include "OgreAbiUtils.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlitDatablock.h"
#include "Animation/OgreBone.h"
#include "Animation/OgreSkeletonInstance.h"
#include "Animation/OgreSkeletonAnimation.h"
#include "OgreMath.h"
#include "OgreMeshManager2.h"
 
 
// ── Newton Dynamics (external, stable) ───────────────────────────────────────
#include "OgreNewt.h"
 
// ── OgreAL sound (external, stable) ──────────────────────────────────────────
#include "OgreAL.h"
 
// ── rapidxml (external, stable) ──────────────────────────────────────────────
#include "utilities/rapidxml.hpp"
 
// ── NOWA minimal stable base ─────────────────────────────────────────────────
// These three files are the root of the component system.
// They change very rarely; a full rebuild on change is acceptable.
#include "gameobject/GameObjectComponent.h"   // base class for all components
#include "utilities/Variant.h"                // used in every component
#include "utilities/XMLConverter.h"           // used in every init() / writeXML()

#endif // NOWA_PLUGINS_PRECOMPILED_H