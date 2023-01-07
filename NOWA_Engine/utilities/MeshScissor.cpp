#include "NOWAPrecompiled.h"
#include "MeshScissor.h"
#include <iterator>

#if 0

namespace NOWA
{
	MeshScissor::MeshScissor(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, const Ogre::Vector3& position)
		: sceneManager(sceneManager),
		camera(camera),
		scissorPlaneObject(nullptr),
		scissorPlaneNode(nullptr)
	{
		// http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Manual+Resource+Loading&structure=Tutorials
		// Input the slicing face information.
		this->planeVertices[0] = Ogre::Vector3(-500.0f, 0.0f, 500.0f);
		this->planeVertices[1] = Ogre::Vector3(-500.0f, 0.0f, -500.0f);
		this->planeVertices[2] = Ogre::Vector3(500.0f, 0.0f, -500.0f);
		this->planeVertices[3] = Ogre::Vector3(500.0f, 0.0f, 500.0f);
		this->planeIndices[0] = 0; this->planeIndices[1] = 2; this->planeIndices[2] = 1;
		this->planeIndices[3] = 0; this->planeIndices[4] = 3; this->planeIndices[5] = 2;

		// create slicing face render object
		this->scissorPlaneMaterial = Ogre::MaterialManager::getSingletonPtr()->create("Scissor.Plane", "General");
		Ogre::Pass* pass = this->scissorPlaneMaterial->getTechnique(0)->getPass(0);
		pass->setCullingMode(Ogre::CullingMode::CULL_NONE);
		pass->setLightingEnabled(false);
		pass->setAmbient(Ogre::ColourValue(1.0f, 0.0f, 0.0f, 0.3f));
		pass->setDiffuse(Ogre::ColourValue(1.0f, 0.0f, 0.0f, 0.3f));
		pass->setSceneBlending(Ogre::SceneBlendType::SBT_TRANSPARENT_ALPHA);
		pass->setDepthWriteEnabled(false);

		this->scissorPlaneObject = this->sceneManager->createManualObject();
		this->scissorPlaneObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		this->scissorPlaneObject->begin(this->scissorPlaneMaterial->getName(), Ogre::RenderOperation::OT_TRIANGLE_LIST);

		this->scissorPlaneObject->position(this->planeVertices[0]); this->scissorPlaneObject->colour(Ogre::ColourValue(1.0f, 0.0f, 0.0f, 0.3f));
		this->scissorPlaneObject->position(this->planeVertices[1]); this->scissorPlaneObject->colour(Ogre::ColourValue(1.0f, 0.0f, 0.0f, 0.3f));
		this->scissorPlaneObject->position(this->planeVertices[2]); this->scissorPlaneObject->colour(Ogre::ColourValue(1.0f, 0.0f, 0.0f, 0.3f));
		this->scissorPlaneObject->position(this->planeVertices[3]); this->scissorPlaneObject->colour(Ogre::ColourValue(1.0f, 0.0f, 0.0f, 0.3f));
		this->scissorPlaneObject->triangle(this->planeIndices[0], this->planeIndices[1], this->planeIndices[2]);
		this->scissorPlaneObject->triangle(this->planeIndices[3], this->planeIndices[4], this->planeIndices[5]);

		this->scissorPlaneObject->end();
		this->scissorPlaneObject->setCastShadows(false);

		this->scissorPlaneNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
		this->scissorPlaneNode->attachObject(this->scissorPlaneObject);

		this->scissorPlaneNode->setPosition(position);
		this->scissorPlaneNode->setVisible(false);
	}

	MeshScissor::~MeshScissor()
	{
		if (this->scissorPlaneNode != nullptr)
		{
			this->scissorPlaneNode->detachAllObjects();
			if (this->scissorPlaneObject != nullptr)
			{
				this->sceneManager->destroyManualObject(this->scissorPlaneObject);
				this->scissorPlaneObject = nullptr;
			}
			this->scissorPlaneNode->getParentSceneNode()->removeAndDestroyChild(this->scissorPlaneNode->getName());
			this->scissorPlaneNode = nullptr;
		}
	}

	void MeshScissor::rotateScissorPlane(Ogre::Real offsetX, Ogre::Real offsetY)
	{
		Ogre::Real tempOffsetX = offsetX * 0.1f;
		Ogre::Real tempOffsetY = offsetY * 0.1f;

		Ogre::Quaternion rotX = Ogre::Quaternion(Ogre::Degree(tempOffsetX), this->camera->getUp());
		Ogre::Quaternion rotY = Ogre::Quaternion(Ogre::Degree(tempOffsetY), this->camera->getRight());

		this->scissorPlaneNode->rotate(rotX, Ogre::Node::TS_WORLD);
		this->scissorPlaneNode->rotate(rotY, Ogre::Node::TS_WORLD);
	}

	void MeshScissor::moveScissorPlane(Ogre::Real offsetX, Ogre::Real offsetY)
	{
		Ogre::Real tempOffsetX = offsetX * 0.1f;
		Ogre::Real tempOffsetY = offsetY * 0.1f;
		// Is this correct?  this->scissorPlaneNode.Position += (mpCamera.Right * fOffsetX) + (mpCamera.Up * fOffsetY);
		this->scissorPlaneNode->translate((this->camera->getRight() * tempOffsetX) + (this->camera->getUp() * tempOffsetY));
	}

