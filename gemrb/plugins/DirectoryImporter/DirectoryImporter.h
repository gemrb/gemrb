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

#ifndef KEYIMP_H
#define KEYIMP_H

#include "ResourceSource.h"
#include "StringMap.h"

class Resource;
class ResourceDesc;

class DirectoryImporter : public ResourceSource {
protected:
	char path[_MAX_PATH];

public:
	DirectoryImporter(void);
	~DirectoryImporter(void);
	bool Open(const char *dir, const char *desc);
	/** predicts the availability of a resource */
	bool HasResource(const char* resname, SClass_ID type);
	bool HasResource(const char* resname, const ResourceDesc &type);
	/** returns resource */
	DataStream* GetResource(const char* resname, SClass_ID type);
	DataStream* GetResource(const char* resname, const ResourceDesc &type);
};

class CachedDirectoryImporter : public DirectoryImporter {
protected:
	StringMap cache;

public:
	CachedDirectoryImporter();
	~CachedDirectoryImporter();

	bool Open(const char *dir, const char *desc);
	void Refresh();
	/** predicts the availability of a resource */
	bool HasResource(const char* resname, SClass_ID type);
	bool HasResource(const char* resname, const ResourceDesc &type);
	/** returns resource */
	DataStream* GetResource(const char* resname, SClass_ID type);
	DataStream* GetResource(const char* resname, const ResourceDesc &type);
};


#endif
