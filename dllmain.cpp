/**
* Copyright (C) 2017 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#include "cfg.h"
#include "dllmain.h"
#include "dxwrapper.h"
#include "wrappers\wrapper.h"
#include "DDrawCompatExternal.h"
#include "DxWndExternal.h"
#include "DSoundCtrl\DSoundCtrlExternal.h"
#include "utils.h"
#include "fullscreen.h"
#include "disasm\initdisasm.h"
#include "writememory\writememory.h"

// Declare varables
HMODULE hModule_dll = NULL;
CRITICAL_SECTION CriticalSection;
screen_res m_ScreenRes;

// Check and store screen resolution
void CallCheckCurrentScreenRes()
{
	CheckCurrentScreenRes(m_ScreenRes);
}

// Clean up tasks and unhook dlls
void RunExitFunctions(bool ForceTerminate)
{
	// Release resources used by the critical section object.
	DeleteCriticalSection(&CriticalSection);

	// *** Force termination ***
	if (ForceTerminate)
	{
		Compat::Log() << "Process not exiting, attempting to terminate process...";

		// Resetting screen resolution to fix some display errors on exit
		if (Config.ResetScreenRes)
		{
			SetScreen(m_ScreenRes);		// Should be run after StopThread and before unloading anything else
			ResetScreen();				// Reset screen back to original Windows settings
		}

		// Terminate the current process
		HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
		Compat::Log() << "Terminating process!";
		TerminateProcess(processHandle, 0);
	}

	// *** Normal exit ***
	Compat::Log() << "Quiting dxwrapper";

	// Stop threads
	StopFullscreenThread();
	StopWriteMemoryThread();

	// Setting screen resolution before exit
	if (Config.ResetScreenRes) SetScreen(m_ScreenRes);		// Should be run after StopThread and before unloading anything else

	// Unload and Unhook DxWnd
	if (Config.DxWnd)
	{
		Compat::Log() << "Unloading dxwnd";
		DxWndEndHook();
	}

	// Unload and Unhook DxWnd
	if (Config.DDrawCompat) UnloadDdrawCompat();

	// Unload dlls
	DllDetach();

	// Clean up memory
	Config.CleanUp();

	// Unload exception handler
	if (Config.HandleExceptions) UnHookExceptionHandler();

	// Resetting screen resolution to fix some display errors on exit
	if (Config.ResetScreenRes) ResetScreen();				// Reset screen back to original Windows settings

	// Final log
	Compat::Log() << "dxwrapper terminated!";
}

// Dll main function
bool APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	static bool FullscreenThreadStartedFlag = false;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		// Init logs
		Compat::Log() << "Starting dxwrapper v" << APP_VERSION;
		GetOSVersion();
		GetProcessNameAndPID();

		// Get handle
		hModule_dll = hModule;
		HANDLE hCurrentThread = GetCurrentThread();

		// Initialize the critical section one time only.
		InitializeCriticalSectionAndSpinCount(&CriticalSection, 0);

		// Set thread priority
		SetThreadPriority(hCurrentThread, THREAD_PRIORITY_HIGHEST);		// Trick to reduce concurrency problems at program startup

		// Initialize config
		Config.Init();

		// Launch processes
		if (Config.szShellPath[0] != '\0') Shell(Config.szShellPath);

		// Set application compatibility options
		if (Config.AddressPointerCount > 0 && Config.BytesToWriteCount > 0) WriteMemory();
		if (Config.HandleExceptions) HookExceptionHandler();
		if (Config.DpiAware) DisableHighDPIScaling();
		SetAppCompat();
		if (Config.Affinity) SetProcessAffinityMask(GetCurrentProcess(), 1);

		// Attach real wrapper dll
		DllAttach();

		// Start compatibility modules
		if (Config.DDrawCompat) Config.DDrawCompat = StartDdrawCompat(hModule_dll);
		if (Config.DSoundCtrl) RunDSoundCtrl();
		if (Config.DxWnd)
		{
			// Check if dxwnd.dll exists then load it
			if (LoadLibrary("dxwnd.dll"))
			{
				Compat::Log() << "Loading dxwnd";
				InitDxWnd();
			}
			// If dxwnd.dll does not exist than disable dxwnd setting
			else
			{
				Config.DxWnd = false;
			}
		}

		// Start fullscreen thread
		if (Config.FullScreen || Config.ForceTermination) StartFullscreenThread();

		// Resetting thread priority
		SetThreadPriority(hCurrentThread, THREAD_PRIORITY_NORMAL);

		// Closing handle
		CloseHandle(hCurrentThread);
	}
	break;
	case DLL_THREAD_ATTACH:
		// Check and store screen resolution
		if (Config.ResetScreenRes) CheckCurrentScreenRes(m_ScreenRes);

		// Check if thread has started
		if (Config.ForceTermination && IsFullscreenThreadRunning())
			FullscreenThreadStartedFlag = true;
		break;
	case DLL_THREAD_DETACH:
		if (Config.ForceTermination)
		{
			// Check if thread has started
			if (IsFullscreenThreadRunning()) FullscreenThreadStartedFlag = true;

			// Check if thread has stopped
			if (FullscreenThreadStartedFlag && !IsFullscreenThreadRunning()) RunExitFunctions(true);
		}
		break;
	case DLL_PROCESS_DETACH:
		// Run all clean up functions
		RunExitFunctions();
		break;
	}
	return true;
}