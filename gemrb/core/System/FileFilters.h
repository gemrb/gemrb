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

struct ExtFilter : DirectoryIterator::FileFilterPredicate {
	ResRef extension;
	explicit ExtFilter(const ResRef& ext) {
		extension = ext;
	}

	bool operator()(ResRef filename) const override {
		const char* extpos = strrchr(filename.c_str(), '.');
		if (extpos) {
			return ResRef(++extpos) == extension;
		}
		return false;
	}
};

struct EndsWithFilter : DirectoryIterator::FileFilterPredicate {
	ResRef endMatch;
	size_t len;

	explicit EndsWithFilter(const ResRef& endString) {
		endMatch = endString;
		len = endMatch.length();
	}

	bool operator()(ResRef filename) const override {
		// this filter ignores file extension
		const char* fname = filename.c_str();
		const char* rpos = strrchr(fname, '.');
		if (rpos) {
			// has an extension
			--rpos;
		} else {
			rpos = fname + strlen(fname)-1;
		}

		const char* fpos = rpos - len + 1;
		if (fpos < fname) {
			// impossible to be equal. this length is out of bounds
			return false;
		}

		const char* cmp = endMatch.c_str();
		while (fpos <= rpos) {
			// our fileters are case insensitive
			if (tolower(*cmp++) != tolower(*fpos++)) {
				return false;
			}
		}
		return true;
	}
};

}

#endif
