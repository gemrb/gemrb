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

std::atomic_bool Logger::EnableLogging {true};

Logger::Logger()
{
	loggingThread = std::thread([this] {
		QueueType queue;
		while (running) {
			std::unique_lock<std::mutex> lk(queueLock);
			cv.wait(lk);
			if (messageQueue.size()) {
				queue.swap(messageQueue);
			}
			lk.unlock();
			ProcessMessages(std::move(queue));
		}
	});
}

Logger::~Logger()
{
	running = false;
	loggingThread.join();
}

Logger::LogWriterID Logger::AddLogWriter(WriterPtr&& writer)
{
	std::lock_guard<std::mutex> l(writerLock);
	writers.push_back(std::move(writer));
	return LogWriterID(&writers.back());
}

void Logger::DestroyLogWriter(LogWriterID id)
{
	std::lock_guard<std::mutex> l(writerLock);
	auto it = std::find_if(writers.begin(), writers.end(), [id](const std::unique_ptr<LogWriter>& writer) {
		return LogWriterID(&writer) == id;
	});

	if (it != writers.end()) {
		writers.erase(it);
	}
}

void Logger::ProcessMessages(QueueType queue)
{
	std::lock_guard<std::mutex> l(writerLock);
	while (queue.size()) {
		for (auto& writer : writers) {
			writer->WriteLogMessage(queue.front());
		}
		queue.pop_front();
	}
}

void Logger::LogMsg(log_level level, const char* owner, const char* message, log_color color)
{
	if (EnableLogging) LogMsg(LogMessage(level, owner, message, color));
}

void Logger::LogMsg(LogMessage&& msg)
{
	if (EnableLogging == false) return;
	
	if (msg.level < FATAL) {
		msg.level = FATAL;
	}

	std::lock_guard<std::mutex> l(queueLock);
	messageQueue.push_back(std::move(msg));
	cv.notify_all();
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

using namespace GemRB;

#ifdef ANDROID

#include "System/Logger/Android.h"
Logger::LogWriter* (*GemRB::createDefaultLogWriter)() = createAndroidLogWriter;

#elif defined(WIN32) && !defined(WIN32_USE_STDIO)

#include "System/Logger/Win32Console.h"
Logger::LogWriter* (*GemRB::createDefaultLogWriter)() = createWin32ConsoleLogWriter;

#elif defined (VITA)

#include "System/Logger/Vita.h"
Logger::LogWriter* (*GemRB::createDefaultLogWriter)() = createVitaLogWriter;

#else

#include "System/Logger/Stdio.h"
Logger::LogWriter* (*GemRB::createDefaultLogWriter)() = createStdioLogWriter;

#endif

