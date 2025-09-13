/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "XboxLogger.h"

#include <iostream>
#include <fstream>

#ifdef XBOX
#include <hal/debug.h>
#include <xboxkrnl/xboxkrnl.h>
#endif

namespace GemRB {

void XboxLogger::WriteLogMessage(const LogMessage& msg)
{
#ifdef XBOX
	// Output to Xbox debug console (visible with debugging tools)
	std::string logLine = fmt::format("[{}] {}: {}\n", 
		msg.owner, LogLevelName(msg.level), msg.message);
	
	// Write to debug output for NXDK debugging
	debugPrint(logLine.c_str());
	
	// Also try to write to a log file on Xbox filesystem
	static bool fileInit = false;
	static std::ofstream logFile;
	
	if (!fileInit) {
		logFile.open("E:\\GemRB\\gemrb.log", std::ios::app);
		fileInit = true;
	}
	
	if (logFile.is_open()) {
		logFile << logLine;
		logFile.flush();
	}
#else
	// Fallback for non-Xbox builds
	std::cerr << fmt::format("[{}] {}: {}\n", 
		msg.owner, LogLevelName(msg.level), msg.message);
#endif
}

LogWriterPtr createXboxLogger()
{
	return std::make_unique<XboxLogger>();
}

}