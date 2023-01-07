#include "NOWAPrecompiled.h"
#include "Camcorder.h"
#include "OgreOverlayManager.h"
#include "OgreBorderPanelOverlayElement.h"
#include "OgreOverlay.h"

namespace NOWA
{
	CamcorderHelper::CamcorderHelper()
		: mode(IDLE),
		enabled(false),
		viewport(0),
		currentAnimation(0),
		currentTrack(0),
		interpolationMode(Ogre::v1::Animation::IM_SPLINE),
		rotationInterpolationMode(Ogre::v1::Animation::RIM_SPHERICAL),
		keyFrameFrequency(3.0),
		playbackSpeed(1.0),
		overlay(0)
	{

	}
	
	CamcorderHelper::~CamcorderHelper()
	{
		delete this->currentAnimation;
	}
	
	void CamcorderHelper::setEnabled(bool enabled)
	{
		this->enabled = enabled;

		if (!this->overlay)
		{
			createOverlay();
		}
		if (enabled)
		{
			this->overlay->show();
		}
		else
		{
			this->overlay->hide();
		}
	}
	
	void CamcorderHelper::setMode(Mode mode)
	{
		if (mode != this->mode)
		{
			switch (mode)
			{
			case PLAYBACK:
				this->savedPos = this->camera->getDerivedPosition();
				this->savedOrientation = this->camera->getDerivedOrientation();
				this->playbackTime = 0;
				break;
			case IDLE:
				if (this->mode == PLAYBACK)
				{
					// return camera to saved position
					this->camera->setPosition(this->savedPos);
					this->camera->setOrientation(this->savedOrientation);
				}
				break;
			case RECORDING:
				this->recordingTime = 0;
				this->lastKeyTime = 0;
				addKeyFrame(this->lastKeyTime);
				break;
			};

			this->mode = mode;
		}
	}
	
	void CamcorderHelper::init(Ogre::Viewport* viewport, Ogre::Camera* camera)
	{
		this->viewport = viewport;
		this->camera = camera;
	}
	
	void CamcorderHelper::setPositionInterpolationMode(Ogre::v1::Animation::InterpolationMode interpolationMode)
	{
		this->interpolationMode = interpolationMode;
		if (this->currentAnimation)
		{
			this->currentAnimation->setInterpolationMode(interpolationMode);
		}
	}
	
	void CamcorderHelper::setRotationInterpolationMode(Ogre::v1::Animation::RotationInterpolationMode interpolationMode)
	{
		this->rotationInterpolationMode = interpolationMode;
		if (this->currentAnimation)
		{
			this->currentAnimation->setRotationInterpolationMode(interpolationMode);
		}
	}
	
	void CamcorderHelper::processUnbufferedKeyboard(OIS::Keyboard* keyboard, Ogre::Real dt)
	{
		static float delay = 0;

		delay -= dt;

		if (!this->enabled)
		{
			return;
		}

		// Toggle playback mode
		if (keyboard->isKeyDown(OIS::KC_RETURN) && delay <= 0)
		{
			if (this->mode == IDLE)
			{
				setMode(PLAYBACK);
			}
			else if (this->mode == PLAYBACK)
			{
				setMode(IDLE);
			}

			delay = 0.5;
		}

		if (keyboard->isKeyDown(OIS::KC_INSERT) && delay <= 0)
		{
			if (this->mode == IDLE)
			{
				setMode(RECORDING);
			}
			else if (this->mode == RECORDING)
			{
				setMode(IDLE);
			}

			delay = 0.5;
		}

		if (keyboard->isKeyDown(OIS::KC_SPACE) && delay <= 0)
		{
			// take snapshot
			if (this->mode == IDLE)
			{
				if (!this->currentAnimation)
				{
					this->lastKeyTime = 0.0f;
				}
				else
				{
					this->lastKeyTime += this->keyFrameFrequency;
				}
				addKeyFrame(this->lastKeyTime);

				delay = 0.5;
			}

		}
		if (keyboard->isKeyDown(OIS::KC_1) && delay <= 0)
		{
			if (this->mode == PLAYBACK)
			{
				this->playbackSpeed -= 0.1f;
				if (this->playbackSpeed < 0.1f)
				{
					this->playbackSpeed = 0.1f;
				}
			}
			else
			{
				this->keyFrameFrequency -= 0.1f;
				if (this->keyFrameFrequency < 0.1f)
				{
					this->keyFrameFrequency = 0.1f;
				}
			}
			delay = 0.1f;
		}
		if (keyboard->isKeyDown(OIS::KC_2) && delay <= 0.0f)
		{
			if (this->mode == PLAYBACK)
			{
				this->playbackSpeed += 0.1f;
			}
			else
			{
				this->keyFrameFrequency += 0.1f;
			}
			delay = 0.1f;
		}

	}
	
