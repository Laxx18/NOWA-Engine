#include "NOWAPrecompiled.h"

#if 0

#include "GeometryDrawer.hpp"

namespace NOWA
{

	IcoSphere::IcoSphere()
		: index(0)
	{
	}

	IcoSphere::~IcoSphere()
	{
	}

	void IcoSphere::create(int recursionLevel)
	{
		vertices.clear();
		lineIndices.clear();
		triangleIndices.clear();
		faces.clear();
		middlePointIndexCache.clear();
		index = 0;

		Ogre::Real t = (1.0f + Ogre::Math::Sqrt(5.0f)) / 2.0f;

		addVertex(Ogre::Vector3(-1.0f, t, 0.0f));
		addVertex(Ogre::Vector3(1.0f, t, 0.0f));
		addVertex(Ogre::Vector3(-1.0f, -t, 0.0f));
		addVertex(Ogre::Vector3(1.0f, -t, 0.0f));

		addVertex(Ogre::Vector3(0.0f, -1.0f, t));
		addVertex(Ogre::Vector3(0.0f, 1.0f, t));
		addVertex(Ogre::Vector3(0.0f, -1.0f, -t));
		addVertex(Ogre::Vector3(0.0f, 1.0f, -t));

		addVertex(Ogre::Vector3(t, 0.0f, -1.0f));
		addVertex(Ogre::Vector3(t, 0.0f, 1.0f));
		addVertex(Ogre::Vector3(-t, 0.0f, -1.0f));
		addVertex(Ogre::Vector3(-t, 0.0f, 1.0f));

		addFace(0, 11, 5);
		addFace(0, 5, 1);
		addFace(0, 1, 7);
		addFace(0, 7, 10);
		addFace(0, 10, 11);

		addFace(1, 5, 9);
		addFace(5, 11, 4);
		addFace(11, 10, 2);
		addFace(10, 7, 6);
		addFace(7, 1, 8);

		addFace(3, 9, 4);
		addFace(3, 4, 2);
		addFace(3, 2, 6);
		addFace(3, 6, 8);
		addFace(3, 8, 9);

		addFace(4, 9, 5);
		addFace(2, 4, 11);
		addFace(6, 2, 10);
		addFace(8, 6, 7);
		addFace(9, 8, 1);

		addLineIndices(1, 0);
		addLineIndices(1, 5);
		addLineIndices(1, 7);
		addLineIndices(1, 8);
		addLineIndices(1, 9);

		addLineIndices(2, 3);
		addLineIndices(2, 4);
		addLineIndices(2, 6);
		addLineIndices(2, 10);
		addLineIndices(2, 11);

		addLineIndices(0, 5);
		addLineIndices(5, 9);
		addLineIndices(9, 8);
		addLineIndices(8, 7);
		addLineIndices(7, 0);

		addLineIndices(10, 11);
		addLineIndices(11, 4);
		addLineIndices(4, 3);
		addLineIndices(3, 6);
		addLineIndices(6, 10);

		addLineIndices(0, 11);
		addLineIndices(11, 5);
		addLineIndices(5, 4);
		addLineIndices(4, 9);
		addLineIndices(9, 3);
		addLineIndices(3, 8);
		addLineIndices(8, 6);
		addLineIndices(6, 7);
		addLineIndices(7, 10);
		addLineIndices(10, 0);

		for (int i = 0; i < recursionLevel; i++)
		{
			std::list<TriangleIndices> faces2;

			for (std::list<TriangleIndices>::iterator j = faces.begin(); j != faces.end(); j++)
			{
				TriangleIndices f =* j;
				int a = getMiddlePoint(f.v1, f.v2);
				int b = getMiddlePoint(f.v2, f.v3);
				int c = getMiddlePoint(f.v3, f.v1);

				removeLineIndices(f.v1, f.v2);
				removeLineIndices(f.v2, f.v3);
				removeLineIndices(f.v3, f.v1);

				faces2.push_back(TriangleIndices(f.v1, a, c));
				faces2.push_back(TriangleIndices(f.v2, b, a));
				faces2.push_back(TriangleIndices(f.v3, c, b));
				faces2.push_back(TriangleIndices(a, b, c));

				addTriangleLines(f.v1, a, c);
				addTriangleLines(f.v2, b, a);
				addTriangleLines(f.v3, c, b);
			}

			faces = faces2;
		}
	}

