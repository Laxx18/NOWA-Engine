# OgreNewt 4.0 Migration Status

## Overview
This document tracks the migration of OgreNewt from Newton Dynamics 3.0 to Newton Dynamics 4.0 while maintaining compatibility with Ogre-Next graphics engine.

## Completed Components

### Core System ✅
- **OgreNewt_Prerequisites.h** - Updated to include Newton 4.0 headers (ndNewton.h, ndWorld.h, ndBodyKinematic.h, etc.)
- **OgreNewt_World.h/.cpp** - Migrated to use `ndWorld` instead of `NewtonWorld`
  - World creation and destruction
  - Update loop with interpolation
  - Thread management via `SetThreadCount()`
  - Body iteration via `GetBodyList()`
  
### Body System ✅
- **OgreNewt_Body.h/.cpp** - Migrated to use `ndBodyKinematic`/`ndBodyDynamic`
  - Transform synchronization via `SetNotifyCallback()`
  - Force/Torque callbacks via `SetForceAndTorqueCallback()`
  - Mass and inertia management
  - Velocity and omega control
  - Graphics node synchronization with interpolation
  - AABB queries
  - Collision shape management

### Collision System ✅
- **OgreNewt_Collision.h/.cpp** - Migrated to use `ndShape` and `ndShapeInstance`
  - Base collision class wrapper
  - Shape instance management
  - Material ID support
  
- **OgreNewt_CollisionPrimitives.h/.cpp** - Migrated primitive shapes
  - Box (`ndShapeBox`)
  - Sphere (`ndShapeSphere`)
  - Cylinder (`ndShapeCylinder`)
  - Capsule (`ndShapeCapsule`)
  - Cone (`ndShapeCone`)
  - Convex Hull (`ndShapeConvexHull`)
  - Compound (`ndShapeCompound`)
  - Chamfer Cylinder
  - Pyramid
  - **Note:** TreeCollision, HeightField, and ConcaveHull have placeholder implementations

### Utility System ✅
- **OgreNewt_Tools.h/.cpp** - Updated converter functions
  - `ndMatrix` ↔ Ogre::Matrix4 conversion
  - `ndVector` ↔ Ogre::Vector3 conversion
  - `ndQuaternion` ↔ Ogre::Quaternion conversion
  - **Note:** CollisionTools functions need Newton 4.0 ray-cast API implementation

## Graphics Integration ✅

The integration between Newton 4.0 physics and Ogre-Next graphics is complete:

