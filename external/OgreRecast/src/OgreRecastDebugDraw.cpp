#include "OgreRecastDebugDraw.h"
#include <OgreSceneManager.h>

OgreRecastDebugDraw::OgreRecastDebugDraw(Ogre::SceneManager* scnMgr)
	: m_pScnMgr(scnMgr),
	m_pNode(nullptr),
	m_pManualObject(nullptr),
	m_sectionCount(0)
{
	
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

void OgreRecastDebugDraw::mustRecreate(void)
{
	m_sectionCount = 0;

	if (nullptr != m_pNode)
	{
		m_pNode->detachObject(m_pManualObject);
		m_pScnMgr->destroyManualObject(m_pManualObject);
		m_pManualObject = nullptr;
		m_pScnMgr->getRootSceneNode()->removeAndDestroyChild(m_pNode);
		m_pNode = nullptr;
	}
}

void OgreRecastDebugDraw::resetForNewFrame(void)
{
	m_sectionCount = 0;
}

void OgreRecastDebugDraw::depthMask(bool state)
{
}

void OgreRecastDebugDraw::texture(bool state)
{
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

void OgreRecastDebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
    m_crtShape = prim;
    m_lstCrtVertex.clear();

    // Beim ersten begin() einer neuen Draw-Sequenz: ManualObject und Akkumulation leeren
    if (0 == m_sectionCount)
    {
        m_accumBatches.clear();
        if (nullptr != m_pManualObject)
            m_pManualObject->clear();
    }

    // Material und OpType für diesen Batch merken
    switch (prim)
    {
    case DU_DRAW_POINTS: m_crtOpType = Ogre::OT_POINT_LIST;    m_crtMaterial = "Rocks"; break;
    case DU_DRAW_LINES:  m_crtOpType = Ogre::OT_LINE_LIST;     m_crtMaterial = "Rocks"; break;
    case DU_DRAW_TRIS:   m_crtOpType = Ogre::OT_TRIANGLE_LIST; m_crtMaterial = "Rocks"; break;
    case DU_DRAW_QUADS:  m_crtOpType = Ogre::OT_TRIANGLE_LIST; m_crtMaterial = "Rocks"; break; // QUADS → TRIS
    default:             m_crtOpType = Ogre::OT_TRIANGLE_LIST; m_crtMaterial = "Rocks"; break;
    }
}

void OgreRecastDebugDraw::end()
{
    if (m_lstCrtVertex.empty())
        return;

    // Nicht sofort an ManualObject übergeben — stattdessen CPU-seitig akkumulieren
    AccumBatch& batch = m_accumBatches[static_cast<int>(m_crtOpType)];
    batch.opType = m_crtOpType;
    batch.material = m_crtMaterial;

    switch (m_crtShape)
    {
    case DU_DRAW_POINTS:
    case DU_DRAW_LINES:
    case DU_DRAW_TRIS:
        // Direkt anhängen
        batch.verts.insert(batch.verts.end(),
            m_lstCrtVertex.begin(), m_lstCrtVertex.end());
        break;

    case DU_DRAW_QUADS:
        // Quad (0,1,2,3) → zwei Dreiecke (0,1,2) + (0,2,3)
        if (m_lstCrtVertex.size() % 4 == 0)
        {
            for (size_t i = 0; i < m_lstCrtVertex.size(); i += 4)
            {
                batch.verts.push_back(m_lstCrtVertex[i + 0]);
                batch.verts.push_back(m_lstCrtVertex[i + 1]);
                batch.verts.push_back(m_lstCrtVertex[i + 2]);
                batch.verts.push_back(m_lstCrtVertex[i + 0]);
                batch.verts.push_back(m_lstCrtVertex[i + 2]);
                batch.verts.push_back(m_lstCrtVertex[i + 3]);
            }
        }
        break;

    default:
        break;
    }

    ++m_sectionCount;
    // KEIN m_pManualObject->begin()/end() hier — das passiert in flushToGPU()
}

void OgreRecastDebugDraw::flushToGPU()
{
    // Wird einmalig aufgerufen, nachdem alle duDebugDraw*-Callbacks durch sind.
    // Erzeugt pro Primitive-Typ EINE ManualObject-Section → minimal GPU-Allocations.
    if (nullptr == m_pManualObject || m_accumBatches.empty())
        return;

    for (auto& [opTypeInt, batch] : m_accumBatches)
    {
        if (batch.verts.empty())
            continue;

        const size_t n = batch.verts.size();

        m_pManualObject->begin(batch.material, batch.opType);

        if (batch.opType == Ogre::OT_TRIANGLE_LIST)
        {
            // Dreiecke: 3 Verts pro Tri, Indices sequenziell
            if (n % 3 == 0)
            {
                for (size_t i = 0; i < n; ++i)
                {
                    m_pManualObject->position(batch.verts[i].pos);
                    m_pManualObject->colour(batch.verts[i].color);
                }
                for (uint32_t i = 0; i < static_cast<uint32_t>(n); i += 3)
                    m_pManualObject->triangle(i, i + 1, i + 2);
            }
        }
        else if (batch.opType == Ogre::OT_LINE_LIST)
        {
            if (n % 2 == 0)
            {
                for (size_t i = 0; i < n; ++i)
                {
                    m_pManualObject->position(batch.verts[i].pos);
                    m_pManualObject->colour(batch.verts[i].color);
                }
                for (uint32_t i = 0; i < static_cast<uint32_t>(n); i += 2)
                {
                    m_pManualObject->index(i);
                    m_pManualObject->index(i + 1);
                }
            }
        }
        else // OT_POINT_LIST
        {
            for (size_t i = 0; i < n; ++i)
            {
                m_pManualObject->position(batch.verts[i].pos);
                m_pManualObject->colour(batch.verts[i].color);
                m_pManualObject->index(static_cast<uint32_t>(i));
            }
        }

        m_pManualObject->end();
    }

    m_accumBatches.clear();
}
