#include "NOWAPrecompiled.h"
#include "ProjectParameter.h"
#include "modules/DotSceneImportModule.h"
#include "modules/OgreNewtModule.h"
#include "Core.h"

namespace NOWA
{
	ProjectParameter::ProjectParameter()
		: createProject(false),
		openProject(false),
		createSceneInOwnState(false),
		ignoreGlobalScene(false),
		hasPhysics(true),
		physicsUpdateRate(60.0f), // Note: ND4 we use Substeps(2) -> 2 * 60 -> 120
		solverModel(4),
		solverForSingleIsland(true),
		broadPhaseAlgorithm(0),
		physicsThreadCount(1),
		linearDamping(0.001f),
		angularDamping(Ogre::Vector3(0.001f, 0.001f, 0.001f)),
		gravity(Ogre::Vector3(0.0f, -19.8f, 0.0f)),
		useV2Item(true),

		ambientLightUpperHemisphere(Ogre::ColourValue(0.3f, 0.5f, 0.7f)),
		ambientLightLowerHemisphere(Ogre::ColourValue(0.6f, 0.45f, 0.3f)),
		hemisphereDir(Ogre::Vector3(0.371386f, 0.642788f, 0.669998f)),
		envmapScale(1.0f),
		shadowFarDistance(100.0f),
		shadowDirectionalLightExtrusionDistance(100.0f),
		shadowDirLightTextureOffset(0.6f),
		shadowColor(Ogre::ColourValue(0.25f, 0.25f, 0.25f)),
		shadowQualityIndex(2),
		ambientLightModeIndex(0),

		forwardMode(2),
		lightWidth(4),
		lightHeight(4),
		// Note: The more slices, the less performant!
		numLightSlices(3),
		lightsPerCell(64),
		decalsPerCell(10),
		cubemapProbesPerCell(10),
		minLightDistance(1.0f),
		maxLightDistance(100.0f),
		renderDistance(Core::getSingletonPtr()->getGlobalRenderDistance()),

		hasRecast(false),
		cellSize(0.1),
		cellHeight(0.1),
		agentMaxSlope(65),
		agentHeight(1.6),
		agentMaxClimb(1.0f),
		agentRadius(0.3),
		edgeMaxLen(12),
		edgeMaxError(1.3),
		regionMinSize(50),
		regionMergeSize(20),
		vertsPerPoly(5),
		detailSampleDist(6),
		detailSampleMaxError(1),
		pointExtends(Ogre::Vector3(5.0f, 5.0f, 5.0f)),
		keepInterResults(false)
	{

	}

	///////////////////////////////////////////////////////////////////////////////

	SceneParameter::SceneParameter()
		: sceneManager(nullptr),
		mainCamera(nullptr),
		sunLight(nullptr),
		ogreNewt(nullptr),
		dotSceneImportModule(nullptr)
	{

	}

};  // namespace end