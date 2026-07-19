/*!
	@file
	@author		Albert Semenov
	@date		04/2009
	@update		2026 v2 fixes for Ogre-Next by NOWA-Engine
*/

#include <cstring>
#include "MyGUI_Ogre2Texture.h"
#include "MyGUI_DataManager.h"
#include "MyGUI_Ogre2RenderManager.h"
#include "MyGUI_Ogre2Diagnostic.h"
#include "MyGUI_Ogre2RTTexture.h"

#include <Ogre.h>
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpu.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStagingTexture.h"

#include "MyGUI_LastHeader.h"

namespace MyGUI
{

	Ogre2Texture::Ogre2Texture(const std::string& _name, const std::string& _group) :
		mTexture(nullptr),			// v2 fix: was never initialised — destroy() read garbage
		mName(_name),
		mGroup(_group),
		mNumElemBytes(0),
		mPixelFormat(Ogre::PFG_UNKNOWN),
		mListener(nullptr),
		mRenderTarget(nullptr),
		mDataReadyNotified(false)
	{
		mTmpData.data = nullptr;

		mDataBlock = HLMS_BLOCKS.createUnlitDataBlock(_name);
	}

	Ogre2Texture::~Ogre2Texture()
	{
		destroy();
	}

	const std::string& Ogre2Texture::getName() const
	{
		return mName;
	}

	void Ogre2Texture::saveToFile(const std::string& _filename)
	{
		Ogre::uchar* readrefdata = static_cast<Ogre::uchar*>(lock(TextureUsage::Read));

		// TODO: Save to file

		unlock();
	}

	void Ogre2Texture::setInvalidateListener(ITextureInvalidateListener* _listener)
	{
		mListener = _listener;
	}

	void Ogre2Texture::destroy()
	{
		if (mTmpData.data != nullptr)
		{
			// v2 fix: lock() allocates with OGRE_MALLOC_SIMD — freeing with
			// delete[] was undefined behavior.
			OGRE_FREE_SIMD(mTmpData.data, Ogre::MEMCATEGORY_RESOURCE);
			mTmpData.data = nullptr;
		}

		if (mRenderTarget != nullptr)
		{
			delete mRenderTarget;
			mRenderTarget = nullptr;
		}

		if (mTexture)
		{
			Ogre::TextureGpuManager *textureMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
			textureMgr->destroyTexture(mTexture);
			mTexture = nullptr;
		}

		mDataReadyNotified = false;

		// Note: mDataBlock is intentionally NOT destroyed here. Renderables may
		// still reference it this frame, and createUnlitDataBlock() reuses an
		// existing datablock by name on re-creation (Lax's 29.07.2024 fix).
	}

	int Ogre2Texture::getWidth()
	{
		return (int)mTexture->getWidth();
	}

	int Ogre2Texture::getHeight()
	{
		return (int)mTexture->getHeight();
	}

	void* Ogre2Texture::lock(TextureUsage _access)
	{
		if (_access == TextureUsage::Write)
		{
			const uint32 rowAlignment = 4u;
			const size_t dataSize = Ogre::PixelFormatGpuUtils::getSizeBytes(mTexture->getWidth(), mTexture->getHeight(), 1u, 1u,
				mTexture->getPixelFormat(),
				rowAlignment);
			uint8 *imageData = reinterpret_cast<uint8*>(OGRE_MALLOC_SIMD(dataSize, Ogre::MEMCATEGORY_RESOURCE));
			mTmpData.data = imageData;
			return mTmpData.data;
		}
		else
		{
			//TODO: FIXME
			return nullptr;
		}
	}

