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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#if TARGET_OS_IPHONE
#import <Uikit/UIAlertView.h>
#endif

#include "AppleLogger.h"

#include "System/Logging.h"

namespace GemRB {
/*
TODO: override parent so we can use NSLog
*/

AppleLogger::AppleLogger()
	: StdioLogger(false)
{}

AppleLogger::~AppleLogger()
{}

void AppleLogger::textcolor(log_color /*c*/)
{}
	
void AppleLogger::log(log_level level, const char* owner, const char* message, log_color color)
{
	if (level == FATAL) {
		// display a GUI alert for FATAL errors
#if TARGET_OS_IPHONE
		UIAlertView *alert =
		[[UIAlertView alloc] initWithTitle: [NSString stringWithFormat:@"Fatal Error in %s", owner]
								   message: [NSString stringWithCString:message encoding:NSASCIIStringEncoding]
								  delegate: nil
						 cancelButtonTitle: @"OK"
						 otherButtonTitles: nil];
		[alert show];
		while (alert.visible) {
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
		}
		[alert release];
#endif
	}
	StdioLogger::log(level, owner, message, color);
}

Logger* createAppleLogger()
{
	return new AppleLogger();
}

}