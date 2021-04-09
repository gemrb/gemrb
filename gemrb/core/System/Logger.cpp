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

#include "System/Logger.h"

#include "System/Logging.h"

#include <cstdio>

namespace GemRB {

Logger::Logger(std::deque<WriterPtr> writers)
: writers(std::move(writers))
{
	loggingThread = std::thread([this] {
		QueueType queue;
		while (running) {
			std::unique_lock<std::mutex> lk(queueLock);
			cv.wait(lk, [this]() { return !messageQueue.empty() || !running; });
			queue.swap(messageQueue);
			lk.unlock();
			ProcessMessages(std::move(queue));
		}
	});
}

Logger::~Logger()
{
	running = false;
	cv.notify_all();
	loggingThread.join();
}

void Logger::AddLogWriter(WriterPtr writer)
{
	std::lock_guard<std::mutex> l(writerLock);
	writers.push_back(std::move(writer));
}

void Logger::ProcessMessages(QueueType queue)
{
	std::lock_guard<std::mutex> l(writerLock);
	while (queue.size()) {
		for (const auto& writer : writers) {
			writer->WriteLogMessage(queue.front());
		}
		queue.pop_front();
	}
}

void Logger::LogMsg(log_level level, const char* owner, const char* message, log_color color)
{
	LogMsg(LogMessage(level, owner, message, color));
}

void Logger::LogMsg(LogMessage&& msg)
{	
	if (msg.level < FATAL) {
		msg.level = FATAL;
	}
	
	if (msg.level == FATAL) {
		// fatal errors must happen now!
		std::lock_guard<std::mutex> l(writerLock);
		for (const auto& writer : writers) {
			writer->WriteLogMessage(std::move(msg));
		}
	} else {
		std::lock_guard<std::mutex> l(queueLock);
		messageQueue.push_back(std::move(msg));
		cv.notify_all();
	}
}

const char* log_level_text[] = {
	"FATAL",
	"ERROR",
	"WARNING",
	"", // MESSAGE
	"COMBAT",
	"DEBUG"
};

}
