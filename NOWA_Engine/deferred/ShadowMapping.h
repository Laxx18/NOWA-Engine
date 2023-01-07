/*
 * ShadowMapping.h
 *
 *  Created on: 01.07.2014
 *      Author: marvin
 */

#ifndef SHADOWMAPPING_H_
#define SHADOWMAPPING_H_

class ShadowMapping : public Ogre::RenderTargetListener
{
public:
	Ogre::Camera * mShadowCamera;
	Ogre::RenderTarget * mTargets[6];
	Ogre::Light::LightTypes mlight_type;
	Ogre::Light* mlight;

public:
	void renderPointlightshadow(Ogre::SceneManager* sm, Ogre::Camera* cam, Ogre::Viewport* vp, Ogre::Light* pointlight, Ogre::Texture* cubictex);

	ShadowMapping(Ogre::SceneManager* sm, Ogre::Light* mParentLight, Ogre::Camera* camera);
	virtual ~ShadowMapping();

	void preRenderTargetUpdate(const  Ogre::RenderTargetEvent& evt);

	void postRenderTargetUpdate(const  Ogre::RenderTargetEvent& evt);
};

#endif /* SHADOWMAPPING_H_ */
