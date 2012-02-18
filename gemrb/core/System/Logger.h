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
 * @file Logger.h
 * Logging targets
 * @author The GemRB Project
 */

#ifndef LOGGER_H
#define LOGGER_H

#include "System/Logging.h" // for log_color

#include <cstdarg>

class GEM_EXPORT Logger {
public:
	Logger();
	virtual ~Logger();

	virtual void log(log_level, const char* owner, const char* message, log_color color) = 0;

	// Deprecated functions
	virtual void vprint(const char* message, va_list ap) = 0;
	virtual void textcolor(log_color) = 0;
	virtual void printBracket(const char *status, log_color color) = 0;
	virtual void printStatus(const char* status, log_color color) = 0;
	virtual void vprintMessage(const char* owner, const char* message, log_color color, va_list ap) = 0;

	virtual void destroy();
};

extern const char* log_level_text[];

extern Logger* (*createDefaultLogger)();

#endif
