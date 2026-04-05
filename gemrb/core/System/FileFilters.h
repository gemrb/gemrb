// SPDX-FileCopyrightText: 2015 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GemRB_FileFilters_h
#define GemRB_FileFilters_h

#include "VFS.h"

namespace GemRB {

struct ExtFilter : Predicate<path_t> {
	path_t extension;

	explicit ExtFilter(path_t ext)
		: extension(std::move(ext))
	{}

	bool operator()(const path_t& filename) const override
	{
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

	bool operator()(const path_t& fname) const override
	{
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
