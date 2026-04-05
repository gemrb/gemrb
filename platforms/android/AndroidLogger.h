// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LOGGER_ANDROID_H
#define LOGGER_ANDROID_H

#include "Logging/Logger.h"

namespace GemRB {

class GEM_EXPORT AndroidLogger : public Logger::LogWriter {
public:
	void WriteLogMessage(const Logger::LogMessage& msg) override;
};

Logger::WriterPtr createAndroidLogger();

}

#endif
