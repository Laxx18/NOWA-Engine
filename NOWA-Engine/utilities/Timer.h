#ifndef TIMER_H
#define TIMER_H

#include <vector>
#include <queue>
#include <utility>

#include "utilities/FastDelegate.h"
using namespace fastdelegate;

#include "defines.h"

namespace NOWA
{
	typedef std::pair<unsigned long, FastDelegate0<void> > TimedCallback;
	

	class EXPORTED PrioritizeCallbacks
	{
	public:
		bool operator() (const TimedCallback& left, const TimedCallback& right) const
		{
			return right.first < left.first;
		}
	};

	// use a priority queue with own sort function (PrioritizeCallbacks)
	typedef std::priority_queue<TimedCallback, std::vector<TimedCallback>, PrioritizeCallbacks> TimedCallbackQueue;

	class EXPORTED Timer
	{
	public:
		static Timer* getInstance(void);

	   /**
		* @return Milliseconds since timer started
		*/
		unsigned long getTimeMS(void) const;

	   /**
		* @return Microseconds since timer started
		*/
		unsigned long getTimeMicro(void) const;

	   /**
		* @brief Delayed execution of a callback
		* @param callback The function to call
		*/
		inline void delay(TimedCallback callback)
		{
			this->queue.push(callback);
		}

	   /**
		* @brief Delayed execution of a callback
		* @param func the callback to execute
		* @param offsetMS to delay execution of callback in ms
		*/
		// void delay(boost::function<void()> function, unsigned long offsetMS);
		void delay(FastDelegate0<void> function, unsigned long offsetMS);

		/*template <class Class>
		void delay(boost::function<void(Class*)> function, Class* pInstancedClassPointer, unsigned long offsetMS)
		{
			TimedCallback callback(this->getTimeMS() + offsetMS, boost::bind(function, pInstancedClassPointer));
			this->delay(callback);
		}*/

	   /**
		* @brief Updates the callback queue, executes pending functions
		*/
		void update(void);

	   /**
		* @brief Clears the queue
		*/
		void clearQueue(void);

	private:
		Timer();
		~Timer();
		Timer(const Timer&);
		Timer& operator = (const Timer&);

	protected:
		TimedCallbackQueue queue;

		static Timer* pInstance;
	};

};  // namespace end

#endif