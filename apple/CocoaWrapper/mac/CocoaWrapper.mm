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

static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;

/* The main class of the application, the application's delegate */
@implementation CocoaWrapper

/*
 * Catch document open requests...this lets us notice files when the app
 *  was launched by double-clicking a document, or when a document was
 *  dragged/dropped on the app's icon. You need to have a
 *  CFBundleDocumentsType section in your Info.plist to get this message,
 *  apparently.
 *
 * Files are added to gArgv, so to the app, they'll look like command line
 *  arguments. Previously, apps launched from the finder had nothing but
 *  an argv[0].
 *
 * This message may be received multiple times to open several docs on launch.
 *
 * This message is ignored once the app's mainline has been called.
 */
- (BOOL)application:(NSApplication *) __unused theApplication openFile:(NSString *)filename
{
    const char *temparg;
    size_t arglen;
    char *arg;
    char **newargv;

    if (!gFinderLaunch)  /* MacOS is passing command line args. */
        return FALSE;

    if (gCalledAppMainline)  /* app has started, ignore this document. */
        return FALSE;

    temparg = [filename UTF8String];
    arglen = strlen(temparg) + 1;
    arg = (char *) malloc(arglen);
    if (arg == NULL)
        return FALSE;

    newargv = (char **) realloc(gArgv, sizeof (char *) * (gArgc + 2));
    if (newargv == NULL)
    {
        free(arg);
        return FALSE;
    }
    gArgv = newargv;

    strlcpy(arg, temparg, arglen);
    gArgv[gArgc++] = arg;
    gArgv[gArgc] = NULL;
    return TRUE;
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
    int status;
	AddLogger(createAppleLogger());

    /* Hand off to main application code */
    gCalledAppMainline = TRUE;

	InterfaceConfig* config = new InterfaceConfig(gArgc, gArgv);

	// load NSUserDefaults into config
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary* dict = [defaults persistentDomainForName:@"net.sourceforge.gemrb"];

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

	core = new Interface();
	if ((status = core->Init(config)) == GEM_ERROR) {
		delete config;
		delete( core );
		Log(MESSAGE, "Cocoa Wrapper", "Unable to initialize core. Terminating.");
	} else {
		// pass control to GemRB
		delete config;
		core->Main();
		delete( core );
		ShutdownLogging();
	}
	// We must exit since the application runloop never returns.
	exit(status);
}

- (IBAction)openGame:(id) __unused sender
{
	// be careful to use only methods available in 10.5!
	NSOpenPanel* op = [NSOpenPanel openPanel];
	[op setCanChooseDirectories:YES];
	[op setCanChooseFiles:NO];
	[op setAllowsMultipleSelection:NO];
	[op runModal]; //blocks till user selection

	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	[defaults setObject:[op filename] forKey:@"GamePath"];
}

@end

/* Main entry point to executable - should *not* be GemRB_main! */
int main (int argc, char **argv)
{
    /* Copy the arguments into a global variable */
    /* This is passed if we are launched by double-clicking */
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgv = (char **) malloc(sizeof (char *) * 2);
        gArgv[0] = argv[0];
        gArgv[1] = NULL;
        gArgc = 1;
        gFinderLaunch = YES;
    } else {
        int i;
        gArgc = argc;
        gArgv = (char **) malloc(sizeof (char *) * (argc+1));
        for (i = 0; i <= argc; i++)
            gArgv[i] = argv[i];
        gFinderLaunch = NO;
    }

    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    /* Ensure the application object is initialised */
    NSApplication* app = [NSApplication sharedApplication];

	[NSBundle loadNibNamed:@"myMain" owner:NSApp];

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
