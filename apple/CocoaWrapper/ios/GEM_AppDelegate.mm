/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2012 The GemRB Project
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

#import "AppleLogger.h"
#import "GEM_AppDelegate.h"
#import "System/FileStream.h"
#import "Interface.h"
#import "System/Logger/File.h"

using namespace GemRB;

@implementation SDLUIKitDelegate (GemDelegate)
+ (NSString *)getAppDelegateClassName
{
	//override this SDL method so an instance of our subclass is used
	return @"GEM_AppDelegate";
}
@end

@implementation GEM_AppDelegate
@synthesize confControl;

// we MUST setup out wrapper interface here or iOS 5 will bitch about root view controller not being set
// and our interface will be the wrong orientation and distorted
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	// Normally you would call super implemetation first, but don't!
	AddLogger(createAppleLogger());
	[self setupWrapper];

    return [super application:application didFinishLaunchingWithOptions:launchOptions];
}

- (void)setupWrapper
{
	configWin = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
	configWin.backgroundColor = [UIColor blackColor];
	confControl = [[GEM_ConfController alloc] init];
	confControl.delegate = self;
	nibObjects = nil;
	// now load the config selector nib and display the list modally
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
		nibObjects = [[NSBundle mainBundle] loadNibNamed:@"GEM_ConfController-ipad" owner:confControl options:nil];
	} else {
		nibObjects = [[NSBundle mainBundle] loadNibNamed:@"GEM_ConfController-iphone" owner:confControl options:nil];
	}

	confControl.rootVC.delegate = confControl;

	[nibObjects retain];
	configWin.rootViewController = confControl.rootVC;
	configWin.screen = [UIScreen mainScreen];
	[configWin makeKeyAndVisible]; // Note that this causes the window to be retained!
}

- (void)setupComplete:(NSString*)configPath
{
	[self runGemRB:configPath];
}

- (void)runGemRB:(NSString*)configPath
{
	int ret = GEM_OK;
	if (configPath) {
		NSLog(@"Using config file:%@", configPath);
		// get argv
		// manipulate argc & argv to have gemrb passed the argument for the config file to use.
		NSMutableArray* procArguments = [[[NSProcessInfo processInfo] arguments] mutableCopy];
		[procArguments addObject:@"-c"];
		[procArguments addObject:configPath];

		int argc = [procArguments count];
		char** argv = (char**)malloc(argc * sizeof(char*));

		int i = 0;
		for (NSString* arg in procArguments) {
			argv[i++] = (char*)[arg cStringUsingEncoding:NSASCIIStringEncoding];
		}

		[configWin resignKeyWindow];
		configWin.rootViewController = nil;

		[configWin release];
		[nibObjects release];
		[confControl release];
		[procArguments release];

		core = new Interface();
		CFGConfig* config = new CFGConfig(argc, argv);
		free(argv);
		if ((ret = core->Init(config)) == GEM_ERROR) {
			delete config;
			delete( core );
			Log(MESSAGE, "Cocoa Wrapper", "Unable to initialize core. Relaunching wraper.");
			// reload the wrapper interface so we can try again instead of dying
			[self setupWrapper];
		} else {
			// pass control to GemRB
			delete config;
			core->Main();
			delete( core );
			ShutdownLogging();
			// We must exit since the application runloop never returns.
			exit(ret);
		}
	}
}

- (void)toggleDebug:(id) __unused sender
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString* logFile = [NSString stringWithFormat:@"%@/GemRB.log", [paths objectAtIndex:0]];
	//OpenFile();
	const char* cLogFile = [logFile cStringUsingEncoding:NSASCIIStringEncoding];
	FileStream *fs = new FileStream();
	if (fs->Create(cLogFile)) {
		AddLogger(createFileLogger(fs));
		Log(MESSAGE, "Cocoa Wrapper", "Started a log file at %s", cLogFile);
	} else {
		delete fs;
		Log(ERROR, "Cocoa Wrapper", "Unable to start log file at %s", cLogFile);
	}
}

- (void)dealloc
{
	// This is really just formallity.
	// these objects would have been deallocated in runGemRB under normal circumstances

	[configWin release];
	[nibObjects release];
	[confControl release];

	[super dealloc];
}

@end