	void CamcorderHelper::addKeyFrame(Ogre::Real time)
	{
		// take a snapshot
		if (!this->currentAnimation)
		{
			this->currentAnimation = new Ogre::v1::Animation("CamCorderAnim", 0);
			this->currentTrack = this->currentAnimation->createNodeTrack(0);
			this->currentAnimation->setInterpolationMode(this->interpolationMode);
			this->currentAnimation->setRotationInterpolationMode(this->rotationInterpolationMode);
			this->currentTrack->setUseShortestRotationPath(false);
		}

		if (time == 0.0f)
		{
			this->currentTrack->removeAllKeyFrames();
		}

		Ogre::v1::TransformKeyFrame* kf = this->currentTrack->createNodeKeyFrame(time);
		this->currentAnimation->setLength(time);

		kf->setTranslate(this->camera->getDerivedPosition());
		kf->setRotation(this->camera->getDerivedOrientation());
	}
	
	void CamcorderHelper::update(Ogre::Real dt)
	{
		if (!this->enabled)
		{
			return;
		}

		if (this->mode == PLAYBACK && this->currentTrack)
		{
			this->playbackTime += dt * this->playbackSpeed;

			Ogre::v1::TransformKeyFrame kf(nullptr, 0.0f);
			this->currentTrack->getInterpolatedKeyFrame(Ogre::v1::TimeIndex(this->playbackTime), &kf);

			this->camera->setPosition(kf.getTranslate());
			this->camera->setOrientation(kf.getRotation());

			this->currentPosText->setCaption(Ogre::StringConverter::toString(this->playbackTime));
			this->modeText->setCaption("PLAY");
			this->modeText->setColour(Ogre::ColourValue::Green);

			this->freqText->setCaption(Ogre::StringConverter::toString(this->playbackSpeed));
			this->freqLabel->setCaption("Playback Speed:");


		}
		else if (this->mode == RECORDING)
		{
			this->recordingTime += dt;
			if (this->recordingTime - this->lastKeyTime >= this->keyFrameFrequency)
			{
				this->lastKeyTime = this->recordingTime;
				addKeyFrame(this->lastKeyTime);
			}

			this->currentPosText->setCaption(Ogre::StringConverter::toString(this->recordingTime));
			this->modeText->setCaption("REC");
			this->modeText->setColour(Ogre::ColourValue::Red);
			this->freqText->setCaption(Ogre::StringConverter::toString(this->keyFrameFrequency));
			this->freqLabel->setCaption("Recording Freq:");

		}
		else
		{
			this->modeText->setCaption("IDLE");
			this->modeText->setColour(Ogre::ColourValue::White);
			this->currentPosText->setCaption("");
			this->freqText->setCaption(Ogre::StringConverter::toString(this->keyFrameFrequency));
			this->freqLabel->setCaption("Recording Freq:");
		}

		if (this->currentAnimation)
		{
			this->lengthText->setCaption(Ogre::StringConverter::toString(this->currentAnimation->getLength()));
			this->keyFramesText->setCaption(Ogre::StringConverter::toString(this->currentTrack->getNumKeyFrames()));
		}
		else
		{
			this->lengthText->setCaption("0");
			this->keyFramesText->setCaption("0");
		}
	}
	
	bool CamcorderHelper::isPlayingBack() const
	{
		return this->mode == PLAYBACK;
	}
	
	bool CamcorderHelper::isRecording() const
	{
		return this->mode == RECORDING;
	}
	
