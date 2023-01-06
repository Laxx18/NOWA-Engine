/*
 * MeshTools.h
 *
 *  Created on: Dec 22, 2011
 *      Author: _1nsane
 */

#ifndef MESHTOOLS_H_
#define MESHTOOLS_H_

#include <vector>
#include <map>
#include <set>

class MeshTools
{
public:
	/*
	 * @param viIndices indices of the mesh
	 * @param viMatchingVertices maps vertex indices to others on the same spot
	 */
	static std::vector<std::set<unsigned int> > findNeighboringTris(const std::vector<unsigned int>& viIndices,
			const std::vector<unsigned int>& viMatchingVertexIndices);
	/*
	 * @param mCutTris list of triangles that were cut and are thus not part of any patch (data type is due to calling algorithm...)
	 * @param vNeighboringTris result of the above
	 */
	static std::vector<std::pair<std::set<unsigned int>, bool> > getTriPatches(const std::vector<std::set<unsigned int> >& vNeighboringTris, const std::map<unsigned int, std::set<unsigned int> >& mCutTris);
};


#endif /* MESHTOOLS_H_ */
