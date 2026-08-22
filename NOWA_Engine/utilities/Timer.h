#ifndef NOWA_TIMER_H
#define NOWA_TIMER_H

#include "defines.h"

#include <atomic>
#include <chrono>
#include <ctime>

namespace NOWA
{
    /**
     * @class   Timer
     * @brief   Monotonic timer based on std::chrono::steady_clock.
     *
     * Drop-in replacement for Ogre::Timer. Method names, signatures and return types match, so
     * an existing call site like
     *
     *     Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
     *
     * keeps compiling unchanged once the getter returns a NOWA::Timer* instead of an
     * Ogre::Timer*.
     *
     * Why this class exists - two problems with Ogre's Win32 Timer:
     *
     * 1) PERFORMANCE. Ogre wraps every QueryPerformanceCounter read in a
     *    SetThreadAffinityMask() pair, pinning the calling thread to a single core (reset()
     *    scans for the lowest bit of the process affinity mask, so in practice core 0) and
     *    unpinning it afterwards. That is two kernel transitions per call plus, whenever the
     *    thread was not already on that core, a real thread migration with the corresponding
     *    cache and TLB loss. With a logic thread, a render thread and a physics thread all
     *    timing themselves, they additionally serialise on that one core.
     *    The pinning was a workaround for broken multi-core HALs around 2005. Since Windows
     *    Vista, QPC is consistent across cores on any machine with an invariant TSC, and
     *    Microsoft's "Acquiring high-resolution time stamps" documentation explicitly advises
     *    against setting thread affinity for QPC reads.
     *
     * 2) CORRECTNESS. Ogre's getMilliseconds() and getMicroseconds() MUTATE member state on
     *    every call (mStartTime and mLastTime, for the GetTickCount based leap compensation).
     *    A single Ogre::Timer instance read from two threads is therefore a data race, and the
     *    compensation can cascade between them: thread A shifts mStartTime, thread B computes
     *    its offset against the already shifted base, exceeds the +/-100 ms threshold and
     *    shifts again. The returned time then visibly jumps. A per-frame delta computed from
     *    such a timer explodes, which is a classic cause of stutter and flicker.
     *
     * This class holds its start point in an atomic and performs no read-modify-write, so the
     * same instance can be read from any number of threads concurrently, including while
     * another thread calls reset().
     *
     * Attention: steady_clock, never system_clock. Only steady_clock is guaranteed monotonic;
     * the system clock can jump backwards on NTP sync, DST or a manual change, which would
     * produce negative deltas.
     */
    class EXPORTED Timer
    {
    public:
        Timer()
            : startTicks(std::chrono::steady_clock::now().time_since_epoch().count()),
              zeroClock(std::clock())
        {
        }

        ~Timer()
        {
        }

        /**
         * @brief   Kept for API compatibility with Ogre::Timer.
         * @param   key     Option name. Only "QueryAffinityMask" is recognised.
         * @param   val     Ignored.
         * @return  True if the key is recognised, false otherwise.
         * @note    This is a NO-OP. The affinity mask exists in Ogre only to pin the thread for
         *          the QPC read, and that pinning is exactly what this class avoids. The
         *          function still returns true for "QueryAffinityMask" so that existing code
         *          which checks the result keeps behaving the same.
         */
        bool setOption(const Ogre::String& key, const void* val)
        {
            if ("QueryAffinityMask" == key)
            {
                return true;
            }

            return false;
        }

