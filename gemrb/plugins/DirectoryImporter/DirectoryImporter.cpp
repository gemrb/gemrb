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

#include "Logging/Logging.h"
#include "ResourceDesc.h"
#include "Streams/FileStream.h"

using namespace GemRB;

bool DirectoryImporter::Open(const path_t& dir, std::string desc)
{
	path_t p = dir;
	ResolveCase(p);
	if (!DirExists(p))
		return false;

	description = std::move(desc);
	path.swap(p);
	return true;
}

static bool FindIn(const path_t& path, StringView resRef, const path_t& type)
{
	return FileExists(PathJoinExt(path, resRef, type));
}

static FileStream *SearchIn(const path_t& path, StringView resRef, const path_t& type)
{
	return FileStream::OpenFile(PathJoinExt(path, resRef, type));
}

bool DirectoryImporter::HasResource(StringView resname, SClass_ID type)
{
	return FindIn( path, resname, TypeExt(type) );
}

bool DirectoryImporter::HasResource(StringView resname, const ResourceDesc &type)
{
	return FindIn( path, resname, type.GetExt() );
}

DataStream* DirectoryImporter::GetResource(StringView resname, SClass_ID type)
{
	return SearchIn( path, resname, TypeExt(type) );
}

DataStream* DirectoryImporter::GetResource(StringView resname, const ResourceDesc &type)
{
	return SearchIn( path, resname, type.GetExt() );
}

bool CachedDirectoryImporter::Open(const path_t& dir, std::string desc)
{
	if (!DirectoryImporter::Open(dir, std::move(desc)))
		return false;

	Refresh();

	return true;
}

void CachedDirectoryImporter::Refresh()
{
	cache.clear();

	DirectoryIterator it(path);
	it.SetFlags(DirectoryIterator::Files, true);
	if (!it)
		return;

	do {
		const path_t name = it.GetName();
		auto emplaceResult = cache.emplace(name);
		if (!emplaceResult.second) {
			Log(ERROR, "CachedDirectoryImporter", "Duplicate '{}' files in '{}' directory", name, path);
		}
	} while (++it);
}

static path_t ConstructFilename(StringView resname, const path_t& ext)
{
	path_t buf(resname.c_str(), resname.length());
	buf.push_back('.');
	buf += ext;
	return buf;
}

bool CachedDirectoryImporter::HasResource(StringView resname, SClass_ID type)
{
	const path_t& filename = ConstructFilename(resname, TypeExt(type));
	return cache.find(filename) != cache.cend();
}

bool CachedDirectoryImporter::HasResource(StringView resname, const ResourceDesc &type)
{
	const path_t& filename = ConstructFilename(resname, type.GetExt());
	return cache.find(filename) != cache.cend();
}

DataStream* CachedDirectoryImporter::GetResource(StringView resname, SClass_ID type)
{
	const path_t& filename = ConstructFilename(resname, TypeExt(type));
	const auto lookup = cache.find(filename);

	if (lookup == cache.cend())
		return nullptr;

	path_t buf = path;
	PathAppend(buf, *lookup);
	return FileStream::OpenFile(buf);
}

DataStream* CachedDirectoryImporter::GetResource(StringView resname, const ResourceDesc &type)
{
	const path_t& filename = ConstructFilename(resname, type.GetExt());
	const auto lookup = cache.find(filename);

	if (lookup == cache.cend())
		return nullptr;

	path_t buf = path;
	PathAppend(buf, *lookup);
	return FileStream::OpenFile(buf);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xAB4534, "Directory Importer")
PLUGIN_CLASS(PLUGIN_RESOURCE_DIRECTORY, DirectoryImporter)
PLUGIN_CLASS(PLUGIN_RESOURCE_CACHEDDIRECTORY, CachedDirectoryImporter)
END_PLUGIN()
