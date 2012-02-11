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

Logger theLogger;

void print(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	theLogger.vprint(message, ap);
	va_end(ap);
}

void textcolor(log_color c)
{
	theLogger.textcolor(c);
}

void printBracket(const char* status, log_color color)
{
	theLogger.printBracket(status, color);
}

void printStatus(const char* status, log_color color)
{
	theLogger.printStatus(status, color);
}

void printMessage(const char* owner, const char* message, log_color color, ...)
{
	va_list ap;

	va_start(ap, color);
	theLogger.vprintMessage(owner, message, color, ap);
	va_end(ap);
}

void error(const char* owner, const char* message, ...)
{
	va_list ap;

	va_start(ap, message);
	theLogger.vprintMessage(owner, message, LIGHT_RED, ap);
	va_end(ap);

	exit(1);
}
