// SPDX-FileCopyrightText: 2012 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#import "AppleLogger.h"
#import "GEM_AppDelegate.h"
#import "Streams/FileStream.h"
#import "Interface.h"

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
	[[NSFileManager defaultManager] changeCurrentDirectoryPath:NSHomeDirectory()];
	// Normally you would call super implemetation first, but don't!
	AddLogWriter(Logger::WriterPtr(new AppleLogger()));
	ToggleLogging(true);
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

		[[NSFileManager defaultManager] changeCurrentDirectoryPath:NSHomeDirectory()];
		setenv("PYTHONHOME", "Documents/python", 1);
		setenv("PYTHONPATH", "Documents/python/lib/python27", 1);
		
		try {
			Interface gemrb(LoadFromArgs(argc, argv));
			free(argv);
			gemrb.Main(); // pass control to GemRB
		} catch (CoreInitializationException& cie) {
			Log(FATAL, "Main", "Aborting due to fatal error... {}", cie);
			// reload the wrapper interface so we can try again instead of dying
			[self setupWrapper];
		}

		// We must exit since the application runloop never returns.
		exit(GEM_OK);
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
		AddLogWriter(createStreamLogWriter(fs));
		Log(MESSAGE, "Cocoa Wrapper", "Started a log file at {}", cLogFile);
	} else {
		delete fs;
		Log(ERROR, "Cocoa Wrapper", "Unable to start log file at {}", cLogFile);
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
