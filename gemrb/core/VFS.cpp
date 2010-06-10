/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// VFS.cpp : functions to access filesystem in os-independent way
// and POSIX-like compatibility layer for win

#include "VFS.h"

#include "globals.h"

#include "Interface.h"

#include <cstdarg>
#include <cstring>

#ifdef WIN32

// buffer which readdir returns
static dirent de;


DIR* opendir(const char* filename)
{
	DIR* dirp = ( DIR* ) malloc( sizeof( DIR ) );
	dirp->is_first = 1;

	sprintf( dirp->path, "%s%s*.*", filename, SPathDelimiter );
	//if((hFile = (long)_findfirst(Path, &c_file)) == -1L) //If there is no file matching our search

	return dirp;
}

dirent* readdir(DIR* dirp)
{
	struct _finddata_t c_file;

	if (dirp->is_first) {
		dirp->is_first = 0;
		dirp->hFile = ( long ) _findfirst( dirp->path, &c_file );
		if (dirp->hFile == -1L)
			return NULL;
	} else {
		if (( long ) _findnext( dirp->hFile, &c_file ) != 0) {
			return NULL;
		}
	}

	strcpy( de.d_name, c_file.name );

	return &de;
}

void closedir(DIR* dirp)
{
	_findclose( dirp->hFile );
	free( dirp );
}


