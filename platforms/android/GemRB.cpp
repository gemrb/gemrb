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
 *
 */

// GemRB.cpp : Defines the entry point for the application.

#include <clocale> //language encoding
#include <SDL.h>

#include "AndroidLogger.h"
#include "Interface.h"

// if/when android moves to SDL 1.3 remove these special functions.
// SDL 1.3 fires window events for these conditions that are handled in SDLVideo.cpp.
// see SDL_WINDOWEVENT_MINIMIZED and SDL_WINDOWEVENT_RESTORED
#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
#include "Audio.h"

// pause audio playing if app goes in background
static void appPutToBackground()
{
  GemRB::core->GetAudioDrv()->Pause();
}
// resume audio playing if app return to foreground
static void appPutToForeground()
{
  GemRB::core->GetAudioDrv()->Resume();
}

#endif

using namespace GemRB;

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");
	setenv("SDL_VIDEO_X11_WMCLASS", argv[0], 0);
	setenv("GEMRB_DATA", SDL_AndroidGetExternalStoragePath(), 1);

	// Prevent fragmentation of the heap by malloc (glibc).
	// The default threshold is 128*1024, which can result in a large memory usage
	// due to fragmentation since we use a lot of small objects. On the other hand
	// if the threshold is too low, free() starts to permanently ask the kernel
	// about shrinking the heap.
	#if defined(HAVE_UNISTD_H)
		int pagesize = sysconf(_SC_PAGESIZE);
	#else
		int pagesize = 4*1024;
	#endif
	setenv("MALLOC_TRIM_THRESHOLD_", fmt::format("{}", 5 * pagesize), 1);
	
	AddLogWriter(createAndroidLogger());
	ToggleLogging(true);

	Interface::SanityCheck(VERSION_GEMRB);

	core = new Interface();
	
	if (core->Init(LoadFromArgs(argc, argv)) == GEM_ERROR) {
		delete core;
		Log(MESSAGE, "Main", "Aborting due to fatal error...");

		return -1;
	}
	delete config;

#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
    SDL_ANDROID_SetApplicationPutToBackgroundCallback(&appPutToBackground, &appPutToForeground);
#endif

	core->Main();
	delete( core );
	return 0;
}
