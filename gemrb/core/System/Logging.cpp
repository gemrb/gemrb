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

#include "System/Logging.h"

#include "System/Logger.h"
#include "System/StringBuffer.h"

#if defined(__sgi)
#  include <stdarg.h>
#else
#  include <cstdarg>
#endif
#include <vector>

namespace GemRB {

static std::vector<Logger*> theLogger;

void ShutdownLogging()
{
	for (size_t i = 0; i < theLogger.size(); ++i) {
		theLogger[i]->destroy();
	}
	theLogger.clear();
}

void InitializeLogging()
{
	AddLogger(createDefaultLogger());
}

void AddLogger(Logger* logger)
{
	if (logger)
		theLogger.push_back(logger);
}

void RemoveLogger(Logger* logger)
{
	if (logger) {
		std::vector<Logger*>::iterator itr = theLogger.begin();
		while (itr != theLogger.end()) {
			if (*itr == logger) {
				itr = theLogger.erase(itr);
			} else {
				++itr;
			}
		}
		logger->destroy();
		logger = NULL;
	}
}

static void vLog(log_level level, const char* owner, const char* message, log_color color, va_list ap)
{
	if (theLogger.empty())
		return;

	// Copied from System/StringBuffer.cpp
#ifndef __va_copy
	// Don't try to be smart.
	// Assume this is long enough. If not, message will be truncated.
	// MSVC6 has old vsnprintf that doesn't give length
	const size_t len = 4095;
#else
    va_list ap_copy;
    // __va_copy should always be defined
    // va_copy is only defined by C99 (C++11 and up)
    __va_copy(ap_copy, ap);
    const size_t len = vsnprintf(NULL, 0, message, ap_copy);
    va_end(ap_copy);
#endif

#if defined(__GNUC__)
	__extension__ // Variable-length arrays
#endif
	char buf[len+1];
	vsnprintf(buf, len + 1, message, ap);
	for (size_t i = 0; i < theLogger.size(); ++i) {
		theLogger[i]->log(level, owner, buf, color);
	}
}

void print(const char *message, ...)
{
	va_list ap;
	va_start(ap, message);
	vLog(MESSAGE, "Unknown", message, WHITE, ap);
	va_end(ap);
}

void error(const char* owner, const char* message, ...)
{
	va_list ap;
	va_start(ap, message);
	vLog(FATAL, owner, message, LIGHT_RED, ap);
	va_end(ap);

	ShutdownLogging();

	exit(1);
}

void Log(log_level level, const char* owner, const char* message, ...)
{
	va_list ap;
	va_start(ap, message);
	vLog(level, owner, message, WHITE, ap);
	va_end(ap);
}

void Log(log_level level, const char* owner, StringBuffer const& buffer)
{
	for (size_t i = 0; i < theLogger.size(); ++i) {
		theLogger[i]->log(level, owner, buffer.get().c_str(), WHITE);
	}
}

}
