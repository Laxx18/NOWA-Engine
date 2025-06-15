#ifndef OBJECT_TITLE_H
#define OBJECT_TITLE_H

#include "OgreOverlay.h"
#include "OgreOverlayContainer.h"
#include "OgreOverlayManager.h"
#include "OgreOverlayContainer.h"
#include "OgreFont.h"
#include "OgreFontManager.h"

#include "defines.h"

namespace NOWA
{

	/*class Ogre::MovableObject;
	class Ogre::Camera;
	class Ogre::Overlay;
	class Ogre::OverlayElement;
	class Ogre::OverlayContainer;
	class Ogre::Font;
	class Ogre::FontManager;*/


	//Zum Darstellen eines Objekttitles ueber den Kopfs eines Avatars
	//Vorteil: Performant (leichtgewichtig)
	//Nachteil: Da es sich um ein Overlay handelt, ist der Text immer sichbar fuer die Kamera, egal ob sich das Objekt hinter einem Hindernis befindet

	class EXPORTED ObjectTitle
	{
	public:
		ObjectTitle(const Ogre::String & strName, Ogre::MovableObject* pObject, Ogre::Camera* camera,
			const Ogre::String& strFontName, const Ogre::ColourValue& color = Ogre::ColourValue::White);

		~ObjectTitle();

		void setTitle(const Ogre::String& strTitle);

		void setColor(const Ogre::ColourValue& color);

		void update();
	private:
		Ogre::Vector2 getTextDimensions(Ogre::String text);
	private:
		const Ogre::MovableObject*	pObject;
		const Ogre::Camera*			camera;
		Ogre::v1::Overlay*				pOverlay;
		Ogre::v1::OverlayElement*		pTextarea;
		Ogre::v1::OverlayContainer*		pContainer;
		Ogre::Vector2				textDim;
		Ogre::Font*					pFont;

		unsigned int uniqueId;
	};

} // namespace end

#endif