	void IcoSphere::addLineIndices(int index0, int index1)
	{
		lineIndices.push_back(LineIndices(index0, index1));
	}

	void IcoSphere::removeLineIndices(int index0, int index1)
	{
		std::list<LineIndices>::iterator result = std::find(lineIndices.begin(), lineIndices.end(), LineIndices(index0, index1));

		if (result != lineIndices.end())
			lineIndices.erase(result);
	}

	void IcoSphere::addTriangleLines(int index0, int index1, int index2)
	{
		addLineIndices(index0, index1);
		addLineIndices(index1, index2);
		addLineIndices(index2, index0);
	}

	int IcoSphere::addVertex(const Ogre::Vector3& vertex)
	{
		Ogre::Real length = vertex.length();
		vertices.push_back(Ogre::Vector3(vertex.x / length, vertex.y / length, vertex.z / length));
		return index++;
	}

	int IcoSphere::getMiddlePoint(int index0, int index1)
	{
		bool isFirstSmaller = index0 < index1;
		__int64 smallerIndex = isFirstSmaller ? index0 : index1;
		__int64 largerIndex = isFirstSmaller ? index1 : index0;
		__int64 key = (smallerIndex << 32) | largerIndex;

		if (middlePointIndexCache.find(key) != middlePointIndexCache.end())
			return middlePointIndexCache[key];

		Ogre::Vector3 point1 = vertices[index0];
		Ogre::Vector3 point2 = vertices[index1];
		Ogre::Vector3 middle = point1.midPoint(point2);

		int index = addVertex(middle);
		middlePointIndexCache[key] = index;
		return index;
	}

	void IcoSphere::addFace(int index0, int index1, int index2)
	{
		faces.push_back(TriangleIndices(index0, index1, index2));
	}

	void IcoSphere::addToLineIndices(int baseIndex, std::queue<int>* target)
	{
		for (std::list<LineIndices>::iterator i = lineIndices.begin(); i != lineIndices.end(); i++)
		{
			target->push(baseIndex + (*i).v1);
			target->push(baseIndex + (*i).v2);
		}
	}

	void IcoSphere::addToTriangleIndices(int baseIndex, std::queue<int>* target)
	{
		for (std::list<TriangleIndices>::iterator i = faces.begin(); i != faces.end(); i++)
		{
			target->push(baseIndex + (*i).v1);
			target->push(baseIndex + (*i).v2);
			target->push(baseIndex + (*i).v3);
		}
	}

	int IcoSphere::addToVertices(std::queue<Vertex>* target, const Ogre::Vector3& position, const Ogre::ColourValue& color, Ogre::Real scale)
	{
		Ogre::Matrix4 transform = Ogre::Matrix4::IDENTITY;
		transform.setTrans(position);
		transform.setScale(Ogre::Vector3(scale, scale, scale));

		for (int i = 0; i < (int)vertices.size(); i++)
			target->push(Vertex(transform*  vertices[i], color));

		return vertices.size();
	}

	// ===============================================================================================


	GeometryDrawer::GeometryDrawer(Ogre::SceneManager* sceneManager, Ogre::Real fillAlpha)
		: linesIndex(0),
		trianglesIndex(0),
		drawRenderer(0),
		sceneManager(sceneManager),
		swapBuffer(false),
		isDirty(false),
		rendererIsDone(true)
	{
		icoSphere.create(DEFAULT_ICOSPHERE_RECURSION_LEVEL);
		this->drawRenderer = new DrawRenderer(0, sceneManager, &sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), fillAlpha);

