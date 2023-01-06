/*!
	@file
	@author		Albert Semenov
	@date		04/2009
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
		mName(_name),
		mGroup(_group),
		mNumElemBytes(0),
		mPixelFormat(Ogre::PFG_UNKNOWN),
		mListener(nullptr),
		mRenderTarget(nullptr)
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
			delete [] (uint8*)mTmpData.data;
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
			const size_t bytesPerRow = mTexture->_getSysRamCopyBytesPerRow(0);

			if (mTexture->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
			{
				mTexture->_transitionTo(Ogre::GpuResidency::Resident, imageData);
				mTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
			}
		
			Ogre::StagingTexture *stagingTexture = textureManager->getStagingTexture(mTexture->getWidth(), mTexture->getHeight(),
				1u, 1u,
				mTexture->getPixelFormat());
			stagingTexture->startMapRegion();
			Ogre::TextureBox texBox = stagingTexture->mapRegion(mTexture->getWidth(), mTexture->getHeight(), 1u, 1u,
				mTexture->getPixelFormat());
			texBox.copyFrom(imageData, mTexture->getWidth(), mTexture->getHeight(), bytesPerRow);
			stagingTexture->stopMapRegion();
			stagingTexture->upload(texBox, mTexture, 0, 0, 0, true);
			textureManager->removeStagingTexture(stagingTexture);
			stagingTexture = 0;
		
			OGRE_FREE_SIMD(imageData, Ogre::MEMCATEGORY_RESOURCE);
			mTmpData.data = nullptr;
			
			mTexture->notifyDataIsReady();
			// textureManager->waitForStreamingCompletion();
		}
	}

	bool Ogre2Texture::isLocked()
	{
		// TODO: FIXME
		return false;
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
		// TODO: Ogre::TextureFlags::Manual

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
			textureFlags |= Ogre::TextureFlags::RenderToTexture;
			if (mTexture->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
			{
				mTexture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
				mTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
			}
			// textureMgr->waitForStreamingCompletion();
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
			else if (mPixelFormat == Ogre::PixelFormatGpu::PFG_RGBA8_UNORM)
			{
				mOriginalFormat = PixelFormat::R8G8B8;
				mNumElemBytes = 3;
			}
			else if (mPixelFormat == Ogre::PixelFormatGpu::PFG_RGBA8_UNORM || Ogre::PixelFormatGpu::PFG_RGBA8_UNORM_SRGB)
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
		mDataBlock->setTexture(TEXTURE_UNIT_NUMBER, _value);
	}

	Ogre::HlmsDatablock* Ogre2Texture::getDataBlock()
	{
		return mDataBlock;
	}

	const Ogre::uint8 Ogre2Texture::TEXTURE_UNIT_NUMBER = 0;

	const OgreHlmsBlocks Ogre2Texture::HLMS_BLOCKS;

} // namespace MyGUI
