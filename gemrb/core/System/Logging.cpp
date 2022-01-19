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

#include "System/Logging.h"
#include "System/FileStream.h"
#include "System/Logger/Stdio.h"

#include "Interface.h"
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
		logger = GemRB::make_unique<Logger>(writers);
	} else if (!enable) {
		logger = nullptr;
	}
}

static void ConsoleWinLogMsg(const LogMessage& msg)
{
	if (msg.level > CWLL || msg.level < INTERNAL) return;
	
	TextArea* ta = GetControl<TextArea>("CONSOLE", 1);
	
	if (ta) {
		static const char* colors[] = {
			"[color=FFFFFF]",	// DEFAULT
			"[color=000000]",	// BLACK
			"[color=FF0000]",	// RED
			"[color=00FF00]",	// GREEN
			"[color=603311]",	// BROWN
			"[color=0000FF]",	// BLUE
			"[color=8B008B]",	// MAGENTA
			"[color=00CDCD]",	// CYAN
			"[color=FFFFFF]",	// WHITE
			"[color=CD5555]",	// LIGHT_RED
			"[color=90EE90]",	// LIGHT_GREEN
			"[color=FFFF00]",	// YELLOW
			"[color=BFEFFF]",	// LIGHT_BLUE
			"[color=FF00FF]",	// LIGHT_MAGENTA
			"[color=B4CDCD]",	// LIGHT_CYAN
			"[color=CDCDCD]"	// LIGHT_WHITE
		};
		static constexpr log_color log_level_color[] = {
			RED,
			RED,
			YELLOW,
			LIGHT_WHITE,
			GREEN,
			BLUE
		};

		const wchar_t* fmt = L"%s%s: [/color]%s%s[/color]\n";
		size_t len = msg.message.length() + msg.owner.length() + wcslen(fmt) + 2 * strlen(colors[0]);
		String text(len, L'\0');
		int level = msg.level == INTERNAL ? 0 : msg.level;
		swprintf(&text[0], len, fmt, colors[msg.color], msg.owner.c_str(), colors[log_level_color[level]], msg.message.c_str());
		ta->AppendText(text);
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

static void LogMsg(LogMessage&& msg)
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

static void vLog(log_level level, const char* owner, const char* message, log_color color, va_list ap)
{
	va_list ap_copy;
	va_copy(ap_copy, ap);
	const size_t len = vsnprintf(nullptr, 0, message, ap_copy);
	va_end(ap_copy);

	char *buf = new char[len+1];
	vsnprintf(buf, len + 1, message, ap);
	LogMsg(LogMessage(level, owner, buf, color));
	delete[] buf;
}

void print(const char *message, ...)
{
	va_list ap;
	va_start(ap, message);
	vLog(MESSAGE, "Unknown", message, WHITE, ap);
	va_end(ap);
}

void error(const char* owner, const char* message, ...)
{
	va_list ap;
	va_start(ap, message);
	vLog(FATAL, owner, message, LIGHT_RED, ap);
	va_end(ap);

	exit(1);
}

void Log(log_level level, const char* owner, const char* message, ...)
{
	va_list ap;
	va_start(ap, message);
	vLog(level, owner, message, WHITE, ap);
	va_end(ap);
}

void LogVA(log_level level, const char* owner, const char* message, va_list args)
{
	vLog(level, owner, message, WHITE, args);
}

void Log(log_level level, const char* owner, std::string const& buffer)
{
	LogMsg(LogMessage(level, owner, buffer.c_str(), WHITE));
}

static void addGemRBLog()
{
	char log_path[_MAX_PATH];
	FileStream* log_file = new FileStream();
	PathJoin(log_path, core->config.GamePath, "GemRB.log", nullptr);
	if (log_file->Create(log_path)) {
		AddLogWriter(createStreamLogWriter(log_file));
	} else {
		PathJoin(log_path, core->config.CachePath, "GemRB.log", nullptr);
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
