/*
Based on MeshSlice
*/

#ifndef ANIMATION_SCISSOR_H
#define ANIMATION_SCISSOR_H

#include "defines.h"

// Lots of experiments that did not work
#if 0

namespace NOWA
{
	/*class MeshVertex
	{
	public:
		MeshVertex()
			: position(Ogre::Vector3::ZERO),
			texCoord(Ogre::Vector2::ZERO)
		{

		}

		MeshVertex(const Ogre::Vector3& position, const Ogre::Vector2& texCoord)
			: position(position),
			texCoord(texCoord)
		{
			
		}

		void setPosition(const Ogre::Vector3& position)
		{
			this->position = position;
		}

		Ogre::Vector3 getPosition(void) const
		{
			return this->position;
		}

		void setTextureCoordinate(const Ogre::Vector2& texCoord)
		{
			this->texCoord = texCoord;
		}

		Ogre::Vector2 getTextureCoordinate(void) const
		{
			return this->texCoord;
		}

	private:
		Ogre::Vector3 position;
		Ogre::Vector2 texCoord;
	};

	class MeshIndex
	{
	public:
		MeshIndex(unsigned short faceA, unsigned short faceB, unsigned short faceC)
			: faceA(faceA),
			faceB(faceB),
			faceC(faceC)
		{
			
		}

		void setFaceA(unsigned short faceA)
		{
			this->faceA = faceA;
		}

		unsigned short getFaceA(void) const
		{
			return this->faceA;
		}

		void setFaceB(unsigned short faceB)
		{
			this->faceB = faceB;
		}

		unsigned short getFaceB(void) const
		{
			return this->faceB;
		}

		void setFaceC(unsigned short faceC)
		{
			this->faceC = faceC;
		}

		unsigned short getFaceC(void) const
		{
			return this->faceC;
		}
		
	private:
		unsigned short faceA;
		unsigned short faceB;
		unsigned short faceC;
	};*/

	class SubMeshBlock
	{
	public:
		SubMeshBlock()
			: vertexCount(0),
			indexCount(0),
			minPoint(Ogre::Vector3::ZERO),
			maxPoint(Ogre::Vector3::ZERO)
		{

		}

		void setMaterialName(const Ogre::String& materialName)
		{
			this->materialName = materialName;
		}

		Ogre::String getMaterialName(void) const
		{
			return this->materialName;
		}

		void setVertexCount(unsigned int vertexCount)
		{
			this->vertexCount = vertexCount;
		}

		unsigned int getVertexCount(void) const
		{
			return this->vertexCount;
		}

		std::vector<Ogre::Vector3>& getPosVertices(void)
		{
			return this->posVertices;
		}

		std::vector<Ogre::Vector3>& getPosNormals(void)
		{
			return this->posNormals;
		}

		std::vector<Ogre::Vector2>& getTexVertices(void)
		{
			return this->texVertices;
		}

		void setIndexCount(unsigned int indexCount)
		{
			this->indexCount = indexCount;
		}

		unsigned int getIndexCount(void) const
		{
			return this->indexCount;
		}

		std::vector<unsigned long>& getIndices(void)
		{
			return this->indices;
		}

		void setMinPoint(const Ogre::Vector3& minPoint)
		{
			this->minPoint = minPoint;
		}

		Ogre::Vector3 getMinPoint(void) const
		{
			return this->minPoint;
		}

		void setMaxPoint(const Ogre::Vector3& maxPoint)
		{
			this->maxPoint = maxPoint;
		}

		Ogre::Vector3 getMaxPoint(void) const
		{
			return this->maxPoint;
		}
		
	private:
		Ogre::String materialName;
		unsigned int vertexCount;
		std::vector<Ogre::Vector3> posVertices;
		std::vector<Ogre::Vector3> posNormals;
		std::vector<Ogre::Vector2> texVertices;
		unsigned int indexCount;
		std::vector<unsigned long> indices;
		Ogre::Vector3 minPoint;
		Ogre::Vector3 maxPoint;
	};
	
	class EXPORTED MeshScissor
	{
	public:
		typedef boost::shared_ptr<SubMeshBlock> SubMeshBlockPtr;
	public:
		MeshScissor(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, const Ogre::Vector3& position);
		~MeshScissor();

		void rotateScissorPlane(Ogre::Real offsetX, Ogre::Real offsetY);

		void moveScissorPlane(Ogre::Real offsetX, Ogre::Real offsetY);

		void setVisibleScissorPlane(bool visible);

		std::pair<Ogre::MeshPtr, Ogre::MeshPtr> split(Ogre::MeshPtr meshPtr, Ogre::SceneNode* sceneNode);
	private:
		Ogre::MeshPtr generateMesh(const Ogre::String& name, const std::vector<NOWA::MeshScissor::SubMeshBlockPtr>& subMeshBlockList);
		
		void slice(Ogre::MeshPtr meshPtr, Ogre::SceneNode* sceneNode, std::vector<SubMeshBlockPtr>& outUpperMeshBlockList, std::vector<SubMeshBlockPtr>& outLowerMeshBlockList);
		
		void calculateSplitFaceVertex(Ogre::Plane* tempPlane, std::vector<Ogre::Vector3>& intersectPosVerticesList, 
			std::vector<Ogre::Vector2>& vIntersectTexVerticesList, SubMeshBlockPtr& upperSplitFaceBlock, SubMeshBlockPtr& lowerSplitFaceBlock);

		bool checkIntersectPointPolygonBetweenLine(Ogre::Vector3* vTempPlanePoint, const Ogre::Vector3& faceA, const Ogre::Vector3& faceB, 
			const Ogre::Vector2& texA, const Ogre::Vector2& texB, Ogre::Vector3& intersectPointAB, Ogre::Vector2& texVertexAB);
		
		void getMeshInformation(Ogre::MeshPtr mesh, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale, 
			std::vector<SubMeshBlockPtr>& outSubMeshBlock);
	private:
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;

		Ogre::MaterialPtr scissorPlaneMaterial;
		Ogre::ManualObject* scissorPlaneObject;
		Ogre::SceneNode* scissorPlaneNode;

		Ogre::Vector3 planeVertices[4]; // SliceFace Vertex Info
		unsigned int planeIndices[6]; // SliceFace Index Info
	};

}; // namespace end


#endif

#endif