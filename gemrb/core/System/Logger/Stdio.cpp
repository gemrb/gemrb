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

#include "System/Logger/Stdio.h"

#include "System/Logging.h"

#include <cstdio>

StdioLogger::StdioLogger(bool useColor)
	: useColor(useColor)
{}

StdioLogger::~StdioLogger()
{}

void StdioLogger::vprint(const char *message, va_list ap)
{
	vprintf(message, ap);
}

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


void StdioLogger::textcolor(log_color c)
{
	// Shold this be in an ansi-term subclass?
	// Probably not worth the bother
	if (useColor)
		print("%s", colors[c]);
}

void StdioLogger::printBracket(const char* status, log_color color)
{
	textcolor(WHITE);
	print("[");
	textcolor(color);
	print("%s", status);
	textcolor(WHITE);
	print("]");
}

void StdioLogger::printStatus(const char* status, log_color color)
{
	printBracket(status, color);
	print("\n");
}

void StdioLogger::vprintMessage(const char* owner, const char* message, log_color color, va_list ap)
{
	printBracket(owner, LIGHT_WHITE);
	print(": ");
	textcolor(color);

	vprint(message, ap);
}

Logger* createStdioLogger()
{
#ifndef NOCOLOR
	return new StdioLogger(true);
#else
	return new StdioLogger(false);
#endif
}
