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
public:
	OgreRecastDebugDraw(Ogre::SceneManager* scnMgr);

	void draw(bool enable);

	void mustRecreate(void);

	virtual void depthMask(bool state);
	virtual void texture(bool state);
	virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f);
	virtual void vertex(const float* pos, unsigned int color);
	virtual void vertex(const float x, const float y, const float z, unsigned int color);
	virtual void vertex(const float* pos, unsigned int color, const float* uv);
	virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
	virtual void end();
};
