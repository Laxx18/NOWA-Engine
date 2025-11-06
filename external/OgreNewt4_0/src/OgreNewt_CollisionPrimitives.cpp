#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_World.h"

#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "OgreBitwise.h"
#include "Vao/OgreAsyncTicket.h"

#include "OgreEntity.h"
#include "OgreSubMesh.h"
#include "OgreSceneNode.h"

namespace
{
	bool IsFiniteVec3(const Ogre::Vector3& v)
	{
		return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
	}
}

namespace OgreNewt
{
	namespace CollisionPrimitives
	{
		Null::Null(const OgreNewt::World* world) : Collision(world)
		{
			m_col = new ndShapeNull();
		}

		Box::Box(const World* world) : ConvexCollision(world)
		{
		}

		Box::Box(const World* world, const Ogre::Vector3& size, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeBox(ndFloat32(size.x), ndFloat32(size.y), ndFloat32(size.z)));
			m_col = m_shapeInstance->GetShape();
		}

		Ellipsoid::Ellipsoid(const World* world) : ConvexCollision(world)
		{
		}

		Ellipsoid::Ellipsoid(const World* world, const Ogre::Vector3& size, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			if ((size.x == size.y) && (size.y == size.z))
			{
				m_shapeInstance = new ndShapeInstance(new ndShapeSphere(ndFloat32(size.x * 0.5f)));
				m_col = m_shapeInstance->GetShape();
			}
			else
			{
				// For non-uniform ellipsoid, use sphere with the average radius
				ndFloat32 radius = (size.x + size.y + size.z) / 6.0f;

				m_shapeInstance = new ndShapeInstance(new ndShapeSphere(radius));
				m_col = m_shapeInstance->GetShape();
			}
		}

		Cylinder::Cylinder(const World* world) : ConvexCollision(world)
		{
		}

		Cylinder::Cylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeCylinder(ndFloat32(radius), ndFloat32(radius), ndFloat32(height)));
			m_col = m_shapeInstance->GetShape();
		}

		Capsule::Capsule(const World* world) : ConvexCollision(world)
		{
		}

		Capsule::Capsule(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeCapsule(ndFloat32(radius), ndFloat32(radius), ndFloat32(height - (radius * 2.0f))));
			m_col = m_shapeInstance->GetShape();
		}

		Capsule::Capsule(const World* world, const Ogre::Vector3& size, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeCapsule(ndFloat32(size.x), ndFloat32(size.z), ndFloat32(size.y)));
			m_col = m_shapeInstance->GetShape();
		}

		Cone::Cone(const World* world) : ConvexCollision(world)
		{
		}

		Cone::Cone(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeCone(ndFloat32(radius), ndFloat32(height)));
			m_col = m_shapeInstance->GetShape();
		}

		ChamferCylinder::ChamferCylinder(const World* world) : ConvexCollision(world)
		{
		}

		ChamferCylinder::ChamferCylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeChamferCylinder(ndFloat32(radius), ndFloat32(height)));
			m_col = m_shapeInstance->GetShape();
		}

		ConvexHull::ConvexHull(const World* world) : ConvexCollision(world)
		{
		}

#if 0
		ConvexHull::ConvexHull(const World* world, Ogre::v1::Entity* obj, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance,
			const Ogre::Vector3& forceScale)
			: ConvexCollision(world)
		{
			OGRE_ASSERT_LOW(obj);
			Ogre::Vector3 scale(1, 1, 1);

			if (Ogre::Node* node = obj->getParentNode())
				scale = node->_getDerivedScaleUpdated();
			if (forceScale != Ogre::Vector3::ZERO)
				scale = forceScale;

			Ogre::v1::MeshPtr mesh = obj->getMesh();
			if (mesh.isNull())
			{
				m_shapeInstance = nullptr;
				m_col = nullptr;
				return;
			}

			const unsigned short subCount = mesh->getNumSubMeshes();
			size_t totalVerts = 0;

			// Pass 1: count vertices (shared counted once)
			bool sharedAdded = false;
			for (unsigned short i = 0; i < subCount; ++i)
			{
				Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);
				if (!sub) continue;

				Ogre::v1::VertexData* vdata = nullptr;
				if (sub->useSharedVertices)
				{
					if (!sharedAdded)
					{
						vdata = mesh->sharedVertexData[0];
						sharedAdded = true;
					}
				}
				else
				{
					vdata = sub->vertexData[0];
				}
				if (vdata)
					totalVerts += vdata->vertexCount;
			}

			if (totalVerts < 4)
			{
				// Not enough points for a convex hull
				m_shapeInstance = nullptr;
				m_col = nullptr;
				return;
			}

			// Pass 2: read positions
			Ogre::Vector3* vertices = new Ogre::Vector3[totalVerts];
			unsigned int writeOfs = 0;
			sharedAdded = false;

			for (unsigned short i = 0; i < subCount; ++i)
			{
				Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);
				if (!sub) continue;

				Ogre::v1::VertexData* vdata = nullptr;
				if (sub->useSharedVertices)
				{
					if (!sharedAdded)
					{
						vdata = mesh->sharedVertexData[0];
						sharedAdded = true;
					}
				}
				else
				{
					vdata = sub->vertexData[0];
				}
				if (!vdata) continue;

				const Ogre::v1::VertexElement* posElem =
					vdata->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				if (!posElem) continue;

				const size_t start = vdata->vertexStart;
				const size_t vCount = vdata->vertexCount;

				Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
					vdata->vertexBufferBinding->getBuffer(posElem->getSource());
				if (!vbuf) continue;

				unsigned char* base = static_cast<unsigned char*>(
					vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				const size_t stride = vbuf->getVertexSize();

				for (size_t j = 0; j < vCount; ++j)
				{
					unsigned char* ptr = base + (start + j) * stride;
					float* posPtr = nullptr;
					posElem->baseVertexPointerToElement(ptr, &posPtr);

					Ogre::Vector3 v(posPtr[0], posPtr[1], posPtr[2]);
					vertices[writeOfs++] = v * scale;
				}

				vbuf->unlock();
			}

			if (m_col) m_col->Release();

			// Build the ND4 convex hull with the correct stride (avoid SSE/padding surprises)
			const ndInt32 count = ndInt32(totalVerts);
			const ndInt32 stride = ndInt32(sizeof(vertices[0])); // sizeof(Ogre::Vector3)

			m_shapeInstance = new ndShapeInstance(
				new ndShapeConvexHull(count, stride, ndFloat32(tolerance),
					(ndFloat32*)&vertices[0].x));
			m_col = m_shapeInstance->GetShape();

			// Apply local offset/orientation
			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			delete[] vertices;
		}
