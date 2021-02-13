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

#include "Interface.h"

#if defined(__sgi)
#  include <stdarg.h>
#else
#  include <cstdarg>
#endif
#include <memory>
#include <vector>

namespace GemRB {

static std::vector<std::unique_ptr<Logger>> loggers;

void ShutdownLogging()
{
	loggers.clear();
}

void InitializeLogging()
{
	AddLogger(createDefaultLogger());
}

void AddLogger(Logger* logger)
{
	// check if logging was disabled in settings first
	if (!core->Logging) return;

	if (logger) loggers.push_back(std::unique_ptr<Logger>(logger));
}

void RemoveLogger(Logger* logger)
{
	if (logger) {
		auto it = std::find_if(loggers.begin(), loggers.end(), [logger](const std::unique_ptr<Logger>& item) {
			return item.get() == logger;
		});
		if (it != loggers.end()) {
			loggers.erase(it);
		}
	}
}

static void vLog(log_level level, const char* owner, const char* message, log_color color, va_list ap)
{
	if (loggers.empty())
		return;

    va_list ap_copy;
    va_copy(ap_copy, ap);
    const size_t len = vsnprintf(NULL, 0, message, ap_copy);
    va_end(ap_copy);

	char *buf = new char[len+1];
	vsnprintf(buf, len + 1, message, ap);
	for (size_t i = 0; i < loggers.size(); ++i) {
		loggers[i]->log(level, owner, buf, color);
	}
	delete[] buf;
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

void LogVA(log_level level, const char* owner, const char* message, va_list args)
{
	vLog(level, owner, message, WHITE, args);
}

void Log(log_level level, const char* owner, StringBuffer const& buffer)
{
	for (const auto& logger : loggers) {
		logger->log(level, owner, buffer.get().c_str(), WHITE);
	}
}

}
