#include "ShaderModulePlugin.h"

//Singleton creation
#ifndef WIN32
#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR > 7
template<> OBA::ShaderModule* Ogre::Singleton<OBA::ShaderModule>::msSingleton = 0;
#else
template<> OBA::ShaderModule* Ogre::Singleton<OBA::ShaderModule>::ms_Singleton = 0;
#endif
#endif

namespace OBA
{
	ShaderModulePlugin::ShaderModulePlugin()
	{

	}

	ShaderModulePlugin::~ShaderModulePlugin()
	{

	}

	bool ShaderModulePlugin::getUseRTShaderSystem(void) const
	{
		return this->useRTShaderSystem;
	}

	bool ShaderModulePlugin::getUseShadows(void) const
	{
		return this->useShadows;
	}

	bool ShaderModulePlugin::getUsePerPixelLightning(void) const
	{
		return this->usePerPixelLightning;
	}

	bool ShaderModulePlugin::getUseNormalMapping(void) const
	{
		return this->useNormalMapping;
	}

	bool ShaderModulePlugin::getUseFog(void) const
	{
		return this->useFog;
	}

	void ShaderModulePlugin::initializeRTShaderSystem(Ogre::SceneManager *sceneManager, Ogre::Viewport *viewport, bool usePerPixelLightning, bool useNormalMapping, bool useFog, bool useShadows)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("*** Begin: initialization of RTShaderSystem ***");
		if (Ogre::RTShader::ShaderGenerator::initialize())
		{
			// use the shader system!
			this->useRTShaderSystem = true;
			this->usePerPixelLightning = usePerPixelLightning;
			this->useNormalMapping = useNormalMapping;
			this->useFog = useFog;
			this->useShadows = useShadows;

			this->pRTShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
			this->pRTShaderGenerator->addSceneManager(sceneManager);
			//MaterialManager Listener erstellen und registrieren
			this->pRTShaderMaterialListener = new ShaderGeneratorTechniqueResolverListener(this->pRTShaderGenerator);				
			Ogre::MaterialManager::getSingleton().addListener(this->pRTShaderMaterialListener);

			//Shader Techniken anwenden
			viewport->setMaterialScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

			//GLSL benutzen wenn OpenGL als Rendersystem ausgewaehlt wurde
			/* if (Ogre::Root::getSingletonPtr()->getRenderSystem()->getName().find("OpenGL") != Ogre::String::npos)
			this->pRTShaderGenerator->setTargetLanguage("glsl");		
			//HLSL benutzen wenn Direct3D9 als Rendersystem ausgewaehlt wurde
			else if (Ogre::Root::getSingletonPtr()->getRenderSystem()->getName().find("Direct3D9") != Ogre::String::npos)
			this->pRTShaderGenerator->setTargetLanguage("hlsl");*/

			this->pRTShaderGenerator->setTargetLanguage("cg");
			//Schema Renderstate holen											
			Ogre::RTShader::RenderState *pSchemRenderState = this->pRTShaderGenerator->getRenderState(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

			this->pRTShaderGenerator->setShaderCachePath("");
			// Add per pixel lighting sub render state to the global scheme render state.
			// It will override the default FFP lighting sub render state.
			//if (usePerPixelLighting)
			{
				//Ogre::RTShader::SubRenderState* pPerPixelLightModel = this->pRTShaderGenerator->createSubRenderState(Ogre::RTShader::PerPixelLighting::Type);
				//pSchemRenderState->addTemplateSubRenderState(pPerPixelLightModel);					
			}
			// Invalidate the scheme in order to re-generate all shaders based technique related to this scheme.
			this->pRTShaderGenerator->invalidateScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

			//Konfigurationen anwenden
			this->usePerPixelLightning = usePerPixelLightning;
			this->useNormalMapping = useNormalMapping;
			this->useFog = useFog;
			this->useShadows = useShadows;

			if (this->useFog)
				this->applyPerPixelFog(sceneManager);

			//this->useRTShaderSystem = true;
			Ogre::LogManager::getSingletonPtr()->logMessage("*** Finished: initialization of RTShaderSystem ***");
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("*** Error: RTShaderSystem is not available ***");
			//this->useRTShaderSystem = false;
		}
	}

