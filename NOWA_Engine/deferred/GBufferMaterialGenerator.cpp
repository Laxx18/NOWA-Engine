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
#include "GBufferMaterialGenerator.h"

#include "OgreMaterialManager.h"
#include "OgreGpuProgramManager.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreTechnique.h"
#include "OgreLogManager.h"
#include <iostream>

using namespace Ogre;

//Use this directive to control whether you are writing projective (regular) or linear depth.
#define WRITE_LINEAR_DEPTH

//This is the concrete implementation of the material generator.
class GBufferMaterialGeneratorImpl : public MaterialGenerator::Impl
{
public:
    GBufferMaterialGeneratorImpl(const String& baseName) : 
      mBaseName(baseName)
      {
          mIsSm4 = GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0_level_9_1");
          mIsGLSL = (GpuProgramManager::getSingleton().isSyntaxSupported("glsl") || GpuProgramManager::getSingleton().isSyntaxSupported("glsles")) &&
                    !(GpuProgramManager::getSingleton().isSyntaxSupported("vs_1_1") || GpuProgramManager::getSingleton().isSyntaxSupported("arbvp1"));
        		  ;

		  /*mIsGLSL = (GpuProgramManager::getSingleton().isSyntaxSupported("glsl") || GpuProgramManager::getSingleton().isSyntaxSupported("glsles"))
			  && !(GpuProgramManager::getSingleton().isSyntaxSupported("vs_5_0") || GpuProgramManager::getSingleton().isSyntaxSupported("vp40"))
				  ;*/

      }
    
protected:
    String mBaseName;
    bool mIsSm4;
    bool mIsGLSL;
    virtual GpuProgramPtr generateVertexShader(MaterialGenerator::Perm permutation);
    virtual GpuProgramPtr generateFragmentShader(MaterialGenerator::Perm permutation);
    virtual MaterialPtr generateTemplateMaterial(MaterialGenerator::Perm permutation);

};

GBufferMaterialGenerator::GBufferMaterialGenerator() {
    vsMask = VS_MASK;
    fsMask = FS_MASK;
    matMask = MAT_MASK;
    materialBaseName = "DeferredShading/GBuffer/";
    mImpl = new GBufferMaterialGeneratorImpl(materialBaseName);
}

GpuProgramPtr GBufferMaterialGeneratorImpl::generateVertexShader(MaterialGenerator::Perm permutation)
{
    std::stringstream ss;

    if(mIsGLSL)
    {
        int shadingLangVersion = Root::getSingleton().getRenderSystem()->getNativeShadingLanguageVersion();
        const char *inSemantic = shadingLangVersion >= 150 ? "in" : "attribute";
        const char *outSemantic = shadingLangVersion >= 150 ? "out" : "varying";

        if(GpuProgramManager::getSingleton().isSyntaxSupported("glsles"))
        {
            ss << "#version 300 es" << std::endl;
            ss << "precision mediump int;" << std::endl;
            ss << "precision mediump float;" << std::endl;
        }
        else
            ss << "#version " << shadingLangVersion << std::endl;

        ss << inSemantic << " vec4 vertex;" << std::endl;
        ss << inSemantic << " vec3 normal;" << std::endl;

        uint32 numTexCoords = (permutation & GBufferMaterialGenerator::GBP_TEXCOORD_MASK) >> 8;
        for (uint32 i = 0; i < numTexCoords; i++)
        {
            ss << inSemantic << " vec2 uv" << i << ';' << std::endl;
        }

        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            ss << inSemantic << " vec3 tangent;" << std::endl;
        }

        //TODO : Skinning inputs
        ss << std::endl;

#ifdef WRITE_LINEAR_DEPTH
        ss << outSemantic << " vec3 oViewPos;" << std::endl;
#else
        ss << outSemantic << " float oDepth;" << std::endl;
#endif
        ss << outSemantic << " vec3 oNormal;" << std::endl;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            ss << outSemantic << " vec3 oTangent;" << std::endl;
            ss << outSemantic << " vec3 oBiNormal;" << std::endl;
        }
        for (uint32 i = 0; i < numTexCoords; i++)
        {
            ss << outSemantic << " vec2 oUv" << i << ";" << std::endl;
        }

        ss << std::endl;

        ss << "uniform mat4 cWorldViewProj;" << std::endl;
        ss << "uniform mat4 cWorldView;" << std::endl;

        ss << "void main()" << std::endl;

        ss << "{" << std::endl;
        ss << " gl_Position = cWorldViewProj * vertex;" << std::endl;
        ss << " oNormal = (cWorldView * vec4(normal,0)).xyz;" << std::endl;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            ss << " oTangent = (cWorldView * vec4(tangent,0)).xyz;" << std::endl;
            ss << " oBiNormal = cross(oNormal, oTangent);" << std::endl;
        }

