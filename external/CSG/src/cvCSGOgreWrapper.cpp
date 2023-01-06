//
// CSG Base Code : 
// Java    http://www.geocities.com/danbalby/
// C++ Version: http://code.google.com/p/csgtestworld/
// 

#include "Ogre.h"

#include "cvCSGOgreWrapper.h"
#include "cvCSGConverter.h"
#include "BooleanModeller.h"
#include "Solid.h"

#define csgMin(a, b)	((a>b) ? b : a)
#define csgMax(a, b)	((a>b) ? a : b)

using namespace Ogre;

namespace CSG
{

	void cvEntityBoolean(const std::string& MeshName, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2, evCSG_OPER Op)
	{
		cvCSGConverter* Converter = new cvCSGConverter();

		Solid* Object1 = Converter->convertMesh2Solid(Ent1, 1);
		Solid* Object2 = Converter->convertMesh2Solid(Ent2, 2);

		BooleanModeller* pModeler = new BooleanModeller(Object1, Object2);
		Solid *ObjResult;
		Converter->setBBMin(Ogre::Vector3(99999.0, 99999.0, 99999.0));		// Make it Big
		Converter->setBBMax(Ogre::Vector3(-99999.0, -99999.0, -99999.0));		// Make it Small
		switch (Op)
		{
		case CSG_UNION:
			ObjResult = pModeler->getUnion();
			break;
		case CSG_INTERSECTION:
			ObjResult = pModeler->getIntersection();
			break;
		case CSG_DIFFERENCE:
			ObjResult = pModeler->getDifference();
			break;
		default:
			ObjResult = pModeler->getUnion();
			break;
		}
		Converter->convertSolid2Mesh(MeshName, ObjResult, Ent1, Ent2);

		delete ObjResult;
		delete Object1;
		delete Object2;
		delete pModeler;
		delete Converter;
	}

	void cvEntityUnion(const std::string& MeshName, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2)
	{
		cvEntityBoolean(MeshName, Ent1, Ent2, evCSG_OPER::CSG_UNION);
	}

	void cvEntityIntersection(const std::string& MeshName, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2)
	{
		cvEntityBoolean(MeshName, Ent1, Ent2, evCSG_OPER::CSG_INTERSECTION);
	}

	void cvEntityDifference(const std::string& MeshName, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2)
	{
		cvEntityBoolean(MeshName, Ent1, Ent2, evCSG_OPER::CSG_DIFFERENCE);
	}

}; // namespace end
