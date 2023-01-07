#include "NOWAPrecompiled.h"
#include "MyGUIUtilities.h"

namespace NOWA
{
	MyGUIUtilities* MyGUIUtilities::getInstance()
	{
		static MyGUIUtilities instance;
		return &instance;
	}

}; // namespace end