#ifndef RENDER_QUEUE_ENUMS_H
#define RENDER_QUEUE_ENUMS_H

namespace NOWA
{
	/*
	RenderQueue ID range [0; 100) & [200; 225) default to FAST (i.e. for v2 objects, like Items)
	RenderQueue ID range [100; 200) & [225; 256) default to V1_FAST (i.e. for v1 objects, like v1::Entity)
	By default new Items and other v2 objects are placed in RenderQueue ID 10
	By default new v1::Entity and other v1 objects are placed in RenderQueue ID 110
	*/
	enum eRenderQueues
	{
        RENDER_QUEUE_EARLY_FIRST = 0,
		RENDER_QUEUE_V2_MESH = 10, // E.g. Ogre::ManualObject, Item
		RENDER_QUEUE_TERRA = 11, // Terra v2
		RENDER_QUEUE_DISTORTION = 16, // fast
		RENDER_QUEUE_PARTICLE_STUFF = 155,
		RENDER_QUEUE_LEGACY = 156,
        RENDER_QUEUE_V2_TRANSPARENT = 213,
        RENDER_QUEUE_PARTICLE_TRANSPARENT = 214,
		RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND = 220, // E.g. Ogre::ManualObject, Item
		RENDER_QUEUE_GIZMO = 252,
        RENDER_QUEUE_MYGUI = 253,
		RENDER_QUEUE_MAX = 254 // E.g. Ogre::v1::Overlay
	};

	enum eVisibilityEnums
    {
        VISIBILITY_FLAG_GRASS = (1u << 20u),
        VISIBILITY_FLAG_TREE = (1u << 21u),
    };

}; // namespace end

#endif // RENDER_QUEUE_ENUMS_H