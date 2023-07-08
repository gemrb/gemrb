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
#include "Logging/Logging.h"

// in case unistd isn't there or nonconformant
#ifndef R_OK
#define R_OK 04
#endif

#include <cstdarg>
#include <cstring>
#include <cerrno>

#ifndef WIN32
#include <dirent.h>
#endif

#ifdef HAVE_MMAP
#include <sys/mman.h>
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

#include <tchar.h>

using namespace GemRB;

struct DIR {
	TCHAR path[_MAX_PATH];
	bool is_first;
	struct _finddata_t c_file;
	intptr_t hFile;
};

struct dirent {
	char d_name[_MAX_PATH];
};


// buffer which readdir returns
static dirent de;

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

static DIR* opendir(const char* filename)
{
	TCHAR t_filename[_MAX_PATH] = {0};
	DIR* dirp = ( DIR* ) malloc( sizeof( DIR ) );
	dirp->is_first = 1;

	mbstowcs(t_filename, filename, _MAX_PATH - 1);
	StringCbPrintf(dirp->path, _MAX_PATH * sizeof(TCHAR), TEXT("%s%s*.*"), t_filename, TEXT("\\"));

	return dirp;
}

static dirent* readdir(DIR* dirp)
{
	struct _tfinddata_t c_file;

	if (dirp->is_first) {
		dirp->is_first = 0;
		dirp->hFile = _tfindfirst(dirp->path, &c_file);
		if (dirp->hFile == -1L)
			return NULL;
	} else {
		if (_tfindnext(dirp->hFile, &c_file) != 0) {
			return NULL;
		}
	}

	TCHAR td_name[_MAX_PATH];
	StringCbCopy(td_name, _MAX_PATH, c_file.name);
	wcstombs(de.d_name, td_name, _MAX_PATH);

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
path_t BundlePath(BundleDirectory dir)
{
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef bundleDirURL = NULL;
	switch (dir) {
		case RESOURCES:
			bundleDirURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
			break;
		case PLUGINS:
			// undefined on iOS!
			bundleDirURL = CFBundleCopyBuiltInPlugInsURL(mainBundle);
			break;
		case BUNDLE:
		default:
			// get the bundle directory itself by default
			bundleDirURL = CFBundleCopyBundleURL(mainBundle);
			break;
	}
	
	path_t outPath;
	if (bundleDirURL) {
		CFURLRef absoluteURL = CFURLCopyAbsoluteURL(bundleDirURL);
		CFRelease(bundleDirURL);
		CFStringRef bundleDirPath = CFURLCopyFileSystemPath( absoluteURL, kCFURLPOSIXPathStyle );
		CFRelease(absoluteURL);
		outPath = CFStringGetCStringPtr(bundleDirPath, kCFStringEncodingUTF8);
		CFRelease(bundleDirPath);
	}
	return outPath;
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

void PathAppend(path_t& target, const char* name)
{
	size_t len = target.length();

	if (target[0] != 0 && target[len-1] != PathDelimiter && len+1 < _MAX_PATH) {
		target.push_back(PathDelimiter);
	}
	// strip possible leading backslash, since it is not ignored on all platforms
	// totl has '\data\zcMHar.bif' in the key file, and also the CaseSensitive
	// code breaks with that extra slash, so simple fix: remove it
	if (name[0] == '\\') {
		name = name+1;
	}
	target += name;
}

static bool FindInDir(const char* Dir, char *Filename)
{
	// First test if there's a Filename with exactly same name
	// and if yes, return it and do not search in the Dir
	char TempFilePath[_MAX_PATH];
	assert(strnlen(Dir, _MAX_PATH/2) < _MAX_PATH/2);
	strcpy(TempFilePath, Dir);
	PathAppend( TempFilePath, Filename );

	if (!access( TempFilePath, R_OK )) {
		return true;
	}

	if (core && !core->config.CaseSensitive) {
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
				strlcpy(filename, source, _MAX_PATH/4);
			}
			if (!FindInDir(target, filename)) {
				PathAppend(target, source);
				goto finish;
			}
			PathAppend(target, filename);
			if (slash) source = slash + 1;
		} while (slash);
	}

	va_end( ap );
	return true;

finish:
	while (const char *source = va_arg(ap, char*)) {
		PathAppend(target, source);
	}
	va_end( ap );
	return false;
}

