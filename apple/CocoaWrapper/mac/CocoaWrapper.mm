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
@synthesize configWindow=_configWindow;

- (id)init
{
    self = [super init];
    if (self) {
		_configWindow = nil;
        _showConfigWindow = NO;
    }
    return self;
}

- (BOOL)application:(NSApplication *) __unused theApplication openFile:(NSString *) filename
{
	NSFileManager* fm = [NSFileManager defaultManager];
	// only open if passed a directory
	BOOL isDir = NO;
	if ([fm fileExistsAtPath:filename isDirectory:&isDir] && isDir) {
		NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
		[defaults setObject:filename forKey:@"GamePath"];

		if (_showConfigWindow == NO) {
			// opened via means other than config window
			[self launchGame:nil];
		}

		return YES;
	}
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

// always called before openFile when launched via CLI/DragDrop
- (void)applicationWillFinishLaunching:(NSNotification *) __unused aNotification
{
	AddLogger(createAppleLogger());

	// Load default defaults
	NSString* defaultsPath = [[NSBundle mainBundle] pathForResource:@"defaults" ofType:@"plist"];
	NSDictionary* defaultDict = [NSDictionary dictionaryWithContentsOfFile:defaultsPath];
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults registerDefaults:defaultDict];

	if (![defaults stringForKey:@"CachePath"]) {
		NSString* cachePath = [NSString stringWithFormat:@"%@gemrb", NSTemporaryDirectory()];
		[defaults setValue:cachePath forKey:@"CachePath"];
	}
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) __unused aNotification
{
	// we configure this here so that when GemRB is launched though means such as Drag/Drop we dont show the config window

	_showConfigWindow = YES; //still need to set this to YES in case an error occurs
	[NSBundle loadNibNamed:@"GemRB" owner:self];

	if (core == NULL) {
		[_configWindow makeKeyAndOrderFront:nil];
	}
}

- (IBAction)openGame:(id) __unused sender
{
	// be careful to use only methods available in 10.5!
	NSOpenPanel* op = [NSOpenPanel openPanel];
	[op setCanChooseDirectories:YES];
	[op setCanChooseFiles:NO];
	[op setAllowsMultipleSelection:NO];
	[op setMessage:@"Select a folder containing an IE game."];
	[op setPrompt:@"Select Game"];
	if ([op runModal] == NSFileHandlingPanelOKButton) { //blocks till user selection
		[self application:NSApp openFile:[op filename]];
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

	for ( NSString* key in dict.allKeys ) {
		id obj = [dict objectForKey:key];
		if ( [obj isKindOfClass:[NSDictionary class]] ) {
			// move it to the root level
			[dict addEntriesFromDictionary:obj];
			[dict removeObjectForKey:key];
		}
	}

	for ( NSString* key in dict ) {
		NSString* value = nil;
		id obj = [dict objectForKey:key];
		if ([obj isKindOfClass:[NSNumber class]]) {
			value = [(NSNumber*)obj stringValue];
		} else if ([obj isKindOfClass:[NSString class]]) {
			value = (NSString*)obj;
		}
		if (value) {
			config->SetKeyValuePair([key cStringUsingEncoding:NSASCIIStringEncoding],
								[value cStringUsingEncoding:NSASCIIStringEncoding]);
		}
	}

	int status;
	if ((status = core->Init(config)) == GEM_ERROR) {
		delete config;
		delete( core );
		Log(MESSAGE, "Cocoa Wrapper", "Unable to initialize core. Terminating.");
	} else {
		[_configWindow close];
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

    /* Start the main event loop */
	[pool drain];
    [NSApp run];

    [wrapper release];
    [pool release];

    return 0;
}
