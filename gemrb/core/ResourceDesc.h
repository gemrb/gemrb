// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef RESOURCEDESC_H
#define RESOURCEDESC_H

#include "ie_types.h"

#include "Resource.h"

namespace GemRB {

class TypeID;

/**
 * Class that describes a plugin resource type.
 */
class GEM_EXPORT ResourceDesc {
public:
	using CreateFunc = ResourceHolder<Resource> (*)(DataStream*);

private:
	const TypeID* type;
	path_t ext;
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
	ResourceDesc(const TypeID* type, CreateFunc create, const path_t& ext, ieWord keyType = 0);
	const path_t& GetExt() const;
	const TypeID* GetType() const;
	ieWord GetKeyType() const;
	ResourceHolder<Resource> Create(DataStream*) const;
};

}

#endif
