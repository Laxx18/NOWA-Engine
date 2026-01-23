#ifndef NOWA_H
#define NOWA_H

// References to all resources
extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include <luabind/luabind.hpp>

// Ogre
#include "Ogre.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/Pass/PassClear/OgreCompositorPassClearDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreArchiveManager.h"
#include "OgreAtmosphereComponent.h"

// core
#include "main/Core.h"

// Boost interface
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

// Input devices
#include "OISEvents.h"
#include "OISInputManager.h"
#include "OISKeyboard.h"
#include "OISMouse.h"

// main
#include "main/InputDeviceCore.h"
#include "main/AppStateManager.h"
#include "main/AppState.h"
#include "main/MenuStateMyGui.h"
#include "main/MainConfigurationStateMyGui.h"
#include "main/ClientConfigurationStateMyGui.h"
#include "main/ServerConfigurationStateMyGui.h"
#include "main/PauseStateMyGui.h"
#include "main/IntroState.h"
#include "main/ProjectParameter.h"

#include "main/ProcessManager.h"
#include "main/Events.h"
#include "main/ScriptEvent.h"
#include "main/ScriptEventManager.h"

// Physics
#include "OgreNewt.h"

// Sound
#include "OgreAL.h"

// Ki
#include "ki/Path.h"
#include "ki/Node.h"
#include "ki/Edge.h"
#include "ki/StateMachine.h"
#include "ki/MovingBehavior.h"
#include "ki/GoalComposite.h"

// Gameobject
#include "gameobject/GameObjectController.h"
#include "gameobject/DescriptionComponent.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/PhysicsTerrainComponent.h"
#include "gameobject/PhysicsBuoyancyComponent.h"
#include "gameobject/AttributesComponent.h"
#include "gameobject/NavMeshComponent.h"
#include "gameobject/NavMeshTerraComponent.h"
#include "gameobject/PhysicsRagDollComponent.h"
#include "gameobject/DistributedComponent.h"
#include "gameobject/InputDeviceComponent.h"
#include "gameobject/SoundComponent.h"
#include "gameobject/SimpleSoundComponent.h"
#include "gameobject/GameObjectTitleComponent.h"
#include "gameobject/SpawnComponent.h"
#include "gameobject/ExitComponent.h"
#include "gameobject/AnimationComponent.h"
#include "gameobject/AnimationComponentV2.h"
#include "gameobject/ParticleUniverseComponent.h"
#include "gameobject/PhysicsActiveCompoundComponent.h"
#include "gameobject/PhysicsActiveDestructAbleComponent.h"
#include "gameobject/PhysicsExplosionComponent.h"
#include "gameobject/PhysicsCompoundConnectionComponent.h"
#include "gameobject/PhysicsPlayerControllerComponent.h"
#include "gameobject/PhysicsActiveVehicleComponent.h"
#include "gameobject/PhysicsActiveComplexVehicleComponent.h"
#include "gameobject/PhysicsTriggerComponent.h"
#include "gameobject/PhysicsActiveKinematicComponent.h"
#include "gameobject/AiLuaComponent.h"
#include "gameobject/AiLuaGoalComponent.h"
#include "gameobject/PlaneComponent.h"
#include "gameobject/LightDirectionalComponent.h"
#include "gameobject/LightSpotComponent.h"
#include "gameobject/LightPointComponent.h"
#include "gameobject/LightAreaComponent.h"
#include "gameobject/FogComponent.h"
#include "gameobject/FadeComponent.h"
#include "gameobject/NodeComponent.h"
#include "gameobject/NodeTrackComponent.h"
#include "gameobject/LineComponent.h"
#include "gameobject/LinesComponent.h"
#include "gameobject/ManualObjectComponent.h"
#include "gameobject/RectangleComponent.h"
#include "gameobject/ValueBarComponent.h"
#include "gameobject/RibbonTrailComponent.h"
#include "gameobject/JointComponents.h"
#include "gameobject/PhysicsMaterialComponent.h"
#include "gameobject/CameraComponent.h"
#include "gameobject/ReflectionCameraComponent.h"
#include "gameObject/WorkspaceComponents.h"
#include "gameobject/CompositorEffectComponents.h"
#include "gameobject/DatablockPbsComponent.h"
#include "gameobject/DatablockUnlitComponent.h"
#include "gameobject/DatablockTerraComponent.h"
#include "gameobject/TagPointComponent.h"
#include "gameobject/TagChildNodeComponent.h"
#include "gameObject/AiComponents.h"
#include "gameObject/PlayerControllerComponents.h"
#include "gameObject/CameraBehaviorComponents.h"
#include "gameObject/ProceduralComponents.h"
#include "gameObject/LuaScriptComponent.h"
#include "gameObject/OceanComponent.h"
#include "gameObject/TerraComponent.h"
#include "gameObject/MeshDecalComponent.h"
#include "gameObject/DecalComponent.h"
#include "gameObject/PlanarReflectionComponent.h"
#include "gameObject/MoveMathFunctionComponent.h"
#include "gameObject/TransformComponent.h"
#include "gameObject/MyGUIComponents.h"
#include "gameObject/MyGUIItemBoxComponent.h"
#include "gameObject/MyGUIControllerComponents.h"
#include "gameObject/MyGUIMiniMapComponent.h"
#include "gameObject/LensFlareComponent.h"
#include "gameObject/BackgroundScrollComponent.h"
#include "gameobject/CrowdComponent.h"

