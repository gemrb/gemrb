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
#include "System/StringBuffer.h"

#include "Interface.h"
#include "DisplayMessage.h"
#include "GUI/GameControl.h"

#if defined(__sgi)
#  include <stdarg.h>
#else
#  include <cstdarg>
#endif
#include <memory>
#include <vector>

#ifndef STATIC_LINK
# define STATIC_LINK
#endif
#include "plugindef.h"

namespace GemRB {

using LogMessage = Logger::LogMessage;

static std::atomic<log_level> MWLL;

std::deque<Logger::WriterPtr> writers;

std::unique_ptr<Logger> logger;

void ToggleLogging(bool enable)
{
	if (enable && logger == nullptr) {
		logger = std::unique_ptr<Logger>(new Logger(writers));
	} else if (!enable) {
		logger = nullptr;
	}
}

static void MessageWinLogMsg(const LogMessage& msg)
{
	if (msg.level > MWLL || msg.level < INTERNAL) return;
	
	const GameControl* gc = core->GetGameControl();
	if (displaymsg && gc && !(gc->GetDialogueFlags()&DF_IN_DIALOG)) {
		// FIXME: we check DF_IN_DIALOG here to avoid recurssion in the MessageWindowLogger, but what happens when an error happens during dialog?
		// I'm not super sure about how to avoid that. for now the logger will not log anything in dialog mode.
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

		const wchar_t* fmt = L"%s%s: [/color]%s%s[/color]";
		size_t len = msg.message.length() + msg.owner.length() + wcslen(fmt) + 28; // 28 is for sizeof(colors[x]) * 2
		wchar_t* text = (wchar_t*)malloc(len * sizeof(wchar_t));
		swprintf(text, len, fmt, colors[msg.color], msg.owner.c_str(), colors[log_level_color[msg.level]], msg.message.c_str());
		displaymsg->DisplayMarkupString(text);
		free(text);
	}
}

void SetMessageWindowLogLevel(log_level level)
{
	if (level <= INTERNAL) {
		static const LogMessage offMsg(INTERNAL, "Logger", "MessageWindow logging disabled.", LIGHT_RED);
		MessageWinLogMsg(offMsg);
	} else if (level <= DEBUG) {
		static const LogMessage onMsg(INTERNAL, "Logger", "MessageWindow logging active.", LIGHT_GREEN);
		MessageWinLogMsg(onMsg);
	}
	MWLL = level;
}

static void LogMsg(LogMessage&& msg)
{
	MessageWinLogMsg(msg);
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
    const size_t len = vsnprintf(NULL, 0, message, ap_copy);
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

void Log(log_level level, const char* owner, StringBuffer const& buffer)
{
	LogMsg(LogMessage(level, owner, buffer.get().c_str(), WHITE));
}

static void addGemRBLog()
{
	char log_path[_MAX_PATH];
	FileStream* log_file = new FileStream();
	PathJoin(log_path, core->GamePath, "GemRB.log", NULL);
	if (log_file->Create(log_path)) {
		AddLogWriter(createStreamLogWriter(log_file));
	} else {
		PathJoin(log_path, core->CachePath, "GemRB.log", NULL);
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
