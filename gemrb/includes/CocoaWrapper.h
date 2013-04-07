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

#ifdef GCC_VERSION
#if GCC_VERSION < 40200
#warning You may need to disable wanings as errors or unused warnings to compile on this system.
#undef __unused
#define __unused
#endif
#endif

/*
 !!!:
 Because this file is shared between the CocoaWrapper object and plugins extending it we need to keep it
 in the gemrb/includes directory.
*/
#if __cplusplus
extern "C" {
#define COCOA_EXPORT extern "C" __attribute__ ((visibility("default")))
#else
#define COCOA_EXPORT __attribute__ ((visibility("default")))
#endif

#import <Cocoa/Cocoa.h>

COCOA_EXPORT
@interface CocoaWrapper : NSObject
#ifdef __AVAILABILITY_INTERNAL__MAC_10_0_DEP__MAC_10_6
<NSApplicationDelegate>
#endif
{
	NSWindow* _configWindow;

	BOOL _showConfigWindow;
}
@property(nonatomic, retain) IBOutlet NSWindow* configWindow;

// Override these application delegate methods in plugin categories
- (void)applicationWillTerminate:(NSNotification *)aNotification;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;

- (IBAction)openGame:(id)sender;
- (IBAction)launchGame:(id)sender;
@end

#if __cplusplus
}   // Extern C
#endif