        /**
         * @brief   Restarts the timer.
         * @note    Safe to call while other threads are reading: those simply see either the
         *          old or the new start point, never a torn value.
         */
        void reset(void)
        {
            this->startTicks.store(std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
            this->zeroClock.store(std::clock(), std::memory_order_relaxed);
        }

        /**
         * @brief   Milliseconds elapsed since construction or the last reset().
         */
        Ogre::uint64 getMilliseconds(void) const
        {
            return static_cast<Ogre::uint64>(std::chrono::duration_cast<std::chrono::milliseconds>(this->elapsed()).count());
        }

        /**
         * @brief   Microseconds elapsed since construction or the last reset().
         */
        Ogre::uint64 getMicroseconds(void) const
        {
            return static_cast<Ogre::uint64>(std::chrono::duration_cast<std::chrono::microseconds>(this->elapsed()).count());
        }

        /**
         * @brief   Milliseconds of CPU time elapsed since construction or the last reset().
         * @note    Same std::clock() semantics as Ogre::Timer, including its platform quirks:
         *          on Windows this is wall time since process start rather than consumed CPU
         *          time. Kept only for API parity - prefer getMilliseconds().
         */
        Ogre::uint64 getMillisecondsCPU(void) const
        {
            const std::clock_t newClock = std::clock();
            return static_cast<Ogre::uint64>(static_cast<double>(newClock - this->zeroClock.load(std::memory_order_relaxed)) /
                                             (static_cast<double>(CLOCKS_PER_SEC) / 1000.0));
        }

        /**
         * @brief   Microseconds of CPU time elapsed since construction or the last reset().
         * @note    See getMillisecondsCPU().
         */
        Ogre::uint64 getMicrosecondsCPU(void) const
        {
            const std::clock_t newClock = std::clock();
            return static_cast<Ogre::uint64>(static_cast<double>(newClock - this->zeroClock.load(std::memory_order_relaxed)) /
                                             (static_cast<double>(CLOCKS_PER_SEC) / 1000000.0));
        }

        // ---------------------------------------------------------------------------------
        // Additions beyond the Ogre::Timer API
        // ---------------------------------------------------------------------------------

        /**
         * @brief   Seconds elapsed as a float.
         * @note    Convenience for the very common 'getMilliseconds() * 0.001f' pattern, but
         *          without its intermediate rounding to whole milliseconds.
         */
        float getSeconds(void) const
        {
            return std::chrono::duration<float>(this->elapsed()).count();
        }

        /**
         * @brief   Milliseconds elapsed as a double, with sub-millisecond resolution.
         */
        double getMillisecondsPrecise(void) const
        {
            return std::chrono::duration<double, std::milli>(this->elapsed()).count();
        }

    private:
        std::chrono::steady_clock::duration elapsed(void) const
        {
            const std::chrono::steady_clock::time_point start{std::chrono::steady_clock::duration{this->startTicks.load(std::memory_order_relaxed)}};
            return std::chrono::steady_clock::now() - start;
        }

        // Attention: the raw tick count is stored, not a time_point, because std::atomic needs
        // a trivially copyable type and a time_point is not usable directly.
        std::atomic<std::chrono::steady_clock::rep> startTicks;
        std::atomic<std::clock_t> zeroClock;
    };

    /**
     * @class   ScopedTimer
     * @brief   Stopwatch for ad hoc measurements inside a scope.
     *
     * Usage:
     *     NOWA::ScopedTimer scopedTimer;
     *     ... work ...
     *     const double millis = scopedTimer.getElapsedMilliseconds();
     *
     * Attention: deliberately does NOT log in its destructor. A destructor that writes to the
     * Ogre log flushes to disk on every message, and dropping that into a hot loop hides the
     * very cost you are hunting.
     */
    class ScopedTimer
    {
    public:
        ScopedTimer()
            : startTime(std::chrono::steady_clock::now())
        {
        }

        double getElapsedMilliseconds(void) const
        {
            return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - this->startTime).count();
        }

        double getElapsedMicroseconds(void) const
        {
            return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - this->startTime).count();
        }

        float getElapsedSeconds(void) const
        {
            return std::chrono::duration<float>(std::chrono::steady_clock::now() - this->startTime).count();
        }

        void restart(void)
        {
            this->startTime = std::chrono::steady_clock::now();
        }

    private:
        std::chrono::steady_clock::time_point startTime;
    };

}; // namespace end

#endif