#else
	ConvexHull::ConvexHull(const World* world, Ogre::v1::Entity* obj, unsigned int id,
		const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance,
		const Ogre::Vector3& forceScale)
		: ConvexCollision(world)
	{
		OGRE_ASSERT_LOW(obj);
		Ogre::Vector3 scale(1, 1, 1);
		if (Ogre::Node* node = obj->getParentNode())
			scale = node->_getDerivedScaleUpdated();
		if (forceScale != Ogre::Vector3::ZERO)
			scale = forceScale;

		Ogre::v1::MeshPtr mesh = obj->getMesh();
		if (mesh.isNull()) { m_shapeInstance = nullptr; m_col = nullptr; return; }

		const unsigned short subCount = mesh->getNumSubMeshes();
		size_t totalVerts = 0;

		// Pass 1: count vertices (shared once)
		bool sharedAdded = false;
		for (unsigned short i = 0; i < subCount; ++i)
		{
			Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);
			if (!sub) continue;
			Ogre::v1::VertexData* vdata = nullptr;
			if (sub->useSharedVertices) {
				if (!sharedAdded) { vdata = mesh->sharedVertexData[0]; sharedAdded = true; }
			}
			else {
				vdata = sub->vertexData[0];
			}
			if (vdata) totalVerts += vdata->vertexCount;
		}
		if (totalVerts < 4) { m_shapeInstance = nullptr; m_col = nullptr; return; }

		// Pass 2: read positions -> temp CPU array of Ogre::Vector3
		std::vector<Ogre::Vector3> verts;
		verts.reserve(totalVerts);
		sharedAdded = false;

		for (unsigned short i = 0; i < subCount; ++i)
		{
			Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);
			if (!sub) continue;

			Ogre::v1::VertexData* vdata = nullptr;
			if (sub->useSharedVertices) {
				if (!sharedAdded) { vdata = mesh->sharedVertexData[0]; sharedAdded = true; }
			}
			else {
				vdata = sub->vertexData[0];
			}
			if (!vdata) continue;

			const Ogre::v1::VertexElement* posElem =
				vdata->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
			if (!posElem) continue;

			const size_t start = vdata->vertexStart;
			const size_t vCount = vdata->vertexCount;

			Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
				vdata->vertexBufferBinding->getBuffer(posElem->getSource());
			if (!vbuf) continue;

			unsigned char* base = static_cast<unsigned char*>(
				vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
			const size_t stride = vbuf->getVertexSize();

			for (size_t j = 0; j < vCount; ++j)
			{
				unsigned char* ptr = base + (start + j) * stride;
				float* p = nullptr;
				posElem->baseVertexPointerToElement(ptr, &p);
				Ogre::Vector3 v(p[0], p[1], p[2]);
				verts.push_back(v * scale);
			}
			vbuf->unlock();
		}

		// Clean input & convert to contiguous ndFloat32 buffer (XYZXYZ…)
		const float dedupEps2 = 1e-16f; // collapse ~1e-8 distance
		std::vector<ndFloat32> points; points.reserve(verts.size() * 3);
		{
			// optional: compute bbox for adaptive tolerance
			Ogre::AxisAlignedBox aabb;
			for (const auto& v : verts) if (IsFiniteVec3(v)) aabb.merge(v);
			const Ogre::Real diag = aabb.isNull() ? Ogre::Real(1) : aabb.getSize().length();
			if (tolerance <= 0) tolerance = std::max<Ogre::Real>(diag * Ogre::Real(1e-4), 1e-6f);

			// dedup + push
			for (const auto& v : verts)
			{
				if (!IsFiniteVec3(v)) continue;
				bool dup = false;
				// cheap linear dedup (ok for typical mesh vertex counts for hulls)
				for (size_t i = 0; i + 2 < points.size(); i += 3)
				{
					const float dx = points[i + 0] - v.x;
					const float dy = points[i + 1] - v.y;
					const float dz = points[i + 2] - v.z;
					if (dx * dx + dy * dy + dz * dz <= dedupEps2) { dup = true; break; }
				}
				if (!dup) { points.push_back(v.x); points.push_back(v.y); points.push_back(v.z); }
			}
		}

		if (points.size() < 12) { // < 4 points
			m_shapeInstance = nullptr; m_col = nullptr; return;
		}

		if (m_col) m_col->Release();

		const ndInt32 count = ndInt32(points.size() / 3);
		const ndInt32 stride = ndInt32(sizeof(ndFloat32) * 3); // tightly packed floats

		m_shapeInstance = new ndShapeInstance(
			new ndShapeConvexHull(count, stride, ndFloat32(tolerance),
				reinterpret_cast<const ndFloat32*>(&points[0])));
		m_col = m_shapeInstance->GetShape();

		ndMatrix localM;
		OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
		m_shapeInstance->SetLocalMatrix(localM);
	}
#endif

