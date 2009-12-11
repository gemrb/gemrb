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
 *
 * $Id$
 *
 */

#ifndef KEYIMP_H
#define KEYIMP_H

#include <vector>
#include "../Core/ResourceMgr.h"
#include "Dictionary.h"

struct RESEntry {
	ieResRef ResRef;
	ieWord   Type;
	ieDword  ResLocator;
};

struct BIFEntry {
	char* name;
	ieWord BIFLocator;
};

struct SearchPath {
	char path[_MAX_PATH];
	const char *description;
};

class KeyImp : public ResourceMgr {
private:
	std::vector< BIFEntry> biffiles;
	std::vector< SearchPath> searchPath;
	Dictionary resources;
public:
	KeyImp(void);
	~KeyImp(void);
	bool LoadResFile(const char* resfile);
	/* predicts the availability of a resource */
	bool HasResource(const char* resname, SClass_ID type, bool silent=false);
	/* returns resource */
	DataStream* GetResource(const char* resname, SClass_ID type, bool silent=false);
	/* returns factory resource, currently works only with animations */
	void* GetFactoryResource(const char* resname, SClass_ID type,
		unsigned char mode = IE_NORMAL, bool silent=false);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
