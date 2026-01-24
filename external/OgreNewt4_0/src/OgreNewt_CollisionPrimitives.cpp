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

		// --------------------------------
		// Box
		// --------------------------------

		Box::Box(const World* world) : ConvexCollision(world)
		{
		}

		Box::Box(const World* world, const Ogre::Vector3& size, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos)
			: ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeBox(ndFloat32(size.x), ndFloat32(size.y), ndFloat32(size.z)));

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			m_col = m_shapeInstance->GetShape();
		}

		// --------------------------------
		// Ellipsoid / Sphere
		// --------------------------------

		Ellipsoid::Ellipsoid(const World* world) : ConvexCollision(world)
		{
		}

		Ellipsoid::Ellipsoid(const World* world, const Ogre::Vector3& size, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);

			if ((size.x == size.y) && (size.y == size.z))
			{
				// Uniform sphere: use size.x as radius (same as ND3)
				m_shapeInstance = new ndShapeInstance(new ndShapeSphere(ndFloat32(size.x)));

				m_shapeInstance->SetLocalMatrix(localM);
			}
			else
			{
				const Ogre::Real radius = std::min(std::min(size.x, size.y), size.z);
				const Ogre::Vector3 scale = Ogre::Vector3(size.x / radius, size.y / radius, size.z / radius);

				m_shapeInstance = new ndShapeInstance(new ndShapeSphere(ndFloat32(radius)));

				m_shapeInstance->SetLocalMatrix(localM);
				m_shapeInstance->SetScale(ndVector(ndFloat32(scale.x), ndFloat32(scale.y), ndFloat32(scale.z), ndFloat32(1.0f)));
			}

			m_col = m_shapeInstance->GetShape();
		}


		// --------------------------------
		// Cylinder
		// --------------------------------

		Cylinder::Cylinder(const World* world)
			: ConvexCollision(world)
		{
		}

		Cylinder::Cylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeCylinder(ndFloat32(radius), ndFloat32(radius), ndFloat32(height)));

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			m_col = m_shapeInstance->GetShape();
		}

		// --------------------------------
		// Capsule
		// --------------------------------

		Capsule::Capsule(const World* world) : ConvexCollision(world)
		{
		}

		Capsule::Capsule(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos)
			: ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeCapsule(ndFloat32(radius), ndFloat32(radius), ndFloat32(height - (radius * 2.0f))));

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			m_col = m_shapeInstance->GetShape();
		}

		Capsule::Capsule(const World* world, const Ogre::Vector3& size, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeCapsule(ndFloat32(size.x), ndFloat32(size.z), ndFloat32(size.y)));

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			m_col = m_shapeInstance->GetShape();
		}

		// --------------------------------
		// Cone
		// --------------------------------

		Cone::Cone(const World* world)
			: ConvexCollision(world)
		{
		}

		Cone::Cone(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeCone(ndFloat32(radius), ndFloat32(height)));

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			m_col = m_shapeInstance->GetShape();
		}

		// --------------------------------
		// ChamferCylinder
		// --------------------------------

		ChamferCylinder::ChamferCylinder(const World* world) : ConvexCollision(world)
		{
		}

		ChamferCylinder::ChamferCylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos)
			: ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeChamferCylinder(ndFloat32(radius), ndFloat32(height)));

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			m_col = m_shapeInstance->GetShape();
		}

		ConvexHull::ConvexHull(const World* world) : ConvexCollision(world)
		{
		}

		ConvexHull::ConvexHull(const World* world, Ogre::v1::Entity* obj, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance, const Ogre::Vector3& forceScale)
			: ConvexCollision(world)
		{
			OGRE_ASSERT_LOW(obj);
			Ogre::Vector3 scale(1, 1, 1);

			if (Ogre::Node* node = obj->getParentNode())
			{
				scale = node->_getDerivedScaleUpdated();
			}

			if (forceScale != Ogre::Vector3::ZERO)
			{
				scale = forceScale;
			}

			Ogre::v1::MeshPtr mesh = obj->getMesh();
			if (mesh.isNull())
			{
				m_shapeInstance = nullptr;
				m_col = nullptr;
				return;
			}

			const unsigned short subCount = mesh->getNumSubMeshes();
			size_t totalVerts = 0;

			// Pass 1: count vertices (shared once)
			bool sharedAdded = false;
			for (unsigned short i = 0; i < subCount; ++i)
			{
				Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);

				if (!sub)
				{
					continue;
				}

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
				{
					totalVerts += vdata->vertexCount;
				}
			}
			if (totalVerts < 4)
			{
				m_shapeInstance = nullptr;
				m_col = nullptr;
				return;
			}

			// Pass 2: read positions -> temp CPU array of Ogre::Vector3
			std::vector<Ogre::Vector3> verts;
			verts.reserve(totalVerts);
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

				if (!vdata)
				{
					continue;
				}

				const Ogre::v1::VertexElement* posElem = vdata->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				if (!posElem)
				{
					continue;
				}

				const size_t start = vdata->vertexStart;
				const size_t vCount = vdata->vertexCount;

				Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vdata->vertexBufferBinding->getBuffer(posElem->getSource());
				if (!vbuf)
				{
					continue;
				}

				unsigned char* base = static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
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
				for (const auto& v : verts)
				{
					if (IsFiniteVec3(v))
					{
						aabb.merge(v);
					}
				}

				const Ogre::Real diag = aabb.isNull() ? Ogre::Real(1) : aabb.getSize().length();

				if (tolerance <= 0)
					tolerance = std::max<Ogre::Real>(diag * Ogre::Real(1e-4), 1e-6f);

				// dedup + push
				for (const auto& v : verts)
				{
					if (!IsFiniteVec3(v))
						continue;

					bool dup = false;
					// cheap linear dedup (ok for typical mesh vertex counts for hulls)
					for (size_t i = 0; i + 2 < points.size(); i += 3)
					{
						const float dx = points[i + 0] - v.x;
						const float dy = points[i + 1] - v.y;
						const float dz = points[i + 2] - v.z;
						if (dx * dx + dy * dy + dz * dz <= dedupEps2)
						{
							dup = true;
							break;
						}
					}
					if (!dup)
					{
						points.push_back(v.x);
						points.push_back(v.y);
						points.push_back(v.z);
					}
				}
			}

			if (points.size() < 12)
			{ // < 4 points
				m_shapeInstance = nullptr;
				m_col = nullptr;
				return;
			}

			if (m_col)
				m_col->Release();

			const ndInt32 count = ndInt32(points.size() / 3);
			const ndInt32 stride = ndInt32(sizeof(ndFloat32) * 3); // tightly packed floats

			m_shapeInstance = new ndShapeInstance(new ndShapeConvexHull(count, stride, ndFloat32(tolerance), reinterpret_cast<const ndFloat32*>(&points[0])));

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			m_col = m_shapeInstance->GetShape();
		}

		ConvexHull::ConvexHull(const World* world, Ogre::Item* item, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos,
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
					if (!sub)
					{
						continue;
					}

					const Ogre::VertexArrayObjectArray& vaos = sub->mVao[0]; // VpNormal
					if (vaos.empty())
					{
						continue;
					}

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
								Ogre::Vector3 v(Ogre::Bitwise::halfToFloat(p[0]), Ogre::Bitwise::halfToFloat(p[1]), Ogre::Bitwise::halfToFloat(p[2]));
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
			if (verts.size() < 4)
			{
				if (m_col)
				{
					m_col->Release();
					m_shapeInstance = nullptr;
					m_col = nullptr;
					return;
				}
			}

			// Clean & convert to contiguous ndFloat32 buffer
			const float dedupEps2 = 1e-16f;

			// Optional: adaptive tolerance
			Ogre::AxisAlignedBox aabb;
			for (const auto& v : verts)
			{
				if (IsFiniteVec3(v))
				{
					aabb.merge(v);
				}
			}
			const Ogre::Real diag = aabb.isNull() ? Ogre::Real(1) : aabb.getSize().length();

			if (tolerance <= 0)
			{
				tolerance = std::max<Ogre::Real>(diag * Ogre::Real(1e-4), 1e-6f);
			}

			std::vector<ndFloat32> points; points.reserve(verts.size() * 3);
			for (const auto& v : verts)
			{
				if (!IsFiniteVec3(v))
				{
					continue;
				}

				bool dup = false;
				for (size_t i = 0; i + 2 < points.size(); i += 3)
				{
					const float dx = points[i + 0] - v.x;
					const float dy = points[i + 1] - v.y;
					const float dz = points[i + 2] - v.z;
					if (dx * dx + dy * dy + dz * dz <= dedupEps2)
					{
						dup = true;
						break;
					}
				}
				if (!dup)
				{
					points.push_back(v.x);
					points.push_back(v.y);
					points.push_back(v.z);
				}
			}

			if (points.size() < 12)
			{
				if (m_col)
				{
					m_col->Release();
					m_shapeInstance = nullptr;
					m_col = nullptr;
					return;
				}
			}

			if (m_col)
				m_col->Release();

			const ndInt32 count = ndInt32(points.size() / 3);
			const ndInt32 stride = ndInt32(sizeof(ndFloat32) * 3); // tightly packed XYZ floats

			m_shapeInstance = new ndShapeInstance(new ndShapeConvexHull(count, stride, ndFloat32(tolerance), reinterpret_cast<const ndFloat32*>(&points[0])));
			m_col = m_shapeInstance->GetShape();

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);
		}

		ConvexHull::ConvexHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance)
			: ConvexCollision(world)
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

		OgreNewt::CollisionPrimitives::TreeCollision::TreeCollision(const OgreNewt::World* world, Ogre::v1::Entity* obj, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			Ogre::Vector3 scale(1, 1, 1);
			if (Ogre::Node* node = obj->getParentNode())
				scale = node->_getDerivedScaleUpdated();

			// Flip winding if mirrored scale
			FaceWinding localFw = fw;
			if (scale.x * scale.y * scale.z < 0.0f)
			{
				localFw = (fw == FW_DEFAULT) ? FW_REVERSE : FW_DEFAULT;
			}

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

				// --- Resolve vertex data (shared counted only once per sub that uses it) ---
				Ogre::v1::VertexData* vData = sub->useSharedVertices ? mesh->sharedVertexData[0] : sub->vertexData[0];
				if (!vData)
					continue;

				const Ogre::v1::VertexElement* posElem = vData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				if (!posElem)
					continue;

				// Respect vertexStart when addressing vertices
				const size_t vStart = vData->vertexStart;
				const size_t vCount = vData->vertexCount;

				Ogre::v1::HardwareVertexBufferSharedPtr vBuf =
					vData->vertexBufferBinding->getBuffer(posElem->getSource());
				if (!vBuf) continue;

				unsigned char* vBase = static_cast<unsigned char*>(vBuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
				const size_t vStride = vBuf->getVertexSize();

				// --- Resolve index data (respect indexStart / indexCount) ---
				Ogre::v1::IndexData* iData = sub->indexData[0];
				if (!iData)
				{
					vBuf->unlock();
					continue;
				}

				Ogre::v1::HardwareIndexBufferSharedPtr iBuf = iData->indexBuffer;
				if (iBuf.isNull())
				{
					vBuf->unlock();
					continue;
				}

				const bool use32 = (iBuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);
				const size_t iStart = iData->indexStart;   // first index to read
				const size_t iCount = iData->indexCount;   // how many indices to read

				if (iCount < 3)
				{
					vBuf->unlock();
					continue;
				}

				unsigned char* iBase = static_cast<unsigned char*>(iBuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				const size_t indexSize = iBuf->getIndexSize();
				// Safety: indexSize must match use32
				// assert((use32 && indexSize==4) || (!use32 && indexSize==2));

				const size_t triCount = iCount / 3;

				// --- Iterate triangles via index buffer ---
				for (size_t t = 0; t < triCount; ++t)
				{
					// Read 3 indices with indexStart offset
					unsigned int idx[3];
					if (use32)
					{
						const unsigned int* src = reinterpret_cast<unsigned int*>(iBase);
						idx[0] = src[iStart + t * 3 + 0];
						idx[1] = src[iStart + t * 3 + 1];
						idx[2] = src[iStart + t * 3 + 2];
					}
					else
					{
						const unsigned short* src = reinterpret_cast<unsigned short*>(iBase);
						idx[0] = src[iStart + t * 3 + 0];
						idx[1] = src[iStart + t * 3 + 1];
						idx[2] = src[iStart + t * 3 + 2];
					}

					// Guard index range against this submesh's vertex window (vStart..vStart+vCount-1)
					if (idx[0] < vStart || idx[0] >= vStart + vCount ||
						idx[1] < vStart || idx[1] >= vStart + vCount ||
						idx[2] < vStart || idx[2] >= vStart + vCount)
					{
						continue;
					}

					// Fetch positions respecting vertexStart
					Ogre::Vector3 vtx[3];
					for (int j = 0; j < 3; ++j)
					{
						const size_t vIdx = idx[j] - vStart; // index inside this vertex window
						unsigned char* ptr = vBase + (vIdx * vStride);
						float* pos = nullptr;
						posElem->baseVertexPointerToElement(ptr, &pos);
						vtx[j] = Ogre::Vector3(pos[0], pos[1], pos[2]) * scale;
					}

					// ---- ND4 safety: skip degenerate edges (any edge too small) ----
					const Ogre::Real MIN_EDGE2 = 1.0e-12f;

					Ogre::Vector3 e0 = vtx[1] - vtx[0];
					Ogre::Vector3 e1 = vtx[2] - vtx[1];
					Ogre::Vector3 e2 = vtx[0] - vtx[2];

					if (e0.squaredLength() < MIN_EDGE2 ||
						e1.squaredLength() < MIN_EDGE2 ||
						e2.squaredLength() < MIN_EDGE2)
					{
						// Completely degenerate or almost-degenerate triangle, skip it
						continue;
					}

					// ---- Optional: your "gentle extrusion" area fix can stay here ----
					Ogre::Vector3 n = e0.crossProduct(e2);
					if (n.squaredLength() < 1e-16f)
					{
						if (n.isZeroLength())
							n = Ogre::Vector3::UNIT_Y;
						else
							n.normalise();

						const Ogre::Real EPS = 1e-4f;
						vtx[0] += n * EPS;
						vtx[1] += n * EPS * 0.5f;
						vtx[2] += n * EPS * 0.25f;
					}

					ndVector face[3];
					if (localFw == FW_DEFAULT)
					{
						face[0] = ndVector(vtx[0].x, vtx[0].y, vtx[0].z, 1.0f);
						face[1] = ndVector(vtx[1].x, vtx[1].y, vtx[1].z, 1.0f);
						face[2] = ndVector(vtx[2].x, vtx[2].y, vtx[2].z, 1.0f);
					}
					else
					{
						face[0] = ndVector(vtx[0].x, vtx[0].y, vtx[0].z, 1.0f);
						face[1] = ndVector(vtx[2].x, vtx[2].y, vtx[2].z, 1.0f);
						face[2] = ndVector(vtx[1].x, vtx[1].y, vtx[1].z, 1.0f);
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

		TreeCollision::TreeCollision(const World* world, Ogre::Item* item, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;
			start(id);

			Ogre::MeshPtr mesh = item->getMesh();
			if (mesh.isNull())
			{
				ndPolygonSoupBuilder dummy;
				dummy.Begin(); dummy.End(optimize);
				m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(dummy));
				m_col = m_shapeInstance->GetShape();
				return;
			}

			Ogre::Vector3 scale(1, 1, 1);
			if (Ogre::Node* node = item->getParentNode())
				scale = node->_getDerivedScaleUpdated();

			FaceWinding localFw = fw;
			if (scale.x * scale.y * scale.z < 0.0f)
				localFw = (fw == FW_DEFAULT) ? FW_REVERSE : FW_DEFAULT;

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			//--------------------------------------------------------------------
			// Iterate submeshes
			//--------------------------------------------------------------------
			for (Ogre::SubMesh* subMesh : mesh->getSubMeshes())
			{
				if (!subMesh) continue;
				const Ogre::VertexArrayObjectArray& vaos = subMesh->mVao[0];
				if (vaos.empty()) continue;

				Ogre::VertexArrayObject* vao = vaos[0];
				Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
				if (!indexBuffer) continue;

				//----------------------------------------------------------------
				// 1) Read vertices (async)
				//----------------------------------------------------------------
				Ogre::VertexArrayObject::ReadRequestsVec vReqs;
				vReqs.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
				vao->readRequests(vReqs);
				vao->mapAsyncTickets(vReqs);

				const Ogre::VertexBufferPacked* vb = vReqs[0].vertexBuffer;
				const size_t vCount = vb ? vb->getNumElements() : 0;
				std::vector<Ogre::Vector3> verts(vCount);

				if (vReqs[0].type == Ogre::VET_HALF4)
				{
					for (size_t i = 0; i < vCount; ++i)
					{
						const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(vReqs[0].data);
						verts[i] = Ogre::Vector3(Ogre::Bitwise::halfToFloat(p[0]), Ogre::Bitwise::halfToFloat(p[1]), Ogre::Bitwise::halfToFloat(p[2])) * scale;
						vReqs[0].data += vb->getBytesPerElement();
					}
				}
				else if (vReqs[0].type == Ogre::VET_FLOAT3)
				{
					for (size_t i = 0; i < vCount; ++i)
					{
						const float* p = reinterpret_cast<const float*>(vReqs[0].data);
						verts[i] = Ogre::Vector3(p[0], p[1], p[2]) * scale;
						vReqs[0].data += vb->getBytesPerElement();
					}
				}
				else
				{
					vao->unmapAsyncTickets(vReqs);
					continue;
				}
				vao->unmapAsyncTickets(vReqs);

				//----------------------------------------------------------------
				// 2) Read index buffer (async)
				//----------------------------------------------------------------
				Ogre::IndexBufferPacked::IndexType idxType = indexBuffer->getIndexType();
				Ogre::AsyncTicketPtr ticket = indexBuffer->readRequest(0, indexBuffer->getNumElements());
				const void* idxData = ticket->map();
				const size_t idxCount = indexBuffer->getNumElements();

				//----------------------------------------------------------------
				// Helper to add a triangle
				//----------------------------------------------------------------
				auto emitTri = [&](uint32_t a, uint32_t b, uint32_t c)
					{
						if (a >= verts.size() || b >= verts.size() || c >= verts.size())
							return;

						Ogre::Vector3 v0 = verts[a];
						Ogre::Vector3 v1 = verts[b];
						Ogre::Vector3 v2 = verts[c];

						// ---- ND4 safety: skip degenerate edges (any edge too small) ----
						const Ogre::Real MIN_EDGE2 = 1.0e-12f;

						Ogre::Vector3 e0 = v1 - v0;
						Ogre::Vector3 e1 = v2 - v1;
						Ogre::Vector3 e2 = v0 - v2;

						if (e0.squaredLength() < MIN_EDGE2 ||
							e1.squaredLength() < MIN_EDGE2 ||
							e2.squaredLength() < MIN_EDGE2)
						{
							// Completely degenerate or almost-degenerate triangle, skip it
							return;
						}

						// ---- Optional: gentle extrusion for nearly-zero area ----
						Ogre::Vector3 n = e0.crossProduct(e2);
						if (n.squaredLength() < 1e-16f)
						{
							if (n.isZeroLength())
								n = Ogre::Vector3::UNIT_Y;
							else
								n.normalise();

							const Ogre::Real EPS = 1.0e-4f;
							v0 += n * EPS;
							v1 += n * EPS * 0.5f;
							v2 += n * EPS * 0.25f;
						}

						ndVector face[3];
						if (localFw == FW_DEFAULT)
						{
							face[0] = ndVector(v0.x, v0.y, v0.z, 1.0f);
							face[1] = ndVector(v1.x, v1.y, v1.z, 1.0f);
							face[2] = ndVector(v2.x, v2.y, v2.z, 1.0f);
						}
						else
						{
							face[0] = ndVector(v0.x, v0.y, v0.z, 1.0f);
							face[1] = ndVector(v2.x, v2.y, v2.z, 1.0f);
							face[2] = ndVector(v1.x, v1.y, v1.z, 1.0f);
						}

						meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
					};

				//----------------------------------------------------------------
				// 3) Iterate triangles
				//----------------------------------------------------------------
				if (idxType == Ogre::IndexBufferPacked::IT_16BIT)
				{
					const uint16_t* idx = reinterpret_cast<const uint16_t*>(idxData);
					for (size_t k = 0; k + 2 < idxCount; k += 3)
						emitTri(idx[k], idx[k + 1], idx[k + 2]);
				}
				else
				{
					const uint32_t* idx = reinterpret_cast<const uint32_t*>(idxData);
					for (size_t k = 0; k + 2 < idxCount; k += 3)
						emitTri(idx[k], idx[k + 1], idx[k + 2]);
				}

				ticket->unmap();
			}

			//--------------------------------------------------------------------
			// Finish
			//--------------------------------------------------------------------
			finish(optimize);
			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

		OgreNewt::CollisionPrimitives::TreeCollision::TreeCollision(const OgreNewt::World* world, int numVertices, int numIndices,
			const float* vertices, const int* indices, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			if (m_col)
				m_col->Release();

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			// ----------------------------------------------------------------
			// 1. Basic validation
			// ----------------------------------------------------------------
			if (!vertices || !indices || numVertices <= 0 || numIndices < 3)
			{
				meshBuilder.End(optimize);
				m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
				m_col = m_shapeInstance->GetShape();
				return;
			}

			FaceWinding localFw = fw;

			const int triCount = numIndices / 3;

			// ----------------------------------------------------------------
			// 2. Build triangles from index triplets
			// ----------------------------------------------------------------
			for (int i = 0; i < triCount; ++i)
			{
				const int i0 = indices[i * 3 + 0];
				const int i1 = indices[i * 3 + 1];
				const int i2 = indices[i * 3 + 2];

				// Skip out-of-range or degenerate indices
				if (i0 < 0 || i1 < 0 || i2 < 0 ||
					i0 >= numVertices || i1 >= numVertices || i2 >= numVertices)
				{
					continue;
				}

				Ogre::Vector3 a(vertices[i0 * 3 + 0], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
				Ogre::Vector3 b(vertices[i1 * 3 + 0], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
				Ogre::Vector3 c(vertices[i2 * 3 + 0], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);

				// ----------------------------------------------------------------
				// 3. ND4 safety: skip triangles with tiny edges (like ndPolygonSoupBuilder)
				// ----------------------------------------------------------------
				const Ogre::Real MIN_EDGE2 = 1.0e-12f;

				Ogre::Vector3 e0 = b - a;
				Ogre::Vector3 e1 = c - b;
				Ogre::Vector3 e2 = a - c;

				if (e0.squaredLength() < MIN_EDGE2 ||
					e1.squaredLength() < MIN_EDGE2 ||
					e2.squaredLength() < MIN_EDGE2)
				{
					// Degenerate / collapsed triangle, skip to avoid ND4 assert
					continue;
				}

				// ----------------------------------------------------------------
				// 4. Gentle extrusion for nearly-zero area faces (optional)
				// ----------------------------------------------------------------
				Ogre::Vector3 n = e0.crossProduct(e2);
				const Ogre::Real area2 = n.squaredLength();

				if (area2 < 1e-16f)
				{
					if (n.isZeroLength())
						n = Ogre::Vector3::UNIT_Y;
					else
						n.normalise();

					const Ogre::Real EPS = 1.0e-4f;
					a += n * EPS;
					b += n * EPS * 0.5f;
					c += n * EPS * 0.25f;
				}

				// ----------------------------------------------------------------
				// 5. Add triangle to ND4 polygon soup
				// ----------------------------------------------------------------
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

			// ----------------------------------------------------------------
			// 6. Finalize Newton shape
			// ----------------------------------------------------------------
			meshBuilder.End(optimize);

			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

		TreeCollision::TreeCollision(const World* world, int numVertices, Ogre::Vector3* vertices,
			Ogre::v1::IndexData* indexData, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;
			start(id);

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			// --------------------------------------------------------------------
			// 1. Validate input
			// --------------------------------------------------------------------
			if (!vertices || numVertices <= 0 || !indexData || !indexData->indexBuffer)
			{
				finish(optimize);
				meshBuilder.End(optimize);
				m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
				m_col = m_shapeInstance->GetShape();
				return;
			}

			Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
			const size_t indexCount = indexData->indexCount;
			const size_t indexStart = indexData->indexStart;

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

			const bool use32 = (indexSize == 4);
			auto readIndex = [&](size_t i) -> uint32_t
				{
					return use32 ?
						static_cast<uint32_t>(reinterpret_cast<uint32_t*>(indexDataRaw)[i]) :
						static_cast<uint32_t>(reinterpret_cast<uint16_t*>(indexDataRaw)[i]);
				};

			// Optional: mirrored scale check (can be provided externally)
			FaceWinding localFw = fw;

			// --------------------------------------------------------------------
			// 2. Iterate through triangles
			// --------------------------------------------------------------------
			for (size_t t = 0; t < triCount; ++t)
			{
				const uint32_t i0 = readIndex(indexStart + t * 3 + 0);
				const uint32_t i1 = readIndex(indexStart + t * 3 + 1);
				const uint32_t i2 = readIndex(indexStart + t * 3 + 2);

				if (i0 >= static_cast<uint32_t>(numVertices) || i1 >= static_cast<uint32_t>(numVertices) || i2 >= static_cast<uint32_t>(numVertices))
					continue;

				Ogre::Vector3 vtx[3];
				vtx[0] = vertices[i0];
				vtx[1] = vertices[i1];
				vtx[2] = vertices[i2];

				if (localFw == FW_REVERSE)
					std::swap(vtx[1], vtx[2]);

				// ----------------------------------------------------------------
				// 3. ND4 safety: skip triangles with tiny edges (like ndPolygonSoupBuilder)
				// ----------------------------------------------------------------
				const Ogre::Real MIN_EDGE2 = 1.0e-12f;

				Ogre::Vector3 e0 = vtx[1] - vtx[0];
				Ogre::Vector3 e1 = vtx[2] - vtx[1];
				Ogre::Vector3 e2 = vtx[0] - vtx[2];

				if (e0.squaredLength() < MIN_EDGE2 ||
					e1.squaredLength() < MIN_EDGE2 ||
					e2.squaredLength() < MIN_EDGE2)
				{
					// Degenerate / collapsed triangle, skip to avoid ND4 assert
					continue;
				}

				// ----------------------------------------------------------------
				// 4. Degenerate-face extrusion (ND4 optimizer-safe, almost-flat tris)
				// ----------------------------------------------------------------
				Ogre::Vector3 n = e0.crossProduct(e2);

				if (n.squaredLength() < 1e-16f)
				{
					if (n.isZeroLength())
						n = Ogre::Vector3::UNIT_Y;
					else
						n.normalise();

					const Ogre::Real EPS = 1.0e-4f;
					vtx[0] += n * EPS;
					vtx[1] += n * EPS * 0.5f;
					vtx[2] += n * EPS * 0.25f;
				}

				// ----------------------------------------------------------------
				// 5. Add face to Newton soup
				// ----------------------------------------------------------------
				ndVector face[3];
				if (localFw == FW_DEFAULT)
				{
					face[0] = ndVector(vtx[0].x, vtx[0].y, vtx[0].z, 1.0f);
					face[1] = ndVector(vtx[1].x, vtx[1].y, vtx[1].z, 1.0f);
					face[2] = ndVector(vtx[2].x, vtx[2].y, vtx[2].z, 1.0f);
				}
				else
				{
					face[0] = ndVector(vtx[0].x, vtx[0].y, vtx[0].z, 1.0f);
					face[1] = ndVector(vtx[2].x, vtx[2].y, vtx[2].z, 1.0f);
					face[2] = ndVector(vtx[1].x, vtx[1].y, vtx[1].z, 1.0f);
				}

				meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
			}

			ibuf->unlock();

			// --------------------------------------------------------------------
			// 6. Finalize ND4 collision shape
			// --------------------------------------------------------------------
			finish(optimize);
			meshBuilder.End(optimize);

			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

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

		HeightField::HeightField(const World* world, int width, int height, Ogre::Real* elevationMap, Ogre::Real verticalScale, Ogre::Real horizontalScaleX, Ogre::Real horizontalScaleZ,
			const Ogre::Vector3& position, const Ogre::Quaternion& orientation, unsigned int shapeID)
			: Collision(world), m_faceCount(0), m_categoryId(shapeID)
		{
			if (m_col)
				m_col->Release();

			if (m_shapeInstance)
			{
				delete m_shapeInstance;
				m_shapeInstance = nullptr;
			}

			// 1) Build ND4 heightfield (try inverted diagonals like the demo)
			ndShapeHeightfield* heightfield = new ndShapeHeightfield(ndInt32(width), ndInt32(height), 
				ndShapeHeightfield::m_invertedDiagonals, ndFloat32(horizontalScaleX), ndFloat32(horizontalScaleZ));

			// 2) Wrap in shape instance
			m_shapeInstance = new ndShapeInstance(heightfield);
			m_col = m_shapeInstance->GetShape();

			// 3) Fill elevation map, scaled in Y
			ndArray<ndReal>& heights = heightfield->GetElevationMap();
			const int count = width * height;
			ndAssert(heights.GetCount() == count);

			for (int i = 0; i < count; ++i)
			{
				const ndFloat32 h = ndFloat32(elevationMap[i] * verticalScale);
				heights[i] = ndReal(h);
			}

			heightfield->UpdateElevationMapAabb();

			// 4) Local transform (position + orientation)
			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orientation, position, localM);
			localM.m_posit.m_w = ndFloat32(1.0f);
			m_shapeInstance->SetLocalMatrix(localM);
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

		CompoundCollision::CompoundCollision(const World* world, std::vector<OgreNewt::CollisionPtr> col_array, unsigned int id)
			: Collision(world)
		{
			if (m_col)
				m_col->Release();

			ndShapeCompound* compound = new ndShapeCompound();
			compound->BeginAddRemove();

			for (const OgreNewt::CollisionPtr& col : col_array)
			{
				if (col && col->getNewtonCollision())
				{
					// IMPORTANT: preserve local transform of the child
					ndShapeInstance* childInstSrc = col->getShapeInstance(); // new accessor
					ndShapeInstance* instance = nullptr;

					if (childInstSrc)
					{
						// Copy full instance including its local matrix
						instance = new ndShapeInstance(*childInstSrc);
					}
					else
					{
						// Fallback: at least use the raw shape (no offset)
						instance = new ndShapeInstance(col->getNewtonCollision());
					}

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

			ndMatrix localM;
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
			m_shapeInstance->SetLocalMatrix(localM);

			m_col = m_shapeInstance->GetShape();
		}

	}   // end namespace CollisionPrimitives

}   // end namespace OgreNewt
