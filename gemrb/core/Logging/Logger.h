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

/**
 * @file Logger.h
 * Logging targets
 * @author The GemRB Project
 */

#ifndef LOGGER_H
#define LOGGER_H

#include "exports.h"
#include "Strings/Format.h"
#include <fmt/color.h>

#include <array>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace GemRB {

// !!! Keep this synchronized with GUIDefines !!!
enum log_level : int {
	INTERNAL = -1, // special value that can only be used by the logger itself. these messages cannot be supressed
	FATAL = 0,
	ERROR = 1,
	WARNING = 2,
	MESSAGE = 3,
	COMBAT = 4,
	DEBUG = 5
};

using LOG_FMT = fmt::text_style;

class GEM_EXPORT Logger final {
public:
	static const std::array<LOG_FMT, 6> LevelFormat;
	static const LOG_FMT MSG_STYLE;

	struct LogMessage {
		log_level level = DEBUG;
		std::string owner;
		std::string message;
		LOG_FMT format;
		
		LogMessage(log_level level, std::string owner, std::string message, LOG_FMT fmt)
		: level(level), owner(std::move(owner)), message(std::move(message)), format(fmt) {}
	};

	class LogWriter {
	public:
		std::atomic<log_level> level;
		
		explicit LogWriter(log_level level) : level(level) {}
		virtual ~LogWriter() noexcept = default;
		
		void WriteLogMessage(log_level level, const char* owner, const char* message, LOG_FMT fmt) {
			WriteLogMessage(LogMessage(level, owner, message, fmt));
		}
		virtual void WriteLogMessage(const Logger::LogMessage& msg)=0;
	};

	using WriterPtr = std::shared_ptr<LogWriter>;
private:
	using QueueType = std::deque<LogMessage>;
	QueueType messageQueue;
	std::deque<WriterPtr> writers;
	
	std::atomic_bool running {true};
	std::condition_variable cv;
	std::mutex queueLock;
	std::mutex writerLock;
	std::thread loggingThread;
	
	void threadLoop();
	void ProcessMessages(QueueType queue);
	
public:
	explicit Logger(std::deque<WriterPtr>);
	~Logger();
	
	void AddLogWriter(WriterPtr writer);

	void LogMsg(log_level, const char* owner, const char* message, LOG_FMT fmt);
	void LogMsg(LogMessage&& msg);
};

}

#endif