#if 0
		ConvexHull::ConvexHull(const World* world, Ogre::Item* item, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos,
			Ogre::Real tolerance, const Ogre::Vector3& forceScale)
			: ConvexCollision(world)
		{
			OGRE_ASSERT_LOW(item);

			// Determine scale (node scale unless forceScale is provided)
			Ogre::Vector3 scale(1, 1, 1);
			if (Ogre::Node* node = item->getParentNode())
				scale = node->_getDerivedScaleUpdated();
			if (forceScale != Ogre::Vector3::ZERO)
				scale = forceScale;

			// Gather all positions from VAOs
			std::vector<Ogre::Vector3> verts;
			verts.reserve(4096);

			Ogre::MeshPtr mesh = item->getMesh();
			if (!mesh.isNull())
			{
				const Ogre::Mesh::SubMeshVec& subs = mesh->getSubMeshes();
				for (Ogre::SubMesh* sub : subs)
				{
					if (!sub) continue;

					const Ogre::VertexArrayObjectArray& vaos = sub->mVao[0];
					if (vaos.empty()) continue;

					Ogre::VertexArrayObject* vao = vaos[0];

					// Request position stream
					Ogre::VertexArrayObject::ReadRequestsVec reqs;
					reqs.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
					vao->readRequests(reqs);
					vao->mapAsyncTickets(reqs);

					const Ogre::VertexBufferPacked* vb = reqs[0].vertexBuffer;
					if (vb)
					{
						const size_t count = vb->getNumElements();
						verts.reserve(verts.size() + count);

						if (reqs[0].type == Ogre::VET_HALF4)
						{
							for (size_t i = 0; i < count; ++i)
							{
								const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(reqs[0].data);
								Ogre::Vector3 v(Ogre::Bitwise::halfToFloat(p[0]),
									Ogre::Bitwise::halfToFloat(p[1]),
									Ogre::Bitwise::halfToFloat(p[2]));
								reqs[0].data += vb->getBytesPerElement();
								verts.push_back(v * scale);
							}
						}
						else if (reqs[0].type == Ogre::VET_FLOAT3)
						{
							for (size_t i = 0; i < count; ++i)
							{
								const float* p = reinterpret_cast<const float*>(reqs[0].data);
								Ogre::Vector3 v(p[0], p[1], p[2]);
								reqs[0].data += vb->getBytesPerElement();
								verts.push_back(v * scale);
							}
						}
						// other vertex types are ignored intentionally
					}

					vao->unmapAsyncTickets(reqs);
				}
			}

			// Need at least 4 points for a convex hull
			if (verts.size() < 4)
			{
				if (m_col) m_col->Release();
				m_shapeInstance = nullptr;
				m_col = nullptr;
				return;
			}

			if (m_col)
				m_col->Release();

			// Build ND4 convex hull with the ACTUAL stride of Ogre::Vector3
			const ndInt32 count = ndInt32(verts.size());
			const ndInt32 stride = ndInt32(sizeof(verts[0])); // do NOT hardcode 12; some builds may pad

			m_shapeInstance = new ndShapeInstance(
				new ndShapeConvexHull(count,
					stride,
					ndFloat32(tolerance),
					(ndFloat32*)&verts[0].x));
			m_col = m_shapeInstance->GetShape();

			// Apply local transform (orientation + position) to the shape instance
			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);
		}
#else
	ConvexHull::ConvexHull(const World* world, Ogre::Item* item, unsigned int id,
		const Ogre::Quaternion& orient, const Ogre::Vector3& pos,
		Ogre::Real tolerance, const Ogre::Vector3& forceScale)
		: ConvexCollision(world)
	{
		OGRE_ASSERT_LOW(item);

		Ogre::Vector3 scale(1, 1, 1);
		if (Ogre::Node* node = item->getParentNode())
			scale = node->_getDerivedScaleUpdated();
		if (forceScale != Ogre::Vector3::ZERO)
			scale = forceScale;

		std::vector<Ogre::Vector3> verts;
		verts.reserve(4096);

		Ogre::MeshPtr mesh = item->getMesh();
		if (!mesh.isNull())
		{
			for (Ogre::SubMesh* sub : mesh->getSubMeshes())
			{
				if (!sub) continue;
				const Ogre::VertexArrayObjectArray& vaos = sub->mVao[0]; // VpNormal
				if (vaos.empty()) continue;

				Ogre::VertexArrayObject* vao = vaos[0];

				Ogre::VertexArrayObject::ReadRequestsVec reqs;
				reqs.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
				vao->readRequests(reqs);
				vao->mapAsyncTickets(reqs);

				const Ogre::VertexBufferPacked* vb = reqs[0].vertexBuffer;
				if (vb)
				{
					const size_t count = vb->getNumElements();
					verts.reserve(verts.size() + count);

					if (reqs[0].type == Ogre::VET_HALF4)
					{
						for (size_t i = 0; i < count; ++i)
						{
							const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(reqs[0].data);
							Ogre::Vector3 v(Ogre::Bitwise::halfToFloat(p[0]),
								Ogre::Bitwise::halfToFloat(p[1]),
								Ogre::Bitwise::halfToFloat(p[2]));
							reqs[0].data += vb->getBytesPerElement();
							verts.push_back(v * scale);
						}
					}
					else if (reqs[0].type == Ogre::VET_FLOAT3)
					{
						for (size_t i = 0; i < count; ++i)
						{
							const float* p = reinterpret_cast<const float*>(reqs[0].data);
							Ogre::Vector3 v(p[0], p[1], p[2]);
							reqs[0].data += vb->getBytesPerElement();
							verts.push_back(v * scale);
						}
					}
				}

				vao->unmapAsyncTickets(reqs);
			}
		}

		// Need at least 4 points
		if (verts.size() < 4) { if (m_col) m_col->Release(); m_shapeInstance = nullptr; m_col = nullptr; return; }

		// Clean & convert to contiguous ndFloat32 buffer
		const float dedupEps2 = 1e-16f;

		// Optional: adaptive tolerance
		Ogre::AxisAlignedBox aabb;
		for (const auto& v : verts) if (IsFiniteVec3(v)) aabb.merge(v);
		const Ogre::Real diag = aabb.isNull() ? Ogre::Real(1) : aabb.getSize().length();
		if (tolerance <= 0) tolerance = std::max<Ogre::Real>(diag * Ogre::Real(1e-4), 1e-6f);

		std::vector<ndFloat32> points; points.reserve(verts.size() * 3);
		for (const auto& v : verts)
		{
			if (!IsFiniteVec3(v)) continue;
			bool dup = false;
			for (size_t i = 0; i + 2 < points.size(); i += 3)
			{
				const float dx = points[i + 0] - v.x;
				const float dy = points[i + 1] - v.y;
				const float dz = points[i + 2] - v.z;
				if (dx * dx + dy * dy + dz * dz <= dedupEps2) { dup = true; break; }
			}
			if (!dup) { points.push_back(v.x); points.push_back(v.y); points.push_back(v.z); }
		}

		if (points.size() < 12) { if (m_col) m_col->Release(); m_shapeInstance = nullptr; m_col = nullptr; return; }

		if (m_col) m_col->Release();

		const ndInt32 count = ndInt32(points.size() / 3);
		const ndInt32 stride = ndInt32(sizeof(ndFloat32) * 3); // tightly packed XYZ floats

		m_shapeInstance = new ndShapeInstance(
			new ndShapeConvexHull(count, stride, ndFloat32(tolerance),
				reinterpret_cast<const ndFloat32*>(&points[0])));
		m_col = m_shapeInstance->GetShape();

		ndMatrix localM;
		OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
		m_shapeInstance->SetLocalMatrix(localM);
	}
