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

#ifndef LOGGER_APPLE_H
#define LOGGER_APPLE_H

#include "System/Logger/Stdio.h"

namespace GemRB {

class GEM_EXPORT AppleLogger : public StdioLogger {
public:
	AppleLogger();
	virtual ~AppleLogger();

	void textcolor(log_color);
protected:
	void LogInternal(log_level level, const char* owner, const char* message, log_color color);
};

Logger* createAppleLogger();

}

#endif
