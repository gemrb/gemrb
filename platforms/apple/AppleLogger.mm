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

#import <TargetConditionals.h>

#if TARGET_OS_IPHONE
#import <Uikit/UIAlertView.h>
#elif TARGET_OS_MAC
#import <Cocoa/Cocoa.h>
#import <OSLog/OSLog.h>
#endif

#include "AppleLogger.h"

#include "Logging/Logging.h"

namespace GemRB {
/*
TODO: override parent so we can use NSLog
*/

AppleLogger::AppleLogger()
: Logger::LogWriter(DEBUG)
{}

void AppleLogger::WriteLogMessage(const Logger::LogMessage& msg)
{
	os_log_type_t type = OS_LOG_TYPE_DEBUG;
	switch (msg.level) {
		case FATAL:
			type = OS_LOG_TYPE_FAULT;
			break;
		case ERROR:
		case WARNING:
			type = OS_LOG_TYPE_ERROR;
			break;
		case MESSAGE:
		case COMBAT:
			type = OS_LOG_TYPE_INFO;
			break;
		case DEBUG:
		default:
			type = OS_LOG_TYPE_DEBUG;
	}
	os_log_with_type(OS_LOG_DEFAULT, type, "[%{public}s] %{public}s", msg.owner.c_str(), msg.message.c_str());
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