namespace NOWA
{
	// Components
	typedef boost::shared_ptr<GameObject> GameObjectPtr;
	typedef boost::shared_ptr<DescriptionComponent> DescriptionCompPtr;
	typedef boost::shared_ptr<PhysicsComponent> PhysicsCompPtr;
	typedef boost::shared_ptr<PhysicsArtifactComponent> PhysicsArtifactCompPtr;
	typedef boost::shared_ptr<PhysicsTerrainComponent> PhysicsTerrainCompPtr;
	typedef boost::shared_ptr<PhysicsBuoyancyComponent> PhysicsBuoyancyCompPtr;
	typedef boost::shared_ptr<AttributesComponent> AttributesCompPtr;
	typedef boost::shared_ptr<PhysicsRagDollComponent> PhysicsRagDollCompPtr;
	typedef boost::shared_ptr<DistributedComponent> DistributedCompPtr;
	typedef boost::shared_ptr<InputDeviceComponent> InputDeviceCompPtr;
	typedef boost::shared_ptr<NavMeshComponent> NavMeshComponentCompPtr;
	typedef boost::shared_ptr<NavMeshTerraComponent> NavMeshTerraComponentCompPtr;
	typedef boost::shared_ptr<CrowdComponent> CrowdComponentCompPtr;
	typedef boost::shared_ptr<SoundComponent> SoundCompPtr;
	typedef boost::shared_ptr<SimpleSoundComponent> SimpleSoundCompPtr;
	typedef boost::shared_ptr<SpawnComponent> SpawnCompPtr;
	typedef boost::shared_ptr<ExitComponent> ExitCompPtr;
	typedef boost::shared_ptr<AnimationComponent> AnimationCompPtr;
	typedef boost::shared_ptr<ParticleUniverseComponent> ParticleUniverseCompPtr;
	typedef boost::shared_ptr<PhysicsActiveCompoundComponent> PhysicsActiveCompoundCompPtr;
	typedef boost::shared_ptr<PhysicsPlayerControllerComponent> PhysicsPlayerControllerCompPtr;
	typedef boost::shared_ptr<PhysicsActiveDestructableComponent> PhysicsActiveDestructableCompPtr;
	typedef boost::shared_ptr<PhysicsCompoundConnectionComponent> PhysicsCompoundConnectionCompPtr;
	typedef boost::shared_ptr<PhysicsExplosionComponent> PhysicsExplosionCompPtr;
	typedef boost::shared_ptr<PhysicsTriggerComponent> PhysicsTriggerCompPtr;
	typedef boost::shared_ptr<PhysicsActiveKinematicComponent> PhysicsActiveKinematicCompPtr;
	typedef boost::shared_ptr<PhysicsActiveVehicleComponent> PhysicsActiveVehicleCompPtr;
	typedef boost::shared_ptr<AiLuaComponent> AiLuaCompPtr;
	typedef boost::shared_ptr<AiLuaGoalComponent> AiLuaGoalCompPtr;
	
