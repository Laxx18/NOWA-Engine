
#ifndef NO_DLL
	#ifdef DLL_EXPORTS
	#define EXPORTS __declspec(dllexport)
	#else
	#define EXPORTS __declspec(dllimport)
	#endif
#else
	#define EXPORTS
#endif