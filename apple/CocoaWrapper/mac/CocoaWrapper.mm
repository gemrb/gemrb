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
			[self launchGame:self];
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
	ShutdownLogging();
}

// always called before openFile when launched via CLI/DragDrop
- (void)applicationWillFinishLaunching:(NSNotification *) __unused aNotification
{
	AddLogger(createAppleLogger());

	// Load default defaults
	NSString* defaultsPath = [[NSBundle mainBundle] pathForResource:@"defaults" ofType:@"plist"];
	NSMutableDictionary* defaultDict = [NSMutableDictionary dictionaryWithContentsOfFile:defaultsPath];

	NSString* path = [[NSBundle mainBundle].resourcePath stringByAppendingFormat:@"/GUIScripts"];
	NSFileManager* fm = [NSFileManager defaultManager];
	NSError* error = nil;
	NSArray* items = [fm contentsOfDirectoryAtPath:path error:&error];

	NSMutableArray* gameTypes = [NSMutableArray arrayWithObject:@"auto"];
	for (NSString* subPath in items) {
		BOOL isDir = NO;
		if ([fm fileExistsAtPath:[NSString stringWithFormat:@"%@/%@", path, subPath] isDirectory:&isDir] && isDir) {
			[gameTypes addObject:[subPath lastPathComponent]];
		}
	}
	[defaultDict setValue:gameTypes forKey:@"gameTypes"];

	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults registerDefaults:defaultDict];

	if (![defaults stringForKey:@"CachePath"]) {
		NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, NO);
		NSString* cachePath = [[paths objectAtIndex:0] stringByAppendingPathComponent:@"gemrb"];
		[defaults setValue:cachePath forKey:@"CachePath"];
	}

	NSMutableDictionary* additionalPaths = [[defaults dictionaryForKey:@"AdditionalPaths"] mutableCopy];
	if ([additionalPaths valueForKey:@"CustomFontPath"] == nil) {
		NSArray* paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, NO);
		NSString* fontPath = [[paths objectAtIndex:0] stringByAppendingPathComponent:@"Fonts"];
		[additionalPaths setValue:fontPath forKey:@"CustomFontPath"];
	}
	if ([additionalPaths valueForKey:@"SavePath"] == nil) {
		[additionalPaths setValue:[defaults valueForKey:@"GamePath"] forKey:@"SavePath"];
	}
	[defaults setObject:additionalPaths forKey:@"AdditionalPaths"];
	[additionalPaths release];
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) __unused aNotification
{
	// we configure this here so that when GemRB is launched though means such as Drag/Drop we dont show the config window

	_showConfigWindow = YES; //still need to set this to YES in case an error occurs
	[[NSBundle mainBundle] loadNibNamed:@"GemRB" owner:self topLevelObjects:nil];

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
	[op setMessage:@"Select a folder containing an IE game (has a chitin.key)."];
	[op setPrompt:@"Select Game"];
	if ([op runModal] == NSFileHandlingPanelOKButton) { //blocks till user selection
		[self application:NSApp openFile:op.URL.path];
	}
}

- (IBAction)launchGame:(id) sender
{
	if (core) {
		Log(FATAL, "Launch Game", "GemRB game is currently running. Please close it before trying to open another.");
		return;
	}
	if (sender) {
		// Note: use NSRunLoop over NSObject performSelector!
		NSArray* modes = [NSArray arrayWithObject:NSDefaultRunLoopMode];
		[[NSRunLoop mainRunLoop] performSelector:@selector(launchGame:) target:self argument:nil order:0 modes:modes];
		return;
	}

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
		if (value && ![value isEqualToString:@""]) {
			config->SetKeyValuePair([key cStringUsingEncoding:NSASCIIStringEncoding],
								[value cStringUsingEncoding:NSASCIIStringEncoding]);
		}
	}

	int status;
	if ((status = core->Init(config)) == GEM_ERROR) {
		delete config;
		delete( core );
		core = NULL;
		Log(MESSAGE, "Cocoa Wrapper", "Unable to initialize core. Terminating.");
	} else {
		[_configWindow close];
		// pass control to GemRB
		delete config;
		core->Main();
		delete( core );
		core = NULL;

		if ([defaults boolForKey:@"TerminateOnClose"]) {
			[NSApp terminate:self];
		}
	}
}

- (id)validRequestorForSendType:(NSString *) __unused sendType returnType:(NSString *) __unused returnType
{
	return nil;
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
    NSMethodSignature* signature = [super methodSignatureForSelector:selector];

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
    } else {
		[super forwardInvocation:invocation];
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
