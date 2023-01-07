#ifndef BACKGROUND_MANAGER_2D_H
#define BACKGROUND_MANAGER_2D_H

// see http://www.ogre3d.org/tikiwiki/Displaying+2D+Backgrounds
// http://ogre3d.org/forums/viewtopic.php?f=2&t=30782 --> controllermanager

#include "defines.h"
#include "OgreRectangle2D.h"

namespace NOWA
{
	class EXPORTED BackgroundManager2D
	{
	public:
		BackgroundManager2D(Ogre::SceneManager* sceneManager, const Ogre::String& textureName, const Ogre::String& groupName, 
			unsigned int renderQueueGroup = 254);

		virtual ~BackgroundManager2D();

		void reset(void);

		void scrollAnimation(Ogre::Real xSpeed, Ogre::Real ySpeed);

		void rotateAnimation(Ogre::Real speed);

		void scroll(Ogre::Real u, Ogre::Real v);

		void backgroundRelativeToPositionX(Ogre::Real absoluteXPos, Ogre::Real dt, Ogre::Real xSpeed);

		void backgroundRelativeToPosition(Ogre::Vector2 absolutePos, Ogre::Real dt, Ogre::Real xSpeed, Ogre::Real ySpeed);

		void rotate(Ogre::Degree& degree);

		void scale(Ogre::Real su, Ogre::Real sv);

		Ogre::TextureUnitState* getTextureUnitState(void) const;
	private:
		static unsigned int id;
	private:
		Ogre::v1::Rectangle2D* rectangle;
		const unsigned int nextId;
		Ogre::MaterialPtr materialPtr;
		Ogre::TextureUnitState::TextureEffect effect;
		Ogre::Vector2 lastPosition;
		Ogre::Real lastPositionX;
		bool firstTimePositionSet;
		Ogre::Real xScroll;
		Ogre::Real yScroll;
	};

}; //namespace end

#endif