	void ShaderModulePlugin::deleteRTShaderSystem(void)
	{
		if (this->pRTShaderGenerator)
		{
			//Default Schema widerherstellen
			Ogre::MaterialManager::getSingleton().setActiveScheme(Ogre::MaterialManager::DEFAULT_SCHEME_NAME);
			//MaterialManager Listener entfernen
			if (this->pRTShaderMaterialListener != NULL)
			{        
				Ogre::MaterialManager::getSingleton().removeListener(this->pRTShaderMaterialListener);
				delete this->pRTShaderMaterialListener;
				this->pRTShaderMaterialListener = NULL;
			}
			//RTShader entfernen  
			Ogre::RTShader::ShaderGenerator::finalize();
			this->pRTShaderGenerator = NULL;
		}
	}

	void ShaderModulePlugin::generateRTShaders(Ogre::SubEntity *pSubEntity, const Ogre::String &strNormalMap)
	{
		const Ogre::String &curMaterialName = pSubEntity->getMaterialName();
		bool success;

		// Create the shader based technique of this material.
		success = this->pRTShaderGenerator->createShaderBasedTechnique(curMaterialName, Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);


		// Setup custom shader sub render states according to current setup.
		if (success)
		{					
			Ogre::MaterialPtr curMaterial = Ogre::MaterialManager::getSingleton().getByName(curMaterialName);

			Ogre::Pass *pCurPass = curMaterial->getTechnique(0)->getPass(0);
			//Ogre::TextureUnitState *pTextureUnitState = pCurPass->getTextureUnitState(1);


			//if (mSpecularEnable)
			{
				pCurPass->setSpecular(Ogre::ColourValue::White);
				pCurPass->setShininess(80.0f);
			}
			//else
			//{
			//	curPass->setSpecular(ColourValue::Black);
			//	curPass->setShininess(0.0);
			//}


			// Grab the first pass render state. 
			// NOTE: For more complicated samples iterate over the passes and build each one of them as desired.
			Ogre::RTShader::RenderState *pRenderState = this->pRTShaderGenerator->getRenderState(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, curMaterialName, 0);
			// Remove all sub render states.
			pRenderState->reset();


			/*#ifdef RTSHADER_SYSTEM_BUILD_CORE_SHADERS
			if (mCurLightingModel == SSLM_PerVertexLighting)
			{
			RTShader::SubRenderState* perPerVertexLightModel = mShaderGenerator->createSubRenderState(RTShader::FFPLighting::Type);

			renderState->addTemplateSubRenderState(perPerVertexLightModel);	
			}
			#endif*/

			//#ifdef RTSHADER_SYSTEM_BUILD_EXT_SHADERS
			//else if (mCurLightingModel == SSLM_PerPixelLighting)
			if (this->usePerPixelLightning)
			{
				Ogre::RTShader::SubRenderState *pPerPixelLightModel = this->pRTShaderGenerator->createSubRenderState(Ogre::RTShader::PerPixelLighting::Type);

				pRenderState->addTemplateSubRenderState(pPerPixelLightModel);				
			}
			//else if (mCurLightingModel == SSLM_NormalMapLightingTangentSpace)
			else if (this->useNormalMapping)
			{
				// Apply normal map only on main entity.
				//if (entity->getName() == MAIN_ENTITY_NAME)
				//{
				Ogre::RTShader::SubRenderState *pSubRenderState = this->pRTShaderGenerator->createSubRenderState(Ogre::RTShader::NormalMapLighting::Type);
				Ogre::RTShader::NormalMapLighting *pNormalMapSubRS = static_cast<Ogre::RTShader::NormalMapLighting*>(pSubRenderState);

				pNormalMapSubRS->setNormalMapSpace(Ogre::RTShader::NormalMapLighting::NMS_TANGENT);

				//pNormalMapSubRS->setNormalMapTextureName(pTextureUnitState->getTextureName());	
				pNormalMapSubRS->setNormalMapTextureName(strNormalMap);	
				pRenderState->addTemplateSubRenderState(pNormalMapSubRS);

				// It is secondary entity -> use simple per pixel lighting.
				/*else
				{
				RTShader::SubRenderState* perPixelLightModel = mShaderGenerator->createSubRenderState(RTShader::PerPixelLighting::Type);
				renderState->addTemplateSubRenderState(perPixelLightModel);
				}*/				
			}

			// Apply normal map only on main entity.
			//if (entity->getName() == MAIN_ENTITY_NAME)
			/*{
			Ogre::RTShader::SubRenderState *pSubRenderState = this->pRTShaderGenerator->createSubRenderState(Ogre::RTShader::NormalMapLighting::Type);
			Ogre::RTShader::NormalMapLighting *pNormalMapSubRS = static_cast<Ogre::RTShader::NormalMapLighting*>(pSubRenderState);

			pNormalMapSubRS->setNormalMapSpace(Ogre::RTShader::NormalMapLighting::NMS_OBJECT);
			pNormalMapSubRS->setNormalMapTextureName(strNormalMap);	

			pRenderState->addTemplateSubRenderState(pNormalMapSubRS);
			}*/

			// It is secondary entity -> use simple per pixel lighting.
			/*else
			{
			RTShader::SubRenderState* perPixelLightModel = mShaderGenerator->createSubRenderState(RTShader::PerPixelLighting::Type);
			renderState->addTemplateSubRenderState(perPixelLightModel);
			}	*/			


			//#endif

			/*if (mReflectionMapEnable)
			{				
			RTShader::SubRenderState* subRenderState = mShaderGenerator->createSubRenderState(ShaderExReflectionMap::Type);
			ShaderExReflectionMap* reflectionMapSubRS = static_cast<ShaderExReflectionMap*>(subRenderState);

			reflectionMapSubRS->setReflectionMapType(TEX_TYPE_CUBE_MAP);
			reflectionMapSubRS->setReflectionPower(mReflectionPowerSlider->getValue());

			// Setup the textures needed by the reflection effect.
			reflectionMapSubRS->setMaskMapTextureName("Panels_refmask.png");	
			reflectionMapSubRS->setReflectionMapTextureName("cubescene.jpg");

			renderState->addTemplateSubRenderState(subRenderState);
			mReflectionMapSubRS = subRenderState;				
			}
			else
			{
			mReflectionMapSubRS = NULL;
			}*/

			// Invalidate this material in order to re-generate its shaders.
			this->pRTShaderGenerator->invalidateMaterial(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, curMaterialName);
		}		
	}

