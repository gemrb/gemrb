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

#include "System/VFS.h"

#include "globals.h"

#include "Interface.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cstdarg>
#include <cstring>
#include <cerrno>

#ifndef WIN32
#include <dirent.h>
#endif

#ifdef __APPLE__
// for getting resources inside the bundle
#include <CoreFoundation/CFBundle.h>
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#ifdef WIN32

using namespace GemRB;

struct DIR {
	char path[_MAX_PATH];
	bool is_first;
	struct _finddata_t c_file;
	long hFile;
};

struct dirent {
	char d_name[_MAX_PATH];
};

// buffer which readdir returns
static dirent de;

static DIR* opendir(const char* filename)
{
	DIR* dirp = ( DIR* ) malloc( sizeof( DIR ) );
	dirp->is_first = 1;

	sprintf( dirp->path, "%s%s*.*", filename, SPathDelimiter );
	//if((hFile = (long)_findfirst(Path, &c_file)) == -1L) //If there is no file matching our search

	return dirp;
}

static dirent* readdir(DIR* dirp)
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

static void closedir(DIR* dirp)
{
	_findclose( dirp->hFile );
	free( dirp );
}

#endif // WIN32

namespace GemRB {
#if __APPLE__
//bundle path functions
void CopyBundlePath(char* outPath, ieWord maxLen, BundleDirectory dir)
{
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef bundleDirURL = NULL;
	switch (dir) {
		case RESOURCES:
			bundleDirURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
			break;
		case PLUGINS:
			// undeined on iOS!
			bundleDirURL = CFBundleCopyBuiltInPlugInsURL(mainBundle);
			break;
		case BUNDLE:
		default:
			// get the bundle directory istelf by default
			bundleDirURL = CFBundleCopyBundleURL(mainBundle);
			break;
	}
	if (bundleDirURL) {
		CFURLRef absoluteURL = CFURLCopyAbsoluteURL(bundleDirURL);
		CFRelease(bundleDirURL);
		CFStringRef bundleDirPath = CFURLCopyFileSystemPath( absoluteURL, kCFURLPOSIXPathStyle );
		CFRelease(absoluteURL);
		CFStringGetCString( bundleDirPath, outPath, maxLen, kCFStringEncodingASCII );
		CFRelease(bundleDirPath);
	}
}
#endif

/** Returns true if path is an existing directory */
bool dir_exists(const char* path)
{
	struct stat buf;
	buf.st_mode = 0;

	if (stat(path, &buf) < 0) {
		return false;
	}
	if (!S_ISDIR(buf.st_mode)) {
		return false;
	}

	return true;
}

/** Returns true if path is an existing file */
bool file_exists(const char* path)
{
	struct stat buf;
	buf.st_mode = 0;

	if (stat(path, &buf) < 0) {
		return false;
	}
	if (!S_ISREG(buf.st_mode)) {
		return false;
	}

	return true;
}


/**
 * Appends 'name' to path 'target' and returns 'target'.
 * It takes care of inserting PathDelimiter ('/' or '\\') if needed
 */
char* PathAppend (char* target, const char* name)
{
	size_t len = strlen(target);

	if (target[0] != 0 && target[len-1] != PathDelimiter && len+1 < _MAX_PATH) {
		target[len++] = PathDelimiter;
		target[len] = 0;
	}
	// strip possible leading backslash, since it is not ignored on all platforms
	// totl has '\data\zcMHar.bif' in the key file, and also the CaseSensitive
	// code breaks with that extra slash, so simple fix: remove it
	if (name[0] == '\\') {
		name = name+1;
	}
	strncat( target+len, name, _MAX_PATH - len - 1 );

	return target;
}


static bool FindInDir(const char* Dir, char *Filename)
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

	DirectoryIterator dir(Dir);
	if (!dir) {
		return false;
	}

	// Exact match not found, so try to search for Filename
	// with different case
	do {
		const char *name = dir.GetName();
		if (stricmp( name, Filename ) == 0) {
			strcpy( Filename, name );
			return true;
		}
	} while (++dir);
	return false;
}

bool PathJoin (char *target, const char *base, ...)
{
	if (base == NULL) {
		target[0] = '\0';
		return false;
	}
	if (base != target) {
		strcpy(target, base);
	}

	va_list ap;
	va_start(ap, base);

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

static int strmatch(const char *string, const char *mask)
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
	DirectoryIterator dir(Dir);
	if (!dir) {
		return false;
	}

	do {
		const char *name = dir.GetName();
		if (strmatch( name, glob ) == 0) {
			strcpy( target, name );
			return true;
		}
	} while (++dir);
	return false;
}


