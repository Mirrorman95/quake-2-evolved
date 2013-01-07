/*
 ------------------------------------------------------------------------------
 Copyright (C) 1997-2001 Id Software.

 This file is part of the Quake 2 source code.

 The Quake 2 source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

 The Quake 2 source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 the Quake 2 source code; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ------------------------------------------------------------------------------
*/


//
// win_system.c - User system-specific code
//


#include "../common/common.h"
#include "win_local.h"
#include <float.h>


static cvar_t *				sys_osVersion;
static cvar_t *				sys_processor;
static cvar_t *				sys_memory;
static cvar_t *				sys_videoCard;
static cvar_t *				sys_soundCard;
static cvar_t *				sys_userName;

sysWin_t					sys;


/*
 ==============================================================================

 SPLASH SCREEN

 ==============================================================================
*/


/*
 ==============================================================================

 FILE SYSTEM

 ==============================================================================
*/

#define MAX_LIST_FILES				32768


/*
 ==================
 Sys_SortFileList
 ==================
*/
static int Sys_SortFileList (const void *elem1, const void *elem2){

	const char	*name1 = *(const char **)elem1;
	const char	*name2 = *(const char **)elem2;

	return Str_Compare(name1, name2);
}

/*
 ==================
 Sys_AddFileToList
 ==================
*/
static int Sys_AddFileToList (const char *name, const char **files, int fileCount){

	if (fileCount == MAX_LIST_FILES)
		return fileCount;	// Too many files

	// Add it
	files[fileCount++] = Mem_DupString(name, TAG_TEMPORARY);

	return fileCount;
}

/*
 ==================
 Sys_ListFiles
 ==================
*/
const char **Sys_ListFiles (const char *directory, const char *extension, bool sort, int *numFiles){

	WIN32_FIND_DATA	findInfo;
	HANDLE			findHandle;
	BOOL			findResult = TRUE;
	bool			listDirectories, listFiles;
	char			name[MAX_PATH_LENGTH];
	const char		**fileList;
	const char		*files[MAX_LIST_FILES];
	int				fileCount = 0;
	int				i;

	if (extension != NULL && !Str_Compare(extension, "/")){
		listDirectories = true;
		listFiles = false;
	}
	else {
		listDirectories = false;
		listFiles = true;
	}

	Str_SPrintf(name, sizeof(name), "%s/*", directory);

	// Find the first file
	findHandle = FindFirstFile(name, &findInfo);
	if (findHandle == INVALID_HANDLE_VALUE){
		*numFiles = 0;
		return NULL;
	}

	while (findResult == TRUE){
		// Check for invalid file name
		if (!Str_Compare(findInfo.cFileName, ".") || !Str_Compare(findInfo.cFileName, "..")){
			findResult = FindNextFile(findHandle, &findInfo);
			continue;
		}

		// Ignore certain directories
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			if (!Str_ICompare(findInfo.cFileName, CODE_DIRECTORY) || !Str_ICompare(findInfo.cFileName, DOCS_DIRECTORY) || !Str_ICompare(findInfo.cFileName, SVN_DIRECTORY)){
				findResult = FindNextFile(findHandle, &findInfo);
				continue;
			}
		}

		// Add it
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			if (listDirectories)
				fileCount = Sys_AddFileToList(findInfo.cFileName, files, fileCount);
		}
		else {
			if (listFiles){
				if (extension == NULL)
					fileCount = Sys_AddFileToList(findInfo.cFileName, files, fileCount);
				else {
					Str_ExtractFileExtension(findInfo.cFileName, name, sizeof(name));

					if (!Str_ICompare(extension, name))
						fileCount = Sys_AddFileToList(findInfo.cFileName, files, fileCount);
				}
			}
		}

		// Find the next file
		findResult = FindNextFile(findHandle, &findInfo);
	}

	FindClose(findHandle);

	if (!fileCount){
		*numFiles = 0;
		return NULL;
	}

	// Sort the list if desired
	if (sort)
		qsort(files, fileCount, sizeof(char *), Sys_SortFileList);

	// Copy the list
	fileList = (const char **)Mem_Alloc((fileCount + 1) * sizeof(char *), TAG_TEMPORARY);

	for (i = 0; i < fileCount; i++)
		fileList[i] = files[i];

	fileList[i] = NULL;

	*numFiles = fileCount;

	return fileList;
}

