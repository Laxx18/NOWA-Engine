#include "NOWAPrecompiled.h"
#include "BackgroundManager2D.h"
#include "utilities/MathHelper.h"

namespace NOWA
{
	unsigned int BackgroundManager2D::id = 0;

	BackgroundManager2D::BackgroundManager2D(Ogre::SceneManager* sceneManager, const Ogre::String& textureName, const Ogre::String& groupName, unsigned int renderQueueGroup)
		: nextId(BackgroundManager2D::id++),
		lastPosition(Ogre::Vector2::ZERO),
		lastPositionX(0.0f),
		firstTimePositionSet(true),
		xScroll(0.0f),
		yScroll(0.0f)
	{
		// Ogre::String uniqueMaterialName = "Background" + Ogre::StringConverter::toString(nextId);
		//// Create background material
		//this->materialPtr = Ogre::MaterialManager::getSingleton().create(uniqueMaterialName, groupName);
		//this->materialPtr->getTechnique(0)->getPass(0)->createTextureUnitState(textureName);
		this->materialPtr = Ogre::MaterialManager::getSingletonPtr()->getByName("NOWABackgroundPostprocess");

		/*this->materialPtr->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
		this->materialPtr->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
		this->materialPtr->getTechnique(0)->getPass(0)->setLightingEnabled(false);*/
		// this->materialPtr->getTechnique(0)->setSchemeName("Background");

		/*this->materialPtr->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SceneBlendType::SBT_TRANSPARENT_ALPHA);
		this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 0.25f);*/
		// this->materialPtr->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		// this->materialPtr->getTechnique(0)->getPass(0)->setAlphaRejectSettings(Ogre::CMPF_GREATER, 250);

		Ogre::HlmsMacroblock macroBlock;
		macroBlock.mDepthWrite = false;
		macroBlock.mDepthCheck = false;
		macroBlock.mScissorTestEnabled = false;


		Ogre::HlmsBlendblock blendBlock;
		blendBlock.mIsTransparent = true;
		blendBlock.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
		blendBlock.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
		blendBlock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);

		this->materialPtr->getTechnique(0)->getPass(0)->setMacroblock(macroBlock);
		this->materialPtr->getTechnique(0)->getPass(0)->setBlendblock(blendBlock);

		// Create background rectangle covering the whole screen
		this->rectangle = sceneManager->createRectangle2D(true, Ogre::SCENE_STATIC);
		this->rectangle->setCorners(-1.0, 1.0, 1.0, -1.0);
		this->rectangle->setMaterial(this->materialPtr);
		this->rectangle->setQueryFlags(0 << 0);
		this->rectangle->setCastShadows(false);

		sceneManager->getRenderQueue()->setRenderQueueMode(this->rectangle->getRenderQueueGroup(), Ogre::RenderQueue::V1_LEGACY);

		// Render the background before everything else
		this->rectangle->setRenderQueueGroup(renderQueueGroup);
		// this->rectangle->setUseIdentityProjection(true);
		// this->rectangle->setUseIdentityView(true);

		// Use infinite AAB to always stay visible
		/*Ogre::AxisAlignedBox aabInf;
		aabInf.setInfinite();
		this->rectangle->setBoundingBox(aabInf);*/

		// Attach background to the scene
		Ogre::SceneNode* node = sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
		node->attachObject(this->rectangle);

		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[BackgroundManager2D] Create background: " + uniqueMaterialName);

		//this->effect.amplitude = 0.7f;
		//// effect.frustum = (Frustum)CurrentCamera;
		//this->effect.type = Ogre::TextureUnitState::TextureEffectType::ET_USCROLL;

