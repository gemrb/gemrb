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

#include "System/Logger/Apple.h"

#include "System/Logging.h"

/*
TODO: currently this is a clone of Stdio logger without color.
 I want to re write this to use Apple logging faculties so that
 Console.app can be used to see errors easily.
 
 When the new Logging API is in place we can have fatal errors use GUI alerts.
*/

AppleLogger::AppleLogger()
	: StdioLogger(false)
{}

AppleLogger::~AppleLogger()
{}

void AppleLogger::textcolor(log_color /*c*/)
{}

Logger* createAppleLogger()
{
	return new AppleLogger();
}
