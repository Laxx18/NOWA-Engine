#ifndef OGRE_AL_MODULE_H
#define OGRE_AL_MODULE_H

// Sound
#include "OgreAL.h"
#include "defines.h"
#include "modules/GraphicsModule.h"

namespace NOWA
{

	class EXPORTED OgreALModule
	{
	public:
		using SoundCreationCallback = std::function<void(OgreAL::Sound*)>;
	public:

		/**
		 * @brief		Creates OgreAL and the SoundManager
		 */
		void init(Ogre::SceneManager* sceneManager);

		/**
		 * @brief		Setups the volumes for sound and music
		 * @param[in]	soundVolume		The sound volume to set
		 * @param[in]	musicVolume		The music volume to set
		 */
		void setupVolumes(int soundVolume, int musicVolume);

		/**
		 * @brief		Deletes an OgreAL sound safely
		 * @param[in]	sound		The sound to delete
		 */
		void deleteSound(Ogre::SceneManager* sceneManager, OgreAL::Sound*& sound);

		/**
		 * @brief		Creates a sound file and delivers the sound for playback.
		 * @param[in]	name			The name of the sound file
		 * @param[in]	resourceName	The resource name where the file can be found
		 * @param[in]	loop			If the file playback is at its end, loop it again
		 * @param[in]	stream			If set to true. The sound gets streamed during runtime. If set to false, the music gets loaded into a buffer offline.
		 * @note		Buffering the music file offline, is more stable for big files. Streaming is useful for small files and it is more performant. But using streaming
		 *				for bigger files can cause interrupting the music playback and stops it.
		 *				Also note, the only difference between create sound and create music is the need for different sound volume und music volume specified in a menu.
		 */
		// void createSound(Ogre::SceneManager* sceneManager, const Ogre::String& name, const Ogre::String& resourceName, bool loop, bool stream, SoundCreationCallback callback);

		OgreAL::Sound* createSound(Ogre::SceneManager* sceneManager, const Ogre::String& name, const Ogre::String& resourceName, bool loop = false, bool stream = false);

		/**
		 * Sets the Doppler factor.
		 * The Doppler factor is a simple scaling factor of the source and listener
		 * velocities to exaggerate or deemphasize the Doppler (pitch) shift resulting
		 * from the Doppler Effect Calculations.
		 * @note Negative values will be ignored.
		 */
		void setSoundDopplerEffect(Ogre::Real value);

		/** 
		 * Sets the speed of sound used in the Doppler calculations.
		 * This sets the propagation speed used in the Doppler calculations.
		 * The default is 343.3 m/s (Speed of sound through air), but the
		 * units can be assigned to anything that you want as long as the
		 * velocities of the Sounds and Listener are expressed in the same units.
		 * @note Negative values will be ignored.
		 */
		void setSoundSpeed(Ogre::Real speed);

		/**
		 * @brief Enable or disable sound culling.
		 *
		 * This function enables or disables sound culling. Sound culling is the
		 * cutting of gain after a certain distance. After the specified distance
		 * between the listener and the given sound source, a sound's gain will be 
		 * stored and set to zero. This will render the sound as still playing in 
		 * the background, but inaudible. Given the listener return so as to be in
		 * range of the sound (within the cull distance), the stored gain of the 
		 * sound will be restored.
		 * 
		 * @param[in] distance The distance between the listener and and sound within
		 * the system after which to cull sounds. Set this to any positive value
		 * to enable culling. Set it to zero or any negative value to disable 
		 * the feature.
		*/
		void setSoundCullDistance(Ogre::SceneManager* sceneManager, Ogre::Real distance);

	   /**
		* @brief		Gets a specific sound resource from name
		* @param[in]	soundName The sound name
		* @note	If there is no such sound name, NULL will be returned.
		* @usage		
		*			OgreAL::Sound* sound = OgreALModule::getSound(sceneManager, soundName);
		*			if (sound) {
		*				// Instead of directly manipulating the sound object here, queue it if necessary.
		*				GraphicsModule::RenderCommand renderCommand = [sound]() {
		*					// Modify sound properties safely on the render thread
		*					sound->setVolume(0.5f);  // Example modification
		*				};
		*				GraphicsModule::getInstance()->enqueue(renderCommand);
		*			}
		* return		pSound the corresponding sound
		*/
		OgreAL::Sound* getSound(Ogre::SceneManager* sceneManager, const Ogre::String& soundName);

		OgreAL::SoundManager* getSoundManager(void) const;

		int getSoundVolume(void) const;

		int getMusicVolume(void) const;

		void destroySounds(Ogre::SceneManager* sceneManager);

		void destroyContent(void);

		/**
		 * @brief		Sets whether the sound manager continues with the current scene manager.
		 * @param[in]	bContinue Whether to continue or not
		 * @note		This can be used when sounds are used in an AppState, but another AppState is pushed on the top, yet still the sounds should continue playing from the prior AppState.
		 *				E.g. start music in MenuState and continue playing when pushing LoadMenuState and going back again to MenuState. 
		 */
		void setContinue(bool bContinue);

		/**
		 * @brief		Gets whether the sound manager continues with the current scene manager.
		 * @return		bContinue	True if sound manager is continued, else false
		 */
		bool getIsContinued(void) const;
	public:
		static OgreALModule* getInstance();
	private:
		OgreALModule();
		~OgreALModule();
	private:
		OgreAL::SoundManager* soundManager;
		Ogre::SceneManager* sceneManager;
		int soundVolume;
		int musicVolume;
		bool bContinue;
	};

}; //namespace end

#endif