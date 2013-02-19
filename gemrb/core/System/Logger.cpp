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

#include "System/Logger.h"

#include "System/Logging.h"

#include <cstdio>

namespace GemRB {

Logger::Logger(log_level level)
{
	// set level directly instead of calling SetLogLevel() to avoid pure virtual call
	myLevel = level;
}

Logger::~Logger()
{}

void Logger::destroy()
{
	delete this;
}

bool Logger::SetLogLevel(log_level level)
{
	if (level > INTERNAL) {
		myLevel = level;
		static const char* fmt = "Log Level set to %d";
		char msg[25];
		snprintf(msg, 25, fmt, level);
		// careful to use our log function and not the global one to prevent this message form
		// propagating to other loggers.
		LogInternal(INTERNAL, "Logger", msg, DEFAULT);
		return true;
	} else {
		// careful to use our log function and not the global one to prevent this message form
		// propagating to other loggers.
		LogInternal(INTERNAL, "Logger", "Log Level cannot be set below CRITICAL.", RED);
	}
	return false;
}

void Logger::log(log_level level, const char* owner, const char* message, log_color color)
{
	if (level <= myLevel) {
		LogInternal(level, owner, message, color);
	}
}

const char* log_level_text[] = {
	"FATAL",
	"ERROR",
	"WARNING",
	"", // MESSAGE
	"COMBAT",
	"DEBUG"
};

}

using namespace GemRB;

#ifdef ANDROID

#include "System/Logger/Android.h"
Logger* (*GemRB::createDefaultLogger)() = createAndroidLogger;

#elif defined(WIN32) && !defined(WIN32_USE_STDIO)

#include "System/Logger/Win32Console.h"
Logger* (*GemRB::createDefaultLogger)() = createWin32ConsoleLogger;

#else

#include "System/Logger/Stdio.h"
Logger* (*GemRB::createDefaultLogger)() = createStdioLogger;

#endif