#endif

		ConvexHull::ConvexHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeConvexHull(vertcount, sizeof(Ogre::Vector3), ndFloat32(tolerance), (ndFloat32*)&verts[0].x));
			m_col = m_shapeInstance->GetShape();

			{
				ndMatrix localM;
				OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
				m_shapeInstance->SetLocalMatrix(localM);
			}
		}

		ConcaveHull::ConcaveHull(const World* world) : ConvexCollision(world)
		{
		}

		ConcaveHull::ConcaveHull(const World* world, Ogre::v1::Entity* obj, unsigned int id,
			Ogre::Real tolerance, const Ogre::Vector3& scale) : ConvexCollision(world)
		{
			// Note: Newton 4.0 handles concave hulls differently
			// This may need to use ndShapeCompound with multiple convex pieces
			// Placeholder implementation - needs proper Newton 4.0 mesh decomposition
		}

		ConcaveHull::ConcaveHull(const World* world, const Ogre::Vector3* verts, int vertcount,
			unsigned int id, Ogre::Real tolerance) : ConvexCollision(world)
		{
		}
#if 0
		TreeCollision::TreeCollision(const World* world, Ogre::v1::Entity* obj, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;
			start(id);

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			Ogre::v1::MeshPtr mesh = obj->getMesh();

			Ogre::Vector3 scale(1.0f);
			if (Ogre::Node* node = obj->getParentNode())
				scale = node->_getDerivedScaleUpdated();

			const unsigned short subCount = mesh->getNumSubMeshes();
			for (unsigned short s = 0; s < subCount; ++s)
			{
				Ogre::v1::SubMesh* sub = mesh->getSubMesh(s);

				Ogre::v1::VertexData* vdata =
					sub->useSharedVertices ? mesh->sharedVertexData[0] : sub->vertexData[0];

				if (!vdata || !vdata->vertexDeclaration)
					continue;

				const Ogre::v1::VertexElement* posElem =
					vdata->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				if (!posElem)
					continue;

				Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
					vdata->vertexBufferBinding->getBuffer(posElem->getSource());
				if (vbuf.isNull() || vbuf->getNumVertices() == 0)
					continue;

				unsigned char* vbase =
					static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				Ogre::v1::IndexData* idata = sub->indexData[0];
				if (!idata || !idata->indexBuffer || idata->indexCount < 3)
				{
					vbuf->unlock();
					continue;
				}

				Ogre::v1::HardwareIndexBufferSharedPtr ibuf = idata->indexBuffer;
				const bool use32 = (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);

				const size_t indexCount = idata->indexCount;
				const size_t triCount = indexCount / 3;

				const unsigned long* i32 = nullptr;
				const unsigned short* i16 = nullptr;

				if (use32)
					i32 = static_cast<unsigned long*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
				else
					i16 = static_cast<unsigned short*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				int iOff = 0;
				for (size_t t = 0; t < triCount; ++t)
				{
					Ogre::Vector3 v[3];

					for (int j = 0; j < 3; ++j)
					{
						const int idx = use32 ? static_cast<int>(i32[iOff + j])
							: static_cast<int>(i16[iOff + j]);

						unsigned char* vptr = vbase + idx * vbuf->getVertexSize();
						float* pos = nullptr;
						posElem->baseVertexPointerToElement(vptr, &pos);

						v[j].x = *pos++; v[j].y = *pos++; v[j].z = *pos++;
						v[j] *= scale;
					}

					if (fw == FW_REVERSE)
						std::swap(v[1], v[2]);

					meshBuilder.AddFace(reinterpret_cast<const ndFloat32*>(&v[0].x), static_cast<ndInt32>(sizeof(Ogre::Vector3)), 3, static_cast<ndInt32>(id));

					iOff += 3;
				}

				vbuf->unlock();
				ibuf->unlock();
			}

			finish(optimize);

			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}
