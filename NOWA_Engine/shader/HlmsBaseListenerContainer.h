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

		virtual Ogre::uint32 getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const;

		virtual float* preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr);

		virtual float* prepareConcretePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float*& passBufferPtr);

		void addConcreteListener(HlmsBaseListenerContainer* listener);

		void removeConcreteListener(HlmsBaseListenerContainer* listener);
	private:
		std::vector<HlmsBaseListenerContainer*> concreteListeners;
	};

}; // namespace end

#endif