bool PathJoinExt (char* target, const char* dir, const char* base, const char* ext)
{
	char file[_MAX_PATH];
	assert(strnlen(ext, 5) < 5);
	if (strlcpy(file, base, _MAX_PATH-5) >= _MAX_PATH-5) {
		Log(ERROR, "VFS", "Too long base path: {}!", base);
		return false;
	}
	strcat(file, ".");
	strcat(file, ext);
	return PathJoin(target, dir, file, nullptr);
}

/** Fixes path delimiter character (slash).
 * needslash = true : we add a slash
 * needslash = false: we remove the slash
 */
void FixPath(path_t& path, bool needslash)
{
	size_t i = path.length();

	if (needslash) {
		if (i && path[i - 1] == PathDelimiter) return;
		path.push_back(PathDelimiter);
	} else if (i) {
		if (path[i - 1] != PathDelimiter) return;
		path.pop_back();
	}
}

#ifndef WIN32

void ResolveFilePath(char* FilePath)
{
	if (FilePath[0]=='~') {
		path_t home = HomePath();
		if (home.length()) {
			PathAppend(home, FilePath+1);
			strcpy(FilePath, home.c_str());
			return;
		}
	}

	if (core && !core->config.CaseSensitive) {
		return;
	}
	char TempFilePath[_MAX_PATH];
	if (strlcpy(TempFilePath, FilePath, _MAX_PATH-1) >= _MAX_PATH-1) {
		Log(ERROR, "VFS", "Too long path to resolve: {}!", FilePath);
		return;
	}
	PathJoin(FilePath, TempFilePath[0] == PathDelimiter ? SPathDelimiter : "", TempFilePath, nullptr);
}

void ResolveFilePath(std::string& FilePath)
{
	if (FilePath[0]=='~') {
		path_t home = HomePath();
		if (home.length()) {
			PathAppend(home, FilePath.c_str()+1);
			FilePath = home;
			return;
		}
	}

	if (core && !core->config.CaseSensitive) {
		return;
	}

	char TempFilePath[_MAX_PATH];
	PathJoin(TempFilePath, FilePath[0] == PathDelimiter ? SPathDelimiter : "", FilePath.c_str(), nullptr);
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
	assert(strnlen(path, _MAX_PATH/2) < _MAX_PATH/2);
	strcpy(Tokenized, path);

	char* Token = strtok(Tokenized, SPathDelimiter);
	while(Token != NULL) {
		if(TempFilePath[0] == 0) {
			if(path[0] == PathDelimiter) {
				TempFilePath[0] = PathDelimiter;
				TempFilePath[1] = 0;
			}
			assert(strnlen(Token, _MAX_PATH/2) < _MAX_PATH/2);
			strcat(TempFilePath, Token);
		} else
			PathJoin(TempFilePath, TempFilePath, Token, nullptr);

		if(!MakeDirectory(TempFilePath))
			return false;

		Token = strtok(NULL, SPathDelimiter);
	}
	return true;
}

bool MakeDirectory(const char* path)
{
#ifdef WIN32
#define mkdir(path, mode) _mkdir(path)
#endif
	if (mkdir(path, S_IRWXU) < 0) {
		if (errno != EEXIST) {
			return false;
		}
	}
	return true;
#ifdef WIN32
#undef mkdir
#endif
}

GEM_EXPORT path_t HomePath()
{
	const char* home = getenv("HOME");
	if (home) {
		return home;
	}
#ifdef WIN32
	else {
		// if home is null check HOMEDRIVE + HOMEPATH
		char* homedrive = getenv("HOMEDRIVE");
		home = getenv("HOMEPATH");

		if (home) {
			path_t outPath;
			if (homedrive) {
				outPath = homedrive;
			}
			PathAppend(outPath, home);
			return outPath;
		}
	}
#endif
	return "";
}