#else
		OgreNewt::CollisionPrimitives::TreeCollision::TreeCollision(
			const OgreNewt::World* world, Ogre::v1::Entity* obj,
			bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			Ogre::Vector3 scale(1, 1, 1);
			if (Ogre::Node* node = obj->getParentNode())
				scale = node->_getDerivedScaleUpdated();

			// Flip winding if mirrored scale
			FaceWinding localFw = fw;
			if (scale.x * scale.y * scale.z < 0.0f)
				localFw = (fw == FW_DEFAULT) ? FW_REVERSE : FW_DEFAULT;

			if (m_col)
				m_col->Release();

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			Ogre::v1::MeshPtr mesh = obj->getMesh();
			const unsigned short subCount = mesh->getNumSubMeshes();

			for (unsigned short i = 0; i < subCount; ++i)
			{
				Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);
				if (!sub) continue;

				Ogre::v1::VertexData* vData = sub->useSharedVertices ? mesh->sharedVertexData[0] : sub->vertexData[0];
				if (!vData) continue;

				const Ogre::v1::VertexElement* posElem =
					vData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				if (!posElem) continue;

				Ogre::v1::HardwareVertexBufferSharedPtr vBuf =
					vData->vertexBufferBinding->getBuffer(posElem->getSource());
				unsigned char* base = static_cast<unsigned char*>(vBuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
				const size_t stride = vBuf->getVertexSize();

				Ogre::v1::IndexData* iData = sub->indexData[0];
				if (!iData) { vBuf->unlock(); continue; }

				Ogre::v1::HardwareIndexBufferSharedPtr iBuf = iData->indexBuffer;
				const bool use32 = (iBuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);
				unsigned char* iBase = static_cast<unsigned char*>(iBuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				const unsigned int numPolys = static_cast<unsigned int>(iData->indexCount / 3);
				const unsigned int indexSize = static_cast<unsigned int>(iBuf->getIndexSize());
				const unsigned int iStart = static_cast<unsigned int>(iData->indexStart);

				for (unsigned int poly = 0; poly < numPolys; ++poly)
				{
					Ogre::Vector3 v[3];
					for (int j = 0; j < 3; ++j)
					{
						unsigned int index;
						if (use32)
							index = reinterpret_cast<unsigned int*>(iBase)[iStart + poly * 3 + j];
						else
							index = reinterpret_cast<unsigned short*>(iBase)[iStart + poly * 3 + j];

						unsigned char* ptr = base + (index * stride);
						float* pos = nullptr;
						posElem->baseVertexPointerToElement(ptr, &pos);
						v[j] = Ogre::Vector3(pos[0], pos[1], pos[2]) * scale;
					}

					// Gentle extrusion for degenerate faces
					Ogre::Vector3 e1 = v[1] - v[0];
					Ogre::Vector3 e2 = v[2] - v[0];
					Ogre::Vector3 n = e1.crossProduct(e2);
					Ogre::Real area2 = n.squaredLength();
					if (area2 < 1e-16f)
					{
						if (n.isZeroLength())
							n = Ogre::Vector3::UNIT_Y;
						else
							n.normalise();
						const Ogre::Real EPS = 1e-4f;
						v[0] += n * EPS;
						v[1] += n * EPS * 0.5f;
						v[2] += n * EPS * 0.25f;
					}

					ndVector face[3];
					if (localFw == FW_DEFAULT)
					{
						face[0] = ndVector(v[0].x, v[0].y, v[0].z, 1.0f);
						face[1] = ndVector(v[1].x, v[1].y, v[1].z, 1.0f);
						face[2] = ndVector(v[2].x, v[2].y, v[2].z, 1.0f);
					}
					else
					{
						face[0] = ndVector(v[0].x, v[0].y, v[0].z, 1.0f);
						face[1] = ndVector(v[2].x, v[2].y, v[2].z, 1.0f);
						face[2] = ndVector(v[1].x, v[1].y, v[1].z, 1.0f);
					}

					meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
				}

				vBuf->unlock();
				iBuf->unlock();
			}

			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}
#endif

#if 0
		TreeCollision::TreeCollision(const World* world, Ogre::Item* item, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;

			// begin legacy wrapper state
			start(id);

			// ND4: build polygon soup
			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			Ogre::MeshPtr mesh = item->getMesh();
			Ogre::Vector3 scale(1.0f);

			if (Ogre::Node* node = item->getParentNode())
				scale = node->_getDerivedScaleUpdated();

			// Iterate submeshes / first LOD
			for (Ogre::SubMesh* subMesh : mesh->getSubMeshes())
			{
				const Ogre::VertexArrayObjectArray& vaos = subMesh->mVao[0];
				if (vaos.empty())
					continue;

				Ogre::VertexArrayObject* vao = vaos[0];

				// --- request POSITION stream (Ogre-managed async tickets)
				Ogre::VertexArrayObject::ReadRequestsVec requests;
				requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
				vao->readRequests(requests);
				Ogre::VertexArrayObject::mapAsyncTickets(requests);

				// Position stream info
				const Ogre::VertexElementType posType = requests[0].type;                 // VET_FLOAT3 or VET_HALF4
				const Ogre::VertexBufferPacked* posBuf = requests[0].vertexBuffer;
				const size_t posStride = posBuf->getBytesPerElement();
				// Header note: data is already offseted; to get base pointer, do data - offset
				const char* posBase = requests[0].data - requests[0].offset;

				// --- index buffer
				Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
				if (!indexBuffer || indexBuffer->getNumElements() < 3)
				{
					Ogre::VertexArrayObject::unmapAsyncTickets(requests);
					continue;
				}

				const bool indices32 = (indexBuffer->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);
				const size_t indexCount = indexBuffer->getNumElements();

				// Map index data via async ticket from IndexBufferPacked
				Ogre::AsyncTicketPtr idxTicket = indexBuffer->readRequest(0, indexCount);
				const void* idxDataRaw = idxTicket->map();

				auto getIndex = [&](size_t i) -> uint32_t {
					return indices32
						? static_cast<uint32_t>(reinterpret_cast<const uint32_t*>(idxDataRaw)[i])
						: static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(idxDataRaw)[i]);
					};

				const size_t triCount = indexCount / 3;
				for (size_t t = 0; t < triCount; ++t)
				{
					const uint32_t i0 = getIndex(t * 3 + 0);
					const uint32_t i1 = getIndex(t * 3 + 1);
					const uint32_t i2 = getIndex(t * 3 + 2);

					Ogre::Vector3 v[3];

					auto loadFloat3 = [&](uint32_t idx) -> Ogre::Vector3 {
						const float* p = reinterpret_cast<const float*>(posBase + idx * posStride);
						return Ogre::Vector3(p[0], p[1], p[2]);
						};
					auto loadHalf4 = [&](uint32_t idx) -> Ogre::Vector3 {
						const Ogre::uint16* h = reinterpret_cast<const Ogre::uint16*>(posBase + idx * posStride);
						return Ogre::Vector3(
							Ogre::Bitwise::halfToFloat(h[0]),
							Ogre::Bitwise::halfToFloat(h[1]),
							Ogre::Bitwise::halfToFloat(h[2])
						);
						};

					if (posType == Ogre::VET_FLOAT3)
					{
						v[0] = loadFloat3(i0) * scale;
						v[1] = loadFloat3(i1) * scale;
						v[2] = loadFloat3(i2) * scale;
					}
					else if (posType == Ogre::VET_HALF4)
					{
						v[0] = loadHalf4(i0) * scale;
						v[1] = loadHalf4(i1) * scale;
						v[2] = loadHalf4(i2) * scale;
					}
					else
					{
						// Unsupported vertex format for positions
						continue;
					}

					if (fw == FW_REVERSE)
						std::swap(v[1], v[2]);

					// keep legacy internal storage if you need it
					addPoly(v, static_cast<int>(t));

					// feed Newton soup (x,y,z) tightly packed, stride = sizeof(Vector3)
					meshBuilder.AddFace(
						reinterpret_cast<const ndFloat32*>(&v[0].x),
						static_cast<ndInt32>(sizeof(Ogre::Vector3)),
						3,
						static_cast<ndInt32>(t));
				}

				// Unmap
				idxTicket->unmap();
				Ogre::VertexArrayObject::unmapAsyncTickets(requests);
			}

			// finalize legacy wrapper + ND4 BVH
			finish(optimize);

			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}
