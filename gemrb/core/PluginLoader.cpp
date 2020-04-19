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

#include "SClassID.h" // For PluginID
#include "win32def.h"

#include "Interface.h"
#include "PluginMgr.h"
#include "System/FileFilters.h"
#include "Variables.h"

#include <cstdio>
#include <cstdlib>
#include <set>

#ifdef WIN32
#include <string.h>
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST
# include <assert.h>
#endif

namespace GemRB {

#ifdef WIN32
typedef HMODULE LibHandle;
#else
typedef void *LibHandle;
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

typedef const char* (*Version_t)(void);
typedef const char* (*Description_t)(void);
typedef PluginID (*ID_t)();
typedef bool (* Register_t)(PluginMgr*);

#ifdef HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST
typedef void *(* voidvoid)(void);
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
#define FREE_PLUGIN( handle )  FreeLibrary( handle )
#define GET_PLUGIN_SYMBOL( handle, name )  GetProcAddress( handle, name )
#else
#define FREE_PLUGIN( handle )  dlclose( handle )
#define GET_PLUGIN_SYMBOL( handle, name )  my_dlsym( handle, name )
#endif

#ifdef WIN32
static void PrintDLError(const char *msg=NULL)
{
	char buffer[_MAX_PATH*2];
#ifdef _MSC_VER
	_strerror_s(buffer, NULL);
#else
	sprintf(buffer, "code %lu", GetLastError());
#endif
	Log(DEBUG, "PluginLoader", msg ? msg : "Error: %s", buffer);
}
#else
static void PrintDLError()
{
	Log(DEBUG, "PluginLoader", "Error: %s", dlerror());
}
#endif

static bool LoadPlugin(const char* pluginpath)
{
	// Try to load the Module
#ifdef WIN32
	HMODULE hMod = LoadLibrary( pluginpath );
#else
	// Note: the RTLD_GLOBAL is necessary to export symbols to modules
	//       which python may have to dlopen (-wjp, 20060716)
	// (to reproduce, try 'import bz2' or another .so module)
	void* hMod = dlopen( pluginpath, RTLD_NOW | RTLD_GLOBAL );
#endif
	if (hMod == NULL) {
		Log(ERROR, "PluginLoader", "Cannot Load \"%s\", skipping...", pluginpath);
		PrintDLError();
		return false;
	}

	//using C bindings, so we don't need to jump through extra hoops
	//with the symbol name
	Version_t LibVersion = ( Version_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Version" );
	if (LibVersion==NULL) {
		Log(ERROR, "PluginLoader", "Skipping invalid plugin \"%s\".", pluginpath);
		FREE_PLUGIN( hMod );
		return false;
	}
	if (strcmp(LibVersion(), VERSION_GEMRB) ) {
		Log(ERROR, "PluginLoader", "Skipping plugin \"%s\" with version mistmatch.", pluginpath);
		FREE_PLUGIN( hMod );
		return false;
	}

	Description_t Description = ( Description_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Description" );
	ID_t ID = ( ID_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_ID" );
	Register_t Register = ( Register_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Register" );

	static std::set<PluginID> libs;
	PluginDesc desc = { hMod, ID(), Description(), Register };

	if (libs.find(desc.ID) != libs.end()) {
		Log(WARNING, "PluginLoader", "Plug-in \"%s\" already loaded!", pluginpath);
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
	Log(MESSAGE, "PluginLoader", "Loaded plugin \"%s\" (%s).", desc.Description, pluginpath);
	return true;
}

void LoadPlugins(const char* pluginpath)
{
	Log(MESSAGE, "PluginMgr", "Loading Plugins from %s", pluginpath);

	DirectoryIterator dirIt(pluginpath);
	if (!dirIt) {
		Log(ERROR, "PluginMgr", "Plugin Directory (%s) does not exist!", pluginpath);
		return;
	}

#ifdef WIN32
	const char* pluginExt = "dll";
#else
	const char* pluginExt = "so";
#endif

	dirIt.SetFlags(DirectoryIterator::Files);
	dirIt.SetFilterPredicate(new ExtFilter(pluginExt)); // rewinds

	typedef std::set<std::string> PathSet;
	PathSet delayedPlugins;

	if (!dirIt) {
		return;
	}

	char path[_MAX_PATH];
	do {
		const char *name = dirIt.GetName();
		ieDword flags = 0;
		core->plugin_flags->Lookup (name, flags);

		// module is sent to the back
		if (flags == PLF_DELAY) {
			Log(MESSAGE, "PluginLoader", "Loading \"%s\" delayed.", name);
			delayedPlugins.insert( name );
			continue;
		}

		// module is skipped
		if (flags == PLF_SKIP) {
			Log(MESSAGE, "PluginLoader", "Loading \"%s\" skipped.", name);
			continue;
		}

		PathJoin( path, pluginpath, name, NULL );
		LoadPlugin(path);
	} while (++dirIt);

	PathSet::iterator it = delayedPlugins.begin();
	for (; it != delayedPlugins.end(); ++it) {
		LoadPlugin(it->c_str());
	}
}

}
