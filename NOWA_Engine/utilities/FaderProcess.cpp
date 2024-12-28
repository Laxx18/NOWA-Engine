#include "NOWAPrecompiled.h"
#include "FaderProcess.h"
#include "OgreOverlayManager.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsManager.h"

namespace NOWA
{
	FaderProcess::FaderProcess(FadeOperation fadeOperation, Ogre::Real duration, Interpolator::EaseFunctions selectedEaseFunction, Ogre::Real continueAlpha, Ogre::Real continueDuration, Ogre::Real speedMultiplier)
		: eFadeOperation(fadeOperation),
		currentAlpha(0.0f),
		currentDuration(0.0f),
		totalDuration(0.0f),
		stallDuration(1.0f),
		selectedEaseFunction(selectedEaseFunction),
		continueAlpha(continueAlpha),
		speedMultiplier(speedMultiplier),
		datablock(nullptr),
		overlay(nullptr),
		calledFirstTime(true)
	{
		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		Ogre::HlmsUnlit* hlmsUnlit = dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

		// this->datablock = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock("FadeEffect", "FadeEffect", Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));
		this->datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlmsManager->getDatablock("Materials/OverlayMaterial"));
		if (nullptr == datablock)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Material name 'Materials/OverlayMaterial' cannot be found for 'FaderProcess'.", "FaderProcess::FaderProcess");
		}

		this->datablock->setUseColour(true);

		// http://www.ogre3d.org/forums/viewtopic.php?f=25&t=82797 blendblock, wie macht man unlit transparent

		// Get the _overlay
		this->overlay = Ogre::v1::OverlayManager::getSingleton().getByName("Overlays/FadeInOut");
		this->overlay->setRenderQueueGroup(RENDER_QUEUE_MAX);

		if (this->eFadeOperation == FadeOperation::FADE_IN)
		{
			if (duration < 0.0f)
			{
				duration = -duration;
			}
			if (duration < 0.000001f)
			{
				duration = 1.0f;
			}

			if (0.0f == continueAlpha)
			{
				this->currentAlpha = 1.0f;
			}
			else
			{
				this->currentAlpha = continueAlpha;
			}

			if (0.0f == continueDuration)
			{
				this->currentDuration = duration;
			}
			else
			{
				this->currentDuration = continueDuration;
			}

			this->totalDuration = duration;
			this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, this->currentAlpha));
			this->overlay->show();
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-->Fade in Overlay show");
		}
		else if (this->eFadeOperation == FadeOperation::FADE_OUT)
		{
			if (duration < 0.0f)
			{
				duration = -duration;
			}
			if (duration < 0.000001f)
			{
				duration = 1.0f;
			}

			if (0.0f == continueAlpha)
			{
				this->currentAlpha = 0.0f;
			}
			else
			{
				this->currentAlpha = continueAlpha;
			}

			if (0.0f == continueDuration)
			{
				this->currentDuration = 0.0f;
			}
			else
			{
				this->currentDuration = continueDuration;
			}

			this->totalDuration = duration;
			this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, this->currentAlpha));
			this->overlay->show();
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-->Fade out Overlay show");
		}

		if (0.0f == this->currentDuration)
		{
			this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, 0.0f));
		}

		Ogre::Root::getSingletonPtr()->renderOneFrame();
	}

	FaderProcess::~FaderProcess()
	{

	}

	void FaderProcess::onUpdate(Ogre::Real dt)
	{
		if (this->eFadeOperation != FadeOperation::FADE_NONE && this->datablock)
		{
			// If fading in, decrease the _alpha until it reaches 0.0
			if (this->eFadeOperation == FadeOperation::FADE_IN)
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Fade in: " + Ogre::StringConverter::toString(this->currentDuration) + " alpha: " + Ogre::StringConverter::toString(this->currentAlpha));
				
				this->stallDuration -= dt * this->speedMultiplier;
				if (this->stallDuration <= 0.0f)
				{
					this->currentDuration -= dt * this->speedMultiplier;
					this->currentAlpha = this->currentDuration / this->totalDuration;

					// Call a bit, before process will be destroyed
					if (this->currentAlpha < 0.0f)
					{
						this->currentAlpha = 0.0f;
						this->overlay->hide();
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-->Fade in Overlay hide");
						this->eFadeOperation = FadeOperation::FADE_NONE;
						// Finish the process
						this->succeed();
						return;
					}

					Ogre::Real t = this->currentDuration / this->totalDuration;
					Ogre::Real resultValue = Interpolator::getInstance()->applyEaseFunction(0.0f, 1.0f, this->selectedEaseFunction, t);
					this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, resultValue));
				}
			}
			// If fading out, increase the _alpha until it reaches 1.0
			else if (this->eFadeOperation == FadeOperation::FADE_OUT)
			{
				this->stallDuration -= dt * this->speedMultiplier;
				if (this->stallDuration <= 0.0f)
				{
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Fade out: " + Ogre::StringConverter::toString(this->currentDuration) + " alpha: " + Ogre::StringConverter::toString(this->currentAlpha));
					this->currentDuration += dt;
					this->currentAlpha = this->currentDuration / this->totalDuration;

					if (this->currentAlpha > 1.0f)
					{
						this->currentAlpha = 1.0f;
						this->eFadeOperation = FadeOperation::FADE_NONE;
						this->succeed();
						return;
					}

					Ogre::Real t = this->currentDuration / this->totalDuration;
					Ogre::Real resultValue = Interpolator::getInstance()->applyEaseFunction(0.0f, 1.0f, this->selectedEaseFunction, t);
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Fade out: " + Ogre::StringConverter::toString(resultValue) + " alpha: " + Ogre::StringConverter::toString(this->currentAlpha));
					this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, resultValue));
				}
			}
		}
	}

	void FaderProcess::finished(void)
	{
		// this->overlay->hide();
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-->Finished Overlay hide");
	}

	void FaderProcess::onSuccess(void)
	{
		this->finished();
	}

	Ogre::Real FaderProcess::getCurrentAlpha(void) const
	{
		return this->currentAlpha;
	}

	Ogre::Real FaderProcess::getCurrentDuration(void) const
	{
		return this->currentDuration > 1.0f ? this->currentDuration : 0.0f;
	}

}; // namespace end