path_t GemDataPath()
{
	// check env var; used by the Android wrapper
#ifdef HAVE_SETENV
	const char* dataDir = getenv("GEMRB_DATA");
	if (dataDir) {
		return dataDir;
	}
#endif

	path_t outPath;
	// apple bundle, build time supplied directory or home and then cwd fallback
#ifdef __APPLE__
	outPath = BundlePath(RESOURCES);
#elif defined(DATA_DIR)
	outPath = DATA_DIR;
#else
	outPath = HomePath();
	if (!outPath.empty()) {
		PathAppend(outPath, PACKAGE);
	} else {
		outPath = path_t(".") + SPathDelimiter;
	}
#endif

	return outPath;
}

#ifdef WIN32

void* readonly_mmap(void *fd) {
	HANDLE mappingHandle =
		CreateFileMapping(
			static_cast<HANDLE>(fd),
			nullptr,
			PAGE_READONLY,
			0,
			0,
			nullptr
		);

	if (mappingHandle == nullptr) {
		return nullptr;
	}

	void *start = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, 0);
	CloseHandle(mappingHandle);

	return start;
}

void munmap(void *start, size_t) {
	UnmapViewOfFile(start);
}

#elif defined(HAVE_MMAP)

void* readonly_mmap(void *vfd) {
	int fd = fileno(static_cast<FILE*>(vfd));
	struct stat statData;
	int ret = fstat(fd, &statData);
	assert(ret != -1);

	return mmap(nullptr, statData.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
}

#endif

DirectoryIterator::DirectoryIterator(const char *path)
{
	SetFlags(Files|Directories);
	Path = strdup(path);
	Rewind();
}

DirectoryIterator::~DirectoryIterator()
{
	if (Directory)
		closedir(static_cast<DIR*>(Directory));
	free(Path);
	delete predicate;
}

void DirectoryIterator::SetFlags(int flags, bool reset)
{
	// store the inverse
	entrySkipFlags = Flags(~flags);
	if (reset) Rewind();
}

void DirectoryIterator::SetFilterPredicate(FileFilterPredicate* p, bool chain)
{
	if (chain && predicate) {
		predicate = new AndPredicate<ResRef>(predicate, p);
	} else {
		delete predicate;
		predicate = p;
	}
	Rewind();
}

bool DirectoryIterator::IsDirectory()
{
	path_t path = GetFullPath();
	//this is needed on windows!!!
	FixPath(path, false);
	return dir_exists(path.c_str());
}

const char* DirectoryIterator::GetName()
{
	if (Entry == NULL) return NULL;
	return static_cast<dirent*>(Entry)->d_name;
}

path_t DirectoryIterator::GetFullPath()
{
	return fmt::format("{}{}{}", Path, SPathDelimiter, static_cast<dirent*>(Entry)->d_name);
}

DirectoryIterator& DirectoryIterator::operator++()
{
	bool cont = false;
	do {
		errno = 0;
		Entry = readdir(static_cast<DIR*>(Directory));
		cont = false;
		if (Entry) {
			const char* name = GetName();

			if (entrySkipFlags&Directories) {
				cont = IsDirectory();
			}
			if (cont == false && entrySkipFlags&Files) {
				cont = !IsDirectory();
			}
			if (cont == false && entrySkipFlags&Hidden) {
				cont = name[0] == '.';
			}
			if (cont == false && predicate) {
				size_t len = strnlen(name, _MAX_PATH);
				const char* nameEnd = name; // make sure to take the end of the name, since we'll likely truncate it
				while (--len >= 8U) {
					nameEnd++;
				}
				cont = !(*predicate)(ResRef(nameEnd));
			}
		} else if (errno) {
			//Log(WARNING, "DirectoryIterator", "Cannot readdir: {}\nError: {}", Path, strerror(errno));
		}
	} while (cont);

	return *this;
}

void DirectoryIterator::Rewind()
{
	if (Directory)
		closedir(static_cast<DIR*>(Directory));
	Directory = opendir(Path);
	if (Directory == NULL) {
		Entry = NULL;
		//Log(WARNING, "DirectoryIterator", "Cannot open directory: {}\nError: {}", Path, strerror(errno));
	} else {
		this->operator++();
	}
}

}
