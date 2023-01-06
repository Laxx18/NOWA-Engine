/*
 * CutMeshGenerator.h
 *
 *  Created on: Apr 30, 2011
 *      Author: _1nsane
 */

#ifndef CUTMESHGENERATOR_H_
#define CUTMESHGENERATOR_H_

#include <Ogre.h>

class CutMeshGenerator {
public:
	/*
	 * generates cut mesh in xy-plane with  [0:1] extents and [0:1] UVs
	 * transformation are then applied
	 */
	static void Generate(Ogre::v1::MeshPtr meshptr, float** aafHeightmap, int iWidth, int iHeight,
			Ogre::Matrix4 mTransform, Ogre::Matrix3 mUVTransform, bool bSmooth);
	/*
	 * generates cut mesh in xy-plane with  [0:1] extents and [0:1] UVs
	 * transformation are then applied
	 */
	static void CreateRandomCutMesh(Ogre::v1::MeshPtr inMesh, Ogre::v1::MeshPtr outMesh, float fRoughness, float fResolution, Ogre::String strMatName, bool bSmooth);
};

#endif /* CUTMESHGENERATOR_H_ */