#else
		OgreNewt::CollisionPrimitives::TreeCollision::TreeCollision(
			const OgreNewt::World* world, Ogre::Item* item,
			bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			Ogre::Vector3 scale(1.0f, 1.0f, 1.0f);
			if (Ogre::Node* node = item->getParentNode())
				scale = node->_getDerivedScaleUpdated();

			FaceWinding localFw = fw;
			if (scale.x * scale.y * scale.z < 0.0f)
				localFw = (fw == FW_DEFAULT) ? FW_REVERSE : FW_DEFAULT;

			Ogre::MeshPtr mesh = item->getMesh();
			std::vector<Ogre::Vector3> vertices;
			std::vector<int> indices;

			for (auto* subMesh : mesh->getSubMeshes())
			{
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[0];
				if (vaos.empty()) continue;
				Ogre::VertexArrayObject* vao = vaos[0];

				const size_t vertexOffset = vertices.size();
				Ogre::VertexArrayObject::ReadRequestsVec requests;
				requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
				vao->readRequests(requests);
				vao->mapAsyncTickets(requests);

				const size_t vCount = requests[0].vertexBuffer->getNumElements();
				if (requests[0].type == Ogre::VET_HALF4)
				{
					for (size_t i = 0; i < vCount; ++i)
					{
						const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
						Ogre::Vector3 v(Ogre::Bitwise::halfToFloat(p[0]),
							Ogre::Bitwise::halfToFloat(p[1]),
							Ogre::Bitwise::halfToFloat(p[2]));
						requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
						vertices.push_back(v * scale);
					}
				}
				else if (requests[0].type == Ogre::VET_FLOAT3)
				{
					for (size_t i = 0; i < vCount; ++i)
					{
						const float* p = reinterpret_cast<const float*>(requests[0].data);
						Ogre::Vector3 v(p[0], p[1], p[2]);
						requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
						vertices.push_back(v * scale);
					}
				}
				vao->unmapAsyncTickets(requests);

				Ogre::IndexBufferPacked* ib = vao->getIndexBuffer();
				if (!ib) continue;

				const size_t num = ib->getNumElements();
				const void* data = ib->map(0, num);
				if (ib->getIndexType() == Ogre::IndexBufferPacked::IT_16BIT)
				{
					const Ogre::uint16* idx = reinterpret_cast<const Ogre::uint16*>(data);
					for (size_t i = 0; i < num; ++i)
						indices.push_back(static_cast<int>(idx[i]) + static_cast<int>(vertexOffset));
				}
				else
				{
					const Ogre::uint32* idx = reinterpret_cast<const Ogre::uint32*>(data);
					for (size_t i = 0; i < num; ++i)
						indices.push_back(static_cast<int>(idx[i]) + static_cast<int>(vertexOffset));
				}
				ib->unmap(Ogre::UO_KEEP_PERSISTENT);
			}

			if (m_col)
				m_col->Release();

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			for (size_t i = 0; i + 2 < indices.size(); i += 3)
			{
				Ogre::Vector3 a = vertices[indices[i + 0]];
				Ogre::Vector3 b = vertices[indices[i + 1]];
				Ogre::Vector3 c = vertices[indices[i + 2]];

				// Gentle extrusion for degenerate faces
				Ogre::Vector3 e1 = b - a;
				Ogre::Vector3 e2 = c - a;
				Ogre::Vector3 n = e1.crossProduct(e2);
				Ogre::Real area2 = n.squaredLength();
				if (area2 < 1e-16f)
				{
					if (n.isZeroLength())
						n = Ogre::Vector3::UNIT_Y;
					else
						n.normalise();
					const Ogre::Real EPS = 1e-4f;
					a += n * EPS;
					b += n * EPS * 0.5f;
					c += n * EPS * 0.25f;
				}

				ndVector face[3];
				if (localFw == FW_DEFAULT)
				{
					face[0] = ndVector(a.x, a.y, a.z, 1.0f);
					face[1] = ndVector(b.x, b.y, b.z, 1.0f);
					face[2] = ndVector(c.x, c.y, c.z, 1.0f);
				}
				else
				{
					face[0] = ndVector(a.x, a.y, a.z, 1.0f);
					face[1] = ndVector(c.x, c.y, c.z, 1.0f);
					face[2] = ndVector(b.x, b.y, b.z, 1.0f);
				}

				meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
			}

			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}
#endif

#if 0
		TreeCollision::TreeCollision(const OgreNewt::World* world, int numVertices, int numIndices, const float* vertices, const int* indices, bool optimize, unsigned int id, FaceWinding fw)
			: OgreNewt::Collision(world)
		{
			start(id);

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			// Early outs / sanity
			if (!vertices || !indices || numVertices <= 0 || numIndices < 3 || (numIndices % 3) != 0)
			{
				finish(optimize);
				meshBuilder.End(optimize);
				m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
				m_col = m_shapeInstance->GetShape();
				return;
			}

			// Build an Ogre::Vector3 cache for convenient access
			Ogre::Vector3* vecVertices = new Ogre::Vector3[numVertices];
			for (int v = 0; v < numVertices; ++v)
			{
				const int off = v * 3;
				vecVertices[v].x = vertices[off + 0];
				vecVertices[v].y = vertices[off + 1];
				vecVertices[v].z = vertices[off + 2];
			}

			const int numPolys = numIndices / 3;
			for (int p = 0; p < numPolys; ++p)
			{
				const int i0 = indices[p * 3 + 0];
				const int i1 = indices[p * 3 + 1];
				const int i2 = indices[p * 3 + 2];

				// Bounds check (defensive)
				if (i0 < 0 || i1 < 0 || i2 < 0 ||
					i0 >= numVertices || i1 >= numVertices || i2 >= numVertices)
					continue;

				Ogre::Vector3 v[3];
				v[0] = vecVertices[i0];
				v[1] = vecVertices[i1];
				v[2] = vecVertices[i2];

				if (fw == FW_REVERSE)
					std::swap(v[1], v[2]);

				// Degeneracy filter: skip zero-length edges or near-zero area
				const Ogre::Vector3 e0 = v[1] - v[0];
				const Ogre::Vector3 e1 = v[2] - v[0];
				const float edgeMin2 = 1.0e-20f;   // ~1e-10 length
				const float areaMin2 = 1.0e-12f;   // matches ND’s assert threshold
				if (e0.squaredLength() <= edgeMin2 ||
					e1.squaredLength() <= edgeMin2 ||
					e0.crossProduct(e1).squaredLength() <= areaMin2)
				{
					continue; // skip degenerate
				}

				// Legacy store if you need it elsewhere
				addPoly(v, p);

				// Feed Newton soup: (x,y,z) tightly packed, stride=sizeof(Vector3)
				meshBuilder.AddFace(reinterpret_cast<const ndFloat32*>(&v[0].x), static_cast<ndInt32>(sizeof(Ogre::Vector3)), 3, static_cast<ndInt32>(id)); // or use p if you prefer unique face IDs
			}

			delete[] vecVertices;

			finish(optimize);

			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}
