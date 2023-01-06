#ifndef _OGREAL_UTIL_H_
#define _OGREAL_UTIL_H_

#include "OgreALPrereqs.h"

namespace OgreAL {
	// Predeclare so we don't need to include OgreALSound.h
	class Sound;

	class OgreAL_Export MemberFunctionSlot
	{
	public:
		virtual ~MemberFunctionSlot(){};
		virtual void execute(Sound* source) = 0;
	};

	template<typename U>
	class OgreAL_Export MemberFunctionSlot2
	{
	public:
		virtual ~MemberFunctionSlot2()
		{
		};
		virtual void execute(Sound* source, U* data) = 0;
	};

	template<typename T>
	class MemberFunctionPointer : public MemberFunctionSlot
	{
	public:
		typedef void (T::*MemberFunction)(Sound* source);

		MemberFunctionPointer() : mUndefined(true){}

		MemberFunctionPointer(MemberFunction func, T* obj) :
			mFunction(func),
			mObject(obj),
			mUndefined(false)
		{}

		virtual ~MemberFunctionPointer(){}

		void execute(Sound* source)
		{
			if(!mUndefined)
				(mObject->*mFunction)(source);
		}

	protected:
		MemberFunction mFunction;
		T* mObject;
		bool mUndefined;
	};

	template<typename T, typename U>
	class MemberFunctionPointer2 : public MemberFunctionSlot2
	{
	public:
		typedef void (T::* MemberFunction)(Sound* source, U* data);

		MemberFunctionPointer2() : mUndefined(true)
		{
		}

		MemberFunctionPointer2(MemberFunction func, T* obj) :
			mFunction(func),
			mObject(obj),
			mUndefined(false)
		{
		}

		virtual ~MemberFunctionPointer2()
		{
		}

		void execute(Sound* source, U* data)
		{
			if (!mUndefined)
				(mObject->*mFunction)(source, data);
		}

	protected:
		MemberFunction mFunction;
		T* mObject;
		bool mUndefined;
	};
} // Namespace
#endif
