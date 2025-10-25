/*
	OgreNewt Library

	Ogre implementation of Newton Game Dynamics SDK

	OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

		by Walaber
		some changes by melven
		adapted for Newton Dynamics 4.0
*/
#ifndef _INCLUDE_OGRENEWT_COLLISIONPRIMITIVES
#define _INCLUDE_OGRENEWT_COLLISIONPRIMITIVES

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_MaterialID.h"

#include "OgreEntity.h"
#include "OgreItem.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{
	//! set of basic collision shapes
	namespace CollisionPrimitives
	{
		//! face-winding enum.
		enum FaceWinding
		{
			FW_DEFAULT, FW_REVERSE
		};

		//! null collision (results in no collision)
		class _OgreNewtExport Null : public OgreNewt::Collision
		{
		public:
			//! constructor
			Null(const World* world);

			//! destructor
			~Null() {}
		};

		//! standard primitive Box.
		class _OgreNewtExport Box : public OgreNewt::ConvexCollision
		{
		public:
			//! constructor
			Box(const World* world);

			//! constructor
			Box(const World* world, const Ogre::Vector3& size, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO);

			//! destructor
			~Box() {}
		};

		//! standard primitive Ellipsoid.  
		class _OgreNewtExport Ellipsoid : public OgreNewt::ConvexCollision
		{
		public:
			//! constructor
			Ellipsoid(const World* world);

			//! constructor
			Ellipsoid(const World* world, const Ogre::Vector3& size, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO);

			//! destructor
			~Ellipsoid() {}
		};

		//! standard primitive cylinder.
		class _OgreNewtExport Cylinder : public OgreNewt::ConvexCollision
		{
		public:
			//! constructor
			Cylinder(const World* world);

			//! constructor
			Cylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO);

			//! destructor
			~Cylinder() {}
		};

		//! standard primitive capsule.
		class _OgreNewtExport Capsule : public OgreNewt::ConvexCollision
		{
		public:
			//! constructor
			Capsule(const World* world);

			//! constructor
			Capsule(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO);

			Capsule(const World* world, const Ogre::Vector3& size, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO);

			//! destructor
			~Capsule() {}
		};

		//! standard primitive cone.
		class _OgreNewtExport Cone : public OgreNewt::ConvexCollision
		{
		public:
			//! constructor
			Cone(const World* world);

			//! constructor
			Cone(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO);

			//! destructor
			~Cone() {}
		};

		//! filled-donut shape primitive.
		class _OgreNewtExport ChamferCylinder : public OgreNewt::ConvexCollision
		{
		public:
			//! constructor
			ChamferCylinder(const World* world);

			//! constructor
			ChamferCylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO);

			//! destructor
			~ChamferCylinder() {}
		};

		//! ConvexHull primitive
		class _OgreNewtExport ConvexHull : public OgreNewt::ConvexCollision
		{
		public:
			//! constructor
			ConvexHull(const World* world);

			//! constructor
			ConvexHull(const World* world, Ogre::v1::Entity* ent, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO,
				Ogre::Real tolerance = 0.001f, const Ogre::Vector3& forceScale = Ogre::Vector3::ZERO);

			//! constructor
			ConvexHull(const World* world, Ogre::Item* item, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO,
				Ogre::Real tolerance = 0.001f, const Ogre::Vector3& forceScale = Ogre::Vector3::ZERO);

			//! constructor
			ConvexHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO,
				Ogre::Real tolerance = 0.001f);

			//! destructor
			~ConvexHull() {}
		};

		//! ConcaveHull primitive
		class _OgreNewtExport ConcaveHull : public OgreNewt::ConvexCollision
		{
		public:
			ConcaveHull(const World* world);
			ConcaveHull(const World* world, Ogre::v1::Entity* ent, unsigned int id, Ogre::Real tolerance = 0.001f,
				const Ogre::Vector3& scale = Ogre::Vector3::UNIT_SCALE);
			ConcaveHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id, Ogre::Real tolerance = 0.001f);
			~ConcaveHull() {}
		};

		//! TreeCollision - complex polygonal collision
		class _OgreNewtExport TreeCollision : public OgreNewt::Collision
		{
		public:
			//! constructor
			TreeCollision(const World* world);

			//! constructor
			TreeCollision(const World* world, Ogre::v1::Entity* ent, bool optimize, unsigned int id, FaceWinding fw = FW_DEFAULT);

			TreeCollision(const World* world, Ogre::Item* item, bool optimize, unsigned int id, FaceWinding fw = FW_DEFAULT);

			//! constructor
			TreeCollision(const World* world, int numVertices, int numIndices, const float* vertices, const int* indices,
				bool optimize, unsigned int id, FaceWinding fw = FW_DEFAULT);

			//! constructor
			TreeCollision(const World* world, int numVertices, Ogre::Vector3* vertices, Ogre::v1::IndexData* indexData,
				bool optimize, unsigned int id, FaceWinding fw = FW_DEFAULT);

			//! destructor
			virtual ~TreeCollision() {}

			//! start a tree collision creation
			void start(unsigned int id);

			//! add a poly to the tree collision
			void addPoly(Ogre::Vector3* polys, unsigned int id);

			//! finish the tree collision
			void finish(bool optimize);

			//! set RayCastCallback active/disabled
			void setRayCastCallbackActive(bool active = true)
			{
				setRayCastCallbackActive(active, m_col);
			}

			//! Change the user defined collision attribute stored with faces
			void setFaceId(unsigned int faceId);

			//! used internally
			static int CountFaces(void* const context, const ndFloat32* const polygon, ndInt32 strideInBytes,
				const ndInt32* const indexArray, ndInt32 indexCount);

			static int MarkFaces(void* const context, const ndFloat32* const polygon, ndInt32 strideInBytes,
				const ndInt32* const indexArray, ndInt32 indexCount);

		private:
			static void setRayCastCallbackActive(bool active, ndShape* col);

		private:
			int m_faceCount;
			unsigned int m_categoryId;
		};

		//! HeightField Collision
		class _OgreNewtExport HeightField : public Collision
		{
		public:
			HeightField(const World* world);
			HeightField(const World* world, int width, int height, Ogre::Real* elevationMap,
				Ogre::Real verticleScale, Ogre::Real horizontalScaleX, Ogre::Real horizontalScaleZ,
				const Ogre::Vector3& position, const Ogre::Quaternion& orientation, unsigned int shapeID);
			~HeightField() {}

			//! Change the user defined collision attribute
			void setFaceId(unsigned int faceId);

			void setRayCastCallbackActive(bool active = true)
			{
				setRayCastCallbackActive(active, m_col);
			}

		private:
			static void setRayCastCallbackActive(bool active, ndShape* col);

		private:
			int m_faceCount;
			unsigned int m_categoryId;
		};

		//! TreeCollision created by parsing a tree of SceneNodes
		class _OgreNewtExport TreeCollisionSceneParser : public TreeCollision
		{
		public:
			TreeCollisionSceneParser(OgreNewt::World* world);
			~TreeCollisionSceneParser() {}

			//! parse the scene.
			void parseScene(Ogre::SceneNode* startNode, unsigned int id, bool optimize = true, FaceWinding fw = FW_DEFAULT);

		protected:
			//! filter which Entities will be added
			virtual bool entityFilter(const Ogre::SceneNode* currentNode, const Ogre::v1::Entity* currentEntity, FaceWinding& fw)
			{
				return true;
			}

			//! get ID for this group of polygons
			virtual unsigned int getID(const Ogre::SceneNode* currentNode, const Ogre::v1::Entity* currentEntity, unsigned int currentSubMesh)
			{
				return count++;
			}

		private:
			//! recursive function to parse a single scene node.
			void _parseNode(Ogre::SceneNode* node, const Ogre::Quaternion& curOrient, const Ogre::Vector3& curPos,
				const Ogre::Vector3& curScale, FaceWinding fw, unsigned int id);

			static int count;
		};

		//! create a compound from several collision pieces.
		class _OgreNewtExport CompoundCollision : public OgreNewt::Collision
		{
		public:
			//! constructor
			CompoundCollision(const World* world);

			//! constructor
			CompoundCollision(const World* world, std::vector<OgreNewt::CollisionPtr> col_array, unsigned int id);

			//! destructor
			~CompoundCollision() {}
		};

		//! Pyramid primitive
		class _OgreNewtExport Pyramid : public OgreNewt::ConvexCollision
		{
		public:
			//! constructor
			Pyramid(const World* world);

			//! constructor
			Pyramid(const World* world, const Ogre::Vector3& size, unsigned int id,
				const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& pos = Ogre::Vector3::ZERO,
				Ogre::Real tolerance = 0.001f);

			//! destructor
			~Pyramid() {}
		};

	}   // end namespace CollisionPrimitives

}// end namespace OgreNewt

#endif  // _INCLUDE_OGRENEWT_COLLISIONPRIMITIVES
