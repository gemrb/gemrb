// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#import <SDL.h>
#import "CocoaWrapper.h"

// SDL 1.3 has its own App Delegate, but currently all it does is what this delegate category is doing
// SDL is nice enough to check if a delegate exists prior to assigning its own.

/*
Mac OS X wrapper
*/

@interface CocoaWrapper (SDLTerminate)
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
@end

@implementation CocoaWrapper (SDLTerminate)
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *) __unused sender
{
	NSLog(@"Received terminate request. Dispatching SDL_QUIT event...");
	SDL_Event event;
	event.type = SDL_QUIT;
	if (SDL_PushEvent(&event) == 1) {
		// we deny the application request to terminate.
		// this way we can let GemRB handle quit event so we can save before exit etc.
		return NSTerminateCancel;
	}
	return NSTerminateNow;
}
@end
