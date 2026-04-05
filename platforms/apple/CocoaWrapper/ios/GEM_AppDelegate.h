// SPDX-FileCopyrightText: 2012 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#import <Foundation/Foundation.h>
#import <SDL/uikit/SDL_uikitappdelegate.h>

#import "GEM_ConfController.h"

// make a category to force SDLUIKitDelegate to return our class name
@interface SDLUIKitDelegate (GemDelegate)
+ (NSString*)getAppDelegateClassName; // override this SDL method so an instance of our subclass is used
@end

@interface GEM_AppDelegate : SDLUIKitDelegate <GEM_ConfControllerDelegate> {
	GEM_ConfController* confControl;

	UIWindow* configWin;
	NSArray* nibObjects;
}
@property(readonly) GEM_ConfController* confControl;

- (void)setupWrapper;
- (void)runGemRB:(NSString*)configPath;
- (void)toggleDebug:(id)sender;
@end
