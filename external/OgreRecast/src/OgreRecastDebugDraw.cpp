#include "OgreRecastDebugDraw.h"
#include <OgreSceneManager.h>

OgreRecastDebugDraw::OgreRecastDebugDraw(Ogre::SceneManager* scnMgr)
	: m_pScnMgr(scnMgr),
	m_pNode(nullptr),
	m_pManualObject(nullptr)
{
	
}

void OgreRecastDebugDraw::draw(bool enable)
{
	if (nullptr == m_pNode)
	{
		m_pNode = m_pScnMgr->getRootSceneNode()->createChildSceneNode();
		m_pManualObject = m_pScnMgr->createManualObject();
		m_pManualObject->setRenderQueueGroup(10);
		m_pNode->attachObject(m_pManualObject);
	}
	m_pNode->setVisible(enable);
}

void OgreRecastDebugDraw::mustRecreate(void)
{
	if (nullptr != m_pNode)
	{
		m_pNode->detachObject(m_pManualObject);
		m_pScnMgr->destroyManualObject(m_pManualObject);
		m_pManualObject = nullptr;
		m_pScnMgr->getRootSceneNode()->removeAndDestroyChild(m_pNode);
		m_pNode = nullptr;
	}
}

void OgreRecastDebugDraw::depthMask(bool state)
{
}

void OgreRecastDebugDraw::texture(bool state)
{
}

void OgreRecastDebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
	m_crtShape = prim;

	m_lstCrtVertex.clear();
}

void OgreRecastDebugDraw::vertex(const float * pos, unsigned int color)
{
	oduVertex crtVertex;
	crtVertex.pos.x = pos[0];
	crtVertex.pos.y = pos[1];
	crtVertex.pos.z = pos[2];
	m_lstCrtVertex.push_back(crtVertex);
}

void OgreRecastDebugDraw::vertex(const float x, const float y, const float z, unsigned int color)
{
	oduVertex crtVertex;
	crtVertex.pos.x = x;
	crtVertex.pos.y = y;
	crtVertex.pos.z = z;
	m_lstCrtVertex.push_back(crtVertex);
}

void OgreRecastDebugDraw::vertex(const float * pos, unsigned int color, const float * uv)
{
	vertex(pos, color);
}

void OgreRecastDebugDraw::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	vertex(x, y, z, color);
}

void OgreRecastDebugDraw::end()
{
	int idx = 0;
	int connectIdx = 0;
	bool bSuccessSet = false;
	switch (m_crtShape)
	{
	case DU_DRAW_POINTS:
		m_pManualObject->begin("Rocks", Ogre::OT_POINT_LIST);
		for (int i = 0, n = m_lstCrtVertex.size(); i < n; ++i)
		{
			idx = i;
			m_pManualObject->position(m_lstCrtVertex[idx].pos);
			m_pManualObject->colour(m_lstCrtVertex[idx].color);

			m_pManualObject->index(idx);
			bSuccessSet = true;
		}
		break;
	case DU_DRAW_LINES:
		m_pManualObject->begin("Rocks", Ogre::OT_LINE_LIST);
		if (m_lstCrtVertex.size() % 2 == 0)
		{
			for (int i = 0, n = m_lstCrtVertex.size(); i < n; i += 2)
			{
				idx = i;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);
				++idx;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);

				connectIdx = i;
				m_pManualObject->index(connectIdx++);
				m_pManualObject->index(connectIdx++);
				bSuccessSet = true;
			}
		}
		break;
	case DU_DRAW_TRIS:
		m_pManualObject->begin("Rocks", Ogre::OT_TRIANGLE_LIST);
		if (m_lstCrtVertex.size() % 3 == 0)
		{
			for (int i = 0, n = m_lstCrtVertex.size(); i < n; i += 3)
			{
				idx = i;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);
				++idx;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);
				++idx;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);

				connectIdx = i;
				m_pManualObject->triangle(connectIdx++, connectIdx++, connectIdx++);
				bSuccessSet = true;
			}

		}
		break;
	case DU_DRAW_QUADS:
		m_pManualObject->begin("Rocks", Ogre::OT_TRIANGLE_LIST);
		if (m_lstCrtVertex.size() % 4 == 0)
		{
			for (int i = 0, n = m_lstCrtVertex.size(); i < n; i += 4)
			{
				idx = i;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);
				++idx;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);
				++idx;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);
				++idx;
				m_pManualObject->position(m_lstCrtVertex[idx].pos);
				m_pManualObject->colour(m_lstCrtVertex[idx].color);

				connectIdx = i;
				m_pManualObject->quad(connectIdx++, connectIdx++, connectIdx++, connectIdx++);
				bSuccessSet = true;
			}
		}
		break;
	default:
		break;
	}

	if (!bSuccessSet)
	{
		assert(!"Wrong debug shape");
	}
	m_pManualObject->end();
}
