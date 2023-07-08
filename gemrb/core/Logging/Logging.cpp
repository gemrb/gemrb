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

static std::atomic<log_level> CWLL;

std::deque<Logger::WriterPtr> writers;

std::unique_ptr<Logger> logger;

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
		static const wchar_t* colors[] = {
			L"[color=FFFFFF]",	// DEFAULT
			L"[color=000000]",	// BLACK
			L"[color=FF0000]",	// RED
			L"[color=00FF00]",	// GREEN
			L"[color=603311]",	// BROWN
			L"[color=0000FF]",	// BLUE
			L"[color=8B008B]",	// MAGENTA
			L"[color=00CDCD]",	// CYAN
			L"[color=FFFFFF]",	// WHITE
			L"[color=CD5555]",	// LIGHT_RED
			L"[color=90EE90]",	// LIGHT_GREEN
			L"[color=FFFF00]",	// YELLOW
			L"[color=BFEFFF]",	// LIGHT_BLUE
			L"[color=FF00FF]",	// LIGHT_MAGENTA
			L"[color=B4CDCD]",	// LIGHT_CYAN
			L"[color=CDCDCD]"	// LIGHT_WHITE
		};
		static constexpr log_color log_level_color[] = {
			RED,
			RED,
			YELLOW,
			LIGHT_WHITE,
			GREEN,
			BLUE
		};

		int level = msg.level == INTERNAL ? 0 : msg.level;
		String* decodedMsg = StringFromCString(msg.message.c_str());
		String* decodedOwner = StringFromCString(msg.owner.c_str());
		ta->AppendText(fmt::format(L"{}{}: [/color]{}{}[/color]\n", colors[msg.color], *decodedOwner, colors[log_level_color[level]], *decodedMsg));
		delete decodedMsg;
		delete decodedOwner;
	}
}

void SetConsoleWindowLogLevel(log_level level)
{
	if (level <= INTERNAL) {
		static const LogMessage offMsg(INTERNAL, "Logger", "MessageWindow logging disabled.", LIGHT_RED);
		ConsoleWinLogMsg(offMsg);
	} else if (level <= DEBUG) {
		static const LogMessage onMsg(INTERNAL, "Logger", "MessageWindow logging active.", LIGHT_GREEN);
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
	FileStream* log_file = new FileStream();
	path_t log_path = PathJoin(config.GamePath, "GemRB.log");
	if (log_file->Create(log_path)) {
		AddLogWriter(createStreamLogWriter(log_file));
	} else {
		log_path = PathJoin(config.CachePath, "GemRB.log");
		if (log_file->Create(log_path)) {
			AddLogWriter(createStreamLogWriter(log_file));
		} else {
			Log (WARNING, "Logger", "Could not create a log file, skipping!");
			delete log_file;
		}
	}
}

}

GEMRB_PLUGIN(unused, "tmp/file logger")
PLUGIN_INITIALIZER(addGemRBLog)
END_PLUGIN()
