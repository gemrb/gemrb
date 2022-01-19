/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003-2005 The GemRB Project
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*
*/

#include "ResourceManager.h"

#include "Interface.h"
#include "PluginMgr.h"
#include "Resource.h"
#include "ResourceDesc.h"

namespace GemRB {

bool ResourceManager::AddSource(const char *path, const char *description, PluginID type, int flags)
{
	PluginHolder<ResourceSource> source = MakePluginHolder<ResourceSource>(type);
	if (!source->Open(path, description)) {
		Log(WARNING, "ResourceManager", "Invalid path given: %s (%s)", path, description);
		return false;
	}

	if (flags & RM_REPLACE_SAME_SOURCE) {
		for (auto& path2 : searchPath) {
			if (description == path2->GetDescription()) {
				path2 = source;
				break;
			}
		}
	} else {
		searchPath.push_back(source);
	}
	return true;
}

static void PrintPossibleFiles(std::string& buffer, const char* ResRef, const TypeID *type)
{
	const std::vector<ResourceDesc>& types = PluginMgr::Get()->GetResourceDesc(type);
	for (const auto& type2 : types) {
		AppendFormat(buffer, "{}.{} ", ResRef, type2.GetExt());
	}
}

bool ResourceManager::Exists(const char *ResRef, SClass_ID type, bool silent) const 
{
	if (!ResRef || ResRef[0] == '\0')
		return false;
	// TODO: check various caches
	for (const auto& path : searchPath) {
		if (path->HasResource(ResRef, type)) {
			return true;
		}
	}
	if (!silent) {
		Log(WARNING, "ResourceManager", "'%s.%s' not found...",
			ResRef, core->TypeExt(type));
	}
	return false;
}

bool ResourceManager::Exists(const char *ResRef, const TypeID *type, bool silent) const
{
	if (ResRef[0] == '\0')
		return false;
	// TODO: check various caches
	const std::vector<ResourceDesc> &types = PluginMgr::Get()->GetResourceDesc(type);
	for (const auto& type2 : types) {
		for (const auto& path : searchPath) {
			if (path->HasResource(ResRef, type2)) {
				return true;
			}
		}
	}
	if (!silent) {
		std::string buffer = fmt::format("Couldn't find '{}'... Tried ", ResRef);
		PrintPossibleFiles(buffer, ResRef,type);
		Log(WARNING, "ResourceManager", buffer);
	}
	return false;
}

bool ResourceManager::Exists(const ResRef &resRef, SClass_ID type, bool silent) const
{
	return Exists(resRef.CString(), type, silent);
}

bool ResourceManager::Exists(const ResRef &resRef, const TypeID *type, bool silent) const
{
	return Exists(resRef.CString(), type, silent);
}

DataStream* ResourceManager::GetResource(const ResRef &resname, SClass_ID type, bool silent) const
{
	return GetResource(resname.CString(), type, silent);
}

Resource* ResourceManager::GetResource(const ResRef &resname, const TypeID *type, bool silent, bool useCorrupt) const
{
	return GetResource(resname.CString(), type, silent, useCorrupt);
}

DataStream* ResourceManager::GetResource(const char* ResRef, SClass_ID type, bool silent) const
{
	if (!ResRef || ResRef[0] == '\0')
		return NULL;
	for (const auto& path : searchPath) {
		DataStream *ds = path->GetResource(ResRef, type);
		if (ds) {
			if (!silent) {
				Log(MESSAGE, "ResourceManager", "Found '%s.%s' in '%s'.",
					ResRef, core->TypeExt(type), path->GetDescription().c_str());
			}
			return ds;
		}
	}
	if (!silent) {
		Log(ERROR, "ResourceManager", "Couldn't find '%s.%s'.",
			ResRef, core->TypeExt(type));
	}
	return NULL;
}

Resource* ResourceManager::GetResource(const char* ResRef, const TypeID *type, bool silent, bool useCorrupt) const
{
	if (!ResRef || ResRef[0] == '\0')
		return NULL;
	if (!silent) {
		Log(MESSAGE, "ResourceManager", "Searching for '%s'...", ResRef);
	}
	const std::vector<ResourceDesc> &types = PluginMgr::Get()->GetResourceDesc(type);
	for (const auto& type2 : types) {
		for (const auto& path : searchPath) {
			DataStream *str = path->GetResource(ResRef, type2);
			if (!str && useCorrupt && core->UseCorruptedHack) {
				// don't look at other paths if requested
				core->UseCorruptedHack = false;
				return NULL;
			}
			core->UseCorruptedHack = false;
			if (str) {
				Resource *res = type2.Create(str);
				if (res) {
					if (!silent) {
						Log(MESSAGE, "ResourceManager", "Found '%s.%s' in '%s'.",
							ResRef, type2.GetExt(), path->GetDescription().c_str());
					}
					return res;
				}
			}
		}
	}
	if (!silent) {
		std::string buffer = fmt::format("Couldn't find '{}'... Tried ", ResRef);
		PrintPossibleFiles(buffer, ResRef, type);
		Log(WARNING, "ResourceManager", buffer);
	}
	return NULL;
}

}
