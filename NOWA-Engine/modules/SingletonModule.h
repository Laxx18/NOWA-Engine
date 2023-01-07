#ifndef SINGLETON_MODULE_H
#define SINGLETON_MODULE_H

#include <defines.h>

namespace NOWA
{
	struct EXPORTED DefaultSingletonConcurrencyPolicy
	{
		/**
		* Placeholder function for locking a mutex, thereby preventing access to other threads. This
		* default implementation does not perform any function, the derived class must provide an
		* alternate implementation if this functionality is desired.
		*/
		static void lockMutex()
		{
			/* default implementation does nothing */
			return;
		}

		/**
		* Placeholder function for unlocking a mutex, thereby allowing access to other threads. This
		* default implementation does not perform any function, the derived class must provide an
		* alternate implementation if this functionality is desired.
		*/
		static void unlockMutex()
		{
			/* default implementation does nothing */
			return;
		}

		/**
		* Placeholder function for executing a memory barrier instruction, thereby preventing the
		* compiler from reordering read and writes across this boundary. This default implementation does
		* not perform any function, the derived class must provide an alternate implementation if this
		* functionality is desired.
		*/
		static void memoryBarrier()
		{
			/* default implementation does nothing */
			return;
		}
	};

	/**
	* @brief Singleton design pattern implementation using a dynamically allocated singleton instance.
	*
	* The SingletonDynamic class is intended for use as a base for classes implementing the Singleton
	* design pattern and that dynamic allocation of the singleton object. The default implementation
	* is not thread-safe; however, the class uses a policy-based design pattern that allows the derived
	* classes to achieve threaad-safety by providing an alternate implementation of the
	* ConcurrencyPolicy.
	*
	* @tparam T The type name of the derived (singleton) class
	* @tparam ConcurrencyPolicy The policy implementation for providing thread-safety
	*
	* @note The derived class must have a no-throw default constructor and a no-throw destructor.
	* @note The derived class must list this class as a friend, since, by necessity, the derived class'
	*       constructors must be protected / private.
	*/

	template<typename InstanceType, typename ConcurrencyPolicy = DefaultSingletonConcurrencyPolicy >
	class EXPORTED SingletonModule : public ConcurrencyPolicy

	protected:
		SingletonModule(void) { }

		virtual ~SingletonModule(void)
		{ 
			ConcurrencyPolicy::lockMutex();
			if (SingletonDynamic< T, ConcurrencyPolicy >::pInstance != NULL) {
				delete SingletonDynamic< T, ConcurrencyPolicy >::pInstance;
			}
			ConcurrencyPolicy::unlockMutex();
		}

		/**
		* @brief Factory function for vending mutable references to the sole instance of the singleton object.
		* @return A mutable reference to the one and only instance of the singleton object.
		*/
	public:
		static InstanceType& getInstance(void)
		{
			return (*SingletonModule<InstanceType, ConcurrencyPolicy>::getInternalInstance());
		}
	private:
		static InstanceType getInternalInstance(void)
		{
			if (SingletonModule<InstanceType, ConcurrencyPolicy>::flag == false) {
				ConcurrencyPolicy::lockMutex();

				if (SingletonModule<InstanceType, ConcurrencyPolicy>::instance == NULL) {
					SingletonModule<InstanceType, ConcurrencyPolicy>::instance = new InstanceType();
				}

				ConcurrencyPolicy::unlockMutex();
				ConcurrencyPolicy::memoryBarrier();

				SingletonModule<InstanceType, ConcurrencyPolicy>::flag = true;
			} else {
				ConcurrencyPolicy::memoryBarrier();

				return SingletonModule<InstanceType, ConcurrencyPolicy>::instance;
			}
		}
	
	private:
		// do not copy the singleton
		SingletonModule(SingletonModule const&);
		void operator=(SingletonModule const&);
		SingletonModule& operator=(SingletonModule const &);
	private:
		static InstanceType* instance;

		static volatile bool flag;
	};

	template<typename InstanceType, typename ConcurrencyPolicy >
	TypeName* SingletonModule<TypeName, ConcurrencyPolicy>::instance = NULL;

	template<typename InstanceType, typename ConcurrencyPolicy >
	volatile bool SingletonModule<InstanceType, ConcurrencyPolicy>::flag = false;

}; //namespace end

#endif
