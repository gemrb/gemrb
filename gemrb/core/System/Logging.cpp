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

void print(const char *message, ...)
{
	for (size_t i = 0; i < theLogger.size(); ++i) {
		va_list ap;

		va_start(ap, message);
		theLogger[i]->vprint(message, ap);
		va_end(ap);
	}
}

void textcolor(log_color c)
{
	for (size_t i = 0; i < theLogger.size(); ++i) {
		theLogger[i]->textcolor(c);
	}
}

void printBracket(const char* status, log_color color)
{
	for (size_t i = 0; i < theLogger.size(); ++i) {
		theLogger[i]->printBracket(status, color);
	}
}

void printStatus(const char* status, log_color color)
{
	for (size_t i = 0; i < theLogger.size(); ++i) {
		theLogger[i]->printStatus(status, color);
	}
}

void printMessage(const char* owner, const char* message, log_color color, ...)
{
	for (size_t i = 0; i < theLogger.size(); ++i) {
		va_list ap;

		va_start(ap, color);
		theLogger[i]->vprintMessage(owner, message, color, ap);
		va_end(ap);
	}
}

void error(const char* owner, const char* message, ...)
{
	for (size_t i = 0; i < theLogger.size(); ++i) {
		va_list ap;

		va_start(ap, message);
		theLogger[i]->vprintMessage(owner, message, LIGHT_RED, ap);
		va_end(ap);
	}

	ShutdownLogging();

	exit(1);
}