#ifdef WRITE_LINEAR_DEPTH
        ss << " oViewPos = (cWorldView * vertex).xyz;" << std::endl;
#else
        ss << " oDepth = gl_Position.w;" << std::endl;
#endif

        for (uint32 i=0; i<numTexCoords; i++) {
            ss << " oUv" << i << " = uv" << i << ';' << std::endl;
        }

        ss << "}" << std::endl;

        String programSource = ss.str();
        String programName = mBaseName + "VP_" + StringConverter::toString(permutation);

#if OGRE_DEBUG_MODE
        LogManager::getSingleton().getDefaultLog()->logMessage(programSource);
#endif

        // Create shader object
        HighLevelGpuProgramPtr ptrProgram;
        if(GpuProgramManager::getSingleton().isSyntaxSupported("glsles"))
        {
            ptrProgram = HighLevelGpuProgramManager::getSingleton().createProgram(programName,
                                                                                  ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                                  "glsles", GPT_VERTEX_PROGRAM);
            ptrProgram->setParameter("syntax", "glsles");
        }
        else
        {
            ptrProgram = HighLevelGpuProgramManager::getSingleton().createProgram(programName,
                                                                                  ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                                  "glsl", GPT_VERTEX_PROGRAM);
        }
        ptrProgram->setSource(programSource);

        const GpuProgramParametersSharedPtr& params = ptrProgram->getDefaultParameters();
        params->setNamedAutoConstant("cWorldViewProj", GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
        params->setNamedAutoConstant("cWorldView", GpuProgramParameters::ACT_WORLDVIEW_MATRIX);

        ptrProgram->load();
        
        return GpuProgramPtr(ptrProgram);
    }
    else
    {
        ss << "void ToGBufferVP(" << std::endl;
        ss << " float4 iPosition : POSITION," << std::endl;
        ss << " float3 iNormal   : NORMAL," << std::endl;

        uint32 numTexCoords = (permutation & GBufferMaterialGenerator::GBP_TEXCOORD_MASK) >> 8;
        for (uint32 i=0; i<numTexCoords; i++) 
        {
            ss << " float2 iUV" << i << " : TEXCOORD" << i << ',' << std::endl;
        }

        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            if(mIsSm4)
            {
                ss << " float3 iTangent : TANGENT," << std::endl;
            }
            else
            {
                ss << " float3 iTangent : TANGENT0," << std::endl;
            }
        }

        //TODO : Skinning inputs
        ss << std::endl;
        


        ss << " out float4 oPosition : " ;
        if(mIsSm4)
        {
            ss << "SV_";
        }
        ss << "POSITION," << std::endl;
    #ifdef WRITE_LINEAR_DEPTH
        ss << " out float3 oViewPos : TEXCOORD0," << std::endl;
    #else
        ss << " out float oDepth : TEXCOORD0," << std::endl;
    #endif
        ss << " out float3 oNormal : TEXCOORD1," << std::endl;

        int texCoordNum = 2;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP) 
        {
            ss << " out float3 oTangent : TEXCOORD" << texCoordNum++ << ',' << std::endl;
            ss << " out float3 oBiNormal : TEXCOORD" << texCoordNum++ << ',' << std::endl;
        }
        for (uint32 i=0; i<numTexCoords; i++) 
        {
            ss << " out float2 oUV" << i << " : TEXCOORD" << texCoordNum++ << ',' << std::endl;
        }

        ss << std::endl;
        ss << " uniform float4x4 cWorldViewProj," << std::endl;
       if(numTexCoords)
       	ss << " uniform float4x4 cTextureMatrix," << std::endl;
        ss << " uniform float4x4 cWorldView" << std::endl;


        ss << " )" << std::endl;
        
        
        ss << "{" << std::endl;
        ss << " oPosition = mul(cWorldViewProj, iPosition);" << std::endl;
        ss << " oNormal = mul(cWorldView, float4(iNormal,0)).xyz;" << std::endl;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            ss << " oTangent = mul(cWorldView, float4(iTangent,0)).xyz;" << std::endl;
            ss << " oBiNormal = cross(oNormal, oTangent);" << std::endl;
        }

    #ifdef WRITE_LINEAR_DEPTH
        ss << " oViewPos = mul(cWorldView, iPosition).xyz;" << std::endl;
    #else
        ss << " oDepth = oPosition.w;" << std::endl;
    #endif

        for (uint32 i=0; i<numTexCoords; i++) {
          ss << " oUV" << i << " = mul(cTextureMatrix,float4(iUV"<<i<<",0,1)).xy" << ';' << std::endl;

        	//ss << " oUV" << i << " =cTextureMatrix" << ';' << std::endl;
        	//ss << " oUV" << i << " =iUV" << i << ';' << std::endl;
        }

        ss << "}" << std::endl;
        
        String programSource = ss.str();
        String programName = mBaseName + "VP_" + StringConverter::toString(permutation);

    #if OGRE_DEBUG_MODE
        LogManager::getSingleton().getDefaultLog()->logMessage(programSource);
    #endif

        // Create shader object
        HighLevelGpuProgramPtr ptrProgram = HighLevelGpuProgramManager::getSingleton().createProgram(
            programName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "cg", GPT_VERTEX_PROGRAM);
        ptrProgram->setSource(programSource);
        ptrProgram->setParameter("entry_point","ToGBufferVP");
		if (mIsSm4)
		{
			ptrProgram->setParameter("profiles", "vs_4_0");
		}
		else
		{
			ptrProgram->setParameter("profiles", "vs_1_1 arbvp1");
		}

		/*if (mIsSm4)
		{
			ptrProgram->setParameter("profiles", "vs_4_0");
		}
		else
		{
			ptrProgram->setParameter("profiles", "vs_5_0 vp40");
		}*/

        const GpuProgramParametersSharedPtr& params = ptrProgram->getDefaultParameters();

        params->setNamedAutoConstant("cWorldViewProj", GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
        params->setNamedAutoConstant("cWorldView", GpuProgramParameters::ACT_WORLDVIEW_MATRIX);
        if(numTexCoords)
        	params->setNamedAutoConstant("cTextureMatrix", GpuProgramParameters::ACT_TEXTURE_MATRIX);

        ptrProgram->load();

        return GpuProgramPtr(ptrProgram);
    }
}

