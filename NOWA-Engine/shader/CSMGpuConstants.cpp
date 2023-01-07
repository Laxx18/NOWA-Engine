/*  Copyright 2010-2012 Matthew Paul Reid
	This file is subject to the terms and conditions defined in
	file 'License.txt', which is part of this source code package.
	*/
#include "NOWAPrecompiled.h"
#include "CSMGpuConstants.h"

namespace NOWA
{
	const Ogre::Matrix4 PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE(
		0.5, 0, 0, 0.5,
		0, -0.5, 0, 0.5,
		0, 0, 1, 0,
		0, 0, 0, 1);

	CSMGpuConstants::CSMGpuConstants(size_t cascadeCount)
	{
		mParamsScaleBias = Ogre::GpuProgramManager::getSingletonPtr()->createSharedParameters("params_shadowMatrixScaleBias");
		for (size_t i = 1; i < cascadeCount; i++)
		{
			mParamsScaleBias->addConstantDefinition("texMatrixScaleBias" + Ogre::StringConverter::toString(i), Ogre::GCT_FLOAT4);
		}

		mParamsShadowMatrix = Ogre::GpuProgramManager::getSingletonPtr()->createSharedParameters("params_shadowMatrix");
		mParamsShadowMatrix->addConstantDefinition("texMatrix0", Ogre::GCT_MATRIX_4X4);
	}

	void CSMGpuConstants::updateCascade(const Ogre::Camera& texCam, size_t index)
	{
		if (index == 0)
		{
			mFirstCascadeViewMatrix = texCam.getViewMatrix();
			mFirstCascadeCamWidth = texCam.getOrthoWindowWidth();
			mViewRange = texCam.getFarClipDistance() - texCam.getNearClipDistance();

			Ogre::Matrix4 texMatrix0 = PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE * texCam.getProjectionMatrixWithRSDepth() * mFirstCascadeViewMatrix;
			mParamsShadowMatrix->setNamedConstant("texMatrix0", texMatrix0);
		}
		else
		{
			hack = PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE * texCam.getProjectionMatrixWithRSDepth() * texCam.getViewMatrix();

			Ogre::Matrix4 mat0 = mFirstCascadeViewMatrix;
			Ogre::Matrix4 mat1 = texCam.getViewMatrix();

			Ogre::Real offsetX = mat1[0][3] - mat0[0][3];
			Ogre::Real offsetY = mat1[1][3] - mat0[1][3];
			Ogre::Real offsetZ = mat1[2][3] - mat0[2][3];

			Ogre::Real width0 = mFirstCascadeCamWidth;
			Ogre::Real width1 = texCam.getOrthoWindowWidth();

			Ogre::Real oneOnWidth = 1.0f / width0;
			Ogre::Real offCenter = width1 / (2.0f * width0) - 0.5f;

			Ogre::RenderSystem* rs = Ogre::Root::getSingletonPtr()->getRenderSystem();
			float depthRange = Ogre::Math::Abs(rs->getMinimumDepthInputValue() - rs->getMaximumDepthInputValue());

			Ogre::Vector4 result;
			result.x = offsetX * oneOnWidth + offCenter;
			result.y = -offsetY * oneOnWidth + offCenter;
			result.z = -depthRange * offsetZ / mViewRange;
			result.w = width0 / width1;

			mParamsScaleBias->setNamedConstant("texMatrixScaleBias" + Ogre::StringConverter::toString(index), result);
		}
	}

} // namespace NOWA
