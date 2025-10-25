#ifndef _INCLUDE_OGRENEWT_TRIGGER_CALLBACK
#define _INCLUDE_OGRENEWT_TRIGGER_CALLBACK

#include "OgreNewt_Prerequisites.h"

namespace OgreNewt
{
    //! Abstract callback interface for triggers.
    /*!
        Users subclass this to implement OnEnter, OnInside, and OnExit
        just like before. It is automatically called from the ND4 trigger volume.
    */
    class _OgreNewtExport TriggerCallback
    {
    public:
        TriggerCallback() {}
        virtual ~TriggerCallback() {}

        //! Called when a body enters the trigger volume.
        virtual void OnEnter(const OgreNewt::Body* visitor) {}

        //! Called each step while a body is inside the trigger.
        virtual void OnInside(const OgreNewt::Body* visitor) {}

        //! Called when a body exits the trigger volume.
        virtual void OnExit(const OgreNewt::Body* visitor) {}
    };

} // namespace OgreNewt

#endif
