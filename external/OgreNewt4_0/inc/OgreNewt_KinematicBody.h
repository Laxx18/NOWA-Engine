#ifndef _INCLUDE_OGRENEWT_KINEMATIC_BODY
#define _INCLUDE_OGRENEWT_KINEMATIC_BODY

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"

namespace OgreNewt
{
    class _OgreNewtExport KinematicBody : public Body
    {
    public:
        KinematicBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC);

        virtual ~KinematicBody();

        /// Integrates velocity for a kinematic body. Should be called in your update loop.
        void integrateVelocity(Ogre::Real dt);

        using KinematicContactCallback = std::function<void(OgreNewt::Body*)>;
        void setKinematicContactCallback(KinematicContactCallback callback)
        {
            m_kinematicContactCallback = std::move(callback);
        }

        void removeKinematicContactCallback() { m_kinematicContactCallback = nullptr; }

    protected:
        KinematicContactCallback m_kinematicContactCallback;
    };
}

#endif
