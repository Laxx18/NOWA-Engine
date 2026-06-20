/**
 * File: MovableText.cpp
 *
 * Ported from v1 geometry to Ogre-Next v2 VAO.
 *
 * Key design decisions:
 *  - MovableText is both MovableObject and Renderable (mRenderables.push_back(this)).
 *  - getWorldTransforms() is verbatim from original — billboard via world matrix
 *    override, parent node is never rotated.
 *  - getRenderOperation() throws ERR_NOT_IMPLEMENTED exactly like ManualObject2.
 *    Safe because setDatablock() and _updateHlmsMacroblock() are only called at
 *    the END of _setupGeometry() AFTER the VAO is in mVaos[] — so calculateHashFor()
 *    always takes the V2 path via getVaos() and never reaches getRenderOperation().
 *  - _setupGeometry() builds a VAO (pos3+uv2, BT_IMMUTABLE, no index buffer).
 *  - _notifyAttached() triggers _setupGeometry() on first attach.
 *  - Colour is set via HlmsUnlitDatablock::setColour() — no per-vertex colour buffer.
 */

#include "NOWAPrecompiled.h"
#include "MovableText.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreMeshManager2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"
#include "modules/GraphicsModule.h"

#define UV_MIN 0.0f
#define UV_MAX 1.0f
#define UV_RANGE (UV_MAX - UV_MIN)

namespace
{
    Ogre::String sMovableTextType = "MovableText";
}

namespace NOWA
{

    // ============================================================================
    //  Constructor / Destructor
    // ============================================================================

    MovableText::MovableText(Ogre::IdType id, Ogre::ObjectMemoryManager* objectMemoryManager, Ogre::SceneManager* sceneManager, const Ogre::NameValuePairList* params) :
        Ogre::MovableObject(id, objectMemoryManager, sceneManager, RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND),
        mpCam(nullptr),
        mpFont(nullptr),
        mpHlmsDatablock(nullptr),
        mName(""),
        mCaption(" "),
        mFontName("BlueHighway"),
        mCharHeight(1.0f),
        mColor(Ogre::ColourValue::White),
        mSpaceWidth(0.0f),
        mNeedUpdate(true),
        mOnTop(false),
        mHorizontalAlignment(H_LEFT),
        mVerticalAlignment(V_BELOW),
        mGlobalTranslation(Ogre::Vector3::ZERO),
        mLocalTranslation(Ogre::Vector3::ZERO),
        yOffset(0.0f),
        mVertexBuffer(nullptr),
        mVaoManager(nullptr)
    {
        if (params)
        {
            auto ni = params->find("name");
            if (ni != params->end())
            {
                mName = ni->second;
            }

            ni = params->find("caption");
            if (ni != params->end())
            {
                mCaption = ni->second;
            }

            ni = params->find("fontName");
            if (ni != params->end())
            {
                mFontName = ni->second;
            }
        }

        assert(!mName.empty() && "Trying to create MovableText without name");

        mVaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

        this->setFontName(mFontName);
        this->setCastShadows(false);

        // Register as renderable so SceneManager finds it — same as original
        mRenderables.push_back(this);
    }

    MovableText::~MovableText()
    {
        _destroyGeometry();

        if (mpHlmsDatablock)
        {
            this->_setNullDatablock();
            mpHlmsDatablock->getCreator()->destroyDatablock(mpHlmsDatablock->getName());
            mpHlmsDatablock = nullptr;
        }
    }

    // ============================================================================
    //  VAO cleanup
    // ============================================================================

    void MovableText::_destroyGeometry()
    {
        if (!mVaos[Ogre::VpNormal].empty())
        {
            Ogre::VertexArrayObject* vao = mVaos[Ogre::VpNormal][0];
            Ogre::VertexBufferPackedVec vbVec = vao->getVertexBuffers();
            mVaoManager->destroyVertexArrayObject(vao);
            for (auto* vb : vbVec)
            {
                mVaoManager->destroyVertexBuffer(vb);
            }
            mVaos[Ogre::VpNormal].clear();
            mVaos[Ogre::VpShadow].clear();
            mVertexBuffer = nullptr;
        }
    }

