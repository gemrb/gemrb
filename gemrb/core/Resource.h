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

#include <cstdarg>

namespace GemRB {

/** Resource reference */
class DataStream;

struct ResRef {
private:
	char ref[9] = {'\0'};
public:
	ResRef() = default;
	
	ResRef(std::nullptr_t) = delete;

	ResRef(const char* str) {
		operator=(str);
	};

	ResRef(const ResRef& rhs) = default;
	
	void Reset() {
		memset(ref, 0, sizeof(ref));
	}
	
	// ResRef is case insensitive, but the originals weren't always
	// in some cases we need lower/upper case for save compatibility with originals
	// so we provide factories the create ResRef with the required case
	static ResRef MakeLowerCase(const char* str) {
		if (!str) return ResRef();

		char ref[9];
		strnlwrcpy(ref, str, sizeof(ref) - 1);
		return ref;
	}
	
	static ResRef MakeUpperCase(const char* str) {
		if (!str) return ResRef();

		char ref[9];
		strnuprcpy(ref, str, sizeof(ref) - 1);
		return ref;
	}

	void SNPrintF(const char* format, ...) {
		va_list args;
		va_start(args, format);
		vsnprintf(ref, sizeof(ref), format, args);
		va_end(args);
	}

	ResRef& operator=(const ResRef& rhs) = default;
	
	ResRef& operator=(const char* str) {
		if (str == NULL) {
			Reset();
		} else {
			strncpy(ref, str, sizeof(ref) - 1);
			ref[sizeof(ref)-1] = '\0';
		}
		
		return *this;
	}

	struct Hash
	{
		size_t operator() (const ResRef &) const;
	};
	friend struct Hash;

	bool IsEmpty() const {
		return (ref[0] == '\0');
	}

	bool IsStar() const {
		return (ref[0] == '*');
	}

	const char* CString() const { return ref; }

	operator const char*() const {
		return (ref[0] == '\0') ? NULL : ref;
	}

	// Case insensitive
	bool operator<(const ResRef& rhs) const {
		return strnicmp(ref, rhs.CString(), sizeof(ref)-1) < 0;
	};

	bool operator==(const ResRef& rhs) const {
		return strnicmp(ref, rhs.CString(), sizeof(ref)-1) == 0;
	};

	bool operator!=(const ResRef& rhs) const {
		return !operator==(rhs);
	};

	bool operator==(const char* str) const {
		return strnicmp(ref, str, sizeof(ref)-1) == 0;
	};

	bool operator!=(const char* str) const {
		return !operator==(str);
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
	~Resource() override;
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
