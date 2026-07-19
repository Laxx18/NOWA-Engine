#ifndef FADER_PROCESS_H
#define FADER_PROCESS_H

#include "defines.h"
#include "main/Process.h"
#include "main/ProcessManager.h"
#include "boost/weak_ptr.hpp"
#include "Interpolator.h"

#include <atomic>

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

		// Immediately shows (or hides) the shared fade widget at full opacity,
		// with no animation. Use this to guarantee the screen is already
		// black BEFORE a scene loads / becomes visible — otherwise there can
		// be a frame (or more) of the unfaded scene visible between when it
		// starts rendering and when a FaderProcess(FADE_IN, ...) actually
		// attaches and takes over. Typical use:
		//   FaderProcess::showBlackScreenImmediate();
		//   // ... load / build the scene ...
		//   ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FADE_IN, ...)));
		static void showBlackScreenImmediate(void);
		static void hideBlackScreenImmediate(void);
	protected:
		virtual void onSuccess(void);
	protected:
		Ogre::Real totalDuration;
		Interpolator::EaseFunctions selectedEaseFunction;
		Ogre::Real speedMultiplier;

		// v2: the actual fade animation now runs render-side as a MyGUI
		// ControllerItem attached to a shared full-screen widget (see
		// FaderProcess.cpp) — mirrors how the old v1 code looked up a single
		// shared, named Ogre::v1::Overlay ("Overlays/FadeInOut") rather than
		// creating one per instance. These are written from the render
		// thread and read from the logic thread (onUpdate / the getters).
		std::atomic<Ogre::Real> atomicCurrentAlpha;
		std::atomic<Ogre::Real> atomicCurrentDuration;
		std::atomic<bool> isFinished;

		// Logic-thread-only guard so succeed() fires exactly once after
		// isFinished flips true (onUpdate may still be ticked once more
		// before ProcessManager removes this process).
		bool succeededAlready;
	};

};  // namespace end

#endif
