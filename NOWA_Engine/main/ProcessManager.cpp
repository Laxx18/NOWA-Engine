// ProcessManager.cpp
#include "NOWAPrecompiled.h"
#include "ProcessManager.h"

namespace NOWA
{
    ProcessManager::~ProcessManager(void)
    {
        this->clearAllProcesses();
    }

    ProcessManager* ProcessManager::getInstance(void)
    {
        static ProcessManager instance;

        return &instance;
    }

    //---------------------------------------------------------------------------------------------------------------------
    // The process update tick.  Called every logic tick.  This function returns the number of process chains that
    // succeeded in the upper 32 bits and the number of process chains that failed or were aborted in the lower 32 bits.
    //---------------------------------------------------------------------------------------------------------------------
    unsigned int ProcessManager::updateProcesses(float dt)
    {
        // First, dequeue all pending processes from other threads into our main processList
        ProcessPtr pendingProcess;
        while (this->pendingProcessQueue.try_dequeue(pendingProcess))
        {
            if (pendingProcess)
            {
                this->processList.push_front(pendingProcess);
            }
        }

        unsigned short int successCount = 0;
        unsigned short int failCount = 0;

        // Attention: iterate over a SNAPSHOT, never over live iterators of processList.
        // A process callback may re-enter this manager and modify processList while we are
        // iterating it: ChangeAppStateProcess::onSuccess() runs internalChangeAppState(),
        // which calls abortAllProcesses(true) / clearAllProcesses(). Both erase from
        // processList, which invalidates any iterator held here (crash: "list iterators
        // incompatible", with the iterator reading as 0xdddddddddddddddd).
        ProcessList snapshot(this->processList);

        for (const auto& currProcessPtr : snapshot)
        {
            if (nullptr == currProcessPtr)
            {
                continue;
            }

            // The process may have been removed in the meantime by a re-entrant call from a
            // previous process in this same snapshot. Skip it, it is no longer managed.
            if (std::find(this->processList.begin(), this->processList.end(), currProcessPtr) == this->processList.end())
            {
                continue;
            }

            // process is uninitialized, so initialize it
            if (currProcessPtr->getState() == Process::UNINITIALIZED)
            {
                currProcessPtr->onInit();
            }

            // give the process an update tick if it's running
            if (currProcessPtr->getState() == Process::RUNNING)
            {
                currProcessPtr->onUpdate(dt);
            }

            // check to see if the process is dead
            if (currProcessPtr->isDead())
            {
                // run the appropriate exit function
                switch (currProcessPtr->getState())
                {
                case Process::SUCCEEDED:
                {
                    currProcessPtr->onSuccess();
                    ProcessPtr childProcessPtr = currProcessPtr->removeChild();
                    if (childProcessPtr)
                    {
                        // Child process goes directly into processList since we're on main thread
                        this->processList.push_front(childProcessPtr);
                    }
                    else
                    {
                        ++successCount; // only counts if the whole chain completed
                    }
                    break;
                }
                case Process::FAILED:
                {
                    currProcessPtr->onFail();
                    ++failCount;
                    break;
                }
                case Process::ABORTED:
                {
                    currProcessPtr->onAbort();
                    ++failCount;
                    break;
                }
                }

                // Remove by value, not by iterator. onSuccess()/onFail()/onAbort() above may
                // already have cleared or reordered processList, so a stored iterator would be
                // dangling here. remove() is a no-op if the entry is gone already.
                this->processList.remove(currProcessPtr);
            }
        }

        return ((successCount << 16) | failCount);
    }

    WeakProcessPtr ProcessManager::attachProcess(ProcessPtr processPtr)
    {
        if (!processPtr)
        {
            return WeakProcessPtr();
        }

        // Thread-safe enqueue - can be called from any thread (render or main)
        this->pendingProcessQueue.enqueue(processPtr);
        return WeakProcessPtr(processPtr);
    }

    void ProcessManager::clearAllProcesses(void)
    {
        this->processList.clear();

        // Also drain the pending queue
        ProcessPtr dummy;
        while (this->pendingProcessQueue.try_dequeue(dummy))
        {
        }
    }

    void ProcessManager::abortAllProcesses(bool immediate)
    {
        ProcessList::iterator it = this->processList.begin();
        while (it != this->processList.end())
        {
            ProcessList::iterator tempIt = it;
            ++it;

            ProcessPtr processPtr = *tempIt;
            if (processPtr->isAlive())
            {
                processPtr->setState(Process::ABORTED);
                if (immediate)
                {
                    processPtr->onAbort();
                    this->processList.erase(tempIt);
                }
            }
        }

        // Note: Pending processes in the queue are not aborted
        // They will be added to processList on next update and then aborted if still alive
    }

}; // namespace end