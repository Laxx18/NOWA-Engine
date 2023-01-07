#ifndef FADER_PLUGIN_H
#define FADER_PLUGIN_H

#include "defines.h"
#include "OgrePlugin.h"
#include "OgreOverlay.h"
#include "OgreSingleton.h"
#include "OgreHlmsUnlitDatablock.h"

namespace NOWA
{

	class EXPORTED FaderPlugin : public Ogre::Plugin, Ogre::Singleton<FaderPlugin>
	{
	public:
		FaderPlugin();
		~FaderPlugin();

		virtual void install() override;

		virtual void initialise() override;

		virtual void shutdown() override;

		virtual void uninstall() override;

		void startFadeIn(Ogre::Real duration = 1.0f);

		void startFadeOut(Ogre::Real duration = 1.0f);

		void update(Ogre::Real dt);

		const Ogre::String& getName() const;

		static FaderPlugin& getSingleton(void);

		static FaderPlugin* getSingletonPtr(void);
	private:
		Ogre::String name;
		Ogre::Real alpha;
		Ogre::Real currentDuration;
		Ogre::Real totalDuration;
		Ogre::HlmsUnlitDatablock* datablock;
		Ogre::v1::Overlay* overlay;

		enum FadeOperation
		{
			FADE_NONE,
			FADE_IN,
			FADE_OUT,
		} eFadeOperation;
	};

}; // namespace end

template<> NOWA::FaderPlugin* Ogre::Singleton<NOWA::FaderPlugin>::msSingleton = 0;

#endif

