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

#include <cstdio>

#include "Interface.h"


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
#ifdef ANDROID
    SDL_ANDROID_SetApplicationPutToBackgroundCallback(&appPutToBackground, &appPutToForeground);
#endif
	Interface::SanityCheck(VERSION_GEMRB);
	core = new Interface( argc, argv );
	if (core->Init() == GEM_ERROR) {
		delete( core );
		printf("Press enter to continue...");
		textcolor(DEFAULT);
		getc(stdin);
		return -1;
	}
	core->Main();
	delete( core );
	textcolor(DEFAULT);
	return 0;
}
