#ifndef SIMPLE_SOUND_COMPONENT_H
#define SIMPLE_SOUND_COMPONENT_H

#include "GameObjectComponent.h"
#include <OgreAL.h>

namespace NOWA
{

	/**
	 * @class 	SimpleSoundComponent
	 * @brief 	This component can be used to play and control a sound or music.
	 *			Info: Several components can be attached to the owner game object. A sound will be played when the component gets activated and @connect called and stops when deactivated or @disconnect called.
	 *			Example: Play background music or play sound when player hit the wall.
	 *			Requirements: None
	 */
	class EXPORTED SimpleSoundComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<SimpleSoundComponent> SimpleSoundCompPtr;
	public:
		SimpleSoundComponent();

		virtual ~SimpleSoundComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::connect
		 */
		virtual bool connect(void) override;

		/**
		 * @see		GameObjectComponent::disconnect
		 */
		virtual bool disconnect(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("SimpleSoundComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "SimpleSoundComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: With this component a dolby surround is created and calibrated. It can be used for music, sound and effects like spectrum analysis, doppler effect etc.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @brief Activates the sound.
		 * @param[in] activated If set to true, the will be played, else the sound stops
		 */
		virtual void setActivated(bool activated);

		/**
		* @brief Gets whether the component behaviour is activated or not.
		* @return activated True, if the components behaviour is activated, else false
		*/
		virtual bool isActivated(void) const override;
		
		/**
		 * @brief Sets the sound name to be played.
		 * @param[in] soundName The sound name to set without path. It will be searched automatically in the Audio Resource group.
		 */
		void setSoundName(const Ogre::String& soundName);

		/**
		 * @brief Gets currently used sound name.
		 * @return soundName The sound name to get
		 */
		Ogre::String getSoundName(void) const;

		/**
		 * @brief Sets the sound volume.
		 * @param[in] volume The sound volume to set. 0 means extremely quiet and 100 is maximum.
		 */
		void setVolume(Ogre::Real volume);

		/**
		 * @brief Gets sound volume value
		 * @return volume The sound volume to get
		 */
		Ogre::Real getVolume(void) const;

		/**
		 * @brief Sets whether this sound should be looped.
		 * @param[in] loop If set to true, the sound will be looped
		 */
		void setLoop(bool loop);

		/**
		 * @brief Gets whether is sound play is looped
		 * @return loop true, if the sound is looped, else false
		 */
		bool isLooped(void) const;
		
		/**
		 * @brief Sets whether this sound should be streamed instead of fully loaded to buffer.
		 * @param[in]	stream			If set to true. The sound gets streamed during runtime. If set to false, the music gets loaded into a buffer offline.
		 * @note		Buffering the music file offline, is more stable for big files. Streaming is useful for small files and it is more performant. But using streaming
		 *				for bigger files can cause interrupting the music playback and stops it.
		 */
		void setStream(bool stream);

		/**
		 * @brief Gets whether is sound is streamed instead of fully loaded to buffer.
		 * @return stream true, if the sound is streamed, else false
		 */
		bool isStreamed(void) const;

		/**
		 * @brief Sets whether this should should be played relative to the listener
		 * @param[in] relativeToListener If set to true, the sound will be player relative to the listener
		 * @note	 When set to true, the sound may come out of another dolby surround box depending where the camera is located creating nice and maybe creapy sound effects.
		 *			 When set to false, it does not matter where the camera is, the sound will always have the same values.
		 */
		void setRelativeToListener(bool relativeToListener);

		/**
		 * @brief Gets whether is sound is player relative to the listener.
		 * @return relativeToListener true, if the sound is played relative to listener, else false
		 */
		bool isRelativeToListener(void) const;

		/**
		 * @brief Sets whether to play the sound looped as long as game object is in motion.
		 * @param[in]	playWhenInMotion		If set to true, the sound will start playing as soon as game object is in motion and will stop, when game object is no more in motion.
		 * @note		This override the activate and loop property
		 */
		void setPlayWhenInMotion(bool playWhenInMotion);

		/**
		 * @brief Gets whether to play the sound looped as long as game object is in motion.
		 * @return true, if the sound is played when in motion
		 */
		bool getPlayWhenInMotion(void) const;

		/**
		 * @brief Gets the sound pointer for direct manipulation.
		 * @return sound The sound pointer to get
		 */
		OgreAL::Sound* getSound(void) const;

		/**
		 * @brief	Sets whether to enable spectrum analysis.
		 * @param[in] enable					Whether to activate spectrum analysis or not
		 * @param[in] processingSize			Sets the spectrum processing size. Note: addSoundSpectrumHandler must be used and stream must be set to true and spectrumProcessingSize must always be higher as mSpectrumNumberOfBands and divisible by 2. Default value is 1024. It must not go below 1024
		 * @param[in] numberOfBands				Sets the spectrum number of bands. Note: addSoundSpectrumHandler must be used and stream must be set to true. Default value is 20
		 * @param[in] windowType				Sets the math window type for more harmonic visualization: rectangle, blackman harris, blackman, hanning, hamming, tukey
		 * @param[in] spectrumPreparationType	Sets the spectrum preparation type: raw (for own visualization), linear, logarithmic
		 * @param[in] smoothFactor				Sets the spectrum motion smooth factor (default 0 disabled, max 1)
		 */
		void enableSpectrumAnalysis(bool enable, int processingSize, int numberOfBands, OgreAL::MathWindows::WindowType windowType, 
			OgreAL::AudioProcessor::SpectrumPreparationType spectrumPreparationType, Ogre::Real smoothFactor);

		/**
		 * @brief Sets the lua function name, to react on spectrum data when the audio is playing.
		 * @param[in]	onSpectrumAnalysisFunctionName		The spectrum analysis function name to set 
		 */
		void setOnSpectrumAnalysisFunctionName(const Ogre::String& onSpectrumAnalysisFunctionName);

		/**
		 * @brief Gets the lua function name
		 * @return lua function name to get
		 */
		Ogre::String getOnSpectrumAnalysisFunctionName(void) const;

		/**
		 * @brief Gets the current spectrum time position in seconds
		 * @return currentSpectrumTimePos the current spectrum time position in seconds to get
		 */
		Ogre::Real getCurrentSpectrumTimePosSec(void) const;

		/**
		 * @brief Gets the actual spectrum size, that comes out from the input @spectrumNumberOfBands.
		 * @note  This parameter must be used as border for spectrum analysis!
		 * @return actualSpectrumSize the actually processed spectrum size
		 */
		int getActualSpectrumSize(void) const;

		/**
		 * @brief Gets the frequency (sample rate) of the sound (often 44100, or 48000)
		 * @return frequency the frequency to get
		 */
		int getFrequency(void) const;

		/**
		 * @brief Gets whether in the audio a given spectrum area has been detected like lower bass or midrange
		 * @return hasBeat true if a beat has been detected, else false
		 */
		bool isSpectrumArea(OgreAL::AudioProcessor::SpectrumArea spectrumArea) const;

		/**
		 * @brief Returns the current intensity (energy level) of a specific audio spectrum area.
		 *
		 * 	This function provides the same underlying value that the beat detector uses for
		 * 	deciding whether a band like KICK_DRUM, SNARE_DRUM, HI_HAT, etc. is active.
		 * 	It represents the accumulated magnitude of FFT frequencies inside the band’s
		 * 	defined frequency range (see beatDetectorBandLimits inside AudioProcessor).
		 * 
		 * 	This value is useful when you want to react not only to the binary state
		 * 	(beat or no beat), but proportionally to the band’s real amplitude
		 * 	(e.g. driving animations, physics forces, visual effects, etc.).
		 *
		 * 	@note The returned value is unbounded but typically small (0.0–0.5 range),
		 * 		  depending on audio loudness. It is already clamped to >= 0.0.
		 * 		  To check if a band triggered a beat, use isSpectrumArea() instead.
		 *
		 * 	@param area The spectrum band (e.g., SpectrumArea::KICK_DRUM) to query.
		 *
		 * 	@return The current energy/intensity of the requested spectrum area.
		 * 			Returns 0.0 if no analysis is active or the band index is invalid.
		 */
		Ogre::Real getSpectrumAreaIntensity(OgreAL::AudioProcessor::SpectrumArea area) const;
		
		// These Methods are just for lua and are special, so there are not stored for an editor
		
		void setFadeInOutTime(const Ogre::Vector2& fadeInOutTime);

		void setInnerOuterConeAngle(const Ogre::Vector2& innerOuterConeAngle);

		void setOuterConeGain(Ogre::Real outerConeGain);

		Ogre::Real getOuterConeGain(void) const;

		void setPitch(Ogre::Real pitch);

		Ogre::Real getPitch(void) const;

		void setPriority(int priority);

		int getPriority(void) const;

		void setDistanceValues(const Ogre::Vector3& distanceValues);

		// Ogre::Vector3 getDistanceValues(void) const;

		void setSecondOffset(Ogre::Real secondOffset);

		Ogre::Real getSecondOffset(void) const;
		
		void setVelocity(const Ogre::Vector3& velocity);
		
		Ogre::Vector3 getVelocity(void) const;

		void setDirection(const Ogre::Vector3& direction);
		
		Ogre::Vector3 getDirection(void) const;
	private:
		void setupSound(void);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrSoundName(void) { return "Sound Name"; }
		static const Ogre::String AttrVolume(void) { return "Volume"; }
		static const Ogre::String AttrLoop(void) { return "Loop"; }
		static const Ogre::String AttrStream(void) { return "Stream"; }
		static const Ogre::String AttrRelativeToListener(void) { return "Relative To Listener"; }
		static const Ogre::String AttrPlayWhenInMotion(void) { return "Play when in Motion"; }
		static const Ogre::String AttrSpectrumAnalysisFunctionName(void) { return "Spectrum Analysis Function Name"; }
	private:
		bool createSound(void);
		void destroySound(void);
		void soundSpectrumFuncPtr(OgreAL::Sound* sound);
		std::vector<Ogre::ManualObject*> lines;
		Ogre::SceneNode* lineNode;
	private:
		Variant* activated;
		Variant* soundName;
		Variant* volume;
		Variant* loop;
		Variant* stream;
		Variant* relativeToListener;
		Variant* playWhenInMotion;
		Variant* spectrumSamplingRate;
		Variant* onSpectrumAnalysisFunctionName;
		OgreAL::Sound* sound;
		OgreAL::SoundManager* soundManager;
		Ogre::String oldSoundName;
		Ogre::Vector3 oldPosition;
		Ogre::Quaternion oldOrientation;
	};

}; //namespace end

#endif