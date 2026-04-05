// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NullSource.h"

#include <utility>

using namespace GemRB;

bool NullSource::Open(const path_t&, std::string desc)
{
	description = std::move(desc);
	return true;
}

bool NullSource::HasResource(StringView, SClass_ID)
{
	return false;
}

bool NullSource::HasResource(StringView, const ResourceDesc&)
{
	return false;
}

DataStream* NullSource::GetResource(StringView, SClass_ID)
{
	return NULL;
}

DataStream* NullSource::GetResource(StringView, const ResourceDesc&)
{
	return NULL;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xFA42E34A, "Null Resource Source")
PLUGIN_CLASS(PLUGIN_RESOURCE_NULL, NullSource)
END_PLUGIN()
