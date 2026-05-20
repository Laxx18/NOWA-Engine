/*
    OgreCrowd — OgreRecastDebugDraw.cpp
    Minimal ManualObject-based path line helper.
    Navmesh geometry is drawn by OgreRecastNavmeshVisual.
*/

#include "OgreRecastDebugDraw.h"
#include <OgreSceneManager.h>

OgreRecastDebugDraw::OgreRecastDebugDraw(Ogre::SceneManager* scnMgr)
    : m_pScnMgr(scnMgr),
    m_pNode(nullptr),
    m_pManualObject(nullptr)
{}

OgreRecastDebugDraw::~OgreRecastDebugDraw()
{
    mustRecreate();
}

void OgreRecastDebugDraw::draw(bool enable)
{
    if (false == enable)
    {
        if (nullptr != m_pNode)
            m_pNode->setVisible(false);
        return;
    }

    if (nullptr == m_pNode)
    {
        m_pNode = m_pScnMgr->getRootSceneNode()->createChildSceneNode();
        m_pManualObject = m_pScnMgr->createManualObject();
        m_pManualObject->setRenderQueueGroup(10);
        m_pNode->attachObject(m_pManualObject);
    }
    m_pNode->setVisible(true);
}

void OgreRecastDebugDraw::mustRecreate()
{
    if (nullptr != m_pNode)
    {
        if (nullptr != m_pManualObject)
        {
            m_pNode->detachObject(m_pManualObject);
            m_pScnMgr->destroyManualObject(m_pManualObject);
            m_pManualObject = nullptr;
        }
        m_pScnMgr->getRootSceneNode()->removeAndDestroyChild(m_pNode);
        m_pNode = nullptr;
    }
}