/*
 ==================
 Sys_RecursiveListFilteredFiles
 ==================
*/
static int Sys_RecursiveListFilteredFiles (const char *directory, const char *subdirectory, const char *filter, const char **files, int fileCount){

	WIN32_FIND_DATA	findInfo;
	HANDLE			findHandle;
	BOOL			findResult = TRUE;
	char			name[MAX_PATH_LENGTH];

	if (subdirectory)
		Str_SPrintf(name, sizeof(name), "%s/%s/*", directory, subdirectory);
	else
		Str_SPrintf(name, sizeof(name), "%s/*", directory);

	// Find the first file
	findHandle = FindFirstFile(name, &findInfo);
	if (findHandle == INVALID_HANDLE_VALUE)
		return fileCount;

	while (findResult == TRUE){
		// Check for invalid file name
		if (!Str_Compare(findInfo.cFileName, ".") || !Str_Compare(findInfo.cFileName, "..")){
			findResult = FindNextFile(findHandle, &findInfo);
			continue;
		}

		// Ignore certain directories
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			if (!Str_ICompare(findInfo.cFileName, CODE_DIRECTORY) || !Str_ICompare(findInfo.cFileName, DOCS_DIRECTORY) || !Str_ICompare(findInfo.cFileName, SVN_DIRECTORY)){
				findResult = FindNextFile(findHandle, &findInfo);
				continue;
			}
		}

		// Copy the name
		if (subdirectory)
			Str_SPrintf(name, sizeof(name), "%s/%s", subdirectory, findInfo.cFileName);
		else
			Str_SPrintf(name, sizeof(name), "%s", findInfo.cFileName);

		// If a directory, recurse into it
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			fileCount = Sys_RecursiveListFilteredFiles(directory, name, filter, files, fileCount);

		// Match filter
		if (!Str_MatchFilter(name, filter, false)){
			findResult = FindNextFile(findHandle, &findInfo);
			continue;
		}

		// Add it
		fileCount = Sys_AddFileToList(name, files, fileCount);

		// Find the next file
		findResult = FindNextFile(findHandle, &findInfo);
	}

	FindClose(findHandle);

	return fileCount;
}

/*
 ==================
 Sys_ListFilteredFiles
 ==================
*/
const char **Sys_ListFilteredFiles (const char *directory, const char *filter, bool sort, int *numFiles){

	const char	**fileList;
	const char	*files[MAX_LIST_FILES];
	int			fileCount = 0;
	int			i;

	// List files
	fileCount = Sys_RecursiveListFilteredFiles(directory, NULL, filter, files, 0);
	if (!fileCount){
		*numFiles = 0;
		return NULL;
	}

	// Sort the list if desired
	if (sort)
		qsort(files, fileCount, sizeof(char *), Sys_SortFileList);

	// Copy the list
	fileList = (const char **)Mem_Alloc((fileCount + 1) * sizeof(char *), TAG_TEMPORARY);

	for (i = 0; i < fileCount; i++)
		fileList[i] = files[i];

	fileList[i] = NULL;

	*numFiles = fileCount;

	return fileList;
}

/*
 ==================
 Sys_FreeFileList
 ==================
*/
void Sys_FreeFileList (const char **fileList){

	int		i;

	if (!fileList)
		return;

	for (i = 0; fileList[i]; i++)
		Mem_Free(fileList[i]);

	Mem_Free(fileList);
}

/*
 ==================
 Sys_CreateDirectory
 ==================
*/
void Sys_CreateDirectory (const char *directory){

	CreateDirectory(directory, NULL);
}