	bool CamcorderHelper::isIdle() const
	{
		return this->mode == IDLE;
	}
	
#define OVERLAY_WIDTH 400
#define OVERLAY_HEIGHT 85
	void CamcorderHelper::createOverlay()
	{
		Ogre::v1::OverlayManager& overlayManager = Ogre::v1::OverlayManager::getSingleton();
		this->overlay = overlayManager.create("CamCorder");

		Ogre::v1::BorderPanelOverlayElement* borderPanel = static_cast<Ogre::v1::BorderPanelOverlayElement*>(overlayManager.createOverlayElement("BorderPanel", "CC/Main"));

		borderPanel->setMetricsMode(Ogre::v1::GMM_PIXELS);
		borderPanel->setVerticalAlignment(Ogre::v1::GVA_BOTTOM);
		borderPanel->setHorizontalAlignment(Ogre::v1::GHA_CENTER);
		borderPanel->setPosition(-OVERLAY_WIDTH / 2, -(OVERLAY_HEIGHT + 5));
		borderPanel->setDimensions(OVERLAY_WIDTH, OVERLAY_HEIGHT);
		borderPanel->setMaterialName("Core/StatsBlockCenter");
		borderPanel->setBorderSize(1, 1, 1, 1);
		borderPanel->setBorderMaterialName("Core/StatsBlockBorder");
		borderPanel->setTopLeftBorderUV(0.0000f, 1.0000f, 0.0039f, 0.9961f);
		borderPanel->setTopBorderUV(0.0039f, 1.0000f, 0.9961f, 0.9961f);
		borderPanel->setTopRightBorderUV(0.9961f, 1.0000f, 1.0000f, 0.9961f);
		borderPanel->setLeftBorderUV(0.0000f, 0.9961f, 0.0039f, 0.0039f);
		borderPanel->setRightBorderUV(0.9961f, 0.9961f, 1.0000f, 0.0039f);
		borderPanel->setBottomLeftBorderUV(0.0000f, 0.0039f, 0.0039f, 0.0000f);
		borderPanel->setBottomBorderUV(0.0039f, 0.0039f, 0.9961f, 0.0000f);
		borderPanel->setBottomRightBorderUV(0.9961f, 0.0039f, 1.0000f, 0.0000f);

		this->overlay->add2D(borderPanel);

		Ogre::v1::TextAreaOverlayElement* label = createText(5.0f, 5.0f, "Mode:");
		borderPanel->addChild(label);
		label = createText(5.0f, 20.0f, "Animation Len:");
		borderPanel->addChild(label);
		label = createText(5.0f, 35.0f, "# Keyframes:");
		borderPanel->addChild(label);
		this->freqLabel = createText(5.0f, 50.0f, "Keyframe Freq:");
		borderPanel->addChild(this->freqLabel);
		label = createText(5.0f, 65.0f, "Current Pos:");
		borderPanel->addChild(label);

		label = createText(200.0f, 5.0f, "INSERT = record");
		borderPanel->addChild(label);
		label = createText(200.0f, 20.0f, "RETURN = play");
		borderPanel->addChild(label);
		label = createText(200.0f, 35.0f, "SPACE = manual keyframe");
		borderPanel->addChild(label);
		label = createText(200.0f, 50.0f, "1,2 = change freq / playback");
		borderPanel->addChild(label);

		this->modeText = createText(100.0f, 5.0f, "");
		borderPanel->addChild(this->modeText);
		this->lengthText = createText(100.0f, 20.0f, "");
		borderPanel->addChild(this->lengthText);
		this->keyFramesText = createText(100.0f, 35.0f, "");
		borderPanel->addChild(this->keyFramesText);
		this->freqText = createText(100.0f, 50.0f, "");
		borderPanel->addChild(this->freqText);
		this->currentPosText = createText(100.0f, 65.0f, "");
		borderPanel->addChild(this->currentPosText);

		this->overlay->hide();
	}
	
	Ogre::v1::TextAreaOverlayElement* CamcorderHelper::createText(Ogre::Real left, Ogre::Real top, const Ogre::String& text)
	{
		Ogre::v1::OverlayManager& overlayManager = Ogre::v1::OverlayManager::getSingleton();
		static int counter = 0;
		Ogre::v1::TextAreaOverlayElement* t = static_cast<Ogre::v1::TextAreaOverlayElement*>(overlayManager.createOverlayElement("TextArea", "CC/ModeLabel/" + Ogre::StringConverter::toString(counter++)));
		t->setMetricsMode(Ogre::v1::GMM_PIXELS);
		t->setVerticalAlignment(Ogre::v1::GVA_TOP);
		t->setHorizontalAlignment(Ogre::v1::GHA_LEFT);
		t->setPosition(left, top);
		t->setDimensions(30.0f, text.length() * 20.0f);
		t->setCaption(text);
		t->setFontName("BlueHighway");
		t->setCharHeight(16);

		return t;
	}

	void CamcorderHelper::setAnimation(Ogre::v1::Animation* animation)
	{
		if (this->currentAnimation)
		{
			this->currentAnimation->destroyAllTracks();
		}
		this->currentAnimation = animation;
		this->currentTrack = animation->getNodeTrack(0);
		this->currentAnimation->setInterpolationMode(this->interpolationMode);
		this->currentAnimation->setRotationInterpolationMode(this->rotationInterpolationMode);
		this->currentTrack->setUseShortestRotationPath(false);
	}

}; //namespace end