#else
		OgreNewt::CollisionPrimitives::TreeCollision::TreeCollision(
			const OgreNewt::World* world,
			int numVertices, int numIndices,
			const float* vertices, const int* indices,
			bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			if (m_col)
				m_col->Release();

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			for (int i = 0; i + 2 < numIndices; i += 3)
			{
				Ogre::Vector3 a(vertices[indices[i] * 3 + 0],
					vertices[indices[i] * 3 + 1],
					vertices[indices[i] * 3 + 2]);
				Ogre::Vector3 b(vertices[indices[i + 1] * 3 + 0],
					vertices[indices[i + 1] * 3 + 1],
					vertices[indices[i + 1] * 3 + 2]);
				Ogre::Vector3 c(vertices[indices[i + 2] * 3 + 0],
					vertices[indices[i + 2] * 3 + 1],
					vertices[indices[i + 2] * 3 + 2]);

				// Gentle extrusion for degenerate faces
				Ogre::Vector3 e1 = b - a;
				Ogre::Vector3 e2 = c - a;
				Ogre::Vector3 n = e1.crossProduct(e2);
				Ogre::Real area2 = n.squaredLength();
				if (area2 < 1e-16f)
				{
					if (n.isZeroLength())
						n = Ogre::Vector3::UNIT_Y;
					else
						n.normalise();
					const Ogre::Real EPS = 1e-4f;
					a += n * EPS;
					b += n * EPS * 0.5f;
					c += n * EPS * 0.25f;
				}

				ndVector face[3];
				if (fw == FW_DEFAULT)
				{
					face[0] = ndVector(a.x, a.y, a.z, 1.0f);
					face[1] = ndVector(b.x, b.y, b.z, 1.0f);
					face[2] = ndVector(c.x, c.y, c.z, 1.0f);
				}
				else
				{
					face[0] = ndVector(a.x, a.y, a.z, 1.0f);
					face[1] = ndVector(c.x, c.y, c.z, 1.0f);
					face[2] = ndVector(b.x, b.y, b.z, 1.0f);
				}

				meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
			}

			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

#endif

#if 0
		TreeCollision::TreeCollision(const World* world, int numVertices, Ogre::Vector3* vertices, Ogre::v1::IndexData* indexData, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;
			start(id);

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			if (!vertices || !indexData || !indexData->indexBuffer)
			{
				finish(optimize);
				meshBuilder.End(optimize);
				m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
				m_col = m_shapeInstance->GetShape();
				return;
			}

			Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
			const size_t indexCount = indexData->indexCount;
			if (indexCount < 3)
			{
				finish(optimize);
				meshBuilder.End(optimize);
				m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
				m_col = m_shapeInstance->GetShape();
				return;
			}

			const size_t triCount = indexCount / 3;
			const size_t indexSize = ibuf->getIndexSize();
			assert(indexSize == 2 || indexSize == 4);

			void* indexDataRaw = ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);

			auto readIndex16 = [&](size_t i) -> uint32_t {
				return static_cast<uint32_t>(reinterpret_cast<uint16_t*>(indexDataRaw)[i]);
				};
			auto readIndex32 = [&](size_t i) -> uint32_t {
				return static_cast<uint32_t>(reinterpret_cast<uint32_t*>(indexDataRaw)[i]);
				};

			const bool use32 = (indexSize == 4);
			for (size_t t = 0; t < triCount; ++t)
			{
				const uint32_t i0 = use32 ? readIndex32(t * 3 + 0) : readIndex16(t * 3 + 0);
				const uint32_t i1 = use32 ? readIndex32(t * 3 + 1) : readIndex16(t * 3 + 1);
				const uint32_t i2 = use32 ? readIndex32(t * 3 + 2) : readIndex16(t * 3 + 2);

				if (i0 >= static_cast<uint32_t>(numVertices) ||
					i1 >= static_cast<uint32_t>(numVertices) ||
					i2 >= static_cast<uint32_t>(numVertices))
					continue;

				Ogre::Vector3 v[3];
				v[0] = vertices[i0];
				v[1] = vertices[i1];
				v[2] = vertices[i2];

				if (fw == FW_REVERSE)
					std::swap(v[1], v[2]);

				// Skip degenerate triangles
				const Ogre::Vector3 e0 = v[1] - v[0];
				const Ogre::Vector3 e1 = v[2] - v[0];
				const float edgeMin2 = 1.0e-20f;
				const float areaMin2 = 1.0e-12f;
				if (e0.squaredLength() <= edgeMin2 ||
					e1.squaredLength() <= edgeMin2 ||
					e0.crossProduct(e1).squaredLength() <= areaMin2)
				{
					continue;
				}

				addPoly(v, static_cast<int>(t));

				meshBuilder.AddFace(reinterpret_cast<const ndFloat32*>(&v[0].x), static_cast<ndInt32>(sizeof(Ogre::Vector3)), 3, static_cast<ndInt32>(id));
			}

			ibuf->unlock();

			finish(optimize);

			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}
