#include "NOWAPrecompiled.h"
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

	void HlmsBaseListenerContainer::propertiesMergedPreGenerationStep(Ogre::Hlms* hlms, const Ogre::HlmsCache& passCache, const Ogre::HlmsPropertyVec& renderableCacheProperties,
										   const Ogre::PiecesMap renderableCachePieces[Ogre::NumShaderTypes], const Ogre::HlmsPropertyVec& properties, const Ogre::QueuedRenderable& queuedRenderable, size_t tid)
	{
		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			(*it)->propertiesMergedPreGenerationStep(hlms, passCache, renderableCacheProperties, renderableCachePieces, properties, queuedRenderable, tid);
		}
	}

	Ogre::uint16 HlmsBaseListenerContainer::getNumExtraPassTextures(const Ogre::HlmsPropertyVec& properties, bool casterPass) const
	{
		Ogre::uint32 size = 0;

		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			size += (*it)->getNumExtraPassTextures(properties, casterPass);
		}
		return size;
	}

	void HlmsBaseListenerContainer::setupRootLayout(Ogre::RootLayout& rootLayout, const Ogre::HlmsPropertyVec& properties, size_t tid) const
	{
		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			(*it)->setupRootLayout(rootLayout, properties, tid);
		}
	}

	void HlmsBaseListenerContainer::shaderCacheEntryCreated(const Ogre::String& shaderProfile, const Ogre::HlmsCache* hlmsCacheEntry, const Ogre::HlmsCache& passCache, const Ogre::HlmsPropertyVec& properties,
								 const Ogre::QueuedRenderable& queuedRenderable, size_t tid)
	{
		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			(*it)->shaderCacheEntryCreated(shaderProfile, hlmsCacheEntry, passCache, properties, queuedRenderable, tid);
		}
	}

	void HlmsBaseListenerContainer::preparePassHash(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, Ogre::Hlms* hlms)
	{
		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			(*it)->preparePassHash(shadowNode, casterPass, dualParaboloid, sceneManager, hlms);
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

	void HlmsBaseListenerContainer::hlmsTypeChanged(bool casterPass, Ogre::CommandBuffer* commandBuffer, const Ogre::HlmsDatablock* datablock, size_t texUnit)
	{
		for (auto& it = this->concreteListeners.cbegin(); it != this->concreteListeners.cend(); ++it)
		{
			(*it)->hlmsTypeChanged(casterPass, commandBuffer, datablock, texUnit);
		}
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