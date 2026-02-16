// ProcessManager.h
#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "Process.h"
#include "utilities/ConcurrentQueue.h"
#include <mutex>

namespace NOWA
{
    class EXPORTED ProcessManager
    {
        typedef std::list<ProcessPtr> ProcessList;
        typedef moodycamel::ConcurrentQueue<ProcessPtr> ThreadSafeProcessQueue;

        ProcessList processList;
        ThreadSafeProcessQueue pendingProcessQueue; // Thread-safe queue for processes from other threads

    public:
        ~ProcessManager(void);

        /**
         * @brief	The process update tick. Called every logic tick from the main thread.
         * @param	dt The delta time in milliseconds
         * @return	This function returns the number of process chains that
         *			succeeded in the upper 32 bits and the number of process chains that failed or were aborted in the lower 32 bits.
         */
        unsigned int updateProcesses(float dt);

        /**
         * @brief	Attaches the process to the process manager so it can be run on the next update.
         *			Thread-safe: Can be called from any thread (render or main).
         * @param	processPtr The process to attach to
         */
        WeakProcessPtr attachProcess(ProcessPtr processPtr);

        /**
         * @brief	Aborts all processes.
         *			Should only be called from the main thread.
         * @param	immediate If immediate == true, it immediately calls each ones onAbort() function and destroys all the processes.
         */
        void abortAllProcesses(bool immediate);

        unsigned int getProcessCount(void) const
        {
            return static_cast<unsigned int>(this->processList.size());
        }

        /**
         * @brief	Clears all processes.
         *			Should only be called from the main thread.
         */
        void clearAllProcesses(void);

    public:
        static ProcessManager* getInstance(void);

    private:
        ProcessManager() {

        };

        ProcessManager(const ProcessManager&);
    };

}; // namespace end

#endif