		linesIndex = trianglesIndex = 0;
	}

	GeometryDrawer::~GeometryDrawer()
	{
		if (nullptr != this->drawRenderer)
		{
			delete this->drawRenderer;
			this->drawRenderer = nullptr;
		}
	}



	std::queue<Vertex>& GeometryDrawer::getLineVertices()
	{
		return this->swapBuffer ? lineVertices1 : lineVertices2;
	}

	std::queue<Vertex>& GeometryDrawer::getTriangleVertices()
	{
		return this->swapBuffer ? triangleVertices1 : triangleVertices2;
	}

	std::queue<int>& GeometryDrawer::getLineIndices()
	{
		return this->swapBuffer ? lineIndices1 : lineIndices2;
	}

	std::queue<int>& GeometryDrawer::getTriangleIndices()
	{
		return this->swapBuffer ? triangleIndices1 : triangleIndices2;
	}

	//void GeometryDrawer::initialise()
	//{
	//	/*manualObject = sceneManager->createManualObject("debug_object");
	//	sceneManager->getRootSceneNode()->createChildSceneNode("debug_object")->attachObject(manualObject);
	//	manualObject->setDynamic(true);*/

	//	icoSphere.create(DEFAULT_ICOSPHERE_RECURSION_LEVEL);



	//	linesIndex = trianglesIndex = 0;
	//}

	void GeometryDrawer::setIcoSphereRecursionLevel(int recursionLevel)
	{
		icoSphere.create(recursionLevel);
	}

	//void GeometryDrawer::shutdown()
	//{
	//	//sceneManager->destroySceneNode("debug_object");
	////	sceneManager->destroyManualObject(manualObject);
	//}

	void GeometryDrawer::buildLine(const Ogre::Vector3& start,
		const Ogre::Vector3& end,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int i = addLineVertex(start, Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addLineVertex(end, Ogre::ColourValue(color.r, color.g, color.b, alpha));

		addLineIndices(i, i + 1);
	}

	void GeometryDrawer::buildQuad(const Ogre::Vector3* vertices,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int index = addLineVertex(vertices[0], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addLineVertex(vertices[1], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addLineVertex(vertices[2], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addLineVertex(vertices[3], Ogre::ColourValue(color.r, color.g, color.b, alpha));

		for (int i = 0; i < 4; ++i) addLineIndices(index + i, index + ((i + 1) % 4));
	}

	void GeometryDrawer::buildCircle(const Ogre::Vector3& centre,
		Ogre::Real radius,
		int segmentsCount,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int index = linesIndex;
		Ogre::Real increment = 2*  Ogre::Math::PI / segmentsCount;
		Ogre::Real angle = 0.0f;

		for (int i = 0; i < segmentsCount; i++)
		{
			addLineVertex(Ogre::Vector3(centre.x + radius*  Ogre::Math::Cos(angle), centre.y, centre.z + radius*  Ogre::Math::Sin(angle)),
				Ogre::ColourValue(color.r, color.g, color.b, alpha));
			angle += increment;
		}

		for (int i = 0; i < segmentsCount; i++)
			addLineIndices(index + i, i + 1 < segmentsCount ? index + i + 1 : index);
	}

	void GeometryDrawer::buildFilledCircle(const Ogre::Vector3& centre,
		Ogre::Real radius,
		int segmentsCount,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int index = trianglesIndex;
		Ogre::Real increment = 2*  Ogre::Math::PI / segmentsCount;
		Ogre::Real angle = 0.0f;

		for (int i = 0; i < segmentsCount; i++)
		{
			addTriangleVertex(Ogre::Vector3(centre.x + radius*  Ogre::Math::Cos(angle), centre.y, centre.z + radius*  Ogre::Math::Sin(angle)),
				Ogre::ColourValue(color.r, color.g, color.b, alpha));
			angle += increment;
		}

		addTriangleVertex(centre, Ogre::ColourValue(color.r, color.g, color.b, alpha));

		for (int i = 0; i < segmentsCount; i++)
			addTriangleIndices(i + 1 < segmentsCount ? index + i + 1 : index, index + i, index + segmentsCount);
	}

	void GeometryDrawer::buildCylinder(const Ogre::Vector3& centre,
		Ogre::Real radius,
		int segmentsCount,
		Ogre::Real height,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int index = linesIndex;
		Ogre::Real increment = 2*  Ogre::Math::PI / segmentsCount;
		Ogre::Real angle = 0.0f;

		// Top circle
		for (int i = 0; i < segmentsCount; i++)
		{
			addLineVertex(Ogre::Vector3(centre.x + radius*  Ogre::Math::Cos(angle), centre.y + height / 2, centre.z + radius*  Ogre::Math::Sin(angle)),
				Ogre::ColourValue(color.r, color.g, color.b, alpha));
			angle += increment;
		}

		angle = 0.0f;

		// Bottom circle
		for (int i = 0; i < segmentsCount; i++)
		{
			addLineVertex(Ogre::Vector3(centre.x + radius*  Ogre::Math::Cos(angle), centre.y - height / 2, centre.z + radius*  Ogre::Math::Sin(angle)),
				Ogre::ColourValue(color.r, color.g, color.b, alpha));
			angle += increment;
		}

		for (int i = 0; i < segmentsCount; i++)
		{
			addLineIndices(index + i, i + 1 < segmentsCount ? index + i + 1 : index);
			addLineIndices(segmentsCount + index + i, i + 1 < segmentsCount ? segmentsCount + index + i + 1 : segmentsCount + index);
			addLineIndices(index + i, segmentsCount + index + i);
		}
	}

	void GeometryDrawer::buildFilledCylinder(const Ogre::Vector3& centre,
		Ogre::Real radius,
		int segmentsCount,
		Ogre::Real height,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int index = trianglesIndex;
		Ogre::Real increment = 2*  Ogre::Math::PI / segmentsCount;
		Ogre::Real angle = 0.0f;

		// Top circle
		for (int i = 0; i < segmentsCount; i++)
		{
			addTriangleVertex(Ogre::Vector3(centre.x + radius*  Ogre::Math::Cos(angle), centre.y + height / 2, centre.z + radius*  Ogre::Math::Sin(angle)),
				Ogre::ColourValue(color.r, color.g, color.b, alpha));
			angle += increment;
		}

		addTriangleVertex(Ogre::Vector3(centre.x, centre.y + height / 2, centre.z), Ogre::ColourValue(color.r, color.g, color.b, alpha));

		angle = 0.0f;

		// Bottom circle
		for (int i = 0; i < segmentsCount; i++)
		{
			addTriangleVertex(Ogre::Vector3(centre.x + radius*  Ogre::Math::Cos(angle), centre.y - height / 2, centre.z + radius*  Ogre::Math::Sin(angle)),
				Ogre::ColourValue(color.r, color.g, color.b, alpha));
			angle += increment;
		}

		addTriangleVertex(Ogre::Vector3(centre.x, centre.y - height / 2, centre.z), Ogre::ColourValue(color.r, color.g, color.b, alpha));

		for (int i = 0; i < segmentsCount; i++)
		{
			addTriangleIndices(i + 1 < segmentsCount ? index + i + 1 : index,
				index + i,
				index + segmentsCount);

			addTriangleIndices(i + 1 < segmentsCount ? (segmentsCount + 1) + index + i + 1 : (segmentsCount + 1) + index,
				(segmentsCount + 1) + index + segmentsCount,
				(segmentsCount + 1) + index + i);

			addQuadIndices(
				index + i,
				i + 1 < segmentsCount ? index + i + 1 : index,
				i + 1 < segmentsCount ? (segmentsCount + 1) + index + i + 1 : (segmentsCount + 1) + index,
				(segmentsCount + 1) + index + i);
		}
	}

	void GeometryDrawer::buildCuboid(const Ogre::Vector3* vertices,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int index = addLineVertex(vertices[0], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		for (int i = 1; i < 8; ++i) addLineVertex(vertices[i], Ogre::ColourValue(color.r, color.g, color.b, alpha));

		for (int i = 0; i < 4; ++i) addLineIndices(index + i, index + ((i + 1) % 4));
		for (int i = 4; i < 8; ++i) addLineIndices(index + i, i == 7 ? index + 4 : index + i + 1);
		addLineIndices(index + 1, index + 5);
		addLineIndices(index + 2, index + 4);
		addLineIndices(index, index + 6);
		addLineIndices(index + 3, index + 7);
	}

	void GeometryDrawer::buildFilledCuboid(const Ogre::Vector3* vertices,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int index = addTriangleVertex(vertices[0], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		for (int i = 1; i < 8; ++i) addTriangleVertex(vertices[i], Ogre::ColourValue(color.r, color.g, color.b, alpha));

		addQuadIndices(index, index + 1, index + 2, index + 3);
		addQuadIndices(index + 4, index + 5, index + 6, index + 7);

		addQuadIndices(index + 1, index + 5, index + 4, index + 2);
		addQuadIndices(index, index + 3, index + 7, index + 6);

		addQuadIndices(index + 1, index, index + 6, index + 5);
		addQuadIndices(index + 4, index + 7, index + 3, index + 2);
	}

	void GeometryDrawer::buildFilledQuad(const Ogre::Vector3* vertices,
		const Ogre::ColourValue& color,
		Ogre::Real alpha)
	{
		int index = addTriangleVertex(vertices[0], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addTriangleVertex(vertices[1], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addTriangleVertex(vertices[2], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addTriangleVertex(vertices[3], Ogre::ColourValue(color.r, color.g, color.b, alpha));

		addQuadIndices(index, index + 1, index + 2, index + 3);
	}

	void GeometryDrawer::buildFilledTriangle(const Ogre::Vector3* vertices, const Ogre::ColourValue& color, Ogre::Real alpha)
	{
		int index = addTriangleVertex(vertices[0], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addTriangleVertex(vertices[1], Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addTriangleVertex(vertices[2], Ogre::ColourValue(color.r, color.g, color.b, alpha));

		addTriangleIndices(index, index + 1, index + 2);
	}

	void GeometryDrawer::buildTetrahedron(const Ogre::Vector3& centre, Ogre::Real scale, const Ogre::ColourValue& color, Ogre::Real alpha)
	{
		int index = linesIndex;

		// Distance from the centre
		Ogre::Real bottomDistance = scale*  0.2f;
		Ogre::Real topDistance = scale*  0.62f;
		Ogre::Real frontDistance = scale*  0.289f;
		Ogre::Real backDistance = scale*  0.577f;
		Ogre::Real leftRightDistance = scale*  0.5f;

		addLineVertex(Ogre::Vector3(centre.x, centre.y + topDistance, centre.z),
			Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addLineVertex(Ogre::Vector3(centre.x, centre.y - bottomDistance, centre.z + frontDistance),
			Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addLineVertex(Ogre::Vector3(centre.x + leftRightDistance, centre.y - bottomDistance, centre.z - backDistance),
			Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addLineVertex(Ogre::Vector3(centre.x - leftRightDistance, centre.y - bottomDistance, centre.z - backDistance),
			Ogre::ColourValue(color.r, color.g, color.b, alpha));

		addLineIndices(index, index + 1);
		addLineIndices(index, index + 2);
		addLineIndices(index, index + 3);

		addLineIndices(index + 1, index + 2);
		addLineIndices(index + 2, index + 3);
		addLineIndices(index + 3, index + 1);
	}

	void GeometryDrawer::buildFilledTetrahedron(const Ogre::Vector3& centre, Ogre::Real scale, const Ogre::ColourValue& color, Ogre::Real alpha)
	{
		int index = trianglesIndex;

		// Distance from the centre
		Ogre::Real bottomDistance = scale * 0.2f;
		Ogre::Real topDistance = scale * 0.62f;
		Ogre::Real frontDistance = scale * 0.289f;
		Ogre::Real backDistance = scale * 0.577f;
		Ogre::Real leftRightDistance = scale * 0.5f;

		addTriangleVertex(Ogre::Vector3(centre.x, centre.y + topDistance, centre.z),
			Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addTriangleVertex(Ogre::Vector3(centre.x, centre.y - bottomDistance, centre.z + frontDistance),
			Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addTriangleVertex(Ogre::Vector3(centre.x + leftRightDistance, centre.y - bottomDistance, centre.z - backDistance),
			Ogre::ColourValue(color.r, color.g, color.b, alpha));
		addTriangleVertex(Ogre::Vector3(centre.x - leftRightDistance, centre.y - bottomDistance, centre.z - backDistance),
			Ogre::ColourValue(color.r, color.g, color.b, alpha));

		addTriangleIndices(index, index + 1, index + 2);
		addTriangleIndices(index, index + 2, index + 3);
		addTriangleIndices(index, index + 3, index + 1);

		addTriangleIndices(index + 1, index + 3, index + 2);
	}

	void GeometryDrawer::drawLine(const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::ColourValue& color)
	{
		buildLine(start, end, color);
	}

	void GeometryDrawer::drawCircle(const Ogre::Vector3& centre, Ogre::Real radius, int segmentsCount, const Ogre::ColourValue& color, bool isFilled)
	{
		buildCircle(centre, radius, segmentsCount, color);
		if (isFilled)
			buildFilledCircle(centre, radius, segmentsCount, color, fillAlpha);
		this->isDirty = true;
	}

	void GeometryDrawer::drawCylinder(const Ogre::Vector3& centre, Ogre::Real radius, int segmentsCount, Ogre::Real height, const Ogre::ColourValue& color, bool isFilled)
	{
		buildCylinder(centre, radius, segmentsCount, height, color);
		if (isFilled)
			buildFilledCylinder(centre, radius, segmentsCount, height, color, fillAlpha);
		this->isDirty = true;
	}

	void GeometryDrawer::drawQuad(const Ogre::Vector3* vertices, const Ogre::ColourValue& color, bool isFilled)
	{
		buildQuad(vertices, color);
		if (isFilled)
			buildFilledQuad(vertices, color, fillAlpha);
		this->isDirty = true;
	}

	void GeometryDrawer::drawCuboid(const Ogre::Vector3* vertices, const Ogre::ColourValue& color, bool isFilled)
	{
		buildCuboid(vertices, color);
		if (isFilled)
			buildFilledCuboid(vertices, color, fillAlpha);
		this->isDirty = true;
	}

	void GeometryDrawer::drawSphere(const Ogre::Vector3& centre, Ogre::Real radius, const Ogre::ColourValue& color, bool isFilled)
	{
		int baseIndex = linesIndex;
		linesIndex += icoSphere.addToVertices(&getLineVertices(), centre, color, radius);
		icoSphere.addToLineIndices(baseIndex, &getLineIndices());

		if (isFilled)
		{
			baseIndex = trianglesIndex;
			trianglesIndex += icoSphere.addToVertices(&getTriangleVertices(), centre, Ogre::ColourValue(color.r, color.g, color.b, fillAlpha), radius);
			icoSphere.addToTriangleIndices(baseIndex, &getTriangleIndices());
		}
		this->isDirty = true;
	}

	void GeometryDrawer::drawTetrahedron(const Ogre::Vector3& centre,
		Ogre::Real scale,
		const Ogre::ColourValue& color,
		bool isFilled)
	{
		buildTetrahedron(centre, scale, color);
		if (isFilled)
			buildFilledTetrahedron(centre, scale, color, fillAlpha);

		this->isDirty = true;
	}

	void GeometryDrawer::build()
	{
		if (isEnabled)
		{
			if (this->rendererIsDone && this->isDirty)
			{
				this->rendererIsDone = false;
				//	bool tmp = _swapBuffer;

				this->swapBuffer = this->swapBuffer ? false : true;

				if (!this->swapBuffer)
				{
					this->drawRenderer->updateVertices(lineIndices1, lineVertices1);
					this->rendererIsDone = true;
					this->isDirty = false;
				}
				else
				{

					this->drawRenderer->updateVertices(lineIndices2, lineVertices2);
					this->rendererIsDone = true;
					this->isDirty = false;
				}
			}
		}
	}

	void GeometryDrawer::clear()
	{
		std::queue<int> emptyLi;
		std::swap(getLineIndices(), emptyLi);

		std::queue<Vertex> emptyLV;
		std::swap(getLineVertices(), emptyLV);
	
		
		std::queue<int> emptyTi;
		std::swap(getTriangleIndices(), emptyTi);

		std::queue<Vertex> emptyTv;
		std::swap(getTriangleVertices(), emptyTv);

		
		linesIndex = trianglesIndex = 0;
	}

	int GeometryDrawer::addLineVertex(const Ogre::Vector3& vertex, const Ogre::ColourValue& color)
	{
		getLineVertices().push(Vertex(vertex, color));
		return linesIndex++;
	}

	void GeometryDrawer::addLineIndices(int index1, int index2)
	{
		getLineIndices().push(index1);
		getLineIndices().push(index2);
	}

	int GeometryDrawer::addTriangleVertex(const Ogre::Vector3& vertex, const Ogre::ColourValue& color)
	{
		getTriangleVertices().push(Vertex(vertex, color));
		return trianglesIndex++;
	}

	void GeometryDrawer::addTriangleIndices(int index1, int index2, int index3)
	{
		getTriangleIndices().push(index1);
		getTriangleIndices().push(index2);
		getTriangleIndices().push(index3);
	}

	void GeometryDrawer::addQuadIndices(int index1, int index2, int index3, int index4)
	{
		getTriangleIndices().push(index1);
		getTriangleIndices().push(index2);
		getTriangleIndices().push(index3);

		getTriangleIndices().push(index1);
		getTriangleIndices().push(index3);
		getTriangleIndices().push(index4);
	}

} // namespace end

#endif