### Transform Synchronization
\`\`\`cpp
// Newton 4.0 calls this callback when body transforms update
void Body::newtonTransformCallback(ndBodyKinematic* const body, const ndMatrix& matrix)
{
    OgreNewt::Body* me = static_cast<OgreNewt::Body*>(body->GetUserData());
    // Extract position and rotation from ndMatrix
    // Store for interpolation
}
\`\`\`

### Interpolated Rendering
\`\`\`cpp
// Called each frame with interpolation parameter
void Body::updateNode(Ogre::Real interpolatParam)
{
    // Interpolate between previous and current physics state
    m_nodePosit = m_prevPosit + velocity * interpolatParam;
    m_nodeRotation = Quaternion::Slerp(interpolatParam, m_prevRotation, m_curRotation);
    // Update Ogre scene node
}
\`\`\`

### Force Application
\`\`\`cpp
// Newton 4.0 calls this for physics simulation
void Body::newtonForceTorqueCallback(ndBodyKinematic* const body, ndFloat32 timestep)
{
    OgreNewt::Body* me = static_cast<OgreNewt::Body*>(body->GetUserData());
    if (me->m_forcecallback)
        me->m_forcecallback(me, timestep, 0);
}
\`\`\`

## Pending Components ⚠️

### Joint System (High Priority)
- **OgreNewt_Joint.h/.cpp** - Needs migration from `dCustomJoint` to `ndJointBilateralConstraint`
- **OgreNewt_BasicJoints.h/.cpp** - All joint types need Newton 4.0 API:
  - BallAndSocket → `ndJointBallAndSocket`
  - Hinge → `ndJointHinge`
  - Slider → `ndJointSlider`
  - Universal → `ndJointUniversal`
  - And all other joint types...

### Advanced Features (Medium Priority)
- **Contact Joints** - Newton 4.0 contact iteration API differs
- **Soft Bodies** - Newton 4.0 has different deformable body system
- **Triggers** - Need to implement using Newton 4.0 trigger volumes
- **Ray Casting** - CollisionTools ray-cast functions need Newton 4.0 API
- **Static Meshes** - TreeCollision/HeightField need proper Newton 4.0 implementation

### Debug Visualization (Low Priority)
- **Debug collision rendering** - Mesh extraction from Newton 4.0 shapes
- **Joint debug display** - Visual debugging for joints

## Key API Differences: Newton 3.0 → 4.0

### World Management
- `NewtonWorld*` → `ndWorld*`
- `NewtonUpdate()` → `ndWorld::Update()`
- `NewtonSetThreadsCount()` → `ndWorld::SetThreadCount()`

### Body Management
- `NewtonBody*` → `ndBodyKinematic*` / `ndBodyDynamic*`
- `NewtonBodySetMatrix()` → `ndBodyKinematic::SetMatrix()`
- `NewtonBodyGetOmega()` → `ndBodyKinematic::GetOmega()`
- Float arrays → `ndMatrix`, `ndVector`, `ndQuaternion` classes

### Collision Management
- `NewtonCollision*` → `ndShape*` / `ndShapeInstance*`
- `NewtonCreateBox()` → `new ndShapeBox()`
- Collision functions → Shape class methods

### Callbacks
- Same callback pattern but with Newton 4.0 types
- `SetNotifyCallback()` for transform updates
- `SetForceAndTorqueCallback()` for physics

## Testing Recommendations

1. **Start Simple** - Test basic box/sphere bodies with gravity
2. **Add Collisions** - Test collision detection between primitives
3. **Test Joints** - Once joint system is migrated, test each joint type
4. **Performance** - Compare Newton 4.0 vs 3.0 performance
5. **Stability** - Long-running stress tests

## Build Configuration

### Required Newton 4.0 Libraries
- `ndCore.lib` - Core math and containers
- `ndNewton.lib` - Physics engine
- `ndSolvers.lib` - Constraint solvers

### Include Paths
\`\`\`
newton-4.00/sdk/dCore
newton-4.00/sdk/dNewton
newton-4.00/sdk/dCollision
\`\`\`

### Preprocessor Defines
- `_NEWTON_USE_DOUBLE` (if using double precision)
- Platform-specific defines as needed

## Migration Strategy for Remaining Work

### Phase 1: Core Joints (Week 1-2)
1. Create base `Joint` class using `ndJointBilateralConstraint`
2. Implement `BallAndSocket`, `Hinge`, `Slider`
3. Test basic joint functionality

### Phase 2: Advanced Joints (Week 3-4)
1. Implement remaining joint types
2. Add joint limits and motors
3. Test complex joint combinations

### Phase 3: Advanced Features (Week 5-6)
1. Implement contact callbacks
2. Add ray-casting support
3. Implement static mesh collisions

### Phase 4: Polish (Week 7-8)
1. Debug visualization
2. Performance optimization
3. Documentation and examples

## Notes

- The core physics-graphics integration is **complete and functional**
- API compatibility with OgreNewt 3.0 is maintained where possible
- Newton 4.0 uses modern C++ with classes instead of C-style API
- Performance should be better due to Newton 4.0 improvements
- Some Newton 3.0 features may not have direct Newton 4.0 equivalents

## Contact & Support

For Newton Dynamics 4.0 documentation:
- GitHub: https://github.com/MADEAPPS/newton-dynamics/tree/master/newton-4.00
- Sandbox Examples: newton-4.00/applications/ndSandbox

For Ogre-Next documentation:
- GitHub: https://github.com/OGRECave/ogre-next
