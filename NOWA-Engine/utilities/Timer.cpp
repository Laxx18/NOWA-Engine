#include "NOWAPrecompiled.h"
#include "Timer.h"
#include "main/Core.h"

namespace NOWA
{
	Timer::Timer()
	{
	
	}

	Timer::~Timer()
	{

	}

	Timer* Timer::getInstance(void)
	{
		static Timer instance;

		return &instance;
	}
	
	unsigned long Timer::getTimeMS(void) const 
	{ 
		return Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
	}
 
	unsigned long Timer::getTimeMicro(void) const 
	{ 
		return Core::getSingletonPtr()->getOgreTimer()->getMicroseconds();
	}

	/*void Timer::delay(boost::function<void()> function, unsigned long offsetMS)
	{
		// http://www.radmangames.com/programming/how-to-use-boost-function
		TimedCallback callback(this->getTimeMS() + offsetMS, function);
		this->delay(callback);
	}*/

	void Timer::delay(FastDelegate0<void> function, unsigned long offsetMS)
	{
		// http://www.radmangames.com/programming/how-to-use-boost-function
		TimedCallback callback(this->getTimeMS() + offsetMS, function);
		this->delay(callback);
	}

	void Timer::update(void)
	{
		while (!this->queue.empty()) {
			
			const TimedCallback& callback = this->queue.top();
			long dt = callback.first - this->getTimeMS();
			// if the delta is positive, it has not enough time passed to call back the function
			if (dt > 0) {
				return;
			}
			// now is the right time, fire the function!! Therefore call second with ()
			callback.second();
			this->queue.pop();
		}
	}

	void Timer::clearQueue(void)
	{
		this->queue = TimedCallbackQueue();
	}

}; // namespace end