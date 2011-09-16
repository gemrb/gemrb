/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2006 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */
#import <SDL.h>
#import "CocoaWrapper.h"
#ifdef TARGET_OS_MAC
@interface CocoaWrapper (SDLTerminate)
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
@end

@implementation CocoaWrapper (SDLTerminate)
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *) __unused sender
{
    NSLog(@"Received terminate request. Dispatching SDL_QUIT event...");
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
    // we deny the application request to terminate.
    // this way we can let GemRB handle quit event so we can save before exit etc.
    return NSTerminateCancel;
}
@end
#endif
