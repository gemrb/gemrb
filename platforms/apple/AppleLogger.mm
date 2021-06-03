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
#elif TARGET_OS_MAC
#import <Cocoa/Cocoa.h>
#endif

#include "AppleLogger.h"

#include "System/Logging.h"

namespace GemRB {
/*
TODO: override parent so we can use NSLog
*/

AppleLogger::AppleLogger()
: Logger::LogWriter(DEBUG)
{}

AppleLogger::~AppleLogger()
{}
	
void AppleLogger::WriteLogMessage(const Logger::LogMessage& msg)
{
	NSLog(@"%s", msg.message.c_str()); // send to OS X logging system
	if (msg.level == FATAL) {
		// display a GUI alert for FATAL errors
		NSString* alertTitle = [NSString stringWithFormat:@"Fatal Error in %s", msg.owner.c_str()];
		NSString* alertMessage = [NSString stringWithCString:msg.message.c_str() encoding:NSASCIIStringEncoding];
#if TARGET_OS_IPHONE
		UIAlertView *alert =
		[[UIAlertView alloc] initWithTitle: alertTitle
								   message: alertMessage
								  delegate: nil
						 cancelButtonTitle: @"OK"
						 otherButtonTitles: nil];
		[alert show];
		while (alert.visible) {
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
		}
		[alert release];
#elif TARGET_OS_MAC
		NSAlert* alert = [[NSAlert alloc] init];

		[alert addButtonWithTitle: @"OK"];
		[alert setMessageText: alertTitle];
		[alert setInformativeText: alertMessage];
		[alert setAlertStyle: NSAlertStyleCritical];

		[alert runModal];
		[alert release];
#endif
	}
}

}
