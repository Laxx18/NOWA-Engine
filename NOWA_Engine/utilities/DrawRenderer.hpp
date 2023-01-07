#ifndef DRAW_RENDERER_H
#define DRAW_RENDERER_H

#if 0

#include "defines.h"
#include <vector>
#include <list>
#include <memory>
#include "OgreMovableObject.h"

// #include <XERenderer/GUI/RenderableShape.hpp>

class ObjectMemoryManager;
class SceneManager;
class Vector3;
class ColourValue;
class ManualObject;


namespace NOWA
{
	typedef std::pair<Ogre::Vector3, Ogre::ColourValue> VertexPair;

	struct Vertex
	{
		Vertex()
		{
		}
		Vertex(const Ogre::Vector3& pos, const Ogre::ColourValue& col)
			: position(pos)
			, colour(col)
		{

		}

		Ogre::Vector3 position;
		Ogre::ColourValue colour;
		Ogre::Vector2 uv;
	};

	class DrawRenderable;

	/*
	Wrapper class for render dynamic lines
	*/
	class EXPORTED DrawRenderer : public Ogre::MovableObject
	{
	public:
		DrawRenderer(Ogre::uint32 id, Ogre::SceneManager* sceneManager, Ogre::ObjectMemoryManager* objManager, float fillAlpha);
		virtual ~DrawRenderer();
		
		void updateVertices(std::queue<int>& indices, std::queue<Vertex>& vertices);
		
		// MovableObject overrides
		/** @copydoc MovableObject::getMovableType. */
		const Ogre::String& getMovableType(void) const;

		/// Remove all points from the point list
		void clear();

	private:

		std::vector<std::unique_ptr<DrawRenderable>> renderables;

		float fillAlpha;

		Ogre::ObjectMemoryManager* objectMemoryManager;
		Ogre::SceneManager* sceneManager;
		
		Ogre::SceneNode* sceneNodeLines;

		bool initalized;
	};

	class EXPORTED DrawRenderable : public Ogre::Renderable
	{
	public:

		DrawRenderable(Ogre::SceneManager* sceneManager);

		~DrawRenderable();
			
		void updateVertices(std::queue<int>& indices, std::queue<Vertex>& vertices);
		
		/// Remove all points from the point list
		void clear();

		// Renderable overrides
		/** @copydoc Renderable::getRenderOperation. */
		virtual void getRenderOperation(Ogre::v1::RenderOperation& op, bool casterPass) override;

		/** @copydoc Renderable::getWorldTransforms. */
		virtual void getWorldTransforms(Ogre::Matrix4* xform) const override {}
		/** @copydoc Renderable::getLights. */
		virtual const Ogre::LightList& getLights(void) const override { return this->lightList; }
		/** @copydoc Renderable::getCastsShadows. */
		virtual bool getCastsShadows(void) const { return false; }

	protected:
		//called from Ogre::Renderable
		Ogre::OperationType operationType;

	private:
		/// List of lights for this object
		Ogre::LightList lightList;
		Ogre::SceneManager* sceneManager;
		Ogre::uint32 currentVertexBufferSize;
		Ogre::uint32 currentIndexBufferSize;

		Ogre::VertexArrayObject* vao;
		Ogre::VaoManager* vaoManager;
		Ogre::VertexElement2Vec vertexElements;

		Ogre::VertexBufferPacked* vertexBuffer;
		Ogre::IndexBufferPacked* indexBuffer;
	};

} // namespace end

#endif

#endif