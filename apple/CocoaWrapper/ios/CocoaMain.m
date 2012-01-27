/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include <Python.h>

#import "GEM_AppDelegate.h"

extern int SDL_main(int argc, char *argv[]);

// use the SDL 1.3 build in wrapper for iOS
int SDL_main (int __unused argc, char __unused **argv)
{
	/*
	 Consideration: we are currently getting argv and argc from cocoa process info and not using the ones passed here (they of course ought to be the same)
	*/


	/*
	 Beware!: This relies on haveing a certain revision of SDL 1.3 or later.
	 Early revisions of 1.3 would call exit() when this method returns instad of leaving the runloop running for us.
	 GemRB used to hack its own runloop to workaround the issue, but that caused problems with the config editor
	 Now that SDL no longer exits and we are creating our own app delegate we can safely use this wrapper without hacks.

	 Also note this function never returns. exit is called after GemRB_main returns in the app delegate.
	*/

	//do all the special plugin initializations here
	Py_NoSiteFlag = 1;
	//this mostly just supresses a benign console error
	setenv("PYTHONHOME", "GUIScripts", 0);

	return 0; //never actually returns.
}
