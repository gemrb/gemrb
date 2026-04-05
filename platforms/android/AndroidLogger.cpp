// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AndroidLogger.h"

#include <android/log.h>

namespace GemRB {

void AndroidLogger::WriteLogMessage(const Logger::LogMessage& msg)
{
	android_LogPriority priority = ANDROID_LOG_INFO;
	switch (msg.level) {
		case FATAL:
			priority = ANDROID_LOG_FATAL;
			break;
		case ERROR:
			priority = ANDROID_LOG_ERROR;
			break;
		case WARNING:
			priority = ANDROID_LOG_WARN;
			break;
		case DEBUG:
			priority = ANDROID_LOG_DEBUG;
			break;
	}
	__android_log_print(priority, "GemRB", "[%s/%s]: %s", msg.owner, log_level_text[msg.level], msg.message);
}

Logger::WriterPtr createAndroidLogger()
{
	return Logger::WriterPtr(new AndroidLogger());
}

}
