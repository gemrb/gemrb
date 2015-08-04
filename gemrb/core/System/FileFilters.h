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
	char extension[9];
	ExtFilter(const char* ext) {
		strncpy(extension, ext, sizeof(extension)-1);
		extension[sizeof(extension)-1] = '\0';
	}

	bool operator()(const char* fname) const {
		const char* extpos = strrchr(fname, '.');
		if (extpos) {
			return stricmp(++extpos, extension) == 0;
		}
		return false;
	}
};

struct EndsWithFilter : DirectoryIterator::FileFilterPredicate {
	char* endMatch;
	size_t len;

	EndsWithFilter(const char* endString) {
		endMatch = strdup(endString);
		len = strlen(endMatch);
	}

	~EndsWithFilter() {
		free(endMatch);
	}

	bool operator()(const char* fname) const {
		// this filter ignores file extension
		const char* rpos = strrchr(fname, '.');
		if (rpos) {
			// has an extension
			--rpos;
		} else {
			rpos = fname + strlen(fname)-1;
		}

		const char* fpos = rpos - len;
		if (fpos < fname) {
			// impossible to be equal. this length is out of bounds
			return false;
		}

		const char* cmp = endMatch;
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