/*
 ==================
 Sys_RemoveDirectory
 ==================
*/
void Sys_RemoveDirectory (const char *directory){

	WIN32_FIND_DATA	findInfo;
	HANDLE			findHandle;
	BOOL			findResult = TRUE;
	char			name[MAX_PATH_LENGTH];

	Str_SPrintf(name, sizeof(name), "%s/*", directory);

	// Find the first file
	findHandle = FindFirstFile(name, &findInfo);
	if (findHandle == INVALID_HANDLE_VALUE){
		RemoveDirectory(directory);
		return;
	}

	while (findResult == TRUE){
		// Check for invalid file name
		if (!Str_Compare(findInfo.cFileName, ".") || !Str_Compare(findInfo.cFileName, "..")){
			findResult = FindNextFile(findHandle, &findInfo);
			continue;
		}

		// Ignore certain directories
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			if (!Str_ICompare(findInfo.cFileName, CODE_DIRECTORY) || !Str_ICompare(findInfo.cFileName, DOCS_DIRECTORY) || !Str_ICompare(findInfo.cFileName, SVN_DIRECTORY)){
				findResult = FindNextFile(findHandle, &findInfo);
				continue;
			}
		}

		// Copy the name
		Str_SPrintf(name, sizeof(name), "%s/%s", directory, findInfo.cFileName);

		// If a directory, recurse into it
		if (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			Sys_RemoveDirectory(name);

		// If a file, remove it
		if (!(findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			DeleteFile(name);

		// Find the next file
		findResult = FindNextFile(findHandle, &findInfo);
	}

	FindClose(findHandle);

	// Remove this directory
	RemoveDirectory(directory);
}

/*
 ==================
 Sys_CurrentDirectory
 ==================
*/
const char *Sys_CurrentDirectory (){

	return sys.currentDirectory;
}

/*
 ==================
 Sys_DefaultBaseDirectory
 ==================
*/
const char *Sys_DefaultBaseDirectory (){

	return sys.currentDirectory;
}

/*
 ==================
 Sys_DefaultSaveDirectory
 ==================
*/
const char *Sys_DefaultSaveDirectory (){

	return sys.currentDirectory;
}

/*
 ==================
 Sys_DiskFreeSpace
 ==================
*/
int Sys_DiskFreeSpace (const char *directory){

	ulonglong	freeBytes;
	int			freeMegs;

	if (!GetDiskFreeSpaceEx(directory, (ULARGE_INTEGER *)&freeBytes, NULL, NULL))
		return 0;

	freeMegs = (int)(freeBytes >> 20);

	return freeMegs;
}


/*
 ==============================================================================

 DYNAMIC LIBRARY LOADING

 ==============================================================================
*/


/*
 ==================
 Sys_LoadLibrary
 ==================
*/
void *Sys_LoadLibrary (const char *libPath){

	return LoadLibrary(libPath);
}

/*
 ==================
 Sys_FreeLibrary
 ==================
*/
void Sys_FreeLibrary (void *libHandle){

	if (!libHandle)
		Com_Error(ERR_FATAL, "Sys_FreeLibrary: invalid libHandle");

	if (!FreeLibrary((HMODULE)libHandle))
		Com_Error(ERR_FATAL, "Sys_FreeLibrary: failed to free library");
}

/*
 ==================
 Sys_GetProcAddress
 ==================
*/
void *Sys_GetProcAddress (void *libHandle, const char *procName){

	if (!libHandle)
		Com_Error(ERR_FATAL, "Sys_GetProcAddress: invalid libHandle");

	return GetProcAddress((HMODULE)libHandle, procName);
}


/*
 ==============================================================================

 PROCESSOR DETECTION

 ==============================================================================
*/


/*
 ==================
 Sys_DetectProcessor
 ==================
*/
static void Sys_DetectProcessor (){

	SYSTEM_INFO	systemInfo;
	char		cpuVendor[16];
	int			cpuInfo[4];
	int			stdBits[4], extBits[4];

	// Get the CPU vendor string
	__cpuid(cpuInfo, 0x00000000);

	((int *)cpuVendor)[0] = cpuInfo[1];
	((int *)cpuVendor)[1] = cpuInfo[3];
	((int *)cpuVendor)[2] = cpuInfo[2];
	((int *)cpuVendor)[3] = 0;

	// Get the standard feature bits
	__cpuid(stdBits, 0x00000001);

	// Get the extended feature bits
	__cpuid(extBits, 0x80000001);

	// Set the CPU id
	if (!Str_Compare(cpuVendor, "AuthenticAMD"))
		sys.cpuId = CPUID_AMD;
	else if (!Str_Compare(cpuVendor, "GenuineIntel"))
		sys.cpuId = CPUID_INTEL;
	else
		sys.cpuId = CPUID_GENERIC;

	if (stdBits[3] & BIT(23))
		sys.cpuId |= CPUID_MMX;
	if (extBits[3] & BIT(22))
		sys.cpuId |= CPUID_MMX_EXT;
	if (extBits[3] & BIT(31))
		sys.cpuId |= CPUID_3DNOW;
	if (extBits[3] & BIT(30))
		sys.cpuId |= CPUID_3DNOW_EXT;
	if (stdBits[3] & BIT(25))
		sys.cpuId |= CPUID_SSE;
	if (stdBits[3] & BIT(26))
		sys.cpuId |= CPUID_SSE2;
	if (stdBits[2] & BIT(0))
		sys.cpuId |= CPUID_SSE3;
	if (stdBits[2] & BIT(9))
		sys.cpuId |= CPUID_SSSE3;
	if (stdBits[2] & BIT(19))
		sys.cpuId |= CPUID_SSE41;
	if (stdBits[2] & BIT(20))
		sys.cpuId |= CPUID_SSE42;
	if (extBits[2] & BIT(6))
		sys.cpuId |= CPUID_SSE4A;
	if (extBits[2] & BIT(11))
		sys.cpuId |= CPUID_XOP;
	if (extBits[2] & BIT(16))
		sys.cpuId |= CPUID_FMA4;
	if (stdBits[2] & BIT(28))
		sys.cpuId |= CPUID_AVX;

	// Set the CPU count
	GetSystemInfo(&systemInfo);

	sys.cpuCount = systemInfo.dwNumberOfProcessors;
	if (sys.cpuCount < 1)
		sys.cpuCount = 1;

	// Set the CPU string
	if (sys.cpuId & CPUID_AMD)
		Str_Copy(sys.cpuString, "AMD", sizeof(sys.cpuString));
	else if (sys.cpuId & CPUID_INTEL)
		Str_Copy(sys.cpuString, "Intel", sizeof(sys.cpuString));
	else
		Str_Copy(sys.cpuString, "Generic", sizeof(sys.cpuString));

	if (sys.cpuCount != 1)
		Str_Append(sys.cpuString, Str_VarArgs(" (%i CPUs)", sys.cpuCount), sizeof(sys.cpuString));

	if (sys.cpuId & CPUID_MMX){
		Str_Append(sys.cpuString, " w/ MMX", sizeof(sys.cpuString));

		if (sys.cpuId & CPUID_MMX_EXT)
			Str_Append(sys.cpuString, "+", sizeof(sys.cpuString));

		if (sys.cpuId & CPUID_3DNOW){
			Str_Append(sys.cpuString, " & 3DNow", sizeof(sys.cpuString));

			if (sys.cpuId & CPUID_3DNOW_EXT)
				Str_Append(sys.cpuString, "+", sizeof(sys.cpuString));
		}

		if (sys.cpuId & CPUID_SSE)
			Str_Append(sys.cpuString, " & SSE", sizeof(sys.cpuString));
		if (sys.cpuId & CPUID_SSE2)
			Str_Append(sys.cpuString, " & SSE2", sizeof(sys.cpuString));
		if (sys.cpuId & (CPUID_SSE3 | CPUID_SSSE3))
			Str_Append(sys.cpuString, " & SSE3", sizeof(sys.cpuString));
		if (sys.cpuId & (CPUID_SSE41 | CPUID_SSE42 | CPUID_SSE4A))
			Str_Append(sys.cpuString, " & SSE4", sizeof(sys.cpuString));

		if (sys.cpuId & CPUID_XOP)
			Str_Append(sys.cpuString, " & XOP", sizeof(sys.cpuString));
		if (sys.cpuId & CPUID_FMA4)
			Str_Append(sys.cpuString, " & FMA4", sizeof(sys.cpuString));

		if (sys.cpuId & CPUID_AVX)
			Str_Append(sys.cpuString, " & AVX", sizeof(sys.cpuString));
	}
}

/*
 ==================
 Sys_GetProcessorId
 ==================
*/
int Sys_GetProcessorId (){

	return sys.cpuId;
}

/*
 ==================
 Sys_GetProcessorCount
 ==================
*/
int Sys_GetProcessorCount (){

	return sys.cpuCount;
}

/*
 ==================
 Sys_GetProcessorString
 ==================
*/
const char *Sys_GetProcessorString (){

	return sys.cpuString;
}


/*
 ==============================================================================

 MISCELLANEOUS FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 Sys_Print
 ==================
*/
void Sys_Print (const char *text){

	// Print to console
	Sys_ConsolePrint(text);

	// Print to debugger output, if available
	OutputDebugString(text);
}

/*
 ==================
 Sys_Error

 FIXME: this gets a messed up message when the engine crashes
 ==================
*/
void Sys_Error (const char *fmt, ...){

	static bool	onError;
	char		message[MAX_PRINT_MESSAGE];
	va_list		argPtr;
	MSG			msg;

	// If recursively called, quit immediately
	if (onError)
		Sys_Quit();

	onError = true;

	// Get the message
	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	// Shutdown all the subsystems
//	Com_Shutdown();

	// Echo to console
	Sys_Print("\n");
	Sys_Print(message);
	Sys_Print("\n");

	// Show the error message
	Sys_ConsoleError(message);

	// Wait for the user to quit
	while (1){
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
			if (!GetMessage(&msg, NULL, 0, 0))
				Sys_Quit();

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Don't hog the CPU
		Sleep(25);
	}
}

/*
 ==================
 Sys_DebugBreak
 ==================
*/
void Sys_DebugBreak (){

	DebugBreak();
}

/*
 ==================
 Sys_Sleep
 ==================
*/
void Sys_Sleep (int msec){

	if (msec < 0)
		msec = 0;

	Sleep(msec);
}

/*
 ==================
 Sys_ProcessEvents
 ==================
*/
int Sys_ProcessEvents (){

    MSG		msg;

	// Poll the message queue
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
		if (!GetMessage(&msg, NULL, 0, 0))
			Sys_Quit();

		// Special case for the editor window
		if (sys.hWndEditor){
			if (IsDialogMessage(sys.hWndEditor, &msg))
				continue;
		}

		// Translate and dispatch the message
		sys.msgTime = msg.time;

      	TranslateMessage(&msg);
      	DispatchMessage(&msg);
	}

	// Return the current time
	return timeGetTime();
}

/*
 ==================
 Sys_Milliseconds
 ==================
*/
int Sys_Milliseconds (){

	static bool	initialized;
	static int	base;

	if (!initialized){
		// Let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xFFFF0000;
		initialized = true;
	}

	return timeGetTime() - base;
}

/*
 ==================
 Sys_ClockTicks
 ==================
*/
longlong Sys_ClockTicks (){

	longlong	ticks;

	if (!QueryPerformanceCounter((LARGE_INTEGER *)&ticks))
		return 0;

	return ticks;
}

/*
 ==================
 Sys_ClockTicksPerSecond
 ==================
*/
longlong Sys_ClockTicksPerSecond (){

	return sys.ticksPerSecond;
}

/*
 ==================
 Sys_IsWindowActive
 ==================
*/
bool Sys_IsWindowActive (){

	if (!sys.hWndMain || !sys.isActive)
		return false;

	return true;
}

/*
 ==================
 Sys_IsWindowFullscreen
 ==================
*/
bool Sys_IsWindowFullscreen (){

	if (!sys.hWndMain || !sys.isFullscreen)
		return false;

	return true;
}

/*
 ==================
 Sys_SetEditorWindow
 ==================
*/
void Sys_SetEditorWindow (void *wndHandle){

	sys.hWndEditor = (HWND)wndHandle;
}

/*
 ==================
 Sys_GetInstanceHandle
 ==================
*/
void *Sys_GetInstanceHandle (){

	return sys.hInstance;
}

/*
 ==================
 Sys_GetIconHandle
 ==================
*/
void *Sys_GetIconHandle (){

	return sys.hIcon;
}

/*
 ==================
 Sys_GetClipboardText
 ==================
*/
const char *Sys_GetClipboardText (){

	HANDLE		hClipboardData;
	const char	*data, *text;

	if (!IsClipboardFormatAvailable(CF_TEXT))
		return NULL;

	if (!OpenClipboard(NULL))
		return NULL;

	hClipboardData = GetClipboardData(CF_TEXT);
	if (!hClipboardData){
		CloseClipboard();
		return NULL;
	}

	data = (const char *)GlobalLock(hClipboardData);
	if (!data){
		CloseClipboard();
		return NULL;
	}

	text = Mem_DupString(data, TAG_TEMPORARY);
	GlobalUnlock(hClipboardData);

	CloseClipboard();

	return text;
}

/*
 ==================
 Sys_SetClipboardText
 ==================
*/
void Sys_SetClipboardText (const char *text){

	HANDLE	hClipboardData;
	char	*data;
	int		length;

	if (!OpenClipboard(NULL))
		return;

	EmptyClipboard();

	length = Str_Length(text) + 1;

	hClipboardData = GlobalAlloc(GMEM_MOVEABLE, length);
	if (!hClipboardData){
		CloseClipboard();
		return;
	}

	data = (char *)GlobalLock(hClipboardData);
	if (!data){
		GlobalFree(hClipboardData);
		CloseClipboard();
		return;
	}

	Str_Copy(data, text, length);
	GlobalUnlock(hClipboardData);

	SetClipboardData(CF_TEXT, hClipboardData);

	CloseClipboard();
}

/*
 ==================
 Sys_OpenURL
 ==================
*/
void Sys_OpenURL (const char *url, bool quit){

	SHELLEXECUTEINFO	execInfo;

	if (Str_ICompareChars(url, "http://", 7) && Str_ICompareChars(url, "ftp://", 6)){
		Com_Printf(S_COLOR_RED "Could not open URL \"%s\"\n", url);
		return;
	}

	Mem_Fill(&execInfo, 0, sizeof(SHELLEXECUTEINFO));

	execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	execInfo.lpVerb = "open";
	execInfo.lpFile = url;
	execInfo.nShow = SW_SHOW;

	// Open the URL
	Com_Printf("Opening URL \"%s\"...\n", url);

	if (!ShellExecuteEx(&execInfo)){
		Com_Printf(S_COLOR_RED "Could not open URL \"%s\"\n", url);
		return;
	}

	// Quit the program if desired
	if (quit)
		Sys_Quit();
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 Sys_Info_f
 ==================
*/
static void Sys_Info_f (){

	Com_Printf("\n");
	Com_Printf("OS: %s\n", sys_osVersion->value);
	Com_Printf("CPU: %s\n", sys_processor->value);
	Com_Printf("RAM: %s\n", sys_memory->value);
	Com_Printf("Video: %s\n", sys_videoCard->value);
	Com_Printf("Sound: %s\n", sys_soundCard->value);
	Com_Printf("\n");
}


/*
 ==============================================================================

 VIDEO AND SOUND CARD DETECTION

 ==============================================================================
*/


/*
 ==================
 Sys_GetVideoCard
 ==================
*/
static void Sys_GetVideoCard (char *string, int maxLength){

	DISPLAY_DEVICE	displayDevice;
	HKEY			hKey, hKeyCheck;
	char			path[MAX_STRING_LENGTH];
	ulonglong		memorySizeEx;
	ulong			memorySize;
	int				size, max;
	int				i;

	displayDevice.cb = sizeof(DISPLAY_DEVICE);

	if (!EnumDisplayDevices(NULL, 0, &displayDevice, 0)){
		Str_Copy(string, "<UNKNOWN VIDEO CARD>", maxLength);
		return;
	}

	Str_Copy(string, displayDevice.DeviceString, maxLength);

	// Try to get memory size information from the registry
	if (!Str_ICompareChars(displayDevice.DeviceKey, "\\Registry\\Machine\\", 18)){
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, displayDevice.DeviceKey + 18, 0, KEY_READ, &hKeyCheck) != ERROR_SUCCESS)
			return;

		size = sizeof(memorySizeEx);

		if (RegQueryValueEx(hKeyCheck, "HardwareInformation.qwMemorySize", NULL, NULL, (LPBYTE)&memorySizeEx, (LPDWORD)&size) != ERROR_SUCCESS){
			size = sizeof(memorySize);

			if (RegQueryValueEx(hKeyCheck, "HardwareInformation.MemorySize", NULL, NULL, (LPBYTE)&memorySize, (LPDWORD)&size) != ERROR_SUCCESS){
				RegCloseKey(hKeyCheck);
				return;
			}

			memorySizeEx = memorySize;
		}

		RegCloseKey(hKeyCheck);
	}
	else {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\VIDEO", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
			return;

		size = sizeof(max);

		if (RegQueryValueEx(hKey, "MaxObjectNumber", NULL, NULL, (LPBYTE)&max, (LPDWORD)&size) != ERROR_SUCCESS){
			RegCloseKey(hKey);
			return;
		}

		for (i = 0; i <= max; i++){
			size = sizeof(path) - 1;

			if (RegQueryValueEx(hKey, Str_VarArgs("\\Device\\Video%i", i), NULL, NULL, (LPBYTE)path, (LPDWORD)&size) != ERROR_SUCCESS)
				continue;

			if (Str_ICompareChars(path, "\\Registry\\Machine\\", 18)){
				if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKeyCheck) != ERROR_SUCCESS)
					continue;
			}
			else {
				if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path + 18, 0, KEY_READ, &hKeyCheck) != ERROR_SUCCESS)
					continue;
			}

			size = sizeof(memorySizeEx);

			if (RegQueryValueEx(hKeyCheck, "HardwareInformation.qwMemorySize", NULL, NULL, (LPBYTE)&memorySizeEx, (LPDWORD)&size) != ERROR_SUCCESS){
				size = sizeof(memorySize);

				if (RegQueryValueEx(hKeyCheck, "HardwareInformation.MemorySize", NULL, NULL, (LPBYTE)&memorySize, (LPDWORD)&size) != ERROR_SUCCESS){
					RegCloseKey(hKeyCheck);
					continue;
				}

				memorySizeEx = memorySize;
			}

			RegCloseKey(hKeyCheck);

			break;
		}

		RegCloseKey(hKey);

		if (i > max)
			return;
	}

	Str_SPrintf(string, maxLength, "%s w/ %I64u MB", displayDevice.DeviceString, memorySizeEx >> 20);
}

