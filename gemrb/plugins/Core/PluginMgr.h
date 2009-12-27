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
 * $Id$
 *
 */

/**
 * @file PluginMgr.h
 * Declares PluginMgr, loader for GemRB plugins
 * @author The GemRB Project
 */

#ifndef PLUGINMGR_H
#define PLUGINMGR_H

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include <vector>
#include <list>
#include <cstring>
#include <map>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#ifdef WIN32
typedef HINSTANCE LibHandle;
#else
typedef void *LibHandle;
#endif

class ResourceDesc;
class TypeID;

/**
 * @class PluginMgr
 * Class for loading GemRB plugins from shared libraries or DLLs.
 * It goes over all appropriately named files in PluginPath directory
 * and tries to load them one after another.
 */

struct InterfaceElement {
	void *mgr;
	bool free;
};

class GEM_EXPORT PluginMgr {
public:
	PluginMgr(char* pluginpath);
	~PluginMgr(void);
private:
	std::vector< ClassDesc*> plugins;
	std::vector< LibHandle> libs;
	std::map< const TypeID*, std::vector<ResourceDesc*> > resources;
public:
	/** Return names of all *.so or *.dll files in the given directory */
	bool FindFiles( char* path, std::list< char* > &files);
	bool IsAvailable(SClass_ID plugintype) const;
	void* GetPlugin(SClass_ID plugintype) const;
	std::vector<InterfaceElement> *GetAllPlugin(SClass_ID plugintype) const;	
	void FreePlugin(void* ptr);
	size_t GetPluginCount() const { return plugins.size(); }
	void AddResourceDesc(ResourceDesc*);
	const std::vector<ResourceDesc*>& GetResourceDesc(const TypeID*);
};

#endif
