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
 * @file StringBuffer.h
 * Code for formating debug dumps
 * @author The GemRB Project
 */

#ifndef STRINGBUFFER_H
#define STRINGBUFFER_H

#include "exports.h"

#include <string>

namespace GemRB {

#if defined(__GNUC__)
# define PRINTF_FORMAT(x, y) \
    __attribute__ ((format(printf, x, y)))
#else
# define PRINTF_FORMAT(x, y)
#endif

class GEM_EXPORT StringBuffer {
public:
	StringBuffer();
	virtual ~StringBuffer();

	/// Append formatted string to buffer
	void appendFormatted(const char* message, ...)
		PRINTF_FORMAT(2, 3);
	/// Append a string to buffer
	void append(const char* message);
	/// Append a std::string to buffer
	void append(std::string const& message);

	/// Return buffer
	std::string const& get() const;

private:
	// TODO: Should we manage the string ourselves, and use the small string optimization?
	std::string buffer;
};

#undef PRINTF_FORMAT

}

#endif
