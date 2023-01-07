#include "NOWAPrecompiled.h"
#include "ObjectTextDisplay.h"

namespace NOWA
{

	ObjectTextDisplay::ObjectTextDisplay(const Ogre::String& name, Ogre::v1::OldBone* bone, Ogre::Camera* camera, Ogre::v1::Entity* entity)
		: name(name),
		bone(bone),
		camera(camera),
		entity(entity),
		enabled(false)
	{
		// create an overlay that we can use for later
		// this->overlay = Ogre::OverlayManager::getSingleton().create("shapeName");
		// this->container = static_cast<Ogre::OverlayContainer*>(Ogre::OverlayManager::getSingleton().createOverlayElement(
		// 	"Panel", "container1"));

		/*// create an overlay that we can use for later
		if (overlay == NULL) {
			overlay = Ogre::OverlayManager::getSingleton().create("floatingTextOverlay");
			overlay->show();
		}

		char buf[15];
		sprintf(buf, "c_%s", p->getName().c_str());
		this->elementName = buf;
		this->container = static_cast<Ogre::OverlayContainer*>(Ogre::OverlayManager::getSingleton().createOverlayElement(
			"Panel", buf));

		overlay->add2D(this->container);

		sprintf(buf, "ct_%s", p->getName().c_str());
		this->elementTextName = buf;
		this->textOverlay = Ogre::OverlayManager::getSingleton().createOverlayElement("TextArea", buf);
		// assert_VALID_PTR(m_pText);
		this->textOverlay->setDimensions(1.0, 1.0);
		this->textOverlay->setMetricsMode(Ogre::GMM_PIXELS);
		this->textOverlay->setPosition(0, 0);

		this->textOverlay->setParameter("font_name", "LucidaSans");
		this->textOverlay->setParameter("char_height", "16");
		this->textOverlay->setParameter("horz_align", "center");
		this->textOverlay->setColour(Ogre::ColourValue(1.0, 1.0, 1.0));

		this->container->addChild(this->textOverlay);
		this->container->setEnabled(false);*/

		// create an overlay that we can use for later
		this->overlay = Ogre::v1::OverlayManager::getSingleton().create(this->name);
		this->container = static_cast<Ogre::v1::OverlayContainer*>(Ogre::v1::OverlayManager::getSingleton().createOverlayElement("Panel", this->name + "container"));

		this->overlay->add2D(this->container);

		this->textOverlay = Ogre::v1::OverlayManager::getSingleton().createOverlayElement("TextArea", this->name + "_shapeNameText");
		this->textOverlay->setDimensions(1.0, 1.0);
		this->textOverlay->setMetricsMode(Ogre::v1::GMM_PIXELS);
		this->textOverlay->setPosition(0, 0);

		this->textOverlay->setParameter("font_name", "BlueHighway");
		this->textOverlay->setParameter("char_height", "16");
		this->textOverlay->setParameter("horz_align", "center");
		this->textOverlay->setColour(Ogre::ColourValue(1.0, 1.0, 1.0));
		this->container->addChild(this->textOverlay);
		this->overlay->show();
	}

	ObjectTextDisplay::~ObjectTextDisplay()
	{
		// overlay cleanup -- Ogre would clean this up at app exit but if your app 
		// tends to create and delete these objects often it's a good idea to do it here.

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ObjectTextDisplay] Destructor display: " + this->name + "_shapeNameText");
		this->overlay->hide();
		Ogre::v1::OverlayManager *overlayManager = Ogre::v1::OverlayManager::getSingletonPtr();
		if (this->container->getChild(this->name + "_shapeNameText"))
		{
			this->container->removeChild(this->name + "_shapeNameText");

			this->overlay->remove2D(this->container);
			overlayManager->destroyOverlayElement(this->container);
			overlayManager->destroyOverlayElement(this->textOverlay);
			overlayManager->destroy(this->overlay);
		}
	}

	void ObjectTextDisplay::setEnabled(bool enabled)
	{
		this->enabled = enabled;
		if (enabled)
		{
			this->container->show();
		}
		else
		{
			this->container->hide();
		}
	}

	void ObjectTextDisplay::setText(const Ogre::String& text)
	{
		this->text = text;
		this->textOverlay->setCaption(this->text);
	}

	Ogre::Vector3 ObjectTextDisplay::getBoneWorldPosition(Ogre::v1::Entity* entity, Ogre::v1::OldBone* bone)
	{
// Attention: is this correct?, there is no _getDerivedPosition anymore
		// Ogre::Vector3 worldPosition = bone->_getDerivedTransform().getTrans();
		Ogre::Vector3 worldPosition = bone->_getDerivedPosition();
		// multiply with the parent derived transformation
		Ogre::Node* parentNode = entity->getParentNode();
		Ogre::SceneNode* sceneNode = entity->getParentSceneNode();
		while (parentNode != nullptr)
		{
			// process the current i_Node
			if (parentNode != sceneNode)
			{
				// this is a tag point (a connection point between 2 entities). which means it has a parent i_Node to be processed
				worldPosition = parentNode->_getFullTransform() * worldPosition;
				parentNode = parentNode->getParent();
			}
			else
			{
				// this is the scene i_Node meaning this is the last i_Node to process
				worldPosition = parentNode->_getFullTransform() * worldPosition;
				break;
			}
		}
		return worldPosition;
	}

	void ObjectTextDisplay::update(Ogre::Real dt)
	{
		if (!this->enabled)
			return;

		/*const Ogre::AxisAlignedBox& bbox = this->movableObject->getWorldBoundingBox(true);
		Ogre::Matrix4 mat = this->camera->getViewMatrix();

		bool behind = false;

		// We want to put the text point in the center of the top of the AABB Box.
		Ogre::Vector3 topcenter = bbox.getCenter();
		// Y is up.
		topcenter.y += bbox.getHalfSize().y;
		topcenter = mat * topcenter;
		// We are now in screen pixel coords and depth is +Z away from the viewer.
		behind = (topcenter.z > 0.0);

		if (behind) {
			// Don't show text for objects behind the camera.
			this->container->setPosition(-1000, -1000);
		} else {
			// Not in pixel coordinates, in screen coordinates as described above.
			// We convert to screen relative coord by knowing the window size.
			// Top left screen corner is 0, 0 and the bottom right is 1,1
			// The 0.45's here offset alittle up and right for better "text above head" positioning.
			// The 2.2 and 1.7 compensate for some strangeness in Ogre projection?
			// Tested in wide screen and normal aspect ratio.
			this->container->setPosition(0.45f - topcenter.x / (2.2f * topcenter.z),
				topcenter.y / (1.7f * topcenter.z) + 0.45f);
			// Sizse is relative to screen size being 1.0 by 1.0 (not pixels size)
			this->container->setDimensions(0.1f, 0.1f);
		}*/

		Ogre::Vector3 v = this->getBoneWorldPosition(this->entity, this->bone);

		Ogre::Vector3 screenPos = this->camera->getProjectionMatrix() * this->camera->getViewMatrix() * v;
		screenPos.x = (screenPos.x) / 2;
		screenPos.y = (screenPos.y) / 2;

		float maxX = screenPos.x;
		float minY = -screenPos.y;

		this->container->setPosition(maxX, minY + 0.5f);
	}

}; //namespace end NOWA