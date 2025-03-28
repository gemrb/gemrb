/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

/**
 * @file logging.h
 * Logging definitions.
 * @author The GemRB Project
 */


#ifndef LOGGING_H
#define LOGGING_H

#include "exports.h"

#include "Logging/Logger.h"
#include "fmt/std.h" // needed for Log specialization

namespace GemRB {

GEM_EXPORT void ToggleLogging(bool);
GEM_EXPORT void AddLogWriter(Logger::WriterPtr&&);
GEM_EXPORT void SetConsoleWindowLogLevel(LogLevel level);
GEM_EXPORT void LogMsg(Logger::LogMessage&& msg);
GEM_EXPORT void FlushLogs();

template<typename... ARGS>
void Log(LogLevel level, const char* owner, const char* message, ARGS&&... args)
{
	auto formattedMsg = fmt::format(message, std::forward<ARGS>(args)...);
	LogMsg(Logger::LogMessage(level, owner, std::move(formattedMsg), Logger::MSG_STYLE));
}

/// Log an error and exit.
template<typename... ARGS>
[[noreturn]]
void error(const char* owner, const char* format, ARGS&&... args)
{
	Log(FATAL, owner, format, std::forward<ARGS>(args)...);
	exit(1);
}

}

// poison printf
#if !defined(__MINGW32__) && defined(__GNUC__)
extern "C" int printf(const char* message, ...) __attribute__((deprecated("GemRB doesn't use printf; use Log instead.")));
#endif

#endif
