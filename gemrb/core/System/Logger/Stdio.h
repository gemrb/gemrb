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

#ifndef LOGGER_STDIO_H
#define LOGGER_STDIO_H

#include "System/Logger.h" // for log_color

class GEM_EXPORT StdioLogger : public Logger {
public:
	StdioLogger();
	virtual ~StdioLogger();

	void vprint(const char* message, va_list ap);
	void textcolor(log_color);
	void printBracket(const char *status, log_color color);
	void printStatus(const char* status, log_color color);
	void vprintMessage(const char* owner, const char* message, log_color color, va_list ap);
};

Logger* createStdioLogger();

#endif
