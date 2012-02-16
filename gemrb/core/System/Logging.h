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
#include "win32def.h"

enum log_color {
	DEFAULT,
	BLACK,
	RED,
	GREEN,
	BROWN,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	LIGHT_RED,
	LIGHT_GREEN,
	YELLOW,
	LIGHT_BLUE,
	LIGHT_MAGENTA,
	LIGHT_CYAN,
	LIGHT_WHITE
};

GEM_EXPORT void InitializeLogging();
GEM_EXPORT void ShutdownLogging();

#if defined(__GNUC__)
# define PRINTF_FORMAT(x, y) \
    __attribute__ ((format(printf, x, y)))
#else
# define PRINTF_FORMAT(x, y)
#endif

#if defined(__GNUC__)
# define NORETURN __attribute__ ((noreturn))
#elif defined(_MSC_VER)
# define NORETURN __declspec(noreturn)
#else
# define NORETURN
#endif

GEM_EXPORT void print(const char* message, ...)
	PRINTF_FORMAT(1, 2);

GEM_EXPORT void textcolor(log_color);
GEM_EXPORT void printBracket(const char *status, log_color color);
GEM_EXPORT void printStatus(const char* status, log_color color);
GEM_EXPORT void printMessage(const char* owner, const char* message, log_color color, ...)
	PRINTF_FORMAT(2, 4);

GEM_EXPORT void error(const char* owner, const char* message, ...)
	PRINTF_FORMAT(2, 3) NORETURN;

#undef PRINTF_FORMAT
#undef NORETURN

// poison printf
#if (__GNUC__ >= 4 && (__GNUC_MINOR__ >= 5 || __GNUC__ > 4))
extern "C" int printf(const char* message, ...) __attribute__ ((deprecated("GemRB doesn't use printf; use print instead.")));
#elif __GNUC__ >= 4
extern "C" int printf(const char* message, ...) __attribute__ ((deprecated));
#endif

#endif
