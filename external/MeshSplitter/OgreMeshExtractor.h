/*
 * OgreMeshExtractor.h
 *
 *  Created on: Apr 30, 2011
 *      Author: _1nsane
 */

#ifndef OGREMESHEXTRACTOR_H_
#define OGREMESHEXTRACTOR_H_

#include <Ogre.h>

class OgreMeshExtractor {
public:
	static Ogre::String getRandomMeshName();
	/*
	 * extracts a vertex and index buffer containing the VES_POSITION data of all submeshes.
	 * if the mesh uses shared vertices, they will have the vertex offset of the first submesh using them
	 */
	static void Extract(Ogre::v1::MeshPtr mesh, std::vector<Ogre::Vector3>& viVertices, std::vector<unsigned int>& viSubmeshVertexOffsets,
			std::vector<unsigned int>& viIndices, std::vector<unsigned int>& viSubmeshIndexOffsets, bool bExtractIndicesOnly=false, unsigned int* pnVerts=0, unsigned int* pnIndices=0);

	/*
	 * return a reworked mesh based on the one given:
	 * @param vAppendedVertices are vertices based on the given triangle index with given UV-coordinates in the triangle
	 * hardware vertex buffer will be build according to vertex format
	 * pair<triangle index, uv-coord>
	 * @param viIndices indices of the rebuild mesh, indices smaller than the original mesh's vertex buffer size are interpreted
	 * as referring to the original vertex buffer, those bigger are interpreted as referring to the appended vertices
	 * pair<triangle index, vertex>
	 * triangle index -1 -> vertex indexes appended vertex index
	 * @return rebuilt mesh, shared vertices will be distributed as sub-mesh-vertices
	 */
	static Ogre::v1::MeshPtr RebuildMesh(Ogre::v1::MeshPtr mesh, const std::vector<std::pair<int, Ogre::Vector2> >& vAppendedVertices, const std::vector<std::pair<int, int> >& viIndices);

	/*
	 * careful: doesn't support shared vertices
	 */
	static Ogre::v1::MeshPtr CombineMeshes(Ogre::v1::MeshPtr mesh1, Ogre::v1::MeshPtr mesh2);

	static void FlipMesh(Ogre::v1::MeshPtr mesh);
};

#endif /* OGREMESHEXTRACTOR_H_ */
