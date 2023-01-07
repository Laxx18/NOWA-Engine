#include "HlmsBaseListenerContainer.h"
#include <algorithm>

namespace NOWA
{
	HlmsBaseListenerContainer::HlmsBaseListenerContainer()
	{

	}

	HlmsBaseListenerContainer::~HlmsBaseListenerContainer()
	{
		while (this->concreteListeners.size() > 0)
		{
			Ogre::HlmsListener* listener = this->concreteListeners.back();
			delete listener;
			this->concreteListeners.pop_back();
		}
	}

	Ogre::uint32 HlmsBaseListenerContainer::getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const
	{
		Ogre::uint32 size = 0;

		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			size += (*it)->getPassBufferSize(shadowNode, casterPass, dualParaboloid, sceneManager);
		}
		return size;
	}

	float* HlmsBaseListenerContainer::preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr)
	{
		return this->prepareConcretePassBuffer(shadowNode, casterPass, dualParaboloid, sceneManager, passBufferPtr);
	}

	float* HlmsBaseListenerContainer::prepareConcretePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float*& passBufferPtr)
	{
		// https://forums.ogre3d.org/viewtopic.php?f=25&t=83081&p=550395#p550395
		// Note: prepareConcretePassBuffer uses passBufferPtr as reference to pointer (in/out), so that the address is always correct, even its dispatched deeper to concrete listeners
		// to prevent the assert and shader crash: assert( (size_t)(passBufferPtr - startupPtr) * 4u == mapSize );
		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			passBufferPtr = static_cast<HlmsBaseListenerContainer*>((*it))->prepareConcretePassBuffer(shadowNode, casterPass, dualParaboloid, sceneManager, passBufferPtr);
		}
		
		return passBufferPtr;
	}

	void HlmsBaseListenerContainer::addConcreteListener(HlmsBaseListenerContainer* listener)
	{
		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			if ((*it) == listener)
			{
				return;
			}
		}
		this->concreteListeners.push_back(listener);
	}

	void HlmsBaseListenerContainer::removeConcreteListener(HlmsBaseListenerContainer* listener)
	{
		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			if ((*it) == listener)
			{
				HlmsBaseListenerContainer* currentListener = (*it);
				delete currentListener;
				this->concreteListeners.erase(it);
				break;
			}
		}
	}

}; // namespace end