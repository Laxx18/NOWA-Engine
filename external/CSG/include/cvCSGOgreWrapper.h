//
// Ogre CSG Lib ported by C.O.Park (copjoker@naver.com)
//
// Licence : Free to use, as-it-is, 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.
//
// Ogre CSGLib: http://blog.naver.com/copjoker/120063626465
// CSG Base Code: 
// Java   Code: http://www.geocities.com/danbalby/
// C++ Version: http://code.google.com/p/csgtestworld/
// 

#ifndef FF_CV_CSGOGREWRAPPER_H
#define FF_CV_CSGOGREWRAPPER_H

#include "string"
#include "defines.h"

class Ogre::v1::Entity;

namespace CSG
{
	enum evCSG_OPER
	{
		CSG_UNION,
		CSG_INTERSECTION,
		CSG_DIFFERENCE
	};

	EXPORTS void cvEntityBoolean(const std::string& MeshName, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2, evCSG_OPER Op);
	EXPORTS void cvEntityUnion(const std::string& MeshName, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2);
	EXPORTS void cvEntityIntersection(const std::string& MeshName, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2);
	EXPORTS void cvEntityDifference(const std::string& MeshName, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2);


#endif	// FF_CV_CSGOGREWRAPPER_H

}; // namespace end