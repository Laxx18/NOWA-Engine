/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
same license as the rest of the engine.
-----------------------------------------------------------------------------
*/
#include "NOWAPrecompiled.h"
#include "DeferredLightCP.h"
#include "ShadowMapping.h"
#include "Ogre.h"
using namespace Ogre;

#include "LightMaterialGenerator.h"


DeferredLightRenderOperation::DeferredLightRenderOperation(CompositorInstance* instance, const CompositionPass* pass)
{
	mViewport = instance->getChain()->getViewport();

	//Get the names of the GBuffer textures
	const CompositionPass::InputTex& input0 = pass->getInput(0);
	mTexName0 = instance->getTextureInstanceName(input0.name, input0.mrtIndex);
	const CompositionPass::InputTex& input1 = pass->getInput(1);
	mTexName1 = instance->getTextureInstanceName(input1.name, input1.mrtIndex);

	// Create lights material generator
	mLightMaterialGenerator = new LightMaterialGenerator();

	// Create the ambient light
	mAmbientLight = new AmbientLight();
	const MaterialPtr& mat = mAmbientLight->getMaterial();
	mat->load();
}

DLight* DeferredLightRenderOperation::createDLight(Ogre::Light* light)
{
	DLight *rv = new DLight(mLightMaterialGenerator, light);
	mLights[light] = rv;
	return rv;
}

void injectTechnique(SceneManager* sm, Technique* tech, Renderable* rend, const Ogre::LightList* lightList)
{
	for (unsigned short i = 0; i < tech->getNumPasses(); ++i)
	{
		Ogre::Pass* pass = tech->getPass(i);
		if (lightList != 0)
		{
			sm->_injectRenderWithPass(pass, rend, false, false, lightList);
		}
		else
		{
			sm->_injectRenderWithPass(pass, rend, false);
		}

	}
}

//-------------------------new DeferredLightCompositionPass----------------------------------------------