	typedef boost::shared_ptr<PlaneComponent> PlaneCompPtr;
	typedef boost::shared_ptr<LightDirectionalComponent> LightDirectionalCompPtr;
	typedef boost::shared_ptr<LightSpotComponent> LightSpotCompPtr;
	typedef boost::shared_ptr<LightPointComponent> LightPointCompPtr;
	typedef boost::shared_ptr<LightAreaComponent> LightAreaCompPtr;
	typedef boost::shared_ptr<FogComponent> FogCompPtr;
	typedef boost::shared_ptr<FadeComponent> FadeCompPtr;
	typedef boost::shared_ptr<NodeComponent> NodeCompPtr;
	typedef boost::shared_ptr<NodeTrackComponent> NodeTrackCompPtr;
	typedef boost::shared_ptr<LineComponent> LineCompPtr;
	typedef boost::shared_ptr<LineMeshComponent> LineMeshCompPtr;
	typedef boost::shared_ptr<LineMeshScaleComponent> LineMeshScaleCompPtr;
	typedef boost::shared_ptr<LinesComponent> LinesCompPtr;
	typedef boost::shared_ptr<ManualObjectComponent> ManualObjectCompPtr;
	typedef boost::shared_ptr<RectangleComponent> RectangleCompPtr;
	
	typedef boost::shared_ptr<RibbonTrailComponent> RibbonTrailCompPtr;

	// JointComponents
	typedef boost::shared_ptr<JointComponent> JointCompPtr;
	typedef boost::shared_ptr<JointHingeComponent> JointHingeCompPtr;
	typedef boost::shared_ptr<JointHingeActuatorComponent> JointHingeActuatorCompPtr;
	typedef boost::shared_ptr<JointPointToPointComponent> JointPointToPointCompPtr;
	typedef boost::shared_ptr<JointBallAndSocketComponent> JointBallAndSocketCompPtr;
	// typedef boost::shared_ptr<JointControlledBallAndSocketComponent> JointControlledBallAndSocketCompPtr;
	// typedef boost::shared_ptr<RagDollMotorDofComponent> RagDollMotorDofCompPtr;
	typedef boost::shared_ptr<JointPinComponent> JointPinCompPtr;
	typedef boost::shared_ptr<JointPlaneComponent> JointPlaneCompPtr;
	typedef boost::shared_ptr<JointCorkScrewComponent> JointCorkScrewCompPtr;
	typedef boost::shared_ptr<JointPassiveSliderComponent> JointPassiveSliderCompPtr;
	typedef boost::shared_ptr<JointSliderActuatorComponent> JointSliderActuatorCompPtr;
	typedef boost::shared_ptr<JointActiveSliderComponent> JointActiveSliderCompPtr;
	typedef boost::shared_ptr<JointMathSliderComponent> JointMathSliderCompPtr;
	typedef boost::shared_ptr<JointKinematicComponent> JointKinematicCompPtr;
	typedef boost::shared_ptr<JointTargetTransformComponent> JointTargetTransformCompPtr;
	typedef boost::shared_ptr<JointPathFollowComponent> JointPathFollowCompPtr;
	typedef boost::shared_ptr<JointDryRollingFrictionComponent> JointDryRollingFrictionCompPtr;
	typedef boost::shared_ptr<JointGearComponent> JointGearCompPtr;
	typedef boost::shared_ptr<JointWormGearComponent> JointWormGearCompPtr;
	typedef boost::shared_ptr<JointRackAndPinionComponent> JointRackAndPinionCompPtr;
	typedef boost::shared_ptr<JointPulleyComponent> JointPulleyCompPtr;
	typedef boost::shared_ptr<JointSpringComponent> JointSpringCompPtr;
	typedef boost::shared_ptr<JointAttractorComponent> JointAttractorCompPtr;
	typedef boost::shared_ptr<JointUniversalComponent> JointUniversalCompPtr;
	typedef boost::shared_ptr<JointUniversalActuatorComponent> JointUniversalActuatorCompPtr;
	typedef boost::shared_ptr<JointSlidingContactComponent> JointSlidingContactCompPtr;
	typedef boost::shared_ptr<Joint6DofComponent> Joint6DofCompPtr;
	typedef boost::shared_ptr<JointMotorComponent> JointMotorCompPtr;
	typedef boost::shared_ptr<JointWheelComponent> JointWheelCompPtr;
	typedef boost::shared_ptr<JointFlexyPipeHandleComponent> JointFlexyPipeHandleCompPtr;
	typedef boost::shared_ptr<JointFlexyPipeSpinnerComponent> JointFlexyPipeSpinnerCompPtr;
	typedef boost::shared_ptr<JointVehicleTireComponent> JointVehicleTireCompPtr;
	typedef boost::shared_ptr<JointComplexVehicleTireComponent> JointComplexVehicleTireCompPtr;

