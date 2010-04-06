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

#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include "../../includes/exports.h"
#include "Plugin.h"
#include "../../includes/SClassID.h"
#include "../../includes/globals.h"

class ResourceDesc;
class DataStream;

class GEM_EXPORT ResourceMgr : public Plugin {
public:
	ResourceMgr(void);
	virtual ~ResourceMgr(void);
	virtual bool Open(const char *filename, const char *description) = 0;
	virtual bool HasResource(const char* resname, SClass_ID type) = 0;
	virtual bool HasResource(const char* resname, const ResourceDesc &type) = 0;
	virtual DataStream* GetResource(const char* resname, SClass_ID type) = 0;
	virtual DataStream* GetResource(const char* resname, const ResourceDesc &type) = 0;
	const char *GetDescription() const { return description; }
protected:
	const char *description;
};

#endif
