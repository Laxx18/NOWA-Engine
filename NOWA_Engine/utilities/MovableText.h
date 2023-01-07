/**
* File: MovableText.h
*
* Description: This creates a billboarding object that display a text.
* Note: This object must have a dedicated scene node since it will rotate it to face the camera (OGRE 2.1)
*
* @author	2003 by cTh see gavocanov@rambler.ru
* @update	2006 by barraq see nospam@barraquand.com
* @update	2012 to work with newer versions of OGRE by MindCalamity <mindcalamity@gmail.com>
* @update	2015 to work on OGRE 2.1 (but not on older versions anymore) by Jayray <jeremy.richert1@gmail.com>
*	- See "Notes" on: http://www.ogre3d.org/tikiwiki/tiki-editpage.php?page=MovableText
*/

#ifndef __include_MovableText_H__
#define __include_MovableText_H__

#include "defines.h"
#include "OgreMovableObject.h"
#include "OgreFontManager.h"

using namespace Ogre;

namespace NOWA
{

	class EXPORTED MovableText : public MovableObject, public Renderable
	{
		// Allow MovableTextFactory full access
		friend class MovableTextFactory;

		/******************************** MovableText data ****************************/
	public:
		enum HorizontalAlignment
		{
			H_LEFT, H_CENTER
		};
		enum VerticalAlignment
		{
			V_BELOW, V_ABOVE, V_CENTER
		};
	public:
		MovableText(IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *sceneManager, const NameValuePairList* params);

	protected:
		String					mFontName;
		String					mName;
		String					mCaption;
		HorizontalAlignment		mHorizontalAlignment;
		VerticalAlignment		mVerticalAlignment;

		ColourValue				mColor;
		v1::RenderOperation		mRenderOp;
		LightList				mLList;

		Real					mCharHeight;
		Real					mSpaceWidth;

		bool					mNeedUpdate;
		bool					mUpdateColors;
		bool					mOnTop;
		Ogre::Real				yOffset;

		Real					mTimeUntilNextToggle;

		Vector3					mGlobalTranslation;
		Vector3					mLocalTranslation;

		Camera					*mpCam;
		Font					*mpFont;
		HlmsDatablock			*mpHlmsDatablock;

		/******************************** public methods ******************************/
	public:
		virtual ~MovableText();

		// Set settings
		void setFontName(const String &fontName);
		void setCaption(const String &caption);
		void setColor(const ColourValue &color);
		void setCharacterHeight(Real height);
		void setSpaceWidth(Real width);
		void setTextAlignment(const HorizontalAlignment& horizontalAlignment, const VerticalAlignment& verticalAlignment);
		void setGlobalTranslation(Vector3 trans);
		void setLocalTranslation(Vector3 trans);
		void showOnTop(bool show = true);
		void forceUpdate(void);

		// Get settings
		const String& getFontName()	const
		{
			return mFontName;
		}

		const String & getCaption() const
		{
			return mCaption;
		}

		const ColourValue& getColor() const
		{
			return mColor;
		}

		Real getCharacterHeight() const
		{
			return mCharHeight;
		}

		Real getSpaceWidth() const
		{
			return mSpaceWidth;
		}

		Vector3	getGlobalTranslation() const
		{
			return mGlobalTranslation;
		}

		Vector3	getLocalTranslation() const
		{
			return mLocalTranslation;
		}

		bool getShowOnTop() const
		{
			return mOnTop;
		}

		void setTextYOffset(Ogre::Real yOffset);

		void update(Ogre::Real dt);

		/******************************** protected methods and overload **************/
	protected:

		// from MovableText, create the object
		void						_setupGeometry();
		void						_updateColors();
		void                        _updateHlmsMacroblock();

		// from MovableObject
		const   String&				getMovableType(void) const
		{
			static Ogre::String movType = "MovableText"; return movType;
		};

		void						_updateRenderQueue(RenderQueue* queue, Camera *camera, const Camera *lodCamera);

		// from Renderable
		void						getWorldTransforms(Matrix4 *xform) const;
		void						getRenderOperation(v1::RenderOperation& op, bool casterPass) override;
		const   LightList&			getLights(void) const
		{
			return mLList;
		};
	};

	/** Factory object for creating MovableText instances */
	class EXPORTED MovableTextFactory : public MovableObjectFactory
	{
	protected:
		virtual MovableObject* createInstanceImpl(IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *sceneManager, const NameValuePairList* params = 0);
	public:
		MovableTextFactory()
		{
		}
		~MovableTextFactory()
		{
		}

		const String& getType() const;
		void destroyInstance(MovableObject* obj);

	};

}; // namespace end

#endif