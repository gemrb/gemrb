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

#include <cstdio>

#define ADV_TEXT
#include <conio.h>

namespace GemRB {

Win32ConsoleLogger::Win32ConsoleLogger(log_level level, bool useColor)
: StdioLogWriter(level, useColor)
{
	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
}

void Win32ConsoleLogger::textcolor(log_color c)
{
	static constexpr int colors[] = {
		0,
		0,
		FOREGROUND_RED,
		FOREGROUND_GREEN,
		FOREGROUND_GREEN | FOREGROUND_RED,
		FOREGROUND_BLUE,
		FOREGROUND_RED | FOREGROUND_BLUE,
		FOREGROUND_BLUE | FOREGROUND_GREEN,
		FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
		FOREGROUND_RED | FOREGROUND_INTENSITY,
		FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
		FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY,
		FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED,
	};

	if (useColor)
		SetConsoleTextAttribute(hConsole, colors[c]);
}

Logger::LogWriter* createWin32ConsoleLogger()
{
	return new Win32ConsoleLogger(DEBUG, true);
}

}