		//// effect.subtype = (System.Enum)EnvironmentMap.Reflection;
		//// effect.waveType = WaveformType.Sine;
		//// effect.phase = 0.7f;
		//this->effect.frequency = 0.7f;
		//// effect.baseVal = 0.7f;
		//// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->addEffect(this->effect);
	}

	BackgroundManager2D::~BackgroundManager2D()
	{
		if (this->rectangle)
		{
			delete this->rectangle;
			this->rectangle = nullptr;
		}
	}

	void BackgroundManager2D::reset(void)
	{
		this->lastPosition = Ogre::Vector2::ZERO;
		this->lastPositionX = 0.0f;
		this->firstTimePositionSet = true;
		this->xScroll = 0.0f;
		this->yScroll = 0.0f;
	}

	void BackgroundManager2D::scrollAnimation(Ogre::Real xSpeed, Ogre::Real ySpeed)
	{
		this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setScrollAnimation(xSpeed, ySpeed);
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->addEffect()
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->removeEffect()
	}

	void BackgroundManager2D::rotateAnimation(Ogre::Real speed)
	{
		this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setRotateAnimation(speed);
	}

	void BackgroundManager2D::scroll(Ogre::Real u, Ogre::Real v)
	{
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScroll(u, v);
		//if (Ogre::Math::RealEqual(u, 0.0f, 0.1))
		//{
		//	this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->removeEffect(this->effect.type);
		//	

		//	// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->addEffect()
		//}
		//else
		//{
		//	this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->addEffect(this->effect);
		//}
		// this->effect.frequency = Ogre::Math::Abs(u) / 10.0f;
		// Ogre::ControllerManager::getSingleton().createTextureVScroller(getTextureUnitState(), v);
		// Ogre::ControllerManager::getSingleton().updateAllControllers();
		// Ogre::ControllerManager::getSingleton().createTextureWaveTransformer()
		// http://www.ogre3d.org/forums/viewtopic.php?f=2&t=30668


		//Ogre::Matrix4 transMat;
		//transMat.makeTrans(Ogre::Vector3(u, 0.0f, 0.0f));  // translation matrix
		//Ogre::Matrix4 scaleMat(Ogre::Matrix4::IDENTITY);
		//// scaleMat.setScale(scale);  // scale matrix
		//Ogre::Matrix4 rotMat(Ogre::Quaternion::IDENTITY); // rotation matrix
		//Ogre::Matrix4 mat = /*rotMat * scaleMat **/ transMat;  // translate, then scale then rotate
		//this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureTransform(mat);

		this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScroll(u, v);
	}

	void BackgroundManager2D::backgroundRelativeToPositionX(Ogre::Real absoluteXPos, Ogre::Real dt, Ogre::Real xSpeed)
	{
		if (this->firstTimePositionSet)
		{
			this->lastPositionX = absoluteXPos;
			this->firstTimePositionSet = false;
		}
		absoluteXPos = NOWA::MathHelper::getInstance()->lowPassFilter(absoluteXPos, this->lastPositionX, 0.1f);
		Ogre::Real velocityX = absoluteXPos - this->lastPositionX;

		// Ogre::Real velocityX = absoluteXPos - this->lastPositionX;

		this->xScroll += velocityX * xSpeed * dt;
		// this->xScroll = Ogre::Math::Clamp(this->xScroll, 0.0f, 2.0f);
		if (this->xScroll > 2.0f)
		{
			this->xScroll = 0.0f;
		}
		else if (this->xScroll < 0.0f)
		{
			this->xScroll = 2.0f;
		}
		this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScroll(this->xScroll, 0.0f);
		this->lastPositionX = absoluteXPos;
	}

	void BackgroundManager2D::backgroundRelativeToPosition(Ogre::Vector2 absolutePos, Ogre::Real dt, Ogre::Real xSpeed, Ogre::Real ySpeed)
	{
		if (this->firstTimePositionSet)
		{
			this->lastPosition = absolutePos;
			this->firstTimePositionSet = false;
		}
		absolutePos.x = NOWA::MathHelper::getInstance()->lowPassFilter(absolutePos.x, this->lastPosition.x, 0.1f);
		absolutePos.y = NOWA::MathHelper::getInstance()->lowPassFilter(absolutePos.y, this->lastPosition.y, 0.1f);

		Ogre::Vector2 velocity = absolutePos - this->lastPosition;

		this->xScroll += velocity.x * xSpeed * dt;
		// this->xScroll = Ogre::Math::Clamp(this->xScroll, 0.0f, 2.0f);
		if (this->xScroll > 2.0f)
		{
			this->xScroll = 0.0f;
		}
		else if (this->xScroll < 0.0f)
		{
			this->xScroll = 2.0f;
		}

		this->yScroll -= velocity.y * ySpeed * dt;
		// this->xScroll = Ogre::Math::Clamp(this->xScroll, 0.0f, 2.0f);
		if (this->yScroll > 2.0f)
		{
			this->yScroll = 0.0f;
		}
		else if (this->yScroll < 0.0f)
		{
			this->yScroll = 2.0f;
		}
		
		this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScroll(this->xScroll, this->yScroll);
		this->lastPosition = absolutePos;
	}

	void BackgroundManager2D::rotate(Ogre::Degree& degree)
	{
		this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureRotate(Ogre::Radian(degree.valueRadians()));
	}

	void BackgroundManager2D::scale(Ogre::Real su, Ogre::Real sv)
	{
		this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScale(su, sv);
	}

	Ogre::TextureUnitState* BackgroundManager2D::getTextureUnitState(void) const
	{
		return this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	}

}; //namespace end
