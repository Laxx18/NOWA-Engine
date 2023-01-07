#include "modules/OgreTerrainRTSSMaterialGenerator.h"

#include <Terrain/OgreTerrain.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreTextureUnitState.h>
#include <OgreGpuProgramManager.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreShadowCameraSetupPSSM.h>

namespace Ogre
{
	TerrainRTSSMaterialGenerator::TerrainRTSSMaterialGenerator()
		: TerrainMaterialGeneratorA()
	{
		// Add custom SM2Profile SPAM
		mProfiles.clear();  // TODO - This will have to be changed if TerrainMaterialGeneratorA ever supports more profiles than only CG
		mProfiles.push_back(OGRE_NEW SM2Profile(this, "SM2", "Profile for rendering on Shader Model 2 capable cards (RTSS depth shadows compatible)"));
		// TODO - check hardware capabilities & use fallbacks if required (more profiles needed)
		setActiveProfile(mProfiles[0]);

	}
	//---------------------------------------------------------------------
	TerrainRTSSMaterialGenerator::~TerrainRTSSMaterialGenerator()
	{

	}
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	TerrainRTSSMaterialGenerator::CustomSM2Profile::CustomSM2Profile(TerrainMaterialGenerator* parent, const String& name, const String& desc)
		: TerrainMaterialGeneratorA::SM2Profile(parent, name, desc)
		//: TerrainMaterialGeneratorA::SM2Profile::SM2Profile(parent, name, desc) why???????????????????????????? does that not work????????????
	{

	}
	//---------------------------------------------------------------------
	TerrainRTSSMaterialGenerator::CustomSM2Profile::~CustomSM2Profile()
	{
		// Because the base SM2Profile has no virtual destructor:
		OGRE_DELETE mShaderGen;
	}
	//---------------------------------------------------------------------
	void TerrainRTSSMaterialGenerator::CustomSM2Profile::addTechnique(
		const MaterialPtr& mat, const Terrain* terrain, TechniqueType tt)
	{
		// Initiate specialized mShaderGen
		GpuProgramManager& gmgr = GpuProgramManager::getSingleton();
		HighLevelGpuProgramManager& hmgr = HighLevelGpuProgramManager::getSingleton();
		if (!mShaderGen)
		{
			bool check2x = mLayerNormalMappingEnabled || mLayerParallaxMappingEnabled;
			if (hmgr.isLanguageSupported("cg"))
				mShaderGen = OGRE_NEW TerrainRTSSMaterialGenerator::SM2Profile::ShaderHelperCg();
			else if (hmgr.isLanguageSupported("hlsl") &&
				((check2x && gmgr.isSyntaxSupported("ps_4_0")) ||
				(check2x && gmgr.isSyntaxSupported("ps_2_x")) ||
				(!check2x && gmgr.isSyntaxSupported("ps_2_0"))))
				mShaderGen = OGRE_NEW ShaderHelperHLSL();
			else if (hmgr.isLanguageSupported("glsl"))
				mShaderGen = OGRE_NEW ShaderHelperGLSL();
			//else if (hmgr.isLanguageSupported("glsles"))
			//	mShaderGen = OGRE_NEW ShaderHelperGLSLES();
			else
			{
				// todo
			}

			// check SM3 features
			mSM3Available = GpuProgramManager::getSingleton().isSyntaxSupported("ps_3_0");
			mSM4Available = GpuProgramManager::getSingleton().isSyntaxSupported("ps_4_0");

		}

		// Unfortunately this doesn't work :(
		/*
		// Default implementation
		TerrainMaterialGeneratorA::SM2Profile::addTechnique(mat, terrain, tt);
		*/

		// So we have to replicate the entire method:
		Technique* tech = mat->createTechnique();

		// Only supporting one pass
		Pass* pass = tech->createPass();

		// Doesn't delegate to the proper method otherwise
		HighLevelGpuProgramPtr vprog = ((TerrainRTSSMaterialGenerator::SM2Profile::ShaderHelperCg*)mShaderGen)->generateVertexProgram(this, terrain, tt);
		HighLevelGpuProgramPtr fprog = mShaderGen->generateFragmentProgram(this, terrain, tt);

		pass->setVertexProgram(vprog->getName());
		pass->setFragmentProgram(fprog->getName());

		if (tt == HIGH_LOD || tt == /*RENDER_COMPOSITE_MAP*/ 2)
		{
			// global normal map
			TextureUnitState* tu = pass->createTextureUnitState();
			tu->setTextureName(terrain->getTerrainNormalMap()->getName());
			tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

			// global colour map
			if (terrain->getGlobalColourMapEnabled() && isGlobalColourMapEnabled())
			{
				tu = pass->createTextureUnitState(terrain->getGlobalColourMap()->getName());
				tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}

			// light map
			if (isLightmapEnabled())
			{
				tu = pass->createTextureUnitState(terrain->getLightmap()->getName());
				tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}

			// blend maps
			uint maxLayers = getMaxLayers(terrain);
			uint numBlendTextures = std::min(terrain->getBlendTextureCount(maxLayers), terrain->getBlendTextureCount());
			uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));
			for (uint i = 0; i < numBlendTextures; ++i)
			{
				tu = pass->createTextureUnitState(terrain->getBlendTextureName(i));
				tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}

