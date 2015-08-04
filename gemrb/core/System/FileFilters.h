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
		memcpy(extension, ext, sizeof(extension));
	}

	bool operator()(const char* fname) const {
		const char* extpos = strrchr(fname, '.');
		if (extpos) {
			extpos++;
			return stricmp(extpos, extension) == 0;
		}
		return false;
	}
};

struct LastCharFilter : DirectoryIterator::FileFilterPredicate {
	char lastchar;
	LastCharFilter(char lastchar) {
		this->lastchar = tolower(lastchar);
	}

	bool operator()(const char* fname) const {
		const char* extpos = strrchr(fname, '.');
		if (extpos) {
			extpos--;
			return tolower(*extpos) == lastchar;
		}
		return false;
	}
};

}

#endif
