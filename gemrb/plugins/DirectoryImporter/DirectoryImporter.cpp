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
 */

#include "DirectoryImporter.h"

#include "globals.h"
#include "win32def.h"

#include "Interface.h"
#include "ResourceDesc.h"
#include "System/FileStream.h"

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

static bool FindIn(const char *Path, const char *ResRef, const char *Type)
{
	char p[_MAX_PATH], f[_MAX_PATH] = {0};
	strcpy(f, ResRef);
	strcat(f, Type);
	strlwr(f);

	return PathJoin(p, Path, f, NULL);
}

static FileStream *SearchIn(const char * Path,const char * ResRef, const char *Type)
{
	char p[_MAX_PATH], f[_MAX_PATH] = {0};
	strcpy(f, ResRef);
	strcat(f, Type);
	strlwr(f);

	if (!PathJoin(p, Path, f, NULL))
		return NULL;

	FileStream * fs = new FileStream();
	if(!fs) return NULL;
	if(!fs->Open(p, true)) {
		delete fs;
		return NULL;
	}
	return fs;
}

bool DirectoryImporter::HasResource(const char* resname, SClass_ID type)
{
	return FindIn( path, resname, core->TypeExt(type) );
}

bool DirectoryImporter::HasResource(const char* resname, const ResourceDesc &type)
{
	return FindIn( path, resname, type.GetExt() );
}

DataStream* DirectoryImporter::GetResource(const char* resname, SClass_ID type)
{
	return SearchIn( path, resname, core->TypeExt(type) );
}

DataStream* DirectoryImporter::GetResource(const char* resname, const ResourceDesc &type)
{
	return SearchIn( path, resname, type.GetExt() );
}

#include "plugindef.h"

GEMRB_PLUGIN(0xAB4534, "Directory Importer")
PLUGIN_CLASS(PLUGIN_RESOURCE_DIRECTORY, DirectoryImporter)
END_PLUGIN()
