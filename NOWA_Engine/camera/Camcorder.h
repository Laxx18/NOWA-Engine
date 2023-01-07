#ifndef CAMCORDER_H
#define CAMCORDER_H

#include "defines.h"
#include "OgreTextAreaOverlayElement.h"

/*
-----------------------------------------------------------------------------
Filename:    Camcorder.h
Description: A subsystem for recording camera points and playing them back
See: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Camcorder&structure=Cookbook
--> AnimationSerializer is required! --> http://www.ogre3d.org/tikiwiki/tiki-index.php?page=AnimationSerializer
-----------------------------------------------------------------------------
*/


namespace NOWA
{

	/** Helper class which provides an interface to the Camcorder */
	class EXPORTED CamcorderHelper
	{
	public:
		CamcorderHelper();
		virtual ~CamcorderHelper();

		void init(Ogre::Viewport* viewport, Ogre::Camera* camera);

		/** Process unbuffered keyboard state.
		*/
		void processUnbufferedKeyboard(OIS::Keyboard* keyboard, Ogre::Real dt);

		/** Update the camera based on a frame.
		@remarks
		Will only update the camera if in playback mode, safe to
		call in all cases
		*/
		void update(Ogre::Real dt);


		enum Mode
		{
			/// Not actively doing anything (although manual keyframes can be taken)
			IDLE,
			/// Recording the motion of the camera according to setKeyFrameFrequency
			RECORDING,
			/// Playing back an animation
			PLAYBACK
		};

		/// Get the current mode
		Mode getMode() const
		{
			return this->mode;
		}
		/// Manually change the mode rather than let the input processing do it
		void setMode(Mode mode);

		bool isPlayingBack() const;
		bool isRecording() const;
		bool isIdle() const;

		/// Enable the class, showing the overlay, processing input and recording / playback if required
		void setEnabled(bool enabled);

		bool getEnabled() const
		{
			return this->enabled;
		}

		/// Set the position interpolation mode
		void setPositionInterpolationMode(Ogre::v1::Animation::InterpolationMode interpolationMode);
		/// Set the rotation interpolation mode
		void setRotationInterpolationMode(Ogre::v1::Animation::RotationInterpolationMode interpolationMode);

		Ogre::v1::Animation::InterpolationMode getPositionInterpolationMode() const
		{
			return this->interpolationMode;
		}
		/// Set the rotation interpolation mode
		Ogre::v1::Animation::RotationInterpolationMode getRotationInterpolationMode() const
		{
			return this->rotationInterpolationMode;
		}

		/// Set how often to take a keyframe in seconds when recording 
		void setKeyFrameFrequency(Ogre::Real frequency)
		{
			this->keyFrameFrequency = frequency;
		}
		/// Get how often to take a keyframe in seconds when recording
		Ogre::Real getKeyFrameFrequency() const
		{
			return this->keyFrameFrequency;
		}

		/// Take a keyframe of the current camera position
		void addKeyFrame(Ogre::Real time);

		const Ogre::v1::Animation* getAnimation() const
		{
			return this->currentAnimation;
		}

		void setAnimation(Ogre::v1::Animation* animation);

	protected:
		void createOverlay();

		Ogre::v1::TextAreaOverlayElement* createText(Ogre::Real left, Ogre::Real top, const Ogre::String& text);
	protected:
		Mode mode;
		bool enabled;
		Ogre::Viewport* viewport;
		Ogre::Camera* camera;
		Ogre::Real playbackTime;
		Ogre::Real recordingTime;
		Ogre::Real lastKeyTime;

		Ogre::Vector3 savedPos;
		Ogre::Quaternion savedOrientation;
		Ogre::v1::Animation* currentAnimation;
		Ogre::v1::NodeAnimationTrack* currentTrack;

		Ogre::v1::Animation::InterpolationMode interpolationMode;
		Ogre::v1::Animation::RotationInterpolationMode rotationInterpolationMode;
		Ogre::Real keyFrameFrequency;
		Ogre::Real playbackSpeed;

		Ogre::v1::Overlay* overlay;
		Ogre::v1::OverlayElement* modeText;
		Ogre::v1::OverlayElement* lengthText;
		Ogre::v1::OverlayElement* keyFramesText;
		Ogre::v1::OverlayElement* freqText;
		Ogre::v1::OverlayElement* freqLabel;
		Ogre::v1::OverlayElement* currentPosText;
	};

}; //namespace end

#endif