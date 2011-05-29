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

//this supposed to convince SDL to work on OS/X
//WARNING: commenting this out will cause SDL 1.2.x to crash
#ifdef __APPLE_CC__ // we need startup SDL here
#include <SDL.h>
#endif

#ifdef ANDROID
#include <SDL/SDL.h>
#include "audio.h"

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

	Interface::SanityCheck(VERSION_GEMRB);
	core = new Interface( argc, argv );
	if (core->Init() == GEM_ERROR) {
		delete( core );
		print("Press enter to continue...");
		textcolor(DEFAULT);
		getc(stdin);
		return -1;
	}
#ifdef ANDROID
    SDL_ANDROID_SetApplicationPutToBackgroundCallback(&appPutToBackground, &appPutToForeground);
#endif
	core->Main();
	delete( core );
	textcolor(DEFAULT);
	return 0;
}
