// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LOGGER_VITA_H
#define LOGGER_VITA_H

#include "Logging/Logger.h"

namespace GemRB {

class GEM_EXPORT VitaLogger : public Logger::LogWriter {
public:
	VitaLogger(log_level level)
		: Logger::LogWriter(level) {};

protected:
	void LogInternal(log_level, const char*, const char*, log_color) override;
};

Logger::WriterPtr createVitaLogger();

}

#endif
