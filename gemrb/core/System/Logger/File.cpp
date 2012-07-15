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

#include "System/Logger/File.h"

#include "System/DataStream.h"
#include "System/FileStream.h"
#include "System/Logging.h"

#ifndef STATIC_LINK
# define STATIC_LINK
#endif
#include "plugindef.h"
#include "Interface.h"

#include <cstdio>

namespace GemRB {

FileLogger::FileLogger(DataStream* log_file)
	: StdioLogger(false), log_file(log_file)
{}

FileLogger::~FileLogger()
{
	delete log_file;
}

void FileLogger::print(const char *message)
{
	log_file->Write(message, strlen(message));
}

Logger* createFileLogger(DataStream* log_file)
{
	return new FileLogger(log_file);
}

static void addLogger()
{
	char log_path[_MAX_PATH];
	FileStream* log_file = new FileStream();
	PathJoin(log_path, core->GamePath, "GemRB.log", NULL);
	if (log_file->Create(log_path)) {
		AddLogger(createFileLogger(log_file));
	} else {
		PathJoin(log_path, core->CachePath, "GemRB.log", NULL);
		if (log_file->Create(log_path)) {
			AddLogger(createFileLogger(log_file));
		} else if (log_file->Create("/tmp/GemRB.log")) {
			AddLogger(createFileLogger(log_file));
		} else {
			Log (WARNING, "Logger", "Could not create a log file, skipping!");
		}
	}
}

}

GEMRB_PLUGIN(unused, "tmp/file logger")
PLUGIN_INITIALIZER(addLogger)
END_PLUGIN()
