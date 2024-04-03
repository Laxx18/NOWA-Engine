#ifndef HLMS_BASE_LISTENER_CONTAINER_H
#define HLMS_BASE_LISTENER_CONTAINER_H

#define NOMINMAX

#include "defines.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsListener.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"

namespace NOWA
{
	class EXPORTED HlmsBaseListenerContainer : public Ogre::HlmsListener
	{
	public:
		HlmsBaseListenerContainer();
		
		virtual ~HlmsBaseListenerContainer();

		virtual void propertiesMergedPreGenerationStep(Ogre::Hlms* hlms, const Ogre::HlmsCache& passCache, const Ogre::HlmsPropertyVec& renderableCacheProperties,
			const Ogre::PiecesMap renderableCachePieces[Ogre::NumShaderTypes], const Ogre::HlmsPropertyVec& properties, const Ogre::QueuedRenderable& queuedRenderable, size_t tid);

		virtual Ogre::uint16 getNumExtraPassTextures(const Ogre::HlmsPropertyVec& properties, bool casterPass) const;

		virtual void setupRootLayout(Ogre::RootLayout& rootLayout, const Ogre::HlmsPropertyVec& properties, size_t tid) const;

		virtual void shaderCacheEntryCreated(const Ogre::String& shaderProfile, const Ogre::HlmsCache* hlmsCacheEntry, const Ogre::HlmsCache& passCache, const Ogre::HlmsPropertyVec& properties,
											 const Ogre::QueuedRenderable& queuedRenderable, size_t tid);

		virtual void preparePassHash(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, Ogre::Hlms* hlms);

		virtual Ogre::uint32 getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const;

		virtual float* preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr);

		virtual float* prepareConcretePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float*& passBufferPtr);

		virtual void hlmsTypeChanged(bool casterPass, Ogre::CommandBuffer* commandBuffer, const Ogre::HlmsDatablock* datablock, size_t texUnit);

		void addConcreteListener(HlmsBaseListenerContainer* listener);

		void removeConcreteListener(HlmsBaseListenerContainer* listener);
	private:
		std::vector<HlmsBaseListenerContainer*> concreteListeners;
	};

}; // namespace end

#endif