#else
		TreeCollision::TreeCollision(
			const World* world,
			int numVertices,
			Ogre::Vector3* vertices,
			Ogre::v1::IndexData* indexData,
			bool optimize,
			unsigned int id,
			FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;
			start(id);

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			// Guard: invalid input
			if (!vertices || !indexData || !indexData->indexBuffer)
			{
				finish(optimize);
				meshBuilder.End(optimize);
				m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
				m_col = m_shapeInstance->GetShape();
				return;
			}

			Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
			const size_t indexCount = indexData->indexCount;
			if (indexCount < 3)
			{
				finish(optimize);
				meshBuilder.End(optimize);
				m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
				m_col = m_shapeInstance->GetShape();
				return;
			}

			const size_t triCount = indexCount / 3;
			const size_t indexSize = ibuf->getIndexSize();
			assert(indexSize == 2 || indexSize == 4);

			void* indexDataRaw = ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);

			auto readIndex16 = [&](size_t i) -> uint32_t {
				return static_cast<uint32_t>(reinterpret_cast<uint16_t*>(indexDataRaw)[i]);
				};
			auto readIndex32 = [&](size_t i) -> uint32_t {
				return static_cast<uint32_t>(reinterpret_cast<uint32_t*>(indexDataRaw)[i]);
				};

			const bool use32 = (indexSize == 4);

			for (size_t t = 0; t < triCount; ++t)
			{
				const uint32_t i0 = use32 ? readIndex32(t * 3 + 0) : readIndex16(t * 3 + 0);
				const uint32_t i1 = use32 ? readIndex32(t * 3 + 1) : readIndex16(t * 3 + 1);
				const uint32_t i2 = use32 ? readIndex32(t * 3 + 2) : readIndex16(t * 3 + 2);

				if (i0 >= static_cast<uint32_t>(numVertices) ||
					i1 >= static_cast<uint32_t>(numVertices) ||
					i2 >= static_cast<uint32_t>(numVertices))
					continue;

				Ogre::Vector3 v[3];
				v[0] = vertices[i0];
				v[1] = vertices[i1];
				v[2] = vertices[i2];

				if (fw == FW_REVERSE)
					std::swap(v[1], v[2]);

				// --- Gentle extrusion for degenerate or nearly-flat faces ---
				Ogre::Vector3 e0 = v[1] - v[0];
				Ogre::Vector3 e1 = v[2] - v[0];
				Ogre::Vector3 n = e0.crossProduct(e1);
				Ogre::Real area2 = n.squaredLength();
				if (area2 < 1e-16f)
				{
					if (n.isZeroLength())
						n = Ogre::Vector3::UNIT_Y;
					else
						n.normalise();
					const Ogre::Real EPS = 1e-4f;
					v[0] += n * EPS;
					v[1] += n * EPS * 0.5f;
					v[2] += n * EPS * 0.25f;
				}
				// ------------------------------------------------------------

				addPoly(v, static_cast<int>(t));

				meshBuilder.AddFace(reinterpret_cast<const ndFloat32*>(&v[0].x),
					static_cast<ndInt32>(sizeof(Ogre::Vector3)),
					3,
					static_cast<ndInt32>(id));
			}

			ibuf->unlock();

			finish(optimize);

			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}
#endif

		void TreeCollision::start(unsigned int id)
		{
			m_categoryId = id;
			m_faceCount = 0;
		}

		void TreeCollision::addPoly(Ogre::Vector3* polys, unsigned int ID)
		{
			// This would need to be implemented with a persistent ndPolygonSoupBuilder
			// For now, use the constructor-based approach
		}

		void TreeCollision::finish(bool optimize)
		{
			// Finalization happens in constructor
		}

		void TreeCollision::setFaceId(unsigned int faceId)
		{
			m_categoryId = faceId;
		}

		int TreeCollision::CountFaces(void* const context, const ndFloat32* const polygon,
			ndInt32 strideInBytes, const ndInt32* const indexArray, ndInt32 indexCount)
		{
			TreeCollision* const me = (TreeCollision*)context;
			me->m_faceCount++;
			return 1;
		}

		int TreeCollision::MarkFaces(void* const context, const ndFloat32* const polygon,
			ndInt32 strideInBytes, const ndInt32* const indexArray, ndInt32 indexCount)
		{
			TreeCollision* const me = (TreeCollision*)context;
			me->m_faceCount++;
			return 1;
		}

		void TreeCollision::setRayCastCallbackActive(bool active, ndShape* col)
		{
			// Newton 4.0 handles raycasting differently
		}

		HeightField::HeightField(const World* world) : Collision(world), m_faceCount(0), m_categoryId(0)
		{
		}

		HeightField::HeightField(const World* world, int width, int height, Ogre::Real* elevationMap,
			Ogre::Real verticleScale, Ogre::Real horizontalScaleX, Ogre::Real horizontalScaleZ,
			const Ogre::Vector3& position, const Ogre::Quaternion& orientation, unsigned int shapeID)
			: Collision(world), m_faceCount(0), m_categoryId(shapeID)
		{
			if (m_col)
				m_col->Release();

			// Create Newton 4.0 heightfield
			ndShapeHeightfield* heightfield = new ndShapeHeightfield(
				width,
				height,
				ndShapeHeightfield::m_normalDiagonals,
				ndFloat32(horizontalScaleX),
				ndFloat32(horizontalScaleZ)
			);

			m_shapeInstance = new ndShapeInstance(heightfield);
			m_col = m_shapeInstance->GetShape();

			// Copy elevation data
			ndArray<ndFloat32>& elevations = heightfield->GetElevationMap();
			for (int i = 0; i < width * height; i++)
			{
				elevations[i] = ndFloat32(elevationMap[i] * verticleScale);
			}

			// Update AABB after setting elevation data
			heightfield->UpdateElevationMapAabb();

			m_col = heightfield;
		}

		void HeightField::setFaceId(unsigned int faceId)
		{
			m_categoryId = faceId;
		}

		void HeightField::setRayCastCallbackActive(bool active, ndShape* col)
		{
			// Newton 4.0 handles raycasting differently
		}

		CompoundCollision::CompoundCollision(const World* world) : Collision(world)
		{
		}

		CompoundCollision::CompoundCollision(const World* world, std::vector<OgreNewt::CollisionPtr> col_array,
			unsigned int id) : Collision(world)
		{
			if (m_col)
				m_col->Release();

			ndShapeCompound* compound = new ndShapeCompound();
			compound->BeginAddRemove();

			for (const OgreNewt::CollisionPtr& col : col_array)
			{
				if (col && col->getNewtonCollision())
				{
					ndShapeInstance* instance = new ndShapeInstance(col->getNewtonCollision());
					compound->AddCollision(instance);
				}
			}

			compound->EndAddRemove();

			m_shapeInstance = new ndShapeInstance(compound);
			m_col = m_shapeInstance->GetShape();
		}

		Pyramid::Pyramid(const World* world) : ConvexCollision(world)
		{
		}

		Pyramid::Pyramid(const World* world, const Ogre::Vector3& size, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance)
			: ConvexCollision(world)
		{
			// Create pyramid vertices
			ndFloat32 vertices[15];
			unsigned short idx = 0;

			for (int ix = -1; ix <= 1; ix += 2)
			{
				for (int iz = -1; iz <= 1; iz += 2)
				{
					vertices[idx++] = (size.x / 2.0f) * ix;
					vertices[idx++] = -(size.y / 3.0f);
					vertices[idx++] = (size.z / 2.0f) * iz;
				}
			}

			vertices[idx++] = 0.0f;
			vertices[idx++] = (size.y * 2.0f / 3.0f);
			vertices[idx++] = 0.0f;

			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeConvexHull(5, sizeof(ndFloat32) * 3, ndFloat32(tolerance), vertices));
			m_col = m_shapeInstance->GetShape();
		}

	}   // end namespace CollisionPrimitives
}   // end namespace OgreNewt
