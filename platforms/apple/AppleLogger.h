// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LOGGER_APPLE_H
#define LOGGER_APPLE_H

#include "Logging/Loggers/Stdio.h"

namespace GemRB {

class GEM_EXPORT AppleLogger : public Logger::LogWriter {
public:
	AppleLogger();

protected:
	void WriteLogMessage(const Logger::LogMessage& msg) override;
};

}

#endif
