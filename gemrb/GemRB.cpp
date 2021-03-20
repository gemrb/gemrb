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

#include "Interface.h"
#include "System/Logger/Stdio.h"

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

using namespace GemRB;

int main(int argc, char* argv[])
{
	AddLogWriter(createStdioLogWriter());
	ToggleLogging(true);

	setlocale(LC_ALL, "");
#ifdef HAVE_SETENV
	setenv("SDL_VIDEO_X11_WMCLASS", argv[0], 0);
#endif

#ifdef M_TRIM_THRESHOLD
// Prevent fragmentation of the heap by malloc (glibc).
//
// The default threshold is 128*1024, which can result in a large memory usage
// due to fragmentation since we use a lot of small objects. On the other hand
// if the threshold is too low, free() starts to permanently ask the kernel
// about shrinking the heap.
	#if defined(HAVE_UNISTD_H)
		int pagesize = sysconf(_SC_PAGESIZE);
	#else
		int pagesize = 4*1024;
	#endif
	mallopt(M_TRIM_THRESHOLD, 5*pagesize);
#endif

	Interface::SanityCheck(VERSION_GEMRB);

	core = new Interface();
	CFGConfig* config = new CFGConfig(argc, argv);

	if (core->Init( config ) == GEM_ERROR) {
		delete config;
		delete( core );
		Log(MESSAGE, "Main", "Aborting due to fatal error...");

		return -1;
	}
	delete config;

	core->Main();
	delete( core );

	return 0;
}
