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

#include <cstdio>
#include <cstdarg>

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef WIN32
# define ADV_TEXT
# include <conio.h>
#endif

void vprint(const char *message, va_list ap)
{
#if (!defined(WIN32) || defined(__MINGW32__))
# ifdef ANDROID
	__android_log_vprint(ANDROID_LOG_INFO, "printf:", message, ap);
# else
	vprintf(message, ap);
# endif
#else
	va_list copy;
	// MSVC6 lacks va_copy
	memcpy(&copy, &ap, sizeof(va_list))
	int size = vsnprintf(NULL, 0, message, copy);
	va_end(copy);

	if (size < 0)
		return;

	char *buff = new char[size+1];
	vsprintf(buff, message, ap);

	cprintf("%s", message);

	free(buff);
#endif
}

void print(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vprint(message, ap);
	va_end(ap);
}

#ifdef WIN32
static int colors[] = {
	0,
	FOREGROUND_RED,
	FOREGROUND_GREEN,
	FOREGROUND_GREEN | FOREGROUND_RED,
	FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_BLUE,
	FOREGROUND_BLUE | FOREGROUND_GREEN,
	FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
	(RED | FOREGROUND_INTENSITY),
	(GREEN | FOREGROUND_INTENSITY),
	(GREEN | RED | FOREGROUND_INTENSITY),
	(BLUE | FOREGROUND_INTENSITY),
	(MAGENTA | FOREGROUND_INTENSITY),
	(CYAN | FOREGROUND_INTENSITY),
	(WHITE | FOREGROUND_INTENSITY),
	WHITE
};
#else
static const char* colors[] = {
	"\033[0m",
	"\033[0m\033[30;40m",
	"\033[0m\033[31;40m",
	"\033[0m\033[32;40m",
	"\033[0m\033[33;40m",
	"\033[0m\033[34;40m",
	"\033[0m\033[35;40m",
	"\033[0m\033[36;40m",
	"\033[0m\033[37;40m",
	"\033[1m\033[31;40m",
	"\033[1m\033[32;40m",
	"\033[1m\033[33;40m",
	"\033[1m\033[34;40m",
	"\033[1m\033[35;40m",
	"\033[1m\033[36;40m",
	"\033[1m\033[37;40m"
};
#endif


void textcolor(log_color c)
{
#ifndef NOCOLOR
#ifdef WIN32
	SetConsoleTextAttribute(hConsole, colors[c]);
#else
	print("%s", colors[c]);
#endif
#endif
}

#ifndef ANDROID
void printBracket(const char* status, log_color color)
{
	textcolor(WHITE);
	print("[");
	textcolor(color);
	print("%s", status);
	textcolor(WHITE);
	print("]");
}

void printStatus(const char* status, log_color color)
{
	printBracket(status, color);
	print("\n");
}

void printMessage(const char* owner, const char* message, log_color color, ...)
{
	printBracket(owner, LIGHT_WHITE);
	print(": ");
	textcolor(color);
	va_list ap;

	va_start(ap, color);
	vprint(message, ap);
	va_end(ap);
}
#else /* !ANDROID */
void printBracket(const char* status, log_color color)
{
}

void printStatus(const char* status, log_color color)
{
	__android_log_print(ANDROID_LOG_INFO, "GemRB", "[%s]", status)
}

void printMessage(const char* owner, const char* message, log_color color)
	__android_log_print(ANDROID_LOG_INFO, "GemRB", "%s: %s", owner, message)
}
#endif
