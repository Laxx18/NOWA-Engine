#ifndef HLMS_FOG_H
#define HLMS_FOG_H

#include "defines.h"
#include "OgreHlmsPbs.h"
#include "HlmsBaseListenerContainer.h"

namespace NOWA
{
	class EXPORTED HlmsFogListener : public HlmsBaseListenerContainer
	{
	public:
		HlmsFogListener();
		
		virtual ~HlmsFogListener() = default;

		virtual Ogre::uint32 getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const;

		virtual float* prepareConcretePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float*& passBufferPtr);
	};

}; // namespace end

#endif