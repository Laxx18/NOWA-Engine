/**
* File: MovableText.cpp
*
* description: This create create a billboarding object that display a text.
*
* @author	2003 by cTh see gavocanov@rambler.ru
* @update	2006 by barraq see nospam@barraquand.com
* @update	2012 to work with newer versions of OGRE by MindCalamity <mindcalamity@gmail.com>
*	- See "Notes" on: http://www.ogre3d.org/tikiwiki/tiki-editpage.php?page=MovableText
*/


#include "NOWAPrecompiled.h"
#include "MovableText.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsUnlitDatablock.h"

using namespace Ogre;

#define POS_TEX_BINDING    0
#define COLOUR_BINDING     1

#define UV_MIN		0
#define UV_MAX		1.0f
#define UV_RANGE	UV_MAX - UV_MIN

namespace
{
	Ogre::String type = "MovableText";
}

namespace NOWA
{

	MovableText::MovableText(IdType id, ObjectMemoryManager* objectMemoryManager, SceneManager* sceneManager, const NameValuePairList* params)
		: MovableObject(id, objectMemoryManager, sceneManager, RENDER_QUEUE_V1_OBJECTS_ALWAYS_IN_FOREGROUND)
		, mpCam(nullptr)
		, mpFont(nullptr)
		, mpHlmsDatablock(nullptr)
		, mName("")
		, mCaption(" ")
		// , mFontName("BarthowheelRegular")
		// , mFontName("BlueHighway-8")
		, mFontName("BlueHighway")
		, mCharHeight(1.0f)
		, mColor(ColourValue::White)
		, mTimeUntilNextToggle(0)
		, mSpaceWidth(0)
		, mUpdateColors(true)
		, mOnTop(false)
		, mHorizontalAlignment(H_LEFT)
		, mVerticalAlignment(V_BELOW)
		, mGlobalTranslation(0.0)
		, mLocalTranslation(0.0)
		, yOffset(0.0f)
	{
		if (params != 0)
		{
			NameValuePairList::const_iterator ni;

			ni = params->find("name");
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

		assert(mName != "" && "Trying to create MovableText without name");

		mRenderOp.vertexData = nullptr;
		this->setFontName(mFontName);
		this->_setupGeometry();
		this->setCastShadows(false);

		// Add the MovableText object to the renderables queue (MovableObject)
		mRenderables.push_back(this);
	}

	MovableText::~MovableText()
	{
		if (mpHlmsDatablock != nullptr)
		{
			this->_setNullDatablock();
			mpHlmsDatablock->getCreator()->destroyDatablock(mpHlmsDatablock->getName());
			mpHlmsDatablock = nullptr;
		}
		if (mRenderOp.vertexData)
			delete mRenderOp.vertexData;
	}

#if 0
	void MovableText::setFontName(const String& fontName)
	{
		String hlmsDatablockName = mName + "Datablock";

		HlmsDatablock* currentDatablock = Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(hlmsDatablockName);
		if (currentDatablock != nullptr)
		{
			currentDatablock->getCreator()->destroyDatablock(currentDatablock->getName());
		}

		if (mFontName != fontName || mpHlmsDatablock == nullptr || !mpFont)
		{
			mFontName = fontName;

			mpFont = (Font*)FontManager::getSingleton().getByName(mFontName).getPointer();
			if (!mpFont)
				throw Exception(Exception::ERR_ITEM_NOT_FOUND, "Could not find font " + fontName, "MovableText::setFontName");

			mpFont->load();
			if (mpHlmsDatablock != nullptr)
			{
				mpHlmsDatablock->getCreator()->destroyDatablock(mpHlmsDatablock->getName());
				mpHlmsDatablock = nullptr;
			}

			// Get font HLMS datablock
			HlmsDatablock* fontDatablock = mpFont->getHlmsDatablock();

			// Create a datablock for the MovableText
			const HlmsBlendblock* hlmsBlendblock = Root::getSingleton().getHlmsManager()->getBlendblock(*fontDatablock->getBlendblock());

			mpHlmsDatablock = fontDatablock->getCreator()->createDatablock(hlmsDatablockName, hlmsDatablockName, HlmsMacroblock(), *hlmsBlendblock, HlmsParamVec(), false);
			// reinterpret_cast<HlmsUnlitDatablock*>(mpHlmsDatablock)->setUseColour(true);

			reinterpret_cast<HlmsUnlitDatablock*>(mpHlmsDatablock)->setTexture(0, reinterpret_cast<HlmsUnlitDatablock*>(fontDatablock)->getTexture(0));

			_updateHlmsMacroblock();

			mNeedUpdate = true;
		}
	}
#endif

	void MovableText::setFontName(const String& fontName)
	{
		Ogre::String hlmsDatablockName = mName + "Datablock";

		Ogre::HlmsDatablock* currentDatablock = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(hlmsDatablockName);
		if (nullptr != currentDatablock)
		{
			currentDatablock->getCreator()->destroyDatablock(currentDatablock->getName());
		}

		if (mFontName != fontName || mpHlmsDatablock == nullptr || !mpFont)
		{
			mFontName = fontName;

			mpFont = (Ogre::Font*)Ogre::FontManager::getSingleton().getByName(mFontName).getPointer();
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

			// Get font HLMS datablock
			Ogre::HlmsDatablock* fontDatablock = mpFont->getHlmsDatablock();

			// Create a datablock for the MovableText
			const Ogre::HlmsBlendblock* hlmsBlendblock = Root::getSingleton().getHlmsManager()->getBlendblock(*fontDatablock->getBlendblock());

			mpHlmsDatablock = fontDatablock->getCreator()->createDatablock(hlmsDatablockName, hlmsDatablockName,
				HlmsMacroblock(), *hlmsBlendblock, HlmsParamVec(), false);

			reinterpret_cast<Ogre::HlmsUnlitDatablock*>(mpHlmsDatablock)->setUseColour(true);
			reinterpret_cast<Ogre::HlmsUnlitDatablock*>(mpHlmsDatablock)->setTexture(0, reinterpret_cast<Ogre::HlmsUnlitDatablock*>(fontDatablock)->getTexture(0));

			//Need to reset swizzle here or it will be broken colouring.
			reinterpret_cast<Ogre::HlmsUnlitDatablock*>(mpHlmsDatablock)->setTextureSwizzle(0, Ogre::HlmsUnlitDatablock::R_MASK, Ogre::HlmsUnlitDatablock::R_MASK,
				Ogre::HlmsUnlitDatablock::R_MASK, Ogre::HlmsUnlitDatablock::G_MASK);

			_updateHlmsMacroblock();

			mNeedUpdate = true;
		}
	}

	void MovableText::setCaption(const String& caption)
	{
		if (caption != mCaption)
		{
			mCaption = caption;
			mNeedUpdate = true;
		}
	}

	void MovableText::setColor(const ColourValue& color)
	{
		if (color != mColor)
		{
			mColor = color;
			mUpdateColors = true;
		}
	}

	void MovableText::setCharacterHeight(Real height)
	{
		if (height != mCharHeight)
		{
			mCharHeight = height;
			mNeedUpdate = true;
		}
	}

	void MovableText::setSpaceWidth(Real width)
	{
		if (width != mSpaceWidth)
		{
			mSpaceWidth = width;
			mNeedUpdate = true;
		}
	}

	void MovableText::setTextAlignment(const HorizontalAlignment& horizontalAlignment, const VerticalAlignment& verticalAlignment)
	{
		if (mHorizontalAlignment != horizontalAlignment)
		{
			mHorizontalAlignment = horizontalAlignment;
			mNeedUpdate = true;
		}
		if (mVerticalAlignment != verticalAlignment)
		{
			mVerticalAlignment = verticalAlignment;
			mNeedUpdate = true;
		}
	}

	void MovableText::setGlobalTranslation(Vector3 trans)
	{
		mGlobalTranslation = trans;
	}

	void MovableText::setLocalTranslation(Vector3 trans)
	{
		mLocalTranslation = trans;
	}

	void MovableText::showOnTop(bool show)
	{
		if (mOnTop != show && mpHlmsDatablock != nullptr)
		{
			mOnTop = show;
			_updateHlmsMacroblock();
		}
	}

	void MovableText::forceUpdate(void)
	{
		mNeedUpdate = true;
		_setupGeometry();
	}

	void MovableText::setTextYOffset(Ogre::Real yOffset)
	{
		this->yOffset = yOffset;
		mNeedUpdate = true;
	}

	void MovableText::_updateHlmsMacroblock()
	{
		assert(mpHlmsDatablock);

		HlmsMacroblock macroblockOptions;
		macroblockOptions.mCullMode = CULL_NONE;
		macroblockOptions.mDepthCheck = !mOnTop;
		macroblockOptions.mDepthBiasConstant = 1.0f;
		macroblockOptions.mDepthBiasSlopeScale = 1.0f;
		macroblockOptions.mDepthWrite = mOnTop;

		const HlmsMacroblock* hlmsMacroblock = Root::getSingletonPtr()->getHlmsManager()->getMacroblock(macroblockOptions);
		mpHlmsDatablock->setMacroblock(*hlmsMacroblock);
	}

	void MovableText::_setupGeometry()
	{
		assert(mpFont);
		assert(mpHlmsDatablock);

		unsigned int vertexCount = static_cast<unsigned int>(mCaption.size() * 6);

		if (mRenderOp.vertexData)
		{
			// Removed this test as it causes problems when replacing a caption
			// of the same size: replacing "Hello" with "hello"
			// as well as when changing the text alignment
			//if (mRenderOp.vertexData->vertexCount != vertexCount)
			{
				delete mRenderOp.vertexData;
				mRenderOp.vertexData = nullptr;
				mUpdateColors = true;
			}
		}

		if (!mRenderOp.vertexData)
			mRenderOp.vertexData = new v1::VertexData(nullptr);

		mRenderOp.indexData = 0;
		mRenderOp.vertexData->vertexStart = 0;
		mRenderOp.vertexData->vertexCount = vertexCount;
		mRenderOp.operationType = OperationType::OT_TRIANGLE_LIST;
		mRenderOp.useIndexes = false;

		v1::VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;
		v1::VertexBufferBinding* bind = mRenderOp.vertexData->vertexBufferBinding;
		size_t offset = 0;

		// create/bind positions/tex.ccord. buffer
		if (!decl->findElementBySemantic(VES_POSITION))
			decl->addElement(POS_TEX_BINDING, offset, VET_FLOAT3, VES_POSITION);

		offset += v1::VertexElement::getTypeSize(VET_FLOAT3);

		if (!decl->findElementBySemantic(VES_TEXTURE_COORDINATES))
			decl->addElement(POS_TEX_BINDING, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0);

		v1::HardwareVertexBufferSharedPtr ptbuf = v1::HardwareBufferManager::getSingleton().createVertexBuffer(decl->getVertexSize(POS_TEX_BINDING),
			mRenderOp.vertexData->vertexCount,
			v1::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
		bind->setBinding(POS_TEX_BINDING, ptbuf);

		// Colours - store these in a separate buffer because they change less often
		if (!decl->findElementBySemantic(VES_DIFFUSE))
			decl->addElement(COLOUR_BINDING, 0, VET_COLOUR, VES_DIFFUSE);

		v1::HardwareVertexBufferSharedPtr cbuf = v1::HardwareBufferManager::getSingleton().createVertexBuffer(decl->getVertexSize(COLOUR_BINDING),
			mRenderOp.vertexData->vertexCount,
			v1::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
		bind->setBinding(COLOUR_BINDING, cbuf);

		float* pPCBuff = static_cast<float*>(ptbuf->lock(v1::HardwareBuffer::HBL_DISCARD));

		float largestWidth = 0;
		float left = UV_MIN;
		float top = UV_MAX;

		Real spaceWidth = mSpaceWidth;
		// Derive space width from a capital A
		if (spaceWidth == 0)
			spaceWidth = mpFont->getGlyphAspectRatio('A') * mCharHeight * UV_RANGE;

		// for calculation of AABB
		Ogre::Vector3 min = Ogre::Vector3(0, 0, 0);
		Ogre::Vector3 max = Ogre::Vector3(1, 1, 1);
		Ogre::Vector3 currPos;
		float maxSquaredRadius = 0.0f;
		bool first = true;

		// Use iterator
		String::iterator i, iend;
		iend = mCaption.end();
		bool newLine = true;
		Real len = 0.0f;

		Real verticalOffset = 0;
		switch (mVerticalAlignment)
		{
		case MovableText::V_ABOVE:
			verticalOffset = this->yOffset + (UV_RANGE / 2) * mCharHeight;
			break;
		case MovableText::V_CENTER:
			verticalOffset = (UV_RANGE / 4) * mCharHeight;
			break;
		case MovableText::V_BELOW:
			verticalOffset = 0;
			break;
		}
		// Raise the first line of the caption
		top += verticalOffset;
		for (i = mCaption.begin(); i != iend; ++i)
		{
			if (*i == '\n')
				top += verticalOffset * UV_RANGE;
		}

		for (i = mCaption.begin(); i != iend; ++i)
		{
			if (newLine)
			{
				len = 0.0f;
				for (String::iterator j = i; j != iend && *j != '\n'; j++)
				{
					if (*j == ' ')
						len += spaceWidth;
					else
						len += mpFont->getGlyphAspectRatio((unsigned char)*j) * mCharHeight * UV_RANGE;
				}
				newLine = false;
			}

			if (*i == '\n')
			{
				left = UV_MIN;
				top -= mCharHeight * UV_RANGE;
				newLine = true;

				// Bugfix by Wladimir Lukutin - thanks :)
				// Also reduce tri count
				mRenderOp.vertexData->vertexCount -= 6;
				// Bugfix end.

				continue;
			}

			if (*i == ' ')
			{
				// Just leave a gap, no tris
				left += spaceWidth;
				// Also reduce tri count
				mRenderOp.vertexData->vertexCount -= 6;
				continue;
			}

			Real horiz_height = mpFont->getGlyphAspectRatio((unsigned char)*i);
			Real u1, u2, v1, v2;
			Ogre::Font::UVRect utmp;
			utmp = mpFont->getGlyphTexCoords((unsigned char)*i);
			u1 = utmp.left;
			u2 = utmp.right;
			v1 = utmp.top;
			v2 = utmp.bottom;

			// each vert is (x, y, z, u, v)
			//-------------------------------------------------------------------------------------
			// First tri
			//
			// Upper left
			if (mHorizontalAlignment == MovableText::H_LEFT)
				*pPCBuff++ = left;
			else
				*pPCBuff++ = left - (len / 2);
			*pPCBuff++ = top;
			*pPCBuff++ = 0;
			*pPCBuff++ = u1;
			*pPCBuff++ = v1;

			// Deal with bounds
			if (mHorizontalAlignment == MovableText::H_LEFT)
				currPos = Ogre::Vector3(left, top, UV_MIN);
			else
				currPos = Ogre::Vector3(left - (len / 2), top, UV_MIN);
			if (first)
			{
				min = max = currPos;
				maxSquaredRadius = currPos.squaredLength();
				first = false;
			}
			else
			{
				min.makeFloor(currPos);
				max.makeCeil(currPos);
				maxSquaredRadius = std::max(maxSquaredRadius, currPos.squaredLength());
			}

			top -= mCharHeight * UV_RANGE;

			// Bottom left
			if (mHorizontalAlignment == MovableText::H_LEFT)
				*pPCBuff++ = left;
			else
				*pPCBuff++ = left - (len / 2);
			*pPCBuff++ = top;
			*pPCBuff++ = 0;
			*pPCBuff++ = u1;
			*pPCBuff++ = v2;

			// Deal with bounds
			if (mHorizontalAlignment == MovableText::H_LEFT)
				currPos = Ogre::Vector3(left, top, UV_MIN);
			else
				currPos = Ogre::Vector3(left - (len / 2), top, UV_MIN);
			min.makeFloor(currPos);
			max.makeCeil(currPos);
			maxSquaredRadius = std::max(maxSquaredRadius, currPos.squaredLength());

			top += mCharHeight * UV_RANGE;
			left += horiz_height * mCharHeight * UV_RANGE;

			// Top right
			if (mHorizontalAlignment == MovableText::H_LEFT)
				*pPCBuff++ = left;
			else
				*pPCBuff++ = left - (len / 2);
			*pPCBuff++ = top;
			*pPCBuff++ = 0;
			*pPCBuff++ = u2;
			*pPCBuff++ = v1;
			//-------------------------------------------------------------------------------------

			// Deal with bounds
			if (mHorizontalAlignment == MovableText::H_LEFT)
				currPos = Ogre::Vector3(left, top, UV_MIN);
			else
				currPos = Ogre::Vector3(left - (len / 2), top, UV_MIN);
			min.makeFloor(currPos);
			max.makeCeil(currPos);
			maxSquaredRadius = std::max(maxSquaredRadius, currPos.squaredLength());

			//-------------------------------------------------------------------------------------
			// Second tri
			//
			// Top right (again)
			if (mHorizontalAlignment == MovableText::H_LEFT)
				*pPCBuff++ = left;
			else
				*pPCBuff++ = left - (len / 2);
			*pPCBuff++ = top;
			*pPCBuff++ = 0;
			*pPCBuff++ = u2;
			*pPCBuff++ = v1;

			currPos = Ogre::Vector3(left, top, UV_MIN);
			min.makeFloor(currPos);
			max.makeCeil(currPos);
			maxSquaredRadius = std::max(maxSquaredRadius, currPos.squaredLength());

			top -= mCharHeight * UV_RANGE;
			left -= horiz_height * mCharHeight * UV_RANGE;

			// Bottom left (again)
			if (mHorizontalAlignment == MovableText::H_LEFT)
				*pPCBuff++ = left;
			else
				*pPCBuff++ = left - (len / 2);
			*pPCBuff++ = top;
			*pPCBuff++ = 0;
			*pPCBuff++ = u1;
			*pPCBuff++ = v2;

			currPos = Ogre::Vector3(left, top, UV_MIN);
			min.makeFloor(currPos);
			max.makeCeil(currPos);
			maxSquaredRadius = std::max(maxSquaredRadius, currPos.squaredLength());

			left += horiz_height * mCharHeight * UV_RANGE;

			// Bottom right
			if (mHorizontalAlignment == MovableText::H_LEFT)
				*pPCBuff++ = left;
			else
				*pPCBuff++ = left - (len / 2);
			*pPCBuff++ = top;
			*pPCBuff++ = 0;
			*pPCBuff++ = u2;
			*pPCBuff++ = v2;
			//-------------------------------------------------------------------------------------

			currPos = Ogre::Vector3(left, top, UV_MIN);
			min.makeFloor(currPos);
			max.makeCeil(currPos);
			maxSquaredRadius = std::max(maxSquaredRadius, currPos.squaredLength());

			// Go back up with top
			top += mCharHeight * UV_RANGE;

			float currentWidth = (left + UV_MAX) / UV_RANGE - 0;
			if (currentWidth > largestWidth)
				largestWidth = currentWidth;
		}

		// Unlock vertex buffer
		ptbuf->unlock();

		// update AABB/Sphere radius
		Aabb aabb = Ogre::Aabb::newFromExtents(min, max);
		float radius = Ogre::Math::Sqrt(maxSquaredRadius);
		mObjectData.mLocalAabb->setFromAabb(aabb, mObjectData.mIndex);
		mObjectData.mWorldAabb->setFromAabb(aabb, mObjectData.mIndex);
		mObjectData.mLocalRadius[mObjectData.mIndex] = radius;
		mObjectData.mWorldRadius[mObjectData.mIndex] = radius;


		if (mUpdateColors)
			this->_updateColors();

		mNeedUpdate = false;

		this->setDatablock(mpHlmsDatablock);
	}

	void MovableText::_updateColors(void)
	{
		assert(mpFont);
		assert(mpHlmsDatablock);

		// Convert to system-specific
		RGBA color;
		Root::getSingleton().convertColourValue(mColor, &color);

		//uint8 val8;
		//// Blue
		//val8 = static_cast<uint8>(mColor.b * 255);
		//color = val8 << 24;

		//// Green
		//val8 = static_cast<uint8>(mColor.g * 255);
		//color += val8 << 16;

		//// Red
		//val8 = static_cast<uint8>(mColor.r * 255);
		//color += val8 << 8;

		//// Alpha
		//val8 = static_cast<uint8>(mColor.a * 255);
		//color += val8;

		v1::HardwareVertexBufferSharedPtr vbuf = mRenderOp.vertexData->vertexBufferBinding->getBuffer(COLOUR_BINDING);
		RGBA* pDest = static_cast<RGBA*>(vbuf->lock(v1::HardwareBuffer::HBL_DISCARD));
		for (int i = 0; i < (int)mRenderOp.vertexData->vertexCount; ++i)
			*pDest++ = color;
		vbuf->unlock();

		mUpdateColors = false;
	}

	void MovableText::getWorldTransforms(Matrix4* xform) const
	{
		if (this->isVisible() && mpCam)
		{
			Matrix3 rot3x3, scale3x3 = Matrix3::IDENTITY;

			// store rotation in a matrix
			mpCam->getDerivedOrientation().ToRotationMatrix(rot3x3);

			// parent node position
			Vector3 ppos = mParentNode->_getDerivedPosition() + Vector3::UNIT_Y * mGlobalTranslation;
			ppos += rot3x3 * mLocalTranslation;

			// apply scale
			scale3x3[0][0] = mParentNode->_getDerivedScale().x / 2;
			scale3x3[1][1] = mParentNode->_getDerivedScale().y / 2;
			scale3x3[2][2] = mParentNode->_getDerivedScale().z / 2;

			// apply all transforms to xform       
			*xform = (rot3x3 * scale3x3);
			xform->setTrans(ppos);
		}
	}

	void MovableText::getRenderOperation(v1::RenderOperation& op, bool casterPass)
	{
		if (this->isVisible())
		{
			if (mNeedUpdate)
				this->_setupGeometry();
			if (mUpdateColors)
				this->_updateColors();
		}
		op = mRenderOp;
	}

	void MovableText::_updateRenderQueue(RenderQueue* queue, Camera* camera, const Camera* lodCamera)
	{
		if (this->isVisible())
		{
			if (mNeedUpdate)
				this->_setupGeometry();
			if (mUpdateColors)
				this->_updateColors();

			mpCam = camera;
		}
	}

	void MovableText::update(Ogre::Real dt)
	{
		if (this->isVisible() && mpCam)
		{
			// getParentSceneNode()->_setDerivedOrientation(mpCam->getDerivedOrientation());
			// getParentSceneNode()->lookAt(mpCam->_getCachedDerivedPosition(), Ogre::Node::TransformSpace::TS_PARENT);
		}
	}

	//-----------------------------------------------------------------------
	const String& MovableTextFactory::getType(void) const
	{
		return type;
	}
	//-----------------------------------------------------------------------
	MovableObject* MovableTextFactory::createInstanceImpl(IdType id, ObjectMemoryManager* objectMemoryManager, SceneManager* sceneManager, const NameValuePairList* params)
	{
		return OGRE_NEW MovableText(id, objectMemoryManager, sceneManager, params);
	}
	//-----------------------------------------------------------------------
	void MovableTextFactory::destroyInstance(MovableObject* obj)
	{
		OGRE_DELETE obj;
	}

}; // namespace end
