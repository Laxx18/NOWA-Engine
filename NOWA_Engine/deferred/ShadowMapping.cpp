/*
 * ShadowMapping.cpp
 *
 *  Created on: 01.07.2014
 *      Author: marvin
 */
#include "NOWAPrecompiled.h"
#include "ShadowMapping.h"


typedef std::vector<Ogre::Camera*> ShadowTextureCameraList;
typedef std::map< const Ogre::Camera*, const Ogre::Light* > ShadowCamLightMapping;

ShadowMapping::ShadowMapping(Ogre::SceneManager * sm, Ogre::Light* mParentLight, Ogre::Camera* camera)
{
	// TODO Auto-generated constructor stub

	static int op = 0;
	op++;
	if (mParentLight->getType() == Ogre::Light::LT_POINT)
	{
		mShadowCamera = sm->createCamera("LightPointShadowCam_" + mParentLight->getName());

	}
	else
	{
		mShadowCamera = sm->createCamera("LightSpotShadowCam_" + mParentLight->getName());

	}

	mlight_type = mParentLight->getType();


	mShadowCamera->setFOVy(Ogre::Degree(90.0f));
	mShadowCamera->setAspectRatio(1.0f);
	mShadowCamera->setFixedYawAxis(false);
	mShadowCamera->setNearClipDistance(0.001f);

	Ogre::Camera shadowCam("ShadowCameraSetupCam", 0);
	shadowCam._notifyViewport(camera->getViewport());
	sm->getShadowCameraSetup()->getShadowCamera(sm,
		camera, camera->getViewport(), mParentLight, &shadowCam, 0);

	mShadowCamera->setFarClipDistance(201.0f);
	mlight = mParentLight;
}

ShadowMapping::~ShadowMapping()
{
	// TODO Auto-generated destructor stub
}

void ShadowMapping::renderPointlightshadow(Ogre::SceneManager* sm, Ogre::Camera* cam, Ogre::Viewport* vp, Ogre::Light* pointlight, Ogre::Texture* cubictex)
{
	Ogre::Light* light = pointlight;

	if (!light->getCastShadows())
		return;

	mShadowCamera->setAspectRatio(1);

	if (light->getType() != Ogre::Light::LT_POINT)
		mShadowCamera->setDirection(light->getDerivedDirection());
	if (light->getType() != Ogre::Light::LT_DIRECTIONAL)
		mShadowCamera->setPosition(light->getDerivedPosition());
	// texture iteration per light.

	if (mlight_type != Ogre::Light::LT_POINT)
	{
		//			Radian fovy = light->getSpotlightOuterAngle()*1.2;
		//            	                texCam->setCustomViewMatrix(true,Ogre::Matrix4(
		//            	                		0.1,0.1,0.1,0.1,
		//										0.1,0.1,0.1,0.1,
		//										0.1,0.1,0.1,0.1,
		//										0.1,0.1,0.1,0.1a
		//            	                ));
		////            	                texCam->setCustomProjectionMatrix(true,Ogre::Matrix4(
		////            	                		0.1,0.1,0.1,0.1,
		////										0.1,0.1,0.1,0.1,
		////										0.1,0.1,0.1,0.1,
		////										0.1,0.1,0.1,0.1
		////            	                ));
		//            			texCam->setCustomViewMatrix(false);
		//                		texCam->setCustomProjectionMatrix(false);
		mShadowCamera->setNearClipDistance(light->_deriveShadowNearClipDistance(cam));
		mShadowCamera->setFarClipDistance(light->_deriveShadowFarClipDistance(cam));
		Ogre::Radian fovy = light->getSpotlightOuterAngle() * 1.2f;
		if (fovy.valueDegrees() > 175)
			fovy = Ogre::Degree(175);

		mShadowCamera->setFOVy(Ogre::Degree(fovy));

		//  shadowView->setMaterialScheme("shadow_caster");
//    			// Set perspective projection
		mShadowCamera->setProjectionType(Ogre::PT_PERSPECTIVE);
		//    			// set FOV slightly larger than the spotlight range to ensure coverage

		Ogre::RenderTarget *shadowRTT = cubictex->getBuffer(0)->getRenderTarget();
		shadowRTT->addViewport(mShadowCamera)->setOverlaysEnabled(false);

		mShadowCamera->setLodCamera(cam);
		shadowRTT->addListener(this);
		Ogre::Viewport *shadowView = shadowRTT->getViewport(0);
		shadowView->setMaterialScheme("shadow_caster");

		shadowView->setBackgroundColour(Ogre::ColourValue::White);
		mTargets[0] = shadowRTT;
	}
	else
	{
		for (size_t j = 0; j < 6; ++j)
		{
			Ogre::RenderTarget *shadowRTT = cubictex->getBuffer(j)->getRenderTarget();

			shadowRTT->addViewport(mShadowCamera)->setOverlaysEnabled(false);
			Ogre::Viewport *shadowView = shadowRTT->getViewport(0);

			mShadowCamera->setLodCamera(cam);

			shadowRTT->addListener(this);

			shadowView->setMaterialScheme("shadow_caster");
			mShadowCamera->setNearClipDistance(light->_deriveShadowNearClipDistance(cam));
			mShadowCamera->setFarClipDistance(light->_deriveShadowFarClipDistance(cam));

			shadowView->setBackgroundColour(Ogre::ColourValue::White);
			mTargets[j] = shadowRTT;
		}
	}
}

//float temp_add(float seed=rand()){
//	static unsigned int temp=0;
//
//	temp+=1;
//	if(temp>16){
//		temp=0;
//
//	}
//
//	return (rand_r(&temp)/(rand_r(&temp)*1.0f));
//}

void ShadowMapping::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
	if (mlight_type != Ogre::Light::LT_POINT)
	{
		return;
	}

	static int i = 0;

	i %= 6;
	mShadowCamera->setOrientation(Ogre::Quaternion::IDENTITY);
	if (evt.source == mTargets[0]) mShadowCamera->yaw(Ogre::Degree(-90));
	else if (evt.source == mTargets[1]) mShadowCamera->yaw(Ogre::Degree(90));
	else if (evt.source == mTargets[2]) mShadowCamera->pitch(Ogre::Degree(90));
	else if (evt.source == mTargets[3]) mShadowCamera->pitch(Ogre::Degree(-90));
	else if (evt.source == mTargets[5]) mShadowCamera->yaw(Ogre::Degree(180));
	i++;
}

void ShadowMapping::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{

}