GpuProgramPtr GBufferMaterialGeneratorImpl::generateFragmentShader(MaterialGenerator::Perm permutation)
{
    std::stringstream ss;

    if(mIsGLSL)
    {
        int shadingLangVersion = Root::getSingleton().getRenderSystem()->getNativeShadingLanguageVersion();
        const char *inSemantic = shadingLangVersion >= 150 ? "in" : "varying";
        const char *outData = shadingLangVersion >= 150 ? "fragData" : "gl_FragData";
        const char *textureFunc = shadingLangVersion >= 150 ? "texture" : "texture2D";
        if(GpuProgramManager::getSingleton().isSyntaxSupported("glsles"))
        {
            ss << "#version 300 es" << std::endl;
            ss << "precision mediump int;" << std::endl;
            ss << "precision mediump float;" << std::endl;
        }
        else
        {
            ss << "#version " << shadingLangVersion << std::endl;
            if(shadingLangVersion >= 150)
            {
                ss << "out vec4 fragData[2];";
            }
        }
#ifdef WRITE_LINEAR_DEPTH
        ss << inSemantic << " vec3 oViewPos;" << std::endl;
#else
        ss << inSemantic << " float oDepth;" << std::endl;
#endif
        ss << inSemantic << " vec3 oNormal;" << std::endl;

        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            ss << inSemantic << " vec3 oTangent;" << std::endl;
            ss << inSemantic << " vec3 oBiNormal;" << std::endl;
        }

        uint32 numTexCoords = (permutation & GBufferMaterialGenerator::GBP_TEXCOORD_MASK) >> 8;
        for (uint32 i = 0; i < numTexCoords; i++)
        {
            ss << inSemantic << " vec2 oUv" << i << ';' << std::endl;
        }

        ss << std::endl;

        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            ss << "uniform sampler2D sNormalMap;" << std::endl;
        }

        uint32 numTextures = permutation & GBufferMaterialGenerator::GBP_TEXTURE_MASK;
        for (uint32 i = 0; i < numTextures; i++)
        {
            ss << "uniform sampler2D sTex" << i << ";" << std::endl;
        }
        if (numTextures == 0 || permutation & GBufferMaterialGenerator::GBP_HAS_DIFFUSE_COLOUR)
        {
            ss << "uniform vec4 cDiffuseColour;" << std::endl;
        }

