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

#include "PluginMgr.h"

#include "win32def.h"

#include "Interface.h"
#include "Plugin.h"
#include "ResourceDesc.h"
#include "Variables.h" // FIXME: this should be in Interface.h instead

#include <cstdio>
#include <cstdlib>
#ifdef WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <dlfcn.h>
#endif

typedef const char* (*Version_t)(void);
typedef const char* (*Description_t)(void);
typedef PluginID (*ID_t)();
typedef bool (* Register_t)(PluginMgr*);

#ifdef HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST
#include <assert.h>
typedef void *(* voidvoid)(void);
inline voidvoid my_dlsym(void *handle, const char *symbol)
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
#define PRINT_DLERROR
#else
#define FREE_PLUGIN( handle )  dlclose( handle )
#define GET_PLUGIN_SYMBOL( handle, name )  my_dlsym( handle, name )
#define PRINT_DLERROR print( "%s\n", dlerror() )
#endif

PluginMgr *PluginMgr::Get()
{
	static PluginMgr mgr;
	return &mgr;
}

PluginMgr::PluginMgr()
{
}

void PluginMgr::LoadPlugins(char* pluginpath)
{
	printMessage("PluginMgr", "Loading Plugins from %s\n", WHITE, pluginpath);
	
	char path[_MAX_PATH];
	strcpy( path, pluginpath );

	std::list< char * > files;
	if (! FindFiles( path, files ))
		return;
	
	//Iterate through all the available modules to load
	int file_count = files.size (); // keeps track of first-pass files
	while (! files.empty()) {
		char* file = files.front();
		files.pop_front();
		file_count--;

		PathJoin( path, pluginpath, file, NULL );		
		printBracket( "PluginMgr", LIGHT_WHITE );
		print( ": Loading: " );
		textcolor( LIGHT_WHITE );
		print( "%s", path );
		textcolor( WHITE );
		print( "..." );

		
		ieDword flags = 0;
		core->plugin_flags->Lookup (file, flags);
		 
		// module is sent to the back
		if ((flags == PLF_DELAY) && (file_count >= 0)) {
			printStatus( "DELAYING", YELLOW );
			files.push_back( file );
			continue;
		} 

		// We do not need the basename anymore now
		free( file );
		
		// module is skipped
		if (flags == PLF_SKIP) {
			printStatus( "SKIPPING", YELLOW );
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
			printBracket( "ERROR", LIGHT_RED );
			print( "\nCannot Load Module, Skipping...\n" );
			PRINT_DLERROR;
			continue;
		}

		//printStatus( "OK", LIGHT_GREEN );
		//using C bindings, so we don't need to jump through extra hoops
		//with the symbol name
		Version_t LibVersion = ( Version_t ) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Version" );
		Description_t Description = ( Description_t ) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Description" );
		ID_t ID = ( ID_t ) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_ID" );
		Register_t Register = ( Register_t ) GET_PLUGIN_SYMBOL( hMod, "GemRBPlugin_Register" );

		//printMessage( "PluginMgr", "Checking Plugin Version...", WHITE );
		if (LibVersion==NULL) {
			printStatus( "ERROR", LIGHT_RED );
			print( "Invalid Plug-in, Skipping...\n" );
			FREE_PLUGIN( hMod );
			continue;
		}
		if (strcmp(LibVersion(), VERSION_GEMRB) ) {
			printStatus( "ERROR", LIGHT_RED );
			print( "Plug-in Version not valid, Skipping...\n" );
			FREE_PLUGIN( hMod );
			continue;
		}

		PluginDesc desc = { hMod, ID(), Description(), Register };

		//printStatus( "OK", LIGHT_GREEN );
		//printMessage( "PluginMgr", "Loading Exports for ", WHITE );
		print( " " );
		textcolor( LIGHT_WHITE );
		print( "%s", desc.Description );
		textcolor( WHITE );
		print( "..." );
		printStatus( "OK", LIGHT_GREEN );
		if (libs.find(desc.ID) != libs.end()) {
			printMessage( "PluginMgr", "Plug-in Already Loaded! ", WHITE );
			printStatus( "SKIPPING", YELLOW );
			FREE_PLUGIN( hMod );
			continue;
		}
		if (desc.Register != NULL) {
			if (!desc.Register(this)) {
				printMessage( "PluginMgr", "Plug-in Registration Failed! Perhaps a duplicate? ", WHITE );
				printStatus( "SKIPPING", YELLOW );
				FREE_PLUGIN( hMod );
			}
		}
		libs[desc.ID] = desc;
	}
}

PluginMgr::~PluginMgr(void)
{
//don't free the shared libraries in debug mode, so valgrind can resolve the stack trace
#ifndef _DEBUG
	for (unsigned int i = 0; i < libs.size(); i++) {
#ifdef WIN32
		FreeLibrary(libs[i].handle);
#else
	//	dlclose(libs[i].handle);
#endif
	}
#endif
}

#ifdef WIN32
bool
PluginMgr::FindFiles( char* path, std::list<char*> &files )
{
	//The windows _findfirst/_findnext functions allow the use of wildcards so we'll use them :)
	struct _finddata_t c_file;
	long hFile;
	strcat( path, "*.dll" );
	if (( hFile = ( long ) _findfirst( path, &c_file ) ) == -1L) //If there is no file matching our search
		return false;
		
	do {
		files.push_back( strdup( c_file.name )); 
	} while (_findnext( hFile, &c_file ) == 0);
	
	_findclose( hFile );
	return true;
}

#else // ! WIN32

bool
PluginMgr::FindFiles( char* path, std::list<char*> &files )
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

bool PluginMgr::IsAvailable(SClass_ID plugintype) const
{
	return plugins.find(plugintype) != plugins.end();
}

Plugin* PluginMgr::GetPlugin(SClass_ID plugintype) const
{
	std::map<SClass_ID, PluginFunc>::const_iterator iter = plugins.find(plugintype);
	if (iter != plugins.end())
		return iter->second();
	return NULL;
}

const std::vector<ResourceDesc>& PluginMgr::GetResourceDesc(const TypeID* type)
{
	return resources[type];
}

bool PluginMgr::RegisterPlugin(SClass_ID id, PluginFunc create)
{
	if (plugins.find(id) != plugins.end())
		return false;
	plugins[id] = create;
	return true;
}

void PluginMgr::RegisterResource(const TypeID* type, ResourceFunc create, const char *ext, ieWord keyType)
{
	resources[type].push_back(ResourceDesc(type,create,ext,keyType));
}

void PluginMgr::RegisterInitializer(void (*func)(void))
{
	intializerFunctions.push_back(func);
}

void PluginMgr::RegisterCleanup(void (*func)(void))
{
	cleanupFunctions.push_back(func);
}

void PluginMgr::RunInitializers() const
{
	for (size_t i = 0; i < intializerFunctions.size(); i++)
		intializerFunctions[i]();
}

void PluginMgr::RunCleanup() const
{
	for (size_t i = 0; i < cleanupFunctions.size(); i++)
		cleanupFunctions[i]();
}

bool PluginMgr::RegisterDriver(const TypeID* type, const char* name, PluginFunc create)
{
	driver_map &map = drivers[type];
	driver_map::const_iterator iter = map.find(name);
	if (iter != map.end())
		return false;
	map[name] = create;
	return true;
}

Plugin* PluginMgr::GetDriver(const TypeID* type, const char* name)
{
	driver_map &map = drivers[type];
	if (map.begin() == map.end())
		return NULL;
	driver_map::const_iterator iter = map.find(name);
	if (iter != map.end())
		return iter->second();
	return map.begin()->second();
}