			// layer textures
			for (uint i = 0; i < numLayers; ++i)
			{
				// diffuse / specular
				pass->createTextureUnitState(terrain->getLayerTextureName(i, 0));
				// normal / height
				pass->createTextureUnitState(terrain->getLayerTextureName(i, 1));
			}

		}
		else
		{
			// /*LOW_LOD*/ 1 textures
			// composite map
			TextureUnitState* tu = pass->createTextureUnitState();
			tu->setTextureName(terrain->getCompositeMap()->getName());
			tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

			// That's it!

		}

		// Add shadow textures (always at the end)
		if (isShadowingEnabled(tt, terrain))
		{
			uint numTextures = 1;
			if (getReceiveDynamicShadowsPSSM())
			{
				numTextures = getReceiveDynamicShadowsPSSM()->getSplitCount();
			}
			for (uint i = 0; i < numTextures; ++i)
			{
				TextureUnitState* tu = pass->createTextureUnitState();
				tu->setContentType(TextureUnitState::CONTENT_SHADOW);
				tu->setTextureAddressingMode(TextureUnitState::TAM_BORDER);
				tu->setTextureBorderColour(ColourValue::White);
			}
		}
	}

	/**
	* generate() and generateForCompositeMap() are identical to TerrainMaterialGeneratorA implementation,
	* the only reason for repeating them is that, unfortunately, addTechnique() is not declared virtual.
	**/
	//---------------------------------------------------------------------
	MaterialPtr TerrainRTSSMaterialGenerator::CustomSM2Profile::generate(const Terrain* terrain)
	{
		// re-use old material if exists
		MaterialPtr mat = terrain->_getMaterial();
		if (mat.isNull())
		{
			MaterialManager& matMgr = MaterialManager::getSingleton();

			// it's important that the names are deterministic for a given terrain, so
			// use the terrain pointer as an ID
			const String& matName = terrain->getMaterialName();
			mat = matMgr.getByName(matName);
			if (mat.isNull())
			{
				mat = matMgr.create(matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}
		}
		// clear everything
		mat->removeAllTechniques();

		// Automatically disable normal & parallax mapping if card cannot handle it
		// We do this rather than having a specific technique for it since it's simpler
		GpuProgramManager& gmgr = GpuProgramManager::getSingleton();
		if (!gmgr.isSyntaxSupported("ps_4_0") && !gmgr.isSyntaxSupported("ps_3_0") && !gmgr.isSyntaxSupported("ps_2_x")
			&& !gmgr.isSyntaxSupported("fp40") && !gmgr.isSyntaxSupported("arbfp1"))
		{
			setLayerNormalMappingEnabled(false);
			setLayerParallaxMappingEnabled(false);
		}

		addTechnique(mat, terrain, HIGH_LOD);

		// LOD
		if(mCompositeMapEnabled)
		{
			addTechnique(mat, terrain, /*LOW_LOD*/ (TechniqueType)1);
			Material::LodValueList lodValues;
			lodValues.push_back(TerrainGlobalOptions::getSingleton().getCompositeMapDistance());
			mat->setLodLevels(lodValues);
			Technique* lowLodTechnique = mat->getTechnique(1);
			lowLodTechnique->setLodIndex(1);
		}

		updateParams(mat, terrain);

		return mat;

	}
	//---------------------------------------------------------------------
	MaterialPtr TerrainRTSSMaterialGenerator::CustomSM2Profile::generateForCompositeMap(const Terrain* terrain)
	{
		// re-use old material if exists
		MaterialPtr mat = terrain->_getCompositeMapMaterial();
		if (mat.isNull())
		{
			MaterialManager& matMgr = MaterialManager::getSingleton();

			// it's important that the names are deterministic for a given terrain, so
			// use the terrain pointer as an ID
			const String& matName = terrain->getMaterialName() + "/comp";
			mat = matMgr.getByName(matName);
			if (mat.isNull())
			{
				mat = matMgr.create(matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}
		}
		// clear everything
		mat->removeAllTechniques();

		addTechnique(mat, terrain, /*RENDER_COMPOSITE_MAP*/ (TechniqueType)2);

		updateParamsForCompositeMap(mat, terrain);

		return mat;

	}
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	void TerrainRTSSMaterialGenerator::CustomSM2Profile::CustomShaderHelperCg::defaultVpParams(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const HighLevelGpuProgramPtr& prog)
	{
		GpuProgramParametersSharedPtr params = prog->getDefaultParameters();
		params->setIgnoreMissingParams(true);
		params->setNamedAutoConstant("worldMatrix", GpuProgramParameters::ACT_WORLD_MATRIX);
		params->setNamedAutoConstant("viewProjMatrix", GpuProgramParameters::ACT_VIEWPROJ_MATRIX);
		params->setNamedAutoConstant("lodMorph", GpuProgramParameters::ACT_CUSTOM,
			Terrain::LOD_MORPH_CUSTOM_PARAM);
		params->setNamedAutoConstant("fogParams", GpuProgramParameters::ACT_FOG_PARAMS);

		//if (prof->isShadowingEnabled(tt, terrain))
		{
			uint numTextures = 1;
			if (prof->getReceiveDynamicShadowsPSSM())
			{
				numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
			}
			for (uint i = 0; i < numTextures; ++i)
			{
				params->setNamedAutoConstant("texViewProjMatrix" + StringConverter::toString(i),
					GpuProgramParameters::ACT_TEXTURE_VIEWPROJ_MATRIX, i);
				// Don't add depth range params
				/*
				if (prof->getReceiveDynamicShadowsDepth())
				{
				params->setNamedAutoConstant("depthRange" + StringConverter::toString(i),
				GpuProgramParameters::ACT_SHADOW_SCENE_DEPTH_RANGE, i);
				}
				*/
			}
		}

		if (terrain->_getUseVertexCompression() && tt != /*RENDER_COMPOSITE_MAP*/ 2)
		{
			Matrix4 posIndexToObjectSpace;
			terrain->getPointTransform(&posIndexToObjectSpace);
			params->setNamedConstant("posIndexToObjectSpace", posIndexToObjectSpace);
		}
	}
	//---------------------------------------------------------------------
	void TerrainRTSSMaterialGenerator::CustomSM2Profile::CustomShaderHelperCg::generateVpDynamicShadows(const SM2Profile *prof, const Terrain *terrain, TechniqueType tt, StringUtil::StrStreamType &outStream)
	{
		uint numTextures = 1;
		if (prof->getReceiveDynamicShadowsPSSM())
		{
			numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
		}

		// Calculate the position of vertex in light space
		for (uint i = 0; i < numTextures; ++i)
		{
			outStream <<
				"   oLightSpacePos" << i << " = mul(texViewProjMatrix" << i << ", worldPos); \n";

			// Don't linearize depth range: RTSS PSSM implementation uses view-space depth
			/*
			if (prof->getReceiveDynamicShadowsDepth())
			{
			// make linear
			outStream <<
			"oLightSpacePos" << i << ".z = (oLightSpacePos" << i << ".z - depthRange" << i << ".x) * depthRange" << i << ".w;\n";

			}
			*/
		}


		if (prof->getReceiveDynamicShadowsPSSM())
		{
			outStream <<
				"   // pass cam depth\n"
				"   oUVMisc.z = oPos.z;\n";
		}
	}
	//---------------------------------------------------------------------
	uint TerrainRTSSMaterialGenerator::CustomSM2Profile::CustomShaderHelperCg::generateVpDynamicShadowsParams(uint texCoord, const SM2Profile *prof, const Terrain *terrain, TechniqueType tt, StringUtil::StrStreamType &outStream)
	{
		// out semantics & params
		uint numTextures = 1;
		if (prof->getReceiveDynamicShadowsPSSM())
		{
			numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
		}
		for (uint i = 0; i < numTextures; ++i)
		{
			outStream <<
				", out float4 oLightSpacePos" << i << " : TEXCOORD" << texCoord++ << " \n" <<
				", uniform float4x4 texViewProjMatrix" << i << " \n";

			// Don't add depth range params
			/*
			if (prof->getReceiveDynamicShadowsDepth())
			{
			outStream <<
			", uniform float4 depthRange" << i << " // x = min, y = max, z = range, w = 1/range \n";
			}
			*/
		}

		return texCoord;
	}
	//---------------------------------------------------------------------

	/**
	* This method is identical to TerrainMaterialGeneratorA::SM2Profile::ShaderHelperCg::generateVpHeader()
	* but is needed because generateVpDynamicShadowsParams() is not declared virtual.
	**/
	void TerrainRTSSMaterialGenerator::CustomSM2Profile::CustomShaderHelperCg::generateVpHeader(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		outStream <<
			"void main_vp(\n";
		bool compression = terrain->_getUseVertexCompression() && tt != /*RENDER_COMPOSITE_MAP*/ 2;
		if (compression)
		{
			outStream <<
				"float2 posIndex : POSITION,\n"
				"float height  : TEXCOORD0,\n";
		}
		else
		{
			outStream <<
				"float4 pos : POSITION,\n"
				"float2 uv  : TEXCOORD0,\n";

		}
		if (tt != /*RENDER_COMPOSITE_MAP*/ 2)
			outStream << "float2 delta  : TEXCOORD1,\n"; // lodDelta, lodThreshold

		outStream <<
			"uniform float4x4 worldMatrix,\n"
			"uniform float4x4 viewProjMatrix,\n"
			"uniform float2   lodMorph,\n"; // morph amount, morph LOD target

		if (compression)
		{
			outStream <<
				"uniform float4x4   posIndexToObjectSpace,\n"
				"uniform float    baseUVScale,\n";
		}
		// uv multipliers
		uint maxLayers = prof->getMaxLayers(terrain);
		uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));
		uint numUVMultipliers = (numLayers / 4);
		if (numLayers % 4)
			++numUVMultipliers;
		for (uint i = 0; i < numUVMultipliers; ++i)
			outStream << "uniform float4 uvMul_" << i << ", \n";

		outStream <<
			"out float4 oPos : POSITION,\n"
			"out float4 oPosObj : TEXCOORD0 \n";

		uint texCoordSet = 1;
		outStream <<
			", out float4 oUVMisc : TEXCOORD" << texCoordSet++ <<" // xy = uv, z = camDepth\n";

		// layer UV's premultiplied, packed as xy/zw
		uint numUVSets = numLayers / 2;
		if (numLayers % 2)
			++numUVSets;
		if (tt != /*LOW_LOD*/ 1)
		{
			for (uint i = 0; i < numUVSets; ++i)
			{
				outStream <<
					", out float4 oUV" << i << " : TEXCOORD" << texCoordSet++ << "\n";
			}
		}

		if (prof->getParent()->getDebugLevel() && tt != /*RENDER_COMPOSITE_MAP*/ 2)
		{
			outStream << ", out float2 lodInfo : TEXCOORD" << texCoordSet++ << "\n";
		}

		bool fog = terrain->getSceneManager()->getFogMode() != FOG_NONE && tt != /*RENDER_COMPOSITE_MAP*/ 2;
		if (fog)
		{
			outStream <<
				", uniform float4 fogParams\n"
				", out float fogVal : COLOR\n";
		}

		//if (prof->isShadowingEnabled(tt, terrain))
		{
			texCoordSet = generateVpDynamicShadowsParams(texCoordSet, prof, terrain, tt, outStream);
		}

		// check we haven't exceeded texture coordinates
		if (texCoordSet > 8)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Requested options require too many texture coordinate sets! Try reducing the number of layers.",
				__FUNCTION__);
		}

		outStream <<
			")\n"
			"{\n";
		if (compression)
		{
			outStream <<
				"   float4 pos;\n"
				"   pos = mul(posIndexToObjectSpace, float4(posIndex, height, 1));\n"
				"   float2 uv = float2(posIndex.x * baseUVScale, 1.0 - (posIndex.y * baseUVScale));\n";
		}
		outStream <<
			"   float4 worldPos = mul(worldMatrix, pos);\n"
			"   oPosObj = pos;\n";

		if (tt != /*RENDER_COMPOSITE_MAP*/ 2)
		{
			// determine whether to apply the LOD morph to this vertex
			// we store the deltas against all vertices so we only want to apply
			// the morph to the ones which would disappear. The target LOD which is
			// being morphed to is stored in lodMorph.y, and the LOD at which
			// the vertex should be morphed is stored in uv.w. If we subtract
			// the former from the latter, and arrange to only morph if the
			// result is negative (it will only be -1 in fact, since after that
			// the vertex will never be indexed), we will achieve our aim.
			// sign(vertexLOD - targetLOD) == -1 is to morph
			outStream <<
				"   float toMorph = -min(0, sign(delta.y - lodMorph.y));\n";
			// this will either be 1 (morph) or 0 (don't morph)
			if (prof->getParent()->getDebugLevel())
			{
				// x == LOD level (-1 since value is target level, we want to display actual)
				outStream << "lodInfo.x = (lodMorph.y - 1) / " << terrain->getNumLodLevels() << ";\n";
				// y == LOD morph
				outStream << "lodInfo.y = toMorph * lodMorph.x;\n";
			}

			// morph
			switch (terrain->getAlignment())
			{
			case Terrain::ALIGN_X_Y:
				outStream << "   worldPos.z += delta.x * toMorph * lodMorph.x;\n";
				break;
			case Terrain::ALIGN_X_Z:
				outStream << "   worldPos.y += delta.x * toMorph * lodMorph.x;\n";
				break;
			case Terrain::ALIGN_Y_Z:
				outStream << "   worldPos.x += delta.x * toMorph * lodMorph.x;\n";
				break;
			};
		}


		// generate UVs
		if (tt != /*LOW_LOD*/ 1)
		{
			for (uint i = 0; i < numUVSets; ++i)
			{
				uint layer  =  i * 2;
				uint uvMulIdx = layer / 4;

				outStream <<
					"   oUV" << i << ".xy = " << " uv.xy * uvMul_" << uvMulIdx << "." << getChannel(layer) << ";\n";
				outStream <<
					"   oUV" << i << ".zw = " << " uv.xy * uvMul_" << uvMulIdx << "." << getChannel(layer+1) << ";\n";

			}

		}


	}
	//---------------------------------------------------------------------

	/**
	* This method is identical to TerrainMaterialGeneratorA::SM2Profile::ShaderHelperCg::generateVpFooter()
	* but is needed because generateVpDynamicShadows() is not declared virtual.
	**/
	void TerrainRTSSMaterialGenerator::CustomSM2Profile::CustomShaderHelperCg::generateVpFooter(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{

		outStream <<
			"   oPos = mul(viewProjMatrix, worldPos);\n"
			"   oUVMisc.xy = uv.xy;\n";

		bool fog = terrain->getSceneManager()->getFogMode() != FOG_NONE && tt != /*RENDER_COMPOSITE_MAP*/ 2;
		if (fog)
		{
			if (terrain->getSceneManager()->getFogMode() == FOG_LINEAR)
			{
				outStream <<
					"   fogVal = saturate((oPos.z - fogParams.y) * fogParams.w);\n";
			}
			else
			{
				outStream <<
					"   fogVal = 1 - saturate(1 / (exp(oPos.z * fogParams.x)));\n";
			}
		}

		//if (prof->isShadowingEnabled(tt, terrain))
			generateVpDynamicShadows(prof, terrain, tt, outStream);

		outStream <<
			"}\n";


	}
	//---------------------------------------------------------------------
	void TerrainRTSSMaterialGenerator::CustomSM2Profile::CustomShaderHelperCg::generateVertexProgramSource(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		generateVpHeader(prof, terrain, tt, outStream);

		if (tt != /*LOW_LOD*/ 1)
		{
			uint maxLayers = prof->getMaxLayers(terrain);
			uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));

			for (uint i = 0; i < numLayers; ++i)
				generateVpLayer(prof, terrain, tt, i, outStream);
		}

		generateVpFooter(prof, terrain, tt, outStream);

	}
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr
		TerrainRTSSMaterialGenerator::CustomSM2Profile::CustomShaderHelperCg::generateVertexProgram(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramPtr ret = createVertexProgram(prof, terrain, tt);

		StringUtil::StrStreamType sourceStr;
		generateVertexProgramSource(prof, terrain, tt, sourceStr);
		ret->setSource(sourceStr.str());
		ret->load();
		defaultVpParams(prof, terrain, tt, ret);
#if OGRE_DEBUG_MODE
		LogManager::getSingleton().stream(LML_TRIVIAL) << "*** Terrain Vertex Program: "
			<< ret->getName() << " ***\n" << ret->getSource() << "\n***   ***";
#endif

		return ret;

	}
	//---------------------------------------------------------------------


}
