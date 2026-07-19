#ifndef OBJECT_TITLE_H
#define OBJECT_TITLE_H

#include "defines.h"

namespace MyGUI
{
	class TextBox;
}

namespace NOWA
{

	// Zum Darstellen eines Objekttitles ueber dem Kopf eines Avatars.
	// v2: MyGUI-based screen-space label (was Ogre::v1::Overlay).
	// Deliberately screen-space, not a 3D billboard (e.g. NOWA::MovableText):
	// stays a constant on-screen size regardless of distance, and is never
	// occluded by scene geometry — both required for gizmo labels. MyGUI
	// composites in its own pass after the whole scene, so "always
	// readable" is automatic; no depth/macroblock handling needed.
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
		const Ogre::MovableObject*	pObject;
		const Ogre::Camera*			camera;

		// v2: single widget replaces the old Overlay + OverlayContainer +
		// OverlayElement("TextArea") trio. Sized to exactly wrap the
		// measured text (see setTitle()); update() re-centers it each frame
		// using its current (render-thread-queried) size.
		MyGUI::TextBox* pTextWidget;

		unsigned int uniqueId;
	};

} // namespace end

#endif
