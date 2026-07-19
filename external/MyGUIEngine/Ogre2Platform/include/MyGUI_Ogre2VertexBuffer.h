/*!
	@file
	@author		Albert Semenov
	@date		04/2009
	@update		2026 v2 VAO rewrite for Ogre-Next by NOWA-Engine

	v2 design:
	 - One VertexBufferPacked (BT_DEFAULT) + one VertexArrayObject per MyGUI buffer.
	 - MyGUI only rewrites vertices when a widget is dirty (NOT every frame),
	   therefore BT_DEFAULT + upload() on unlock() is used instead of
	   BT_DYNAMIC_PERSISTENT (which would require a full rewrite every frame).
	 - lock() returns a CPU scratch buffer; unlock() uploads it via staging.
	 - setOperationVertexCount() maps to vao->setPrimitiveRange() — the VAO is
	   never rebuilt for count changes, only for capacity growth.
	 - The Ogre2GuiRenderable lives INSIDE this class (1 buffer = 1 VAO =
	   1 renderable). This removes the RenderManager's unbounded ID->renderable
	   map and ties renderable lifetime to buffer lifetime.
*/

#ifndef MYGUI_OGRE2_VERTEX_BUFFER_H_
#define MYGUI_OGRE2_VERTEX_BUFFER_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_IVertexBuffer.h"
#include "MyGUI_Ogre2GuiRenderable.h"

#include <OgrePrerequisites.h>

#include <vector>

#include "MyGUI_LastHeader.h"

namespace Ogre
{
	class VaoManager;
	class VertexBufferPacked;
	struct VertexArrayObject;
}

namespace MyGUI
{

	class __declspec(dllexport) Ogre2VertexBuffer :
		public IVertexBuffer
	{
	public:
		Ogre2VertexBuffer();
		virtual ~Ogre2VertexBuffer();

		virtual void setVertexCount(size_t _count);
		virtual size_t getVertexCount();

		// Sets the number of vertices actually drawn this batch.
		// v2: maps to VertexArrayObject::setPrimitiveRange(0, _count).
		void setOperationVertexCount(size_t _count);

		virtual Vertex* lock();
		virtual void unlock();

		// The renderable that draws this buffer. Its mVaoPerLod is kept in
		// sync with the (possibly re-created) VAO by this class.
		Ogre2GuiRenderable* getRenderable()
		{
			return &mRenderable;
		}

		Ogre::VertexArrayObject* getVao() const
		{
			return mVao;
		}

	private:
		void createVertexBuffer();
		void destroyVertexBuffer();
		void resizeVertexBuffer();

	private:
		size_t mVertexCount;		// allocated capacity (in vertices)
		size_t mNeedVertexCount;	// requested size from MyGUI

		Ogre::VaoManager* mVaoManager;
		Ogre::VertexBufferPacked* mVertexBuffer;
		Ogre::VertexArrayObject* mVao;

		// CPU scratch buffer handed out by lock(), uploaded on unlock().
		std::vector<Vertex> mCpuBuffer;

		Ogre2GuiRenderable mRenderable;
	};

} // namespace MyGUI

#endif // MYGUI_OGRE2_VERTEX_BUFFER_H_
