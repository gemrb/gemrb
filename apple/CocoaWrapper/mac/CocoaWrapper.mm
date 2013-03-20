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

#import "CocoaWrapper.h"

#import "AppleLogger.h"
#import "Interface.h"
#import "System/FileStream.h"
#import "System/Logger/File.h"

using namespace GemRB;

/* For some reaon, Apple removed setAppleMenu from the headers in 10.4,
 but the method still is there and works. To avoid warnings, we declare
 it ourselves here. */
@interface NSApplication(Missing_Methods)
- (void)setAppleMenu:(NSMenu *)menu;
@end

/* The main class of the application, the application's delegate */
@implementation CocoaWrapper
@synthesize prefrences=_prefrences;

- (BOOL)application:(NSApplication *) __unused theApplication openFile:(NSString *) __unused filename
{
	// TODO: implement this in some way
	return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *) __unused sender
{
    //override this method using plugin categories.
    NSLog(@"Application preparing for termination...");
    return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification *) __unused aNotification
{
    //override this method using plugin categories.
    NSLog(@"Application terminate");
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) __unused note
{
	AddLogger(createAppleLogger());

	// Load default defaults
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"defaults" ofType:@"plist"]]];
}

- (IBAction)openGame:(id) __unused sender
{
	// be careful to use only methods available in 10.5!
	NSOpenPanel* op = [NSOpenPanel openPanel];
	[op setCanChooseDirectories:YES];
	[op setCanChooseFiles:NO];
	[op setAllowsMultipleSelection:NO];
	if ([op runModal] == NSFileHandlingPanelOKButton) { //blocks till user selection
		NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
		[defaults setObject:[op filename] forKey:@"GamePath"];
	}
}

- (IBAction)launchGame:(id) __unused sender
{
	core = new Interface();
	InterfaceConfig* config = new InterfaceConfig(0, NULL);

	// load NSUserDefaults into config
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary* userValues = [defaults persistentDomainForName:@"net.sourceforge.gemrb"];
	NSDictionary* defaultValues = [NSDictionary dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"defaults" ofType:@"plist"]];

	NSMutableDictionary* dict = [NSMutableDictionary dictionaryWithDictionary:defaultValues];
	[dict addEntriesFromDictionary:userValues];

	for ( NSString* key in dict ) {
		NSString* value = nil;
		id obj = [dict objectForKey:key];
		if ([obj isKindOfClass:[NSNumber class]]) {
			value = [(NSNumber*)obj stringValue];
		} else if ([obj isKindOfClass:[NSString class]]) {
			value = (NSString*)obj;
		}
		config->SetKeyValuePair([key cStringUsingEncoding:NSASCIIStringEncoding],
								[value cStringUsingEncoding:NSASCIIStringEncoding]);
	}

	int status;
	if ((status = core->Init(config)) == GEM_ERROR) {
		delete config;
		delete( core );
		Log(MESSAGE, "Cocoa Wrapper", "Unable to initialize core. Terminating.");
	} else {
		[_prefrences close];
		// pass control to GemRB
		delete config;
		core->Main();
		delete( core );
		ShutdownLogging();
		// We must exit since the application runloop never returns.

		// TODO: we want to be able to use NSApplication terminate method
		// need fancier logic in shouldTerminate implementation first (in SDL plugin)
		exit(status);
	}
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
	if ([NSApp respondsToSelector:aSelector]) {
		return YES;
	}
	return [super respondsToSelector:aSelector];
}

- (NSMethodSignature*) methodSignatureForSelector:(SEL)selector
{
    // Check if car can handle the message
    NSMethodSignature* signature = [super
									methodSignatureForSelector:selector];

    // If not, can the car info string handle the message?
    if (!signature)
        signature = [NSApp methodSignatureForSelector:selector];

    return signature;
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    SEL selector = [invocation selector];

    if ([NSApp respondsToSelector:selector])
    {
        [invocation invokeWithTarget:NSApp];
    }
}

@end

/* Main entry point to executable - should *not* be GemRB_main! */
int main (int __unused argc, char ** __unused argv)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    /* Ensure the application object is initialised */
    NSApplication* app = [NSApplication sharedApplication];

    /* Set up the menubar */
    [NSApp setMainMenu:[[NSMenu alloc] init]];

    CocoaWrapper* wrapper = [[CocoaWrapper alloc] init];
    [app setDelegate:wrapper];

	[NSBundle loadNibNamed:@"GemRB" owner:wrapper];

    /* Start the main event loop */
	[pool drain];
    [NSApp run];

    [wrapper release];
    [pool release];

    return 0;
}
