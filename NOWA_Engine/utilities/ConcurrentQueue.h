#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

/*#include <WinSock2.h>
#include <windows.h>
#include <Ws2tcpip.h>*/

#include "defines.h"
#include <boost/thread/mutex.hpp>
#include <queue>

namespace NOWA
{


	/*
	Description

	helper class


	allows automatic Locking/ Unlocking of a Resource,
	protected by a Critical Section:
	- locking when this object gets constructed
	- unlocking when this object is destructed
	(goes out of scope)


	Usage


	when you need protected access to a resource, do the following
	1. Create a Critical Section associated with the resource
	2. Embody the code accessing the resource in braces {}
	3. Construct an ScopedCriticalSection object


	Example:
	// we assume that m_CriticalSection
	// is a private variable, and we use it to protect
	// 'this' from being accessed while we need safe access to a resource



	// code that does not need resource locking

	{
	ScopedCriticalSection I_am_locked( m_cs);

	// code that needs thread locking
	}

	// code that does not need resource locking


	*/

	// concurrent_queue was grabbed from 
	// http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
	// and was written by Anthony Williams
	//
	// class concurrent_queue					- Chapter 18, page 669
	//

	template<typename Data>
	class EXPORTED ConcurrentQueue
	{
	public:
		ConcurrentQueue()
		{
			this->dataPushed = CreateEvent(NULL, TRUE, FALSE, NULL);
		}

		void push(Data const& data)
		{
			boost::mutex::scoped_lock lock(this->mutex);

			this->queue.push(data);

			PulseEvent(this->dataPushed);
		}

		bool empty() const
		{
			boost::mutex::scoped_lock lock(this->mutex);
			return this->queue.empty();
		}

		bool tryPop(Data& poppedValue)
		{
			boost::mutex::scoped_lock lock(this->mutex);
			if(this->queue.empty()) {
				return false;
			}

			poppedValue = this->queue.front();
			this->queue.pop();
			return true;
		}

		void waitAndPop(Data& poppedValue)
		{
			boost::mutex::scoped_lock lock(this->mutex);
			while(this->queue.empty()) {
				WaitForSingleObject(this->dataPushed);
			}

			poppedValue = this->queue.front();
			this->queue.pop();
		}
	private:
		std::queue<Data> queue;
		boost::mutex mutex;

		HANDLE dataPushed;
	};

}; // namespace end

#endif

