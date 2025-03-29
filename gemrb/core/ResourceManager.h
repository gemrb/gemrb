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

#ifndef RESOURCEMANGER_H
#define RESOURCEMANGER_H

#include "SClassID.h"
#include "exports.h"

#include "Holder.h"
#include "Resource.h"

#include "System/VFS.h"

#include <memory>
#include <vector>

namespace GemRB {

#define RM_REPLACE_SAME_SOURCE 1

class ResourceSource;
class TypeID;

class GEM_EXPORT ResourceManager {
public:
	/**
	 * Add ResourceSource to search path
	 * @param[in] path Path to be used for source.
	 *                 Note: This is modified by ResolveFilePath.
	 * @param[in] description Description of the source.
	 * @param[in] type Plugin type used for source.
	 **/
	bool AddSource(const path_t& path, const std::string& description, PluginID type, int flags = 0);

	/** returns true if resource exists */
	bool Exists(const String& resRef, SClass_ID type, bool silent = false) const;
	bool Exists(StringView resRef, SClass_ID type, bool silent = false) const;
	bool Exists(StringView resRef, const TypeID* type, bool silent = false) const;
	/** Returns stream associated to given resource */
	DataStream* GetResourceStream(StringView resname, SClass_ID type, bool silent = false) const;

	template<class T>
	inline ResourceHolder<T> GetResourceHolder(StringView resname, bool silent = false, ieWord prefferedType = 0) const
	{
		return std::static_pointer_cast<T>(GetResource(resname, &T::ID, silent, prefferedType));
	}

private:
	/** Returns Resource object associated to given resource */
	ResourceHolder<Resource> GetResource(StringView resname, const TypeID* type, bool silent = false, ieWord prefferedType = 0) const;

	std::vector<PluginHolder<ResourceSource>> searchPath;
};

}

#endif
