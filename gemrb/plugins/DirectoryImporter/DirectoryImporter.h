// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DIRIMP_H
#define DIRIMP_H

#include "ResourceSource.h"

#include "Strings/CString.h"
#include "System/VFS.h"

#include <set>

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
	bool HasResource(StringView resname, const ResourceDesc& type) override;
	/** returns resource */
	DataStream* GetResource(StringView resname, SClass_ID type) override;
	DataStream* GetResource(StringView resname, const ResourceDesc& type) override;
};

class CachedDirectoryImporter : public DirectoryImporter {
protected:
	// the case is case insensitive, but we will only store valid names
	std::set<path_t, CstrLessCI> cache;

public:
	CachedDirectoryImporter() noexcept = default;
	bool Open(const path_t& dir, std::string desc) override;
	void Refresh();
	/** predicts the availability of a resource */
	bool HasResource(StringView resname, SClass_ID type) override;
	bool HasResource(StringView resname, const ResourceDesc& type) override;
	/** returns resource */
	DataStream* GetResource(StringView resname, SClass_ID type) override;
	DataStream* GetResource(StringView resname, const ResourceDesc& type) override;
};


}

#endif
