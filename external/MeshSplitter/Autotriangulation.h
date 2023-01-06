/*
 * Autotriangulation.h
 *
 *  Created on: Sep 11, 2011
 *      Author: insi
 */

#ifndef AUTO_TRIANGULATION
#define AUTO_TRIANGULATION

#include <vector>
#include <string>

/*
 * simple 2D vertex class used by triangulation
 */
class SVertex
{
public:
	float x,y;
	SVertex(float x,float y){this->x=x;this->y=y;}
	SVertex operator +(const SVertex& rval) const {return SVertex(x+rval.x, y+rval.y);}
	SVertex operator -(const SVertex& rval) const {return SVertex(x-rval.x, y-rval.y);}
	float operator *(const SVertex& rval) const {return x*rval.x+y*rval.y;}
	SVertex operator *(const float& rval) const {return SVertex(x*rval, y*rval);}
};

/*
 * does the actual triangulation
 */

std::vector<unsigned int>
TriangulatePolygons(const std::vector<SVertex>& vertices, const std::vector<std::vector<unsigned int> >& polygons, std::string& strErr);

#endif
