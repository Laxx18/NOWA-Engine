#ifndef _INCLUDE_OGRENEWT_KINEMATIC_BODY
#define _INCLUDE_OGRENEWT_KINEMATIC_BODY

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"

namespace OgreNewt
{
    class _OgreNewtExport KinematicBody : public Body
    {
    public:
        KinematicBody(World* world,
            Ogre::SceneManager* sceneManager,
            const OgreNewt::CollisionPtr& col,
            Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC);

        virtual ~KinematicBody();

        /// Integrates velocity for a kinematic body. Should be called in your update loop.
        void integrateVelocity(Ogre::Real dt);

        // modern callback type (kept)
        using KinematicContactCallback = std::function<void(OgreNewt::Body*)>;

        // Existing overloads (kept)
        // 1) direct std::function
        void setKinematicContactCallback(KinematicContactCallback callback)
        {
            m_kinematicContactCallback = std::move(callback);
        }

        // 2) legacy member function pointer
        template <class T>
        void setKinematicContactCallback(void (T::* method)(OgreNewt::Body*), T* instance)
        {
            m_kinematicContactCallback = [instance, method](OgreNewt::Body* body)
                {
                    if (instance && method)
                        (instance->*method)(body);
                };
        }

        // 3) NEW: functor/lambda that expects (T*, Body*) + instance to inject
        template <class T, class F>
        void setKinematicContactCallback(F&& fn, T* instance)
        {
            // accepts any callable fn(T*, OgreNewt::Body*)
            m_kinematicContactCallback =
                [instance, fn = std::forward<F>(fn)](OgreNewt::Body* body)
                {
                    fn(instance, body);
                };
        }

        void removeKinematicContactCallback()
        {
            m_kinematicContactCallback = nullptr;
        }

    protected:
        KinematicContactCallback m_kinematicContactCallback;
    };
}

#endif
