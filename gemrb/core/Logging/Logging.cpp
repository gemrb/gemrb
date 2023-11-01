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

#include "Logging/Logging.h"
#include "Streams/FileStream.h"
#include "Logging/Loggers/Stdio.h"

#include "GUI/GUIScriptInterface.h"
#include "GUI/TextArea.h"

#include <cstdarg>
#include <memory>
#include <vector>

#ifndef STATIC_LINK
# define STATIC_LINK
#endif
#include "plugindef.h"

namespace GemRB {

using LogMessage = Logger::LogMessage;

static std::atomic<LogLevel> CWLL;
static std::deque<Logger::WriterPtr> writers;
static std::unique_ptr<Logger> logger;

void ToggleLogging(bool enable)
{
	if (enable && logger == nullptr) {
		logger = std::make_unique<Logger>(writers);
	} else if (!enable) {
		logger = nullptr;
	}
}

static void ConsoleWinLogMsg(const LogMessage& msg)
{
	if (msg.level > CWLL || msg.level < INTERNAL) return;
	
	TextArea* ta = GetControl<TextArea>("CONSOLE", 1);
	
	if (ta) {
		auto GetRGB = [](const fmt::text_style& style) {
			return style.get_foreground().value.rgb_color;
		};
		
		LogLevel level = msg.level == INTERNAL ? FATAL : msg.level;
		const auto& format = Logger::LevelFormat[level];
		const auto& FMT = FMT_STRING(u"[color={:X}]{}: [/color][color={:X}]{}[/color]\n");
		// MessageWindow supports only colors
		ta->AppendText(fmt::format(FMT, GetRGB(msg.format), StringFromTLK(msg.owner),
								   GetRGB(format), StringFromTLK(msg.message)));
	}
}

void SetConsoleWindowLogLevel(LogLevel level)
{
	assert(level <= DEBUG);
	if (level == INTERNAL) {
		static const LogMessage offMsg(INTERNAL, "Logger", "MessageWindow logging disabled.", fmt::fg(fmt::color::red));
		ConsoleWinLogMsg(offMsg);
	} else {
		static const LogMessage onMsg(INTERNAL, "Logger", "MessageWindow logging active.", fmt::fg(fmt::color::green));
		ConsoleWinLogMsg(onMsg);
	}
	CWLL = level;
}

void LogMsg(LogMessage&& msg)
{
	ConsoleWinLogMsg(msg);
	if (logger) {
		logger->LogMsg(std::move(msg));
	}
}

void AddLogWriter(Logger::WriterPtr&& writer)
{
	writers.push_back(std::move(writer));
	if (logger) {
		return logger->AddLogWriter(writers.back());
	}
}

static void addGemRBLog(const CoreSettings& config)
{
	path_t path = PathJoin<false>(config.GamePath, "GemRB.log");
	FILE* file = fopen(path.c_str(), "wb");
	if (file) {
		AddLogWriter(createStreamLogWriter(file));
	} else {
		path = PathJoin(config.CachePath, "GemRB.log");
		file = fopen(path.c_str(), "wb");
		if (file) {
			AddLogWriter(createStreamLogWriter(file));
		} else {
			Log (WARNING, "Logger", "Could not create a log file, skipping!");
		}
	}
}

}

GEMRB_PLUGIN(unused, "tmp/file logger")
PLUGIN_INITIALIZER(addGemRBLog)
END_PLUGIN()
