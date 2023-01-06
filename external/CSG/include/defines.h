
#ifdef DLL_EXPORTS
#define EXPORTS __declspec(dllexport)
#else
#define EXPORTS __declspec(dllimport)
#endif