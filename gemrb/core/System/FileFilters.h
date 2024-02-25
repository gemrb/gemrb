/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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
 */

#ifndef GemRB_FileFilters_h
#define GemRB_FileFilters_h

#include "VFS.h"

namespace GemRB {

struct ExtFilter : Predicate<path_t> {
	path_t extension;

	explicit ExtFilter(path_t ext)
	: extension(std::move(ext))
	{}

	bool operator()(const path_t& filename) const override {
		size_t extpos = filename.find_last_of('.');
		if (extpos != path_t::npos) {
			return stricmp(&filename[extpos + 1], extension.data()) == 0;
		}
		return false;
	}
};

struct EndsWithFilter : Predicate<path_t> {
	path_t endMatch;

	explicit EndsWithFilter(path_t endString)
	: endMatch(std::move(endString))
	{}

	bool operator()(const path_t& fname) const override {
		if (fname.empty()) return false;
		// this filter ignores file extension
		size_t rpos = fname.find_last_of('.');
		if (rpos != path_t::npos) {
			// has an extension
			--rpos;
		} else {
			rpos = fname.length() - 1;
		}

		size_t fpos = rpos - endMatch.length() + 1;
		if (fpos >= fname.length()) {
			// impossible to be equal. this length is out of bounds
			return false;
		}

		return strnicmp(endMatch.data(), &fname[fpos], endMatch.length()) == 0;
	}
};

}

#endif
