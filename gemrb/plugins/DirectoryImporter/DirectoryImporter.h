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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef DIRIMP_H
#define DIRIMP_H

#include <set>

#include "ResourceSource.h"
#include "System/VFS.h"

namespace GemRB {

class ResourceDesc;

class DirectoryImporter : public ResourceSource {
protected:
	path_t path;

public:
	DirectoryImporter() noexcept = default;
	bool Open(const path_t& dir, std::string desc) override;
	/** predicts the availability of a resource */
	bool HasResource(StringView resname, SClass_ID type) override;
	bool HasResource(StringView resname, const ResourceDesc &type) override;
	/** returns resource */
	DataStream* GetResource(StringView resname, SClass_ID type) override;
	DataStream* GetResource(StringView resname, const ResourceDesc &type) override;
};

class CachedDirectoryImporter : public DirectoryImporter {
protected:
	// the case is case insensitive, but we will only store valid names
	std::set<path_t, CstrCmpCI<path_t>> cache;

public:
	CachedDirectoryImporter() noexcept = default;
	bool Open(const path_t& dir, std::string desc) override;
	void Refresh();
	/** predicts the availability of a resource */
	bool HasResource(StringView resname, SClass_ID type) override;
	bool HasResource(StringView resname, const ResourceDesc &type) override;
	/** returns resource */
	DataStream* GetResource(StringView resname, SClass_ID type) override;
	DataStream* GetResource(StringView resname, const ResourceDesc &type) override;
};


}

#endif
