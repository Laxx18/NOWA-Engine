#include "NOWAPrecompiled.h"
#include "GameObjectFactory.h"
#include "utilities/XMLConverter.h"

#include "main/Core.h"

#include "PhysicsComponent.h"
#include "PhysicsTerrainComponent.h"
#include "PhysicsArtifactComponent.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsCompoundConnectionComponent.h"
#include "PhysicsRagDollComponent.h"
#include "AttributesComponent.h"
#include "NavMeshComponent.h"
#include "NavMeshTerraComponent.h"
#include "CrowdComponent.h"
#include "DistributedComponent.h"
#include "SoundComponent.h"
#include "SimpleSoundComponent.h"
#include "GameObjectTitleComponent.h"
#include "SpawnComponent.h"
#include "ExitComponent.h"
#include "ActivationComponent.h"
#include "AnimationComponent.h"
#include "ParticleUniverseComponent.h"
#include "PhysicsActiveCompoundComponent.h"
#include "PhysicsActiveDestructableComponent.h"
#include "PhysicsExplosionComponent.h"
#include "PhysicsPlayerControllerComponent.h"
#include "PhysicsTriggerComponent.h"
#include "PhysicsActiveKinematicComponent.h"
#include "PhysicsBuoyancyComponent.h"
#include "AiLuaComponent.h"
#include "PlaneComponent.h"
#include "LightDirectionalComponent.h"
#include "LightSpotComponent.h"
#include "LightPointComponent.h"
#include "LightAreaComponent.h"
#include "FogComponent.h"
#include "FadeComponent.h"
#include "NodeComponent.h"
#include "NodeTrackComponent.h"
#include "LineComponent.h"
#include "LinesComponent.h"
#include "ManualObjectComponent.h"
#include "RectangleComponent.h"
#include "ValueBarComponent.h"
#include "InventoryItemComponent.h"
#include "BillboardComponent.h"
#include "RibbonTrailComponent.h"
#include "JointComponents.h"
#include "PhysicsMaterialComponent.h"
#include "TimeTriggerComponent.h"
#include "TimeLineComponent.h"
#include "CameraComponent.h"
#include "ReflectionCameraComponent.h"
#include "WorkspaceComponents.h"
#include "CompositorEffectComponents.h"
#include "HdrEffectComponent.h"
#include "DatablockPbsComponent.h"
#include "DatablockUnlitComponent.h"
#include "DatablockTerraComponent.h"
#include "TagPointComponent.h"
#include "TagChildNodeComponent.h"
#include "AiComponents.h"
#include "PlayerControllerComponents.h"
#include "CameraBehaviorComponents.h"
#include "LookAfterComponent.h"
#include "ProceduralComponents.h"
#include "LuaScriptComponent.h"
#include "VehicleComponent.h"
#include "OceanComponent.h"
#include "TerraComponent.h"
#include "MeshDecalComponent.h"
#include "DecalComponent.h"
#include "PlanarReflectionComponent.h"
#include "MoveMathFunctionComponent.h"
#include "AttributeEffectComponent.h"
#include "TransformComponent.h"
#include "LensFlareComponent.h"
#include "MyGUIComponents.h"
#include "MyGUIItemBoxComponent.h"
#include "MyGUIControllerComponents.h"
#include "MyGUIMiniMapComponent.h"
#include "BackgroundScrollComponent.h"

#include "modules/RakNetModule.h"
#include "main/Events.h"
#include "main/AppStateManager.h"
#include "network/ConnectionType.h"

