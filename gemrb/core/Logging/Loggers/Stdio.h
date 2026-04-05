// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LOGGER_STDIO_H
#define LOGGER_STDIO_H

#include "Logging/Logger.h" // for log_color

#include <cstdio>

namespace GemRB {

enum class ANSIColor : uint8_t {
	None,
	Basic,
	True,
	count
};

class GEM_EXPORT StreamLogWriter : public Logger::LogWriter {
public:
	StreamLogWriter(LogLevel, FILE*, ANSIColor color);
	~StreamLogWriter() override;

	StreamLogWriter(const StreamLogWriter&) = delete;

	StreamLogWriter& operator=(const StreamLogWriter&) = delete;

	void WriteLogMessage(const Logger::LogMessage& msg) override;
	void Flush() override;

private:
	ANSIColor color;
	FILE* stream;
};

GEM_EXPORT Logger::WriterPtr createStdioLogWriter(ANSIColor);
GEM_EXPORT Logger::WriterPtr createStdioLogWriter();
GEM_EXPORT Logger::WriterPtr createStreamLogWriter(FILE*);

}

#endif
