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

#include "System/StringBuffer.h"

#include "win32def.h"

#include <cstdio>
#include <cstdarg>

namespace GemRB {

StringBuffer::StringBuffer()
{}

StringBuffer::~StringBuffer()
{}

void StringBuffer::appendFormatted(const char* message, ...)
{
	va_list ap;

#if defined(_MSC_VER)
	// Don't try to be smart.
	// Assume this is long enough. If not, message will be truncated.
	// MSVC6 has old vsnprintf that doesn't give length
	const size_t len = 4095;
#else
	va_start(ap, message);
	const size_t len = vsnprintf(NULL, 0, message, ap);
	va_end(ap);
#endif

#if defined(__GNUC__)
	__extension__ // Variable-length arrays
#endif
	char buf[len+1];
	va_start(ap, message);
	vsnprintf(buf, len + 1, message, ap);
	va_end(ap);

	// TODO: If we manage the string ourselves, we can avoid this extra copy.
	buffer += buf;
}

void StringBuffer::append(const char* message)
{
	buffer += message;
}

void StringBuffer::append(std::string const& message)
{
	buffer += message;
}

std::string const& StringBuffer::get() const
{
	return buffer;
}

}
