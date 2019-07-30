/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 *
 */

/**
 * @file Resource.h
 * Declares Resource class, base class for all resources
 * @author The GemRB Project
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include "Plugin.h"
#include "System/String.h"

namespace GemRB {

/** Resource reference */
typedef char ieResRef[9];

class DataStream;

/* Hopefully we can slowly replace the char array version with this struct... */
struct ResRef {
private:
	char ref[9];
private:
	void Clear() {
		memset(ref, 0, sizeof(ref));
	}
public:
	ResRef() {
		Clear();
	}

	ResRef(const char* str) {
		operator=(str);
	};

	bool IsEmpty() {
		return (ref[0] == '\0');
	}
	const char* CString() const { return ref; }
	operator const char*() const {
		return (ref[0] == '\0') ? NULL : ref;
	}

	void operator=(const char* str) {
		if (str == NULL) {
			Clear();
		} else {
			strlcpy(ref, str, sizeof(ref));
		}
	}

	bool operator<(const ResRef& rhs) const {
		return strnicmp(ref, rhs.CString(), sizeof(ref)-1) < 0;
	};

	bool operator==(const ResRef& rhs) const {
		return strnicmp(ref, rhs.CString(), sizeof(ref)-1) == 0;
	};
};

/**
 * Base class for all GemRB resources
 */

class GEM_EXPORT Resource : public Plugin {
protected:
	DataStream* str;
public:
	Resource();
	virtual ~Resource();
	/**
	 * Reads the resource from the given stream.
	 *
	 * This should only be called once for a given resource object.
	 * @param[in] stream Non-NULL Stream containg the resource
	 * @retval true stream contains the given resource.
	 * @retval false stream does not contain valid resource.
	 */
	virtual bool Open(DataStream* stream) = 0;
};

}

#endif
