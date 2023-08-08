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
#include "Logging/Logging.h"
#include "PluginMgr.h"
#include "Resource.h"
#include "ResourceDesc.h"

namespace GemRB {

path_t TypeExt(SClass_ID type)
{
	static const std::map<SClass_ID, path_t> extensions = {
		{ IE_2DA_CLASS_ID, "2da" },
		{ IE_ACM_CLASS_ID, "acm" },
		{ IE_ARE_CLASS_ID, "are" },
		{ IE_BAM_CLASS_ID, "bam" },
		{ IE_BCS_CLASS_ID, "bcs" },
		{ IE_BS_CLASS_ID, "bs" },
		{ IE_BIF_CLASS_ID, "bif" },
		{ IE_BMP_CLASS_ID, "bmp" },
		{ IE_PNG_CLASS_ID, "png" },
		{ IE_CHR_CLASS_ID, "chr" },
		{ IE_CHU_CLASS_ID, "chu" },
		{ IE_CRE_CLASS_ID, "cre" },
		{ IE_DLG_CLASS_ID, "dlg" },
		{ IE_EFF_CLASS_ID, "eff" },
		{ IE_GAM_CLASS_ID, "gam" },
		{ IE_IDS_CLASS_ID, "ids" },
		{ IE_INI_CLASS_ID, "ini" },
		{ IE_ITM_CLASS_ID, "itm" },
		{ IE_MOS_CLASS_ID, "mos" },
		{ IE_MUS_CLASS_ID, "mus" },
		{ IE_MVE_CLASS_ID, "mve" },
		{ IE_OGG_CLASS_ID, "ogg" },
		{ IE_PLT_CLASS_ID, "plt" },
		{ IE_PRO_CLASS_ID, "pro" },
		{ IE_PVRZ_CLASS_ID, "pvrz" },
		{ IE_SAV_CLASS_ID, "sav" },
		{ IE_SPL_CLASS_ID, "spl" },
		{ IE_SRC_CLASS_ID, "src" },
		{ IE_STO_CLASS_ID, "sto" },
		{ IE_TIS_CLASS_ID, "tis" },
		{ IE_TLK_CLASS_ID, "tlk" },
		{ IE_TOH_CLASS_ID, "toh" },
		{ IE_TOT_CLASS_ID, "tot" },
		{ IE_VAR_CLASS_ID, "var" },
		{ IE_VEF_CLASS_ID, "vef" },
		{ IE_VVC_CLASS_ID, "vvc" },
		{ IE_WAV_CLASS_ID, "wav" },
		{ IE_WED_CLASS_ID, "wed" },
		{ IE_WFX_CLASS_ID, "wfx" },
		{ IE_WMP_CLASS_ID, "wmp" },
		{ IE_BIO_CLASS_ID, core->HasFeature(GFFlags::BIOGRAPHY_RES) ? "res" : "bio" }
	};

	const auto extIt = extensions.find(type);
	if (extIt == extensions.end()) {
		Log(ERROR, "Interface", "No extension associated to class ID: {}", type);
		return "";
	}
	return extIt->second;
}

bool ResourceManager::AddSource(const path_t& path, const std::string& description, PluginID type, int flags)
{
	PluginHolder<ResourceSource> source = MakePluginHolder<ResourceSource>(type);
	if (!source->Open(path, description)) {
		Log(WARNING, "ResourceManager", "Invalid path given: {} ({})", path, description);
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

static void PrintPossibleFiles(std::string& buffer, StringView ResRef, const TypeID *type)
{
	const std::vector<ResourceDesc>& types = PluginMgr::Get()->GetResourceDesc(type);
	for (const auto& type2 : types) {
		AppendFormat(buffer, "{}.{} ", ResRef, type2.GetExt());
	}
}

bool ResourceManager::Exists(const String& resource, SClass_ID type, bool silent) const {
	auto mbString = MBStringFromString(resource);
	auto mbResource = StringView{mbString};

	return Exists(mbResource, type, silent);
}

bool ResourceManager::Exists(StringView ResRef, SClass_ID type, bool silent) const
{
	if (ResRef.empty())
		return false;
	// TODO: check various caches
	for (const auto& path : searchPath) {
		if (path->HasResource(ResRef, type)) {
			return true;
		}
	}
	if (!silent) {
		Log(WARNING, "ResourceManager", "'{}.{}' not found...",
			ResRef, TypeExt(type));
	}
	return false;
}

bool ResourceManager::Exists(StringView ResRef, const TypeID *type, bool silent) const
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
		Log(WARNING, "ResourceManager", "{}", buffer);
	}
	return false;
}

DataStream* ResourceManager::GetResourceStream(StringView ResRef, SClass_ID type, bool silent) const
{
	if (ResRef.empty())
		return nullptr;
	for (const auto& path : searchPath) {
		DataStream *ds = path->GetResource(ResRef, type);
		if (ds) {
			if (!silent) {
				Log(MESSAGE, "ResourceManager", "Found '{}.{}' in '{}'.", ResRef, TypeExt(type), path->GetDescription());
			}
			return ds;
		}
	}
	if (!silent) {
		Log(ERROR, "ResourceManager", "Couldn't find '{}.{}'.", ResRef, TypeExt(type));
	}
	return NULL;
}

ResourceHolder<Resource> ResourceManager::GetResource(StringView ResRef, const TypeID *type, bool silent, bool useCorrupt) const
{
	if (ResRef.empty())
		return nullptr;
	if (!silent) {
		Log(MESSAGE, "ResourceManager", "Searching for '{}'...", ResRef);
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
				auto res = type2.Create(str);
				if (res) {
					if (!silent) {
						Log(MESSAGE, "ResourceManager", "Found '{}.{}' in '{}'.",
							ResRef, type2.GetExt(), path->GetDescription());
					}
					return res;
				}
			}
		}
	}
	if (!silent) {
		std::string buffer = fmt::format("Couldn't find '{}'... Tried ", ResRef);
		PrintPossibleFiles(buffer, ResRef, type);
		Log(WARNING, "ResourceManager", "{}", buffer);
	}
	return NULL;
}

}
