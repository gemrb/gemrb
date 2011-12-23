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

#import "exports.h"

extern int GemRB_main(int argc, char *argv[]);
GEM_EXPORT
extern int SDL_main(int argc, char *argv[]);

// use the SDL 1.3 build in wrapper for iOS
int SDL_main (int argc, char **argv)
{
	//do all the special plugin initializations here
	Py_NoSiteFlag = 1;

	int ret = GemRB_main(argc, argv);
	if (ret != 0) {
		// TODO: inject into error() function instead and rewrite the core to always use error instead of returning.
		
		// put up a message letting the user know something failed.
		UIAlertView *alert = 
        [[UIAlertView alloc] initWithTitle: @"Engine Initialization Failure."
								   message: @"Check the log for causes."
								  delegate: nil
						 cancelButtonTitle: @"OK"
						 otherButtonTitles: nil];
		[alert show];
		while (alert.visible) {
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
		}
		[alert release];
	}
    return ret;
}
