#ifndef FF_CV_CSGCONVERTER_H
#define FF_CV_CSGCONVERTER_H

#include <string>
#include "Ogre.h"
#include "Solid.h"
#include "ML_Vector.h"
#include "defines.h"

inline mlVector3D ov2sv(const Ogre::Vector3& src)
{
	return mlVector3D(src.x, src.y, src.z);
}

inline Ogre::Vector3 sv2ov(const mlVector3D* src)
{
	return Ogre::Vector3(src->x, src->y, src->z);
}

inline gxColor ouv2scolor(const Ogre::Vector3& src)
{
	return gxColor(src.x*65535.0f, src.y*65535.0f, src.z);
}

inline Ogre::Vector3 scolor2ouv(const gxColor* src)
{
	return Ogre::Vector3((float)src->red/65535.0f, (float)src->green/65535.0f, src->blue);
}

struct svCSGMeshInfo
{
	int	ObjNumber;
	int	MeshNumber;
	int	From;
	int	To;
	std::string MaterialName;
};

#define CV_CSG_MAX_MESH	1000

class EXPORTS cvCSGConverter {
public:

	cvCSGConverter();
	~cvCSGConverter();

	// Ogre Mesh -> Solid
	Solid*	convertMesh2Solid(Ogre::v1::Entity *pEntity, int ObjNum);
	// Solid -> Ogre Mesh
	bool	convertSolid2Mesh(const std::string& Name, Solid *Solid, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2);

	void	setBBMin(Ogre::Vector3 Min) { mBBMin = Min; }
	void	setBBMax(Ogre::Vector3 Max) { mBBMax = Max; }

protected:
	std::string getSubMeshMaterialName(Ogre::v1::Entity *Ent1, int SubMeshNo);
	void	getMeshInformation( const Ogre::v1::MeshPtr mesh,
								size_t &vertex_count,
								Ogre::Vector3* &vertices,
								Ogre::Vector3* &uvs,
								size_t &index_count,
								unsigned long* &indices,
								const Ogre::Vector3 &position,
								const Ogre::Quaternion &orient,
								const Ogre::Vector3 &scale);

	Ogre::Vector3		mBBMin;			// Bounding Box Min/Max
	Ogre::Vector3		mBBMax;
	int					mSubMeshCount;
	svCSGMeshInfo*		mSubMeshInfo[CV_CSG_MAX_MESH];
};

#endif	// FF_CV_CSGCONVERTER_H