	//Wird intern genutzt
	void ShaderModulePlugin::applyShadowType(Ogre::SceneManager *sceneManager, Ogre::Camera *camera, Ogre::Real viewRange)
	{
		// Grab the scheme render state.												
		////////////Ogre::RTShader::RenderState *pSchemRenderState = this->pRTShaderGenerator->getRenderState(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

		sceneManager->setShadowTechnique( Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED /*Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED*/);

		// 3 textures per directional light
		sceneManager->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, 3);
		sceneManager->setShadowColour(Ogre::ColourValue(0.4f, 0.4f, 0.4f));

		sceneManager->setShadowTextureCount(3);
		sceneManager->setShadowTextureConfig(0, 2048, 2048, Ogre::PF_FLOAT32_R);
		sceneManager->setShadowTextureConfig(1, 1024, 1024, Ogre::PF_FLOAT32_R);
		sceneManager->setShadowTextureConfig(2, 1024, 1024, Ogre::PF_FLOAT32_R);
		sceneManager->setShadowTextureSelfShadow(true);
		sceneManager->setShadowCasterRenderBackFaces(true);
		sceneManager->setShadowDirectionalLightExtrusionDistance(viewRange);
		// Set up caster material - this is just a standard depth/shadow map caster
		sceneManager->setShadowTextureCasterMaterial("PSSM/shadow_caster");

		sceneManager->setShadowDirLightTextureOffset(100.0f);
		sceneManager->setLateMaterialResolving(true);
		//Darf man hier nicht setzten!
		//sceneManager->setShadowTextureReceiverMaterial("Ogre/DepthShadowmap/Receiver/Float");
		//sceneManager->setShadowTextureCasterMaterial(Ogre::StringUtil::BLANK);

		// Disable fog on the caster pass.
		Ogre::MaterialPtr passCasterMaterial = Ogre::MaterialManager::getSingleton().getByName("PSSM/shadow_caster");
		Ogre::Pass *pPssmCasterPass = passCasterMaterial->getTechnique(0)->getPass(0);
		//pPssmCasterPass->setFog(true);

