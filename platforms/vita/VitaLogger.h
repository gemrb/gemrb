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