#ifdef WRITE_LINEAR_DEPTH
        ss << "uniform float cFarDistance;" << std::endl;
#endif

        ss << "uniform float cSpecularity;" << std::endl;

        ss << "void main()" << std::endl;
        ss << "{" << std::endl;

        if (numTexCoords > 0 && numTextures > 0)
        {
            ss << outData << "[0].rgb = " << textureFunc << "(sTex0, oUv0).rgb;" << std::endl;
            if (permutation & GBufferMaterialGenerator::GBP_HAS_DIFFUSE_COLOUR)
            {
                ss << outData << "[0].rgb *= cDiffuseColour.rgb;" << std::endl;
            }
        }
        else
        {
            ss << outData << "[0].rgb = cDiffuseColour.rgb;" << std::endl;
        }

        ss << outData << "[0].a = cSpecularity;" << std::endl;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            ss << " vec3 texNormal = (" << textureFunc << "(sNormalMap, oUv0).rgb-0.5)*2.0;" << std::endl;
            ss << " mat3 normalRotation = mat3(oTangent, oBiNormal, oNormal);" << std::endl;
            ss << outData << "[1].rgb = normalize(texNormal * normalRotation);" << std::endl;
        }
        else
        {
            ss << outData << "[1].rgb = normalize(oNormal);" << std::endl;
        }
        ss << outData << "[1].rgb = normalize(oNormal);" << std::endl;
#ifdef WRITE_LINEAR_DEPTH
        ss << outData << "[1].a = length(oViewPos) / cFarDistance;" << std::endl;
#else
        ss << outData << "[1].a = oDepth;" << std::endl;
#endif

        ss << "}" << std::endl;

        String programSource = ss.str();
        String programName = mBaseName + "FP_" + StringConverter::toString(permutation);

#if OGRE_DEBUG_MODE
        LogManager::getSingleton().getDefaultLog()->logMessage(programSource);
#endif

        // Create shader object
        HighLevelGpuProgramPtr ptrProgram;
        if(GpuProgramManager::getSingleton().isSyntaxSupported("glsles"))
        {
            ptrProgram = HighLevelGpuProgramManager::getSingleton().createProgram(programName,
                                                                                  ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                                  "glsles", GPT_FRAGMENT_PROGRAM);
            ptrProgram->setParameter("syntax", "glsles");
        }
        else
        {
            ptrProgram = HighLevelGpuProgramManager::getSingleton().createProgram(programName,
                                                                                  ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                                  "glsl", GPT_FRAGMENT_PROGRAM);
        }
        ptrProgram->setSource(programSource);

        const GpuProgramParametersSharedPtr& params = ptrProgram->getDefaultParameters();
        params->setNamedAutoConstant("cSpecularity", GpuProgramParameters::ACT_SURFACE_SHININESS);
        if (numTextures == 0 || permutation & GBufferMaterialGenerator::GBP_HAS_DIFFUSE_COLOUR)
        {
            params->setNamedAutoConstant("cDiffuseColour", GpuProgramParameters::ACT_SURFACE_DIFFUSE_COLOUR);
        }

        // Bind samplers
        int samplerNum = 0;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            params->setNamedConstant("sNormalMap", samplerNum++);
        }
        for (uint32 i = 0; i < numTextures; i++, samplerNum++)
        {
            params->setNamedConstant("sTex" + StringConverter::toString(i), samplerNum);
        }

#ifdef WRITE_LINEAR_DEPTH
        //TODO : Should this be the distance to the far corner, not the far clip distance?
        params->setNamedAutoConstant("cFarDistance", GpuProgramParameters::ACT_FAR_CLIP_DISTANCE);