	typedef boost::shared_ptr<PhysicsComponent> PhysicsCompPtr;
	typedef boost::shared_ptr<PhysicsActiveComponent> PhysicsActiveCompPtr;
	typedef boost::shared_ptr<CameraComponent> CameraCompPtr;
	typedef boost::shared_ptr<ReflectionCameraComponent> ReflectionCameraCompPtr;

	// Workspace Components
	typedef boost::shared_ptr<WorkspaceBaseComponent> WorkspaceBaseCompPtr;
	typedef boost::shared_ptr<WorkspacePbsComponent> WorkspacePbsCompPtr;
	typedef boost::shared_ptr<WorkspaceSkyComponent> WorkspaceSkyCompPtr;
	typedef boost::shared_ptr<WorkspaceBackgroundComponent> WorkspaceBackgroundCompPtr;
	typedef boost::shared_ptr<WorkspaceCustomComponent> WorkspaceCustomCompPtr;

	// Compositor Effects
	typedef boost::shared_ptr<CompositorEffectBloomComponent> CompositorEffectBloomCompPtr;
	typedef boost::shared_ptr<CompositorEffectGlassComponent> CompositorEffectGlassCompPtr;
	typedef boost::shared_ptr<CompositorEffectOldTvComponent> CompositorEffectOldTvCompPtr;
	typedef boost::shared_ptr<CompositorEffectBlackAndWhiteComponent> CompositorEffectBlackAndWhiteCompPtr;
	typedef boost::shared_ptr<CompositorEffectColorComponent> CompositorEffectColorCompPtr;
	typedef boost::shared_ptr<CompositorEffectEmbossedComponent> CompositorEffectEmbossedCompPtr;
	typedef boost::shared_ptr<CompositorEffectSharpenEdgesComponent> CompositorEffectSharpenEdgesCompPtr;

	typedef boost::shared_ptr<DatablockPbsComponent> DatablockPbsCompPtr;
	typedef boost::shared_ptr<DatablockUnlitComponent> DatablockUnlitCompPtr;
	typedef boost::shared_ptr<DatablockTerraComponent> DatablockTerraCompPtr;
	typedef boost::shared_ptr<TagPointComponent> TagPointCompPtr;
	typedef boost::shared_ptr<TagChildNodeComponent> TagChildNodeCompPtr;
	typedef boost::shared_ptr<LuaScriptComponent> LuaScriptCompPtr;
	// typedef boost::shared_ptr<AreaOfInterestComponent> AreaOfInterestCompPtr;
	// typedef boost::shared_ptr<OceanComponent> OceanComponentCompPtr;
	typedef boost::shared_ptr<TerraComponent> TerraComponentCompPtr;
	typedef boost::shared_ptr<MeshDecalComponent> MeshDecalCompPtr;
	typedef boost::shared_ptr<DecalComponent> DecalCompPtr;
	typedef boost::shared_ptr<PlanarReflectionComponent> PlanarReflectionCompPtr;
	
