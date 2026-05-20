/*
    OgreCrowd
    ---------

    Copyright (c) 2012 Jonas Hauquier

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

#pragma once

#include <OgreSceneNode.h>
#include <OgreManualObject2.h>

// ---------------------------------------------------------------------------
// OgreRecastDebugDraw
//
// Minimal debug draw helper — used ONLY for the path line visualisation.
// Navmesh polygon/edge geometry is handled by OgreRecastNavmeshVisual
// (static Ogre::Mesh via VaoManager, not ManualObject).
//
// The path line is a simple OT_LINE_STRIP ManualObject with a handful of
// vertices created once per findPath result — ManualObject overhead is
// negligible here.
// ---------------------------------------------------------------------------
class __declspec(dllexport) OgreRecastDebugDraw
{
public:
    explicit OgreRecastDebugDraw(Ogre::SceneManager* scnMgr);
    ~OgreRecastDebugDraw();

    // Show or hide the path line node.
    void draw(bool enable);

    // Destroy the ManualObject and SceneNode, resetting this object to its
    // initial state. Call before recreating the path line.
    void mustRecreate();

    // Access the ManualObject for direct path line construction.
    Ogre::ManualObject* getManualObject() const
    {
        return m_pManualObject;
    }
    Ogre::SceneNode* getNode()         const
    {
        return m_pNode;
    }

private:
    Ogre::SceneManager* m_pScnMgr;
    Ogre::SceneNode* m_pNode;
    Ogre::ManualObject* m_pManualObject;

    OgreRecastDebugDraw(const OgreRecastDebugDraw&) = delete;
    OgreRecastDebugDraw& operator=(const OgreRecastDebugDraw&) = delete;
};