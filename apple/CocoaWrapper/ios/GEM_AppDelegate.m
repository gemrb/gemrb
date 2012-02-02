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

#import "GEM_AppDelegate.h"

#import "exports.h"
extern int GemRB_main(int argc, char *argv[]);
GEM_EXPORT

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
	[configWin makeKeyAndVisible];
}

- (void)setupComplete:(NSString*)configPath
{
	[self runGemRB:configPath];
}

- (void)runGemRB:(NSString*)configPath
{
	int ret = 0;
	if (configPath) {
		NSLog(@"Using config file:%@", configPath);
		// get argv
		// manipulate argc & argv to have gemrb passed the argument for the config file to use.
		NSMutableArray* procArguments = [[[NSProcessInfo processInfo] arguments] mutableCopy];
		[procArguments addObject:@"-c"];
		[procArguments addObject:configPath];

		int argc = [procArguments count];
		char** argv = malloc(argc * sizeof(char*));

		int i = 0;
		for (NSString* arg in procArguments) {
			argv[i++] = (char*)[arg cStringUsingEncoding:NSASCIIStringEncoding];
		}

		[configWin resignKeyWindow];
		configWin.rootViewController = nil;

		[configWin release];
		[nibObjects release];

		[confControl release];

		// pass control to GemRB
		[procArguments release];
		ret = GemRB_main(argc, argv);
		free(argv);
	}
	if (ret != 0) {
		// put up a message letting the user know something failed.
		UIAlertView *alert =
		[[UIAlertView alloc] initWithTitle: @"Engine Initialization Failure."
								   message: @"Check the log for causes."
								  delegate: nil
						 cancelButtonTitle: @"OK"
						 otherButtonTitles: nil];
		[alert show];
		while (alert.visible) {
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
		}
		[alert release];
		// reload the wrapper interface so we can try again instead of dying
		[self setupWrapper];
	} else {
		// We must exit since the application runloop never returns.
		exit(ret);
	}
}

@end