#endif
        
        ptrProgram->load();
        return GpuProgramPtr(ptrProgram);
    }
    else
    {
        ss << "void ToGBufferFP(" << std::endl;
        if(mIsSm4)
        {
            ss << "float4 oPosition : SV_POSITION," << std::endl;
        }

    #ifdef WRITE_LINEAR_DEPTH
        ss << " float3 iViewPos : TEXCOORD0," << std::endl;
    #else
        ss << " float1 iDepth : TEXCOORD0," << std::endl;
    #endif
        ss << " float3 iNormal   : TEXCOORD1," << std::endl;

        int texCoordNum = 2;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP) 
        {
            ss << " float3 iTangent : TEXCOORD" << texCoordNum++ << ',' << std::endl;
            ss << " float3 iBiNormal : TEXCOORD" << texCoordNum++ << ',' << std::endl;
        }

        uint32 numTexCoords = (permutation & GBufferMaterialGenerator::GBP_TEXCOORD_MASK) >> 8;
        for (uint32 i=0; i<numTexCoords; i++)
        {
            ss << " float2 iUV" << i << " : TEXCOORD" << texCoordNum++ << ',' << std::endl;
        }

        ss << std::endl;

        ss << " out float4 oColor0 : COLOR0," << std::endl;
        ss << " out float4 oColor1 : COLOR1," << std::endl;
        ss << " out float4 oColor2 : COLOR2," << std::endl;
        ss << " out float4 oColor3 : COLOR3," << std::endl;
        ss << std::endl;

        int samplerNum = 0;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
        {
            if(mIsSm4)
            {
                ss << " uniform sampler2D sNormalMap : register(s" << samplerNum++ << ")," << std::endl;
            }
            else
            {
                ss << " uniform sampler sNormalMap : register(s" << samplerNum++ << ")," << std::endl;
            }
        }
        if (permutation & GBufferMaterialGenerator::GBP_SPECULAR_MAP)
        {
            if(mIsSm4)
            {
                ss << " uniform sampler2D sSpecularMap : register(s" << samplerNum++ << ")," << std::endl;
            }
            else
            {
                ss << " uniform sampler sSpecularMap : register(s" << samplerNum++ << ")," << std::endl;
            }
        }
        if (permutation & GBufferMaterialGenerator::GBP_GLOW_MAP)
        {
            if(mIsSm4)
            {
                ss << " uniform sampler2D sGlowMap : register(s" << samplerNum++ << ")," << std::endl;
            }
            else
            {
                ss << " uniform sampler sGlowMap : register(s" << samplerNum++ << ")," << std::endl;
            }
        }
        if (permutation & GBufferMaterialGenerator::GBP_CUBE_MAP)
        {

                ss << " uniform samplerCUBE sCubeMap : register(s" << samplerNum++ << ")," << std::endl;


        }
        uint32 numTextures = permutation & GBufferMaterialGenerator::GBP_TEXTURE_MASK;
        for (uint32 i=0; i<numTextures; i++) {
            if(mIsSm4)
            {
                ss << " uniform sampler2D sTex" << i << " : register(s" << samplerNum++ << ")," << std::endl;
            }
            else
            {
                ss << " uniform sampler sTex" << i << " : register(s" << samplerNum++ << ")," << std::endl;
            }
        }
        if (numTextures == 0 || permutation & GBufferMaterialGenerator::GBP_HAS_DIFFUSE_COLOUR)
        {
            ss << " uniform float4 cDiffuseColour," << std::endl;
        }

    #ifdef WRITE_LINEAR_DEPTH
        ss << " uniform float cFarDistance," << std::endl;
    #endif
        
        ss << " uniform float cSpecularity" << std::endl;

        ss << " )" << std::endl;
        
        
        ss << "{" << std::endl;

        if (numTexCoords > 0 && numTextures > 0) 
        {
        	if(permutation&GBufferMaterialGenerator::GBP_HAS_ALPHA){
        		ss << "if(tex2D(sTex0, iUV0).a<0.5){clip(-1);}" << std::endl;
        	}
            ss << " oColor0.rgb = tex2D(sTex0, iUV0).rgb;" << std::endl;
            if (permutation & GBufferMaterialGenerator::GBP_HAS_DIFFUSE_COLOUR)
            {
                ss << " oColor0.rgb *= cDiffuseColour.rgb;" << std::endl;
            }
        }
        else
        {
            ss << " oColor0.rgb = cDiffuseColour.rgb;" << std::endl;
        }

        
        ss << " oColor0.a = cSpecularity;" << std::endl;
        if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP) 
        {
            ss << " float3 texNormal = (tex2D(sNormalMap, iUV0).rgb-0.5)*2;" << std::endl;
            ss << " float3x3 normalRotation = float3x3(iTangent, iBiNormal, iNormal);" << std::endl;
            ss << " oColor1.rgb = normalize(mul(texNormal, normalRotation));" << std::endl;
        } else 
        {
            ss << " oColor1.rgb = normalize(iNormal);" << std::endl;
        }
        if (permutation & GBufferMaterialGenerator::GBP_SPECULAR_MAP)
        {
            ss << " oColor2.rgb = tex2D(sSpecularMap, iUV0).r;" << std::endl;
        }else{
        	ss << " oColor2.rgb =1.0;" << std::endl;
        }


        if (permutation & GBufferMaterialGenerator::GBP_CUBE_MAP)
        {
        	 //ss << " oColor0.rgb *=texCUBE(sCubeMap,iNormal);" << std::endl;
        	ss << " oColor0.rgb =lerp(texCUBE(sCubeMap, normalize(oColor1.rgb)),oColor0.rgb,0.7);" << std::endl;
        }


        if (permutation & GBufferMaterialGenerator::GBP_GLOW_MAP)
        {
            ss << " oColor3.rgba = tex2D(sGlowMap, iUV0).rgba;" << std::endl;
        }else{
        	ss << " oColor3.rgba =float4(0,0,0,1);" << std::endl;
        }
    #ifdef WRITE_LINEAR_DEPTH
        ss << " oColor1.a = length(iViewPos) / cFarDistance;" << std::endl;
    #else
        ss << " oColor1.a = iDepth;" << std::endl;

    #endif

        ss << "}" << std::endl;
        
        String programSource = ss.str();
        std::cout << ss.str();
        String programName = mBaseName + "FP_" + StringConverter::toString(permutation);

    #if OGRE_DEBUG_MODE
        LogManager::getSingleton().getDefaultLog()->logMessage(programSource);
    #endif

        // Create shader object
        HighLevelGpuProgramPtr ptrProgram = HighLevelGpuProgramManager::getSingleton().createProgram(
            programName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "cg", GPT_FRAGMENT_PROGRAM);
        ptrProgram->setSource(programSource);
        ptrProgram->setParameter("entry_point","ToGBufferFP");
       /* if(mIsSm4)
        {
            ptrProgram->setParameter("profiles","ps_4_0");
        }
        else
        {
            ptrProgram->setParameter("profiles","ps_5_0 fp40");
        }*/
		if (mIsSm4)
		{
			ptrProgram->setParameter("profiles", "ps_4_0");
		}
		else
		{
			ptrProgram->setParameter("profiles", "ps_2_0 arbfp1");
		}
		// scheme GBuffer
		// MaterialManager::getSingleton().getByName(material)->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant(uniform, value);
        const GpuProgramParametersSharedPtr& params = ptrProgram->getDefaultParameters();
        params->setNamedAutoConstant("cSpecularity", GpuProgramParameters::ACT_SURFACE_SHININESS);

        if (numTextures == 0 || permutation & GBufferMaterialGenerator::GBP_HAS_DIFFUSE_COLOUR)
        {
            params->setNamedAutoConstant("cDiffuseColour", GpuProgramParameters::ACT_SURFACE_DIFFUSE_COLOUR);
        }

    #ifdef WRITE_LINEAR_DEPTH
        //TODO : Should this be the distance to the far corner, not the far clip distance?
        params->setNamedAutoConstant("cFarDistance", GpuProgramParameters::ACT_FAR_CLIP_DISTANCE);
    #endif

        ptrProgram->load();
        return GpuProgramPtr(ptrProgram);
    }
}

