// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

	return 0; //never actually returns.
}
