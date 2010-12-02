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

#ifdef ANDROID
# include <android/log.h>
#endif

#ifdef WIN32
# define ADV_TEXT
# include <conio.h>
extern GEM_EXPORT HANDLE hConsole;
# define textcolor(i) SetConsoleTextAttribute(hConsole, i)

# ifndef __MINGW32__
#  define printf cprintf //broken in mingw !!
# endif

#define BLACK 0
#define RED FOREGROUND_RED
#define GREEN FOREGROUND_GREEN
#define BROWN FOREGROUND_GREEN | FOREGROUND_RED
#define BLUE FOREGROUND_BLUE
#define MAGENTA FOREGROUND_RED | FOREGROUND_BLUE
#define CYAN FOREGROUND_BLUE | FOREGROUND_GREEN
#define WHITE FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
#define LIGHT_RED (RED | FOREGROUND_INTENSITY)
#define LIGHT_GREEN (GREEN | FOREGROUND_INTENSITY)
#define YELLOW (GREEN | RED | FOREGROUND_INTENSITY)
#define LIGHT_BLUE (BLUE | FOREGROUND_INTENSITY)
#define LIGHT_MAGENTA (MAGENTA | FOREGROUND_INTENSITY)
#define LIGHT_CYAN (CYAN | FOREGROUND_INTENSITY)
#define LIGHT_WHITE (WHITE | FOREGROUND_INTENSITY)
#define DEFAULT WHITE

#else //WIN32
# include <config.h>
# include <cstdio>
# include <cstdlib>

# define textcolor(i) i

#define DEFAULT printf("\033[0m");
#define BLACK printf("\033[0m\033[30;40m");
#define RED printf("\033[0m\033[31;40m");
#define GREEN printf("\033[0m\033[32;40m");
#define BROWN printf("\033[0m\033[33;40m");
#define BLUE printf("\033[0m\033[34;40m");
#define MAGENTA printf("\033[0m\033[35;40m");
#define CYAN printf("\033[0m\033[36;40m");
#define WHITE printf("\033[0m\033[37;40m");
#define LIGHT_RED printf("\033[1m\033[31;40m");
#define LIGHT_GREEN printf("\033[1m\033[32;40m");
#define YELLOW printf("\033[1m\033[33;40m");
#define LIGHT_BLUE printf("\033[1m\033[34;40m");
#define LIGHT_MAGENTA printf("\033[1m\033[35;40m");
#define LIGHT_CYAN printf("\033[1m\033[36;40m");
#define LIGHT_WHITE printf("\033[1m\033[37;40m");
#endif //WIN32

#ifndef ANDROID
#define printBracket(status, color) textcolor(WHITE); printf("["); textcolor(color); printf("%s", status); textcolor(WHITE); printf("]")
#define printStatus(status, color) printBracket(status, color); printf("\n")
#define printMessage(owner, message, color) printBracket(owner, LIGHT_WHITE); printf(": "); textcolor(color); printf("%s", message)
#else
#define printBracket(status, color)
#define printStatus(status, color) __android_log_print(ANDROID_LOG_INFO, "GemRB", "[%s]", status)
#define printMessage(owner, message, color) __android_log_print(ANDROID_LOG_INFO, "GemRB", "%s: %s", owner, message)
#endif

#endif
