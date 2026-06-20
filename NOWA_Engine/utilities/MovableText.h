/**
 * File: MovableText.h
 *
 * Description: This creates a billboarding object that displays text.
 *              Ported from v1 geometry to Ogre-Next v2 VAO.
 *              Billboard transform via getWorldTransforms() — parent node never rotated.
 *
 * @author  2003 by cTh see gavocanov@rambler.ru
 * @update  2006 by barraq see nospam@barraquand.com
 * @update  2012 by MindCalamity <mindcalamity@gmail.com>
 * @update  2015 for OGRE 2.1 by Jayray <jeremy.richert1@gmail.com>
 * @update  2026 v2 VAO rewrite for Ogre-Next by NOWA-Engine
 */

#ifndef __include_MovableText_H__
#define __include_MovableText_H__

#include "OgreFontManager.h"
#include "OgreMovableObject.h"
#include "defines.h"

namespace NOWA
{

    class EXPORTED MovableText : public Ogre::MovableObject, public Ogre::Renderable
    {
        friend class MovableTextFactory;

    public:
        enum HorizontalAlignment
        {
            H_LEFT,
            H_CENTER
        };
        enum VerticalAlignment
        {
            V_BELOW,
            V_ABOVE,
            V_CENTER
        };

    public:
        MovableText(Ogre::IdType id, Ogre::ObjectMemoryManager* objectMemoryManager, Ogre::SceneManager* sceneManager, const Ogre::NameValuePairList* params);
        virtual ~MovableText();

        // Setters
        void setFontName(const Ogre::String& fontName);
        void setCaption(const Ogre::String& caption);
        void setColor(const Ogre::ColourValue& color);
        void setCharacterHeight(Ogre::Real height);
        void setSpaceWidth(Ogre::Real width);
        void setTextAlignment(const HorizontalAlignment& h, const VerticalAlignment& v);
        void setGlobalTranslation(Ogre::Vector3 trans);
        void setLocalTranslation(Ogre::Vector3 trans);
        void showOnTop(bool show = true);
        void setTextYOffset(Ogre::Real yOffset);
        void forceUpdate(void);
        void update(Ogre::Real dt);

        // Getters
        const Ogre::String& getFontName() const
        {
            return mFontName;
        }
        const Ogre::String& getCaption() const
        {
            return mCaption;
        }
        const Ogre::ColourValue& getColor() const
        {
            return mColor;
        }
        Ogre::Real getCharacterHeight() const
        {
            return mCharHeight;
        }
        Ogre::Real getSpaceWidth() const
        {
            return mSpaceWidth;
        }
        Ogre::Vector3 getGlobalTranslation() const
        {
            return mGlobalTranslation;
        }
        Ogre::Vector3 getLocalTranslation() const
        {
            return mLocalTranslation;
        }
        bool getShowOnTop() const
        {
            return mOnTop;
        }

    protected:
        void _setupGeometry();
        void _destroyGeometry();
        void _updateHlmsMacroblock();

        // MovableObject
        const Ogre::String& getMovableType(void) const override
        {
            static Ogre::String t = "MovableText";
            return t;
        }
        void _updateRenderQueue(Ogre::RenderQueue* queue, Ogre::Camera* camera, const Ogre::Camera* lodCamera) override;
        void _notifyAttached(Ogre::Node* parent) override;

        // Renderable
        void getWorldTransforms(Ogre::Matrix4* xform) const override;
        void getRenderOperation(Ogre::v1::RenderOperation& op, bool casterPass) override;
        const Ogre::LightList& getLights(void) const override
        {
            return mLList;
        }

    private:
        Ogre::String mFontName;
        Ogre::String mName;
        Ogre::String mCaption;
        HorizontalAlignment mHorizontalAlignment;
        VerticalAlignment mVerticalAlignment;

        Ogre::ColourValue mColor;
        Ogre::LightList mLList;

        Ogre::Real mCharHeight;
        Ogre::Real mSpaceWidth;
        bool mNeedUpdate;
        bool mOnTop;
        Ogre::Real yOffset;

        Ogre::Vector3 mGlobalTranslation;
        Ogre::Vector3 mLocalTranslation;

        Ogre::Camera* mpCam;
        Ogre::Font* mpFont;
        Ogre::HlmsDatablock* mpHlmsDatablock;

        // v2 VAO resources
        Ogre::VertexArrayObjectArray mVaos[2]; // [VpNormal], [VpShadow]
        Ogre::VertexBufferPacked* mVertexBuffer;
        Ogre::VaoManager* mVaoManager;
    };

    class EXPORTED MovableTextFactory : public Ogre::MovableObjectFactory
    {
    protected:
        Ogre::MovableObject* createInstanceImpl(Ogre::IdType id, Ogre::ObjectMemoryManager* objectMemoryManager, Ogre::SceneManager* sceneManager, const Ogre::NameValuePairList* params = nullptr) override;

    public:
        MovableTextFactory()
        {
        }
        ~MovableTextFactory()
        {
        }
        const Ogre::String& getType() const override;
        void destroyInstance(Ogre::MovableObject* obj) override;
    };

} // namespace NOWA

#endif