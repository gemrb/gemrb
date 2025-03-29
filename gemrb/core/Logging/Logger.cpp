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

#include "Logging/Logger.h"

namespace GemRB {

const EnumArray<LogLevel, LOG_FMT> Logger::LevelFormat {
	fmt::fg(fmt::color::red) | fmt::emphasis::bold,
	fmt::fg(fmt::color::red) | fmt::emphasis::bold,
	fmt::fg(fmt::color::yellow) | fmt::emphasis::bold,
	fmt::fg(fmt::color::white),
	fmt::fg(fmt::color::green) | fmt::emphasis::bold,
	fmt::fg(fmt::color::blue)
};

const LOG_FMT Logger::MSG_STYLE = fmt::fg(fmt::color::ghost_white);

Logger::Logger(std::deque<WriterPtr> writers)
	: writers(std::move(writers))
{}

Logger::~Logger()
{
	running = false;
	cv.notify_all();
	if (loggingThread.joinable())
		loggingThread.join();
}

void Logger::StartProcessingThread()
{
	loggingThread = std::thread([this] {
		while (running) {
			QueueType queue;
			std::unique_lock<std::mutex> lk(queueLock);
			cv.wait(lk, [this]() { return !messageQueue.empty() || !running; });
			queue.swap(messageQueue);
			lk.unlock();
			ProcessMessages(std::move(queue));
		}
	});
}

void Logger::AddLogWriter(WriterPtr writer)
{
	std::lock_guard<std::mutex> l(writerLock);
	writers.push_back(std::move(writer));

	if (!loggingThread.joinable()) {
		StartProcessingThread();
		cv.notify_all(); // notify for anything already queued
	}
}

void Logger::ProcessMessages(QueueType queue)
{
	std::lock_guard<std::mutex> l(writerLock);
	while (!queue.empty()) {
		for (const auto& writer : writers) {
			writer->WriteLogMessage(queue.front());
		}
		queue.pop_front();
	}
	for (const auto& writer : writers) {
		writer->Flush();
	}
}

void Logger::LogMsg(LogLevel level, const char* owner, const char* message, LOG_FMT fmt)
{
	LogMsg(LogMessage(level, owner, message, fmt));
}

void Logger::LogMsg(LogMessage&& msg)
{
	if (msg.level == FATAL) {
		// fatal errors must happen now!
		std::lock_guard<std::mutex> l(writerLock);
		for (const auto& writer : writers) {
			writer->WriteLogMessage(msg);
			writer->Flush();
		}
	} else {
		std::lock_guard<std::mutex> l(queueLock);
		messageQueue.push_back(std::move(msg));
		cv.notify_all();
	}
}

void Logger::Flush()
{
	cv.notify_all();
	std::lock_guard<std::mutex> l(writerLock);
	for (const auto& writer : writers) {
		writer->Flush();
	}
}

}