	Ogre::MeshPtr MeshScissor::generateMesh(const Ogre::String& name, const std::vector<NOWA::MeshScissor::SubMeshBlockPtr>& subMeshBlockList)
	{
		// see: http://www.ogre3d.org/tikiwiki/Manual+Resource+Loading
		Ogre::MeshPtr meshPtr = Ogre::MeshManager::getSingleton().createManual(name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		for (int i = 0; i < static_cast<int>(subMeshBlockList.size()); i++)
		{
			Ogre::SubMesh* subMesh = meshPtr->createSubMesh();

			Ogre::VertexData* vertexData = new Ogre::VertexData();
			subMesh->useSharedVertices = false;
			subMesh->vertexData = vertexData;

			// define the vertex format
			Ogre::VertexDeclaration* vertexDecl = vertexData->vertexDeclaration;
			size_t currOffset = 0;
			// positions
			vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
			currOffset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
			// normals: What is with normals?? Problem: they must come directly after vertex position in one container!
			// vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
			// currOffset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);

			// allocate the vertex buffer
			vertexData->vertexCount = subMeshBlockList[i]->getVertexCount();
			Ogre::HardwareVertexBufferSharedPtr vBuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(/*vertexDecl->getVertexSize(0)*/currOffset, vertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
			
			// do not know if that works
			vBuf->writeData(0, vBuf->getSizeInBytes(), &(subMeshBlockList[i]->getPosVertices()[0]), true);

			// does not work:
			/*float* pVertex = static_cast<float*>(vBuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
			for (size_t j = 0; j < subMeshBlockList[i]->getPosVertices().size(); j++)
			{
				*pVertex++ = subMeshBlockList[i]->getPosVertices()[j].x;
				*pVertex++ = subMeshBlockList[i]->getPosVertices()[j].y;
				*pVertex++ = subMeshBlockList[i]->getPosVertices()[j].z;
			}
			vBuf->unlock();*/

			/// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
			Ogre::VertexBufferBinding* bind = vertexData->vertexBufferBinding;
			bind->setBinding(0, vBuf);

			// 2nd buffer
			currOffset = 0;
			// two dimensional texture coordinates
			vertexDecl->addElement(1, currOffset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0);
			currOffset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
			/// Allocate vertex buffer of the requested number of vertices (vertexCount) 
			/// and bytes per vertex (offset)
			vBuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
				currOffset, vertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
			/// Upload the vertex data to the card
			vBuf->writeData(0, vBuf->getSizeInBytes(), &(subMeshBlockList[i]->getTexVertices()[0]), true);

			/*float* pTexVertex = static_cast<float*>(vBuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
			for (size_t j = 0; j < subMeshBlockList[i]->getTexVertices().size(); j++)
			{
				*pTexVertex++ = subMeshBlockList[i]->getTexVertices()[j].x;
				*pTexVertex++ = subMeshBlockList[i]->getTexVertices()[j].y;
			}
			vBuf->unlock();*/

			/// Set vertex buffer binding so buffer 1 is bound to our colour buffer
			bind->setBinding(1, vBuf);

			/// Allocate index buffer of the requested number of vertices (ibufCount) 
			Ogre::HardwareIndexBufferSharedPtr ibuf = Ogre::HardwareBufferManager::getSingleton().
				createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, subMeshBlockList[i]->getIndexCount(), Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

			/// Set parameters of the submesh
			subMesh->indexData->indexBuffer = ibuf;
			subMesh->indexData->indexCount = subMeshBlockList[i]->getIndexCount();
			subMesh->indexData->indexStart = 0;

			/// Upload the index data to the card
			ibuf->writeData(0, ibuf->getSizeInBytes(), &(subMeshBlockList[i]->getIndices()[0]), true);

			/*unsigned long* pIndices = static_cast<unsigned long*>(vBuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
			for (size_t j = 0; j < subMeshBlockList[i]->getIndices().size(); j++)
			{
				*pIndices++ = subMeshBlockList[i]->getIndices()[j];
			}
			vBuf->unlock();*/
		}

		/// Set bounding information (for culling)
		// here merge min max of all block lists!
		meshPtr->_setBounds(Ogre::AxisAlignedBox(subMeshBlockList[0]->getMinPoint(), subMeshBlockList[0]->getMaxPoint()));
		Ogre::Real radius = std::max(subMeshBlockList[0]->getMaxPoint().x - subMeshBlockList[0]->getMinPoint().x,
			std::max(subMeshBlockList[0]->getMaxPoint().y - subMeshBlockList[0]->getMinPoint().y, subMeshBlockList[0]->getMaxPoint().z - subMeshBlockList[0]->getMinPoint().z)) / 2.0f;
		meshPtr->_setBoundingSphereRadius(radius);
		
		/// Notify -Mesh object that it has been loaded
		meshPtr->load();

		return std::move(meshPtr);
	}

	std::pair<Ogre::MeshPtr, Ogre::MeshPtr> MeshScissor::split(Ogre::MeshPtr meshPtr, Ogre::SceneNode* sceneNode)
	{
		std::vector<NOWA::MeshScissor::SubMeshBlockPtr> outUpperMeshBlockList;
		std::vector<NOWA::MeshScissor::SubMeshBlockPtr> outLowerMeshBlockList;
		this->slice(meshPtr, sceneNode, outUpperMeshBlockList, outLowerMeshBlockList);

		// Now create the upper and lower mesh from the lists

		// 1. Upper mesh
		// After a slice, there is only one mesh and sub mesh is the mesh
		Ogre::MeshPtr upperMeshPtr = this->generateMesh(meshPtr->getName() + "_upper", outUpperMeshBlockList);
		Ogre::MeshPtr lowerMeshPtr = this->generateMesh(meshPtr->getName() + "_lower", outLowerMeshBlockList);

		return std::make_pair(upperMeshPtr, lowerMeshPtr);
	}

	void MeshScissor::slice(Ogre::MeshPtr meshPtr, Ogre::SceneNode* sceneNode, std::vector<SubMeshBlockPtr>& outUpperMeshBlockList,
		std::vector<SubMeshBlockPtr>& outLowerMeshBlockList)
	{
		// set invisible target model
		sceneNode->setVisible(false);

		std::vector<SubMeshBlockPtr> subMeshBlockList;
		this->getMeshInformation(meshPtr, sceneNode->getPosition(), sceneNode->getOrientation(), sceneNode->getScale(), subMeshBlockList);

		// transformation slicing face coordinate local to world
		Ogre::Matrix4 nodeTransform = this->scissorPlaneNode->_getFullTransform();
		Ogre::Vector3 tempPlanePoint[4];
		
		for (int nIdx = 0; nIdx < 4; nIdx++)
		{
			Ogre::Vector3 pos = this->planeVertices[nIdx];
			tempPlanePoint[nIdx] = nodeTransform * pos;
		}
		Ogre::Plane tempPlane(tempPlanePoint[0], tempPlanePoint[2], tempPlanePoint[1]);

		// slicing face
		std::vector<SubMeshBlockPtr> upperMeshBlockList;
		std::vector<SubMeshBlockPtr> lowerMeshBlockList;
		std::vector<Ogre::Vector3> upperVertices;
		std::vector<Ogre::Vector3> upperNormals;
		std::vector<Ogre::Vector2> upperTexVertices;
		std::vector<Ogre::Vector3> lowerVertices;
		std::vector<Ogre::Vector3> lowerNormals;
		std::vector<Ogre::Vector2> lowerTexVertices;
		std::vector<Ogre::Vector3> intersectPosVerticesList;
		std::vector<Ogre::Vector2> intersectTexVerticesList;

		// calculate each polygon intersect by slicing face(plane)
		for (int nMeshBlockIdx = 0; nMeshBlockIdx < static_cast<int>(subMeshBlockList.size()); nMeshBlockIdx++)
		{
			unsigned int nTriCount = subMeshBlockList[nMeshBlockIdx]->getIndexCount() / 3;
			for (unsigned int nTdx = 0; nTdx < nTriCount; nTdx++)
			{
				// Triangle ABC
				Ogre::Vector3 faceA = Ogre::Vector3(subMeshBlockList[nMeshBlockIdx]->getPosVertices()[(int)subMeshBlockList[nMeshBlockIdx]->getIndices()[(int)nTdx * 3 + 0]]);
				Ogre::Vector3 faceB = Ogre::Vector3(subMeshBlockList[nMeshBlockIdx]->getPosVertices()[(int)subMeshBlockList[nMeshBlockIdx]->getIndices()[(int)nTdx * 3 + 1]]);
				Ogre::Vector3 faceC = Ogre::Vector3(subMeshBlockList[nMeshBlockIdx]->getPosVertices()[(int)subMeshBlockList[nMeshBlockIdx]->getIndices()[(int)nTdx * 3 + 2]]);
				Ogre::Vector2 texA = Ogre::Vector2(subMeshBlockList[nMeshBlockIdx]->getTexVertices()[(int)subMeshBlockList[nMeshBlockIdx]->getIndices()[(int)nTdx * 3 + 0]]);
				Ogre::Vector2 texB = Ogre::Vector2(subMeshBlockList[nMeshBlockIdx]->getTexVertices()[(int)subMeshBlockList[nMeshBlockIdx]->getIndices()[(int)nTdx * 3 + 1]]);
				Ogre::Vector2 texC = Ogre::Vector2(subMeshBlockList[nMeshBlockIdx]->getTexVertices()[(int)subMeshBlockList[nMeshBlockIdx]->getIndices()[(int)nTdx * 3 + 2]]);

				// triangle point A->B
				Ogre::Vector3 intersectPointAB = Ogre::Vector3::ZERO;
				Ogre::Vector2 intersectTexPointAB = Ogre::Vector2::ZERO;

				bool bIsABIntersect = this->checkIntersectPointPolygonBetweenLine(tempPlanePoint, faceA, faceB, texA, texB, intersectPointAB, intersectTexPointAB);
				if (bIsABIntersect == true)
				{
					bool exist = false;
					for (auto& intersectPosVertices : intersectPosVerticesList)
					{
						if (intersectPointAB == intersectPosVertices)
						{
							if (intersectPosVertices.positionEquals(intersectPointAB, 0.0001f) == true)
							{
								exist = true;
							}
						}
					}
					if (false == exist)
					{
						intersectPosVerticesList.emplace_back(intersectPointAB);
						intersectTexVerticesList.emplace_back(intersectTexPointAB);
					}
				}

				// triangle point B->C
				Ogre::Vector3 intersectPointBC = Ogre::Vector3::ZERO;
				Ogre::Vector2 intersectTexPointBC = Ogre::Vector2::ZERO;
				bool bIsBCIntersect = this->checkIntersectPointPolygonBetweenLine(tempPlanePoint, faceB, faceC, texB, texC, intersectPointBC, intersectTexPointBC);
				if (bIsBCIntersect == true)
				{
					bool exist = false;
					for (auto& intersectPosVertices : intersectPosVerticesList)
					{
						if (intersectPointBC == intersectPosVertices)
						{
							if (intersectPosVertices.positionEquals(intersectPointBC, 0.0001f) == true)
							{
								exist = true;
							}
						}
					}
					if (false == exist)
					{
						intersectPosVerticesList.emplace_back(intersectPointBC);
						intersectTexVerticesList.emplace_back(intersectTexPointBC);
					}
				}

				// triangle point C->A
				Ogre::Vector3 intersectPointCA = Ogre::Vector3::ZERO;
				Ogre::Vector2 intersectTexPointCA = Ogre::Vector2::ZERO;
				bool bIsCAIntersect = this->checkIntersectPointPolygonBetweenLine(tempPlanePoint, faceC, faceA, texC, texA, intersectPointCA, intersectTexPointCA);
				if (bIsCAIntersect == true)
				{
					bool exist = false;
					for (auto& intersectPosVertices : intersectPosVerticesList)
					{
						if (intersectPointCA == intersectPosVertices)
						{
							if (intersectPosVertices.positionEquals(intersectPointCA, 0.0001f) == true)
							{
								exist = true;
							}
						}
					}
					if (false == exist)
					{
						intersectPosVerticesList.emplace_back(intersectPointCA);
						intersectTexVerticesList.emplace_back(intersectTexPointCA);
					}
				}

				// if this triangle is no intersected.
				if (bIsABIntersect == false && bIsBCIntersect == false && bIsCAIntersect == false)
				{
					// decide the triangle location by triangle point A
					Ogre::Plane::Side enSide = tempPlane.getSide(faceA);
					if (enSide == Ogre::Plane::POSITIVE_SIDE)
					{
						upperVertices.emplace_back(faceA);
						upperVertices.emplace_back(faceB);
						upperVertices.emplace_back(faceC);

						/*Ogre::Vector3 dir0 = faceC - faceA;
						Ogre::Vector3 dir1 = faceA - faceB;
						Ogre::Vector3 normal = dir0.crossProduct(dir1).normalisedCopy();
						upperNormals.emplace_back(normal);
						upperNormals.emplace_back(normal);
						upperNormals.emplace_back(normal);*/

						upperTexVertices.emplace_back(texA);
						upperTexVertices.emplace_back(texB);
						upperTexVertices.emplace_back(texC);
					}
					else if (enSide == Ogre::Plane::NEGATIVE_SIDE)
					{
						lowerVertices.emplace_back(faceA);
						lowerVertices.emplace_back(faceB);
						lowerVertices.emplace_back(faceC);

						/*Ogre::Vector3 dir0 = faceC - faceA;
						Ogre::Vector3 dir1 = faceA - faceB;
						Ogre::Vector3 normal = dir0.crossProduct(dir1).normalisedCopy();
						lowerNormals.emplace_back(normal);
						lowerNormals.emplace_back(normal);
						lowerNormals.emplace_back(normal);*/

						lowerTexVertices.emplace_back(texA);
						lowerTexVertices.emplace_back(texB);
						lowerTexVertices.emplace_back(texC);
					}
				}
				else // intersected(AB or BC or CA)
				{
					// calculate upper polygon info and lower polygon info
					if (bIsABIntersect == false && bIsBCIntersect == true && bIsCAIntersect == true)
					{
						if (tempPlane.getSide(faceC) == Ogre::Plane::POSITIVE_SIDE)
						{
							// Position Vertex
							upperVertices.emplace_back(faceC); // a
							upperVertices.emplace_back(intersectPointCA); // b
							upperVertices.emplace_back(intersectPointBC); // c

							/*Ogre::Vector3 dir0 = intersectPointBC - faceC;
							Ogre::Vector3 dir1 = faceC - intersectPointCA;
							Ogre::Vector3 normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/

							lowerVertices.emplace_back(intersectPointCA); // a
							lowerVertices.emplace_back(faceA); // b
							lowerVertices.emplace_back(faceB); // c

							/*dir0 = faceB - intersectPointCA;
							dir1 = intersectPointCA - faceA;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							lowerVertices.emplace_back(intersectPointCA); // a
							lowerVertices.emplace_back(faceB); // b
							lowerVertices.emplace_back(intersectPointBC); // c

							/*dir0 = intersectPointBC - intersectPointCA;
							dir1 = intersectPointCA - faceB;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							// Texture Vertex
							upperTexVertices.emplace_back(texC);
							upperTexVertices.emplace_back(intersectTexPointCA);
							upperTexVertices.emplace_back(intersectTexPointBC);

							lowerTexVertices.emplace_back(intersectTexPointCA);
							lowerTexVertices.emplace_back(texA);
							lowerTexVertices.emplace_back(texB);
							lowerTexVertices.emplace_back(intersectTexPointCA);
							lowerTexVertices.emplace_back(texB);
							lowerTexVertices.emplace_back(intersectTexPointBC);
						}
						else
						{
							lowerVertices.emplace_back(faceC); // a
							lowerVertices.emplace_back(intersectPointCA); // b
							lowerVertices.emplace_back(intersectPointBC); // c

							/*Ogre::Vector3 dir0 = intersectPointBC - faceC;
							Ogre::Vector3 dir1 = faceC - intersectPointCA;
							Ogre::Vector3 normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							upperVertices.emplace_back(intersectPointCA); // a
							upperVertices.emplace_back(faceA); // b
							upperVertices.emplace_back(faceB); // c

							/*dir0 = faceB - intersectPointCA;
							dir1 = intersectPointCA - faceA;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/


							upperVertices.emplace_back(intersectPointCA); // a
							upperVertices.emplace_back(faceB); // b
							upperVertices.emplace_back(intersectPointBC); // c

							/*dir0 = intersectPointBC - intersectPointCA;
							dir1 = intersectPointCA - faceB;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/


							lowerTexVertices.emplace_back(texC);
							lowerTexVertices.emplace_back(intersectTexPointCA);
							lowerTexVertices.emplace_back(intersectTexPointBC);

							upperTexVertices.emplace_back(intersectTexPointCA);
							upperTexVertices.emplace_back(texA);
							upperTexVertices.emplace_back(texB);
							upperTexVertices.emplace_back(intersectTexPointCA);
							upperTexVertices.emplace_back(texB);
							upperTexVertices.emplace_back(intersectTexPointBC);
						}
					}
					else if (bIsABIntersect == true && bIsBCIntersect == true && bIsCAIntersect == false)
					{
						if (tempPlane.getSide(faceB) == Ogre::Plane::POSITIVE_SIDE)
						{
							upperVertices.emplace_back(intersectPointAB); // a
							upperVertices.emplace_back(faceB); // b
							upperVertices.emplace_back(intersectPointBC); // c

							/*Ogre::Vector3 dir0 = intersectPointBC - intersectPointAB;
							Ogre::Vector3 dir1 = intersectPointAB - faceB;
							Ogre::Vector3 normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/

							lowerVertices.emplace_back(faceC); // a
							lowerVertices.emplace_back(faceA); // b
							lowerVertices.emplace_back(intersectPointAB); // c

							/*dir0 = intersectPointAB - faceC;
							dir1 = faceC - faceA;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							lowerVertices.emplace_back(faceC); // a
							lowerVertices.emplace_back(intersectPointAB); // b
							lowerVertices.emplace_back(intersectPointBC); // c

							/*dir0 = intersectPointBC - faceC;
							dir1 = faceC - intersectPointAB;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							upperTexVertices.emplace_back(intersectTexPointAB);
							upperTexVertices.emplace_back(texB);
							upperTexVertices.emplace_back(intersectTexPointBC);

							lowerTexVertices.emplace_back(texC);
							lowerTexVertices.emplace_back(texA);
							lowerTexVertices.emplace_back(intersectTexPointAB);
							lowerTexVertices.emplace_back(texC);
							lowerTexVertices.emplace_back(intersectTexPointAB);
							lowerTexVertices.emplace_back(intersectTexPointBC);
						}
						else
						{
							lowerVertices.emplace_back(intersectPointAB); // a
							lowerVertices.emplace_back(faceB); // b
							lowerVertices.emplace_back(intersectPointBC); // c

							/*Ogre::Vector3 dir0 = intersectPointBC - intersectPointAB;
							Ogre::Vector3 dir1 = intersectPointAB - faceB;
							Ogre::Vector3 normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							upperVertices.emplace_back(faceC); // a
							upperVertices.emplace_back(faceA); // b
							upperVertices.emplace_back(intersectPointAB); // c

							/*dir0 = intersectPointAB - faceC;
							dir1 = faceC - faceA;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/


							upperVertices.emplace_back(faceC); // a
							upperVertices.emplace_back(intersectPointAB); // b
							upperVertices.emplace_back(intersectPointBC); // c

							/*dir0 = intersectPointBC - faceC;
							dir1 = faceC - intersectPointAB;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/


							lowerTexVertices.emplace_back(intersectTexPointAB);
							lowerTexVertices.emplace_back(texB);
							lowerTexVertices.emplace_back(intersectTexPointBC);

							upperTexVertices.emplace_back(texC);
							upperTexVertices.emplace_back(texA);
							upperTexVertices.emplace_back(intersectTexPointAB);
							upperTexVertices.emplace_back(texC);
							upperTexVertices.emplace_back(intersectTexPointAB);
							upperTexVertices.emplace_back(intersectTexPointBC);
						}
					}
					else if (bIsABIntersect == true && bIsBCIntersect == false && bIsCAIntersect == true)
					{
						if (tempPlane.getSide(faceA) == Ogre::Plane::POSITIVE_SIDE)
						{
							upperVertices.emplace_back(intersectPointCA); // a
							upperVertices.emplace_back(faceA); // b
							upperVertices.emplace_back(intersectPointAB); // c

							/*Ogre::Vector3 dir0 = intersectPointAB - intersectPointCA;
							Ogre::Vector3 dir1 = intersectPointCA - faceA;
							Ogre::Vector3 normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/

							lowerVertices.emplace_back(faceC); // a
							lowerVertices.emplace_back(intersectPointCA); // b
							lowerVertices.emplace_back(intersectPointAB); // c

							/*dir0 = intersectPointAB - faceC;
							dir1 = faceC - intersectPointCA;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							lowerVertices.emplace_back(faceC); // a
							lowerVertices.emplace_back(intersectPointAB); // b
							lowerVertices.emplace_back(faceB); // c

							/*dir0 = faceB - faceC;
							dir1 = faceC - intersectPointAB;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							upperTexVertices.emplace_back(intersectTexPointCA);
							upperTexVertices.emplace_back(texA);
							upperTexVertices.emplace_back(intersectTexPointAB);

							lowerTexVertices.emplace_back(texC);
							lowerTexVertices.emplace_back(intersectTexPointCA);
							lowerTexVertices.emplace_back(intersectTexPointAB);
							lowerTexVertices.emplace_back(texC);
							lowerTexVertices.emplace_back(intersectTexPointAB);
							lowerTexVertices.emplace_back(texB);
						}
						else
						{
							lowerVertices.emplace_back(intersectPointCA); // a
							lowerVertices.emplace_back(faceA); // b
							lowerVertices.emplace_back(intersectPointAB); // c

							/*Ogre::Vector3 dir0 = intersectPointAB - intersectPointCA;
							Ogre::Vector3 dir1 = intersectPointCA - faceA;
							Ogre::Vector3 normal = dir0.crossProduct(dir1).normalisedCopy();
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);
							lowerNormals.emplace_back(normal);*/

							upperVertices.emplace_back(faceC); // a
							upperVertices.emplace_back(intersectPointCA); // b
							upperVertices.emplace_back(intersectPointAB); // c

							/*dir0 = intersectPointAB - faceC;
							dir1 = faceC - intersectPointCA;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/

							upperVertices.emplace_back(faceC); // a
							upperVertices.emplace_back(intersectPointAB); // b
							upperVertices.emplace_back(faceB); // c

							/*dir0 = faceB - faceC;
							dir1 = faceC - intersectPointAB;
							normal = dir0.crossProduct(dir1).normalisedCopy();
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);
							upperNormals.emplace_back(normal);*/

							lowerTexVertices.emplace_back(intersectTexPointCA);
							lowerTexVertices.emplace_back(texA);
							lowerTexVertices.emplace_back(intersectTexPointAB);

							upperTexVertices.emplace_back(texC);
							upperTexVertices.emplace_back(intersectTexPointCA);
							upperTexVertices.emplace_back(intersectTexPointAB);
							upperTexVertices.emplace_back(texC);
							upperTexVertices.emplace_back(intersectTexPointAB);
							upperTexVertices.emplace_back(texB);
						}
					}
				}
			}

			// Create the render object by upper slicing mesh info
			if (upperVertices.size() >= 3)
			{
				SubMeshBlockPtr tempUpperMeshBlock = boost::make_shared<SubMeshBlock>();
				tempUpperMeshBlock->setMaterialName(subMeshBlockList[nMeshBlockIdx]->getMaterialName());
				tempUpperMeshBlock->setVertexCount(static_cast<unsigned int>(upperVertices.size()));

				// http://www.ogre3d.org/forums/viewtopic.php?f=2&t=82692
				Ogre::Vector3 minPoint = Ogre::Vector3::ZERO;
				Ogre::Vector3 maxPoint = Ogre::Vector3::ZERO;
				
				minPoint = maxPoint = Ogre::Vector3(upperVertices[0]);
				for (int nVdx = 0; nVdx < static_cast<int>(upperVertices.size()); nVdx++)
				{
					minPoint = Ogre::Vector3(
						std::min(minPoint.x, upperVertices[nVdx].x),
						std::min(minPoint.y, upperVertices[nVdx].y),
						std::min(minPoint.z, upperVertices[nVdx].z));

					maxPoint = Ogre::Vector3(
						std::max(maxPoint.x, upperVertices[nVdx].x),
						std::max(maxPoint.y, upperVertices[nVdx].y),
						std::max(maxPoint.z, upperVertices[nVdx].z));
					
					tempUpperMeshBlock->getPosVertices().emplace_back(upperVertices[nVdx]);
					// tempUpperMeshBlock->getPosNormals().emplace_back(upperNormals[nVdx]);
					tempUpperMeshBlock->getTexVertices().emplace_back(upperTexVertices[nVdx]);
				}
				tempUpperMeshBlock->setMinPoint(minPoint);
				tempUpperMeshBlock->setMaxPoint(maxPoint);
				tempUpperMeshBlock->setIndexCount(static_cast<unsigned int>(upperVertices.size()));
				for (int nVdx = 0; nVdx < static_cast<int>(upperVertices.size()); nVdx++)
				{
					tempUpperMeshBlock->getIndices().emplace_back(static_cast<unsigned int>(nVdx));
				}

				upperMeshBlockList.emplace_back(tempUpperMeshBlock);

				upperVertices.clear();
				upperTexVertices.clear();
			}

			// Create the render object by lower slicing mesh info
			if (lowerVertices.size() >= 3)
			{
				SubMeshBlockPtr tempLowerMeshBlock = boost::make_shared<SubMeshBlock>();
				tempLowerMeshBlock->setMaterialName(subMeshBlockList[nMeshBlockIdx]->getMaterialName());
				tempLowerMeshBlock->setVertexCount(static_cast<unsigned int>(lowerVertices.size()));

				Ogre::Vector3 minPoint = Ogre::Vector3::ZERO;
				Ogre::Vector3 maxPoint = Ogre::Vector3::ZERO;
				
				for (int nVdx = 0; nVdx < static_cast<int>(lowerVertices.size()); nVdx++)
				{
					minPoint = Ogre::Vector3(
						std::min(minPoint.x, lowerVertices[nVdx].x),
						std::min(minPoint.y, lowerVertices[nVdx].y),
						std::min(minPoint.z, lowerVertices[nVdx].z));

					maxPoint = Ogre::Vector3(
						std::max(maxPoint.x, lowerVertices[nVdx].x),
						std::max(maxPoint.y, lowerVertices[nVdx].y),
						std::max(maxPoint.z, lowerVertices[nVdx].z));
					
					tempLowerMeshBlock->getPosVertices().emplace_back(lowerVertices[nVdx]);
					// tempLowerMeshBlock->getPosNormals().emplace_back(lowerNormals[nVdx]);
					tempLowerMeshBlock->getTexVertices().emplace_back(lowerTexVertices[nVdx]);
				}
				tempLowerMeshBlock->setMinPoint(minPoint);
				tempLowerMeshBlock->setMaxPoint(maxPoint);
				tempLowerMeshBlock->setIndexCount(static_cast<unsigned int>(lowerVertices.size()));
				for (int nVdx = 0; nVdx < static_cast<int>(lowerVertices.size()); nVdx++)
				{
					tempLowerMeshBlock->getIndices().emplace_back(static_cast<int>(nVdx));
				}

				lowerMeshBlockList.emplace_back(tempLowerMeshBlock);

				lowerVertices.clear();
				lowerTexVertices.clear();
			}
		}

		// 
		SubMeshBlockPtr upperSplitFaceBlock = nullptr;
		SubMeshBlockPtr lowerSplitFaceBlock = nullptr;
		this->calculateSplitFaceVertex(&tempPlane, intersectPosVerticesList, intersectTexVerticesList, upperSplitFaceBlock, lowerSplitFaceBlock);
		if (upperSplitFaceBlock != nullptr)
		{
			if (upperMeshBlockList.size() >= 1)
			{
				upperSplitFaceBlock->setMaterialName(upperMeshBlockList[0]->getMaterialName());
			}
			upperMeshBlockList.emplace_back(upperSplitFaceBlock);
		}
		if (lowerSplitFaceBlock != nullptr)
		{
			if (lowerMeshBlockList.size() >= 1)
			{
				lowerSplitFaceBlock->setMaterialName(lowerMeshBlockList[0]->getMaterialName());
			}
			lowerMeshBlockList.emplace_back(lowerSplitFaceBlock);
		}
		// outUpperMeshBlockList.emplace_back(upperMeshBlockList);
		// outLowerMeshBlockList.emplace_back(lowerMeshBlockList);
		// for physics mesh
		std::copy(upperMeshBlockList.begin(), upperMeshBlockList.end(), std::back_inserter(outUpperMeshBlockList));
		std::copy(lowerMeshBlockList.begin(), lowerMeshBlockList.end(), std::back_inserter(outLowerMeshBlockList));
	}

	void MeshScissor::setVisibleScissorPlane(bool visible)
	{
		this->scissorPlaneNode->setVisible(visible);
	}

	void MeshScissor::calculateSplitFaceVertex(Ogre::Plane* tempPlane, std::vector<Ogre::Vector3>& intersectPosVerticesList, 
		std::vector<Ogre::Vector2>& intersectTexVerticesList, SubMeshBlockPtr& upperSplitFaceBlock, SubMeshBlockPtr& lowerSplitFaceBlock)
	{
		if (intersectPosVerticesList.size() >= 3)
		{
			upperSplitFaceBlock = boost::make_shared<SubMeshBlock>();
			// upperSplitFaceBlock->setMaterialName("BaseWhiteNoLighting");
			upperSplitFaceBlock->setMaterialName(upperSplitFaceBlock->getMaterialName());

			lowerSplitFaceBlock = boost::make_shared<SubMeshBlock>();
			// lowerSplitFaceBlock->setMaterialName("BaseWhiteNoLighting");
			lowerSplitFaceBlock->setMaterialName(lowerSplitFaceBlock->getMaterialName());

			// center point
			Ogre::Vector3 centerPoint = Ogre::Vector3::ZERO;
			Ogre::AxisAlignedBox aabb;
			for (int nVdx = 0; nVdx < static_cast<int>(intersectPosVerticesList.size()); nVdx++)
			{
				aabb.merge(intersectPosVerticesList[nVdx]);
			}
			//centerPoint = pTempPlane.ProjectVector(aabb.Center) + (-pTempPlane.normal * pTempPlane.d);
			centerPoint = aabb.getCenter();

			// 
			for (int nVdxA = 0; nVdxA < static_cast<int>(intersectPosVerticesList.size()); nVdxA++)
			{
				for (int nVdxB = nVdxA + 1; nVdxB < static_cast<int>(intersectPosVerticesList.size()); nVdxB++)
				{
					Ogre::Vector3 centerToCurrentA = intersectPosVerticesList[nVdxA] - centerPoint;
					centerToCurrentA.normalise();
					Ogre::Vector3 centerToCurrentB = intersectPosVerticesList[nVdxB] - centerPoint;
					centerToCurrentB.normalise();

					float fAngleA = Ogre::Math::ATan2(centerToCurrentA.z, centerToCurrentA.x).valueDegrees();
					float fAngleB = Ogre::Math::ATan2(centerToCurrentB.z, centerToCurrentB.x).valueDegrees();

					if (fAngleA > fAngleB)
					{
						Ogre::Vector3 vTemp = intersectPosVerticesList[nVdxA];
						intersectPosVerticesList[nVdxA] = intersectPosVerticesList[nVdxB];
						intersectPosVerticesList[nVdxB] = vTemp;
					}
				}
			}

			// 
			intersectPosVerticesList.push_back(intersectPosVerticesList[0]);
			unsigned int nUdx = 0;
			unsigned int nLdx = 0;
			for (int nVdx = 0; nVdx < static_cast<int>(intersectPosVerticesList.size()) - 1; nVdx++)
			{
				Ogre::Vector3 faceA = centerPoint;
				Ogre::Vector3 faceB = intersectPosVerticesList[nVdx + 0];
				Ogre::Vector3 faceC = intersectPosVerticesList[nVdx + 1];

				upperSplitFaceBlock->getPosVertices().emplace_back(faceA);
				upperSplitFaceBlock->getPosVertices().emplace_back(faceB);
				upperSplitFaceBlock->getPosVertices().emplace_back(faceC);
				upperSplitFaceBlock->getTexVertices().emplace_back(Ogre::Vector2::ZERO);
				upperSplitFaceBlock->getTexVertices().emplace_back(Ogre::Vector2::ZERO);
				upperSplitFaceBlock->getTexVertices().emplace_back(Ogre::Vector2::ZERO);
				upperSplitFaceBlock->getIndices().emplace_back(nUdx++);
				upperSplitFaceBlock->getIndices().emplace_back(nUdx++);
				upperSplitFaceBlock->getIndices().emplace_back(nUdx++);

				lowerSplitFaceBlock->getPosVertices().emplace_back(faceA);
				lowerSplitFaceBlock->getPosVertices().emplace_back(faceC);
				lowerSplitFaceBlock->getPosVertices().emplace_back(faceB);
				lowerSplitFaceBlock->getTexVertices().emplace_back(Ogre::Vector2::ZERO);
				lowerSplitFaceBlock->getTexVertices().emplace_back(Ogre::Vector2::ZERO);
				lowerSplitFaceBlock->getTexVertices().emplace_back(Ogre::Vector2::ZERO);
				lowerSplitFaceBlock->getIndices().emplace_back(nLdx++);
				lowerSplitFaceBlock->getIndices().emplace_back(nLdx++);
				lowerSplitFaceBlock->getIndices().emplace_back(nLdx++);
			}

			upperSplitFaceBlock->setVertexCount(static_cast<unsigned int>(upperSplitFaceBlock->getPosVertices().size()));
			upperSplitFaceBlock->setIndexCount(static_cast<unsigned int>(upperSplitFaceBlock->getIndices().size()));
			lowerSplitFaceBlock->setVertexCount(static_cast<unsigned int>(lowerSplitFaceBlock->getPosVertices().size()));
			lowerSplitFaceBlock->setIndexCount(static_cast<unsigned int>(lowerSplitFaceBlock->getIndices().size()));
		}
	}

	bool MeshScissor::checkIntersectPointPolygonBetweenLine(Ogre::Vector3* tempPlanePoint, const Ogre::Vector3& faceA, const Ogre::Vector3& faceB, 
		const Ogre::Vector2& texA, const Ogre::Vector2& texB, Ogre::Vector3& intersectPointAB, Ogre::Vector2& texVertexAB)
	{
		bool isIntersect = false;

		// 
		Ogre::Vector3 dir = faceB - faceA;
		Ogre::Real length = dir.length();
		dir.normalise();

		// 
		Ogre::Ray ray(faceA, dir);

		Ogre::Vector3 planePointList[6];
		planePointList[0] = tempPlanePoint[0];
		planePointList[1] = tempPlanePoint[2];
		planePointList[2] = tempPlanePoint[1];
		planePointList[3] = tempPlanePoint[0];
		planePointList[4] = tempPlanePoint[3];
		planePointList[5] = tempPlanePoint[2];

		for (int nIdx = 0; nIdx < 2; nIdx++)
		{
			Ogre::Vector3 planePointA = planePointList[nIdx * 3 + 0];
			Ogre::Vector3 planePointB = planePointList[nIdx * 3 + 1];
			Ogre::Vector3 planePointC = planePointList[nIdx * 3 + 2];

			std::pair<bool, Ogre::Real> result = Ogre::Math::intersects(ray, planePointA, planePointB, planePointC, true, true);
			if (result.first == true)
			{
				// 
				if (result.second < length)
				{
					intersectPointAB = ray.getPoint(result.second);
					isIntersect = true;

					Ogre::Vector2 texDir = texB - texA;
					Ogre::Real texLength = texDir.length();
					texDir.normalise();

					Ogre::Real ratio = result.second / length;
					Ogre::Real texRatioLength = texLength * ratio;
					texVertexAB = texA + (texDir * texRatioLength);

					break;
				}
			}
		}

		return isIntersect;
	}

	void MeshScissor::getMeshInformation(Ogre::MeshPtr mesh, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale,
		std::vector<SubMeshBlockPtr>& outSubMeshBlock)
	{
		SubMeshBlockPtr sharedSubMeshBlock = nullptr;

		for (unsigned short nIdx = 0; nIdx < mesh->getNumSubMeshes(); nIdx++)
		{
			Ogre::SubMesh* subMesh = mesh->getSubMesh(nIdx);

			if (subMesh->useSharedVertices == true)
			{
				if (sharedSubMeshBlock == nullptr)
				{
					Ogre::VertexData* vertexData = mesh->sharedVertexData;
					sharedSubMeshBlock = boost::make_shared<SubMeshBlock>();
					sharedSubMeshBlock->setMaterialName(subMesh->getMaterialName());
					sharedSubMeshBlock->setVertexCount(vertexData->vertexCount);

					const Ogre::VertexElement* posElement = vertexData->vertexDeclaration->findElementBySemantic(Ogre::VertexElementSemantic::VES_POSITION);
					Ogre::HardwareVertexBufferSharedPtr posBuffer = vertexData->vertexBufferBinding->getBuffer(posElement->getSource());
					unsigned char* posVertex = (unsigned char*)posBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
					Ogre::Real* posReal;
					for (int j = 0; j < static_cast<int>(vertexData->vertexCount); ++j, posVertex += posBuffer->getVertexSize())
					{
						posElement->baseVertexPointerToElement(posVertex, &posReal);
						Ogre::Vector3 pt(posReal[0], posReal[1], posReal[2]);
						sharedSubMeshBlock->getPosVertices().emplace_back((orientation * (pt * scale)) + position);
					}
					posBuffer->unlock();

					const Ogre::VertexElement* texElement = vertexData->vertexDeclaration->findElementBySemantic(Ogre::VertexElementSemantic::VES_TEXTURE_COORDINATES);
					Ogre::HardwareVertexBufferSharedPtr texBuffer = vertexData->vertexBufferBinding->getBuffer(texElement->getSource());
					unsigned char* texVertex = (unsigned char*)texBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
					Ogre::Real* texReal;
					for (int j = 0; j < static_cast<int>(vertexData->vertexCount); ++j, texVertex += texBuffer->getVertexSize())
					{
						texElement->baseVertexPointerToElement(texVertex, &texReal);
						Ogre::Vector2 tx(texReal[0], texReal[1]);
						sharedSubMeshBlock->getTexVertices().emplace_back(tx);
					}
					texBuffer->unlock();
				}
				// Attention
				sharedSubMeshBlock->setIndexCount(sharedSubMeshBlock->getIndexCount() + subMesh->indexData->indexCount);
				unsigned int numTriangles = subMesh->indexData->indexCount / 3;
				Ogre::HardwareIndexBufferSharedPtr iBuffer = subMesh->indexData->indexBuffer;
				bool use32BitIndex = (iBuffer->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);
				unsigned int* tempLong = (unsigned int*)iBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
				unsigned short* tempShort = (unsigned short*)tempLong;
				if (use32BitIndex == true)
				{
					for (int k = 0; k < static_cast<int>(subMesh->indexData->indexCount); ++k)
					{
						sharedSubMeshBlock->getIndices().emplace_back((unsigned long)tempLong[k]);
					}
				}
				else
				{
					for (int k = 0; k < static_cast<int>(subMesh->indexData->indexCount); ++k)
						sharedSubMeshBlock->getIndices().emplace_back((unsigned long)tempShort[k]);
				}
				iBuffer->unlock();
			}
			else
			{
				
				Ogre::VertexData* subMeshVertexData = subMesh->vertexData;
				
				SubMeshBlockPtr subMeshBlock = boost::make_shared<SubMeshBlock>();
				subMeshBlock->setMaterialName(subMesh->getMaterialName());
				subMeshBlock->setVertexCount(subMeshVertexData->vertexCount);
				subMeshBlock->setIndexCount(subMesh->indexData->indexCount);
				
				const Ogre::VertexElement* posElement = subMeshVertexData->vertexDeclaration->findElementBySemantic(Ogre::VertexElementSemantic::VES_POSITION);
				Ogre::HardwareVertexBufferSharedPtr posBuffer = subMeshVertexData->vertexBufferBinding->getBuffer(posElement->getSource());
				unsigned char* posVertex = (unsigned char*)posBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
				Ogre::Real* posReal;
				for (int j = 0; j < static_cast<int>(subMeshVertexData->vertexCount); ++j, posVertex += posBuffer->getVertexSize())
				{
					posElement->baseVertexPointerToElement(posVertex, &posReal);
					Ogre::Vector3 pt(posReal[0], posReal[1], posReal[2]);
					subMeshBlock->getPosVertices().emplace_back((orientation * (pt * scale)) + position);
				}
				posBuffer->unlock();

				const Ogre::VertexElement* texElement = subMeshVertexData->vertexDeclaration->findElementBySemantic(Ogre::VertexElementSemantic::VES_TEXTURE_COORDINATES);
				Ogre::HardwareVertexBufferSharedPtr texBuffer = subMeshVertexData->vertexBufferBinding->getBuffer(texElement->getSource());
				unsigned char* texVertex = (unsigned char*)texBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
				Ogre::Real* texReal;
				for (int j = 0; j < static_cast<int>(subMeshVertexData->vertexCount); ++j, texVertex += texBuffer->getVertexSize())
				{
					texElement->baseVertexPointerToElement(texVertex, &texReal);
					Ogre::Vector2 tx(texReal[0], texReal[1]);
					subMeshBlock->getTexVertices().emplace_back(tx);
				}
				texBuffer->unlock();

				unsigned int numTriangles = subMesh->indexData->indexCount / 3;
				Ogre::HardwareIndexBufferSharedPtr iBuffer = subMesh->indexData->indexBuffer;
				bool use32BitIndex = (iBuffer->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);
				unsigned int* tempLong = (unsigned int*)iBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
				unsigned short* tempShort = (unsigned short*)tempLong;
				if (use32BitIndex == true)
				{
					for (int k = 0; k < static_cast<int>(subMesh->indexData->indexCount); ++k)
					{
						subMeshBlock->getIndices().emplace_back((unsigned long)tempLong[k]);
					}
				}
				else
				{
					for (int k = 0; k < static_cast<int>(subMesh->indexData->indexCount); ++k)
					{
						subMeshBlock->getIndices().emplace_back((unsigned long)tempShort[k]);
					}
				}
				iBuffer->unlock();

				outSubMeshBlock.emplace_back(subMeshBlock);
			}
		}

		if (sharedSubMeshBlock != nullptr)
		{
			outSubMeshBlock.emplace_back(sharedSubMeshBlock);
		}
	}

}; //namespace end

#endif
