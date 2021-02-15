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

#include <cstdio>

namespace GemRB {

StdioLogWriter::StdioLogWriter(log_level level, bool useColor)
: LogWriter(level), useColor(useColor)
{}

void StdioLogWriter::print(const char* message)
{
	fprintf(stdout, "%s", message);
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


void StdioLogWriter::textcolor(log_color c)
{
	// Shold this be in an ansi-term subclass?
	// Probably not worth the bother
	if (useColor)
		print(colors[c]);
}

void StdioLogWriter::printBracket(const char* status, log_color color)
{
	textcolor(WHITE);
	print("[");
	textcolor(color);
	print(status);
	textcolor(WHITE);
	print("]");
}

void StdioLogWriter::printStatus(const char* status, log_color color)
{
	printBracket(status, color);
	print("\n");
}

static constexpr log_color log_level_color[] = {
	LIGHT_RED,
	LIGHT_RED,
	YELLOW,
	LIGHT_WHITE,
	GREEN,
	BLUE
};

void StdioLogWriter::WriteLogMessage(const Logger::LogMessage& msg)
{
	textcolor(LIGHT_WHITE);
	print("[");
	print(msg.owner.c_str());
	if (log_level_text[msg.level][0]) {
		print("/");
		textcolor(log_level_color[msg.level]);
		print(log_level_text[msg.level]);
	}
	textcolor(LIGHT_WHITE);
	print("]: ");

	textcolor(msg.color);
	print(msg.message.c_str());
	print("\n");
}

Logger::LogWriter* createStdioLogWriter()
{
#ifndef NOCOLOR
	return new StdioLogWriter(DEBUG, true);
#else
	return new StdioLogWriter(DEBUG, false);
#endif
}

}