    // ============================================================================
    //  Macroblock
    // ============================================================================

    void MovableText::_updateHlmsMacroblock()
    {
        assert(mpHlmsDatablock);
        Ogre::HlmsMacroblock mb;
        mb.mCullMode = Ogre::CULL_NONE;
        mb.mDepthCheck = !mOnTop;
        mb.mDepthBiasConstant = 1.0f;
        mb.mDepthBiasSlopeScale = 1.0f;
        mb.mDepthWrite = mOnTop;
        const Ogre::HlmsMacroblock* hlmsMb = Ogre::Root::getSingletonPtr()->getHlmsManager()->getMacroblock(mb);
        mpHlmsDatablock->setMacroblock(*hlmsMb);
    }

    // ============================================================================
    //  Font / datablock
    // ============================================================================

    void MovableText::setFontName(const Ogre::String& fontName)
    {
        Ogre::String hlmsDatablockName = mName + "Datablock";

        Ogre::HlmsDatablock* currentDatablock = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(hlmsDatablockName);
        if (nullptr != currentDatablock)
        {
            currentDatablock->getCreator()->destroyDatablock(currentDatablock->getName());
        }

        if (mFontName != fontName || !mpHlmsDatablock || !mpFont)
        {
            mFontName = fontName;

            mpFont = static_cast<Ogre::Font*>(Ogre::FontManager::getSingleton().getByName(mFontName).getPointer());
            if (!mpFont)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage("[MovableText]: Could not find font " + fontName);
                throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not find font " + fontName, "MovableText::setFontName");
            }
            mpFont->load();

            if (nullptr != mpHlmsDatablock)
            {
                mpHlmsDatablock->getCreator()->destroyDatablock(mpHlmsDatablock->getName());
                mpHlmsDatablock = nullptr;
            }

            Ogre::HlmsDatablock* fontDatablock = mpFont->getHlmsDatablock();
            const Ogre::HlmsBlendblock* hlmsBlendblock = Ogre::Root::getSingleton().getHlmsManager()->getBlendblock(*fontDatablock->getBlendblock());

            mpHlmsDatablock = fontDatablock->getCreator()->createDatablock(hlmsDatablockName, hlmsDatablockName, Ogre::HlmsMacroblock(), *hlmsBlendblock, Ogre::HlmsParamVec(), false);

            reinterpret_cast<Ogre::HlmsUnlitDatablock*>(mpHlmsDatablock)->setUseColour(true);
            reinterpret_cast<Ogre::HlmsUnlitDatablock*>(mpHlmsDatablock)->setTexture(0, reinterpret_cast<Ogre::HlmsUnlitDatablock*>(fontDatablock)->getTexture(0));
            reinterpret_cast<Ogre::HlmsUnlitDatablock*>(mpHlmsDatablock)->setTextureSwizzle(0, Ogre::HlmsUnlitDatablock::R_MASK, Ogre::HlmsUnlitDatablock::R_MASK, Ogre::HlmsUnlitDatablock::R_MASK, Ogre::HlmsUnlitDatablock::G_MASK);

            // NOTE: Do NOT call _updateHlmsMacroblock() or setDatablock() here.
            // Both trigger flushRenderables() -> calculateHashFor() -> getRenderOperation()
            // which crashes because the VAO does not exist yet at this point.
            // Both are called at the end of _setupGeometry() after the VAO is built.
            mNeedUpdate = true;
        }
    }

    // ============================================================================
    //  Geometry rebuild
    // ============================================================================

    void MovableText::_setupGeometry()
    {
        assert(mpFont);
        assert(mpHlmsDatablock);
        assert(mVaoManager);

        // ── Build CPU vertex data: float3 pos + float2 uv (5 floats/vert) ────
        std::vector<float> cpuVerts;
        cpuVerts.reserve(mCaption.size() * 6u * 5u);

        float left = UV_MIN;
        float top = UV_MAX;

        Ogre::Real spaceWidth = mSpaceWidth;
        if (spaceWidth == 0.0f)
        {
            spaceWidth = mpFont->getGlyphAspectRatio('A') * mCharHeight * UV_RANGE;
        }

        Ogre::Vector3 bmin(std::numeric_limits<float>::max());
        Ogre::Vector3 bmax(-std::numeric_limits<float>::max());
        float maxSqRadius = 0.0f;
        bool first = true;
        size_t actualVerts = 0;

        // Vertical offset — verbatim from original
        Ogre::Real verticalOffset = 0.0f;
        switch (mVerticalAlignment)
        {
        case V_ABOVE:
            verticalOffset = yOffset + (UV_RANGE / 2.0f) * mCharHeight;
            break;
        case V_CENTER:
            verticalOffset = (UV_RANGE / 4.0f) * mCharHeight;
            break;
        case V_BELOW:
            verticalOffset = 0.0f;
            break;
        }
        top += verticalOffset;
        for (auto ch : mCaption)
        {
            if (ch == '\n')
            {
                top += verticalOffset * UV_RANGE;
            }
        }

        auto trackPos = [&](float x, float y)
        {
            Ogre::Vector3 p(x, y, UV_MIN);
            if (first)
            {
                bmin = bmax = p;
                maxSqRadius = p.squaredLength();
                first = false;
            }
            else
            {
                bmin.makeFloor(p);
                bmax.makeCeil(p);
                maxSqRadius = std::max(maxSqRadius, p.squaredLength());
            }
        };

        auto pushVert = [&](float x, float y, float u, float v)
        {
            cpuVerts.push_back(x);
            cpuVerts.push_back(y);
            cpuVerts.push_back(0.0f);
            cpuVerts.push_back(u);
            cpuVerts.push_back(v);
            trackPos(x, y);
            ++actualVerts;
        };

        bool newLine = true;
        Ogre::Real len = 0.0f;

        for (auto it = mCaption.begin(); it != mCaption.end(); ++it)
        {
            if (newLine)
            {
                len = 0.0f;
                for (auto jt = it; jt != mCaption.end() && *jt != '\n'; ++jt)
                {
                    if (*jt == ' ')
                    {
                        len += spaceWidth;
                    }
                    else
                    {
                        len += mpFont->getGlyphAspectRatio((unsigned char)*jt) * mCharHeight * UV_RANGE;
                    }
                }
                newLine = false;
            }

            if (*it == '\n')
            {
                left = UV_MIN;
                top -= mCharHeight * UV_RANGE;
                newLine = true;
                continue;
            }

            if (*it == ' ')
            {
                left += spaceWidth;
                continue;
            }

            const float horiz = mpFont->getGlyphAspectRatio((unsigned char)*it);
            Ogre::Font::UVRect uv = mpFont->getGlyphTexCoords((unsigned char)*it);

            const float xL = (mHorizontalAlignment == H_LEFT) ? left : left - (len / 2.0f);
            const float xR = xL + horiz * mCharHeight * UV_RANGE;
            const float yT = top;
            const float yB = top - mCharHeight * UV_RANGE;

            // tri 0: upper-left, bottom-left, top-right
            pushVert(xL, yT, uv.left, uv.top);
            pushVert(xL, yB, uv.left, uv.bottom);
            pushVert(xR, yT, uv.right, uv.top);
            // tri 1: top-right, bottom-left, bottom-right
            pushVert(xR, yT, uv.right, uv.top);
            pushVert(xL, yB, uv.left, uv.bottom);
            pushVert(xR, yB, uv.right, uv.bottom);

            top += mCharHeight * UV_RANGE;
            left += horiz * mCharHeight * UV_RANGE;
        }

        if (actualVerts == 0)
        {
            mNeedUpdate = false;
            return;
        }

        // ── Destroy old VAO before rebuilding ─────────────────────────────────
        _destroyGeometry();

        // ── Upload vertex buffer ──────────────────────────────────────────────
        Ogre::VertexElement2Vec elements;
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        const size_t vertBytes = actualVerts * 5u * sizeof(float);
        float* gpuBuf = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertBytes, Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(gpuBuf, cpuVerts.data(), vertBytes);

        try
        {
            mVertexBuffer = mVaoManager->createVertexBuffer(elements, actualVerts, Ogre::BT_IMMUTABLE, gpuBuf, true /*keepAsShadow*/);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(gpuBuf, Ogre::MEMCATEGORY_GEOMETRY);
            Ogre::LogManager::getSingletonPtr()->logMessage("[MovableText] createVertexBuffer failed: " + e.getDescription(), Ogre::LML_CRITICAL);
            return;
        }

        // ── Build VAO (no index buffer — vertices already expanded) ──────────
        Ogre::VertexBufferPackedVec vbVec;
        vbVec.push_back(mVertexBuffer);

        Ogre::VertexArrayObject* vao = mVaoManager->createVertexArrayObject(vbVec, nullptr, Ogre::OT_TRIANGLE_LIST);

        mVaos[Ogre::VpNormal].push_back(vao);
        mVaos[Ogre::VpShadow].push_back(vao);

        // ── AABB — verbatim from original ─────────────────────────────────────
        if (first)
        {
            bmin = Ogre::Vector3(0.0f);
            bmax = Ogre::Vector3(1.0f);
        }
        Ogre::Aabb aabb = Ogre::Aabb::newFromExtents(bmin, bmax);
        float radius = Ogre::Math::Sqrt(maxSqRadius);

        mObjectData.mLocalAabb->setFromAabb(aabb, mObjectData.mIndex);
        mObjectData.mWorldAabb->setFromAabb(aabb, mObjectData.mIndex);
        mObjectData.mLocalRadius[mObjectData.mIndex] = radius;
        mObjectData.mWorldRadius[mObjectData.mIndex] = radius;

        // ── VAO is now in mVaos[] — safe to call _updateHlmsMacroblock() and
        //    setDatablock() because calculateHashFor() will take the V2 path. ─
        _updateHlmsMacroblock();
        this->setDatablock(mpHlmsDatablock);
        reinterpret_cast<Ogre::HlmsUnlitDatablock*>(mpHlmsDatablock)->setColour(mColor);

        mNeedUpdate = false;
    }

    // ============================================================================
    //  _notifyAttached — build geometry as soon as we have a parent node
    // ============================================================================

    void MovableText::_notifyAttached(Ogre::Node* parent)
    {
        Ogre::MovableObject::_notifyAttached(parent);

        if (parent && mNeedUpdate && mpFont && mpHlmsDatablock)
        {
            _setupGeometry();
        }
        else if (!parent)
        {
            _destroyGeometry();
        }
    }

    // ============================================================================
    //  _updateRenderQueue — verbatim from original
    // ============================================================================

    void MovableText::_updateRenderQueue(Ogre::RenderQueue* /*queue*/, Ogre::Camera* camera, const Ogre::Camera* /*lodCamera*/)
    {
        if (this->isVisible())
        {
            if (mNeedUpdate)
            {
                this->_setupGeometry();
            }

            mpCam = camera;
        }
    }

    // ============================================================================
    //  getWorldTransforms — verbatim from original
    // ============================================================================

    void MovableText::getWorldTransforms(Ogre::Matrix4* xform) const
    {
        if (this->isVisible() && mpCam)
        {
            Ogre::Matrix3 rot3x3, scale3x3 = Ogre::Matrix3::IDENTITY;

            mpCam->getDerivedOrientation().ToRotationMatrix(rot3x3);

            Ogre::Vector3 ppos = mParentNode->_getDerivedPosition() + Ogre::Vector3::UNIT_Y * mGlobalTranslation;
            ppos += rot3x3 * mLocalTranslation;

            scale3x3[0][0] = mParentNode->_getDerivedScale().x / 2.0f;
            scale3x3[1][1] = mParentNode->_getDerivedScale().y / 2.0f;
            scale3x3[2][2] = mParentNode->_getDerivedScale().z / 2.0f;

            *xform = Ogre::Matrix4(rot3x3 * scale3x3);
            xform->setTrans(ppos);
        }
    }

    // ============================================================================
    //  getRenderOperation — throws like ManualObject2.
    //  Never reached during normal operation because setDatablock() and
    //  _updateHlmsMacroblock() are only called after mVaos[] is populated,
    //  so calculateHashFor() always takes the V2 path via getVaos().
    // ============================================================================

    void MovableText::getRenderOperation(Ogre::v1::RenderOperation& /*op*/, bool /*casterPass*/)
    {
        OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
            "MovableText does not implement getRenderOperation. "
            "Do not place it in a v1-compatible render queue.",
            "MovableText::getRenderOperation");
    }

    // ============================================================================
    //  Public setters — verbatim from original
    // ============================================================================

    void MovableText::setCaption(const Ogre::String& caption)
    {
        if (caption != mCaption)
        {
            mCaption = caption;
            mNeedUpdate = true;
        }
    }

    void MovableText::setColor(const Ogre::ColourValue& color)
    {
        if (color != mColor)
        {
            mColor = color;
            if (mpHlmsDatablock)
            {
                reinterpret_cast<Ogre::HlmsUnlitDatablock*>(mpHlmsDatablock)->setColour(mColor);
            }
        }
    }

    void MovableText::setCharacterHeight(Ogre::Real h)
    {
        if (h != mCharHeight)
        {
            mCharHeight = h;
            mNeedUpdate = true;
        }
    }

    void MovableText::setSpaceWidth(Ogre::Real w)
    {
        if (w != mSpaceWidth)
        {
            mSpaceWidth = w;
            mNeedUpdate = true;
        }
    }

    void MovableText::setTextAlignment(const HorizontalAlignment& h, const VerticalAlignment& v)
    {
        if (mHorizontalAlignment != h)
        {
            mHorizontalAlignment = h;
            mNeedUpdate = true;
        }
        if (mVerticalAlignment != v)
        {
            mVerticalAlignment = v;
            mNeedUpdate = true;
        }
    }

    void MovableText::setGlobalTranslation(Ogre::Vector3 t)
    {
        mGlobalTranslation = t;
    }
    void MovableText::setLocalTranslation(Ogre::Vector3 t)
    {
        mLocalTranslation = t;
    }

    void MovableText::showOnTop(bool show)
    {
        if (mOnTop != show && mpHlmsDatablock)
        {
            mOnTop = show;
            // _updateHlmsMacroblock calls setMacroblock -> flushRenderables -> calculateHashFor.
            // Safe here because showOnTop() is only called after _setupGeometry() has run
            // and the VAO is already in mVaos[].
            _updateHlmsMacroblock();
        }
    }

    void MovableText::setTextYOffset(Ogre::Real offset)
    {
        yOffset = offset;
        mNeedUpdate = true;
    }

    void MovableText::forceUpdate(void)
    {
        mNeedUpdate = true;
        _setupGeometry();
    }

    void MovableText::update(Ogre::Real /*dt*/)
    {
    }

    // ============================================================================
    //  Factory
    // ============================================================================

    const Ogre::String& MovableTextFactory::getType() const
    {
        return sMovableTextType;
    }

    Ogre::MovableObject* MovableTextFactory::createInstanceImpl(Ogre::IdType id, Ogre::ObjectMemoryManager* mem, Ogre::SceneManager* sm, const Ogre::NameValuePairList* params)
    {
        return OGRE_NEW MovableText(id, mem, sm, params);
    }

    void MovableTextFactory::destroyInstance(Ogre::MovableObject* obj)
    {
        OGRE_DELETE obj;
    }

} // namespace NOWA