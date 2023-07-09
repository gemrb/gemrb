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

#include "NullSource.h"

#include "globals.h"

#include "ResourceDesc.h"
#include "Streams/FileStream.h"

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
