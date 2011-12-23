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
#import "GEM_ConfController.h"

extern int GemRB_main(int argc, char *argv[]);
GEM_EXPORT
extern int SDL_main(int argc, char *argv[]);

// use the SDL 1.3 build in wrapper for iOS
int SDL_main (int argc, char **argv)
{
	//do all the special plugin initializations here
	Py_NoSiteFlag = 1;

	UIWindow* win = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
	win.backgroundColor = [UIColor blackColor];
	GEM_ConfController* confControl = [[GEM_ConfController alloc] init];
	NSArray* nibObjects = nil;
	// now load the config selector nib and display the list modally
	nibObjects = [[NSBundle mainBundle] loadNibNamed:@"GEM_ConfViewController-ipad" owner:confControl options:nil];

	[nibObjects retain];
	win.rootViewController = confControl.rootVC;
	win.screen = [UIScreen mainScreen];
	[win makeKeyAndVisible];
	[confControl runModal]; //doesnt return until the user pushes 'Play'
	const char* configPath = [[confControl selectedConfigPath] cStringUsingEncoding:NSASCIIStringEncoding];
	if (configPath != NULL) {
		NSLog(@"Using config file:%s", configPath);
		//manipulate argc & argv to have gemrb passed the argument for the config file to use.
		argc += 2;
		argv = realloc(argv, sizeof(char*) * argc);
		argv[argc - 2] = "-c";

		//hope that this cstrig doesnt get deallocated until we are done with it...
		//the Docs say it will be deallocated when the NSAutoreleasePool is drained which happens at the end of every run loop
		argv[argc - 1] = (char*)configPath;
	}else{
		//popup a message???
	}
	[win resignKeyWindow];
	win.rootViewController = nil;

	[win release];
	[nibObjects release];
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
