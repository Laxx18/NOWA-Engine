#include "NOWAPrecompiled.h"
#include "DecalsModule.h"

#include <vector>

#include "modules/GraphicsModule.h"

#include "OgreRoot.h"
#include "OgreRenderSystem.h"
#include "OgreSceneManager.h"
#include "OgreSceneNode.h"
#include "OgreDecal.h"
#include "OgreTextureGpuManager.h"
#include "OgreTextureGpu.h"
#include "OgreImage2.h"
#include "OgreResourceGroupManager.h"

namespace NOWA
{
	DecalsModule::DecalsModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		sceneManager(nullptr),
		decalPoolsReady(false),
		disabledDiffuse(nullptr),
		disabledNormal(nullptr),
		disabledEmissive(nullptr)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DecalsModule] Module created");
	}

	DecalsModule::~DecalsModule()
	{

	}

	void DecalsModule::setPoolSettings(const DecalTexturePoolSettings& settings)
	{
		this->poolSettings = settings;
	}

	const DecalsModule::DecalTexturePoolSettings& DecalsModule::getPoolSettings(void) const
	{
		return this->poolSettings;
	}

	Ogre::TextureGpu* DecalsModule::getDisabledDiffuseTexture(void) const
	{
		return this->disabledDiffuse;
	}

	Ogre::TextureGpu* DecalsModule::getDisabledNormalTexture(void) const
	{
		return this->disabledNormal;
	}

	Ogre::TextureGpu* DecalsModule::getDisabledEmissiveTexture(void) const
	{
		return this->disabledEmissive;
	}

	void DecalsModule::init(Ogre::SceneManager* sceneManager)
	{
		if (nullptr == sceneManager)
		{
			return;
		}

		if (this->sceneManager == sceneManager && true == this->decalPoolsReady)
		{
			return;
		}

		if (nullptr != this->sceneManager && this->sceneManager != sceneManager)
		{
			this->destroyContent();
		}

		this->sceneManager = sceneManager;
		this->ensureDecalPools(sceneManager);
	}

	void DecalsModule::ensureDecalPools(Ogre::SceneManager* sceneManager)
	{
		if (true == this->decalPoolsReady)
		{
			return;
		}

		auto* root = Ogre::Root::getSingletonPtr();
		if (nullptr == root)
		{
			return;
		}

		auto* rs = root->getRenderSystem();
		if (nullptr == rs)
		{
			return;
		}

		auto* textureManager = rs->getTextureGpuManager();
		if (nullptr == textureManager)
		{
			return;
		}

		if (true == this->decalPoolsReady)
		{
			return;
		}

		const Ogre::uint32 poolId = this->poolSettings.poolId;
		const Ogre::uint32 res = this->poolSettings.resolution;
		const Ogre::uint8 numMipmaps = this->poolSettings.numMipmaps;
		const Ogre::uint16 maxTexturesInArray = this->poolSettings.maxTexturesInArray;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::ensureDecalPools", _6(sceneManager, textureManager, poolId, res, numMipmaps, maxTexturesInArray),
		{
			/*
			Decals support up to 3 texture arrays (diffuse, normals, emissive).
			All textures within the same array must share resolution & format.

			We reserve the pools early to guarantee that all decal textures end up in
			the correct arrays (as done in the Ogre-Next decals sample).
			*/
			textureManager->reservePoolId(poolId, res, res, numMipmaps, maxTexturesInArray, Ogre::PFG_RGBA8_UNORM_SRGB);
			textureManager->reservePoolId(poolId, res, res, numMipmaps, maxTexturesInArray, Ogre::PFG_RG8_SNORM);

			/*
			Create a disabled diffuse, normal and emissive slice at index 0.
			This allows each decal to disable a specific channel by referencing the
			disabled texture.
			*/
			{
				Ogre::uint8* blackBuffer = reinterpret_cast<Ogre::uint8*>(OGRE_MALLOC_SIMD(res * res * 4u, Ogre::MEMCATEGORY_RESOURCE));
				memset(blackBuffer, 0, res * res * 4u);

				Ogre::Image2 image;
				image.loadDynamicImage(blackBuffer, res, res, 1u, Ogre::TextureTypes::Type2D, Ogre::PFG_RGBA8_UNORM_SRGB, true);
				image.generateMipmaps(false, Ogre::Image2::FILTER_NEAREST);

				this->disabledDiffuse = textureManager->createOrRetrieveTexture("decals_disabled_diffuse", Ogre::GpuPageOutStrategy::Discard,
					Ogre::TextureFlags::AutomaticBatching | Ogre::TextureFlags::ManualTexture,
					Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0, poolId);

				this->disabledDiffuse->setResolution(image.getWidth(), image.getHeight());
				this->disabledDiffuse->setNumMipmaps(image.getNumMipmaps());
				this->disabledDiffuse->setPixelFormat(image.getPixelFormat());
				this->disabledDiffuse->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				image.uploadTo(this->disabledDiffuse, 0, this->disabledDiffuse->getNumMipmaps() - 1u);
				image.freeMemory();
			}

			{
				Ogre::uint8* blackBuffer = reinterpret_cast<Ogre::uint8*>(OGRE_MALLOC_SIMD(res * res * 2u, Ogre::MEMCATEGORY_RESOURCE));
				memset(blackBuffer, 0, res * res * 2u);

				Ogre::Image2 image;
				image.loadDynamicImage(blackBuffer, res, res, 1u, Ogre::TextureTypes::Type2D, Ogre::PFG_RG8_SNORM, true);
				image.generateMipmaps(false, Ogre::Image2::FILTER_NEAREST);

				this->disabledNormal = textureManager->createOrRetrieveTexture("decals_disabled_normals", Ogre::GpuPageOutStrategy::Discard,
					Ogre::TextureFlags::AutomaticBatching | Ogre::TextureFlags::ManualTexture,
					Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0, poolId);

				this->disabledNormal->setResolution(image.getWidth(), image.getHeight());
				this->disabledNormal->setNumMipmaps(image.getNumMipmaps());
				this->disabledNormal->setPixelFormat(image.getPixelFormat());
				this->disabledNormal->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				image.uploadTo(this->disabledNormal, 0, this->disabledNormal->getNumMipmaps() - 1u);
				image.freeMemory();
			}

			{
				Ogre::uint8* blackBuffer = reinterpret_cast<Ogre::uint8*>(OGRE_MALLOC_SIMD(res * res * 4u, Ogre::MEMCATEGORY_RESOURCE));
				memset(blackBuffer, 0, res * res * 4u);

				Ogre::Image2 image;
				image.loadDynamicImage(blackBuffer, res, res, 1u, Ogre::TextureTypes::Type2D, Ogre::PFG_RGBA8_UNORM_SRGB, true);
				image.generateMipmaps(false, Ogre::Image2::FILTER_NEAREST);

				this->disabledEmissive = textureManager->createOrRetrieveTexture("decals_disabled_emissive", Ogre::GpuPageOutStrategy::Discard,
					Ogre::TextureFlags::AutomaticBatching | Ogre::TextureFlags::ManualTexture,
					Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0, poolId);

				this->disabledEmissive->setResolution(image.getWidth(), image.getHeight());
				this->disabledEmissive->setNumMipmaps(image.getNumMipmaps());
				this->disabledEmissive->setPixelFormat(image.getPixelFormat());
				this->disabledEmissive->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				image.uploadTo(this->disabledEmissive, 0, this->disabledEmissive->getNumMipmaps() - 1u);
				image.freeMemory();
			}

			// The SceneManager needs to know which pools/arrays to sample from.
			// Any texture belonging to the correct pool is sufficient.
			sceneManager->setDecalsDiffuse(this->disabledDiffuse);
			sceneManager->setDecalsNormals(this->disabledNormal);
			sceneManager->setDecalsEmissive(this->disabledEmissive);

			this->decalPoolsReady = true;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DecalsModule] Decal pools reserved and disabled textures created.");
		});
	}

	Ogre::TextureGpu* DecalsModule::getOrCreateTexture(Ogre::SceneManager* sceneManager, const Ogre::String& textureName, Ogre::uint32 poolId, bool isNormalMap, bool isEmissive)
	{
		if (textureName.empty())
		{
			return nullptr;
		}

		auto* root = Ogre::Root::getSingletonPtr();
		if (nullptr == root)
		{
			return nullptr;
		}

		auto* rs = root->getRenderSystem();
		if (nullptr == rs)
		{
			return nullptr;
		}

		auto* textureManager = rs->getTextureGpuManager();
		if (nullptr == textureManager)
		{
			return nullptr;
		}

		if (true == isNormalMap)
		{
			auto it = this->normalTextures.find(textureName);
			if (it != this->normalTextures.end() && nullptr != it->second.texture)
			{
				++it->second.refCount;
				it->second.texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				return it->second.texture;
			}

			Ogre::TextureGpu* tex = textureManager->createOrRetrieveTexture( textureName, Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::NormalMap, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, poolId);

			if (nullptr == tex)
			{
				return nullptr;
			}

			tex->scheduleTransitionTo(Ogre::GpuResidency::Resident);
			TextureEntry entry;
			entry.texture = tex;
			entry.refCount = 1u;
			this->normalTextures[textureName] = entry;
			return tex;
		}

		// Diffuse & emissive share the same pool/format, but we keep separate bookkeeping.
		std::unordered_map<Ogre::String, TextureEntry>& mapRef = (true == isEmissive) ? this->emissiveTextures : this->diffuseTextures;

		auto it = mapRef.find(textureName);
		if (it != mapRef.end() && nullptr != it->second.texture)
		{
			++it->second.refCount;
			it->second.texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
			return it->second.texture;
		}

		Ogre::TextureGpu* tex = textureManager->createOrRetrieveTexture(textureName, Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::Diffuse, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, poolId);

		if (nullptr == tex)
		{
			return nullptr;
		}

		tex->scheduleTransitionTo(Ogre::GpuResidency::Resident);
		TextureEntry entry;
		entry.texture = tex;
		entry.refCount = 1u;
		mapRef[textureName] = entry;

		return tex;
	}

	Ogre::Decal* DecalsModule::createDecal(Ogre::SceneManager* sceneManager, Ogre::SceneNode* parentNode)
	{
		if (nullptr == sceneManager || nullptr == parentNode)
		{
			return nullptr;
		}

		this->init(sceneManager);
		this->ensureDecalPools(sceneManager);

		Ogre::Decal* createdDecal = nullptr;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::createDecal", _3(sceneManager, parentNode, outDecal = &createdDecal),
		{
			Ogre::Decal* decal = sceneManager->createDecal();
			decal->setQueryFlags(0);
			parentNode->attachObject(decal);

			if (nullptr != this->disabledDiffuse)
			{
				decal->setDiffuseTexture(this->disabledDiffuse);
			}
			if (nullptr != this->disabledNormal)
			{
				decal->setNormalTexture(this->disabledNormal);
			}
			if (nullptr != this->disabledEmissive)
			{
				decal->setEmissiveTexture(this->disabledEmissive);
			}

			*outDecal = decal;
		});

		if (nullptr != createdDecal)
		{
			this->decals.insert(createdDecal);
			this->decalEntries[createdDecal] = DecalEntry();
		}

		return createdDecal;
	}

	void DecalsModule::destroyDecal(Ogre::SceneManager* sceneManager, Ogre::Decal*& decal)
	{
		if (nullptr == sceneManager || nullptr == decal)
		{
			return;
		}

		Ogre::Decal* decalToDestroy = decal;
		decal = nullptr;

		auto itEntry = this->decalEntries.find(decalToDestroy);
		if (itEntry != this->decalEntries.end())
		{
			if (false == itEntry->second.diffuseTextureName.empty())
			{
				auto itTex = this->diffuseTextures.find(itEntry->second.diffuseTextureName);
				if (itTex != this->diffuseTextures.end() && itTex->second.refCount > 0u)
				{
					--itTex->second.refCount;
				}
			}

			if (false == itEntry->second.normalTextureName.empty())
			{
				auto itTex = this->normalTextures.find(itEntry->second.normalTextureName);
				if (itTex != this->normalTextures.end() && itTex->second.refCount > 0u)
				{
					--itTex->second.refCount;
				}
			}

			if (false == itEntry->second.emissiveTextureName.empty())
			{
				auto itTex = this->emissiveTextures.find(itEntry->second.emissiveTextureName);
				if (itTex != this->emissiveTextures.end() && itTex->second.refCount > 0u)
				{
					--itTex->second.refCount;
				}
			}
		}

		this->decals.erase(decalToDestroy);
		this->decalEntries.erase(decalToDestroy);

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::destroyDecal", _2(sceneManager, decalToDestroy),
		{
			sceneManager->destroyDecal(decalToDestroy);
		});
	}

	void DecalsModule::setDecalTextures(Ogre::SceneManager* sceneManager, Ogre::Decal* decal, const Ogre::String& diffuseTextureName, const Ogre::String& normalTextureName, const Ogre::String& emissiveTextureName)
	{
		if (nullptr == sceneManager || nullptr == decal)
		{
			return;
		}

		this->init(sceneManager);
		this->ensureDecalPools(sceneManager);

		// Release old bookkeeping first.
		auto itEntry = this->decalEntries.find(decal);
		if (itEntry != this->decalEntries.end())
		{
			if (false == itEntry->second.diffuseTextureName.empty())
			{
				auto itTex = this->diffuseTextures.find(itEntry->second.diffuseTextureName);
				if (itTex != this->diffuseTextures.end() && itTex->second.refCount > 0u)
				{
					--itTex->second.refCount;
				}
			}

			if (false == itEntry->second.normalTextureName.empty())
			{
				auto itTex = this->normalTextures.find(itEntry->second.normalTextureName);
				if (itTex != this->normalTextures.end() && itTex->second.refCount > 0u)
				{
					--itTex->second.refCount;
				}
			}

			if (false == itEntry->second.emissiveTextureName.empty())
			{
				auto itTex = this->emissiveTextures.find(itEntry->second.emissiveTextureName);
				if (itTex != this->emissiveTextures.end() && itTex->second.refCount > 0u)
				{
					--itTex->second.refCount;
				}
			}
		}

		const Ogre::uint32 poolId = this->poolSettings.poolId;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::setDecalTextures", _6(sceneManager, decal, diffuseTextureName, normalTextureName, emissiveTextureName, poolId),
		{
			Ogre::TextureGpu* diffuseTex = this->disabledDiffuse;
			Ogre::TextureGpu* normalTex = this->disabledNormal;
			Ogre::TextureGpu* emissiveTex = this->disabledEmissive;

			if (false == diffuseTextureName.empty())
			{
				diffuseTex = this->getOrCreateTexture(sceneManager, diffuseTextureName, poolId, false, false);
				if (nullptr == diffuseTex)
				{
					diffuseTex = this->disabledDiffuse;
				}
			}

			if (false == normalTextureName.empty())
			{
				normalTex = this->getOrCreateTexture(sceneManager, normalTextureName, poolId, true, false);
				if (nullptr == normalTex)
				{
					normalTex = this->disabledNormal;
				}
			}

			if (false == emissiveTextureName.empty())
			{
				emissiveTex = this->getOrCreateTexture(sceneManager, emissiveTextureName, poolId, false, true);
				if (nullptr == emissiveTex)
				{
					emissiveTex = this->disabledEmissive;
				}
			}
			else
			{
				// Hlms optimisation: emissive == diffuse
				emissiveTex = diffuseTex;
			}

			decal->setDiffuseTexture(diffuseTex);
			decal->setNormalTexture(normalTex);
			decal->setEmissiveTexture(emissiveTex);

			// Ensure SceneManager samples from our pools.
			sceneManager->setDecalsDiffuse(diffuseTex ? diffuseTex : this->disabledDiffuse);
			sceneManager->setDecalsNormals(normalTex ? normalTex : this->disabledNormal);
			sceneManager->setDecalsEmissive(emissiveTex ? emissiveTex : this->disabledEmissive);
		});

		// Update bookkeeping on main thread.
		this->decalEntries[decal].diffuseTextureName = diffuseTextureName;
		this->decalEntries[decal].normalTextureName = normalTextureName;
		this->decalEntries[decal].emissiveTextureName = emissiveTextureName;
	}

	void DecalsModule::setIgnoreAlpha(Ogre::Decal* decal, bool ignoreAlpha)
	{
		if (nullptr == decal)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::setIgnoreAlpha", _2(decal, ignoreAlpha),
		{
			decal->setIgnoreAlphaDiffuse(ignoreAlpha);
		});
	}

	void DecalsModule::setMetalness(Ogre::Decal* decal, Ogre::Real metalness)
	{
		if (nullptr == decal)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::setMetalness", _2(decal, metalness),
		{
			decal->setMetalness(metalness);
		});
	}

	void DecalsModule::setRoughness(Ogre::Decal* decal, Ogre::Real roughness)
	{
		if (nullptr == decal)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::setRoughness", _2(decal, roughness),
		{
			decal->setRoughness(roughness);
		});
	}

	void DecalsModule::setRectSize(Ogre::Decal* decal, const Ogre::Vector3& rectSizeAndDepth)
	{
		if (nullptr == decal)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::setRectSize", _2(decal, rectSizeAndDepth),
		{
			decal->setRectSize(Ogre::Vector2(rectSizeAndDepth.x, rectSizeAndDepth.y), rectSizeAndDepth.z);
		});
	}

	void DecalsModule::destroyContent(void)
	{
		if (nullptr != this->sceneManager && false == this->decals.empty())
		{
			auto* sm = this->sceneManager;
			std::vector<Ogre::Decal*> decalsToDestroy;
			decalsToDestroy.reserve(this->decals.size());
			for (auto* d : this->decals)
			{
				decalsToDestroy.push_back(d);
			}

			this->decals.clear();
			this->decalEntries.clear();

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("DecalsModule::destroyContent", _2(sm, decalsToDestroy),
			{
				for (auto* d : decalsToDestroy)
				{
					sm->destroyDecal(d);
				}
			});
		}

		this->diffuseTextures.clear();
		this->normalTextures.clear();
		this->emissiveTextures.clear();

		this->disabledDiffuse = nullptr;
		this->disabledNormal = nullptr;
		this->disabledEmissive = nullptr;

		this->decalPoolsReady = false;
		this->sceneManager = nullptr;
	}

}