void DeferredLightRenderOperation::execute(SceneManager *sm, RenderSystem *rs)
{
	Ogre::Camera* cam = mViewport->getCamera();

	mAmbientLight->updateFromCamera(cam);
	Technique* tech = mAmbientLight->getMaterial()->getBestTechnique();
	injectTechnique(sm, tech, mAmbientLight, 0);

	const LightList& lightList = sm->_getLightsAffectingFrustum();
	for (LightList::const_iterator it = lightList.begin(); it != lightList.end(); it++)
	{
		Light* light = *it;
		Ogre::LightList ll;
		ll.push_back(light);

		//if (++i != 2) continue;
		//if (light->getType() != Light::LT_DIRECTIONAL) continue;
		//if (light->getDiffuseColour() != ColourValue::Red) continue;

		LightsMap::iterator dLightIt = mLights.find(light);
		DLight* dLight = 0;
		if (dLightIt == mLights.end())
		{
			dLight = createDLight(light);
		}
		else
		{
			dLight = dLightIt->second;
			dLight->updateFromParent();
		}
		dLight->updateFromCamera(cam);
		tech = dLight->getMaterial()->getBestTechnique();

		//Update shadow texture
		if (dLight->getCastChadows())
		{
			//POINTLIGHT
			//***********************************************************

			if (light->getType() == Ogre::Light::LT_POINT)
			{
				Pass* pass = tech->getPass(0);

				if (!dLight->handled)
				{
					TextureUnitState* tus = pass->getTextureUnitState("ShadowMap");
					assert(tus);
					const TexturePtr& shadowTex = sm->getShadowTexture(0);

					// create our dynamic cube map texture
// Attention: here later get this data from MenuState config?
					/*Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().createManual("dyncubemapC_" + light->getName(),
						Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_CUBE_MAP, DeferredShadingConfig::getInstance()->shadowResolutionPoint, DeferredShadingConfig::getInstance()->shadowResolutionPoint, 0, Ogre::PF_R32_SINT, Ogre::TU_RENDERTARGET);*/
					Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().createManual("dyncubemapC_" + light->getName(),
						Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_CUBE_MAP, 1024, 1024, 0, Ogre::PF_R32_SINT, Ogre::TU_RENDERTARGET);
					Ogre::TexturePtr * ptrs = new Ogre::TexturePtr[6];
					ptrs[0] = tex;


					dLight->smm = new ShadowMapping(sm, light, mViewport->getCamera());
					dLight->smm->renderPointlightshadow(sm, mViewport->getCamera(), mViewport, light, tex.get());
					dLight->handled = true;
					tus->setCubicTexture(ptrs, true);
					tus->setTextureFiltering((Ogre::TextureFilterOptions)0);
					//                                      if (tus->_getTexturePtr() != shadowTex)
					//                                      {
					//                                          tus->_setTexturePtr(shadowTex);
					//

				}
				//  dLight->smm->mShadowCamera->setPosition(dLight->smm->mShadowCamera->getPosition()+Vector3(0.001,0.001,0.001));

   //         		pass->getFragmentProgramParameters()->setNamedConstant("test2",mViewport->getCamera()->getViewMatrix().inverseAffine());
   // Attention: here later get this data from MenuState config?
				/*pass->getFragmentProgramParameters()->setNamedConstant("shadowres", DeferredShadingConfig::getInstance()->shadowResolutionPoint*1.0f);
				pass->getFragmentProgramParameters()->setNamedConstant("shadowsample", DeferredShadingConfig::getInstance()->ShadowSampling*1.0f);*/
				pass->getFragmentProgramParameters()->setNamedConstant("shadowres",  1024.0f);
				pass->getFragmentProgramParameters()->setNamedConstant("shadowsample", 1.0f);
			}
			else
			{
				//SPOTLIGHT
				//***********************************************************
				Pass* pass = tech->getPass(0);
				TextureUnitState* tus = pass->getTextureUnitState("ShadowMap");

				if (!dLight->handled)
				{
					// Attention: here later get this data from MenuState config?
					/*Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().createManual("dyncubemapS_" + light->getName(),
						Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, DeferredShadingConfig::getInstance()->shadowResolutionSpot, 
						DeferredShadingConfig::getInstance()->shadowResolutionSpot, 0, Ogre::PF_R32_SINT, Ogre::TU_RENDERTARGET);*/
					Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().createManual("dyncubemapS_" + light->getName(),
						Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, 1024, 1024, 0, Ogre::PF_R32_SINT, Ogre::TU_RENDERTARGET);
					////
					dLight->smm = new ShadowMapping(sm, light, mViewport->getCamera());
					dLight->smm->renderPointlightshadow(sm, mViewport->getCamera(), mViewport, light, tex.get());
					dLight->handled = true;
				}
				const TexturePtr& shadowTex = Ogre::TextureManager::getSingleton().getByName("dyncubemapS_" + light->getName(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				
				Vector3 pos1 = dLight->smm->mShadowCamera->getPosition();
				
				const Matrix4 PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE(
					0.5, 0, 0, 0.5,
					0, -0.5, 0, 0.5,
					0, 0, 1, 0,
					0, 0, 0, 1);
				Matrix4 viewMatrix;
				Matrix4 TextureViewProjMatrix;
				dLight->smm->mShadowCamera->calcViewMatrixRelative(
					Vector3(0, 0, 0), viewMatrix);
				TextureViewProjMatrix =
					PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE *
					dLight->smm->mShadowCamera->getProjectionMatrixWithRSDepth() *
					viewMatrix;

				// pass->getFragmentProgramParameters()->setNamedConstant("test", TextureViewProjMatrix);
				// pass->getFragmentProgramParameters()->setNamedConstant("test2", mViewport->getCamera()->getViewMatrix().inverseAffine());
// Attention: here later get this data from MenuState config?
				/*pass->getFragmentProgramParameters()->setNamedConstant("shadowres", DeferredShadingConfig::getInstance()->shadowResolutionSpot*1.0f);
				pass->getFragmentProgramParameters()->setNamedConstant("shadowsample", DeferredShadingConfig::getInstance()->ShadowSampling*1.0f);*/
				pass->getFragmentProgramParameters()->setNamedConstant("shadowres", 1024.0f);
				pass->getFragmentProgramParameters()->setNamedConstant("shadowsample", 1024.0f);

				tus->setTexture(shadowTex);
				//*******************************************************************************************************

				//SceneManager::RenderContext* context = sm->_pauseRendering();
				//sm->prepareShadowTextures(cam, mViewport, &ll);
				//sm->_resumeRendering(context);

				//// assert(tus);
				//// const TexturePtr& shadowTex = sm->getShadowTexture(0);
				//tus->_setTexturePtr(shadowTex);

				//********************************************************************************************************
				tus->setTextureFiltering((Ogre::TextureFilterOptions)0);

			}
		}

		injectTechnique(sm, tech, dLight, &ll);
	}
}

DeferredLightRenderOperation::~DeferredLightRenderOperation()
{
	for (LightsMap::iterator it = mLights.begin(); it != mLights.end(); ++it)
	{
		delete it->second;
	}
	mLights.clear();

	delete mAmbientLight;
	delete mLightMaterialGenerator;
}