MaterialPtr GBufferMaterialGeneratorImpl::generateTemplateMaterial(MaterialGenerator::Perm permutation)
{
    String matName = mBaseName + "Mat_" + StringConverter::toString(permutation);

    MaterialPtr matPtr = MaterialManager::getSingleton().create
        (matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    Pass* pass = matPtr->getTechnique(0)->getPass(0);
    pass->setName(mBaseName + "Pass_" + StringConverter::toString(permutation));
    pass->setLightingEnabled(false);
    if (permutation & GBufferMaterialGenerator::GBP_NORMAL_MAP)
    {

        pass->createTextureUnitState();
    }

    if (permutation & GBufferMaterialGenerator::GBP_SPECULAR_MAP)
    {
        pass->createTextureUnitState();
    }
    if (permutation & GBufferMaterialGenerator::GBP_CUBE_MAP)
    {

        pass->createTextureUnitState();
    }
    if (permutation & GBufferMaterialGenerator::GBP_GLOW_MAP)
    {

        pass->createTextureUnitState();
    }
    uint32 numTextures = permutation & GBufferMaterialGenerator::GBP_TEXTURE_MASK;
    for (uint32 i=0; i<numTextures; i++)
    {
        pass->createTextureUnitState();
    }

    return matPtr;
}
