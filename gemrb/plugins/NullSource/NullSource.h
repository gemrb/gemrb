// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef NULLSOURCE_H
#define NULLSOURCE_H

#include "ResourceSource.h"

namespace GemRB {

class NullSource : public ResourceSource {
public:
	bool Open(const path_t& filename, std::string description) override;
	bool HasResource(StringView resname, SClass_ID type) override;
	bool HasResource(StringView resname, const ResourceDesc& type) override;
	DataStream* GetResource(StringView resname, SClass_ID type) override;
	DataStream* GetResource(StringView resname, const ResourceDesc& type) override;
};

}

#endif
