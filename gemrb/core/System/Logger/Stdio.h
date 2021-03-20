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

#ifndef LOGGER_STDIO_H
#define LOGGER_STDIO_H

#include "System/Logger.h" // for log_color

namespace GemRB {

class DataStream;

class GEM_EXPORT StreamLogWriter : public Logger::LogWriter {
public:
	StreamLogWriter(log_level, DataStream*);
	~StreamLogWriter();
	
	StreamLogWriter(const StreamLogWriter&) = delete;

	StreamLogWriter& operator=(const StreamLogWriter&) = delete;

	void WriteLogMessage(const Logger::LogMessage& msg) override;

protected:
	void Print(const std::string& message);

private:
	DataStream* stream;
};

class GEM_EXPORT StdioLogWriter : public StreamLogWriter {
public:
	StdioLogWriter(log_level, bool useColor);
	~StdioLogWriter();
	
	void WriteLogMessage(const Logger::LogMessage& msg) override;
protected:
	void textcolor(log_color);
	bool useColor;
private:
	void printBracket(const char *status, log_color color);
	void printStatus(const char* status, log_color color);
};

GEM_EXPORT Logger::WriterPtr createStdioLogWriter();
Logger::WriterPtr createStreamLogWriter(DataStream*);

}

#endif
