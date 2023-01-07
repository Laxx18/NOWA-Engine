#ifndef PROCESS_H
#define PROCESS_H

#include "defines.h"
#include "boost/weak_ptr.hpp"

namespace NOWA
{
	class Process;
	typedef boost::shared_ptr<Process> ProcessPtr;
	typedef boost::weak_ptr<Process> WeakProcessPtr;

	/**
	 * @brief	Processes are ended by one of three methods: Success, Failure, or Aborted.
	 *			- Success means the process completed successfully. If the process has a child, it will be attached to
	 *			  the process manager.
	 *			- Failure means the process started but failed in some way. If the process has a child, it will be
	 *			   aborted.
	 *			- Aborted processes are processes that are canceled while not submitted to the process manager. Depending
	 *			   on the circumstances, they may or may not have gotten an onInit() call. For example, a process can
	 *			   spawn another process and call attachToParent() on itself.  If the new process fails, the child will
	 *			   get an abort() call on it, even though its status is RUNNING.
	 */
	class EXPORTED Process
	{
		friend class ProcessManager;

	public:
		enum State
		{
			// Processes that are neither dead nor alive
			UNINITIALIZED = 0,  // created but not running
			REMOVED,  // removed from the process list but not destroyed; 
					  // this can happen when a process that is already running is parented to another process

			// Living processes
			RUNNING,  // initialized and running
			PAUSED,  // initialized but paused

			// Dead processes
			SUCCEEDED,  // completed successfully
			FAILED,  // failed to complete
			ABORTED,  // aborted; may not have started
		};

	private:
		State state;
		ProcessPtr childProcess;

	public:
		Process(void);
		virtual ~Process(void);

	protected:
		// interface; these functions should be overridden by the subclass as needed
		/**
		 * @brief	Called during the first update. Responsible for setting the initial state (typically RUNNING)
		 */
		virtual void onInit(void)
		{
			state = RUNNING;
		}

		/**
		 * @brief	Called every frame
		 * @param	dt The delta time in milliseconds
		 */
		virtual void onUpdate(float dt) = 0;

		/**
		 * @brief	Called if the process succeeds
		 */
		virtual void onSuccess(void)
		{
		}

		/**
		 * @brief	Called if the process fails
		 */
		virtual void onFail(void)
		{
		}

		/**
		 * @brief	Called if the process is aborted
		 */
		virtual void onAbort(void)
		{
		}

	public:
		inline void succeed(void);
		inline void fail(void);

		inline void pause(void);
		inline void resume(void);

		State getState(void) const
		{
			return state;
		}
		bool isAlive(void) const
		{
			return (state == RUNNING || state == PAUSED);
		}
		bool isDead(void) const
		{
			return (state == SUCCEEDED || state == FAILED || state == ABORTED);
		}
		bool isRemoved(void) const
		{
			return (state == REMOVED);
		}
		bool isPaused(void) const
		{
			return state == PAUSED;
		}

		/**
		 * @brief	Attaches a child process to the current process
		 */
		inline void attachChild(ProcessPtr childProcess);

		/**
		 * @brief	Removes the child from this process.
		 * @note	This releases ownership of the child to the caller and completely removes it from the process chain.
		 */
		ProcessPtr removeChild(void);

		/**
		 * @brief	Gets a child process from the current process
		 * @note	The ownership of the child process is not be released
		 */
		ProcessPtr peekChild(void)
		{
			return this->childProcess;
		}

	private:
		void setState(State newState)
		{
			this->state = newState;
		}
	};

	inline void Process::succeed(void)
	{
		// assert(state == RUNNING || state == PAUSED);
		state = SUCCEEDED;
	}

	inline void Process::fail(void)
	{
		assert(state == RUNNING || state == PAUSED);
		state = FAILED;
	}

	inline void Process::attachChild(ProcessPtr childProcess)
	{
		if (this->childProcess)
		{
			this->childProcess->attachChild(childProcess);
		}
		else
		{
			this->childProcess = childProcess;
		}
	}

	inline void Process::pause(void)
	{
		if (state == RUNNING)
		{
			state = PAUSED;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Process] Attempting to pause a process that isn't running");
		}
	}

	inline void Process::resume(void)
	{
		if (state == PAUSED)
		{
			state = RUNNING;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Process] Attempting to resume a process that isn't running");
		}
	}

	/*
	inline ProcessPtr Process::getTopLevelProcess(void)
	{
	if (this->parent)
	returnthis->parent->getTopLevelProcess();
	else
	return this;
	}
	*/

	class DelayProcess : public Process
	{
	public:
		explicit DelayProcess(Ogre::Real timeToDelaySec)
			: timeToDelaySec(timeToDelaySec),
			timeDelayedSoFar(0)
		{

		}
	protected:
		virtual void onUpdate(float dt) override
		{
			this->timeDelayedSoFar += dt;
			if (this->timeDelayedSoFar >= timeToDelaySec)
			{
				this->succeed();
			}
		}
	private:
		Ogre::Real timeToDelaySec;
		Ogre::Real timeDelayedSoFar;
	};

	class ClosureProcess : public Process
	{
	public:
		explicit ClosureProcess(std::function<void(void)> closure)
			: closure(closure)
		{

		}
	protected:
		virtual void onUpdate(float dt) override
		{
			this->closure();
			this->succeed();
		}
	private:
		std::function<void(void)> closure;
	};


};  // namespace end

#endif