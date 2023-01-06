/*
 * VertexMerger.h
 *
 *  Created on: Apr 28, 2011
 *      Author: _1nsane
 */

#ifndef VERTEXMERGER_H_
#define VERTEXMERGER_H_

#include <vector>
#include <OgreVector3.h>

class VertexMerger
{
public:
	/*
	 * return a look-up table, pointing to vertex indices that have the same spatial position as the one requested
	 */
	static std::vector<unsigned int> GetMatchingIndices(std::vector<Ogre::Vector3> vVertices, const float& fEps=1e-5f);
};

#endif /* VERTEXMERGER_H_ */
