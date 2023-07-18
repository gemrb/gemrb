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

#include "Win32Console.h"

#include "config.h"

#define ADV_TEXT
#include <conio.h>
#include <fcntl.h>

namespace GemRB {

static FILE* HandleToFile(HANDLE handle) {
	if (handle == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	int fd = _open_osfhandle((intptr_t)handle, _O_TEXT);
	if (fd == -1) {
		return nullptr;
	}
	return _fdopen(fd, "w");
}

Win32ConsoleLogger::Win32ConsoleLogger(log_level level, bool useColor)
: StreamLogWriter(level, HandleToFile(GetStdHandle(STD_OUTPUT_HANDLE)), useColor)
{
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleMode(hConsole, &dwMode);
	SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

Win32ConsoleLogger::~Win32ConsoleLogger()
{
	SetConsoleMode(hConsole, dwMode);
}

Logger::WriterPtr createWin32ConsoleLogger()
{
	return Logger::WriterPtr(new Win32ConsoleLogger(DEBUG, true));
}

}
