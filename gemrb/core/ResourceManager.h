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

#ifndef RESOURCEMANGER_H
#define RESOURCEMANGER_H

#include <vector>
#include "exports.h"
#include "SClassID.h"

class DataStream;
class Resource;
class ResourceSource;
class TypeID;

class GEM_EXPORT ResourceManager {
public:
	ResourceManager();
	~ResourceManager();

	/**
	 * Add ResourceSource to search path
	 * @param[in] path Path to be used for source.
	 *                 Note: This is modified by ResolveFilePath.
	 * @param[in] description Description of the source.
	 * @param[in] type Plugin type used for source.
	 **/
	bool AddSource(char *path, const char *description, PluginID type);

	/** returns true if resource exists */
	bool Exists(const char *ResRef, SClass_ID type, bool silent=false);
	/** returns true if resource exists */
	bool Exists(const char *ResRef, const TypeID *type, bool silent=false);

	/** Returns stream associated to given resource */
	DataStream* GetResource(const char* resname, SClass_ID type, bool silent = false) const;
	/** Returns Resource object associated to given resource */
	Resource* GetResource(const char* resname, const TypeID *type, bool silent = false) const;

private:
	std::vector<ResourceSource*> searchPath;
};

#endif