		// shadow camera setup
		if (this->pPSSMSetup.isNull())
		{
			Ogre::Real shadowDistance = 0.0f;
			if (viewRange / 10.0f < 100.0f)
				shadowDistance = 50.0f;
			else
				shadowDistance = viewRange / 10.0f;
			//sceneManager->setShadowFarDistance(shadowDistance);

			sceneManager->setShadowFarDistance(viewRange);
			//sceneManager->setShowDebugShadows(true);

			Ogre::PSSMShadowCameraSetup *pssmSetup = new Ogre::PSSMShadowCameraSetup();

			pssmSetup->calculateSplitPoints(3, camera->getNearClipDistance() * 0.0001f, sceneManager->getShadowFarDistance());
			pssmSetup->setSplitPadding(camera->getNearClipDistance());
			//pssmSetup->setOptimalAdjustFactor(0, 16.0f);
			//pssmSetup->setOptimalAdjustFactor(1, 4.0f);
			//pssmSetup->setOptimalAdjustFactor(2, 2.0f);
			pssmSetup->setOptimalAdjustFactor(0, 2.0f);
			pssmSetup->setOptimalAdjustFactor(1, 1.0f);
			pssmSetup->setOptimalAdjustFactor(2, 0.5f);

			//pssmSetup->setCameraLightDirectionThreshold(Ogre::Degree(-40));
			this->pPSSMSetup.bind(pssmSetup);
			sceneManager->setShadowCameraSetup(Ogre::ShadowCameraSetupPtr(this->pPSSMSetup));

			/*Ogre::RTShader::SubRenderState *pSubRenderState = this->pRTShaderGenerator->createSubRenderState(Ogre::RTShader::IntegratedPSSM3::Type);	
			Ogre::RTShader::IntegratedPSSM3 *pPssm3SubRenderState = static_cast<Ogre::RTShader::IntegratedPSSM3*>(pSubRenderState);
			const Ogre::PSSMShadowCameraSetup::SplitPointList& srcSplitPoints = pssmSetup->getSplitPoints();
			Ogre::RTShader::IntegratedPSSM3::SplitPointList dstSplitPoints;

			// Ogre::Vector4 splitPoints(0.0);

			for (unsigned int i=0; i < srcSplitPoints.size(); ++i)
			{
				dstSplitPoints.push_back(srcSplitPoints[i]);
				//splitPoints[i] = srcSplitPoints[i];
			}

			pPssm3SubRenderState->setSplitPoints(dstSplitPoints);
			pSchemRenderState->addTemplateSubRenderState(pSubRenderState);	*/	

			/*Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName( "Ogre/DepthShadowmap/Receiver/Float" );
			for( int i = 0; i < mat->getNumTechniques(); ++i )
			{
			mat->getTechnique(i)->getPass(0)->getFragmentProgramParameters()->setNamedConstant( "pssmSplitPoints", splitPoints);
			}*/
		}


