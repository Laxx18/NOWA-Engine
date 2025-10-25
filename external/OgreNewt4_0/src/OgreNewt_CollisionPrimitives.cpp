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
			const Ogre::Vector3& forceScale) : ConvexCollision(world)
		{
			Ogre::Vector3 scale(1.0, 1.0, 1.0);
			Ogre::v1::MeshPtr mesh = obj->getMesh();

			Ogre::Node* node = obj->getParentNode();
			if (node)
			{
				scale = node->_getDerivedScaleUpdated();
			}
			if (forceScale != Ogre::Vector3::ZERO)
			{
				scale = forceScale;
			}

			unsigned short sub = mesh->getNumSubMeshes();
			size_t total_verts = 0;

			Ogre::v1::VertexData* v_data;
			bool addedShared = false;

			for (unsigned short i = 0; i < sub; i++)
			{
				Ogre::v1::SubMesh* sub_mesh = mesh->getSubMesh(i);
				if (sub_mesh->useSharedVertices)
				{
					if (!addedShared)
					{
						v_data = mesh->sharedVertexData[0];
						total_verts += v_data->vertexCount;
						addedShared = true;
					}
				}
				else
				{
					v_data = sub_mesh->vertexData[0];
					total_verts += v_data->vertexCount;
				}
			}

			addedShared = false;
			Ogre::Vector3* vertices = new Ogre::Vector3[total_verts];
			unsigned int offset = 0;

			for (unsigned short i = 0; i < sub; i++)
			{
				Ogre::v1::SubMesh* sub_mesh = mesh->getSubMesh(i);
				Ogre::v1::VertexDeclaration* v_decl;
				const Ogre::v1::VertexElement* p_elem;
				float* v_Posptr;
				size_t v_count;

				v_data = nullptr;

				if (sub_mesh->useSharedVertices)
				{
					if (!addedShared)
					{
						v_data = mesh->sharedVertexData[0];
						v_count = v_data->vertexCount;
						v_decl = v_data->vertexDeclaration;
						p_elem = v_decl->findElementBySemantic(Ogre::VES_POSITION);
						addedShared = true;
					}
				}
				else
				{
					v_data = sub_mesh->vertexData[0];
					v_count = v_data->vertexCount;
					v_decl = v_data->vertexDeclaration;
					p_elem = v_decl->findElementBySemantic(Ogre::VES_POSITION);
				}

				if (v_data)
				{
					size_t start = v_data->vertexStart;
					Ogre::v1::HardwareVertexBufferSharedPtr v_sptr = v_data->vertexBufferBinding->getBuffer(p_elem->getSource());
					unsigned char* v_ptr = static_cast<unsigned char*>(v_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
					unsigned char* v_offset;

					for (size_t j = start; j < (start + v_count); j++)
					{
						v_offset = v_ptr + (j * v_sptr->getVertexSize());
						p_elem->baseVertexPointerToElement(v_offset, &v_Posptr);

						vertices[offset].x = *v_Posptr; v_Posptr++;
						vertices[offset].y = *v_Posptr; v_Posptr++;
						vertices[offset].z = *v_Posptr; v_Posptr++;
						vertices[offset] *= scale;
						offset++;
					}

					v_sptr->unlock();
				}
			}

			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeConvexHull(ndInt32(total_verts), sizeof(Ogre::Vector3), ndFloat32(tolerance), (ndFloat32*)&vertices[0].x));
			m_col = m_shapeInstance->GetShape();

			delete[] vertices;
		}

		ConvexHull::ConvexHull(const World* world, Ogre::Item* item, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance,
			const Ogre::Vector3& forceScale) : ConvexCollision(world)
		{
			Ogre::Vector3 scale(1.0, 1.0, 1.0);
			Ogre::MeshPtr mesh = item->getMesh();

			Ogre::Node* node = item->getParentNode();
			if (node)
			{
				scale = node->_getDerivedScaleUpdated();
			}
			if (forceScale != Ogre::Vector3::ZERO)
			{
				scale = forceScale;
			}

			unsigned int numVertices = 0;
			Ogre::Mesh::SubMeshVec::const_iterator subMeshIterator = mesh->getSubMeshes().begin();

			while (subMeshIterator != mesh->getSubMeshes().end())
			{
				Ogre::SubMesh* subMesh = *subMeshIterator;
				numVertices += static_cast<unsigned int>(subMesh->mVao[0][0]->getVertexBuffers()[0]->getNumElements());
				subMeshIterator++;
			}

			Ogre::Vector3* vertices = OGRE_ALLOC_T(Ogre::Vector3, numVertices, Ogre::MEMCATEGORY_GEOMETRY);
			unsigned int subMeshOffset = 0;

			subMeshIterator = mesh->getSubMeshes().begin();
			while (subMeshIterator != mesh->getSubMeshes().end())
			{
				Ogre::SubMesh* subMesh = *subMeshIterator;
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[0];

				if (!vaos.empty())
				{
					Ogre::VertexArrayObject* vao = vaos[0];
					Ogre::VertexArrayObject::ReadRequestsVec requests;
					requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));

					vao->readRequests(requests);
					vao->mapAsyncTickets(requests);
					unsigned int subMeshVerticiesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());

					if (requests[0].type == Ogre::VET_HALF4)
					{
						for (size_t i = 0; i < subMeshVerticiesNum; ++i)
						{
							const Ogre::uint16* pos = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
							Ogre::Vector3 vec;
							vec.x = Ogre::Bitwise::halfToFloat(pos[0]);
							vec.y = Ogre::Bitwise::halfToFloat(pos[1]);
							vec.z = Ogre::Bitwise::halfToFloat(pos[2]);
							requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
							vertices[i + subMeshOffset] = vec * scale;
						}
					}
					else if (requests[0].type == Ogre::VET_FLOAT3)
					{
						for (size_t i = 0; i < subMeshVerticiesNum; ++i)
						{
							const float* pos = reinterpret_cast<const float*>(requests[0].data);
							Ogre::Vector3 vec;
							vec.x = *pos++;
							vec.y = *pos++;
							vec.z = *pos++;
							requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
							vertices[i + subMeshOffset] = vec * scale;
						}
					}
					else
					{
						throw Ogre::Exception(0, "Vertex Buffer type not recognised", "ConvexHull");
					}
					subMeshOffset += subMeshVerticiesNum;
					vao->unmapAsyncTickets(requests);
				}
				subMeshIterator++;
			}

			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeConvexHull(ndInt32(numVertices), sizeof(Ogre::Vector3), ndFloat32(tolerance),(ndFloat32*)&vertices[0].x));
			m_col = m_shapeInstance->GetShape();

			OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
		}

		ConvexHull::ConvexHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance) : ConvexCollision(world)
		{
			if (m_col)
				m_col->Release();

			m_shapeInstance = new ndShapeInstance(new ndShapeConvexHull(vertcount, sizeof(Ogre::Vector3), ndFloat32(tolerance), (ndFloat32*)&verts[0].x));
			m_col = m_shapeInstance->GetShape();
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

		TreeCollision::TreeCollision(const World* world) : Collision(world), m_faceCount(-1), m_categoryId(0)
		{
		}

		TreeCollision::TreeCollision(const World* world, Ogre::v1::Entity* obj, bool optimize,
			unsigned int id, FaceWinding fw) : Collision(world), m_faceCount(0), m_categoryId(id)
		{
			Ogre::Vector3 scale(1.0, 1.0, 1.0);
			Ogre::v1::MeshPtr mesh = obj->getMesh();

			Ogre::Node* node = obj->getParentNode();
			if (node)
			{
				scale = node->_getDerivedScaleUpdated();
			}

			std::vector<Ogre::Vector3> vertices;
			std::vector<int> indices;

			unsigned short sub = mesh->getNumSubMeshes();
			Ogre::v1::VertexData* v_data;
			bool addedShared = false;

			for (unsigned short i = 0; i < sub; i++)
			{
				Ogre::v1::SubMesh* sub_mesh = mesh->getSubMesh(i);
				Ogre::v1::VertexDeclaration* v_decl;
				const Ogre::v1::VertexElement* p_elem;
				float* v_Posptr;
				size_t v_count;

				v_data = nullptr;

				if (sub_mesh->useSharedVertices)
				{
					if (!addedShared)
					{
						v_data = mesh->sharedVertexData[0];
						v_count = v_data->vertexCount;
						v_decl = v_data->vertexDeclaration;
						p_elem = v_decl->findElementBySemantic(Ogre::VES_POSITION);
						addedShared = true;
					}
				}
				else
				{
					v_data = sub_mesh->vertexData[0];
					v_count = v_data->vertexCount;
					v_decl = v_data->vertexDeclaration;
					p_elem = v_decl->findElementBySemantic(Ogre::VES_POSITION);
				}

				if (v_data)
				{
					size_t start = v_data->vertexStart;
					size_t vertexOffset = vertices.size();

					Ogre::v1::HardwareVertexBufferSharedPtr v_sptr = v_data->vertexBufferBinding->getBuffer(p_elem->getSource());
					unsigned char* v_ptr = static_cast<unsigned char*>(v_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
					unsigned char* v_offset;

					for (size_t j = start; j < (start + v_count); j++)
					{
						v_offset = v_ptr + (j * v_sptr->getVertexSize());
						p_elem->baseVertexPointerToElement(v_offset, &v_Posptr);

						Ogre::Vector3 vert;
						vert.x = *v_Posptr; v_Posptr++;
						vert.y = *v_Posptr; v_Posptr++;
						vert.z = *v_Posptr; v_Posptr++;
						vert *= scale;
						vertices.push_back(vert);
					}

					v_sptr->unlock();

					// Get indices
					Ogre::v1::IndexData* i_data = sub_mesh->indexData[0];
					size_t index_count = i_data->indexCount;
					size_t index_start = i_data->indexStart;

					Ogre::v1::HardwareIndexBufferSharedPtr i_sptr = i_data->indexBuffer;

					if (i_sptr->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT)
					{
						unsigned int* i_Longptr = static_cast<unsigned int*>(i_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
						for (size_t j = index_start; j < index_start + index_count; j++)
						{
							int index = static_cast<int>(i_Longptr[j]) + static_cast<int>(vertexOffset);
							if (fw == FW_REVERSE)
							{
								// Reverse winding order
								if ((j - index_start) % 3 == 0)
									indices.push_back(index);
								else if ((j - index_start) % 3 == 1)
									indices.push_back(static_cast<int>(i_Longptr[j + 1]) + static_cast<int>(vertexOffset));
								else
									indices.push_back(static_cast<int>(i_Longptr[j - 1]) + static_cast<int>(vertexOffset));
							}
							else
							{
								indices.push_back(index);
							}
						}
					}
					else
					{
						unsigned short* i_Shortptr = static_cast<unsigned short*>(i_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
						for (size_t j = index_start; j < index_start + index_count; j++)
						{
							int index = static_cast<int>(i_Shortptr[j]) + static_cast<int>(vertexOffset);
							if (fw == FW_REVERSE)
							{
								if ((j - index_start) % 3 == 0)
									indices.push_back(index);
								else if ((j - index_start) % 3 == 1)
									indices.push_back(static_cast<int>(i_Shortptr[j + 1]) + static_cast<int>(vertexOffset));
								else
									indices.push_back(static_cast<int>(i_Shortptr[j - 1]) + static_cast<int>(vertexOffset));
							}
							else
							{
								indices.push_back(index);
							}
						}
					}

					i_sptr->unlock();
				}
			}

			if (m_col)
				m_col->Release();

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			// Add all triangles
			for (size_t i = 0; i < indices.size(); i += 3)
			{
				ndVector face[3];
				face[0] = ndVector(vertices[indices[i]].x, vertices[indices[i]].y, vertices[indices[i]].z, ndFloat32(1.0f));
				face[1] = ndVector(vertices[indices[i + 1]].x, vertices[indices[i + 1]].y, vertices[indices[i + 1]].z, ndFloat32(1.0f));
				face[2] = ndVector(vertices[indices[i + 2]].x, vertices[indices[i + 2]].y, vertices[indices[i + 2]].z, ndFloat32(1.0f));

				meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
			}

			meshBuilder.End(optimize);

			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

		TreeCollision::TreeCollision(const World* world, Ogre::Item* item, bool optimize,
			unsigned int id, FaceWinding fw) : Collision(world), m_faceCount(0), m_categoryId(id)
		{
			Ogre::Vector3 scale(1.0, 1.0, 1.0);
			Ogre::MeshPtr mesh = item->getMesh();

			Ogre::Node* node = item->getParentNode();
			if (node)
			{
				scale = node->_getDerivedScaleUpdated();
			}

			std::vector<Ogre::Vector3> vertices;
			std::vector<int> indices;

			Ogre::Mesh::SubMeshVec::const_iterator subMeshIterator = mesh->getSubMeshes().begin();

			while (subMeshIterator != mesh->getSubMeshes().end())
			{
				Ogre::SubMesh* subMesh = *subMeshIterator;
				Ogre::VertexArrayObjectArray vaos = subMesh->mVao[0];

				if (!vaos.empty())
				{
					size_t vertexOffset = vertices.size();
					Ogre::VertexArrayObject* vao = vaos[0];

					// Read vertex positions
					Ogre::VertexArrayObject::ReadRequestsVec requests;
					requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
					vao->readRequests(requests);
					vao->mapAsyncTickets(requests);

					unsigned int subMeshVerticiesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());

					if (requests[0].type == Ogre::VET_HALF4)
					{
						for (size_t i = 0; i < subMeshVerticiesNum; ++i)
						{
							const Ogre::uint16* pos = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
							Ogre::Vector3 vec;
							vec.x = Ogre::Bitwise::halfToFloat(pos[0]);
							vec.y = Ogre::Bitwise::halfToFloat(pos[1]);
							vec.z = Ogre::Bitwise::halfToFloat(pos[2]);
							requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
							vertices.push_back(vec * scale);
						}
					}
					else if (requests[0].type == Ogre::VET_FLOAT3)
					{
						for (size_t i = 0; i < subMeshVerticiesNum; ++i)
						{
							const float* pos = reinterpret_cast<const float*>(requests[0].data);
							Ogre::Vector3 vec;
							vec.x = *pos++;
							vec.y = *pos++;
							vec.z = *pos++;
							requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
							vertices.push_back(vec * scale);
						}
					}

					vao->unmapAsyncTickets(requests);

					// Read indices
					Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
					if (indexBuffer)
					{
						size_t numElements = indexBuffer->getNumElements();
						const void* indexData = indexBuffer->map(0, numElements);

						if (indexBuffer->getIndexType() == Ogre::IndexBufferPacked::IT_16BIT)
						{
							const Ogre::uint16* indices16 = reinterpret_cast<const Ogre::uint16*>(indexData);
							for (size_t i = 0; i < numElements; i++)
							{
								indices.push_back(static_cast<int>(indices16[i]) + static_cast<int>(vertexOffset));
							}
						}
						else
						{
							const Ogre::uint32* indices32 = reinterpret_cast<const Ogre::uint32*>(indexData);
							for (size_t i = 0; i < numElements; i++)
							{
								indices.push_back(static_cast<int>(indices32[i]) + static_cast<int>(vertexOffset));
							}
						}
						indexBuffer->unmap(Ogre::UO_KEEP_PERSISTENT);
					}
				}
				subMeshIterator++;
			}

			if (m_col)
				m_col->Release();

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			for (size_t i = 0; i < indices.size(); i += 3)
			{
				ndVector face[3];
				face[0] = ndVector(vertices[indices[i]].x, vertices[indices[i]].y, vertices[indices[i]].z, ndFloat32(1.0f));
				face[1] = ndVector(vertices[indices[i + 1]].x, vertices[indices[i + 1]].y, vertices[indices[i + 1]].z, ndFloat32(1.0f));
				face[2] = ndVector(vertices[indices[i + 2]].x, vertices[indices[i + 2]].y, vertices[indices[i + 2]].z, ndFloat32(1.0f));

				if (fw == FW_REVERSE)
				{
					std::swap(face[1], face[2]);
				}

				meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
			}

			meshBuilder.End(optimize);

			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

		TreeCollision::TreeCollision(const World* world, int numVertices, int numIndices,
			const float* vertices, const int* indices, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world), m_faceCount(0), m_categoryId(id)
		{
			if (m_col)
				m_col->Release();

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			for (int i = 0; i < numIndices; i += 3)
			{
				ndVector face[3];
				int idx0 = indices[i] * 3;
				int idx1 = indices[i + 1] * 3;
				int idx2 = indices[i + 2] * 3;

				face[0] = ndVector(vertices[idx0], vertices[idx0 + 1], vertices[idx0 + 2], ndFloat32(1.0f));
				face[1] = ndVector(vertices[idx1], vertices[idx1 + 1], vertices[idx1 + 2], ndFloat32(1.0f));
				face[2] = ndVector(vertices[idx2], vertices[idx2 + 1], vertices[idx2 + 2], ndFloat32(1.0f));

				if (fw == FW_REVERSE)
				{
					std::swap(face[1], face[2]);
				}

				meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
			}

			meshBuilder.End(optimize);

			m_shapeInstance = new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder));
			m_col = m_shapeInstance->GetShape();
		}

		TreeCollision::TreeCollision(const World* world, int numVertices, Ogre::Vector3* vertices,
			Ogre::v1::IndexData* indexData, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world), m_faceCount(0), m_categoryId(id)
		{
			if (m_col)
				m_col->Release();

			ndPolygonSoupBuilder meshBuilder;
			meshBuilder.Begin();

			size_t index_count = indexData->indexCount;
			size_t index_start = indexData->indexStart;
			Ogre::v1::HardwareIndexBufferSharedPtr i_sptr = indexData->indexBuffer;

			if (i_sptr->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT)
			{
				unsigned int* i_Longptr = static_cast<unsigned int*>(i_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
				for (size_t j = index_start; j < index_start + index_count; j += 3)
				{
					ndVector face[3];
					face[0] = ndVector(vertices[i_Longptr[j]].x, vertices[i_Longptr[j]].y, vertices[i_Longptr[j]].z, ndFloat32(1.0f));
					face[1] = ndVector(vertices[i_Longptr[j + 1]].x, vertices[i_Longptr[j + 1]].y, vertices[i_Longptr[j + 1]].z, ndFloat32(1.0f));
					face[2] = ndVector(vertices[i_Longptr[j + 2]].x, vertices[i_Longptr[j + 2]].y, vertices[i_Longptr[j + 2]].z, ndFloat32(1.0f));

					if (fw == FW_REVERSE)
					{
						std::swap(face[1], face[2]);
					}

					meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
				}
			}
			else
			{
				unsigned short* i_Shortptr = static_cast<unsigned short*>(i_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
				for (size_t j = index_start; j < index_start + index_count; j += 3)
				{
					ndVector face[3];
					face[0] = ndVector(vertices[i_Shortptr[j]].x, vertices[i_Shortptr[j]].y, vertices[i_Shortptr[j]].z, ndFloat32(1.0f));
					face[1] = ndVector(vertices[i_Shortptr[j + 1]].x, vertices[i_Shortptr[j + 1]].y, vertices[i_Shortptr[j + 1]].z, ndFloat32(1.0f));
					face[2] = ndVector(vertices[i_Shortptr[j + 2]].x, vertices[i_Shortptr[j + 2]].y, vertices[i_Shortptr[j + 2]].z, ndFloat32(1.0f));

					if (fw == FW_REVERSE)
					{
						std::swap(face[1], face[2]);
					}

					meshBuilder.AddFace(&face[0].m_x, sizeof(ndVector), 3, id);
				}
			}

			i_sptr->unlock();
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
