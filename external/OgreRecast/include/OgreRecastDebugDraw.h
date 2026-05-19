#include "debugUtils/DebugDraw.h"
#include <OgreSceneNode.h>
#include <OgreManualObject2.h>

struct oduVertex
{
	Ogre::Vector3 pos;
	Ogre::ColourValue color;
	Ogre::Vector2 uv;
};

class __declspec(dllexport) OgreRecastDebugDraw :public duDebugDraw
{
private:
	Ogre::SceneManager* m_pScnMgr;
	Ogre::SceneNode* m_pNode;
	Ogre::ManualObject* m_pManualObject;
	duDebugDrawPrimitives m_crtShape;

	std::vector<oduVertex> m_lstCrtVertex;
	int m_sectionCount;

	// Akkumulierte Geometrie pro OpType, gefüllt von end(), geleert von flushToGPU()
	struct AccumBatch
	{
		std::vector<oduVertex>   verts;
		Ogre::OperationType      opType = Ogre::OT_TRIANGLE_LIST;
		std::string              material;
		// Für QUADS: wir wandeln in Tris um, daher gleich OT_TRIANGLE_LIST
	};
	// Keyed by opType (int), 4 mögliche Batches
	std::unordered_map<int, AccumBatch> m_accumBatches;

	Ogre::OperationType m_crtOpType = Ogre::OT_TRIANGLE_LIST;
	std::string         m_crtMaterial;
public:
	OgreRecastDebugDraw(Ogre::SceneManager* scnMgr);

	void draw(bool enable);

	void mustRecreate(void);

	void resetForNewFrame(void);

	void flushToGPU();

	virtual void depthMask(bool state);
	virtual void texture(bool state);
	virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f);
	virtual void vertex(const float* pos, unsigned int color);
	virtual void vertex(const float x, const float y, const float z, unsigned int color);
	virtual void vertex(const float* pos, unsigned int color, const float* uv);
	virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
	virtual void end();
};
