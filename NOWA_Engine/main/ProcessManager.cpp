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
		unsigned short int successCount = 0;
		unsigned short int failCount = 0;

		ProcessList::iterator it = this->processList.begin();

		while (it != this->processList.end())
		{
			// grab the next process
			ProcessPtr currProcessPtr = (*it);

			// save the iterator and increment the old one in case we need to remove this process from the list
			ProcessList::iterator thisIt = it;
			++it;

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
						this->attachProcess(childProcessPtr);
					}
					else
					{
						++successCount;  // only counts if the whole chain completed
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

				// remove the process and destroy it
				this->processList.erase(thisIt);
			}
		}

		return ((successCount << 16) | failCount);
	}

	WeakProcessPtr ProcessManager::attachProcess(ProcessPtr processPtr)
	{
		this->processList.push_front(processPtr);
		return WeakProcessPtr(processPtr);
	}

	void ProcessManager::clearAllProcesses(void)
	{
		this->processList.clear();
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
	}

};  // namespace end