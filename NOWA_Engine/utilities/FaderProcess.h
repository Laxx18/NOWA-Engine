#ifndef FADER_PROCESS_H
#define FADER_PROCESS_H

#include "defines.h"
#include "main/Process.h"
#include "main/ProcessManager.h"
#include "boost/weak_ptr.hpp"
#include "OgreOverlay.h"
#include "OgreHlmsUnlitDatablock.h"
#include "Interpolator.h"

namespace NOWA
{
	class EXPORTED FaderProcess : public Process
	{
	public:
		enum class FadeOperation
		{
			FADE_NONE,
			FADE_IN,
			FADE_OUT,
		} eFadeOperation;
	public:
		explicit FaderProcess(FadeOperation fadeOperation, Ogre::Real duration, Interpolator::EaseFunctions selectedEaseFunction = Interpolator::Linear, Ogre::Real continueAlpha = 0.0f, Ogre::Real continueDuration = 0.0f, Ogre::Real speedMultiplier = 1.0f);

		virtual ~FaderProcess();

		virtual void finished(void);

		Ogre::Real getCurrentAlpha(void) const;

		Ogre::Real getCurrentDuration(void) const;

		virtual void onUpdate(Ogre::Real dt) override;
	protected:
		virtual void onSuccess(void);
	protected:
		Ogre::Real currentAlpha;
		Ogre::Real currentDuration;
		Ogre::Real stallDuration;
		Ogre::Real totalDuration;
		Interpolator::EaseFunctions selectedEaseFunction;
		Ogre::Real continueAlpha;
		Ogre::Real speedMultiplier;
		Ogre::HlmsUnlitDatablock* datablock;
		Ogre::v1::Overlay* overlay;
		bool calledFirstTime;
	};

};  // namespace end

#endif