	void Ogre2Texture::unlock()
	{
		Ogre::TextureGpuManager *textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

		if (mTmpData.data != nullptr)
		{
			// write
			uint8 *imageData = reinterpret_cast<uint8*>(mTmpData.data);

			// v2 fix: row pitch computed from the format directly (with the same
			// rowAlignment=4 used in lock()) — _getSysRamCopyBytesPerRow assumed
			// a system RAM copy which we no longer hand over (see below).
			const size_t bytesPerRow = Ogre::PixelFormatGpuUtils::getSizeBytes(
				mTexture->getWidth(), 1u, 1u, 1u, mTexture->getPixelFormat(), 4u);

			if (mTexture->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
			{
				// v2 fix: pass nullptr, NOT imageData. Passing the pointer hands
				// ownership of it to the texture as its sysram copy — combined
				// with the manual staging upload + OGRE_FREE_SIMD below this was
				// a double-free / use-after-free race. The canonical manual
				// upload pattern is: transition with nullptr, upload via staging,
				// notifyDataIsReady() exactly once.
				mTexture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
				mTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
			}

			Ogre::StagingTexture *stagingTexture = textureManager->getStagingTexture(mTexture->getWidth(), mTexture->getHeight(),
				1u, 1u,
				mTexture->getPixelFormat());
			stagingTexture->startMapRegion();
			Ogre::TextureBox texBox = stagingTexture->mapRegion(mTexture->getWidth(), mTexture->getHeight(), 1u, 1u,
				mTexture->getPixelFormat());
			texBox.copyFrom(imageData, mTexture->getWidth(), mTexture->getHeight(), (uint32)bytesPerRow);
			stagingTexture->stopMapRegion();
			stagingTexture->upload(texBox, mTexture, 0, 0, 0, true);
			textureManager->removeStagingTexture(stagingTexture);
			stagingTexture = 0;

			OGRE_FREE_SIMD(imageData, Ogre::MEMCATEGORY_RESOURCE);
			mTmpData.data = nullptr;

			// v2 fix: only once per residency transition — repeated calls assert
			// in debug builds (e.g. a texture that is updated every frame).
			if (!mDataReadyNotified)
			{
				mTexture->notifyDataIsReady();
				mDataReadyNotified = true;
			}
		}
	}

	bool Ogre2Texture::isLocked()
	{
		return mTmpData.data != nullptr;
	}

	Ogre::PixelFormatGpu Ogre2Texture::convertFormat(PixelFormat _format)
	{
		if (_format == PixelFormat::L8) return Ogre::PFG_R8_UNORM;
		else if (_format == PixelFormat::L8A8) return Ogre::PFG_RG8_UNORM;
		else if (_format == PixelFormat::R8G8B8) return Ogre::PFG_RGBA8_UNORM;
		else if (_format == PixelFormat::R8G8B8A8) return Ogre::PFG_RGBA8_UNORM;

		return Ogre::PFG_UNKNOWN;
	}

	void Ogre2Texture::setFormat(PixelFormat _format)
	{
		mOriginalFormat = _format;
		mPixelFormat = convertFormat(_format);
		mNumElemBytes = 0;

		if (_format == PixelFormat::L8) mNumElemBytes = 1;
		else if (_format == PixelFormat::L8A8) mNumElemBytes = 2;
		else if (_format == PixelFormat::R8G8B8) mNumElemBytes = 3;
		else if (_format == PixelFormat::R8G8B8A8) mNumElemBytes = 4;
	}

	void Ogre2Texture::setUsage(TextureUsage _usage)
	{
		mOriginalUsage = _usage;
	}

	void Ogre2Texture::createManual(int _width, int _height, TextureUsage _usage, PixelFormat _format)
	{
		setFormat(_format);
		setUsage(_usage);
		uint32 textureFlags = 0; // Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB;
		if (_usage.isValue(TextureUsage::RenderTarget))
		{
			textureFlags |= Ogre::TextureFlags::RenderToTexture;
		}
		else
		{
			// v2: manually filled texture (fonts, canvas). ManualTexture stops
			// the manager from ever trying to stream it from a file.
			textureFlags |= Ogre::TextureFlags::ManualTexture;
		}

		Ogre::TextureGpuManager *textureMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
		mTexture = textureMgr->createOrRetrieveTexture(
			mName,
			Ogre::GpuPageOutStrategy::Discard,
			textureFlags,
			Ogre::TextureTypes::Type2D,
			Ogre::ResourceGroupManager::
			AUTODETECT_RESOURCE_GROUP_NAME);

		if (mTexture)
		{
			mTexture->setResolution(_width, _height);
			mTexture->setPixelFormat(mPixelFormat);
		}

		if (_usage.isValue(TextureUsage::RenderTarget))
		{
			if (mTexture->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
			{
				mTexture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
				mTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
			}
			// Render targets have no "data" to wait for — contents come from
			// rendering. Mark ready so the texture is usable for sampling.
			if (!mDataReadyNotified)
			{
				mTexture->notifyDataIsReady();
				mDataReadyNotified = true;
			}
		}
		setDataBlockTexture(mTexture);
	}

	void Ogre2Texture::loadFromFile(const std::string& _filename)
	{
		setUsage(TextureUsage::Default);

		Ogre::TextureGpuManager* manager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
		bool needLoadTexture = false;

		if ( !manager->hasTextureResource(_filename, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME) )
		{
			DataManager& resourcer = DataManager::getInstance();
			if (!resourcer.isDataExist(_filename))
			{
				MYGUI_PLATFORM_LOG(Error, "Texture '" + _filename + "' not found, set default texture");
			}
			else
			{
				needLoadTexture = true;
			}
		}
		else
		{
			needLoadTexture = true;
		}

		if (needLoadTexture)
		{
			// NOTE: PrefersLoadingFromFileAsSRGB assumes a gamma-correct
			// pipeline. If the GUI ever looks too dark / washed out, remove
			// this flag (and the matching one in Ogre2RenderManager::getTexture).
			Ogre::TextureGpuManager *textureMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
			mTexture = textureMgr->createOrRetrieveTexture(
				_filename,
				Ogre::GpuPageOutStrategy::Discard,
				Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
				Ogre::TextureTypes::Type2D,
				Ogre::ResourceGroupManager::
				AUTODETECT_RESOURCE_GROUP_NAME);

			if (mTexture)
			{
				if (mTexture->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
				{
					mTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				}
				mTexture->waitForData();
				// File-loaded textures are made ready by the streaming system —
				// notifyDataIsReady() must NOT be called manually for them.
				mDataReadyNotified = true;
			}
		}

		setFormatByOgreTexture();

		setDataBlockTexture(mTexture);
	}

	void Ogre2Texture::setFormatByOgreTexture()
	{
		mOriginalFormat = PixelFormat::Unknow;
		mPixelFormat = Ogre::PFG_UNKNOWN;
		mNumElemBytes = 0;

		if (mTexture)
		{
			mPixelFormat = mTexture->getPixelFormat();

			if (mPixelFormat == Ogre::PixelFormatGpu::PFG_R8_UNORM)
			{
				mOriginalFormat = PixelFormat::L8;
				mNumElemBytes = 1;
			}
			else if (mPixelFormat == Ogre::PixelFormatGpu::PFG_RG8_UNORM)
			{
				mOriginalFormat = PixelFormat::L8A8;
				mNumElemBytes = 2;
			}
			// v2 fix: the old code reported PFG_RGBA8_UNORM as R8G8B8 with
			// 3 bytes/elem (wrong pitch for anyone locking the texture), and
			// the follow-up branch was `|| PFG_RGBA8_UNORM_SRGB` without a
			// comparison — a constant-true expression that swallowed every
			// remaining format. Both RGBA8 variants are 4-byte R8G8B8A8.
			else if (mPixelFormat == Ogre::PixelFormatGpu::PFG_RGBA8_UNORM
				  || mPixelFormat == Ogre::PixelFormatGpu::PFG_RGBA8_UNORM_SRGB)
			{
				mOriginalFormat = PixelFormat::R8G8B8A8;
				mNumElemBytes = 4;
			}
			else
			{
				mOriginalFormat = PixelFormat::Unknow;
				mNumElemBytes = Ogre::PixelFormatGpuUtils::getBytesPerPixel(mPixelFormat);
			}
		}
	}

	void Ogre2Texture::loadResource(Ogre::Resource* resource)
	{
		// v1 leftover: TextureGpu does not go through Ogre::Resource, so this
		// never fires anymore. Device-lost texture invalidation would need a
		// TextureGpuListener instead (see notes).
		if (mListener)
			mListener->textureInvalidate(this);
	}

	IRenderTarget* Ogre2Texture::getRenderTarget()
	{
		if (mRenderTarget == nullptr)
			mRenderTarget = new Ogre2RTTexture(mTexture);

		return mRenderTarget;
	}

	void Ogre2Texture::setDataBlockTexture( Ogre::TextureGpu* _value )
	{
		// v2 fix: apply the clamp samplerblock — it existed but was never used.
		mDataBlock->setTexture(TEXTURE_UNIT_NUMBER, _value, HLMS_BLOCKS.getSamplerBlock());
	}

	Ogre::HlmsDatablock* Ogre2Texture::getDataBlock()
	{
		return mDataBlock;
	}

	const Ogre::uint8 Ogre2Texture::TEXTURE_UNIT_NUMBER = 0;

	const OgreHlmsBlocks Ogre2Texture::HLMS_BLOCKS;

} // namespace MyGUI
