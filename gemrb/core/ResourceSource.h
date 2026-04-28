// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef RESOURCESOURCE_H
#define RESOURCESOURCE_H

#include "SClassID.h"
#include "exports.h"

#include "Plugin.h"

#include <string>

namespace GemRB {

class DataStream;
class ResourceDesc;

GEM_EXPORT path_t TypeExt(SClass_ID type);

class GEM_EXPORT ResourceSource : public Plugin {
public:
	virtual bool Open(const path_t& filename, std::string description) = 0;
	virtual void MergeBifsFromPath(const path_t&) {}
	virtual bool HasResource(StringView resname, SClass_ID type) = 0;
	virtual bool HasResource(StringView resname, const ResourceDesc& type) = 0;
	virtual DataStream* GetResource(StringView resname, SClass_ID type) = 0;
	virtual DataStream* GetResource(StringView resname, const ResourceDesc& type) = 0;
	const std::string& GetDescription() const { return description; }

protected:
	std::string description;
};

}

#endif
