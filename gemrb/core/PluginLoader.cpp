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

#include "Variables.h"
#include "Interface.h"
#include "PluginMgr.h"

#include <cstdio>
#include <cstdlib>
#include <list>
#include <set>

#ifdef WIN32
#include <io.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <dlfcn.h>
#endif

#ifdef HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST
# include <assert.h>
#endif

namespace GemRB {

#ifdef WIN32
#define TCHAR_FORMAT "%ls"
typedef HMODULE LibHandle;
#else
#define TCHAR_FORMAT "%s"
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

/** Return names of all *.so or *.dll files in the given directory */
#ifdef WIN32

static bool FindFiles(TCHAR* path, std::list<TCHAR*> &files)
{
	//The windows _findfirst/_findnext functions allow the use of wildcards so we'll use them :)
	struct _tfinddata_t c_file;
	intptr_t hFile;

	StringCchCat(path, _MAX_PATH, TEXT("*.dll"));
	if ((hFile = _tfindfirst(path, &c_file)) == -1L) {
		//If there is no file matching our search
		Log(ERROR, "PluginLoader", "Error looking up dlls.");
		return false;
	}

	do {
		files.push_back(_tcsdup(c_file.name));
	} while (_tfindnext(hFile, &c_file) == 0);

	_findclose( hFile );
	return true;
}

#else // ! WIN32

static bool FindFiles(char* path, std::list<char*> &files)
{
	DirectoryIterator dir(path);
	if (!dir) //If we cannot open the Directory
		return false;

	do {
		const char *name = dir.GetName();
		if (fnmatch( "*.so", name, 0 ) != 0) //If the current file has no ".so" extension, skip it
			continue;
		files.push_back( strdup( name ));
	} while (++dir);

	return true;
}
#endif  // ! WIN32

void LoadPlugins(char* pluginpath)
{
	std::set<PluginID> libs;

	Log(MESSAGE, "PluginMgr", "Loading Plugins from %s", pluginpath);

#ifdef WIN32
	TCHAR path[_MAX_PATH];
	TCHAR t_pluginpath[_MAX_PATH] = {0};
	std::list<TCHAR*> files;

	mbstowcs(t_pluginpath, pluginpath, _MAX_PATH - 1);
	StringCbCopy(path, _MAX_PATH, t_pluginpath);
#else
	char path[_MAX_PATH];
	std::list<char*> files;

	strlcpy(path, pluginpath, _MAX_PATH);
#endif

	if (! FindFiles( path, files )) {
#ifndef STATIC_LINK
		Log(ERROR, "PluginLoader", "Cannot find any plugins!");
#endif
		return;
	}

	//Iterate through all the available modules to load
	int file_count = files.size (); // keeps track of first-pass files
	while (! files.empty()) {
#ifdef WIN32
		TCHAR* t_file = files.front();
		char file[_MAX_PATH] = {0};
		wcstombs(file, t_file, _MAX_PATH);
#else
		char* file = files.front();
		char* t_file = file;
#endif
		files.pop_front();
		file_count--;

#ifdef WIN32
		StringCbPrintf(path, _MAX_PATH * sizeof(TCHAR), TEXT("%s/%s"), t_pluginpath, t_file);
#else
		PathJoin( path, pluginpath, file, NULL );
#endif

		ieDword flags = 0;
		core->plugin_flags->Lookup (file, flags);

		// module is sent to the back
		if ((flags == PLF_DELAY) && (file_count >= 0)) {
			Log(MESSAGE, "PluginLoader", "Loading \"" TCHAR_FORMAT "\" delayed.", path);
			files.push_back(t_file);
			continue;
		}

		// module is skipped
		if (flags == PLF_SKIP) {
			Log(MESSAGE, "PluginLoader", "Loading \"" TCHAR_FORMAT "\" skipped.", path);
			continue;
		}



		// Try to load the Module
#ifdef WIN32
		HMODULE hMod = LoadLibrary( path );
#else
		// Note: the RTLD_GLOBAL is necessary to export symbols to modules
		//       which python may have to dlopen (-wjp, 20060716)
		// (to reproduce, try 'import bz2' or another .so module)
		void* hMod = dlopen( path, RTLD_NOW | RTLD_GLOBAL );
#endif
		if (hMod == NULL) {
			Log(ERROR, "PluginLoader", "Cannot Load \"" TCHAR_FORMAT "\", skipping...", path);
			continue;
		}

		//using C bindings, so we don't need to jump through extra hoops
		//with the symbol name
		Version_t LibVersion = ( Version_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Version" );
		Description_t Description = ( Description_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Description" );
		ID_t ID = ( ID_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_ID" );
		Register_t Register = ( Register_t ) (void*) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Register" );

		if (LibVersion==NULL) {
			Log(ERROR, "PluginLoader", "Skipping invalid plugin \"" TCHAR_FORMAT "\".", path);
			FREE_PLUGIN( hMod );
			continue;
		}
		if (strcmp(LibVersion(), VERSION_GEMRB) ) {
			Log(ERROR, "PluginLoader", "Skipping plugin \"" TCHAR_FORMAT "\" with version mistmatch.", path);
			FREE_PLUGIN( hMod );
			continue;
		}

		PluginDesc desc = { hMod, ID(), Description(), Register };

		if (libs.find(desc.ID) != libs.end()) {
			Log(WARNING, "PluginLoader", "Plug-in \"" TCHAR_FORMAT "\" already loaded!", path);
			FREE_PLUGIN( hMod );
			continue;
		}
		if (desc.Register != NULL) {
			if (!desc.Register(PluginMgr::Get())) {
				Log(WARNING, "PluginLoader", "Plugin Registration Failed! Perhaps a duplicate?");
				FREE_PLUGIN( hMod );
			}
		}
		libs.insert(desc.ID);

		Log(MESSAGE, "PluginLoader", "Loaded plugin \"%s\" (%s).", desc.Description, file);

		// We do not need the basename anymore now
		free(t_file);
	}
}

}
