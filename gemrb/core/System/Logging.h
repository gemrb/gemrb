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

#ifdef ANDROID
#	include <android/log.h>
#endif

#ifdef WIN32
#	define ADV_TEXT
#	include <conio.h>
extern GEM_EXPORT HANDLE hConsole;
#	define textcolor(i) SetConsoleTextAttribute(hConsole, i)
#else //WIN32
#	ifndef ANDROID
#		include <config.h>
#	endif
#	include <cstdio>
#	include <cstdlib>
#	define textcolor(i) i
#endif //WIN32

#ifdef NOCOLOR
#	define DEFAULT print("%s","");
#	define BLACK print("%s","");
#	define RED print("%s","");
#	define GREEN print("%s","");
#	define BROWN print("%s","");
#	define BLUE print("%s","");
#	define MAGENTA print("%s","");
#	define CYAN print("%s","");
#	define WHITE print("%s","");
#	define LIGHT_RED print("%s","");
#	define LIGHT_GREEN print("%s","");
#	define YELLOW print("%s","");
#	define LIGHT_BLUE print("%s","");
#	define LIGHT_MAGENTA print("%s","");
#	define LIGHT_CYAN print("%s","");
#	define LIGHT_WHITE print("%s","");
#else
#	ifdef WIN32
#		define BLACK 0
#		define RED FOREGROUND_RED
#		define GREEN FOREGROUND_GREEN
#		define BROWN FOREGROUND_GREEN | FOREGROUND_RED
#		define BLUE FOREGROUND_BLUE
#		define MAGENTA FOREGROUND_RED | FOREGROUND_BLUE
#		define CYAN FOREGROUND_BLUE | FOREGROUND_GREEN
#		define WHITE FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
#		define LIGHT_RED (RED | FOREGROUND_INTENSITY)
#		define LIGHT_GREEN (GREEN | FOREGROUND_INTENSITY)
#		define YELLOW (GREEN | RED | FOREGROUND_INTENSITY)
#		define LIGHT_BLUE (BLUE | FOREGROUND_INTENSITY)
#		define LIGHT_MAGENTA (MAGENTA | FOREGROUND_INTENSITY)
#		define LIGHT_CYAN (CYAN | FOREGROUND_INTENSITY)
#		define LIGHT_WHITE (WHITE | FOREGROUND_INTENSITY)
#		define DEFAULT WHITE
#	else //WIN32
#		define DEFAULT print("\033[0m");
#		define BLACK print("\033[0m\033[30;40m");
#		define RED print("\033[0m\033[31;40m");
#		define GREEN print("\033[0m\033[32;40m");
#		define BROWN print("\033[0m\033[33;40m");
#		define BLUE print("\033[0m\033[34;40m");
#		define MAGENTA print("\033[0m\033[35;40m");
#		define CYAN print("\033[0m\033[36;40m");
#		define WHITE print("\033[0m\033[37;40m");
#		define LIGHT_RED print("\033[1m\033[31;40m");
#		define LIGHT_GREEN print("\033[1m\033[32;40m");
#		define YELLOW print("\033[1m\033[33;40m");
#		define LIGHT_BLUE print("\033[1m\033[34;40m");
#		define LIGHT_MAGENTA print("\033[1m\033[35;40m");
#		define LIGHT_CYAN print("\033[1m\033[36;40m");
#		define LIGHT_WHITE print("\033[1m\033[37;40m");
#	endif //WIN32
#endif

#ifndef ANDROID
#	define printBracket(status, color) textcolor(WHITE); print("["); textcolor(color); print("%s", status); textcolor(WHITE); print("]")
#	define printStatus(status, color) printBracket(status, color); print("\n")
#	define printMessage(owner, message, color) printBracket(owner, LIGHT_WHITE); print(": "); textcolor(color); print("%s", message)
#else
#	define printBracket(status, color)
#	define printStatus(status, color) __android_log_print(ANDROID_LOG_INFO, "GemRB", "[%s]", status)
#	define printMessage(owner, message, color) __android_log_print(ANDROID_LOG_INFO, "GemRB", "%s: %s", owner, message)
#endif

GEM_EXPORT void print(const char* message, ...);

#if (__GNUC__ > 4)
// poison printf
extern "C" int printf(const char* message, ...) __attribute__ ((deprecated))
#endif

#endif