	typedef boost::shared_ptr<MoveMathFunctionComponent> MoveMathFunctionCompPtr;
	typedef boost::shared_ptr<TransformComponent> TransformCompPtr;
	typedef boost::shared_ptr<LensFlareComponent> LensFlareCompPtr;

	// AiComponents
	typedef boost::shared_ptr<AiMoveComponent> AiMoveCompPtr;
	typedef boost::shared_ptr<AiMoveRandomlyComponent> AiMoveRandomlyCompPtr;
	typedef boost::shared_ptr<AiPathFollowComponent> AiPathFollowCompPtr;
	typedef boost::shared_ptr<AiWanderComponent> AiWanderCompPtr;
	typedef boost::shared_ptr<AiFlockingComponent> AiFlockingCompPtr;
	typedef boost::shared_ptr<AiRecastPathNavigationComponent> AiRecastPathNavigationCompPtr;
	typedef boost::shared_ptr<AiObstacleAvoidanceComponent> AiObstacleAvoidanceCompPtr;
	typedef boost::shared_ptr<AiHideComponent> AiHideCompPtr;
	typedef boost::shared_ptr<AiMoveComponent2D> AiMoveComp2DPtr;
	typedef boost::shared_ptr<AiPathFollowComponent2D> AiPathFollowComp2DPtr;
	typedef boost::shared_ptr<AiWanderComponent2D> AiWanderComp2DPtr;

	typedef boost::shared_ptr<PlayerControllerComponent> PlayerControllerCompPtr;
	typedef boost::shared_ptr<PlayerControllerJumpNRunComponent> PlayerControllerJumpNRunCompPtr;
	typedef boost::shared_ptr<PlayerControllerJumpNRunLuaComponent> PlayerControllerJumpNRunLuaCompPtr;
	typedef boost::shared_ptr<PlayerControllerClickToPointComponent> PlayerControllerClickToPointCompPtr;

	typedef boost::shared_ptr<CameraBehaviorComponent> CameraBehaviorCompPtr;
	typedef boost::shared_ptr<CameraBehaviorBaseComponent> CameraBehaviorBaseCompPtr;
	typedef boost::shared_ptr<CameraBehaviorFirstPersonComponent> CameraBehaviorFirstPersonCompPtr;
	typedef boost::shared_ptr<CameraBehaviorThirdPersonComponent> CameraBehaviorThirdPersonCompPtr;
	typedef boost::shared_ptr<CameraBehaviorFollow2DComponent> CameraBehaviorFollow2DCompPtr;
	typedef boost::shared_ptr<CameraBehaviorZoomComponent> CameraBehaviorZoomCompPtr;

	typedef boost::shared_ptr<ProceduralPrimitiveComponent> ProceduralPrimitiveCompPtr;
	typedef boost::shared_ptr<ProceduralBooleanComponent> ProceduralBooleanCompPtr;

	// MYGUI
	typedef boost::shared_ptr<MyGUIWindowComponent> MyGUIWindowCompPtr;
	typedef boost::shared_ptr<MyGUITextComponent> MyGUITextCompPtr;
	typedef boost::shared_ptr<MyGUIButtonComponent> MyGUIButtonCompPtr;
	typedef boost::shared_ptr<MyGUICheckBoxComponent> MyGUICheckBoxCompPtr;
	typedef boost::shared_ptr<MyGUIImageBoxComponent> MyGUIImageBoxCompPtr;
	typedef boost::shared_ptr<MyGUIProgressBarComponent> MyGUIProgressBarCompPtr;
	typedef boost::shared_ptr<MyGUIItemBoxComponent> MyGUIItemBoxCompPtr;
	typedef boost::shared_ptr<MyGUIListBoxComponent> MyGUIListBoxCompPtr;
	typedef boost::shared_ptr<MyGUIComboBoxComponent> MyGUIComboBoxCompPtr;
	typedef boost::shared_ptr<MyGUIMiniMapComponent> MyGUIMiniMapCompPtr;
	typedef boost::shared_ptr<MyGUIMessageBoxComponent> MyGUIMessageBoxCompPtr;
	
