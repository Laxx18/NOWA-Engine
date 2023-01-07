#include "NOWAPrecompiled.h"
#include "HlmsFog.h"

namespace NOWA
{
	HlmsFogListener::HlmsFogListener()
	{

	}

	Ogre::uint32 HlmsFogListener::getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const
	{
		Ogre::uint32 size = 0;

		if (!casterPass)
		{
			size = sizeof(float) * 4 * 2; // float4 + float4 in bytes
		}

		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "--> Fog Size: " + Ogre::StringConverter::toString(size));
		return size;
	}

	float* HlmsFogListener::prepareConcretePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float*& passBufferPtr)
	{
		if (!casterPass)
		{
			//Linear Fog parameters
			*passBufferPtr++ = sceneManager->getFogStart();
			*passBufferPtr++ = sceneManager->getFogEnd();
			*passBufferPtr++ = 0.0f;
			*passBufferPtr++ = 0.0f;

			*passBufferPtr++ = sceneManager->getFogColour().r;
			*passBufferPtr++ = sceneManager->getFogColour().g;
			*passBufferPtr++ = sceneManager->getFogColour().b;
			*passBufferPtr++ = 1.0f;
		}
		return passBufferPtr;
	}

}; // namespace end