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
#include "Resource.h"
#include "ResourceDesc.h"
#include "ResourceSource.h"
#include "Interface.h"
#include "PluginMgr.h"

ResourceManager::ResourceManager()
{
}


ResourceManager::~ResourceManager()
{
}

bool ResourceManager::AddSource(const char *path, const char *description, PluginID type)
{
	PluginHolder<ResourceSource> source(type);
	if (!source->Open(path, description)) {
		return false;
	}
	searchPath.push_back(source);
	return true;
}

static void PrintPossibleFiles(const char* ResRef, const TypeID *type)
{
	const std::vector<ResourceDesc>& types = PluginMgr::Get()->GetResourceDesc(type);
	for (size_t j = 0; j < types.size(); j++) {
		printf("%s%s ", ResRef, types[j].GetExt());
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
		printMessage( "ResourceManager", "Searching for ", WHITE );
		printf( "%s%s...", ResRef, core->TypeExt( type ) );
		printStatus( "NOT FOUND", YELLOW );
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
		printMessage( "ResourceManager", "Searching for ", WHITE );
		printf( "%s... ", ResRef );
		printf("Tried ");
		PrintPossibleFiles(ResRef,type);
		printStatus( "NOT FOUND", YELLOW );
	}
	return false;
}

DataStream* ResourceManager::GetResource(const char* ResRef, SClass_ID type, bool silent) const
{
	if (ResRef[0] == '\0')
		return false;
	if (!silent) {
		printMessage( "ResourceManager", "Searching for ", WHITE );
		printf( "%s%s...", ResRef, core->TypeExt( type ) );
	}
	for (size_t i = 0; i < searchPath.size(); i++) {
		DataStream *ds = searchPath[i]->GetResource(ResRef, type);
		if (ds) {
			if (!silent) {
				printStatus( searchPath[i]->GetDescription(), GREEN );
			}
			return ds;
		}
	}
	if (!silent) {
		printStatus( "ERROR", LIGHT_RED );
	}
	return NULL;
}

Resource* ResourceManager::GetResource(const char* ResRef, const TypeID *type, bool silent) const
{
	if (ResRef[0] == '\0')
		return false;
	if (!silent) {
		printMessage( "ResourceManager", "Searching for ", WHITE );
		printf( "%s... ", ResRef );
	}
	const std::vector<ResourceDesc> &types = PluginMgr::Get()->GetResourceDesc(type);
	for (size_t j = 0; j < types.size(); j++) {
		for (size_t i = 0; i < searchPath.size(); i++) {
			DataStream *str = searchPath[i]->GetResource(ResRef, types[j]);
			if (str) {
				Resource *res = types[j].Create(str);
				if (res) {
					if (!silent) {
						printf( "%s%s...", ResRef, types[j].GetExt() );
						printStatus( searchPath[i]->GetDescription(), GREEN );
					}
					return res;
				}
			}
		}
	}
	if (!silent) {
		printf("Tried ");
		PrintPossibleFiles(ResRef,type);
		printStatus( "ERROR", LIGHT_RED );
	}
	return NULL;
}
