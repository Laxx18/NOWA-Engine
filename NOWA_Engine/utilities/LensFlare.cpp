#include "NOWAPrecompiled.h"
#include "LensFlare.h"
#include "modules/GraphicsModule.h"

namespace NOWA
{

	LensFlare::LensFlare(const Ogre::Vector3& lightPosition, Ogre::Camera* camera, Ogre::SceneManager* sceneManager)
		: sceneManager(sceneManager),
		camera(camera),
		hidden(true),
		node(nullptr)
	{
		this->createLensFlare();
		this->setLightPosition(lightPosition);
	}

	/* ------------------------------------------------------------------------- */
	/// Destructor
	/* ------------------------------------------------------------------------- */
	LensFlare::~LensFlare()
	{
		this->node->detachObject(this->haloSet);
		this->node->detachObject(this->burstSet);
		this->sceneManager->destroyBillboardSet(this->haloSet);
		this->sceneManager->destroyBillboardSet(this->burstSet);

		NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->node);
		this->sceneManager->destroySceneNode(this->node);
	}


	void LensFlare::createLensFlare()
	{
		Ogre::Real scale = 2000;

		// -----------------------------------------------------
		// We create 2 sets of billboards for the lensflare
		// -----------------------------------------------------
		this->haloSet = this->sceneManager->createBillboardSet();
		this->haloSet->setName("halo");
// Attention: Where is this material? Is it in the default resource group name?
		this->haloSet->setMaterialName("lensflare/halo", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		this->haloSet->setCullIndividually(true);
		this->haloSet->setQueryFlags(0);	// They should not be detected by rays.

		this->burstSet = this->sceneManager->createBillboardSet();
		this->burstSet->setName("burst");
// Attention: Where is this material? Is it in the default resource group name?
		this->burstSet->setMaterialName("lensflare/burst", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		this->burstSet->setCullIndividually(true);
		this->burstSet->setQueryFlags(0);

		// The node is located at the light source.
		this->node = this->sceneManager->getRootSceneNode()->createChildSceneNode();

		this->node->attachObject(this->burstSet);
		this->node->attachObject(this->haloSet);

		// -------------------------------
		// Creation of the Halo billboards
		// -------------------------------
		Ogre::v1::Billboard* halo1 = this->haloSet->createBillboard(0, 0, 0);
		halo1->setDimensions(scale * 0.5f, scale * 0.5f);
		Ogre::v1::Billboard* halo2 = this->haloSet->createBillboard(0, 0, 0);
		halo2->setDimensions(scale, scale);
		Ogre::v1::Billboard* halo3 = this->haloSet->createBillboard(0, 0, 0);
		halo3->setDimensions(scale * 0.25f, scale * 0.25f);


		// -------------------------------
		// Creation of the "Burst" billboards
		// -------------------------------
		Ogre::v1::Billboard* burst1 = this->burstSet->createBillboard(0, 0, 0);
		burst1->setDimensions(scale * 0.25f, scale * 0.25f);
		Ogre::v1::Billboard* burst2 = this->burstSet->createBillboard(0, 0, 0);
		burst2->setDimensions(scale * 0.5f, scale * 0.5f);
		Ogre::v1::Billboard* burst3 = this->burstSet->createBillboard(0, 0, 0);
		burst3->setDimensions(scale * 0.25f, scale * 0.25f);

	}

	void LensFlare::update()
	{
		// If the Light is out of the Ogre::Camera field Of View, the lensflare is hidden.
		if (!this->camera->isVisible(this->lightPosition))
		{
			this->haloSet->setVisible(false);
			this->burstSet->setVisible(false);
			return;
		}
		else
		{
			this->haloSet->setVisible(true);
			this->burstSet->setVisible(true);
		}

		Ogre::Real lightDistance = this->lightPosition.distance(this->camera->getPosition());
		Ogre::Vector3 cameraVector = this->camera->getDirection(); // normalized vector (length 1)

		cameraVector = this->camera->getPosition() + (lightDistance * cameraVector);


		// The LensFlare effect takes place along this vector.
		Ogre::Vector3 lensFlareVector = (cameraVector - this->lightPosition);
		//lensFlareVector += Ogre::Vector3(-64,-64,0);  // sprite dimension (to be adjusted, but not necessary)

		// The different sprites are placed along this line.
		this->haloSet->getBillboard(0)->setPosition(lensFlareVector * 0.500f);
		this->haloSet->getBillboard(1)->setPosition(lensFlareVector*  0.125f);
		this->haloSet->getBillboard(2)->setPosition(-lensFlareVector * 0.250f);

		this->burstSet->getBillboard(0)->setPosition(lensFlareVector * 0.333f);
		this->burstSet->getBillboard(1)->setPosition(-lensFlareVector * 0.500f);
		this->burstSet->getBillboard(2)->setPosition(-lensFlareVector * 0.180f);

		// We redraw the lensflare (in case it was previouly out of the camera field, and hidden)
		this->setVisible(true);
	}


	/* ------------------------------------------------------------------------- */
	/// This function shows (or hide) the lensflare effect.
	/* ------------------------------------------------------------------------- */
	void LensFlare::setVisible(bool visible)
	{
		this->haloSet->setVisible(visible);
		this->burstSet->setVisible(visible);
		this->hidden = !visible;
	}

	void LensFlare::setLightPosition(const Ogre::Vector3& pos)
	{
		this->lightPosition = pos;
		this->node->setPosition(this->lightPosition);
	}

	void LensFlare::setBurstColor(const Ogre::ColourValue& color)
	{
		this->burstSet->getBillboard(0)->setColour(color);
		this->burstSet->getBillboard(1)->setColour(color * 0.8f);
		this->burstSet->getBillboard(2)->setColour(color * 0.6f);
	}

	void LensFlare::setHaloColor(const Ogre::ColourValue& color)
	{
		this->haloSet->getBillboard(0)->setColour(color * 0.8f);
		this->haloSet->getBillboard(1)->setColour(color * 0.6f);
		this->haloSet->getBillboard(2)->setColour(color);
	}

}; //namespace end
