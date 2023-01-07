#ifndef OBJECT_TEXT_DISPLAY_H
#define OBJECT_TEXT_DISPLAY_H

#include "defines.h"

#include "OgreOverlay.h"
#include "OgreOverlayManager.h"
#include "OgreOverlayContainer.h"
#include "OgreFont.h"
#include "OgreFontManager.h"

namespace NOWA
{
	/*
	class Ogre::Camera;
	class Ogre::SceneNode;
	class Ogre::Overlay;
	class Ogre::v1::Entity;
	class Ogre::Bone;
	class Ogre::Font;
	class Ogre::FontManager;
	*/

	// static Ogre::Overlay* overlay;

	class EXPORTED ObjectTextDisplay
	{
	public:
		ObjectTextDisplay(const Ogre::String& name, Ogre::v1::OldBone* bone, Ogre::Camera* camera, Ogre::v1::Entity* entity);

		~ObjectTextDisplay();

		void setEnabled(bool enabled);

		void setText(const Ogre::String& text);

		void update(Ogre::Real dt);
	private:
		Ogre::Vector3 getBoneWorldPosition(Ogre::v1::Entity* entity, Ogre::v1::OldBone* bone);
	
	private:
		Ogre::String name;
		Ogre::v1::OldBone* bone;
		Ogre::Camera* camera;
		Ogre::v1::Entity* entity;
		Ogre::String text;
		bool enabled;
		Ogre::v1::Overlay* overlay;
		Ogre::v1::OverlayElement* textOverlay;
		Ogre::String elementName;
		Ogre::String elementTextName;
		Ogre::v1::OverlayContainer* container;
	};

};  //namespace end

#endif