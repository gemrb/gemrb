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

#include "System/Logging.h"

namespace GemRB {

enum log_color {
	DEFAULT,
	BLACK,
	RED,
	GREEN,
	BROWN,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	LIGHT_RED,
	LIGHT_GREEN,
	YELLOW,
	LIGHT_BLUE,
	LIGHT_MAGENTA,
	LIGHT_CYAN,
	LIGHT_WHITE
};

class GEM_EXPORT Logger {
private:
	log_level myLevel;
public:
	Logger(log_level = DEBUG);
	virtual ~Logger();
	virtual void destroy();

	bool SetLogLevel(log_level);
	void log(log_level, const char* owner, const char* message, log_color color);
protected:
	virtual void LogInternal(log_level, const char*, const char*, log_color) {};
};

extern const char* log_level_text[];

extern Logger* (*createDefaultLogger)();

}

#endif
