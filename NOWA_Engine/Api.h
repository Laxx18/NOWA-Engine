#ifndef API_H
#define API_H

#include "defines.h"

namespace NOWA
{

//#ifdef __cplusplus
//	class ProjectParameter;
//#else
//	typedef struct ProjectParameter{} ProjectParameter;
//#endif

	///////////////////////////////////////////Callbacks/////////////////////////////////////////////

	typedef void (*HandleSceneLoaded) (bool worldChanged, ProjectParameter projectParameter);

	///////////////////////////////////////////Main Functions////////////////////////////////////////

	//TODO: Like in unity or qt, function to set window handle, so that render window can be embedded in external application like a WPF application.

	EXPORTED int init(const wchar_t* applicationName, const wchar_t* graphicsConfigurationName, const wchar_t* applicationConfigurationName, const wchar_t* startProjectName, int iconId);

	EXPORTED int exit(void);

	EXPORTED void setSceneLoadedCallback(HandleSceneLoaded callback);

}; //namespace end

#endif
