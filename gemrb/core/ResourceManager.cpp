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
#include "ResourceSource.h"
#include "System/StringBuffer.h"

namespace GemRB {

ResourceManager::ResourceManager()
{
}


ResourceManager::~ResourceManager()
{
}

bool ResourceManager::AddSource(const char *path, const char *description, PluginID type, int flags)
{
	PluginHolder<ResourceSource> source(type);
	if (!source->Open(path, description)) {
		Log(WARNING, "ResourceManager", "Invalid path given: %s (%s)", path, description);
		return false;
	}

	if (flags & RM_REPLACE_SAME_SOURCE) {
		for (size_t i = 0; i < searchPath.size(); i++) {
			if (!stricmp(description, searchPath[i]->GetDescription())) {
				searchPath[i] = source;
				break;
			}
		}
	} else {
		searchPath.push_back(source);
	}
	return true;
}

static void PrintPossibleFiles(StringBuffer& buffer, const char* ResRef, const TypeID *type)
{
	const std::vector<ResourceDesc>& types = PluginMgr::Get()->GetResourceDesc(type);
	for (size_t j = 0; j < types.size(); j++) {
		buffer.appendFormatted("%s.%s ", ResRef, types[j].GetExt());
	}
}

bool ResourceManager::Exists(const char *ResRef, SClass_ID type, bool silent) const 
{
	if (ResRef[0] == '\0')
		return false;
	// TODO: check various caches
	for (size_t i = 0; i < searchPath.size(); i++) {
		if (searchPath[i]->HasResource( ResRef, type )) {
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
	for (size_t j = 0; j < types.size(); j++) {
		for (size_t i = 0; i < searchPath.size(); i++) {
			if (searchPath[i]->HasResource(ResRef, types[j])) {
				return true;
			}
		}
	}
	if (!silent) {
		StringBuffer buffer;
		buffer.appendFormatted("Couldn't find '%s'... ", ResRef);
		buffer.append("Tried ");
		PrintPossibleFiles(buffer, ResRef,type);
		Log(WARNING, "ResourceManager", buffer);
	}
	return false;
}

DataStream* ResourceManager::GetResource(const char* ResRef, SClass_ID type, bool silent) const
{
	if (ResRef[0] == '\0')
		return NULL;
	for (size_t i = 0; i < searchPath.size(); i++) {
		DataStream *ds = searchPath[i]->GetResource(ResRef, type);
		if (ds) {
			if (!silent) {
				Log(MESSAGE, "ResourceManager", "Found '%s.%s' in '%s'.",
					ResRef, core->TypeExt(type), searchPath[i]->GetDescription());
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

Resource* ResourceManager::GetResource(const char* ResRef, const TypeID *type, bool silent) const
{
	if (ResRef[0] == '\0')
		return NULL;
	if (!silent) {
		Log(MESSAGE, "ResourceManager", "Searching for '%s'...", ResRef);
	}
	const std::vector<ResourceDesc> &types = PluginMgr::Get()->GetResourceDesc(type);
	for (size_t j = 0; j < types.size(); j++) {
		for (size_t i = 0; i < searchPath.size(); i++) {
			DataStream *str = searchPath[i]->GetResource(ResRef, types[j]);
			if (str) {
				Resource *res = types[j].Create(str);
				if (res) {
					if (!silent) {
						Log(MESSAGE, "ResourceManager", "Found '%s.%s' in '%s'.",
							ResRef, types[j].GetExt(), searchPath[i]->GetDescription());
					}
					return res;
				}
			}
		}
	}
	if (!silent) {
		StringBuffer buffer;
		buffer.appendFormatted("Couldn't find '%s'... ", ResRef);
		buffer.append("Tried ");
		PrintPossibleFiles(buffer, ResRef,type);
		Log(WARNING, "ResourceManager", buffer);
	}
	return NULL;
}

}
