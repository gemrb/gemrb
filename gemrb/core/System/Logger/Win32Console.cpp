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

#include "System/Logger/Win32Console.h"

#include "System/Logging.h"

#include <cstdio>

#define ADV_TEXT
#include <conio.h>

#ifdef __MINGW32__
# define cprintf _cprintf
#endif

Win32ConsoleLogger::Win32ConsoleLogger(bool useColor)
	: StdioLogger(useColor)
{
	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
}

Win32ConsoleLogger::~Win32ConsoleLogger()
{}

void Win32ConsoleLogger::vprint(const char *message, va_list ap)
{
	// Don't try to be smart.
	// Assume this is long enough. If not, message will be truncated.
	// MSVC6 has old vsnprintf that doesn't give length
	char buff[_MAX_PATH];
	vsnprintf(buff, _MAX_PATH, message, ap);
	cprintf("%s", buff);
}

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

void Win32ConsoleLogger::textcolor(log_color c)
{
	if (useColor)
		SetConsoleTextAttribute(hConsole, colors[c]);
}

Logger* createWin32ConsoleLogger()
{
	return new Win32ConsoleLogger(true);
}