_FILE* _fopen(const char* filename, const char* mode)
{
	DWORD OpenFlags = 0;
	DWORD AccessFlags = 0;
	DWORD ShareFlags = 0;

	while (*mode) {
		if (( *mode == 'w' ) || ( *mode == 'W' )) {
			OpenFlags |= OPEN_ALWAYS;
			AccessFlags |= GENERIC_WRITE;
			ShareFlags |= FILE_SHARE_READ;
		} else if (( *mode == 'r' ) || ( *mode == 'R' )) {
			OpenFlags |= OPEN_EXISTING;
			AccessFlags |= GENERIC_READ;
			ShareFlags |= FILE_SHARE_READ|FILE_SHARE_WRITE;
		} else if (( *mode == 'a' ) || ( *mode == 'A' )) {
			OpenFlags |= OPEN_ALWAYS;
			AccessFlags |= GENERIC_READ|GENERIC_WRITE;
			ShareFlags |= FILE_SHARE_READ|FILE_SHARE_WRITE;
		} else if (*mode == '+') {
			AccessFlags |= GENERIC_READ|GENERIC_WRITE;
			ShareFlags |= FILE_SHARE_READ|FILE_SHARE_WRITE;
		}
		mode++;
	}
	HANDLE hFile = CreateFile( filename, AccessFlags, ShareFlags, NULL,
					OpenFlags, FILE_ATTRIBUTE_NORMAL, NULL );
	if (hFile == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	_FILE* ret = ( _FILE* ) malloc( sizeof( _FILE ) );
	ret->hFile = hFile;
	return ret;
}

size_t _fread(void* ptr, size_t size, size_t n, _FILE* stream)
{
	if (!stream) {
		return ( size_t ) 0;
	}
	unsigned long read;
	if (!ReadFile( stream->hFile, ptr, ( unsigned long ) ( size * n ), &read,
			NULL )) {
		return ( size_t ) 0;
	}
	return ( size_t ) read;
}

size_t _fwrite(const void* ptr, size_t size, size_t n, _FILE* stream)
{
	if (!stream) {
		return ( size_t ) 0;
	}
	unsigned long wrote;
	if (!WriteFile( stream->hFile, ptr, ( unsigned long ) ( size * n ),
			&wrote, NULL )) {
		return ( size_t ) 0;
	}
	return ( size_t ) wrote;
}

size_t _fseek(_FILE* stream, long offset, int whence)
{
	if (!stream) {
		return ( size_t ) 1;
	}
	unsigned long method;
	switch (whence) {
		case SEEK_SET:
			method = FILE_BEGIN;
			break;
		case SEEK_CUR:
			method = FILE_CURRENT;
			break;
		case SEEK_END:
			method = FILE_END;
			break;
		default:
			return ( size_t ) 1;
	}
	if (SetFilePointer( stream->hFile, offset, NULL, method ) == 0xffffffff) {
		return ( size_t ) 1;
	}
	return ( size_t ) 0;
}

int _fgetc(_FILE* stream)
{
	if (!stream) {
		return 0;
	}
	unsigned char tmp;
	unsigned long read;
	BOOL bResult = ReadFile( stream->hFile, &tmp, ( unsigned long ) 1, &read,
					NULL );
	if (bResult && read) {
		return ( int ) tmp;
	} 
	return EOF;
}

long int _ftell(_FILE* stream)
{
	if (!stream) {
		return EOF;
	}
	unsigned long pos = SetFilePointer( stream->hFile, 0, NULL, FILE_CURRENT );
	if (pos == 0xffffffff) {
		return -1L;
	}
	return ( long int ) pos;
}

int _feof(_FILE* stream)
{
	if (!stream) {
		return 0;
	}
	unsigned char tmp;
	unsigned long read;
	BOOL bResult = ReadFile( stream->hFile, &tmp, ( unsigned long ) 1, &read,
					NULL );
	if (bResult && ( read == 0 )) {
		return 1;
	} //EOF
	bResult = SetFilePointer( stream->hFile, -1, NULL, FILE_CURRENT );
	return 0;
}

int _fclose(_FILE* stream)
{
	if (!stream) {
		return EOF;
	}
	if (!CloseHandle( stream->hFile )) {
		return EOF;
	}
	free( stream );
	return 0;
}

#endif // WIN32


/** Returns true if path is an existing directory */
bool dir_exists(const char* path)
{
	struct stat buf;

	buf.st_mode = 0;
	stat( path, &buf );
	return S_ISDIR( buf.st_mode ) != 0;
}



/**
 * Appends dir 'dir' to path 'target' and returns 'target'.
 * It takes care of inserting PathDelimiter ('/' or '\\') if needed
 */
char* PathAppend (char* target, const char* dir)
{
	if (target[0] != 0 && target[strlen( target ) - 1] != PathDelimiter)
		strncat( target, SPathDelimiter, _MAX_PATH );
	strncat( target, dir, _MAX_PATH );

	return target;
}


bool FindInDir(const char* Dir, char *Filename)
{
	// First test if there's a Filename with exactly same name
	// and if yes, return it and do not search in the Dir
	char TempFilePath[_MAX_PATH];
	strcpy(TempFilePath, Dir);
	PathAppend( TempFilePath, Filename );

	if (!access( TempFilePath, R_OK )) {
		return true;
	}

	if (!core->CaseSensitive) {
		return false;
	}

	DIR* dir = opendir( Dir );
	if (dir == NULL) {
		return false;
	}

	// Exact match not found, so try to search for Filename
	// with different case
	struct dirent* de = readdir( dir );
	if (de == NULL) {
		closedir( dir );
		return false;
	}
	do {
		if (stricmp( de->d_name, Filename ) == 0) {
			strcpy( Filename, de->d_name );
			closedir(dir);
			return true;
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir ); //No other files in the directory, close it
	return false;
}

bool PathJoin (char *target, const char *base, ...)
{
	va_list ap;
	va_start(ap, base);

	if (base == NULL) {
		target[0] = '\0';
		return false;
	}

	strcpy(target, base);

	while (char *source = va_arg(ap, char*)) {
		char *slash;
		do {
			char filename[_MAX_PATH] = { '\0' };
			slash = strchr(source, PathDelimiter);
			if (slash == source) {
				++source;
				continue;
			} else if (slash) {
				strncat(filename, source, slash-source);
			} else {
				strcpy(filename, source);
			}
			if (!FindInDir(target, filename)) {
				PathAppend(target, source);
				goto finish;
			}
			PathAppend(target, filename);
			source = slash + 1;
		} while (slash);
	}
	va_end( ap );
	return true;
finish:
	while (char *source = va_arg(ap, char*)) {
		PathAppend(target, source);
	}
	va_end( ap );
	return false;
}

bool PathJoinExt (char* target, const char* dir, const char* base, const char* ext)
{
	char file[_MAX_PATH];
	strcpy(file, base);
	strcat(file, ".");
	strcat(file, ext);
	return PathJoin(target, dir, file, NULL);
}

/** Fixes path delimiter character (slash).
 * needslash = true : we add a slash
 * needslash = false: we remove the slash
 */
void FixPath (char *path, bool needslash)
{
	size_t i = strlen( path ) - 1;

	if (needslash) {
		if (path[i] == PathDelimiter) return;

		// if path is already too long, don't do anything
		if (i >= _MAX_PATH - 2) return;
		i++;
		path[i++] = PathDelimiter;
	}
	else {
		if (path[i] != PathDelimiter) return;
	}
	path[i] = 0;
}

int strmatch(const char *string, const char *mask)
{
	while(*mask) {
		if (*mask!='?') {
			if (tolower(*mask)!=tolower(*string)) {
				return 1;
			}
		}
		mask++;
		string++;
	}
	return 0;
}

bool FileGlob(char* target, const char* Dir, const char *glob)
{
	DIR* dir = opendir( Dir );
	if (dir == NULL) {
		return false;
	}

	struct dirent* de = readdir( dir );
	if (de == NULL) {
		closedir( dir );
		return false;
	}
	do {
		if (strmatch( de->d_name, glob ) == 0) {
			strcpy( target, de->d_name );
			closedir(dir);
			return true;
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir ); //No other files in the directory, close it
	return false;
}


#ifndef WIN32

void ResolveFilePath(char* FilePath)
{
	char TempFilePath[_MAX_PATH];

	if (FilePath[0]=='~') {
		const char *home = getenv("HOME");
		if (home) {
			strcpy(TempFilePath, FilePath+1);
			PathJoin(FilePath, home, TempFilePath, NULL);
			return;
		}
	}

	if (core && !core->CaseSensitive) {
		return;
	}
	strcpy(TempFilePath, FilePath);
	PathJoin(FilePath, TempFilePath[0]==PathDelimiter?SPathDelimiter:"", TempFilePath, NULL);
}

#endif

void ExtractFileFromPath(char *file, const char *full_path)
{
	const char *p;
	if ((p = strrchr (full_path, PathDelimiter)))
		strcpy(file, p+1);
	else if ((p = strchr (full_path, ':')))
		strcpy(file, p+1);
	else
		strcpy(file, full_path);
}
