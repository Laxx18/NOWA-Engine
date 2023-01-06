#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionSerializer.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_World.h"
#include <fstream>

namespace OgreNewt
{
	CollisionSerializer::CollisionSerializer()
	{
	}


	CollisionSerializer::~CollisionSerializer()
	{
	}


	void CollisionSerializer::exportCollision(const CollisionPtr& collision, const Ogre::String& filename)
	{
		if (!collision)
			OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Argument collision is NULL", "CollisionSerializer::exportCollision");

		std::fstream *file = new std::fstream();
		file->open(filename, std::fstream::out | std::fstream::binary);

		if (!file->is_open())
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Unable to open file " + filename + " for writing", "CollisionSerializer::exportCollision");
		}

		mStream = Ogre::DataStreamPtr(new Ogre::FileStreamDataStream(file, true));
		NewtonCollisionSerialize(collision->getWorld()->getNewtonWorld(),
			collision->m_col,
			&CollisionSerializer::_newtonSerializeCallback, this);

		mStream->close();
	}


	//  CollisionPtr CollisionSerializer::importCollision(Ogre::DataStreamPtr& stream, OgreNewt::World* world)
	CollisionPtr CollisionSerializer::importCollision(Ogre::DataStream& stream, OgreNewt::World* world)
	{
		CollisionPtr dest;

		NewtonCollision* col = NewtonCreateCollisionFromSerialization(world->getNewtonWorld(), &CollisionSerializer::_newtonDeserializeCallback, &stream);

		// the type doesn't really matter... but lets do it correctly
		switch (Collision::getCollisionPrimitiveType(col))
		{
		case BoxPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::Box(world));
			break;
		case ConePrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::Cone(world));
			break;
		case EllipsoidPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::Ellipsoid(world));
			break;
		case CapsulePrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::Capsule(world));
			break;
		case CylinderPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::Cylinder(world));
			break;
		case CompoundCollisionPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::CompoundCollision(world));
			break;
		case ConvexHullPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::ConvexHull(world));
			break;
		// case ConvexHullModifierPrimitiveType:
		// 	dest = CollisionPtr(new ConvexModifierCollision(world));
			break;
		case ChamferCylinderPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::ChamferCylinder(world));
			break;
		case TreeCollisionPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::TreeCollision(world));
			break;
		case NullPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::Null(world));
			break;
			/* case HeighFieldPrimitiveType:
				 dest = CollisionPtr(new CollisionPrimitives::HeightField(world));
				 break;*/
		case DeformableType:
			dest = CollisionPtr(new CollisionPrimitives::TetrahedraDeformableCollision(world));
			break;
		case ConcaveHullPrimitiveType:
			dest = CollisionPtr(new CollisionPrimitives::ConcaveHull(world));
			break;
		case ScenePrimitiveType:
		default:
			dest = CollisionPtr(new Collision(world));

		}

		dest->m_col = col;

		return dest;
	}


	void CollisionSerializer::_newtonSerializeCallback(void* serializeHandle, const void* buffer, int size)
	{
		CollisionSerializer* me = (static_cast<CollisionSerializer*>(serializeHandle));
		me->writeData(buffer, 1, size);
// Attention: Is that correct??
		// me->writeData(buffer, size, 1);
	}


	void CollisionSerializer::_newtonDeserializeCallback(void* deserializeHandle, void* buffer, int size)
	{
		//  Ogre::DataStreamPtr ptr(*static_cast<Ogre::DataStreamPtr*>(deserializeHandle));
		Ogre::DataStream& ptr = *(static_cast<Ogre::DataStream*>(deserializeHandle));

		ptr.read(buffer, size);
	}
}   // end namespace OgreNewt


