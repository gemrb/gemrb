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

#ifndef RESOURCEDESC_H
#define RESOURCEDESC_H

#include "SClassID.h"
#include "exports.h"
#include "ie_types.h"

namespace GemRB {

class DataStream;
class Resource;
class TypeID;

/**
 * Class that describes a plugin resource type.
 */
class GEM_EXPORT ResourceDesc {
public:
	typedef Resource* (*CreateFunc)(DataStream*);
private:
	const TypeID *type;
	const char *ext;
	ieWord keyType; // IE Specific
	CreateFunc create;
public:
	/**
	 * Create resource descriptor.
	 *
	 * @param[in] type Base class for resource.
	 * @param[in] create Function to create resource from a stream.
	 * @param[in] ext Extension used for resource files.
	 * @param[in] keyType \iespecific Type identifier used in key/biff files.
	 */
	ResourceDesc(const TypeID* type, CreateFunc create, const char *ext, ieWord keyType = 0);
	~ResourceDesc(void);
	const char* GetExt() const;
	const TypeID* GetType() const;
	ieWord GetKeyType() const;
	Resource* Create(DataStream*) const;
};

}

#endif