	typedef boost::shared_ptr<MyGUIPositionControllerComponent> MyGUIPositionControllerCompPtr;
	typedef boost::shared_ptr<MyGUIFadeAlphaControllerComponent> MyGUIFadeAlphaControllerCompPtr;
	typedef boost::shared_ptr<MyGUIScrollingMessageControllerComponent> MyGUIScrollingMessageControllerCompPtr;
	typedef boost::shared_ptr<MyGUIEdgeHideControllerComponent> MyGUIEdgeHideControllerCompPtr;
	typedef boost::shared_ptr<MyGUIRepeatClickControllerComponent> MyGUIRepeatClickControllerCompPtr;
	

	typedef boost::shared_ptr<NOWA::BackgroundScrollComponent> BackgroundScrollCompPtr;

}; // namespace end

// Modules
// #include "modules/CaelumModule.h"
#include "modules/RakNetModule.h"
#include "modules/OgreNewtModule.h"
#include "modules/OgreALModule.h"
#include "modules/ParticleUniverseModule.h"
#include "modules/OgreRecastModule.h"
#include "modules/LuaScriptApi.h"
#include "modules/GameProgressModule.h"
#include "modules/MiniMapModule.h"
#include "modules/CommandModule.h"
#include "modules/DotSceneExportModule.h"
#include "modules/DotSceneImportModule.h"
#include "modules/HLMSModule.h"
#include "modules/WorkspaceModule.h"
#include "modules/DeployResourceModule.h"
#include "modules/InputDeviceModule.h"
#include "modules/MeshDecalGeneratorModule.h"
#include "modules/GraphicsModule.h"

// Editor
#include "editor/Picker.h"
#include "editor/EditorManager.h"
#include "editor/SelectionRectangle.h"
#include "editor/SelectionManager.h"

// Camera
#include "camera/Camcorder.h"
#include "camera/CameraManager.h"
#include "camera/BaseCamera.h"
#include "camera/FirstPersonCamera.h"
#include "camera/ThirdPersonCamera.h"
#include "camera/FollowCamera2D.h"
#include "camera/ZoomCamera.h"
#include "camera/BasePhysicsCamera.h"

// Utilities
#include "utilities/Timer.h"
#include "utilities/FastDelegate.h"
#include "utilities/rapidxml.hpp"
#include "utilities/MathHelper.h"
#include "utilities/Interpolator.h"
#include "utilities/MyGUIUtilities.h"
#include "utilities/SkeletonVisualizer.h"
#include "utilities/ObjectTitle.h"
#include "utilities/MovableText.h"
#include "utilities/Outline.h"
#include "utilities/MeshDecal.h"
#include "utilities/MeshScissor.h"
#include "utilities/AnimationSerializer.h"
#include "utilities/AnimationBlender.h"
#include "utilities/AnimationBlenderV2.h"
// #include "utilities/DynamicLines.h"
#include "utilities/BoundingBoxDraw.h"
#include "utilities/LensFlare.h"
#include "utilities/MiniDump.h"
#include "utilities/Variant.h"
#include "utilities/FaderProcess.h"
#include "utilities/AutoCompleteSearch.h"
#include "utilities/LowPassAngleFilter.h"
#include "utilities/CPlusPlusComponentGenerator.h"

using namespace fastdelegate;

// Network
#include "network/GameObjectStateHistory.h"
#include "network/Client.h"
#include "network/Server.h"

// Console
#include "console/LuaConsoleInterpreter.h"
#include "console/LuaConsole.h"
#include "console/EditString.h"

#endif
