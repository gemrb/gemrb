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

#include <cstdarg>
#include <vector>

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

static void vLog(log_level level, const char* owner, const char* message, log_color color, va_list ap)
{
	if (theLogger.empty())
		return;

	// Copied from System/StringBuffer.cpp
#if defined(_MSC_VER)
	// Don't try to be smart.
	// Assume this is long enough. If not, message will be truncated.
	// MSVC6 has old vsnprintf that doesn't give length
	const size_t len = 4095;
#else
	va_list ap_copy;
	va_copy(ap_copy, ap);
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
