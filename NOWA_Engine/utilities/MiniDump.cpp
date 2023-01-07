//========================================================================
// Minidump.cpp : This is a crash trapper - similar to a UNIX style core dump
//
// Part of the GameCode4 Application
//
// GameCode4 is the sample application that encapsulates much of the source code
// discussed in "Game Coding Complete - 4th Edition" by Mike McShaffry and David
// "Rez" Graham, published by Charles River Media. 
// ISBN-10: 1133776574 | ISBN-13: 978-1133776574
//
// If this source code has found it's way to you, and you think it has helped you
// in any way, do the authors a favor and buy a new copy of the book - there are 
// detailed explanations in it that compliment this code well. Buy a copy at Amazon.com
// by clicking here: 
//    http://www.amazon.com/gp/product/1133776574/ref=olp_product_details?ie=UTF8&me=&seller=
//
// There's a companion web site at http://www.mcshaffry.com/GameCode/
// 
// The source code is managed and maintained through Google Code: 
//    http://code.google.com/p/gamecode4/
//
// (c) Copyright 2012 Michael L. McShaffry and David Graham
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser GPL v3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See 
// http://www.gnu.org/licenses/lgpl-3.0.txt for more details.
//
// You should have received a copy of the GNU Lesser GPL v3
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//========================================================================

#include "NOWAPrecompiled.h"
#include <time.h>
#include "MiniDump.h"

#pragma comment(lib, "version.lib")


// based on dbghelp.h
typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

namespace NOWA
{

	MiniDumper *MiniDumper::gpDumper = nullptr;

	//
	// MiniDumper::MiniDumper
	//
	MiniDumper::MiniDumper(bool headless)
	{
		// Detect if there is more than one MiniDumper.
		// assert(!gpDumper);

		if (!gpDumper)
		{
			::SetUnhandledExceptionFilter(Handler);
			gpDumper = this;
			m_bHeadless = headless;						// doesn't throw up a dialog, just writes the file.
		}
	}


	//
	// MiniDumper::Handler
	//
	LONG MiniDumper::Handler(_EXCEPTION_POINTERS *pExceptionInfo)
	{
		LONG retval = EXCEPTION_CONTINUE_SEARCH;

		if (!gpDumper)
		{
			return retval;
		}

		return gpDumper->WriteMiniDump(pExceptionInfo);
	}

	//
	// MiniDumper::WriteMiniDump
	//
	LONG MiniDumper::WriteMiniDump(_EXCEPTION_POINTERS *pExceptionInfo)
	{
		time(&m_lTime);

		LONG retval = EXCEPTION_CONTINUE_SEARCH;
		m_pExceptionInfo = pExceptionInfo;

		// You have to find the right dbghelp.dll. 
		// Look next to the EXE first since the one in System32 might be old (Win2k)

		HMODULE hDll = nullptr;
		TCHAR szDbgHelpPath[_MAX_PATH];

		if (GetModuleFileName(nullptr, m_szAppPath, _MAX_PATH))
		{
			TCHAR *pSlash = strrchr(m_szAppPath, '\\');
			if (pSlash)
			{
				strcpy(m_szAppBaseName, pSlash + 1);
				*(pSlash + 1) = 0;
			}
			strcpy(szDbgHelpPath, m_szAppPath);
			strcat(szDbgHelpPath, "DBGHELP.DLL");
			hDll = ::LoadLibrary(szDbgHelpPath);
		}

		if (hDll == nullptr)
		{
			// If we haven't found it yet - try one more time.
			hDll = ::LoadLibrary("DBGHELP.DLL");
		}

		LPCTSTR szResult = nullptr;

		if (hDll)
		{
			MINIDUMPWRITEDUMP pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
			if (pMiniDumpWriteDump)
			{
				TCHAR szScratch[USER_DATA_BUFFER_SIZE];

				VSetDumpFileName();

				// ask the user if they want to save a dump file
				sprintf(szScratch, "There was an unexpected error:\n\n%s\nWould you like to save a diagnostic file?\n\nFilename: %s", VGetUserMessage(), m_szDumpPath);
				if (m_bHeadless || (::MessageBox(nullptr, szScratch, nullptr, MB_YESNO) == IDYES))
				{
					// create the file
					HANDLE hFile = ::CreateFile(m_szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL, nullptr);

					if (hFile != INVALID_HANDLE_VALUE)
					{
						_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

						ExInfo.ThreadId = ::GetCurrentThreadId();
						ExInfo.ExceptionPointers = pExceptionInfo;
						ExInfo.ClientPointers = 0;

						// write the dump
						BOOL bOK = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, VGetUserStreamArray(), 0);
						if (bOK)
						{
							szResult = nullptr;
							retval = EXCEPTION_EXECUTE_HANDLER;
						}
						else
						{
							sprintf(szScratch, "Failed to save dump file to '%s' (error %d)", m_szDumpPath, GetLastError());
							szResult = szScratch;
						}
						::CloseHandle(hFile);
					}
					else
					{
						sprintf(szScratch, "Failed to create dump file '%s' (error %d)", m_szDumpPath, GetLastError());
						szResult = szScratch;
					}
				}
			}
			else
			{
				szResult = "DBGHELP.DLL too old";
			}
		}
		else
		{
			szResult = "DBGHELP.DLL not found";
		}

		if (szResult && !m_bHeadless)
			::MessageBox(0, szResult, 0, MB_OK);

		TerminateProcess(GetCurrentProcess(), 0);

		// MLM Note: ExitThread will work, and it allows the MiniDumper to kill a crashed thread
		// without affecting the rest of the application. The question of the day:
		//   Is That A Good Idea??? Answer: ABSOLUTELY NOT!!!!!!!
		//
		//ExitThread(0);

		return retval;
	}

	//
	// MiniDumper::VSetDumpFileName
	//
	void MiniDumper::VSetDumpFileName(void)
	{
		sprintf(m_szDumpPath, "%s%s.%ld.dmp", m_szAppPath, m_szAppBaseName, m_lTime);
	}

}; // namespace end