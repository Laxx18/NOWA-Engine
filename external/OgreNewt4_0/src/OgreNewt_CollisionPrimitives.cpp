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

		// ---- TreeCollision from Ogre::v1::Entity* (ND4 feed, ND3 semantics) ----
		TreeCollision::TreeCollision(const World* world, Ogre::v1::Entity* obj, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			Ogre::Vector3 scale;

			m_categoryId = id;

			start(id);

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			Ogre::v1::MeshPtr mesh = obj->getMesh();

			// get scale, if attached to node
			Ogre::Node* node = obj->getParentNode();
			if (node)
			{
				// Here no performance issues, because a collision is not created often :)
				scale = node->_getDerivedScaleUpdated();
			}

			//find number of sub-meshes
			unsigned short sub = mesh->getNumSubMeshes();

			for (unsigned short cs = 0; cs < sub; cs++)
			{
				Ogre::v1::SubMesh* sub_mesh = mesh->getSubMesh(cs);

				//vertex data!
				Ogre::v1::VertexData* v_data;

				if (sub_mesh->useSharedVertices)
				{
					v_data = mesh->sharedVertexData[0];
				}
				else
				{
					v_data = sub_mesh->vertexData[0];
				}

				//let's find more information about the Vertices...
				Ogre::v1::VertexDeclaration* v_decl = v_data->vertexDeclaration;
				const Ogre::v1::VertexElement* p_elem = v_decl->findElementBySemantic(Ogre::VES_POSITION);

				// get pointer!
				Ogre::v1::HardwareVertexBufferSharedPtr v_sptr = v_data->vertexBufferBinding->getBuffer(p_elem->getSource());
				unsigned char* v_ptr = static_cast<unsigned char*>(v_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				//now find more about the index!!
				Ogre::v1::IndexData* i_data = sub_mesh->indexData[0];
				size_t index_count = i_data->indexCount;
				size_t poly_count = index_count / 3;

				// get pointer!
				Ogre::v1::HardwareIndexBufferSharedPtr i_sptr = i_data->indexBuffer;

				// 16 or 32 bit indices?
				bool uses32bit = (i_sptr->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);
				unsigned long* i_Longptr;
				unsigned short* i_Shortptr;


				if (uses32bit)
				{
					i_Longptr = static_cast<unsigned long*>(i_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				}
				else
				{
					i_Shortptr = static_cast<unsigned short*>(i_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
				}


				//now loop through the indices, getting polygon info!
				int i_offset = 0;

				for (size_t i = 0; i < poly_count; i++)
				{
					Ogre::Vector3 poly_verts[3];
					unsigned char* v_offset;
					float* v_Posptr;
					int idx;

					if (uses32bit)
					{
						for (int j = 0; j < 3; j++)
						{
							idx = i_Longptr[i_offset + j];        // index to first vertex!
							v_offset = v_ptr + (idx * v_sptr->getVertexSize());
							p_elem->baseVertexPointerToElement(v_offset, &v_Posptr);
							//now get vertex position from v_Posptr!
							poly_verts[j].x = *v_Posptr; v_Posptr++;
							poly_verts[j].y = *v_Posptr; v_Posptr++;
							poly_verts[j].z = *v_Posptr; v_Posptr++;

							poly_verts[j] *= scale;
						}
					}
					else
					{
						for (int j = 0; j < 3; j++)
						{
							idx = i_Shortptr[i_offset + j];       // index to first vertex!
							v_offset = v_ptr + (idx * v_sptr->getVertexSize());
							p_elem->baseVertexPointerToElement(v_offset, &v_Posptr);
							//now get vertex position from v_Posptr!

							// switch poly winding.
							poly_verts[j].x = *v_Posptr; v_Posptr++;
							poly_verts[j].y = *v_Posptr; v_Posptr++;
							poly_verts[j].z = *v_Posptr; v_Posptr++;

							poly_verts[j] *= scale;
						}
					}

					if (fw == FW_DEFAULT)
					{
						meshBuilder.AddFace(
							reinterpret_cast<const ndFloat32*>(&poly_verts[0].x),
							static_cast<ndInt32>(sizeof(Ogre::Vector3)),
							3,
							static_cast<ndInt32>(id)
						);
					}
					else
					{
						Ogre::Vector3 rev_poly_verts[3];
						rev_poly_verts[0] = poly_verts[0];
						rev_poly_verts[1] = poly_verts[2];
						rev_poly_verts[2] = poly_verts[1];

						meshBuilder.AddFace(
							reinterpret_cast<const ndFloat32*>(&rev_poly_verts[0].x),
							static_cast<ndInt32>(sizeof(Ogre::Vector3)),
							3,
							static_cast<ndInt32>(id)
						);
					}

					i_offset += 3;
				}

				//unlock the buffers!
				v_sptr->unlock();
				i_sptr->unlock();

			}

			//done!
			optimize = false;
			finish(optimize);

			// ND4: finalize soup and build BVH shape
			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

		TreeCollision::TreeCollision(const World* world, Ogre::Item* item, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;

			Ogre::Vector3 scale;
			// Start building collision tree
			start(id);

			// ND4: begin polygon soup build
			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			Ogre::MeshPtr mesh = item->getMesh();
			// Get scale, if attached to node
			Ogre::Node* node = item->getParentNode();
			if (node)
			{
				// Here no performance issues, because a collision is not created often :)
				scale = node->_getDerivedScaleUpdated();
			}

			//First, we compute the total number of vertices and indices and init the buffers.
			unsigned int numVertices = 0;

			Ogre::Mesh::SubMeshVec::const_iterator subMeshIterator = mesh->getSubMeshes().begin();

			while (subMeshIterator != mesh->getSubMeshes().end())
			{
				Ogre::SubMesh* subMesh = *subMeshIterator;
				numVertices += static_cast<unsigned int>(subMesh->mVao[0][0]->getVertexBuffers()[0]->getNumElements());

				subMeshIterator++;
			}

			unsigned int addedVertices = 0;
			unsigned int subMeshOffset = 0;

			unsigned int index = 0;

			/*
			Read submeshes
			*/
			subMeshIterator = mesh->getSubMeshes().begin();
			while (subMeshIterator != mesh->getSubMeshes().end())
			{
				Ogre::SubMesh* subMesh = *subMeshIterator;
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[0];

				if (!vaos.empty())
				{
					//Get the first LOD level 
					Ogre::VertexArrayObject* vao = vaos[0];
					bool indices32 = (vao->getIndexBuffer()->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);

					const Ogre::VertexBufferPackedVec& vertexBuffers = vao->getVertexBuffers();
					Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();

					//request async read from buffer 
					Ogre::VertexArrayObject::ReadRequestsVec requests;
					requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));

					vao->readRequests(requests);
					vao->mapAsyncTickets(requests);
					unsigned int subMeshVerticiesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());
					if (requests[0].type == Ogre::VET_HALF4)
					{
						for (size_t i = 0; i < subMeshVerticiesNum; ++i)
						{
							Ogre::Vector3 polygonVertices[3];
							for (int j = 0; j < 3; j++)
							{
								const Ogre::uint16* pos = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
								Ogre::Vector3 vec;
								vec.x = Ogre::Bitwise::halfToFloat(pos[0]);
								vec.y = Ogre::Bitwise::halfToFloat(pos[1]);
								vec.z = Ogre::Bitwise::halfToFloat(pos[2]);

								requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
								polygonVertices[j] = vec * scale;
								// Here no need, to use other indexing, just go from 0 to 2 and requests[0].data adds the index offset, to always get new fresh vertex data
							}
							if (fw == FW_DEFAULT)
							{
								addPoly(polygonVertices, index);

								// ND4: feed identical layout (x,y,z) with Vector3 stride
								meshBuilder.AddFace(
									reinterpret_cast<const ndFloat32*>(&polygonVertices[0].x),
									static_cast<ndInt32>(sizeof(Ogre::Vector3)),
									3,
									static_cast<ndInt32>(index));
							}
							else
							{
								Ogre::Vector3 revertedPolygonVertices[3];
								revertedPolygonVertices[0] = polygonVertices[0];
								revertedPolygonVertices[1] = polygonVertices[2];
								revertedPolygonVertices[2] = polygonVertices[1];

								addPoly(revertedPolygonVertices, index);

								// ND4: reversed winding, same (x,y,z) layout & stride
								meshBuilder.AddFace(
									reinterpret_cast<const ndFloat32*>(&revertedPolygonVertices[0].x),
									static_cast<ndInt32>(sizeof(Ogre::Vector3)),
									3,
									static_cast<ndInt32>(index));
							}
						}
					}
					else if (requests[0].type == Ogre::VET_FLOAT3)
					{
						for (size_t i = 0; i < subMeshVerticiesNum; ++i)
						{
							Ogre::Vector3 polygonVertices[3];
							for (int j = 0; j < 3; j++)
							{
								const float* pos = reinterpret_cast<const float*>(requests[0].data);
								Ogre::Vector3 vec;
								vec.x = *pos++;
								vec.y = *pos++;
								vec.z = *pos++;
								requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
								polygonVertices[j] = vec * scale;
							}
							if (fw == FW_DEFAULT)
							{
								addPoly(polygonVertices, index);

								// ND4: feed identical layout (x,y,z) with Vector3 stride
								meshBuilder.AddFace(
									reinterpret_cast<const ndFloat32*>(&polygonVertices[0].x),
									static_cast<ndInt32>(sizeof(Ogre::Vector3)),
									3,
									static_cast<ndInt32>(index));
							}
							else
							{
								Ogre::Vector3 revertedPolygonVertices[3];
								revertedPolygonVertices[0] = polygonVertices[0];
								revertedPolygonVertices[1] = polygonVertices[2];
								revertedPolygonVertices[2] = polygonVertices[1];

								addPoly(revertedPolygonVertices, index);

								// ND4: reversed winding, same (x,y,z) layout & stride
								meshBuilder.AddFace(
									reinterpret_cast<const ndFloat32*>(&revertedPolygonVertices[0].x),
									static_cast<ndInt32>(sizeof(Ogre::Vector3)),
									3,
									static_cast<ndInt32>(index));
							}
						}
					}
					else
					{
						throw Ogre::Exception(0, "Vertex Buffer type not recognised", "TreeCollision");
					}

					index++;
					subMeshOffset += subMeshVerticiesNum;
					vao->unmapAsyncTickets(requests);
				}
				subMeshIterator++;
			}

			// Done!
			optimize = false;
			finish(optimize);

			// ND4: finalize soup and build BVH
			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

		TreeCollision::TreeCollision(const OgreNewt::World* world, int numVertices, int numIndices, const float* vertices, const int* indices, bool optimize, unsigned int id, FaceWinding fw) : OgreNewt::Collision(world)
		{
			start(id);

			// ND4: begin polygon soup build
			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			int numPolys = numIndices / 3;

			Ogre::Vector3* vecVertices = new Ogre::Vector3[numVertices];

			for (int curVertex = 0; curVertex < numVertices; curVertex++)
			{
				vecVertices[curVertex].x = vertices[0 + curVertex * 3];
				vecVertices[curVertex].y = vertices[1 + curVertex * 3];
				vecVertices[curVertex].z = vertices[2 + curVertex * 3];
			}

			for (int poly = 0; poly < numPolys; poly++)
			{
				Ogre::Vector3 poly_verts[3];

				if (fw == FW_DEFAULT)
				{
					poly_verts[0] = vecVertices[indices[0 + poly * 3]];
					poly_verts[1] = vecVertices[indices[1 + poly * 3]];
					poly_verts[2] = vecVertices[indices[2 + poly * 3]];
				}
				else
				{
					poly_verts[0] = vecVertices[indices[0 + poly * 3]];
					poly_verts[2] = vecVertices[indices[1 + poly * 3]];
					poly_verts[1] = vecVertices[indices[2 + poly * 3]];
				}

				addPoly(poly_verts, 0);

				// ND4: same (x,y,z) layout and stride as ND3 addPoly
				meshBuilder.AddFace(
					reinterpret_cast<const ndFloat32*>(&poly_verts[0].x),
					static_cast<ndInt32>(sizeof(Ogre::Vector3)),
					3,
					static_cast<ndInt32>(0));
			}

			delete[] vecVertices;

			optimize = false;
			finish(optimize);

			// ND4: finalize soup and build BVH
			meshBuilder.End(optimize);
			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

		TreeCollision::TreeCollision(const World* world, int numVertices, Ogre::Vector3* vertices, Ogre::v1::IndexData* indexData, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;

			start(id);

			// ND4: begin polygon soup build
			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			size_t numPolys = indexData->indexCount / 3;
			Ogre::v1::HardwareIndexBufferSharedPtr hwIndexBuffer = indexData->indexBuffer;
			size_t indexSize = hwIndexBuffer->getIndexSize();
			void* indices = hwIndexBuffer->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);

			assert((indexSize == 2) || (indexSize == 4));

			if (indexSize == 2)
			{
				unsigned short* curIndex = (unsigned short*)indices;
				for (unsigned int poly = 0; poly < numPolys; poly++)
				{
					Ogre::Vector3 poly_verts[3];

					if (fw == FW_DEFAULT)
					{
						poly_verts[0] = vertices[*curIndex]; curIndex++;
						poly_verts[1] = vertices[*curIndex]; curIndex++;
						poly_verts[2] = vertices[*curIndex]; curIndex++;
					}
					else
					{
						poly_verts[0] = vertices[*curIndex]; curIndex++;
						poly_verts[2] = vertices[*curIndex]; curIndex++;
						poly_verts[1] = vertices[*curIndex]; curIndex++;
					}

					addPoly(poly_verts, 0);

					// ND4: same (x,y,z) layout and stride as ND3 addPoly
					meshBuilder.AddFace(
						reinterpret_cast<const ndFloat32*>(&poly_verts[0].x),
						static_cast<ndInt32>(sizeof(Ogre::Vector3)),
						3,
						static_cast<ndInt32>(0));
				}
			}
			else
			{
				unsigned int* curIndex = (unsigned int*)indices;
				for (unsigned int poly = 0; poly < numPolys; poly++)
				{
					Ogre::Vector3 poly_verts[3];

					if (fw == FW_DEFAULT)
					{
						poly_verts[0] = vertices[*curIndex]; curIndex++;
						poly_verts[1] = vertices[*curIndex]; curIndex++;
						poly_verts[2] = vertices[*curIndex]; curIndex++;
					}
					else
					{
						poly_verts[0] = vertices[*curIndex]; curIndex++;
						poly_verts[2] = vertices[*curIndex]; curIndex++;
						poly_verts[1] = vertices[*curIndex]; curIndex++;
					}

					addPoly(poly_verts, 0);

					// ND4: same (x,y,z) layout and stride as ND3 addPoly
					meshBuilder.AddFace(reinterpret_cast<const ndFloat32*>(&poly_verts[0].x), static_cast<ndInt32>(sizeof(Ogre::Vector3)), 3, static_cast<ndInt32>(0));
				}
			}

			hwIndexBuffer->unlock();
			optimize = false;
			finish(optimize);

			// ND4: finalize soup and build BVH
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
