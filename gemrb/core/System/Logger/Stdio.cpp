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
#include "System/FileStream.h"

#include <cstdio>

#include "Interface.h"
#include "plugindef.h"

namespace GemRB {

StreamLogWriter::StreamLogWriter(log_level level, DataStream* stream)
: Logger::LogWriter(level), stream(stream)
{}

StreamLogWriter::~StreamLogWriter()
{
	delete stream;
}

void StreamLogWriter::Print(const std::string& msg)
{
	stream->Write(msg.c_str(), (uint32_t)msg.length());
}

void StreamLogWriter::WriteLogMessage(const Logger::LogMessage& msg)
{
	Print("[" + msg.owner + "/" + log_level_text[msg.level] + "]: " + msg.message + "\n");
}

Logger::WriterPtr createStreamLogWriter(DataStream* stream)
{
	return Logger::WriterPtr(new StreamLogWriter(DEBUG, stream));
}

static FileStream* DupStdOut()
{
	int fd = dup(fileno(stdout));
	assert(fd != -1);
	FILE* fp = fdopen(fd, "w");
	return new FileStream(File(fp));
}

StdioLogWriter::StdioLogWriter(log_level level, bool useColor)
: StreamLogWriter(level, DupStdOut()), useColor(useColor)
{}

StdioLogWriter::~StdioLogWriter()
{
	textcolor(DEFAULT); // undo any changes to the terminal
}

void StdioLogWriter::textcolor(log_color c)
{
	// Shold this be in an ansi-term subclass?
	// Probably not worth the bother
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

	if (useColor)
		Print(colors[c]);
}

void StdioLogWriter::printBracket(const char* status, log_color color)
{
	textcolor(WHITE);
	Print("[");
	textcolor(color);
	Print(status);
	textcolor(WHITE);
	Print("]");
}

void StdioLogWriter::printStatus(const char* status, log_color color)
{
	printBracket(status, color);
	Print("\n");
}

void StdioLogWriter::WriteLogMessage(const Logger::LogMessage& msg)
{
	if (useColor) {
		static constexpr log_color log_level_color[] = {
			LIGHT_RED,
			LIGHT_RED,
			YELLOW,
			LIGHT_WHITE,
			GREEN,
			BLUE
		};

		textcolor(LIGHT_WHITE);
		Print("[");
		Print(msg.owner);
		if (log_level_text[msg.level][0]) {
			Print("/");
			textcolor(log_level_color[msg.level]);
			Print(log_level_text[int(msg.level)]);
		}
		textcolor(LIGHT_WHITE);
		Print("]: ");

		textcolor(msg.color);
		Print(msg.message);
		Print("\n");
	} else {
		StreamLogWriter::WriteLogMessage(msg);
	}
	
	fflush(stdout);
}

Logger::WriterPtr createStdioLogWriter()
{
#ifndef NOCOLOR
	return Logger::WriterPtr(new StdioLogWriter(DEBUG, true));
#else
	return Logger::WriterPtr(new StdioLogWriter(DEBUG, false));
#endif
}

}
