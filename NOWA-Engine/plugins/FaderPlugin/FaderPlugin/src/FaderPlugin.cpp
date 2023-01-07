#include "FaderPlugin.h"
#include "OgreOverlayManager.h"
#include "OgreTechnique.h"
#include "OgreTextureUnitState.h"
#include "OgreMaterialManager.h"
#include "OgreRoot.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"

namespace NOWA
{

	FaderPlugin::FaderPlugin()
		: name("FaderPlugin"),
		alpha(0.0f),
		eFadeOperation(FADE_NONE),
		currentDuration(0.0f),
		totalDuration(0.0f),
		datablock(nullptr),
		overlay(nullptr)
	{

	}

	FaderPlugin::~FaderPlugin(void)
	{

	}

	void FaderPlugin::initialise()
	{
		/*Ogre::HlmsMacroblock hlmsMacroblockBase;
		hlmsMacroblockBase.mDepthWrite = false;
		hlmsMacroblockBase.mDepthCheck = true;
		const Ogre::HlmsMacroblock* hlmsMacroblock = Ogre::Root::getSingletonPtr()->getHlmsManager()->getMacroblock(hlmsMacroblockBase);

		Ogre::HlmsBlendblock hlmsBlendblockBase;
		hlmsBlendblockBase.setBlendType(Ogre::SceneBlendType::SBT_TRANSPARENT_ALPHA);
		const Ogre::HlmsBlendblock* hlmsBlendblock = Ogre::Root::getSingletonPtr()->getHlmsManager()->getBlendblock(hlmsBlendblockBase);*/

		/*Ogre::Hlms* hlms = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
		Ogre::HlmsUnlit* hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlms);*/
		// pMaterial->setMacroblock(hlmsMacroblockBase);
		// pMaterial->setBlendblock(*hlmsBlendblock);

		/*Ogre::HlmsDatablock* hlmsDatablock = hlmsUnlit->getDatablock("Materials/OverlayMaterial");
		hlmsDatablock->setMacroblock(hlmsMacroblock);
		hlmsDatablock->setBlendblock(hlmsBlendblock);*/
		// hlmsDatablock = hlmsUnlit->getDatablock("ui/cursor");
		// hlmsDatablock->setMacroblock(hlmsMacroblock);
		// hlmsDatablock->setBlendblock(hlmsBlendblock);
		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		Ogre::HlmsUnlit* hlmsUnlit = dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

		// this->datablock = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock("FadeEffect", "FadeEffect", Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));
		this->datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlmsManager->getDatablock("Materials/OverlayMaterial"));
		// Via code, or scene_blend alpha_blend in material
	/*	Ogre::HlmsBlendblock blendblock;
		blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);
		this->datablock->setBlendblock(blendblock);*/
		this->datablock->setUseColour(true);

		// http://www.ogre3d.org/forums/viewtopic.php?f=25&t=82797 blendblock, wie macht man unlit transparent
		
		// Get the _overlay
		this->overlay = Ogre::v1::OverlayManager::getSingleton().getByName("Overlays/FadeInOut");
		this->overlay->hide();
	}

	void FaderPlugin::startFadeIn(Ogre::Real duration)
	{
		if (duration < 0.0f)
		{
			duration = -duration;
		}
		if (duration < 0.000001f)
		{
			duration = 1.0f;
		}
		this->alpha = 1.0f;
		this->totalDuration = duration;
		this->currentDuration = duration;
		this->eFadeOperation = FADE_IN;
		this->overlay->show();
	}

	void FaderPlugin::startFadeOut(Ogre::Real duration)
	{
		if (duration < 0.0f)
		{
			duration = -duration;
		}
		if (duration < 0.000001f)
		{
			duration = 1.0f;
		}

		this->alpha = 0.0f;
		this->totalDuration = duration;
		this->currentDuration = 0.0f;
		this->eFadeOperation = FADE_OUT;
		this->overlay->show();
	}

	void FaderPlugin::update(Ogre::Real dt)
	{
		if (this->eFadeOperation != FADE_NONE && this->datablock)
		{
			// Set the _alpha value of the overlay
			 // Change the transparency
			// this->datablock->setTransparency(1.0f / this->alpha);
			this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, this->alpha));

			// If fading in, decrease the _alpha until it reaches 0.0
			if (this->eFadeOperation == FADE_IN)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage("Fade in: " + Ogre::StringConverter::toString(this->currentDuration) + " alpha: " + Ogre::StringConverter::toString(this->alpha));
				this->currentDuration -= dt;
				this->alpha = this->currentDuration / this->totalDuration;
				if (this->alpha < 0.0f)
				{
					this->overlay->hide();
					this->eFadeOperation = FADE_NONE;
				}
			}
			// If fading out, increase the _alpha until it reaches 1.0
			else if (this->eFadeOperation == FADE_OUT)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage("Fade out: " + Ogre::StringConverter::toString(this->currentDuration) + " alpha: " + Ogre::StringConverter::toString(this->alpha));
				this->currentDuration += dt;
				this->alpha = this->currentDuration / this->totalDuration;
				if (this->alpha > 1.0f)
				{
					this->eFadeOperation = FADE_NONE;
				}
			}
		}
	}

	const Ogre::String& FaderPlugin::getName() const
	{
		return this->name;
	}

	void FaderPlugin::install()
	{

	}

	void FaderPlugin::shutdown()
	{

	}

	void FaderPlugin::uninstall()
	{

	}

	FaderPlugin* FaderPlugin::getSingletonPtr(void)
	{
		return msSingleton;
	}

	FaderPlugin& FaderPlugin::getSingleton(void)
	{
		assert(msSingleton);
		return (*msSingleton);
	}

	FaderPlugin* pFader;

	extern "C" EXPORTED void dllStartPlugin()
	{
		pFader = new FaderPlugin();
		Ogre::Root::getSingleton().installPlugin(pFader);
	}

	extern "C" EXPORTED void dllStopPlugin()
	{
		Ogre::Root::getSingleton().uninstallPlugin(pFader);
		delete pFader;
		pFader = static_cast<FaderPlugin*>(0);
	}

}; //namespace end