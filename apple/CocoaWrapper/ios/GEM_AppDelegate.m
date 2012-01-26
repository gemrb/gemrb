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

@implementation SDLUIKitDelegate (GemDelegate)
+ (NSString *)getAppDelegateClassName
{
	//override this SDL method so an instance of our subclass is used
	return @"GEM_AppDelegate";
}
@end

@implementation GEM_AppDelegate
@synthesize confController;

// we MUST setup out wrapper interface here or iOS 5 will bitch about root view controller not being set
// and our interface will be the wrong orientation and distorted
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	// Normally you would call super implemetation first, but don't!
	configWin = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
	configWin.backgroundColor = [UIColor blackColor];
	confControl = [[GEM_ConfController alloc] init];
	nibObjects = nil;
	// now load the config selector nib and display the list modally
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
		nibObjects = [[NSBundle mainBundle] loadNibNamed:@"GEM_ConfController-ipad" owner:confControl options:nil];
	} else {
		nibObjects = [[NSBundle mainBundle] loadNibNamed:@"GEM_ConfController-iphone" owner:confControl options:nil];
	}

	[nibObjects retain];
	configWin.rootViewController = confControl.rootVC;
	configWin.screen = [UIScreen mainScreen];
	[configWin makeKeyAndVisible];

    return [super application:application didFinishLaunchingWithOptions:launchOptions];
}

- (NSString*)runSetup
{
	[confControl runModal];

	[configWin resignKeyWindow];
	configWin.rootViewController = nil;

	[configWin release];
	[nibObjects release];

	return [confControl selectedConfigPath];
}

@end
