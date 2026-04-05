// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 !!!:
 Because this file is shared between the CocoaWrapper object and plugins extending it we need to keep it
 in the gemrb/includes directory.
*/
#if __cplusplus
extern "C" {
	#define COCOA_EXPORT extern "C" __attribute__((visibility("default")))
#else
	#define COCOA_EXPORT __attribute__((visibility("default")))
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
- (void)applicationWillTerminate:(NSNotification*)aNotification;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender;

- (IBAction)openGame:(id)sender;
- (IBAction)launchGame:(id)sender;
@end

#if __cplusplus
} // Extern C
#endif
