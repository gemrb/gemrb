/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "DirectoryImporter.h"
#include "../../includes/globals.h"
#include "../Core/FileStream.h"
#include "../Core/Interface.h"
#include "../Core/ResourceDesc.h"
#ifndef WIN32
#include <unistd.h>
#endif

DirectoryImporter::DirectoryImporter(void)
{
}

DirectoryImporter::~DirectoryImporter(void)
{
}

bool DirectoryImporter::Open(const char *dir, const char *desc)
{
	description = desc;
	strcpy(path, dir);
	return true;
}

static bool exists(char *file)
{
	FILE *f = fopen( file, "rb" );
	if (f) {
		fclose(f);
		return true;
	}
	return false;
}

static bool FindIn(const char *Path, const char *ResRef, const char *Type)
{
	char p[_MAX_PATH], f[_MAX_PATH] = {0};
	strncpy(f, ResRef, _MAX_PATH);
	strcat(f, Type);
	strlwr(f);
	PathJoin( p, Path, f, NULL );
	ResolveFilePath(p);
	return exists(p);
}

static FileStream *SearchIn(const char * Path,const char * ResRef, const char *Type)
{
	char p[_MAX_PATH], f[_MAX_PATH] = {0};
	strncpy(f, ResRef, _MAX_PATH);
	strcat(f, Type);
	strlwr(f);
	PathJoin( p, Path, f, NULL );
	ResolveFilePath(p);
	if (exists(p)) {
		FileStream * fs = new FileStream();
		if(!fs) return NULL;
		fs->Open(p, true);
		return fs;
	}
	return NULL;
}

bool DirectoryImporter::HasResource(const char* resname, SClass_ID type, bool)
{
	if (FindIn( path, resname, core->TypeExt(type) ))
		return true;
	return false;
}

bool DirectoryImporter::HasResource(const char* resname, std::vector<ResourceDesc> types, bool)
{
	if (types.size() == 0)
		return false;
	for (size_t j = 0; j < types.size(); j++) {
		if (FindIn( path, resname, types[j].GetExt() )) {
			printf("%s%s ", resname, types[j].GetExt());
			return true;
		}
	}
	return false;
}

DataStream* DirectoryImporter::GetResource(const char* resname, SClass_ID type, bool silent)
{
	if (!strcmp(resname, "")) return NULL;

	DataStream *fs = SearchIn( path, resname, core->TypeExt(type));
	if (fs && !silent) {
		printBracket(description, LIGHT_GREEN);
		printf("\n");
	}
	return fs;
}

Resource* DirectoryImporter::GetResource(const char* resname, const std::vector<ResourceDesc> &types, bool silent)
{
	if (!strcmp(resname, "")) return NULL;

	//Search it in the GemRB override Directory

	for (size_t j = 0; j < types.size(); j++) {
		FileStream *fs = SearchIn( path, resname, types[j].GetExt());
		if (fs) {
			Resource * res = types[j].Create(fs);
			if (res) {
				if (!silent) {
					printf("%s%s ", resname, types[j].GetExt());
					printBracket(description, LIGHT_GREEN);
					printf("\n");
				}
				return res;
			}
		}
	}
	return NULL;
}

#include "../../includes/plugindef.h"

GEMRB_PLUGIN(0xAB4534, "Directory Importer")
PLUGIN_CLASS(PLUGIN_RESOURCE_DIRECTORY, DirectoryImporter)
END_PLUGIN()
