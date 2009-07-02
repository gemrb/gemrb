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

#include "../../includes/win32def.h"
#include "stdio.h"
#include "stdlib.h"
#include "PluginMgr.h"
#include "Plugin.h"
#include "Interface.h"
// FIXME: this should be in Interface.h instead
#include "Variables.h"

#ifdef WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <dlfcn.h>
#endif

typedef char*(* charvoid)(void);
typedef ClassDesc*(* cdvoid)(void);

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
#define PRINT_DLERROR printf( "%s\n", dlerror() )
#endif

PluginMgr::PluginMgr(char* pluginpath)
{
	printMessage( "PluginMgr", "Loading Plugins from ", WHITE );
	printf( "%s\n", pluginpath );
	
	char path[_MAX_PATH];
	strcpy( path, pluginpath );

	std::list< char * > files;
	if (! FindFiles( path, files ))
		return;
	
	charvoid LibVersion;
	charvoid LibDescription;
	cdvoid LibClassDesc;

	//Iterate through all the available modules to load
	int file_count = files.size (); // keeps track of first-pass files
	while (! files.empty()) {
		char* file = files.front();
		files.pop_front();
		file_count--;

		PathJoin( path, pluginpath, file, NULL );		
		printBracket( "PluginMgr", LIGHT_WHITE );
		printf( ": Loading: " );
		textcolor( LIGHT_WHITE );
		printf( "%s", path );
		textcolor( WHITE );
		printf( "..." );

		
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
		void* hMod = dlopen( path, RTLD_NOW | RTLD_GLOBAL ); 
#endif
		if (hMod == NULL) {
			printBracket( "ERROR", LIGHT_RED );
			printf( "\nCannot Load Module, Skipping...\n" );
			PRINT_DLERROR;
			continue;
		}

		printStatus( "OK", LIGHT_GREEN );
		//using C bindings, so we don't need to jump through extra hoops
		//with the symbol name
		LibVersion = ( charvoid ) GET_PLUGIN_SYMBOL( hMod, "LibVersion" );
		LibDescription = ( charvoid ) GET_PLUGIN_SYMBOL( hMod, "LibDescription" );
		LibClassDesc = ( cdvoid ) GET_PLUGIN_SYMBOL( hMod, "LibClassDesc" );
		
		printMessage( "PluginMgr", "Checking Plugin Version...", WHITE );
		if (LibVersion==NULL) {
			printStatus( "ERROR", LIGHT_RED );
			printf( "Invalid Plug-in, Skipping...\n" );
			FREE_PLUGIN( hMod );
			continue;
		}
		if (strcmp(LibVersion(), VERSION_GEMRB) ) {
			printStatus( "ERROR", LIGHT_RED );
			printf( "Plug-in Version not valid, Skipping...\n" );
			FREE_PLUGIN( hMod );
			continue;
		}
		printStatus( "OK", LIGHT_GREEN );
		printMessage( "PluginMgr", "Loading Exports for ", WHITE );
		textcolor( LIGHT_WHITE );
		printf( "%s", LibDescription() );
		textcolor( WHITE );
		printf( "..." );
		printStatus( "OK", LIGHT_GREEN );
		bool error = false;
		ClassDesc* plug = LibClassDesc();
		if (plug == NULL) {
			printMessage( "PluginMgr", "Plug-in Exports Error! ", WHITE );
			printStatus( "ERROR", LIGHT_RED );
			continue;
		} 
		for (unsigned int x = 0; x < plugins.size(); x++) {
			if (plugins[x]->ClassID() == plug->ClassID()) {
				printMessage( "PluginMgr", "Plug-in Already Loaded! ", WHITE );
				printStatus( "SKIPPING", YELLOW );
				error = true;
				break;
			}

			if (plugins[x]->SuperClassID() == plug->SubClassID()) {
				printMessage( "PluginMgr", "Duplicate Plug-in! ", WHITE );
				printStatus( "SKIPPING", YELLOW );
				error = true;
				break;
			}
		}
		if (error) {
			FREE_PLUGIN( hMod );
			continue;
		}
		plugins.push_back( plug );
		libs.push_back( hMod );
	}
}

PluginMgr::~PluginMgr(void)
{
//don't free the shared libraries in debug mode, so valgrind can resolve the stack trace
#ifndef _DEBUG
	for (unsigned int i = 0; i < libs.size(); i++) {
#ifdef WIN32
		FreeLibrary(libs[i]);
#else
	//	dlclose(libs[i]);
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
	DIR* dir = opendir( path );
	if (dir == NULL) //If we cannot open the Directory
		return false;

	struct dirent* de = readdir( dir );  //Lookup the first entry in the Directory
	if (de == NULL) {
		//If no entry exists just return
		closedir( dir );
		return false;
	}
	
	do {
		if (fnmatch( "*.so", de->d_name, 0 ) != 0) //If the current file has no ".so" extension, skip it
			continue;
		files.push_back( strdup( de->d_name ));		
	} while (( de = readdir( dir ) ) != NULL);
	
	closedir( dir ); //No other files in the directory, close it
	return true;
}
#endif  // ! WIN32

bool PluginMgr::IsAvailable(SClass_ID plugintype) const
{
	for (unsigned int i = 0; i < plugins.size(); i++) {
		if (plugins[i]->SuperClassID() == plugintype) {
			return true;
		}
	}
	return false;
}

void* PluginMgr::GetPlugin(SClass_ID plugintype) const
{
	if (!&plugins) {
		return NULL;
	}
	size_t plugs = plugins.size();
	while (plugs--) {
		if (plugins[plugs]->SuperClassID() == plugintype) {
			return ( plugins[plugs] )->Create();
		}
	}
	return NULL;
}

std::vector<InterfaceElement> *PluginMgr::GetAllPlugin(SClass_ID plugintype) const
{
	if (!&plugins) {
		return NULL;
	}
	std::vector<InterfaceElement> *ret = new std::vector<InterfaceElement>;

	size_t plugs = plugins.size();
	while (plugs--) {
		if (plugins[plugs]->SubClassID() == plugintype) {
			InterfaceElement tmp={( plugins[plugs] )->Create(), tmp.mgr==NULL};
			ret->push_back( tmp );
		}
	}
	return ret;
}

void PluginMgr::FreePlugin(void* ptr)
{
	if (ptr)
		( ( Plugin * ) ptr )->release();
}
