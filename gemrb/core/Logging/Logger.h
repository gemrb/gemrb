// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file Logger.h
 * Logging targets
 * @author The GemRB Project
 */

#ifndef LOGGER_H
#define LOGGER_H

#include "exports.h"

#include "EnumIndex.h"

#include "fmt/color.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace GemRB {

// !!! Keep this synchronized with GUIDefines !!!
enum LogLevel : uint8_t {
	INTERNAL = uint8_t(-1), // special value that can only be used by the logger itself. These messages cannot be suppressed
	FATAL = 0,
	ERROR = 1,
	WARNING = 2,
	MESSAGE = 3,
	COMBAT = 4,
	DEBUG = 5,
	count
};

using LOG_FMT = fmt::text_style;

class GEM_EXPORT Logger final {
public:
	static const EnumArray<LogLevel, LOG_FMT> LevelFormat;
	static const LOG_FMT MSG_STYLE;

	struct LogMessage {
		LogLevel level = DEBUG;
		std::string owner;
		std::string message;
		LOG_FMT format;

		LogMessage(LogLevel level, std::string owner, std::string message, LOG_FMT fmt)
			: level(level), owner(std::move(owner)), message(std::move(message)), format(fmt) {}
	};

	class LogWriter {
	public:
		std::atomic<LogLevel> level;

		explicit LogWriter(LogLevel level)
			: level(level) {}
		virtual ~LogWriter() noexcept = default;

		void WriteLogMessage(LogLevel logLevel, const char* owner, const char* message, LOG_FMT fmt)
		{
			WriteLogMessage(LogMessage(logLevel, owner, message, fmt));
		}
		virtual void WriteLogMessage(const Logger::LogMessage& msg) = 0;
		virtual void Flush() {};
	};

	using WriterPtr = std::shared_ptr<LogWriter>;

private:
	using QueueType = std::deque<LogMessage>;
	QueueType messageQueue;
	std::deque<WriterPtr> writers;

	std::atomic_bool running { true };
	std::condition_variable cv;
	std::mutex queueLock;
	std::mutex writerLock;
	std::thread loggingThread;

	void threadLoop();
	void ProcessMessages(QueueType queue);
	void StartProcessingThread();

public:
	explicit Logger(std::deque<WriterPtr>);
	~Logger();

	void AddLogWriter(WriterPtr writer);

	void LogMsg(LogLevel, const char* owner, const char* message, LOG_FMT fmt);
	void LogMsg(LogMessage&& msg);
	void Flush();
};

}

#endif
