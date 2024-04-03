#ifndef HLMS_WIND_H
#define HLMS_WIND_H

#include "defines.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsListener.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"

namespace NOWA
{
	class EXPORTED HlmsWindListener : public Ogre::HlmsListener
	{
	public:
		HlmsWindListener();
		
		virtual ~HlmsWindListener() = default;

		void setTime(Ogre::Real time);

		void addTime(Ogre::Real time);

		virtual Ogre::uint32 getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const;

		virtual float* preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr);
	private:
		Ogre::Real windStrength;
		Ogre::Real globalTime;
	};

	//////////////////////////////////////////////////////////////////////////////

	class EXPORTED HlmsWind : public Ogre::HlmsPbs
	{
	public:
		HlmsWind(Ogre::Archive* dataFolder, Ogre::ArchiveVec* libraryFolders);

		virtual ~HlmsWind() = default;

		void addTime(float time);
		
		void setup(Ogre::SceneManager* sceneManager);

		void shutdown(Ogre::SceneManager* sceneManager);

		void notifyPropertiesMergedPreGenerationStep(size_t tid);

		Ogre::uint32 fillBuffersForV1(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable,
			bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer);
		
		
		Ogre::uint32 fillBuffersForV2(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable,
			bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer);
		
		Ogre::uint32 fillBuffersFor(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable,
			bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer, bool isV1);
	public:
		static void getDefaultPaths(Ogre::String& outDataFolderPath, Ogre::StringVector& outLibraryFoldersPaths);
	private:
		void calculateHashForPreCreate(Ogre::Renderable* renderable, Ogre::PiecesMap* inOutPieces) override;

		void loadTexturesAndSamplers(Ogre::SceneManager* sceneManager);
	private:
		HlmsWindListener windListener;

		Ogre::HlmsSamplerblock const* noiseSamplerBlock;

		Ogre::TextureGpu* noiseTexture;
	};

}; // namespace end

#endif