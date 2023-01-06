/* Copyright (c) <2003-2019> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef __D_MODEL_STDAFX__
#define __D_MODEL_STDAFX__

#include <stdlib.h>


#ifdef _MSC_VER
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#endif
	#include <windows.h>
	#include <crtdbg.h>
#endif


#ifdef _MACOSX_VER
	#ifndef _MAC_IPHONE
		#include <semaphore.h>
	#endif	

	#include <unistd.h>
	#include <libkern/OSAtomic.h>
	#include <sys/sysctl.h>
#endif

#include <Newton.h>
#include <dStdAfxMath.h>
#include <dContainersStdAfx.h>
#include <dCustomJointLibraryStdAfx.h>

#include <dMathDefines.h>
#include <dVector.h>
#include <dMatrix.h>
#include <dQuaternion.h>
#include <dLinearAlgebra.h>
 
#include <dCRC.h>
#include <dHeap.h>
#include <dList.h>
#include <dTree.h>
#include <dRtti.h>
#include <dArray.h>
#include <dPointer.h>
#include <dClassInfo.h>
#include <dRefCounter.h>

#include <dCustomJoint.h>
#include <dCustomListener.h>
//#include <dCustomBallAndSocket.h>
#include <dCustomKinematicController.h>

// some utilities functions
int Calculate2dConvexHullProjection(int count, dVector* const points);



#endif
