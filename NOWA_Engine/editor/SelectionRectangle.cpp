#include "NOWAPrecompiled.h"
#include "SelectionRectangle.h"
#include "main/Core.h"

namespace NOWA
{
	SelectionRectangle::SelectionRectangle(const Ogre::String& name, Ogre::SceneManager* sceneManager, Ogre::Camera* camera)
		: Ogre::ManualObject(Ogre::Id::generateNewId<SelectionRectangle>(), &sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), sceneManager),
		camera(camera)
	{
		this->setName(name);
		//// Assure that the rectangle is allways visible on the surface
		this->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
		this->setQueryFlags(0 << 0);
		this->setCastShadows(false);
		// this->setVisibilityFlags(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND + 1);
	}

	void SelectionRectangle::setCorners(Ogre::Real left, Ogre::Real top, Ogre::Real right, Ogre::Real bottom)
	{
		// Start updating the manual object
		if (getNumSections() > 0)
		{
			beginUpdate(0);
		}
		else
		{
			clear();
			// begin("TransparentBlueNoLighting", Ogre::OperationType::OT_TRIANGLE_LIST);
			begin("BlueNoLighting", Ogre::OperationType::OT_LINE_LIST);
			// begin("TransparentBlueNoLighting", Ogre::OperationType::OT_TRIANGLE_STRIP);
		}

		// Transform from 2D to 3D according to the camera's current position and orientation
		Ogre::Vector3 dir = this->camera->getDirection();
		Ogre::Vector3 pos = this->camera->_getCachedDerivedPosition();
		
		// Create a ray for each corner
		Ogre::Ray topLeftRay = this->camera->getCameraToViewportRay(left, top);
		Ogre::Ray topRightRay = this->camera->getCameraToViewportRay(right, top);
		Ogre::Ray bottomLeftRay = this->camera->getCameraToViewportRay(left, bottom);
		Ogre::Ray bottomRightRay = this->camera->getCameraToViewportRay(right, bottom);

		// Shoot the ray onto the normal of the orientated plane to get the distance
		// Rotate the normal always to the camera and get the position of the camera
		Ogre::Real r1 = topLeftRay.intersects(Ogre::Plane(dir, pos + dir)).second;
		Ogre::Real r2 = topRightRay.intersects(Ogre::Plane(dir, pos + dir)).second;
		Ogre::Real r3 = bottomLeftRay.intersects(Ogre::Plane(dir, pos + dir)).second;
		Ogre::Real r4 = bottomRightRay.intersects(Ogre::Plane(dir, pos + dir)).second;

		Ogre::Vector3 topLeft = topLeftRay.getPoint(r1);
		Ogre::Vector3 topRight = topRightRay.getPoint(r2);
		Ogre::Vector3 bottomLeft = bottomLeftRay.getPoint(r3);
		Ogre::Vector3 bottomRight = bottomRightRay.getPoint(r4);

		position(topLeft); // 0
		position(topRight); // 3
		line(0, 1);

		position(topLeft); // 0
		position(bottomLeft); // 6
		line(2, 3);

		position(bottomRight); // 7
		position(topRight); // 3
		line(4, 5);

		position(bottomRight); // 7
		position(bottomLeft); // 6
		line(6, 7);
		
		end();
	}

	void SelectionRectangle::setCorners(const Ogre::Vector2 &topLeft, const Ogre::Vector2 &bottomRight)
	{
		setCorners(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
	}

}; // namespace end