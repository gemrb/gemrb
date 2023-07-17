/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef RESOURCESOURCE_H
#define RESOURCESOURCE_H

#include "SClassID.h"
#include "exports.h"

#include "Plugin.h"

#include <string>

namespace GemRB {

class DataStream;
class ResourceDesc;

class GEM_EXPORT ResourceSource : public Plugin {
public:
	virtual bool Open(const path_t& filename, std::string description) = 0;
	virtual bool HasResource(StringView resname, SClass_ID type) = 0;
	virtual bool HasResource(StringView resname, const ResourceDesc &type) = 0;
	virtual DataStream* GetResource(StringView resname, SClass_ID type) = 0;
	virtual DataStream* GetResource(StringView resname, const ResourceDesc &type) = 0;
	const std::string& GetDescription() const { return description; }
protected:
	std::string description;
};

}

#endif
