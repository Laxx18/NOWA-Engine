# Ogre_Sample_GpuParticles

Sample showing how to create gpu particles with Ogre. Currently only DirectX11 is working (shaders are written only in HLSL).
This contains ParticleSystemWorld, capable of storing multiple particle types at the same time.

![screen](screens/Sample_GpuParticles.jpg 'Screen')

# Instalation

CMAKE. There are 5 paths to set: OGRE_SOURCE, OGRE_BINARIES, OGRE_DEPENDENCIES_DIR, SDL2_SOURCE, SDL2_BINARIES.
Make sure there is RenderSystem_Direct3D11_d in plugins.

# Structures

Emitters cores - Manage how particles are created/updated.

Emitters instances - Instance of emitter core with position and orientation.

Particles - Stored on GPU side and never read to CPU.

Bucket - Particles from the same emitter are grouped in buckets (64 per bucket by default, may be multiplication of 64 as well).
 Those buckets contains data like active from and to indexes, emitter index and are send to GPU each frame.

# Algorithm remarks

When particle emitter instance is started, there are assigned buckets capable of storing particleLifetimeMax x emissionRate elements
(because this is maximum number of particles from emitter which may live at the same time).

On CPU each emitter instance keeps oldest and newest index of particles. Those indexes move by particleLifetimeMax x emissionRate x frameTime each frame 
and can wrap to 0 if they exceed assigned size. As particleLifetime for some particles may be shorter than others, some particles inside this range may be dead.
We simply ignore them on shader side (hide them) and on CPU side assume that all of them lived particleLifetimeMax. From this CPU range of particles we can fill buckets for GPU.

# Thanks

Those sources helped me greatly in creating this sample:

https://wickedengine.net/2017/11/07/gpu-based-particle-simulation/
Especially particles vertex and fragment shader source code, as well as depth buffer collisions.

https://spotcpp.com/creating-a-custom-hlms-to-add-support-for-wind-and-fog/
Sample how to create custom hlms in Ogre.

https://forums.ogre3d.org/viewtopic.php?p=550810
https://forums.ogre3d.org/viewtopic.php?p=550732
https://forums.ogre3d.org/viewtopic.php?p=550635
Help from Ogre community.
