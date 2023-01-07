#ifndef LENS_FLARE_H
#define LENS_FLARE_H

#include "defines.h"

class Ogre::Camera;
class Ogre::SceneManager;
class Ogre::SceneNode;
class Ogre::v1::BillboardSet;

namespace NOWA
{
	/* ------------------------------------------------------------------------- */
	/// A lens Flare effect.
	/**
	This class will create a lensflare effect, between The light position and the
	camera position.
	Some functions will allow to change the lensflare color effect, in case of colored
	light, for instance.
	@see http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Displaying+LensFlare&structure=Cookbook
	*/
	/* ------------------------------------------------------------------------- */
	class EXPORTED LensFlare
	{
	public:
		LensFlare(const Ogre::Vector3& lightPosition, Ogre::Camera* camera, Ogre::SceneManager* sceneManager);
		virtual ~LensFlare();

		void createLensFlare();
		void update();
		void setVisible(bool visible);

		/* ------------------------------------------------------------------------- */
		/// This function updates the light source position. 
		/** This function can be used if the light source is moving.*/
		/* ------------------------------------------------------------------------- */
		void setLightPosition(const Ogre::Vector3& pos);

		void setHaloColor(const Ogre::ColourValue& color);

		void setBurstColor(const Ogre::ColourValue& color);

	protected:
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		Ogre::ColourValue color;
		Ogre::SceneNode* node;
		Ogre::v1::BillboardSet* haloSet;
		Ogre::v1::BillboardSet* burstSet;
		Ogre::Vector3 lightPosition;
		bool hidden;
	};

}; // namespace end

#endif