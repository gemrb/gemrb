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

#include "System/Logger/Android.h"

#include <android/log.h>

AndroidLogger::AndroidLogger()
{}

AndroidLogger::~AndroidLogger()
{}

void AndroidLogger::vprint(const char *message, va_list ap)
{
	__android_log_vprint(ANDROID_LOG_INFO, "GemRB/print:", message, ap);
}

void AndroidLogger::textcolor(log_color c)
{
}

void AndroidLogger::printBracket(const char* status, log_color color)
{
}

void AndroidLogger::printStatus(const char* status, log_color color)
{
	__android_log_print(ANDROID_LOG_INFO, "GemRB", "[%s]", status);
}

void AndroidLogger::vprintMessage(const char* owner, const char* message, log_color color, va_list ap)
{
	// FIXME: We drop owner on the floor.
	va_list ap;
	va_start(ap, message);
	__android_log_vprint(ANDROID_LOG_INFO, "GemRB", message, ap);
	va_end(ap);
}

void createAndroidLogger()
{
	return new AndroidLogger();
}
