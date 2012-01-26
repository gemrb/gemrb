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

#import <Foundation/Foundation.h>
#import <SDL/uikit/SDL_uikitappdelegate.h>

#import "GEM_ConfController.h"

// make a category to force SDLUIKitDelegate to return our class name
@interface SDLUIKitDelegate (GemDelegate)
+ (NSString *)getAppDelegateClassName; //override this SDL method so an instance of our subclass is used
@end

@interface GEM_AppDelegate : SDLUIKitDelegate
{
	GEM_ConfController* confControl;

	UIWindow* configWin;
	NSArray* nibObjects;
}
@property(readonly) GEM_ConfController* confController;

// runs the wrapper and returns the sresulting config path
- (NSString*)runSetup;

@end
