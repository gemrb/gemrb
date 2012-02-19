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
#include "System/Logging.h"

#include <cstdio>

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
