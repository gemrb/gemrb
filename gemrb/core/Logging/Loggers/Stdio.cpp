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

#include "Logging/Loggers/Stdio.h"

#include "EnumIndex.h"
#include "Streams/FileStream.h"

#include <cstdio>

#include "plugindef.h"

namespace GemRB {

StreamLogWriter::StreamLogWriter(log_level level, FILE* stream, ANSIColor color)
: Logger::LogWriter(level), color(color), stream(stream)
{
	assert(stream);
}

StreamLogWriter::~StreamLogWriter()
{
	fclose(stream);
}

static const LOG_FMT& LevelFormat(ANSIColor color, log_level level)
{
	static LOG_FMT nullfmt;

	static const std::array<LOG_FMT, 6> BasicFMT {
		fmt::fg(fmt::terminal_color::bright_red),
		fmt::fg(fmt::terminal_color::bright_red),
		fmt::fg(fmt::terminal_color::bright_yellow),
		fmt::fg(fmt::terminal_color::white),
		fmt::fg(fmt::terminal_color::bright_green),
		fmt::fg(fmt::terminal_color::blue)
	};

	switch (color) {
		case ANSIColor::Basic:
			return BasicFMT[level];
		case ANSIColor::True:
			return Logger::LevelFormat[level];
		default:
			return nullfmt;
	}
}

static const LOG_FMT& DefaultFormat(ANSIColor color)
{
	static EnumArray<ANSIColor, LOG_FMT> formats {
		fmt::text_style(),
		fmt::fg(fmt::terminal_color::bright_white),
		fmt::fg(fmt::color::white) | fmt::emphasis::bold
	};

	return formats[color];
}

static const std::string& DefaultFormatCode(ANSIColor color)
{
	static EnumArray<ANSIColor, std::string> codes {
		"",
		fmt::to_string(fmt::styled("", DefaultFormat(ANSIColor::Basic))),
		fmt::to_string(fmt::styled("", DefaultFormat(ANSIColor::True)))
	};

	return codes[color];
}

void StreamLogWriter::WriteLogMessage(const Logger::LogMessage& msg)
{
	static constexpr char RESET[] = "\x1b[0m";
	static const auto F_STRING = FMT_STRING("[{}{:/>{}}{}]: {}\n");
	
	static const std::string LogLevelText[] = {
		"FATAL",
		"ERROR",
		"WARN",
		"", // MESSAGE
		"COMBAT",
		"DEBUG"
	};

	const auto& lvlTxt = LogLevelText[msg.level];
	const int hasLvl = !lvlTxt.empty();
	if (color != ANSIColor::None) {
		const LOG_FMT& defaultFmt = DefaultFormat(color);
		const std::string& defaultStyle = DefaultFormatCode(color);

		auto Style = [&](auto&& param, fmt::text_style style) {
			if (param.empty()) {
				return param;
			}
			std::string str = RESET;
			str.reserve(param.length() + defaultStyle.length() * 2);
			str += fmt::to_string(fmt::styled(param, style));
			str.append(defaultStyle, 0, defaultStyle.length() - (sizeof(RESET) - 1));
			return str;
		};

		const auto& lvlFmt = LevelFormat(color, msg.level);
		fmt::print(stream, defaultFmt, F_STRING, msg.owner, "", hasLvl, Style(lvlTxt, lvlFmt), Style(msg.message, msg.format));
	} else {
		fmt::print(stream, F_STRING, msg.owner, "", hasLvl, lvlTxt, msg.message);
	}
}

static Logger::WriterPtr createStreamLogWriter(FILE* stream, ANSIColor color)
{
	if (stream) {
		return std::make_shared<StreamLogWriter>(DEBUG, stream, color);
	}
	return nullptr;
}

Logger::WriterPtr createStdioLogWriter(ANSIColor color)
{
	int fd = dup(fileno(stdout));
	return createStreamLogWriter(fdopen(fd, "w"), color);
}

Logger::WriterPtr createStreamLogWriter(FILE* stream)
{
	return createStreamLogWriter(stream, ANSIColor::None);
}

}
