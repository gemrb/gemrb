// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

constexpr int RM_REPLACE_SAME_SOURCE = 1;
constexpr int RM_USE_EMPTY_SOURCE = 2;
constexpr int RM_SILENT = 4;

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
	inline ResourceHolder<T> GetResourceHolder(StringView resname, bool silent = false, ieWord preferredType = 0) const
	{
		return std::static_pointer_cast<T>(GetResource(resname, &T::ID, silent, preferredType));
	}

private:
	/** Returns Resource object associated to given resource */
	ResourceHolder<Resource> GetResource(StringView resname, const TypeID* type, bool silent = false, ieWord preferredType = 0) const;

	std::vector<PluginHolder<ResourceSource>> searchPath;
};

}

#endif
