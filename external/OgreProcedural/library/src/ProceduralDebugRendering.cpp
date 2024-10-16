/*
-----------------------------------------------------------------------------
This source file is part of ogre-procedural

For the latest info, see http://www.ogreprocedural.org

Copyright (c) 2010-2013 Michael Broutin

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
#include "ProceduralStableHeaders.h"
#include "ProceduralDebugRendering.h"
#include "OgreRoot.h"

using namespace Ogre;

namespace Procedural
{

Ogre::v1::ManualObject* ShowNormalsGenerator::buildManualObject() const
{
	if (mTriangleBuffer == NULL)
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "The input triangle buffer must not be null", "Procedural::ShowNormalsGenerator::buildManualObject()");
	SceneManager* sceneMgr = Ogre::Root::getSingleton().getSceneManagerIterator().begin()->second;
	if (sceneMgr == NULL)
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Scene Manager must be set in Root", "Procedural::ShowNormalsGenerator::buildManualObject()");
	// ManualObject* manual = sceneMgr->createManualObject();
	// manual->begin("BaseWhiteNoLighting", RenderOperation::OT_LINE_LIST);
	Ogre::v1::ManualObject* manual = new Ogre::v1::ManualObject(0, &sceneMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), sceneMgr);
	manual->setRenderQueueGroup(110);
	manual->begin("BaseWhite", Ogre::OperationType::OT_LINE_LIST);
	int index = 0;

	const std::vector<TriangleBuffer::Vertex>& vertices = mTriangleBuffer->getVertices();
	for (std::vector<TriangleBuffer::Vertex>::const_iterator it = vertices.begin(); it!= vertices.end(); ++it)
	{
		manual->position(it->mPosition);
		manual->position(it->mPosition + it->mNormal * mSize);

		if (mVisualStyle == VS_ARROW)
		{
			Vector3 axis2 = it->mNormal.perpendicular();
			Vector3 axis3 = it->mNormal.crossProduct(axis2);

			manual->position(it->mPosition + it->mNormal * mSize);
			manual->index(index++);
			manual->position(it->mPosition + (.8f * it->mNormal  + .1f * axis2) * mSize);
			manual->index(index++);

			manual->position(it->mPosition + it->mNormal * mSize);
			manual->index(index++);
			manual->position(it->mPosition + .8f * (it->mNormal  - .1f * axis2) * mSize);
			manual->index(index++);

			manual->position(it->mPosition + it->mNormal * mSize);
			manual->index(index++);
			manual->position(it->mPosition + .8f * ( it->mNormal + .1f * axis3)* mSize);
			manual->index(index++);

			manual->position(it->mPosition + it->mNormal * mSize);
			manual->index(index++);
			manual->position(it->mPosition + .8f * (it->mNormal - .1f * axis3)* mSize);
			manual->index(index++);
		}
	}
	manual->end();

	return manual;
}

v1::MeshPtr ShowNormalsGenerator::buildMesh(const std::string& name, const String& group) const
{
	SceneManager* sceneMgr = Ogre::Root::getSingleton().getSceneManagerIterator().begin()->second;
	v1::ManualObject* mo = buildManualObject();
	Ogre::v1::MeshPtr mesh = mo->convertToMesh(name, group);

	// sceneMgr->destroyManualObject(mo);
	delete mo;

	return mesh;
}
}
