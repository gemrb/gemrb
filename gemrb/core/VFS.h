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

/**
 * @file VFS.h
 * Compatibility layer for file and dir access functions on Un*x and MS Win
 * @author The GemRB Project
 */

#ifndef VFS_H
#define VFS_H

#ifdef WIN32
#include <io.h>
#include <windows.h>
#include <direct.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <dlfcn.h>
#endif

#ifndef _MAX_PATH
#ifdef WIN32
#define _MAX_PATH 260
#else
#define _MAX_PATH FILENAME_MAX
#endif
#endif
#include "exports.h"
#include "globals.h"

//#ifndef S_ISDIR
//#define S_ISDIR(x) ((x) & S_IFDIR)
//#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef R_OK
#define R_OK 04
#endif

#ifdef WIN32

typedef struct DIR {
	char path[_MAX_PATH];
	bool is_first;
	struct _finddata_t c_file;
	long hFile;
} DIR;

typedef struct dirent {
	char d_name[_MAX_PATH];
} dirent;

DIR* opendir(const char* filename);
dirent* readdir(DIR* dirp);
void closedir(DIR* dirp);

typedef struct _FILE {
	HANDLE hFile;
} _FILE;

GEM_EXPORT _FILE* _fopen(const char* filename, const char* mode);
GEM_EXPORT size_t _fread(void* ptr, size_t size, size_t n, _FILE* stream);
GEM_EXPORT size_t _fwrite(const void* ptr, size_t size, size_t n,
	_FILE* stream);
GEM_EXPORT size_t _fseek(_FILE* stream, long offset, int whence);
GEM_EXPORT int _fgetc(_FILE* stream);
GEM_EXPORT long int _ftell(_FILE* stream);
GEM_EXPORT int _feof(_FILE* stream);
GEM_EXPORT int _fclose(_FILE* stream);
#define mkdir(path, rights)  _mkdir(path)
#define ResolveFilePath(p)

#else  // ! WIN32

#define _FILE FILE
#define _fopen fopen
#define _fread fread
#define _fwrite fwrite
#define _fseek fseek
#define _fgetc fgetc
#define _ftell ftell
#define _feof feof
#define _fclose fclose

/** Handle ~ -> $HOME mapping and do initial case-sensitity check */
GEM_EXPORT void ResolveFilePath(char* FilePath);

#endif  // ! WIN32

/**
 * Finds a file matching a glob.
 *
 * @param[out] target name of matching file
 * @param[in] Dir directory to look in
 * @param[in] glob pattern to match
 * @return true if match is found
 */
GEM_EXPORT bool FileGlob(char *target, const char* Dir, const char* glob);
GEM_EXPORT bool dir_exists(const char* path);

/**
 * Joins NULL-terminated list of directories and copies it to 'target'.
 *
 * @param[out] target Joined path.
 * @param[in] base Properly cased path to join to.
 * @param[in] ... NULL terminated list of paths to join.
 *
 * This does a case-sensitive look up for all path components after the first and
 * properly handles the case when paramater contain slashes.
 *
 * NOTE: This no longer handles target==base.
 *
 * Example:
 * char filepath[_MAX_PATH];
 * PathJoin( filepath, core->GUIScriptsPath, core->GameType, 'GUIDefines.py', NULL );
 */
GEM_EXPORT bool PathJoin (char* target, const char* base, ...);
GEM_EXPORT bool PathJoinExt (char* target, const char* dir, const char* file, const char* ext = NULL);
GEM_EXPORT void FixPath (char *path, bool needslash);

GEM_EXPORT void ExtractFileFromPath(char *file, const char *full_path);

#endif  // !VFS_H
