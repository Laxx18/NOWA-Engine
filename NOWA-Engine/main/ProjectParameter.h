#ifndef PROJECT_PARAMETER_H
#define PROJECT_PARAMETER_H

#include "defines.h"

class Ogre::SceneManager;
class Ogre::Camera;
class Ogre::Light;
class OgreNewt::World;

namespace NOWA
{
	struct EXPORTED ProjectParameter
	{
		ProjectParameter();

		Ogre::String sceneName;
		Ogre::String projectName;
		bool createProject;
		bool openProject;
		bool createSceneInOwnState;
		bool ignoreGlobalScene;
		bool hasPhysics;
		Ogre::Real physicsUpdateRate;
		unsigned short solverModel;
		bool solverForSingleIsland;
		unsigned short broadPhaseAlgorithm;
		unsigned short physicsThreadCount;
		Ogre::Real linearDamping;
		Ogre::Vector3 angularDamping;
		Ogre::Vector3 gravity;

		Ogre::ColourValue ambientLightUpperHemisphere;
		Ogre::ColourValue ambientLightLowerHemisphere;
		Ogre::Vector3 hemisphereDir;
		Ogre::Real envmapScale;
		Ogre::Real shadowFarDistance;
		Ogre::Real shadowDirectionalLightExtrusionDistance;
		Ogre::Real shadowDirLightTextureOffset;
		Ogre::ColourValue shadowColor;
		unsigned short shadowQualityIndex;
		unsigned short ambientLightModeIndex;

		unsigned short forwardMode;
		unsigned int lightWidth;
		unsigned int lightHeight;
		unsigned int numLightSlices;
		unsigned int lightsPerCell;
		unsigned int decalsPerCell;
		unsigned int cubemapProbesPerCell;
		Ogre::Real minLightDistance;
		Ogre::Real maxLightDistance;
		Ogre::Real renderDistance;

		bool hasRecast;
		Ogre::Real cellSize;
		Ogre::Real cellHeight;
		Ogre::Real agentMaxSlope;
		Ogre::Real agentHeight;
		Ogre::Real agentMaxClimb;
		Ogre::Real agentRadius;
		Ogre::Real edgeMaxLen;
		Ogre::Real edgeMaxError;
		Ogre::Real regionMinSize;
		Ogre::Real regionMergeSize;
		unsigned short vertsPerPoly;
		Ogre::Real detailSampleDist;
		Ogre::Real detailSampleMaxError;
		Ogre::Vector3 pointExtends;
		bool keepInterResults;
	};

	class DotSceneImportModule;

	struct EXPORTED SceneParameter
	{
		SceneParameter();

		Ogre::String appStateName;
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* mainCamera;
		Ogre::Light* sunLight;
		OgreNewt::World* ogreNewt;
		DotSceneImportModule* dotSceneImportModule;
	};

}; // Namespace end

#endif
