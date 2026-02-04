#ifndef DECALS_MODULE_H
#define DECALS_MODULE_H

#include "defines.h"

#include <unordered_map>
#include <unordered_set>

#include "OgrePrerequisites.h"
#include "OgreString.h"
#include "OgreVector3.h"

namespace Ogre
{
	class SceneManager;
	class SceneNode;
	class Decal;
	class TextureGpu;
}

namespace NOWA
{
	/**
	 * @brief Central decal management (texture pools + creation/destruction).
	 * @note  Mirrors the Ogre-Next decals sample behaviour (pool reservation + disabled slice 0),
	 *        but is designed to be used by NOWA-Engine components and NOWA-Design editor.
	 */
	class EXPORTED DecalsModule
	{
	public:
		friend class AppState; // Only AppState may create this class
	public:
		struct DecalTexturePoolSettings
		{
			DecalTexturePoolSettings()
				: poolId(1u),
				resolution(512u),
				numMipmaps(8u),
				maxTexturesInArray(128u)
			{

			}

			/// Unique pool id (must match the one used for all decal textures)
			Ogre::uint32 poolId;

			/// Resolution for diffuse/normal/emissive textures (must match all decal textures)
			Ogre::uint32 resolution;

			/// Mipmap count for the reserved pool
			Ogre::uint8 numMipmaps;

			/// Max number of textures in the single array (slice count). Increase carefully (VRAM).
			Ogre::uint16 maxTexturesInArray;
		};

	public:
		void init(Ogre::SceneManager* sceneManager);

		/**
		 * @brief Destroys all decals tracked by the module and releases internal references.
		 * @note  Does not attempt to destroy pooled textures (Ogre pools are intended to live for the session).
		 */
		void destroyContent(void);

		/**
		 * @brief Creates an Ogre::Decal and attaches it to the given scene node.
		 * @return The created decal or nullptr on failure.
		 */
		Ogre::Decal* createDecal(Ogre::SceneManager* sceneManager, Ogre::SceneNode* parentNode);

		/**
		 * @brief Destroys a decal created by this module (safe for render-threaded usage).
		 */
		void destroyDecal(Ogre::SceneManager* sceneManager, Ogre::Decal*& decal);

		/**
		 * @brief Assigns decal textures using the reserved pools (diffuse/normal/emissive).
		 * @note  Empty strings will use the module's disabled textures. If emissive is empty, emissive = diffuse (Hlms optimisation).
		 */
		void setDecalTextures(Ogre::SceneManager* sceneManager, Ogre::Decal* decal,
			const Ogre::String& diffuseTextureName,
			const Ogre::String& normalTextureName,
			const Ogre::String& emissiveTextureName);

		void setIgnoreAlpha(Ogre::Decal* decal, bool ignoreAlpha);
		void setMetalness(Ogre::Decal* decal, Ogre::Real metalness);
		void setRoughness(Ogre::Decal* decal, Ogre::Real roughness);

		/**
		 * @param rectSizeAndDepth x,y = rect size, z = depth (projection distance)
		 */
		void setRectSize(Ogre::Decal* decal, const Ogre::Vector3& rectSizeAndDepth);

		void setPoolSettings(const DecalTexturePoolSettings& settings);

		const DecalTexturePoolSettings& getPoolSettings(void) const;

		Ogre::TextureGpu* getDisabledDiffuseTexture(void) const;
		Ogre::TextureGpu* getDisabledNormalTexture(void) const;
		Ogre::TextureGpu* getDisabledEmissiveTexture(void) const;

	private:
		DecalsModule(const Ogre::String& appStateName);
		~DecalsModule();

	private:
		struct TextureEntry
		{
			TextureEntry()
				: texture(nullptr),
				refCount(0u)
			{

			}

			Ogre::TextureGpu* texture;
			Ogre::uint32 refCount;
		};

		struct DecalEntry
		{
			DecalEntry()
			{
			}

			Ogre::String diffuseTextureName;
			Ogre::String normalTextureName;
			Ogre::String emissiveTextureName;
		};

	private:
		Ogre::TextureGpu* getOrCreateTexture(Ogre::SceneManager* sceneManager,
			const Ogre::String& textureName,
			Ogre::uint32 poolId,
			bool isNormalMap,
			bool isEmissive);

		void ensureDecalPools(Ogre::SceneManager* sceneManager);

	private:
		Ogre::String appStateName;
		Ogre::SceneManager* sceneManager;
		bool decalPoolsReady;
		DecalTexturePoolSettings poolSettings;

		Ogre::TextureGpu* disabledDiffuse;
		Ogre::TextureGpu* disabledNormal;
		Ogre::TextureGpu* disabledEmissive;

		std::unordered_map<Ogre::String, TextureEntry> diffuseTextures;
		std::unordered_map<Ogre::String, TextureEntry> normalTextures;
		std::unordered_map<Ogre::String, TextureEntry> emissiveTextures;

		std::unordered_set<Ogre::Decal*> decals;
		std::unordered_map<Ogre::Decal*, DecalEntry> decalEntries;
	};

}; //namespace end

#endif
