#ifndef _INCLUDE_OGRENEWT_TRIGGER_BODY
#define _INCLUDE_OGRENEWT_TRIGGER_BODY

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_TriggerControllerManager.h"

namespace OgreNewt
{
    //! Trigger body that detects enter/stay/exit using Newton Dynamics 4.0 trigger volumes
    class _OgreNewtExport TriggerBody : public Body
    {
    public:
        //! Constructor (API kept the same as your ND3 version)
        /*!
            Creates a trigger in an OgreNewt::World, based on a specific collision shape to react
            when other objects enter/are-inside/leave this volume.
            \param world OgreNewt world
            \param sceneManager for debug rendering (unchanged)
            \param col trigger collision shape
            \param triggerCallback heap-allocated callback (will be released in destructor)
            \param memoryType Ogre memory type
        */
        TriggerBody(World* world,
            Ogre::SceneManager* sceneManager,
            const OgreNewt::CollisionPtr& col,
            TriggerCallback* triggerCallback,
            Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC);

        virtual ~TriggerBody();

        //! Recreate the trigger with a new collision
        void reCreateTrigger(const OgreNewt::CollisionPtr& col);

        //! Access the trigger callback
        TriggerCallback* getTriggerCallback(void) const;

        //! Integrate velocity for kinematic trigger if you move it via velocity/omega
        void integrateVelocity(Ogre::Real dt);

    private:
        // ND4 trigger volume body (owned by the ND4 world via shared_ptr; we keep raw pointer for convenience)
        class OgreNewtTriggerVolume* m_triggerVolume;
        // Your user callback (we own it, same as ND3 behavior)
        TriggerCallback* m_triggerCallback;
    };

} // namespace OgreNewt

#endif
