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
 * $Id$
 *
 */

#ifndef RESOURCEDESC_H
#define RESOURCEDESC_H

#include "../../includes/SClassID.h"
#include "../../includes/ie_types.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class Resource;
class DataStream;
class TypeID;

class GEM_EXPORT ResourceDesc {
public:
	typedef Resource* (*CreateFunc)(DataStream*);
private:
	const TypeID *type;
	const char *ext;
	ieWord keyType; // IE Specific
public:
	ResourceDesc(const TypeID* type, CreateFunc create, const char *ext, ieWord keyType = 0);
	~ResourceDesc(void);
	const char* GetExt();
	const TypeID* GetType();
	ieWord GetKeyType();
	const CreateFunc Create;
};

#endif
