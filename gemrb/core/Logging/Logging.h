// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
GEM_EXPORT void SetMainLogLevel(LogLevel level);
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
[[deprecated("GemRB doesn't use printf; use Log instead.")]]
int printf(const char* message, ...);

#endif
