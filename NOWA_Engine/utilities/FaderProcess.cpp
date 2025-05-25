#include "NOWAPrecompiled.h"
#include "FaderProcess.h"
#include "OgreOverlayManager.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsManager.h"
#include "modules/GraphicsModule.h"

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
		ENQUEUE_RENDER_COMMAND_MULTI("FaderProcess::FaderProcess", _6(fadeOperation, &duration, selectedEaseFunction, continueAlpha, continueDuration, speedMultiplier),
		{
			Ogre::HlmsManager * hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
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
		});
	}

	FaderProcess::~FaderProcess()
	{

	}

	void FaderProcess::onUpdate(Ogre::Real dt)
	{
		if (this->eFadeOperation == FadeOperation::FADE_NONE || !this->datablock)
			return;

		this->stallDuration -= dt * this->speedMultiplier;
		if (this->stallDuration > 0.0f)
			return;

		// Compute progress and easing
		this->currentDuration += (this->eFadeOperation == FadeOperation::FADE_IN ? -1.0f : 1.0f) * dt * this->speedMultiplier;
		Ogre::Real t = Ogre::Math::Clamp(this->currentDuration / this->totalDuration, 0.0f, 1.0f);
		this->currentAlpha = (this->eFadeOperation == FadeOperation::FADE_IN) ? 1.0f - t : t;

		Ogre::Real resultValue = Interpolator::getInstance()->applyEaseFunction(0.0f, 1.0f, this->selectedEaseFunction, t);

		// Copy relevant values for render thread (safe)
		auto datablock = this->datablock;
		FadeOperation fadeOp = this->eFadeOperation;
		auto overlay = this->overlay;

		// Apply color
		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("FaderProcess::applyAlpha", _2(resultValue, datablock),
		{
			if (datablock)
			{
				datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, resultValue));
			}
		});

		// Finish if done
		if ((fadeOp == FadeOperation::FADE_IN && this->currentAlpha <= 0.0f) ||
			(fadeOp == FadeOperation::FADE_OUT && this->currentAlpha >= 1.0f))
		{
			this->currentAlpha = (fadeOp == FadeOperation::FADE_IN) ? 0.0f : 1.0f;
			this->eFadeOperation = FadeOperation::FADE_NONE;

			if (fadeOp == FadeOperation::FADE_IN && overlay)
			{
				ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("FaderProcess::hideOverlay", _1(overlay),
				{
					overlay->hide();
				});
			}

			this->succeed();
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