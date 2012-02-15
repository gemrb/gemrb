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
{}

AppleLogger::~AppleLogger()
{}

void AppleLogger::vprint(const char *message, va_list ap)
{
	vprintf(message, ap);
}

void AppleLogger::textcolor(log_color /*c*/)
{}

void AppleLogger::printBracket(const char* status, log_color color)
{
	textcolor(WHITE);
	print("[");
	textcolor(color);
	print("%s", status);
	textcolor(WHITE);
	print("]");
}

void AppleLogger::printStatus(const char* status, log_color color)
{
	printBracket(status, color);
	print("\n");
}

void AppleLogger::vprintMessage(const char* owner, const char* message, log_color color, va_list ap)
{
	printBracket(owner, LIGHT_WHITE);
	print(": ");
	textcolor(color);

	vprint(message, ap);
}

Logger* createStdioLogger()
{
	return new AppleLogger();
}
