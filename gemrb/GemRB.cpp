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

#include "win32def.h" // logging

#include "Interface.h"

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __APPLE__ // MAC OS X and iOS
// We want to wrap GemRB witin a cocoa environment
// this will declare GemRBs main function as GemRB_main
// and our cocoa main function will call GemRB_main
// our cocoa wrapper is in CocoaWrapper.m
// the plugins (ex SDLViedo) can extend this wrapper using obj-c categories.
extern "C" int GemRB_main(int argc, char *argv[]);
#define main GemRB_main
#endif

#ifdef ANDROID
#include <SDL/SDL.h>
// if/when android moves to SDL 1.3 remove these special functions.
// SDL 1.3 fires window events for these conditions that are handled in SDLVideo.cpp.
// see SDL_WINDOWEVENT_MINIMIZED and SDL_WINDOWEVENT_RESTORED
#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
#include "Audio.h"

// pause audio playing if app goes in background
static void appPutToBackground()
{
  core->GetAudioDrv()->Pause();
}
// resume audio playing if app return to foreground
static void appPutToForeground()
{
  core->GetAudioDrv()->Resume();
}
#endif
#endif

int main(int argc, char* argv[])
{
#ifdef M_TRIM_THRESHOLD
// Prevent fragmentation of the heap by malloc (glibc).
//
// The default threshold is 128*1024, which can result in a large memory usage
// due to fragmentation since we use a lot of small objects. On the other hand
// if the threshold is too low, free() starts to permanently ask the kernel
// about shrinking the heap.
	#ifdef HAVE_UNISTD_H
		int pagesize = sysconf(_SC_PAGESIZE);
	#else
		int pagesize = 4*1024;
	#endif
	mallopt(M_TRIM_THRESHOLD, 5*pagesize);
#endif
	using namespace GemRB;

	Interface::SanityCheck(VERSION_GEMRB);
	InitializeLogging();

	core = new Interface( argc, argv );
	if (core->Init() == GEM_ERROR) {
		delete( core );
		Log(MESSAGE, "Main", "Press enter to continue...");
		getc(stdin);
		ShutdownLogging();
		return -1;
	}
#ifdef ANDROID
#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
    SDL_ANDROID_SetApplicationPutToBackgroundCallback(&appPutToBackground, &appPutToForeground);
#endif
#endif
	core->Main();
	delete( core );
	ShutdownLogging();
	return 0;
}
