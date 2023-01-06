#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_RayCast.h"
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

		// OgreNewt::CollisionPrimitives::Null
		Null::Null(const OgreNewt::World *world) : Collision(world)
		{
			m_col = NewtonCreateNull(m_world->getNewtonWorld());
			NewtonCollisionSetUserData(m_col, this);
		}


		// OgreNewt::CollisionPrimitives::Box
		Box::Box(const World* world) : ConvexCollision(world)
		{
		}

		Box::Box(const World* world, const Ogre::Vector3& size, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			m_col = NewtonCreateBox(m_world->getNewtonWorld(), (float)size.x, (float)size.y, (float)size.z, id, &matrix[0]);
			NewtonCollisionSetUserData(m_col, this);
		}

		// OgreNewt::CollisionPrimitives::Ellipsoid
		Ellipsoid::Ellipsoid(const World* world) : ConvexCollision(world)
		{
		}

		Ellipsoid::Ellipsoid(const World* world, const Ogre::Vector3& size, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			float matrix[16];

			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			NewtonCollision* sphere_col = 0;
			if ((size.x == size.y) && (size.y == size.z))
				m_col = NewtonCreateSphere(m_world->getNewtonWorld(), (float)size.x, id, &matrix[0]);
			else
			{
				float radius = std::min(std::min(size.x, size.y), size.z);
				Ogre::Vector3 scale = Ogre::Vector3(size.x / radius, size.y / radius, size.z / radius);
				m_col = NewtonCreateSphere(m_world->getNewtonWorld(), radius, id, &matrix[0]);
				NewtonCollisionSetScale(m_col, scale.x, scale.y, scale.z);
			}
			NewtonCollisionSetUserData(m_col, this);
		}

		// OgreNewt::CollisionPrimitives::Cylinder
		Cylinder::Cylinder(const World* world) : ConvexCollision(world)
		{
		}

		Cylinder::Cylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			// Attention, its possible to create an unproportional body, by settigs radius1 and radius2 different
			m_col = NewtonCreateCylinder(m_world->getNewtonWorld(), radius, radius, height, id, &matrix[0]);
			NewtonCollisionSetUserData(m_col, this);
		}

		// OgreNewt::CollisionPrimitives::Capsule
		Capsule::Capsule(const World* world) : ConvexCollision(world)
		{
		}

		Capsule::Capsule(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			// Attention, its possible to create an unproportional body, by settigs radius1 and radius2 different
			// m_col = NewtonCreateCapsule(m_world->getNewtonWorld(), radius, radius, height, id, &matrix[0]);
			m_col = NewtonCreateCapsule(m_world->getNewtonWorld(), (float)radius, (float)radius, (float)(height - (radius * 2.0)), id, &matrix[0]);
			NewtonCollisionSetUserData(m_col, this);
		}

		// OgreNewt::CollisionPrimitives::Cone
		Cone::Cone(const World* world) : ConvexCollision(world)
		{
		}

		Cone::Cone(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			m_col = NewtonCreateCone(m_world->getNewtonWorld(), radius, height, id, &matrix[0]);
			NewtonCollisionSetUserData(m_col, this);
		}

		// OgreNewt::CollisionPrimitives::ChamferCylinder
		ChamferCylinder::ChamferCylinder(const World* world) : ConvexCollision(world)
		{
		}

		ChamferCylinder::ChamferCylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
		{
			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			m_col = NewtonCreateChamferCylinder(m_world->getNewtonWorld(), radius, height, id, &matrix[0]);
			NewtonCollisionSetUserData(m_col, this);
		}


		// OgreNewt::CollisionPrimitives::ConvexHull
		ConvexHull::ConvexHull(const World* world) : ConvexCollision(world)
		{
		}

		ConvexHull::ConvexHull(const World* world, Ogre::v1::Entity* obj, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance, const Ogre::Vector3& forceScale) : ConvexCollision(world)
		{
			Ogre::Vector3 scale(1.0, 1.0, 1.0);
			// Get the mesh!
			Ogre::v1::MeshPtr mesh = obj->getMesh();

			// get scale, if attached to node
			Ogre::Node * node = obj->getParentNode();
			if (node)
			{
				// Here no performance issues, because a collision is not created often :)
				scale = node->_getDerivedScaleUpdated();
			}
			if (forceScale != Ogre::Vector3::ZERO)
			{
				scale = forceScale;
			}
			//find number of submeshes
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

			//make array to hold vertex positions!
			Ogre::Vector3* vertices = new Ogre::Vector3[total_verts];
			unsigned int offset = 0;

			//loop back through, adding vertices as we go!
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
					//pointer
					Ogre::v1::HardwareVertexBufferSharedPtr v_sptr = v_data->vertexBufferBinding->getBuffer(p_elem->getSource());
					unsigned char* v_ptr = static_cast<unsigned char*>(v_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
					unsigned char* v_offset;

					//loop through vertex data...
					for (size_t j = start; j < (start + v_count); j++)
					{
						//get offset to Position data!
						v_offset = v_ptr + (j * v_sptr->getVertexSize());
						p_elem->baseVertexPointerToElement(v_offset, &v_Posptr);

						//now get vertex positions...
						vertices[offset].x = *v_Posptr; v_Posptr++;
						vertices[offset].y = *v_Posptr; v_Posptr++;
						vertices[offset].z = *v_Posptr; v_Posptr++;

						vertices[offset] *= scale;

						offset++;
					}

					//unlock buffer
					v_sptr->unlock();
				}
			}

			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			//okay, let's try making the ConvexHull!
			m_col = NewtonCreateConvexHull(m_world->getNewtonWorld(), (int)total_verts, (float*)&vertices[0].x, sizeof(Ogre::Vector3), tolerance, id, &matrix[0]);
			
			// Is that correct
			NewtonCollisionSetUserData(m_col, this);

			delete[] vertices;
		}

		ConvexHull::ConvexHull(const World* world, Ogre::Item* item, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance, const Ogre::Vector3& forceScale)
			: ConvexCollision(world)
		{
			Ogre::Vector3 scale(1.0, 1.0, 1.0);
			// Get the mesh!
			Ogre::MeshPtr mesh = item->getMesh();

			// get scale, if attached to node
			Ogre::Node* node = item->getParentNode();
			if (node)
			{
				// Here no performance issues, because a collision is not created often :)
				scale = node->_getDerivedScaleUpdated();
			}
			if (forceScale != Ogre::Vector3::ZERO)
			{
				scale = forceScale;
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

			//allocate memory to the input array reference, handleRequest calls delete on this memory
			Ogre::Vector3* vertices = OGRE_ALLOC_T(Ogre::Vector3, numVertices, Ogre::MEMCATEGORY_GEOMETRY);

			unsigned int addedVertices = 0;
			unsigned int subMeshOffset = 0;

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

			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			//okay, let's try making the ConvexHull!
			m_col = NewtonCreateConvexHull(m_world->getNewtonWorld(), (int)numVertices, (float*)&vertices[0].x, sizeof(Ogre::Vector3), tolerance, id, &matrix[0]);

			// Is that correct
			NewtonCollisionSetUserData(m_col, this);

			delete[] vertices;
		}

		// OgreNewt::CollisionPrimitives::ConvexHull
		ConvexHull::ConvexHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance) : ConvexCollision(world)
		{
			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			if (m_col)
				NewtonDestroyCollision(m_col);

			//make the collision primitive.
			m_col = NewtonCreateConvexHull(m_world->getNewtonWorld(), vertcount, (float*)&verts[0].x, sizeof(Ogre::Vector3), tolerance, id, &matrix[0]);
			NewtonCollisionSetUserData(m_col, this);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// OgreNewt::CollisionPrimitives::ConcaveHull
		ConcaveHull::ConcaveHull(const World* world) : ConvexCollision(world)
		{
		}

		ConcaveHull::ConcaveHull(const World* world, Ogre::v1::Entity* obj, unsigned int id, Ogre::Real tolerance, const Ogre::Vector3& scale)
			: ConvexCollision(world)
		{
			if (m_col)
				NewtonDestroyCollision(m_col);

			//first create a mesh
			NewtonMesh* const newtonMesh = NewtonMeshCreate(m_world->getNewtonWorld());
			NewtonMeshBeginFace(newtonMesh);

			Ogre::v1::MeshPtr mesh = obj->getMesh();
			//find number of sub-meshes
			unsigned short sub = mesh->getNumSubMeshes();

			for (unsigned short cs = 0; cs < sub; cs++)
			{
				Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(cs);

				//vertex data!
				Ogre::v1::VertexData* v_data;

				if (subMesh->useSharedVertices)
				{
					v_data = mesh->sharedVertexData[0];
				}
				else
				{
					v_data = subMesh->vertexData[0];
				}

				//let's find more information about the Vertices...
				Ogre::v1::VertexDeclaration* v_decl = v_data->vertexDeclaration;
				const Ogre::v1::VertexElement* p_elem = v_decl->findElementBySemantic(Ogre::VES_POSITION);

				// get pointer!
				Ogre::v1::HardwareVertexBufferSharedPtr v_sptr = v_data->vertexBufferBinding->getBuffer(p_elem->getSource());
				unsigned char* v_ptr = static_cast<unsigned char*>(v_sptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				//now find more about the index!!
				Ogre::v1::IndexData* i_data = subMesh->indexData[0];
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

							NewtonMeshAddPoint(newtonMesh, poly_verts[j].x, poly_verts[j].y, poly_verts[j].z);
						}
					}
					// NewtonMeshAddFace(nmesh, 3, (float*)&poly_verts[0].x, sizeof(Ogre::Vector3), cs);
					

					i_offset += 3;
				}

				//unlock the buffers!
				v_sptr->unlock();
				i_sptr->unlock();

			}
			//done!
			NewtonMeshEndFace(newtonMesh);
			NewtonMeshFixTJoints(newtonMesh);
			NewtonMesh* const newtonConvexMesh = NewtonMeshApproximateConvexDecomposition(newtonMesh, 0.5f, 0.1f, 10, 1000, NULL, NULL);

			if (newtonMesh)
				NewtonMeshDestroy(newtonMesh);

			if (m_col)
				NewtonDestroyCollision(m_col);

			m_col = NewtonCreateCompoundCollisionFromMesh(m_world->getNewtonWorld(), newtonConvexMesh, tolerance, id, id);
			NewtonCollisionSetUserData(m_col, this);

			if (newtonConvexMesh)
				NewtonMeshDestroy(newtonConvexMesh);

			//CollisionSerializer serializer = CollisionSerializer();
			//serializer.exportCollision(OgreNewt::CollisionPtr(this), std::string("d:/") + (obj->getName()));
		}

		// OgreNewt::CollisionPrimitives::ConcaveHull
		ConcaveHull::ConcaveHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id, Ogre::Real tolerance)
			: ConvexCollision(world)
		{
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		TetrahedraDeformableCollision::TetrahedraDeformableCollision(const World* world)
			: ConvexCollision(world)
		{

		}
		
		TetrahedraDeformableCollision::TetrahedraDeformableCollision(const World* world, const NewtonCollision* const collision, unsigned int id)
			: ConvexCollision(world)
		{
			NewtonMesh* const skinMesh = NewtonMeshCreateFromCollision(collision);

			// NewtonMeshApplyBoxMapping(m_skinMesh, 0, 0, 0);
			// Now make an tetrahedra iso surface approximation of this mesh
			NewtonMesh* const tetraIsoSurface = NewtonMeshCreateTetrahedraIsoSurface(skinMesh);

			// calculate the linear blend weight for the tetrahedra mesh
			NewtonCreateTetrahedraLinearBlendSkinWeightsChannel(tetraIsoSurface, skinMesh);
			NewtonDestroyCollision(collision);

			if (m_col)
				NewtonDestroyCollision(m_col);

			// make a deformable collision mesh
			m_col = NewtonCreateDeformableSolid(world->getNewtonWorld(), tetraIsoSurface, id);
			NewtonCollisionSetUserData(m_col, this);

			////create a rigid body with a deformable mesh
			//m_body = CreateRigidBody(scene, mass, deformableCollision);

			//// create the soft body mesh
			//DemoMesh* const mesh = new TetrahedraSoftMesh(tetraIsoSurface, m_body);
			//// DemoMesh* const mesh = new LinearBlendMeshTetra(skinMesh, m_body);
			//SetMesh(mesh, dGetIdentityMatrix());

			//// do not forget to destroy this objects, else you get bad memory leaks.
			//mesh->Release();
			NewtonMeshDestroy(skinMesh);
			// NewtonDestroyCollision(m_col);
			NewtonMeshDestroy(tetraIsoSurface);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		TreeCollision::TreeCollision(const World* world)
			: Collision(world),
			m_faceCount(-1),
			m_categoryId(0)
		{
		}


		TreeCollision::TreeCollision(const World* world, Ogre::v1::Entity* obj, bool optimize, unsigned int id, FaceWinding fw) : Collision(world)
		{
			Ogre::Vector3 scale;

			m_categoryId = id;

			start(id);

			Ogre::v1::MeshPtr mesh = obj->getMesh();

			// get scale, if attached to node
			Ogre::Node * node = obj->getParentNode();
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
						addPoly(poly_verts, id);
					}
					else
					{
						Ogre::Vector3 rev_poly_verts[3];
						rev_poly_verts[0] = poly_verts[0];
						rev_poly_verts[1] = poly_verts[2];
						rev_poly_verts[2] = poly_verts[1];

						addPoly(rev_poly_verts, id);
					}

					i_offset += 3;
				}

				//unlock the buffers!
				v_sptr->unlock();
				i_sptr->unlock();

			}
			//done!
			finish(optimize);
		}

		TreeCollision::TreeCollision(const World* world, Ogre::Item* item, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;

			Ogre::Vector3 scale;
			// Start building collision tree
			start(id);

			Ogre::MeshPtr mesh = item->getMesh();
			// Get scale, if attached to node
			Ogre::Node * node = item->getParentNode();
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
							}
							else
							{
								Ogre::Vector3 revertedPolygonVertices[3];
								revertedPolygonVertices[0] = polygonVertices[0];
								revertedPolygonVertices[1] = polygonVertices[2];
								revertedPolygonVertices[2] = polygonVertices[1];

								addPoly(revertedPolygonVertices, index);
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
							}
							else
							{
								Ogre::Vector3 revertedPolygonVertices[3];
								revertedPolygonVertices[0] = polygonVertices[0];
								revertedPolygonVertices[1] = polygonVertices[2];
								revertedPolygonVertices[2] = polygonVertices[1];

								addPoly(revertedPolygonVertices, index);
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
			finish(optimize);
		}

		TreeCollision::TreeCollision(const OgreNewt::World *world, int numVertices, int numIndices, const float *vertices, const int *indices, bool optimize, unsigned int id, FaceWinding fw) : OgreNewt::Collision(world)
		{
			start(id);

			int numPolys = numIndices / 3;

			Ogre::Vector3 *vecVertices = new Ogre::Vector3[numVertices];

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
			}

			delete[] vecVertices;

			finish(optimize);
		}


		TreeCollision::TreeCollision(const World* world, int numVertices, Ogre::Vector3* vertices, Ogre::v1::IndexData* indexData, bool optimize, unsigned int id, FaceWinding fw)
			: Collision(world)
		{
			m_categoryId = id;

			start(id);

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

					//invert vertex winding (otherwise, raycasting won't work???)
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
				}
			}

			hwIndexBuffer->unlock();
			finish(optimize);
		}


		void TreeCollision::start(unsigned int id)
		{
			m_categoryId = id;

			m_col = NewtonCreateTreeCollision(m_world->getNewtonWorld(), id);
			NewtonTreeCollisionBeginBuild(m_col);
		}

		void TreeCollision::addPoly(Ogre::Vector3* polys, unsigned int ID)
		{
			NewtonTreeCollisionAddFace(m_col, 3, (float*)&polys[0].x, sizeof(Ogre::Vector3), ID);
		}

		void TreeCollision::finish(bool optimize)
		{
			NewtonTreeCollisionEndBuild(m_col, optimize);
			NewtonCollisionSetUserData(m_col, this);
		}


		void TreeCollision::setFaceId(unsigned int faceId)
		{
			NewtonTreeCollisionForEachFace(m_col, CountFaces, this);

			m_faceCount = 0;
			NewtonTreeCollisionForEachFace(m_col, MarkFaces, this);
		}

		int TreeCollision::CountFaces(void* const context, const dFloat* const polygon, int strideInBytes, const int* const indexArray, int indexCount)
		{
			TreeCollision* const me = (TreeCollision*)context;
			me->m_faceCount++;
			return 1;
		}

		int TreeCollision::MarkFaces(void* const context, const dFloat* const polygon, int strideInBytes, const int* const indexArray, int indexCount)
		{
			TreeCollision* const me = (TreeCollision*)context;

			// repmap material index, by enumerating the face and storing the user material info at each face index
			int faceIndex = NewtonTreeCollisionGetFaceAttribute(me->m_col, indexArray, indexCount);
			NewtonTreeCollisionSetFaceAttribute(me->m_col, indexArray, indexCount, /*me->m_faceCount*/ me->m_categoryId);

			me->m_faceCount++;
			return 1;
		}

		float _CDECL TreeCollision::newtonRayCastCallback(const NewtonBody* const body, const NewtonCollision* const treeCollision, dFloat interception,
			dFloat* normal, int faceId, void* userData)
		{
			Body* bod = static_cast<Raycast*> (userData)->mLastBody;
			/*OgreNewt::CollisionPtr collision =
				static_cast<Raycast*> (userData)->mLastCollision;*/

			//! TODO: what do we need to return here?
			if (!bod/* || !collision*/)
				return 0;

			((Raycast*)userData)->userCallback(bod, /*collision*/ nullptr, interception, Ogre::Vector3(normal[0], normal[1], normal[2]), faceId);

			((Raycast*)userData)->mBodyAdded = true;

			return interception;
		}

		void TreeCollision::setRayCastCallbackactive(bool active, const NewtonCollision *col)
		{
			if (active)
				NewtonTreeCollisionSetUserRayCastCallback(col, newtonRayCastCallback);
			else
				NewtonTreeCollisionSetUserRayCastCallback(col, nullptr);
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		HeightField::HeightField(const OgreNewt::World *world, int width, int height, int gridsDiagonals, Ogre::Real* elevationMap, char* attributeMap,
			Ogre::Real verticleScale, Ogre::Real horizontalScaleX, Ogre::Real horizontalScaleZ, const Ogre::Vector3& position, const Ogre::Quaternion& orientation,unsigned int shapeID)
			: Collision (world)
		{
			if (m_col)
				NewtonDestroyCollision(m_col);

			m_col = NewtonCreateHeightFieldCollision(world->getNewtonWorld(), width, height, gridsDiagonals, 0, elevationMap, attributeMap, verticleScale, horizontalScaleX, horizontalScaleZ, shapeID);
			NewtonCollisionSetUserData(m_col, this);

			float matrix[16];
			OgreNewt::Converters::QuatPosToMatrix(orientation, position, &matrix[0]);
			NewtonCollisionSetMatrix(m_col, &matrix[0]);

			// esp function calling convention error
			// NewtonHeightFieldSetUserRayCastCallback(m_col, newtonRayCastCallback);
		}

		float _CDECL HeightField::newtonRayCastCallback(const NewtonBody*  const body, const NewtonCollision* const heightFieldCollision, dFloat interception, int row, int col, dFloat* normal, int faceId, void* usedData)
		{
			Body* bod = ((HeightFieldRaycast*)usedData)->m_heightfieldcollisioncallback_lastbody;
		            
			//! TODO: what do we need to return here?
			if(!bod)
				return 0;
					
			// esp function calling convention error
			((HeightFieldRaycast*)usedData)->userCallback( bod, interception, Ogre::Vector3(normal[0], normal[1], normal[2]), faceId );

			((HeightFieldRaycast*)usedData)->m_heightfieldcollisioncallback_bodyalreadyadded = true;

			return interception;

		}

		void HeightField::setRayCastCallbackactive(bool active, const NewtonCollision *col )
		{
			if(active)
				NewtonHeightFieldSetUserRayCastCallback(col, newtonRayCastCallback);
			else
				NewtonHeightFieldSetUserRayCastCallback(col, nullptr);
		}

		int TreeCollisionSceneParser::count = 0;


		TreeCollisionSceneParser::TreeCollisionSceneParser(OgreNewt::World* world) : TreeCollision(world)
		{
		}

		void TreeCollisionSceneParser::parseScene(Ogre::SceneNode *startNode, unsigned int id, bool optimize, FaceWinding fw)
		{
			count = 0;

			start(id);

			// parse the individual nodes.
			Ogre::Quaternion rootOrient = Ogre::Quaternion::IDENTITY;
			Ogre::Vector3 rootPos = Ogre::Vector3::ZERO;
			Ogre::Vector3 rootScale = startNode->getScale();;

			_parseNode(startNode, rootOrient, rootPos, rootScale, fw, id);

			finish(optimize);
		}

		void TreeCollisionSceneParser::_parseNode(Ogre::SceneNode *node, const Ogre::Quaternion &curOrient, const Ogre::Vector3 &curPos, const Ogre::Vector3 &curScale, FaceWinding fw, unsigned int id)
		{
			// parse this scene node.
			// do children first.
			Ogre::Quaternion thisOrient = curOrient * node->getOrientation();
			Ogre::Vector3 thisPos = curPos + (curOrient * (node->getPosition() * curScale));
			Ogre::Vector3 thisScale = curScale * node->getScale();

			Ogre::SceneNode::NodeVecIterator child_it = node->getChildIterator();

			while (child_it.hasMoreElements())
			{
				_parseNode((Ogre::SceneNode*)child_it.getNext(), thisOrient, thisPos, thisScale, fw, id);
			}


			// now add the polys from this node.
			//now get the mesh!
			size_t num_obj = node->numAttachedObjects();
			for (size_t co = 0; co < num_obj; co++)
			{
				Ogre::MovableObject* obj = node->getAttachedObject(co);
				if (obj->getMovableType() != "Entity")
					continue;

				Ogre::v1::Entity* ent = (Ogre::v1::Entity*)obj;

				if (!entityFilter(node, ent, fw))
					continue;

				Ogre::v1::MeshPtr mesh = ent->getMesh();

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

								poly_verts[j] = thisPos + (thisOrient * (poly_verts[j] * curScale));
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

								poly_verts[j] = thisPos + (thisOrient * (poly_verts[j] * curScale));
							}
						}

						if (fw == FW_DEFAULT)
						{
							addPoly(poly_verts, id);
						}
						else
						{
							Ogre::Vector3 rev_poly_verts[3];
							rev_poly_verts[0] = poly_verts[0];
							rev_poly_verts[0] = poly_verts[2];
							rev_poly_verts[0] = poly_verts[1];

							addPoly(rev_poly_verts, id);
						}

						i_offset += 3;
					}

					//unlock the buffers!
					v_sptr->unlock();
					i_sptr->unlock();

				}
			}

		}

		// OgreNewt::CollisionPrimitives::CompoundCollision
		CompoundCollision::CompoundCollision(const World* world)
			: Collision(world)
		{
		}

		CompoundCollision::CompoundCollision(const World* world, std::vector<OgreNewt::CollisionPtr> col_array, unsigned int id)
			:
			Collision(world)
		{
			if (m_col)
				NewtonDestroyCollision(m_col);
			
			//get the number of elements.
			size_t num = col_array.size();

			// create simple array.
			NewtonCollision** array = new NewtonCollision*[num];

			for (unsigned int i = 0; i < num; i++)
			{
				array[i] = (NewtonCollision*)col_array[i]->getNewtonCollision();
			}

			m_col = NewtonCreateCompoundCollision(world->getNewtonWorld(), id);
			NewtonCompoundCollisionBeginAddRemove(m_col);
			for (const OgreNewt::CollisionPtr col : col_array)
			{
				NewtonCompoundCollisionAddSubCollision(m_col, col->getNewtonCollision());
			}
			NewtonCompoundCollisionEndAddRemove(m_col);
			NewtonCollisionSetUserData(m_col, this);
			delete[] array;
		}

		////////////////////////////////////////////////////////////////////////

		SceneCollision::SceneCollision(const World* world)
			: Collision(world)
		{
			if (nullptr != m_col)
			{
				NewtonDestroyCollision(m_col);
			}


		}

		SceneCollision::~SceneCollision()
		{
		}

		void SceneCollision::AddCollision(CollisionPtr collision, unsigned int id)
		{
			// create simple array.
			NewtonCollision* newtonCollision = (NewtonCollision*)collision->getNewtonCollision();

			if (nullptr == m_col)
			{
				m_col = NewtonCreateSceneCollision(m_world->getNewtonWorld(), id);
			}
			NewtonSceneCollisionBeginAddRemove(m_col);
			
			NewtonSceneCollisionAddSubCollision(m_col, collision->getNewtonCollision());

			NewtonSceneCollisionEndAddRemove(m_col);
			NewtonCollisionSetUserData(m_col, this);
		}

		void SceneCollision::RemoveCollision(CollisionPtr collision)
		{
			NewtonCollision* const newtonCollisionRoot = collision->getNewtonCollision();
			NewtonSceneCollisionBeginAddRemove(m_col);

			NewtonCollision* nullCollision = NewtonCreateNull(m_world->getNewtonWorld());
			NewtonSceneCollisionRemoveSubCollision(newtonCollisionRoot, nullCollision);

			NewtonSceneCollisionEndAddRemove(m_col);
		}

		void SceneCollision::RemoveCollision(NewtonCollision* newtonCollision)
		{
			NewtonSceneCollisionBeginAddRemove(m_col);

			NewtonCollision* nullCollision = NewtonCreateNull(m_world->getNewtonWorld());
			NewtonSceneCollisionRemoveSubCollision(newtonCollision, nullCollision);

			NewtonSceneCollisionEndAddRemove(m_col);
		}

		////////////////////////////////////////////////////////////////////////

		// OgreNewt::CollisionPrimitives::Pyramid
		Pyramid::Pyramid(const World* world) :
			ConvexCollision(world)
		{
		}

		Pyramid::Pyramid(const World* world, const Ogre::Vector3& size, unsigned int id,
			const Ogre::Quaternion& orient, const Ogre::Vector3& pos,
			Ogre::Real tolerance) :
			ConvexCollision(world)
		{
			dFloat matrix[16];

			OgreNewt::Converters::QuatPosToMatrix(orient, pos, &matrix[0]);

			// create a simple pyramid collision primitive using the Newton Convex Hull interface.
			// this function places the center of mass 1/3 up y from the base.

			dFloat* vertices = new dFloat[15];
			unsigned short idx = 0;

			// make the bottom base.
			for (int ix = -1; ix <= 1; ix += 2)
			{
				for (int iz = -1; iz <= 1; iz += 2)
				{
					vertices[idx++] = (size.x / 2.0f) * ix;
					vertices[idx++] = -(size.y / 3.0f);
					vertices[idx++] = (size.z / 2.0f) * iz;
				}
			}

			// make the tip.
			vertices[idx++] = 0.0f;
			vertices[idx++] = (size.y * 2.0f / 3.0f);
			vertices[idx++] = 0.0f;

			//make the collision primitive.
			m_col = NewtonCreateConvexHull(m_world->getNewtonWorld(), 5,
				vertices, sizeof(dFloat) * 3, tolerance, id, &matrix[0]);

			delete[] vertices;
			NewtonCollisionSetUserData(m_col, this);
		}

	}   // end namespace CollisionPrimitives
}   // end namespace OgreNewt