		// Invalidate the scheme in order to re-generate all shaders based technique related to this scheme.
		//////////////this->pRTShaderGenerator->invalidateScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
	}

	void ShaderModulePlugin::applyPerPixelFog(Ogre::SceneManager *sceneManager)
	{
		// Grab the scheme render state.
		Ogre::RTShader::RenderState *pSchemRenderState = this->pRTShaderGenerator->getRenderState(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
		const Ogre::RTShader::SubRenderStateList &subRenderStateList = pSchemRenderState->getTemplateSubRenderStateList();
		Ogre::RTShader::SubRenderStateListConstIterator it = subRenderStateList.begin();
		Ogre::RTShader::SubRenderStateListConstIterator itEnd = subRenderStateList.end();
		Ogre::RTShader::FFPFog *pFogSubRenderState = NULL;

		// Search for the fog sub state.
		for (; it != itEnd; ++it)
		{
			Ogre::RTShader::SubRenderState *pCurSubRenderState = *it;

			if (pCurSubRenderState->getType() == Ogre::RTShader::FFPFog::Type)
			{
				pFogSubRenderState = static_cast<Ogre::RTShader::FFPFog*>(pCurSubRenderState);
				break;
			}
		}

		// Create the fog sub render state if need to.
		if (pFogSubRenderState == NULL)
		{			
			Ogre::RTShader::SubRenderState *pSubRenderState = this->pRTShaderGenerator->createSubRenderState(Ogre::RTShader::FFPFog::Type);

			pFogSubRenderState = static_cast<Ogre::RTShader::FFPFog*>(pSubRenderState);
			pSchemRenderState->addTemplateSubRenderState(pFogSubRenderState);
		}


		// Select the desired fog calculation mode.
		//if (mPerPixelFogEnable)
		{
			//pFogSubRenderState->setCalcMode(Ogre::RTShader::FFPFog::CM_PER_PIXEL);
			pFogSubRenderState->setCalcMode(Ogre::RTShader::FFPFog::CM_PER_VERTEX);
		}
		//else
		//{
		//	fogSubRenderState->setCalcMode(FFPFog::CM_PER_VERTEX);
		//}

		// Invalidate the scheme in order to re-generate all shaders based technique related to this scheme.
		this->pRTShaderGenerator->invalidateScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

	}

	Ogre::MaterialPtr ShaderModulePlugin::buildDepthShadowMaterial(const Ogre::String& materialName)
	{
		Ogre::MaterialPtr objectMaterial = Ogre::MaterialManager::getSingleton().getByName(materialName);
		Ogre::MaterialPtr shadowMaterial = Ogre::MaterialManager::getSingleton().getByName("Ogre/shadow/depth/integrated/pssm");
		//MaterialPtr shadowMaterial = MaterialManager::getSingleton().getByName("Examples/OffsetMapping/IntegratedShadows/Custom");
		//MaterialPtr resultMaterial = shadowMaterial->clone("DepthShadows/" + materialName);


		Ogre::Pass *objectPass = objectMaterial->getTechnique(0)->getPass(0);
		Ogre::Pass *shadowPass = shadowMaterial->getTechnique(0)->getPass(0);
		Ogre::TextureUnitState *pTextureUnitState = objectPass->getTextureUnitState(0);
		if (pTextureUnitState != NULL)
		{
			shadowPass->getTextureUnitState("diffuse")->setTextureName(pTextureUnitState->getTextureName());

			Ogre::Vector4 splitPoints;
			const Ogre::PSSMShadowCameraSetup::SplitPointList& splitPointList = static_cast<Ogre::PSSMShadowCameraSetup*>(this->pPSSMSetup.get())->getSplitPoints();
			for (int i = 0; i < 3; ++i)
			{
				splitPoints[i] = splitPointList[i];
			}
			shadowPass->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);
			//resultMaterial->copyDetailsTo(objectMaterial);

			//shadowMaterial->copyDetailsTo(objectMaterial);
			//objectMaterial.setNull();
			//shadowMaterial.setNull();
			return shadowMaterial;
		}
		else
			return objectMaterial;
	}

	void ShaderModulePlugin::substractOutTangentsForShader(Ogre::Entity *entity)
	{
		Ogre::MeshPtr pObj = entity->getMesh();
		unsigned short src, dest;
		if (!pObj->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
			pObj->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
	}

	/*MaterialPtr Core::buildDepthShadowMaterial(const String& materialName)
	{

	MaterialPtr objectMaterial = MaterialManager::getSingleton().getByName(materialName);
	MaterialPtr shadowMaterial = MaterialManager::getSingleton().getByName("Ogre/shadow/depth/integrated/pssm");
	//MaterialPtr resultMaterial = shadowMaterial->clone("DepthShadows/" + materialName);

	Pass *objectPass = objectMaterial->getTechnique(0)->getPass(0);

	TextureUnitState *pTextureUnitState = objectPass->getTextureUnitState("DiffuseTexture");
	if (pTextureUnitState != NULL)
	{
	Pass *shadowPass = shadowMaterial->getTechnique(0)->getPass(0);
	if (shadowPass)
	{
	shadowPass->getTextureUnitState("diffuse")->setTextureName(pTextureUnitState->getTextureName());

	Vector4 splitPoints;
	const PSSMShadowCameraSetup::SplitPointList& splitPointList = static_cast<PSSMShadowCameraSetup*>(this->pPSSMSetup.get())->getSplitPoints();
	for (int i = 0; i < 3; ++i)
	{
	splitPoints[i] = splitPointList[i];
	}
	shadowPass->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);
	}
	}
	shadowMaterial->copyDetailsTo(objectMaterial);
	return shadowMaterial;
	}*/

	/*MaterialPtr Core::buildDepthShadowMaterial(const String& materialName)
	{
	String matName = "DepthShadows/" + materialName;

	MaterialPtr ret = MaterialManager::getSingleton().getByName(matName);
	if (ret.isNull())
	{
	MaterialPtr baseMat = MaterialManager::getSingleton().getByName("Ogre/shadow/depth/integrated/pssm");
	ret = baseMat->clone(matName);
	Pass* p = ret->getTechnique(0)->getPass(0);
	p->getTextureUnitState("diffuse")->setTextureName(materialName);

	Vector4 splitPoints;
	const PSSMShadowCameraSetup::SplitPointList& splitPointList = 
	static_cast<PSSMShadowCameraSetup*>(pPSSMSetup.get())->getSplitPoints();
	for (int i = 0; i < 3; ++i)
	{
	splitPoints[i] = splitPointList[i];
	}
	p->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);


	}

	return ret;
	}*/

	void ShaderModulePlugin::configureShadows(Ogre::SceneManager *sceneManager, Ogre::Camera *camera, bool enabled, Ogre::Real viewRange)
	{

		/*Texture shadows involve rendering shadow casters from the point of view of the light into a texture, which is then projected 
		onto shadow receivers. The main advantage of texture shadows as opposed to 7.1 Stencil Shadows is that the overhead of increasing 
		the geometric detail is far lower, since there is no need to perform per-triangle calculations. Most of the work in rendering texture 
		shadows is done by the graphics card, meaning the technique scales well when taking advantage of the latest cards, which are at present outpacing CPUs in terms of their speed of development. In addition, texture shadows are much more customisable - you can pull them into shaders to apply as you like (particularly with Integrated Texture Shadows, you can perform filtering to create softer shadows or perform other special effects on them. Basically, most modern engines use texture shadows as their primary shadow technique simply because they are more powerful, and the increasing speed of GPUs is rapidly amortizing the fillrate / texture access costs of using them.
		The main disadvantage to texture shadows is that, because they are simply a texture, they have a fixed resolution which means if stretched, the pixellation of the texture can become obvious. There are ways to combat this though:

		Choosing a projection basis
		The simplest projection is just to render the shadow casters from the lights perspective using a regular camera setup. 
		This can look bad though, so there are many other projections which can help to improve the quality from the main camera's 
		perspective. OGRE supports pluggable projection bases via it's ShadowCameraSetup class, and comes with several existing 
		options - Uniform (which is the simplest), Uniform Focussed (which is still a normal camera projection, except that the camera 
		is focussed into the area that the main viewing camera is looking at), LiSPSM (Light Space Perspective Shadow Mapping - which 
		both focusses and distorts the shadow frustum based on the main view camera) and Plan Optimal (which seeks to optimise the shadow 
		fidelity for a single receiver plane). 
		Filtering
		You can also sample the shadow texture multiple times rather than once to soften the shadow edges and improve the appearance. 
		Percentage Closest Filtering (PCF) is the most popular approach, although there are multiple variants depending on the number 
		and pattern of the samples you take. Our shadows demo includes a 5-tap PCF example combined with depth shadow mapping. 
		Using a larger texture
		Again as GPUs get faster and gain more memory, you can scale up to take advantage of this. 

		If you combine all 3 of these techniques you can get a very high quality shadow solution. The other issue is with point lights. 
		Because texture shadows require a render to texture in the direction of the light, omnidirectional lights (point lights) would 
		require 6 renders to totally cover all the directions shadows might be cast. For this reason, Ogre primarily supports directional 
		lights and spotlights for generating texture shadows; you can use point lights but they will only work if off-camera since they 
		are essentially turned into a spotlight shining into your camera frustum for the purposes of texture shadows.*/

		if (enabled)
		{

			//Schatten einstellen
			//SHADOWTYPE_TEXTURE_MODULATIVE
			//the shadow colour for modulative, black for additive
			//sceneManager->setShadowTechnique(SHADOWDETAILTYPE_TEXTURE);
			/*sceneManager->setShadowTechnique(SHADOWDETAILTYPE_TEXTURE);

			sceneManager->setShadowFarDistance(30);
			//Parallel Split Shadow Mapping (PSSM)
			//3 Texturen pro Licht (PSSM)
			//sceneManager->setShadowTextureCountPerLightType(Light::LT_DIRECTIONAL, 3);

			if (this->pPSSMSetup.isNull())
			{
			//Schattenkamera einstellen
			PSSMShadowCameraSetup* pssmSetup = new PSSMShadowCameraSetup();
			pssmSetup->setSplitPadding(camera->getNearClipDistance());
			pssmSetup->calculateSplitPoints(2, camera->getNearClipDistance(), sceneManager->getShadowFarDistance());
			pssmSetup->setOptimalAdjustFactor(0, 1);
			pssmSetup->setOptimalAdjustFactor(1, 10);


			// get split points
			Vector4 splitPoints;
			const PSSMShadowCameraSetup::SplitPointList& splitPointList = pssmSetup->getSplitPoints();
			for (int i = 0; i < 2; ++i)
			splitPoints[i] = splitPointList[i];

			// prepare materials for pssm
			ResourceManager::ResourceMapIterator materials = MaterialManager::getSingleton().getResourceIterator();
			while (materials.hasMoreElements())
			{
			Material *material = static_cast<Material*>(materials.getNext().getPointer());
			Pass *pass = material->getTechnique(0)->getPass("pssm");
			if (pass != NULL)
			pass->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);
			}
			//pssmSetup->setUseAggressiveFocusRegion(true);
			//pssmSetup->setOptimalAdjustFactor(0, 2);
			//pssmSetup->setOptimalAdjustFactor(1, 1);
			//pssmSetup->setOptimalAdjustFactor(2, 0.5);

			this->pPSSMSetup.bind(pssmSetup);
			}
			//DefaultShadowCameraSetup *pssmSetup = new DefaultShadowCameraSetup();

			//sceneManager->setShadowCameraSetup(ShadowCameraSetupPtr(pssmSetup));
			sceneManager->setShadowCameraSetup(this->pPSSMSetup);
			sceneManager->setShadowTextureCount(2);
			sceneManager->setShadowTextureConfig(0, 2048, 2048, PF_X8B8G8R8);
			sceneManager->setShadowTextureConfig(1, 2048, 2048, PF_X8B8G8R8);
			//sceneManager->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
			//Dafuer muessen Depth-Shadows eingestellt werden
			sceneManager->setShadowTextureSelfShadow(true);
			sceneManager->setShadowCasterRenderBackFaces(true);
			//sceneManager->setShadowTextureCasterMaterial( StringUtil::BLANK);
			//sceneManager->setShadowTextureCasterMaterial("PSSM/shadow_caster");
			//sceneManager->setShadowTextureReceiverMaterial("Ogre/DepthShadowmap/Receiver/Float");
			sceneManager->setShadowDirectionalLightExtrusionDistance(100);
			sceneManager->setShadowDirLightTextureOffset(100);
			//sceneManager->setShadowIndexBufferSize(25600);


			//LiSPSMShadowCameraSetup *pLiSPSMSetup = new LiSPSMShadowCameraSetup();
			sceneManager->setShadowUseInfiniteFarPlane(false);

			//sceneManager->setShadowTextureCasterMaterial("Ogre/DepthShadowmap/Caster/Float");
			//sceneManager->setShadowTextureReceiverMaterial("Ogre/DepthShadowmap/Receiver/Float/PCF");
			*/

			Ogre::Real shadowDistance = 0.0f;
			if (viewRange / 10.0f > 100.0f)
				shadowDistance = 100.0f;
			else
				shadowDistance = viewRange / 10.0f;

			sceneManager->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);
			sceneManager->setShadowFarDistance(shadowDistance);
			sceneManager->setShadowTextureCount(3);
			sceneManager->setShadowTextureConfig(0, 1024, 1024, Ogre::PF_FLOAT32_R);
			sceneManager->setShadowTextureConfig(1, 1024, 1024, Ogre::PF_FLOAT32_R);
			sceneManager->setShadowTextureConfig(2, 1024, 1024, Ogre::PF_FLOAT32_R);
			sceneManager->setShadowTextureSelfShadow(false);
			sceneManager->setShadowCasterRenderBackFaces(false);
			//sceneManager->setShadowTextureCasterMaterial(StringUtil::BLANK);
			if (this->pPSSMSetup.isNull())
			{
				Ogre::PSSMShadowCameraSetup* pssmSetup = new Ogre::PSSMShadowCameraSetup();
				pssmSetup->setSplitPadding(camera->getNearClipDistance());
				pssmSetup->calculateSplitPoints(3, camera->getNearClipDistance(), sceneManager->getShadowFarDistance());
				pssmSetup->setOptimalAdjustFactor(0, 2);
				pssmSetup->setOptimalAdjustFactor(1, 1);
				pssmSetup->setOptimalAdjustFactor(2, 0.5);
				pPSSMSetup.bind(pssmSetup);
			}
			sceneManager->setShadowCameraSetup(this->pPSSMSetup);
			//sceneManager->setShadowTextureCasterMaterial("shadowCaster");

			//sceneManager->setShadowTextureCasterMaterial("Ogre/DepthShadowmap/Caster/Float");
			//sceneManager->setShadowTextureReceiverMaterial("Ogre/DepthShadowmap/Receiver/Float");
			//Nur Relevant fuer Stencil-Shadows
			//sceneManager->setShadowUseInfiniteFarPlane(false);
			//Nur relevant fuer additive Schatten
			//sceneManager->setShadowUseLightClipPlanes(false);
			//Relevant fuer Directional-Lights
			//Distanz bis wann der Schatten gerendert werden soll (Point-, Spotlights haben die LightAttentuation)
			sceneManager->setShadowDirectionalLightExtrusionDistance(100);
			//Verhindern, das ausserhalb des Kamerakegels die Schattentextur mit Pixeln belegt wird
			//Vorsicht: Hier testen ob Artefakte enstehen wenn man sich bewegt
			sceneManager->setShadowDirLightTextureOffset(100);
			sceneManager->setShadowIndexBufferSize(25600);
		}
		else
		{
			sceneManager->setShadowTechnique(Ogre::SHADOWTYPE_NONE);
		}
	}

		//Override
	const Ogre::String& ShaderModulePlugin::getName() const
	{
		return this->name;
	}

	//Override
	void ShaderModulePlugin::install()
	{

	}

	//Override
	void ShaderModulePlugin::initialise()
	{
		useRTShaderSystem = false;
		pRTShaderGenerator = static_cast<Ogre::RTShader::ShaderGenerator*>(0);
		pRTShaderMaterialListener = static_cast<ShaderGeneratorTechniqueResolverListener*>(0);
		this->name = "ShaderModulePlugin";
	}

	//Override
	void ShaderModulePlugin::shutdown()
	{

	}

	//Override
	void ShaderModulePlugin::uninstall()
	{
		//Schattenkamera auf 0 setzen, dies muss hier stehen sonst erfolgt ein Absturz beim Beenden der Anwendung!
		if (!this->pPSSMSetup.isNull())
			this->pPSSMSetup.setNull();
	}


	ShaderModulePlugin* ShaderModulePlugin::getSingletonPtr(void)
	{
#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR > 7
		return msSingleton;
#else
		return ms_Singleton;
#endif
	}

	ShaderModulePlugin& ShaderModulePlugin::getSingleton(void)
	{  
#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR > 7
		assert(msSingleton);
		return (*msSingleton);
#else
		assert(ms_Singleton);
		return (*ms_Singleton);
#endif
	}

	ShaderModulePlugin* pShaderModule;

	extern "C" EXPORTED void dllStartPlugin()
	{
		pShaderModule = new ShaderModulePlugin();
		Ogre::Root::getSingleton().installPlugin(pShaderModule);
	}

	extern "C" EXPORTED void dllStopPlugin()
	{
		Ogre::Root::getSingleton().uninstallPlugin(pShaderModule);
	delete pShaderModule;
	pShaderModule = static_cast<ShaderModulePlugin*>(0);
}

}; // namespace end