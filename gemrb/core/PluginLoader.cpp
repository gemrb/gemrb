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
 */

#include "PluginLoader.h"

#include "InterfaceConfig.h"
#include "Logging/Logging.h"
#include "Platform.h"
#include "PluginMgr.h"
#include "System/FileFilters.h"

#include <cstdio>
#include <cstdlib>
#include <set>

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#ifdef HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST
# include <cassert>
#endif

namespace GemRB {

#ifdef WIN32
using LibHandle = HMODULE;
#else
using LibHandle = void*;
#endif

namespace GemRB {

class PluginMgr;

}

struct PluginDesc {
	LibHandle handle;
	PluginID ID;
	const char *Description;
	bool (*Register)(PluginMgr*);
};

using Version_t = const char* (*)(void);
using Description_t = const char* (*)(void);
using ID_t = PluginID (*)();
using Register_t = bool (*)(PluginMgr*);

#ifdef HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST
using voidvoid = void* (*)(void);
static inline voidvoid my_dlsym(void *handle, const char *symbol)
{
	void *value = dlsym(handle,symbol);
	voidvoid ret;
	assert(sizeof(ret)==sizeof(value) );
	memcpy(&ret, &value, sizeof(ret) );
	return ret;
}
#else
#define my_dlsym dlsym
#endif

#ifdef WIN32
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#define FREE_PLUGIN( handle )  FreeLibrary( handle )
#define GET_PLUGIN_SYMBOL( handle, name )  GetProcAddress( handle, name )
#else
#define FREE_PLUGIN( handle )  dlclose( handle )
#define GET_PLUGIN_SYMBOL( handle, name )  my_dlsym( handle, name )
#endif

#ifdef WIN32
static void PrintDLError()
{
	Log(DEBUG, "PluginLoader", "Error code: {}", GetLastError());
}
#else
static void PrintDLError()
{
	Log(DEBUG, "PluginLoader", "Error: {}", dlerror());
}
#endif

static bool LoadPlugin(const char* pluginpath)
{
#ifdef WIN32
	std::wstring path(_MAX_PATH, L'\0');
	mbstowcs(const_cast<wchar_t*>(path.data()), pluginpath, path.length());
	HMODULE hMod = LoadLibraryW(path.data());
#else
	// Note: the RTLD_GLOBAL is necessary to export symbols to modules
	//       which python may have to dlopen (-wjp, 20060716)
	// (to reproduce, try 'import bz2' or another .so module)
	void* hMod = dlopen(pluginpath, RTLD_NOW | RTLD_GLOBAL);
#endif

	if (hMod == NULL) {
		Log(ERROR, "PluginLoader", "Cannot Load \"{}\", skipping...", pluginpath);
		PrintDLError();
		return false;
	}

	//using C bindings, so we don't need to jump through extra hoops
	//with the symbol name
	Version_t LibVersion = ( Version_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Version" );
	if (LibVersion==NULL) {
		Log(ERROR, "PluginLoader", "Skipping invalid plugin \"{}\".", pluginpath);
		FREE_PLUGIN( hMod );
		return false;
	}
	if (strcmp(LibVersion(), VERSION_GEMRB) ) {
		Log(ERROR, "PluginLoader", "Skipping plugin \"{}\" with version mistmatch.", pluginpath);
		FREE_PLUGIN( hMod );
		return false;
	}

	Description_t Description = ( Description_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Description" );
	ID_t ID = ( ID_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_ID" );
	Register_t Register = ( Register_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Register" );

	static std::set<PluginID> libs;
	PluginDesc desc = { hMod, ID(), Description(), Register };

	if (libs.find(desc.ID) != libs.end()) {
		Log(WARNING, "PluginLoader", "Plug-in \"{}\" already loaded!", pluginpath);
		FREE_PLUGIN( hMod );
		return false;
	}
	if (desc.Register != NULL) {
		if (!desc.Register(PluginMgr::Get())) {
			Log(WARNING, "PluginLoader", "Plugin Registration Failed! Perhaps a duplicate?");
			FREE_PLUGIN( hMod );
		}
	}
	libs.insert(desc.ID);
	Log(MESSAGE, "PluginLoader", "Loaded plugin \"{}\" ({}).", desc.Description, pluginpath);
	return true;
}

void LoadPlugins(const path_t& pluginpath, const plugin_flags_t& pluginFlags)
{
	Log(MESSAGE, "PluginMgr", "Loading Plugins from {}", pluginpath);

	DirectoryIterator dirIt(pluginpath);
	if (!dirIt) {
		Log(ERROR, "PluginMgr", "Plugin Directory ({}) does not exist!", pluginpath);
		return;
	}

#ifdef WIN32
	auto pluginExt = "dll";
#else
	auto pluginExt = "so";
#endif

	dirIt.SetFlags(DirectoryIterator::Files);
	dirIt.SetFilterPredicate(std::make_shared<ExtFilter>(pluginExt)); // rewinds

	using PathSet = std::set<std::string>;
	PathSet delayedPlugins;

	if (!dirIt) {
		return;
	}

	do {
		const path_t& name = dirIt.GetName();

		auto flags = pluginFlags.Get(name);
		if (flags != nullptr) {
			// module is sent to the back
			if (*flags == PluginFlagsType::DELAY) {
				Log(MESSAGE, "PluginLoader", "Loading \"{}\" delayed.", name);
				delayedPlugins.emplace(name);
				continue;
			}

			// module is skipped
			if (*flags == PluginFlagsType::SKIP) {
				Log(MESSAGE, "PluginLoader", "Loading \"{}\" skipped.", name);
				continue;
			}
		}

		LoadPlugin(PathJoin(pluginpath, name).c_str());
	} while (++dirIt);

	PathSet::iterator it = delayedPlugins.begin();
	for (; it != delayedPlugins.end(); ++it) {
		LoadPlugin(it->c_str());
	}
}

}
