/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// GemRB.cpp : Defines the entry point for Xbox application.

#include "Interface.h"
#include "XboxLogger.h"

#include <Python.h>
#include <SDL.h>
#include <clocale> //language encoding

#ifdef XBOX
#include <hal/debug.h>
#include <xboxkrnl/xboxkrnl.h>
#include <windows.h>
#endif

// Memory allocation for Xbox - use conservative values due to 64MB limit
#ifdef XBOX
// Reserve about 48MB for GemRB, leaving 16MB for system
static const size_t XBOX_HEAP_SIZE = 48 * 1024 * 1024;
#endif

char* xboxArgv[3];
char configPath[256];

void XboxSetArguments(int* argc, char** argv[])
{
#ifdef XBOX
	int xboxArgc = 1;
	xboxArgv[0] = (char*) "gemrb";
	
	// Check if config file exists and use it
	HANDLE hFile = CreateFile("E:\\GemRB\\GemRB.cfg", 
		GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		xboxArgv[1] = (char*) "-c";
		strcpy(configPath, "E:\\GemRB\\GemRB.cfg");
		xboxArgv[2] = configPath;
		xboxArgc = 3;
	}
	
	*argc = xboxArgc;
	*argv = xboxArgv;
#else
	// Non-Xbox fallback
	*argc = 1;
	*argv = xboxArgv;
	xboxArgv[0] = (char*) "gemrb";
#endif
}

using namespace GemRB;

int main(int argc, char* argv[])
{
#ifdef XBOX
	// Initialize Xbox-specific systems
	HalInitiateShutdown();
	
	// Set up minimal memory management
	// Xbox has limited memory, so we need to be conservative
	
	// Initialize debug output early
	debugPrint("GemRB starting on Xbox...\n");
#endif

	// Selecting game config from Xbox filesystem
	XboxSetArguments(&argc, &argv);

	setlocale(LC_ALL, "");

	AddLogWriter(createXboxLogger());
	ToggleLogging(true);

	SanityCheck();

#ifdef XBOX
	// Minimize Python overhead for Xbox's limited memory
	Py_NoSiteFlag = 1;
	Py_IgnoreEnvironmentFlag = 1;
	Py_NoUserSiteDirectory = 1;
	Py_OptimizeFlag = 2; // Enable optimization to reduce memory usage
#endif

	try {
		Interface gemrb(LoadFromArgs(argc, argv));
		gemrb.Main();
	} catch (CoreInitializationException& cie) {
		Log(FATAL, "Main", "Aborting due to fatal error... {}", cie);
		ToggleLogging(false);
#ifdef XBOX
		debugPrint("GemRB fatal error - shutting down Xbox\n");
		HalInitiateShutdown();
#endif
		return GEM_ERROR;
	}

#ifdef XBOX
	// Xbox-specific cleanup
	debugPrint("GemRB shutting down normally\n");
#endif

	VideoDriver.reset();
	ToggleLogging(false);

#ifdef XBOX
	// Proper Xbox shutdown
	HalInitiateShutdown();
#endif

	return 0;
}