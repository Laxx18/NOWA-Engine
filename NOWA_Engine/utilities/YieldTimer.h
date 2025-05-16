
#ifndef YIELD_TIMER_H
#define YIELD_TIMER_H

#include "OgreTimer.h"

namespace NOWA
{
    class YieldTimer
    {
        Ogre::Timer* externalTimer;

    public:
        YieldTimer( Ogre::Timer *externalTimer ) : externalTimer( externalTimer ) {}

        Ogre::uint64 yield( double frameTime, Ogre::uint64 startTime )
        {
            Ogre::uint64 endTime = this->externalTimer->getMicroseconds();

            while( frameTime * 1000000.0 > double( endTime - startTime ) )
            {
                endTime = this->externalTimer->getMicroseconds();

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
                SwitchToThread();
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX || OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
                sched_yield();
#endif
            }

            return endTime;
        }
    };
}  // namespace Demo

#endif