namespace NOWA
{
	GameObjectFactory::GameObjectFactory()
	{
		// Component factory is of type: GameObjectComponent and the specific id, register searches for the hashed id and puts it in a map for a later creation

		this->componentFactory.registerClass<AttributesComponent>(AttributesComponent::getStaticClassId(), AttributesComponent::getStaticClassName());
		this->componentFactory.registerClass<NavMeshComponent>(NavMeshComponent::getStaticClassId(), NavMeshComponent::getStaticClassName());
		this->componentFactory.registerClass<NavMeshTerraComponent>(NavMeshTerraComponent::getStaticClassId(), NavMeshTerraComponent::getStaticClassName());
		this->componentFactory.registerClass<CrowdComponent>(CrowdComponent::getStaticClassId(), CrowdComponent::getStaticClassName());
		this->componentFactory.registerClass<DistributedComponent>(DistributedComponent::getStaticClassId(), DistributedComponent::getStaticClassName());
		this->componentFactory.registerClass<SoundComponent>(SoundComponent::getStaticClassId(), SoundComponent::getStaticClassName());
		this->componentFactory.registerClass<SimpleSoundComponent>(SimpleSoundComponent::getStaticClassId(), SimpleSoundComponent::getStaticClassName());
		this->componentFactory.registerClass<GameObjectTitleComponent>(GameObjectTitleComponent::getStaticClassId(), GameObjectTitleComponent::getStaticClassName());
		this->componentFactory.registerClass<SpawnComponent>(SpawnComponent::getStaticClassId(), SpawnComponent::getStaticClassName());
		this->componentFactory.registerClass<ExitComponent>(ExitComponent::getStaticClassId(), ExitComponent::getStaticClassName());
		this->componentFactory.registerClass<ActivationComponent>(ActivationComponent::getStaticClassId(), ActivationComponent::getStaticClassName());
		this->componentFactory.registerClass<AnimationComponent>(AnimationComponent::getStaticClassId(), AnimationComponent::getStaticClassName());
		this->componentFactory.registerClass<ParticleUniverseComponent>(ParticleUniverseComponent::getStaticClassId(), ParticleUniverseComponent::getStaticClassName());
		this->componentFactory.registerClass<PlaneComponent>(PlaneComponent::getStaticClassId(), PlaneComponent::getStaticClassName());
		this->componentFactory.registerClass<AiLuaComponent>(AiLuaComponent::getStaticClassId(), AiLuaComponent::getStaticClassName());
		this->componentFactory.registerClass<NodeComponent>(NodeComponent::getStaticClassId(), NodeComponent::getStaticClassName());
		this->componentFactory.registerClass<NodeTrackComponent>(NodeTrackComponent::getStaticClassId(), NodeTrackComponent::getStaticClassName());
		this->componentFactory.registerClass<LineComponent>(LineComponent::getStaticClassId(), LineComponent::getStaticClassName());
		this->componentFactory.registerClass<LineMeshComponent>(LineMeshComponent::getStaticClassId(), LineMeshComponent::getStaticClassName());
		this->componentFactory.registerClass<LineMeshScaleComponent>(LineMeshScaleComponent::getStaticClassId(), LineMeshScaleComponent::getStaticClassName());
		this->componentFactory.registerClass<LinesComponent>(LinesComponent::getStaticClassId(), LinesComponent::getStaticClassName());
		this->componentFactory.registerClass<ManualObjectComponent>(ManualObjectComponent::getStaticClassId(), ManualObjectComponent::getStaticClassName());
		this->componentFactory.registerClass<RectangleComponent>(RectangleComponent::getStaticClassId(), RectangleComponent::getStaticClassName());
		this->componentFactory.registerClass<ValueBarComponent>(ValueBarComponent::getStaticClassId(), ValueBarComponent::getStaticClassName());
		this->componentFactory.registerClass<InventoryItemComponent>(InventoryItemComponent::getStaticClassId(), InventoryItemComponent::getStaticClassName());
		this->componentFactory.registerClass<TimeTriggerComponent>(TimeTriggerComponent::getStaticClassId(), TimeTriggerComponent::getStaticClassName());
		this->componentFactory.registerClass<TimeLineComponent>(TimeLineComponent::getStaticClassId(), TimeLineComponent::getStaticClassName());
		this->componentFactory.registerClass<CameraComponent>(CameraComponent::getStaticClassId(), CameraComponent::getStaticClassName());
		this->componentFactory.registerClass<ReflectionCameraComponent>(ReflectionCameraComponent::getStaticClassId(), ReflectionCameraComponent::getStaticClassName());

		// Workspace components
		this->componentFactory.registerClass<WorkspacePbsComponent>(WorkspacePbsComponent::getStaticClassId(), WorkspacePbsComponent::getStaticClassName());
		this->componentFactory.registerClass<WorkspaceSkyComponent>(WorkspaceSkyComponent::getStaticClassId(), WorkspaceSkyComponent::getStaticClassName());
		this->componentFactory.registerClass<WorkspaceBackgroundComponent>(WorkspaceBackgroundComponent::getStaticClassId(), WorkspaceBackgroundComponent::getStaticClassName());
		this->componentFactory.registerClass<WorkspaceCustomComponent>(WorkspaceCustomComponent::getStaticClassId(), WorkspaceCustomComponent::getStaticClassName());

		this->componentFactory.registerClass<HdrEffectComponent>(HdrEffectComponent::getStaticClassId(), HdrEffectComponent::getStaticClassName());
		this->componentFactory.registerClass<BackgroundScrollComponent>(BackgroundScrollComponent::getStaticClassId(), BackgroundScrollComponent::getStaticClassName());

		// Compositor Effects Comonents
		this->componentFactory.registerClass<CompositorEffectBloomComponent>(CompositorEffectBloomComponent::getStaticClassId(), CompositorEffectBloomComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectGlassComponent>(CompositorEffectGlassComponent::getStaticClassId(), CompositorEffectGlassComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectOldTvComponent>(CompositorEffectOldTvComponent::getStaticClassId(), CompositorEffectOldTvComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectKeyholeComponent>(CompositorEffectKeyholeComponent::getStaticClassId(), CompositorEffectKeyholeComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectBlackAndWhiteComponent>(CompositorEffectBlackAndWhiteComponent::getStaticClassId(), CompositorEffectBlackAndWhiteComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectColorComponent>(CompositorEffectColorComponent::getStaticClassId(), CompositorEffectColorComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectEmbossedComponent>(CompositorEffectEmbossedComponent::getStaticClassId(), CompositorEffectEmbossedComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectSharpenEdgesComponent>(CompositorEffectSharpenEdgesComponent::getStaticClassId(), CompositorEffectSharpenEdgesComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectDepthOfFieldComponent>(CompositorEffectDepthOfFieldComponent::getStaticClassId(), CompositorEffectDepthOfFieldComponent::getStaticClassName());
		this->componentFactory.registerClass<CompositorEffectHeightFogComponent>(CompositorEffectHeightFogComponent::getStaticClassId(), CompositorEffectHeightFogComponent::getStaticClassName());
		
		
		this->componentFactory.registerClass<DatablockPbsComponent>(DatablockPbsComponent::getStaticClassId(), DatablockPbsComponent::getStaticClassName());
		this->componentFactory.registerClass<DatablockUnlitComponent>(DatablockUnlitComponent::getStaticClassId(), DatablockUnlitComponent::getStaticClassName());
		this->componentFactory.registerClass<DatablockTerraComponent>(DatablockTerraComponent::getStaticClassId(), DatablockTerraComponent::getStaticClassName());
		this->componentFactory.registerClass<TagPointComponent>(TagPointComponent::getStaticClassId(), TagPointComponent::getStaticClassName());
		this->componentFactory.registerClass<TagChildNodeComponent>(TagChildNodeComponent::getStaticClassId(), TagChildNodeComponent::getStaticClassName());
		this->componentFactory.registerClass<LookAfterComponent>(LookAfterComponent::getStaticClassId(), LookAfterComponent::getStaticClassName());
		this->componentFactory.registerClass<BillboardComponent>(BillboardComponent::getStaticClassId(), BillboardComponent::getStaticClassName());
		this->componentFactory.registerClass<RibbonTrailComponent>(RibbonTrailComponent::getStaticClassId(), RibbonTrailComponent::getStaticClassName());
		this->componentFactory.registerClass<LuaScriptComponent>(LuaScriptComponent::getStaticClassId(), LuaScriptComponent::getStaticClassName());
		// this->componentFactory.registerClass<OceanComponent>(OceanComponent::getStaticClassId(), OceanComponent::getStaticClassName());
		this->componentFactory.registerClass<TerraComponent>(TerraComponent::getStaticClassId(), TerraComponent::getStaticClassName());
		this->componentFactory.registerClass<MeshDecalComponent>(MeshDecalComponent::getStaticClassId(), MeshDecalComponent::getStaticClassName());
		this->componentFactory.registerClass<DecalComponent>(DecalComponent::getStaticClassId(), DecalComponent::getStaticClassName());
		this->componentFactory.registerClass<PlanarReflectionComponent>(PlanarReflectionComponent::getStaticClassId(), PlanarReflectionComponent::getStaticClassName());
		this->componentFactory.registerClass<MoveMathFunctionComponent>(MoveMathFunctionComponent::getStaticClassId(), MoveMathFunctionComponent::getStaticClassName());
		this->componentFactory.registerClass<AttributeEffectComponent>(AttributeEffectComponent::getStaticClassId(), AttributeEffectComponent::getStaticClassName());
		this->componentFactory.registerClass<TransformComponent>(TransformComponent::getStaticClassId(), TransformComponent::getStaticClassName());
		this->componentFactory.registerClass<LensFlareComponent>(LensFlareComponent::getStaticClassId(), LensFlareComponent::getStaticClassName());
		
		// Light components
		this->componentFactory.registerClass<LightDirectionalComponent>(LightDirectionalComponent::getStaticClassId(), LightDirectionalComponent::getStaticClassName());
		this->componentFactory.registerClass<LightSpotComponent>(LightSpotComponent::getStaticClassId(), LightSpotComponent::getStaticClassName());
		this->componentFactory.registerClass<LightPointComponent>(LightPointComponent::getStaticClassId(), LightPointComponent::getStaticClassName());
		this->componentFactory.registerClass<LightAreaComponent>(LightAreaComponent::getStaticClassId(), LightAreaComponent::getStaticClassName());

		this->componentFactory.registerClass<FogComponent>(FogComponent::getStaticClassId(), FogComponent::getStaticClassName());
		this->componentFactory.registerClass<FadeComponent>(FadeComponent::getStaticClassId(), FadeComponent::getStaticClassName());

		// Physics components
		this->componentFactory.registerClass<PhysicsArtifactComponent>(PhysicsArtifactComponent::getStaticClassId(), PhysicsArtifactComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsTerrainComponent>(PhysicsTerrainComponent::getStaticClassId(), PhysicsTerrainComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsActiveComponent>(PhysicsActiveComponent::getStaticClassId(), PhysicsActiveComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsRagDollComponent>(PhysicsRagDollComponent::getStaticClassId(), PhysicsRagDollComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsActiveCompoundComponent>(PhysicsActiveCompoundComponent::getStaticClassId(), PhysicsActiveCompoundComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsActiveDestructableComponent>(PhysicsActiveDestructableComponent::getStaticClassId(), PhysicsActiveDestructableComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsExplosionComponent>(PhysicsExplosionComponent::getStaticClassId(), PhysicsExplosionComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsMaterialComponent>(PhysicsMaterialComponent::getStaticClassId(), PhysicsMaterialComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsCompoundConnectionComponent>(PhysicsCompoundConnectionComponent::getStaticClassId(), PhysicsCompoundConnectionComponent::getStaticClassName());
		this->componentFactory.registerClass<VehicleComponent>(VehicleComponent::getStaticClassId(), VehicleComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsPlayerControllerComponent>(PhysicsPlayerControllerComponent::getStaticClassId(), PhysicsPlayerControllerComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsTriggerComponent>(PhysicsTriggerComponent::getStaticClassId(), PhysicsTriggerComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsActiveKinematicComponent>(PhysicsActiveKinematicComponent::getStaticClassId(), PhysicsActiveKinematicComponent::getStaticClassName());
		this->componentFactory.registerClass<PhysicsBuoyancyComponent>(PhysicsBuoyancyComponent::getStaticClassId(), PhysicsBuoyancyComponent::getStaticClassName());

// Attention: is this correct?, since this component is also in joints now!
		// this->componentFactory.registerClass<PhysicsMathSliderComponent>(PhysicsMathSliderComponent::getStaticClassId(), PhysicsMathSliderComponent::getStaticClassName());
		
		// Joint components
		this->componentFactory.registerClass<JointComponent>(JointComponent::getStaticClassId(), JointComponent::getStaticClassName());
		this->componentFactory.registerClass<JointHingeComponent>(JointHingeComponent::getStaticClassId(), JointHingeComponent::getStaticClassName());
		this->componentFactory.registerClass<JointHingeActuatorComponent>(JointHingeActuatorComponent::getStaticClassId(), JointHingeActuatorComponent::getStaticClassName());
		this->componentFactory.registerClass<JointPointToPointComponent>(JointPointToPointComponent::getStaticClassId(), JointPointToPointComponent::getStaticClassName());
		this->componentFactory.registerClass<JointBallAndSocketComponent>(JointBallAndSocketComponent::getStaticClassId(), JointBallAndSocketComponent::getStaticClassName());
		// this->componentFactory.registerClass<JointControlledBallAndSocketComponent>(JointControlledBallAndSocketComponent::getStaticClassId(), JointControlledBallAndSocketComponent::getStaticClassName());
		// this->componentFactory.registerClass<RagDollMotorDofComponent>(RagDollMotorDofComponent::getStaticClassId(), RagDollMotorDofComponent::getStaticClassName());
		this->componentFactory.registerClass<JointPinComponent>(JointPinComponent::getStaticClassId(), JointPinComponent::getStaticClassName());
		this->componentFactory.registerClass<JointPlaneComponent>(JointPlaneComponent::getStaticClassId(), JointPlaneComponent::getStaticClassName());
		this->componentFactory.registerClass<JointCorkScrewComponent>(JointCorkScrewComponent::getStaticClassId(), JointCorkScrewComponent::getStaticClassName());
		this->componentFactory.registerClass<JointPassiveSliderComponent>(JointPassiveSliderComponent::getStaticClassId(), JointPassiveSliderComponent::getStaticClassName());
		this->componentFactory.registerClass<JointSliderActuatorComponent>(JointSliderActuatorComponent::getStaticClassId(), JointSliderActuatorComponent::getStaticClassName());
		this->componentFactory.registerClass<JointActiveSliderComponent>(JointActiveSliderComponent::getStaticClassId(), JointActiveSliderComponent::getStaticClassName());
		this->componentFactory.registerClass<JointMathSliderComponent>(JointMathSliderComponent::getStaticClassId(), JointMathSliderComponent::getStaticClassName());
		this->componentFactory.registerClass<JointKinematicComponent>(JointKinematicComponent::getStaticClassId(), JointKinematicComponent::getStaticClassName());
		this->componentFactory.registerClass<JointTargetTransformComponent>(JointTargetTransformComponent::getStaticClassId(), JointTargetTransformComponent::getStaticClassName());
		this->componentFactory.registerClass<JointPathFollowComponent>(JointPathFollowComponent::getStaticClassId(), JointPathFollowComponent::getStaticClassName());
		this->componentFactory.registerClass<JointDryRollingFrictionComponent>(JointDryRollingFrictionComponent::getStaticClassId(), JointDryRollingFrictionComponent::getStaticClassName());
		this->componentFactory.registerClass<JointGearComponent>(JointGearComponent::getStaticClassId(), JointGearComponent::getStaticClassName());
		this->componentFactory.registerClass<JointWormGearComponent>(JointWormGearComponent::getStaticClassId(), JointWormGearComponent::getStaticClassName());
		this->componentFactory.registerClass<JointRackAndPinionComponent>(JointRackAndPinionComponent::getStaticClassId(), JointRackAndPinionComponent::getStaticClassName());
		this->componentFactory.registerClass<JointPulleyComponent>(JointPulleyComponent::getStaticClassId(), JointPulleyComponent::getStaticClassName());
		this->componentFactory.registerClass<JointSpringComponent>(JointSpringComponent::getStaticClassId(), JointSpringComponent::getStaticClassName());
		this->componentFactory.registerClass<JointAttractorComponent>(JointAttractorComponent::getStaticClassId(), JointAttractorComponent::getStaticClassName());
		this->componentFactory.registerClass<JointUniversalComponent>(JointUniversalComponent::getStaticClassId(), JointUniversalComponent::getStaticClassName());
		this->componentFactory.registerClass<JointUniversalActuatorComponent>(JointUniversalActuatorComponent::getStaticClassId(), JointUniversalActuatorComponent::getStaticClassName());
		this->componentFactory.registerClass<JointSlidingContactComponent>(JointSlidingContactComponent::getStaticClassId(), JointSlidingContactComponent::getStaticClassName());
		this->componentFactory.registerClass<Joint6DofComponent>(Joint6DofComponent::getStaticClassId(), Joint6DofComponent::getStaticClassName());
		this->componentFactory.registerClass<JointMotorComponent>(JointMotorComponent::getStaticClassId(), JointMotorComponent::getStaticClassName());
		this->componentFactory.registerClass<JointWheelComponent>(JointWheelComponent::getStaticClassId(), JointWheelComponent::getStaticClassName());
		this->componentFactory.registerClass<JointFlexyPipeHandleComponent>(JointFlexyPipeHandleComponent::getStaticClassId(), JointFlexyPipeHandleComponent::getStaticClassName());
		this->componentFactory.registerClass<JointFlexyPipeSpinnerComponent>(JointFlexyPipeSpinnerComponent::getStaticClassId(), JointFlexyPipeSpinnerComponent::getStaticClassName());

		// AI Components
		this->componentFactory.registerClass<AiMoveComponent>(AiMoveComponent::getStaticClassId(), AiMoveComponent::getStaticClassName());
		this->componentFactory.registerClass<AiMoveRandomlyComponent>(AiMoveRandomlyComponent::getStaticClassId(), AiMoveRandomlyComponent::getStaticClassName());
		this->componentFactory.registerClass<AiPathFollowComponent>(AiPathFollowComponent::getStaticClassId(), AiPathFollowComponent::getStaticClassName());
		this->componentFactory.registerClass<AiWanderComponent>(AiWanderComponent::getStaticClassId(), AiWanderComponent::getStaticClassName());
		this->componentFactory.registerClass<AiFlockingComponent>(AiFlockingComponent::getStaticClassId(), AiFlockingComponent::getStaticClassName());
		this->componentFactory.registerClass<AiRecastPathNavigationComponent>(AiRecastPathNavigationComponent::getStaticClassId(), AiRecastPathNavigationComponent::getStaticClassName());
		this->componentFactory.registerClass<AiObstacleAvoidanceComponent>(AiObstacleAvoidanceComponent::getStaticClassId(), AiObstacleAvoidanceComponent::getStaticClassName());
		this->componentFactory.registerClass<AiHideComponent>(AiHideComponent::getStaticClassId(), AiHideComponent::getStaticClassName());
		this->componentFactory.registerClass<AiMoveComponent2D>(AiMoveComponent2D::getStaticClassId(), AiMoveComponent2D::getStaticClassName());
		this->componentFactory.registerClass<AiPathFollowComponent2D>(AiPathFollowComponent2D::getStaticClassId(), AiPathFollowComponent2D::getStaticClassName());
		this->componentFactory.registerClass<AiWanderComponent2D>(AiWanderComponent2D::getStaticClassId(), AiWanderComponent2D::getStaticClassName());

		// Player Controller Components
		this->componentFactory.registerClass<PlayerControllerJumpNRunComponent>(PlayerControllerJumpNRunComponent::getStaticClassId(), PlayerControllerJumpNRunComponent::getStaticClassName());
		this->componentFactory.registerClass<PlayerControllerJumpNRunLuaComponent>(PlayerControllerJumpNRunLuaComponent::getStaticClassId(), PlayerControllerJumpNRunLuaComponent::getStaticClassName());
		this->componentFactory.registerClass<PlayerControllerClickToPointComponent>(PlayerControllerClickToPointComponent::getStaticClassId(), PlayerControllerClickToPointComponent::getStaticClassName());

		// Camera Behavior Components
		this->componentFactory.registerClass<CameraBehaviorBaseComponent>(CameraBehaviorBaseComponent::getStaticClassId(), CameraBehaviorBaseComponent::getStaticClassName());
		this->componentFactory.registerClass<CameraBehaviorFirstPersonComponent>(CameraBehaviorFirstPersonComponent::getStaticClassId(), CameraBehaviorFirstPersonComponent::getStaticClassName());
		this->componentFactory.registerClass<CameraBehaviorThirdPersonComponent>(CameraBehaviorThirdPersonComponent::getStaticClassId(), CameraBehaviorThirdPersonComponent::getStaticClassName());
		this->componentFactory.registerClass<CameraBehaviorFollow2DComponent>(CameraBehaviorFollow2DComponent::getStaticClassId(), CameraBehaviorFollow2DComponent::getStaticClassName());
		this->componentFactory.registerClass<CameraBehaviorZoomComponent>(CameraBehaviorZoomComponent::getStaticClassId(), CameraBehaviorZoomComponent::getStaticClassName());
		
		// Procedural Components
		this->componentFactory.registerClass<ProceduralPrimitiveComponent>(ProceduralPrimitiveComponent::getStaticClassId(), ProceduralPrimitiveComponent::getStaticClassName());
		this->componentFactory.registerClass<ProceduralBooleanComponent>(ProceduralBooleanComponent::getStaticClassId(), ProceduralBooleanComponent::getStaticClassName());

		// MYGUI Components
		this->componentFactory.registerClass<MyGUIWindowComponent>(MyGUIWindowComponent::getStaticClassId(), MyGUIWindowComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUITextComponent>(MyGUITextComponent::getStaticClassId(), MyGUITextComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIButtonComponent>(MyGUIButtonComponent::getStaticClassId(), MyGUIButtonComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUICheckBoxComponent>(MyGUICheckBoxComponent::getStaticClassId(), MyGUICheckBoxComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIImageBoxComponent>(MyGUIImageBoxComponent::getStaticClassId(), MyGUIImageBoxComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIProgressBarComponent>(MyGUIProgressBarComponent::getStaticClassId(), MyGUIProgressBarComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIItemBoxComponent>(MyGUIItemBoxComponent::getStaticClassId(), MyGUIItemBoxComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIListBoxComponent>(MyGUIListBoxComponent::getStaticClassId(), MyGUIListBoxComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIComboBoxComponent>(MyGUIComboBoxComponent::getStaticClassId(), MyGUIComboBoxComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIMessageBoxComponent>(MyGUIMessageBoxComponent::getStaticClassId(), MyGUIMessageBoxComponent::getStaticClassName());
		
		this->componentFactory.registerClass<MyGUIPositionControllerComponent>(MyGUIPositionControllerComponent::getStaticClassId(), MyGUIPositionControllerComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIFadeAlphaControllerComponent>(MyGUIFadeAlphaControllerComponent::getStaticClassId(), MyGUIFadeAlphaControllerComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIScrollingMessageControllerComponent>(MyGUIScrollingMessageControllerComponent::getStaticClassId(), MyGUIScrollingMessageControllerComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIEdgeHideControllerComponent>(MyGUIEdgeHideControllerComponent::getStaticClassId(), MyGUIEdgeHideControllerComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIRepeatClickControllerComponent>(MyGUIRepeatClickControllerComponent::getStaticClassId(), MyGUIRepeatClickControllerComponent::getStaticClassName());
		this->componentFactory.registerClass<MyGUIMiniMapComponent>(MyGUIMiniMapComponent::getStaticClassId(), MyGUIMiniMapComponent::getStaticClassName());
	}

	GameObjectFactory::~GameObjectFactory()
	{
		
	}

	GameObjectFactory* GameObjectFactory::getInstance()
	{
		static GameObjectFactory instance;

		return &instance;
	}

	GameObjectPtr GameObjectFactory::createOrSetGameObjectFromXML(rapidxml::xml_node<>* xmlNode, Ogre::SceneManager* sceneManager, Ogre::SceneNode* sceneNode,
		Ogre::MovableObject* movableObject, GameObject::eType type, const Ogre::String& filename, bool forceCreation, bool forceClampY, GameObjectPtr existingGameObjectPtr)
	{
		rapidxml::xml_node<>* propertyElement = xmlNode->first_node("property");
		Ogre::String gameObjectName;
		Ogre::String category;
		Ogre::String tagName;
		Ogre::String startPoseName;
		int controlledByClientID = 0;
		Ogre::Vector3 defaultDirection = Ogre::Vector3::UNIT_Z;
		bool dynamic = true;
		bool useReflection = false;
		bool visible = true;
		bool global = false;
		bool clampY = false;
		unsigned long id = 0L;
		unsigned long referenceId = 0L;
		unsigned int renderQueueIndex = 10;
		bool renderQueueIndexSet = false;
		unsigned int renderDistance = 0;
		Ogre::Real lodDistance = 0.0f;
		unsigned int shadowRenderingDistance = 0;
		bool renderDistanceSet = false;
		bool shadowRenderingDistanceSet = false;

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Object")
		{
			gameObjectName = XMLConverter::getAttrib(propertyElement, "data", "GameObject");
			propertyElement = propertyElement->next_sibling("property");
		}
		// Check if there is an id for the game object (maybe from undo/redo) then use that id, to recover the game object with the same id
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Id")
		{
			id = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Category")
		{
			category = XMLConverter::getAttrib(propertyElement, "data", "Default");
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TagName")
		{
			tagName = XMLConverter::getAttrib(propertyElement, "data", "Default");
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ControlledByClient")
		{
			controlledByClientID = XMLConverter::getAttribInt(propertyElement, "data", 0);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Static")
		{
			dynamic = ! XMLConverter::getAttribBool(propertyElement, "data", false);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseReflection")
		{
			useReflection = XMLConverter::getAttribBool(propertyElement, "data", false);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Visible")
		{
			visible = XMLConverter::getAttribBool(propertyElement, "data", false);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DefaultDirection")
		{
			defaultDirection = XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_Z);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Global")
		{
			global = XMLConverter::getAttribBool(propertyElement, "data", false);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ClampY")
		{
			clampY = XMLConverter::getAttribBool(propertyElement, "data", false);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReferenceId")
		{
			referenceId = XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RenderQueueIndex")
		{
			renderQueueIndex = XMLConverter::getAttribUnsignedInt(propertyElement, "data", 10);
			propertyElement = propertyElement->next_sibling("property");
			renderQueueIndexSet = true;
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RenderDistance")
		{
			renderDistance = XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1000);
			propertyElement = propertyElement->next_sibling("property");
			renderDistanceSet = true;
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LodDistance")
		{
			lodDistance = XMLConverter::getAttribReal(propertyElement, "data", 300.0f);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowRenderingDistance")
		{
			shadowRenderingDistance = XMLConverter::getAttribUnsignedInt(propertyElement, "data", 300);
			propertyElement = propertyElement->next_sibling("property");
			shadowRenderingDistanceSet = true;
		}

		// Read all datablocks, this is necessary e.g. for plane component, because its mesh is created at runtime and never saved as resource
		// But the datablock does exist on disk, so in order not to have always a data block with the name 'Missing', get the data block name from game object
		std::vector<Ogre::String> dataBlocks;
		unsigned int i = 0;
		while (nullptr != propertyElement)
		{
			if (XMLConverter::getAttrib(propertyElement, "name") == "DataBlock" + Ogre::StringConverter::toString(i++))
			{
				Ogre::String dataBlockName = XMLConverter::getAttrib(propertyElement, "data");
				dataBlocks.emplace_back(dataBlockName);
				propertyElement = propertyElement->next_sibling("property");
			}
			else
			{
				break;
			}
		}

		GameObjectPtr gameObjectPtr = nullptr;

		if (gameObjectName == "GameObject")
		{
			// skip the creation if this is a network scenario, the game object is controlled by a client
			// forceParse: a game object can be created manually too, so forceCreation must be on, so that it will be created and not skipped again
			if (AppStateManager::getSingletonPtr()->getRakNetModule()->isNetworkSzenario() && controlledByClientID != 0 && !forceCreation)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectFactory] Skipping creation for game object: " + sceneNode->getName() +
					" because it will be replicated later.");
				
				// also delete the already created entity, and node etc.
// Attention: is that correct?
				if (movableObject != nullptr && sceneManager->hasMovableObject(movableObject))
				{
					sceneManager->destroyMovableObject(movableObject);
				}

				auto nodeIt = sceneNode->getChildIterator();
				while (nodeIt.hasMoreElements())
				{
					//go through all scenenodes in the scene
					Ogre::Node* subNode = nodeIt.getNext();
					subNode->removeAllChildren();
					//add them to the tree as parents
				}
				sceneNode->detachAllObjects();
				sceneManager->destroySceneNode(sceneNode);
				sceneNode = nullptr;
				return nullptr;
			}
			else
			{
				// Upgrade functionality of old projects: force setting global ids for those game objects
				if (sceneNode->getName() == "MainGameObject")
				{
					id = NOWA::GameObjectController::MAIN_GAMEOBJECT_ID;
				}
				else if (sceneNode->getName() == "MainCamera")
				{
					id = NOWA::GameObjectController::MAIN_CAMERA_ID;
				}
				else if (sceneNode->getName() == "SunLight")
				{
					id = NOWA::GameObjectController::MAIN_LIGHT_ID;
				}

				// attention with: no ref by category, since each attribute, that is no reference must use boost::ref
				if (nullptr == existingGameObjectPtr)
				{
					gameObjectPtr = boost::make_shared<GameObject>(sceneManager, sceneNode, movableObject, category, dynamic, type, id);
				}
				else
				{
					gameObjectPtr = existingGameObjectPtr;
				}

				// If this is a global game object, and in the local scene the same game object does already exist, the global one cannot be used, because the local one has already been registered
				// Note: First all local scene game objects are loaded and after that all global ones
				// If its a snapshot (saved game loaded, only values are set, hence its not necessary to check if the game object does exist, because it does and just the values are set...
#if 0
				if (true == global)
				{
					auto localExistingGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
					if (nullptr != localExistingGameObjectPtr)
					{
						Ogre::String message = "[GameObjectFactory] For the local game object: " + sceneNode->getName() +
							" the same global game object does exist. The global game object will be overwritten with the local one."
							" If this is not intended fix this scenario by deleting in the .scene xml file and do not save the scene.";
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
						boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
						NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
					}
				}
#endif
				gameObjectPtr->setTagName(tagName);
				gameObjectPtr->setControlledByClientID(controlledByClientID);
				gameObjectPtr->setDefaultDirection(defaultDirection);
				gameObjectPtr->setUseReflection(useReflection);
				gameObjectPtr->setVisible(visible);
				gameObjectPtr->setGlobal(global);
				gameObjectPtr->setClampY(clampY);
				gameObjectPtr->setReferenceId(referenceId);

				if (true == renderQueueIndexSet)
				{
					gameObjectPtr->setRenderQueueIndex(renderQueueIndex);
				}
				if (true == renderDistanceSet)
				{
					gameObjectPtr->setRenderDistance(renderDistance);
				}
				if (true == shadowRenderingDistanceSet)
				{
					gameObjectPtr->setShadowRenderingDistance(shadowRenderingDistance);
				}
				// Do not set via setter, because else the lod distance is re-calculated, but its just required to re-calculate if the user sets a different distance.
				gameObjectPtr->lodDistance->setValue(lodDistance);
				gameObjectPtr->setDataBlocks(dataBlocks);
			}
			if (!gameObjectPtr->init())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Failed to initialize GameObject '" + gameObjectPtr->getName() + "'");
				// Do not destroy any scene here, because its done automatically when this local gameObjectPtr is running out of scope!
				return nullptr;
			}

			// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
			bool noneDynamicGameObject = false;
			if (false == gameObjectPtr->isDynamic())
			{
				noneDynamicGameObject = true;
				gameObjectPtr->setDynamic(true);
			}
			if (true == noneDynamicGameObject)
			{
				gameObjectPtr->setDynamic(false);
			}

			unsigned int index = 0;
			// Loop through each property element and load the component
			while (nullptr != propertyElement)
			{
				Ogre::String componentName = XMLConverter::getAttrib(propertyElement, "name");

				if (Ogre::StringUtil::match(componentName, "Componen*", true))
				{
					GameObjectCompPtr existingGameObjectCompPtr = nullptr;
					if (nullptr != existingGameObjectPtr)
					{
						existingGameObjectCompPtr = NOWA::makeStrongPtr(existingGameObjectPtr->getComponentByIndex(index++));
					}

					GameObjectCompPtr componentPtr(this->createComponent(propertyElement, filename, gameObjectPtr, existingGameObjectCompPtr));
					if (nullptr != componentPtr)
					{
						if (nullptr == existingGameObjectCompPtr)
						{
							// Add circular association in order, that a component can call the owner gameobject and over that, another component to use its functionality
							gameObjectPtr->addComponent(componentPtr);
							componentPtr->setOwner(gameObjectPtr);
						}
					}
					else
					{
						// If an error occurs, we kill the game obect and bail. We could keep going, but the game object is will only be 
						// partially complete so it is not worth it. Note that the game object instance will be destroyed because it
						// will fall out of scope with nothing else pointing to it.
						sceneNode->removeAndDestroyAllChildren();
						sceneManager->destroySceneNode(sceneNode);
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Error: Could not create component: "
																		+ componentName + " for GameObject '" + gameObjectPtr->getName() + "'. Maybe the component has not been registered?");

						sceneNode = nullptr;
						return nullptr;
					}
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Error: GameObject '" + gameObjectPtr->getName()
																	+ "': Expected XML attribute: '" + XMLConverter::getAttrib(propertyElement, "name") + "'");

					propertyElement = propertyElement->next_sibling("property");
					/*throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameObjectFactory] GameObject '" + gameObjectPtr->getName()
						 + "': Expected XML attribute: '" + XMLConverter::getAttrib(propertyElement, "name") + "'\n", "NOWA");*/

						 // break;
				}
			}

			if (nullptr == existingGameObjectPtr)
			{
				const auto& existingGameObjectNamePtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(sceneNode->getName());

				// If naming collision: Somehow a local scene has the game object and the global one, kill the first one and register the second one.
				if (nullptr != existingGameObjectNamePtr)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Error: GameObject '" + existingGameObjectNamePtr->getName()
																	+ "' already exists. It will be deleted and the newer one registered. This scenario may haben if "
																	"somehow a local scene has the game object and the global one, kill the first one and register the second one.");
					AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObjectImmediately(existingGameObjectNamePtr->getId());
				}
				// Registers the game object to the controller, but do it before postInit to set a categoryID for the component
				AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
			}
			else
			{
				gameObjectPtr->resetVariants();
			}

			// Calls raycast (internally checks if game object should be clamped, when loaded and components like physics component etc. has been created)
			if (true == forceClampY)
			{
				gameObjectPtr->performRaycastForYClamping();
			}

			if (nullptr != existingGameObjectPtr)
			{
				boost::shared_ptr<EventDataNewGameObject> newGameObjectEvent(boost::make_shared<EventDataNewGameObject>(gameObjectPtr->getId()));
				AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(newGameObjectEvent);
			}
		}
		else
		{
			// If game object could not be initialized, destroy ogre data
			sceneNode->removeAndDestroyAllChildren();
			sceneManager->destroySceneNode(sceneNode);
			return nullptr;
		}

		if (false == Core::getSingletonPtr()->getIsGame() && nullptr != existingGameObjectPtr)
		{
			// Resets all attribute changes flag
			gameObjectPtr->resetChanges();
		}

		return gameObjectPtr;
	}

	GameObjectCompPtr GameObjectFactory::createComponent(GameObjectPtr gameObjectPtr, const Ogre::String& componentClassName)
	{
		GameObjectCompPtr componentPtr(this->componentFactory.create(NOWA::getIdFromName(componentClassName)));

		if (nullptr != componentPtr)
		{
			componentPtr->setOwner(gameObjectPtr);

			// Adds bi-directional association in order, that a component can call the owner gameobject and over that, another component to use its functionality

			// First adds (because occurence index will be increased, if x-times the same components has been added and can be used in post init, if post init failes, remove the component again 
			gameObjectPtr->addComponent(componentPtr);
			if (false == componentPtr->postInit())
			{
				gameObjectPtr->deleteComponent(componentPtr->getClassName());

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Error: Could not post-init component: "
					+ componentClassName + " for GameObject '" + gameObjectPtr->getName() + "'.");

				return nullptr;
			}

			// Sends event, that component has been created
			boost::shared_ptr<EventDataNewComponent> eventDataNewComponent(new EventDataNewComponent(componentClassName));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNewComponent);

			return componentPtr;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Error: Could not create component: "
				+ componentClassName + " for GameObject '" + gameObjectPtr->getName() + "'.");
			return nullptr;
		}
	}

	GameObjectCompPtr GameObjectFactory::createComponent(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename, GameObjectPtr gameObjectPtr, GameObjectCompPtr existingGameObjectCompPtr)
	{
		Ogre::String name = XMLConverter::getAttrib(propertyElement, "data", "");

#if 0
		if (nullptr == existingGameObjectCompPtr || existingGameObjectCompPtr->customDataString == GameObjectComponent::AttrCustomDataNewCreation())
		{
			if (nullptr != existingGameObjectCompPtr && existingGameObjectCompPtr->customDataString == GameObjectComponent::AttrCustomDataNewCreation())
			{
				gameObjectPtr->deleteComponent(existingGameObjectCompPtr.get());
			}
#endif

		if (nullptr == existingGameObjectCompPtr)
		{
			GameObjectCompPtr componentPtr(this->componentFactory.create(NOWA::getIdFromName(name)));
			if (nullptr != componentPtr)
			{
				if (false == componentPtr->init(propertyElement, filename))
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Error: Failed to initialize component: " + name);
					return nullptr;
				}
			}
			else
			{
				Ogre::String message = "[GameObjectFactory] Error: Could not find GameObjectComponent named: '" + name + "'. If its a plugin component, see your application folders plugin.cfg";
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
				boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataFeedback);
				return nullptr;
			}
			
			// Sends event, that component has been created
			boost::shared_ptr<EventDataNewComponent> eventDataNewComponent(new EventDataNewComponent(name));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNewComponent);

			return componentPtr;
		}
		else
		{
			existingGameObjectCompPtr->init(propertyElement, filename);
		}

		return existingGameObjectCompPtr;
	}

	GameObjectPtr GameObjectFactory::createGameObject(Ogre::SceneManager* sceneManager, Ogre::SceneNode* sceneNode, Ogre::MovableObject* movableObject, GameObject::eType type, const unsigned long id)
	{
		Ogre::String category = "Default";
		bool dynamic = true;

		GameObjectPtr gameObjectPtr = boost::make_shared<GameObject>(sceneManager, sceneNode, movableObject, category, dynamic, type, id);
		gameObjectPtr->setControlledByClientID(0);

		if (false == gameObjectPtr->init())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Failed to initialize GameObject '" + gameObjectPtr->getName() + "'");
			return nullptr;
		}
		else
		{
			const auto& existingGameObjectNamePtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(sceneNode->getName());

			// If naming collision: Somehow a local scene has the game object and the global one, kill the first one and register the second one.
			if (nullptr != existingGameObjectNamePtr)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectFactory] Error: GameObject '" + existingGameObjectNamePtr->getName()
																+ "' already exists. It will be deleted and the newer one registered. This scenario may haben if "
																"somehow a local scene has the game object and the global one, kill the first one and register the second one.");
				AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObjectImmediately(existingGameObjectNamePtr->getId());
			}
			// Registers the game object to the controller, but do it before postInit to set a categoryID for the component
			AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);

			boost::shared_ptr<EventDataNewGameObject> newGameObjectEvent(boost::make_shared<EventDataNewGameObject>(gameObjectPtr->getId()));
			AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(newGameObjectEvent);
			return gameObjectPtr;
		}
	}

	GenericObjectFactory<GameObjectComponent, unsigned int>* GameObjectFactory::getComponentFactory(void)
	{
		return &this->componentFactory;
	}

}; //namespace end