#ifndef WIN32

void ResolveFilePath(char* FilePath)
{
	char TempFilePath[_MAX_PATH];

	if (FilePath[0]=='~') {
		if (CopyHomePath(TempFilePath, _MAX_PATH)) {
			PathAppend(TempFilePath, FilePath+1);
			strcpy(FilePath, TempFilePath);
			return;
		}
	}

	if (core && !core->CaseSensitive) {
		return;
	}
	strcpy(TempFilePath, FilePath);
	PathJoin(FilePath, TempFilePath[0]==PathDelimiter?SPathDelimiter:"", TempFilePath, NULL);
}

void ResolveFilePath(std::string& FilePath)
{
	char TempFilePath[_MAX_PATH];

	if (FilePath[0]=='~') {
		if (CopyHomePath(TempFilePath, _MAX_PATH)) {
			PathAppend(TempFilePath, FilePath.c_str()+1);
			FilePath = TempFilePath;
			return;
		}
	}

	if (core && !core->CaseSensitive) {
		return;
	}
	PathJoin(TempFilePath, FilePath[0]==PathDelimiter?SPathDelimiter:"", FilePath.c_str(), NULL);
	FilePath = TempFilePath;
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

bool MakeDirectories(const char* path)
{
	char TempFilePath[_MAX_PATH] = "";
	char Tokenized[_MAX_PATH];
	strcpy(Tokenized, path);

	char* Token = strtok(Tokenized, &PathDelimiter);
	while(Token != NULL) {
		if(TempFilePath[0] == 0) {
			if(path[0] == PathDelimiter) {
				TempFilePath[0] = PathDelimiter;
				TempFilePath[1] = 0;
			}
			strcat(TempFilePath, Token);
		} else
			PathJoin(TempFilePath, TempFilePath, Token, NULL);

		if(!MakeDirectory(TempFilePath))
			return false;

		Token = strtok(NULL, &PathDelimiter);
	}
	return true;
}

bool MakeDirectory(const char* path)
{
#ifdef WIN32
#define mkdir(path, mode) _mkdir(path)
#endif
	if (mkdir(path, S_IREAD|S_IWRITE|S_IEXEC) < 0) {
		if (errno != EEXIST) {
			return false;
		}
	}
	// Ignore errors from chmod
	chmod(path, S_IREAD|S_IWRITE|S_IEXEC);
	return true;
#ifdef WIN32
#undef mkdir
#endif
}

GEM_EXPORT char* CopyHomePath(char* outPath, ieWord maxLen)
{
	char* home = getenv( "HOME" );
	if (home) {
		strlcpy(outPath, home, maxLen);
		return outPath;
	}
#ifdef WIN32
	else {
		// if home is null check HOMEDRIVE + HOMEPATH
		char* homedrive = getenv("HOMEDRIVE");
		home = getenv("HOMEPATH");

		if (home) {
			outPath[0] = '\0'; //ensure start string length is 0
			if (homedrive) {
				strlcpy(outPath, homedrive, maxLen);
			}
			PathAppend(outPath, home);
			return outPath;
		}
	}
#endif
	return NULL;
}


DirectoryIterator::DirectoryIterator(const char *path)
	: Directory(NULL), Entry(NULL), Path(path)
{
	Rewind();
}

DirectoryIterator::~DirectoryIterator()
{
	if (Directory)
		closedir(static_cast<DIR*>(Directory));
}

bool DirectoryIterator::IsDirectory()
{
	char dtmp[_MAX_PATH];
	GetFullPath(dtmp);
	//this is needed on windows!!!
	FixPath(dtmp, false);
	return dir_exists(dtmp);
}

char* DirectoryIterator::GetName()
{
	return static_cast<dirent*>(Entry)->d_name;
}

void DirectoryIterator::GetFullPath(char *name)
{
	snprintf(name, _MAX_PATH, "%s%s%s", Path, SPathDelimiter, static_cast<dirent*>(Entry)->d_name);
}

DirectoryIterator& DirectoryIterator::operator++()
{
	Entry = readdir(static_cast<DIR*>(Directory));
	return *this;
}

void DirectoryIterator::Rewind()
{
	if (Directory)
		closedir(static_cast<DIR*>(Directory));
	Directory = opendir(Path);
	if (Directory == NULL)
		Entry = NULL;
	else
		Entry = readdir(static_cast<DIR*>(Directory));
}


}
