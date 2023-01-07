#include "NOWAPrecompiled.h"
#include "BackgroundManager2D.h"
#include "OgrePredefinedControllers.h"
#include "modules/WorkspaceModule.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"

#include "OgreOverlayManager.h"

// Must be ported to hlms
#if 1
#include "utilities/MathHelper.h"

namespace NOWA
{
	unsigned int BackgroundManager2D::id = 0;

	BackgroundManager2D::BackgroundManager2D(Ogre::SceneManager* sceneManager, const Ogre::String& textureName, const Ogre::String& resourceGroupName, unsigned int renderQueueGroup)
		: nextId(BackgroundManager2D::id++),
		lastPosition(Ogre::Vector2::ZERO),
		lastPositionX(0.0f),
		firstTimePositionSet(true),
		xScroll(0.0f),
		yScroll(0.0f)
	{
		Ogre::String uniqueMaterialName = "Background" + Ogre::StringConverter::toString(nextId);
		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		Ogre::HlmsUnlit* hlmsUnlit = dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));
		Ogre::HlmsTextureManager* hlmsTextureManager = hlmsManager->getTextureManager();

		Ogre::HlmsMacroblock macroBlock;
		macroBlock.mDepthWrite = false;
		macroBlock.mDepthCheck = false;
		macroBlock.mScissorTestEnabled = false;
		
		Ogre::HlmsBlendblock blendBlock;
		blendBlock.mIsTransparent = true;
		blendBlock.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
		blendBlock.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
		blendBlock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);

		this->datablock = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock(uniqueMaterialName, uniqueMaterialName, macroBlock, blendBlock, Ogre::HlmsParamVec()));

		Ogre::HlmsTextureManager::TextureLocation texLocation = hlmsTextureManager->createOrRetrieveTexture(textureName, Ogre::HlmsTextureManager::TEXTURE_TYPE_DIFFUSE);

		this->datablock->setTexture(0, texLocation.xIdx, texLocation.texture);

		Ogre::HlmsSamplerblock samplerblock;
		samplerblock.setAddressingMode(Ogre::TAM_WRAP);
		this->datablock->setSamplerblock(0, samplerblock);

		this->datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlmsManager->getDatablock("Materials/OverlayMaterial"));
		if (nullptr == datablock)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
				"Material name 'Materials/OverlayMaterial' cannot be found for 'FaderProcess'.",
				"FaderProcess::FaderProcess");
		}
		// Via code, or scene_blend alpha_blend in material
		// Ogre::HlmsBlendblock blendblock;
		// blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);
		// this->datablock->setBlendblock(blendblock);
		this->datablock->setUseColour(true);

		this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, 1.0f));

		// http://www.ogre3d.org/forums/viewtopic.php?f=25&t=82797 blendblock, wie macht man unlit transparent

		// Get the _overlay
		// this->overlay = Ogre::v1::OverlayManager::getSingleton().getByName("Overlays/FadeInOut");
		// this->overlay->show();

		// this->datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlmsManager->getDatablock("Materials/OverlayMaterial"));

		// this->datablock->setEnableAnimationMatrix(0, true);
		// this->datablock->setUseColour(true);
		// this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, 1.0f));

		this->rectangle = sceneManager->createRectangle2D(true, Ogre::SCENE_STATIC);
		// sceneManager->getRenderQueue()->setRenderQueueMode(this->rectangle->getRenderQueueGroup(), Ogre::RenderQueue::V1_FAST);

		// object_->setCorners(0.2, 0.2, 0.6, 0.6);
		// this->rectangle->setCorners(-1.0, 1.0, 1.0, -1.0);
		this->rectangle->setCorners(0, 0, 1, 1);
		this->rectangle->setDatablock(datablock);
		this->rectangle->setNormals(Ogre::Vector3::UNIT_Z, Ogre::Vector3::UNIT_Z, Ogre::Vector3::UNIT_Z, Ogre::Vector3::UNIT_Z);
		this->rectangle->setUseIdentityProjection(true);
		this->rectangle->setUseIdentityView(true);
		this->rectangle->setRenderQueueGroup(renderQueueGroup);

		//// Attach to scene
		Ogre::SceneNode * node = sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
		node->attachObject(this->rectangle);

		//Ogre::CompositorManager2* compositorManager = Ogre::Root::getSingletonPtr()->getCompositorManager2();

		//if (!compositorManager->hasNodeDefinition(uniqueMaterialName))
		//{
		//	Ogre::CompositorNodeDef* backgroundDef = compositorManager->addNodeDefinition("Background1");

		//	//Input channels
		//	backgroundDef->addTextureSourceName("Far_Far_Mountain.png", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
		//	// motionBlurDef->addTextureSourceName("rt_output", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

		//	// motionBlurDef->mCustomIdentifier = "Ogre/Postprocess";

		//	//Local textures
		//	/*motionBlurDef->setNumLocalTextureDefinitions(1);
		//	{
		//		Ogre::TextureDefinitionBase::TextureDefinition *texDef =
		//			motionBlurDef->addTextureDefinition("sum");
		//		texDef->width = 0;
		//		texDef->height = 0;
		//		texDef->formatList.push_back(Ogre::PF_R8G8B8);
		//	}*/

		//	backgroundDef->setNumTargetPass(1);

		//	/// Initialisation pass for sum texture
		//	{
		//		Ogre::CompositorTargetDef *targetDef = backgroundDef->addTargetPass("Background");
		//		{
		//			Ogre::CompositorPassQuadDef *passQuad;
		//			passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
		//			passQuad->mNumInitialPasses = 1;
		//			// passQuad->mMaterialName = "Ogre/Copy/4xFP32";
		//			passQuad->addQuadTextureSource(0, "Far_Far_Mountain.png", 0);
		//		}
		//	}

		//	//Output channels
		//	backgroundDef->setNumOutputChannels(2);
		//	backgroundDef->mapOutputChannel(0, "rt_output");
		//	backgroundDef->mapOutputChannel(1, "Far_Far_Mountain.png");

		//	WorkspaceModule::getInstance()->getWorkspace()->
		//}
		
		
		//// Create background material
		//this->materialPtr = Ogre::MaterialManager::getSingleton().create(uniqueMaterialName, groupName);
		//this->materialPtr->getTechnique(0)->getPass(0)->createTextureUnitState(textureName);
		//this->materialPtr->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
		//this->materialPtr->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
		//this->materialPtr->getTechnique(0)->getPass(0)->setLightingEnabled(false);
		//// this->materialPtr->getTechnique(0)->setSchemeName("Background");

		///*this->materialPtr->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SceneBlendType::SBT_TRANSPARENT_ALPHA);
		//this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 0.25f);*/
		//this->materialPtr->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		//this->materialPtr->getTechnique(0)->getPass(0)->setAlphaRejectSettings(Ogre::CMPF_GREATER, 250);

		// Create background rectangle covering the whole screen
		//this->rectangle = new Ogre::v1::Rectangle2D(true, );
		//this->rectangle->setCorners(-1.0, 1.0, 1.0, -1.0);
		//this->rectangle->setMaterial(uniqueMaterialName);
		//this->rectangle->setQueryFlags(0 << 0);
		//this->rectangle->setCastShadows(false);

		//// Render the background before everything else
		//this->rectangle->setRenderQueueGroup(renderQueueGroup);
		//// this->rectangle->setUseIdentityProjection(true);
		//// this->rectangle->setUseIdentityView(true);

		//// Use infinite AAB to always stay visible
		//Ogre::AxisAlignedBox aabInf;
		//aabInf.setInfinite();
		//this->rectangle->setBoundingBox(aabInf);

		//// Attach background to the scene
		//Ogre::SceneNode* node = sceneManager->getRootSceneNode()->createChildSceneNode(uniqueMaterialName);
		//node->attachObject(this->rectangle);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[BackgroundManager2D] Create background: " + uniqueMaterialName);

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
		if (nullptr != this->datablock)
		{
			Ogre::Hlms* hlmsUnlit = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
			hlmsUnlit->destroyDatablock(this->datablock->getName());
			this->datablock = nullptr;
		}
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
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setScrollAnimation(xSpeed, ySpeed);
		// this->datablock->set

		Ogre::Matrix4 mat(Ogre::Matrix4::IDENTITY);
		mat.setScale(Ogre::Vector3(1, 1, 1));
		mat.setTrans(Ogre::Vector3(xSpeed, ySpeed, 0));
		this->datablock->setEnableAnimationMatrix(0, true);
		this->datablock->setAnimationMatrix(0, mat);

		//this->datablock->setAnimationMatrix
		//Ogre::Controller<Ogre::Real>* ret = 0;

		//float speed = 1.0;
		//Ogre::TexCoordModifierControllerValue* ctrlVal = 0;

		//if (speed != 0)
		//{
		//	Ogre::SharedPtr< Ogre::ControllerValue<Ogre::Real> > val;
		//	Ogre::SharedPtr< Ogre::ControllerFunction<Ogre::Real> > func;

		//	val.bind(OGRE_NEW Ogre::TexCoordModifierControllerValue());
		//	ctrlVal = static_cast<Ogre::TexCoordModifierControllerValue*>(val.get());

		//	// Create function: use -speed since we're altering texture coords so they have reverse effect
		//	func.bind(OGRE_NEW Ogre::ScaleControllerFunction(-speed, true));
		//	ret = Ogre::ControllerManager::getSingleton().createController(Ogre::ControllerManager::getSingleton().getFrameTimeSource(), val, func);
		//}

		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->addEffect()
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->removeEffect()
	}

	void BackgroundManager2D::rotateAnimation(Ogre::Real speed)
	{
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setRotateAnimation(speed);
		/*this->datablock->setEnableAnimationMatrix(0, true);
		Ogre::Matrix4 mat(Ogre::Matrix4::IDENTITY);
		mat.setScale(Ogre::Vector3(1, 1, 1));
		this->datablock->setEnableAnimationMatrix(0, true);
		this->datablock->setAnimationMatrix(0, mat);*/
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

		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScroll(u, v);
		Ogre::Matrix4 mat(Ogre::Matrix4::IDENTITY);
		mat.setScale(Ogre::Vector3(1, 1, 1));
		mat.setTrans(Ogre::Vector3(u, v, 0));
		this->datablock->setEnableAnimationMatrix(0, true);
		this->datablock->setAnimationMatrix(0, mat);
	}

	void BackgroundManager2D::backgroundRelativeToPositionX(Ogre::Real absoluteXPos, Ogre::Real dt, Ogre::Real xSpeed)
	{
		//if (this->firstTimePositionSet)
		//{
		//	this->lastPositionX = absoluteXPos;
		//	this->firstTimePositionSet = false;
		//}
		//absoluteXPos = NOWA::MathHelper::getInstance()->lowPassFilter(absoluteXPos, this->lastPositionX, 0.1f);
		//Ogre::Real velocityX = absoluteXPos - this->lastPositionX;

		//// Ogre::Real velocityX = absoluteXPos - this->lastPositionX;

		//this->xScroll += velocityX * xSpeed * dt;
		//// this->xScroll = Ogre::Math::Clamp(this->xScroll, 0.0f, 2.0f);
		//if (this->xScroll > 2.0f)
		//{
		//	this->xScroll = 0.0f;
		//}
		//else if (this->xScroll < 0.0f)
		//{
		//	this->xScroll = 2.0f;
		//}
		//// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScroll(this->xScroll, 0.0f);
		//Ogre::Matrix4 mat(Ogre::Matrix4::IDENTITY);
		//mat.setScale(Ogre::Vector3(1, 1, 1));
		//mat.setTrans(Ogre::Vector3(this->xScroll, 0, 0));
		//this->datablock->setEnableAnimationMatrix(0, true);
		//this->datablock->setAnimationMatrix(0, mat);
		//this->lastPositionX = absoluteXPos;
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
		this->datablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, 1.0f));
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScroll(this->xScroll, this->yScroll);
		/*Ogre::Matrix4 mat(Ogre::Matrix4::IDENTITY);
		mat.setScale(Ogre::Vector3(1, 1, 1));
		mat.setTrans(Ogre::Vector3(this->xScroll, this->yScroll, 0));
		this->datablock->setEnableAnimationMatrix(0, true);
		this->datablock->setAnimationMatrix(0, mat);*/
		this->lastPosition = absolutePos;
	}

	void BackgroundManager2D::rotate(Ogre::Degree& degree)
	{
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureRotate(Ogre::Radian(degree.valueRadians()));
		// Ogre::Matrix4 rot(Ogre::Quaternion(
		// 	d_rotation.d_w, d_rotation.d_x, d_rotation.d_y, d_rotation.d_z));
	}

	void BackgroundManager2D::scale(Ogre::Real su, Ogre::Real sv)
	{
		// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureScale(su, sv);
		Ogre::Matrix4 mat(Ogre::Matrix4::IDENTITY);
		mat.setScale(Ogre::Vector3(su, sv, 1));
		this->datablock->setEnableAnimationMatrix(0, true);
		this->datablock->setAnimationMatrix(0, mat);
	}

	Ogre::TextureUnitState* BackgroundManager2D::getTextureUnitState(void) const
	{
		return nullptr;// this->materialPtr->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	}

}; //namespace end

#endif