/*
 ==================
 Sys_GetSoundCard
 ==================
*/
static void Sys_GetSoundCard (char *string, int maxLength){

	WAVEOUTCAPS	waveOutCaps;

	if (waveOutGetDevCaps(0, &waveOutCaps, sizeof(WAVEOUTCAPS)) != MMSYSERR_NOERROR){
		Str_Copy(string, "<UNKNOWN SOUND CARD>", maxLength);
		return;
	}

	Str_Copy(string, waveOutCaps.szPname, maxLength);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 Sys_Init
 ==================
*/
void Sys_Init (){

	OSVERSIONINFOEX	osVersionInfo;
	MEMORYSTATUSEX	memoryStatus;
	char			string[MAX_STRING_LENGTH];
	int				length;

	Com_Printf("-------- System Initialization --------\n");

	// Add commands
	Cmd_AddCommand("sysInfo", Sys_Info_f, "Shows system information", NULL);

	// Get OS version info
	osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (!GetVersionEx((OSVERSIONINFO *)&osVersionInfo))
		Com_Error(ERR_FATAL, "Couldn't get OS version info");

	if (osVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
		Com_Error(ERR_FATAL, ENGINE_NAME " requires Windows XP or later");
	if (osVersionInfo.dwMajorVersion < 5 || (osVersionInfo.dwMajorVersion == 5 && osVersionInfo.dwMinorVersion < 1))
		Com_Error(ERR_FATAL, ENGINE_NAME " requires Windows XP or later");

	if (osVersionInfo.dwMajorVersion == 5 && osVersionInfo.dwMinorVersion == 1)
		Str_Copy(string, "Windows XP", sizeof(string));
	else if (osVersionInfo.dwMajorVersion == 5 && osVersionInfo.dwMinorVersion == 2){
		if (osVersionInfo.wProductType == VER_NT_WORKSTATION)
			Str_Copy(string, "Windows XP", sizeof(string));
		else
			Str_Copy(string, "Windows Server 2003", sizeof(string));
	}
	else if (osVersionInfo.dwMajorVersion == 6 && osVersionInfo.dwMinorVersion == 0){
		if (osVersionInfo.wProductType == VER_NT_WORKSTATION)
			Str_Copy(string, "Windows Vista", sizeof(string));
		else
			Str_Copy(string, "Windows Server 2008", sizeof(string));
	}
	else if (osVersionInfo.dwMajorVersion == 6 && osVersionInfo.dwMinorVersion == 1){
		if (osVersionInfo.wProductType == VER_NT_WORKSTATION)
			Str_Copy(string, "Windows 7", sizeof(string));
		else
			Str_Copy(string, "Windows Server 2008 R2", sizeof(string));
	}
	else if (osVersionInfo.dwMajorVersion == 6 && osVersionInfo.dwMinorVersion == 2){
		if (osVersionInfo.wProductType == VER_NT_WORKSTATION)
			Str_Copy(string, "Windows 8", sizeof(string));
		else
			Str_Copy(string, "Windows Server 2012", sizeof(string));
	}
	else
		Str_SPrintf(string, sizeof(string), "Windows NT %i.%i", osVersionInfo.dwMajorVersion, osVersionInfo.dwMinorVersion);

	if (osVersionInfo.szCSDVersion[0])
		Str_Append(string, Str_VarArgs(" (%s)", osVersionInfo.szCSDVersion), sizeof(string));

	Com_Printf("OS: %s\n", string);
	sys_osVersion = CVar_Register("sys_osVersion", string, CVAR_STRING, CVAR_READONLY, "OS version", 0, 0);

	// Detect processor
	Sys_DetectProcessor();

	Com_Printf("CPU: %s\n", sys.cpuString);
	sys_processor = CVar_Register("sys_processor", sys.cpuString, CVAR_STRING, CVAR_READONLY, "CPU string", 0, 0);

	// Get system memory
	memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);

	if (!GlobalMemoryStatusEx(&memoryStatus))
		Com_Error(ERR_FATAL, "Couldn't get memory status");

	Str_SPrintf(string, sizeof(string), "%I64u MB", memoryStatus.ullTotalPhys >> 20);

	Com_Printf("RAM: %s\n", string);
	sys_memory = CVar_Register("sys_memory", string, CVAR_STRING, CVAR_READONLY, "System memory", 0, 0);

	// Get video card
	Sys_GetVideoCard(string, sizeof(string));

	Com_Printf("Video: %s\n", string);
	sys_videoCard = CVar_Register("sys_videoCard", string, CVAR_STRING, CVAR_READONLY, "Video card", 0, 0);

	// Get sound card
	Sys_GetSoundCard(string, sizeof(string));

	Com_Printf("Sound: %s\n", string);
	sys_soundCard = CVar_Register("sys_soundCard", string, CVAR_STRING, CVAR_READONLY, "Sound card", 0, 0);

	// Get user name
	length = sizeof(string);

	if (!GetUserName(string, (LPDWORD)&length))
		Str_Copy(string, "", sizeof(string));

	sys_userName = CVar_Register("sys_userName", string, CVAR_STRING, CVAR_READONLY, "User name", 0, 0);

	// Initialize multimedia timer
	timeBeginPeriod(1);

	// Initialize high-resolution performance timer
	if (!QueryPerformanceFrequency((LARGE_INTEGER *)&sys.ticksPerSecond))
		Com_Error(ERR_FATAL, "No high-resolution performance timer available");

	// Get the current directory
	if (!GetCurrentDirectory(sizeof(sys.currentDirectory), sys.currentDirectory))
		Com_Error(ERR_FATAL, "Couldn't get current directory");

	Com_Printf("Working directory: %s\n", sys.currentDirectory);

#if defined SIMD_X86

	// Make sure the CPU supports the required instruction sets
	if ((sys.cpuId & SIMD_X86_CPUID) != SIMD_X86_CPUID)
		Com_Error(ERR_FATAL, ENGINE_NAME " requires a CPU w/ " SIMD_X86_NAME " for SIMD processing");

	Com_Printf("Using " SIMD_X86_NAME " for SIMD processing\n");

	// Enable Flush-To-Zero and Denormals-Are-Zero modes
	_mm_setcsr(_mm_getcsr() | (_MM_FLUSH_ZERO_MASK | _MM_DENORMALS_ZERO_MASK));

#endif

	Com_Printf("---------------------------------------\n");
}

/*
 ==================
 Sys_Shutdown
 ==================
*/
void Sys_Shutdown (){

	// Remove commands
	Cmd_RemoveCommand("sysInfo");

	// Shutdown multimedia timer
	timeEndPeriod(1);
}

/*
 ==================
 Sys_Quit
 ==================
*/
void Sys_Quit (){

	// Shutdown all the subsystems
	Com_Shutdown();

	// Destroy the system console
	Sys_DestroyConsole();

	// Exit the process
	ExitProcess(0);
}


// ============================================================================


/*
 ==================
 WinMain
 ==================
*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

	// Previous instances do not exist in Win32
	if (hPrevInstance)
		return FALSE;

	// No abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// Get the application instance
	sys.hInstance = hInstance;

	// Load the application icon
	sys.hIcon = LoadIcon(sys.hInstance, MAKEINTRESOURCE(IDI_ICON1));

	// Create the system console
	Sys_CreateConsole();

	// Initialize all the subsystems
	Com_Init(lpCmdLine);

	// Main program loop
	while (1){
		// Don't run at full speed unless needed
		if (!sys.isMinimized || com_dedicated->integerValue)
			Sleep(10);

		// Run a frame
		Com_Frame();
	}

	// Never gets here
    return TRUE;
}