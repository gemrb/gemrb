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

#include "Strings/UTF8Comparison.h"
#include "System/VFS.h"

#include "globals.h"

#include "Interface.h"
#include "Logging/Logging.h"

#include <cctype>
#include <locale>
#include <cstdarg>
#include <cstring>
#include <cerrno>

#ifdef WIN32
// that's a workaround to live with `NOUSER` in `win32def.h`
#define LPMSG void*
#include <shlwapi.h>
// there is a macro in shlwapi.h that turns `PathAppendW` into `PathAppend`
#undef PathAppend
#else
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

#include <array>

using namespace GemRB;

struct dirent {
	dirent() : buffer(_MAX_PATH, '\0'), d_name(const_cast<char*>(buffer.data())) {}

	std::string buffer;
	char *d_name;

	dirent& operator=(std::string&& entryName) {
		auto cutOff = entryName.length();
		buffer = std::move(entryName);
		buffer.resize(_MAX_PATH);
		buffer[cutOff] = 0;
		d_name = const_cast<char*>(buffer.data());

		return *this;
	}
};

struct DIR {
	DIR() : path(), is_first(true), hFile(0) {}
	~DIR() {
		_findclose(hFile);
	}

	std::wstring path;
	bool is_first;
	struct _wfinddata_t c_file;
	intptr_t hFile;
	dirent entry;
};

static DIR* opendir(const char* filename) {
	auto dir = new DIR{};

	// consider $PATH\\*.*\0 for _wfindfirst
	if (strlen(filename) > _MAX_PATH - 5) {
		return nullptr;
	}

	auto buffer = StringFromUtf8(filename);
	buffer.resize(_MAX_PATH);
	dir->path = fmt::format(L"{}\\*.*", reinterpret_cast<const wchar_t*>(buffer.c_str()));

	return dir;
}

static dirent* readdir(DIR *dir) {
	struct _wfinddata_t c_file;

	if (dir->is_first) {
		dir->is_first = false;
		dir->hFile = _wfindfirst(dir->path.data(), &c_file);
		if (dir->hFile == -1L)
			return nullptr;
	} else {
		if (_wfindnext(dir->hFile, &c_file) != 0) {
			return nullptr;
		}
	}

	char16_t *c = reinterpret_cast<char16_t*>(c_file.name);
	size_t n = 0;
	for (; n < _MAX_PATH && *c != u'\0'; ++c, ++n) {}

	dir->entry = RecodedStringFromWideStringBytes(reinterpret_cast<char16_t*>(c_file.name), n * 2, "UTF-8");

	return &dir->entry;
}

static void closedir(DIR *dir) {
	delete dir;
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
static bool DirExists(StringView path)
{
	// 'path' may be in the middle of a string
	// so we cheat to avoid copying, we will change it back
	char term = '\0';
	char* end = const_cast<char*>(path.end());
	std::swap(*end, term); // use swap because end may be a delimiter or a terminator

#ifdef WIN32
	auto buffer = StringFromUtf8(path.c_str());
	auto wideChars = reinterpret_cast<const wchar_t*>(buffer.c_str());

	// `stat` does not work reliably with non-ASCII names
	if (!PathFileExists(wideChars)) {
		return false;
	}

	bool ret = (GetFileAttributes(wideChars) & FILE_ATTRIBUTE_DIRECTORY) > 0;
#else
	struct stat buf;
	buf.st_mode = 0;
	bool ret = stat(path.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode);
	std::swap(*end, term);
#endif

	return ret;
}

bool DirExists(const path_t& path)
{
	return DirExists(StringView(path));
}

/** Returns true if path is an existing file */
bool FileExists(const path_t& path)
{
#ifdef WIN32
	auto buffer = StringFromUtf8(path.c_str());
	auto wideChars = reinterpret_cast<const wchar_t*>(buffer.c_str());

	// `stat` does not work reliably with non-ASCII names
	if (!PathFileExists(wideChars)) {
		return false;
	}

	if (GetFileAttributes(wideChars) & FILE_ATTRIBUTE_DIRECTORY) {
		return false;
	}
#else
	struct stat buf;
	buf.st_mode = 0;

	if (stat(path.c_str(), &buf) < 0) {
		return false;
	}
	if (!S_ISREG(buf.st_mode)) {
		return false;
	}
#endif

	return true;
}

void PathAppend(path_t& target, const path_t& name)
{
	if (name.empty()) {
		return;
	}

	auto scope = StringView{name.c_str(), name.length()};
	// TotL has '\data\zcMHar.bif' in the key file
	if (name[0] == '\\') {
		scope = StringView{name.c_str() + 1, name.length() - 1};
	}

	if (!target.empty() && target.back() != PathDelimiter && *scope.begin() != PathDelimiter) {
		target.push_back(PathDelimiter);
	}

	target.append(scope.begin(), scope.end());
}

static bool FindMatchInDir(const char* dir, MutableStringView item)
{
	// this is specifically designed over a UTF-8 default, so most things except Win
	bool multibyteCheck =
		std::any_of(item.begin(), item.end(), [](char c) { return c < 0; });

	for (DirectoryIterator dirit(dir); dirit; ++dirit) {
		const path_t& name = dirit.GetName();
		bool equal =
			multibyteCheck
				? UTF8_stricmp(name.c_str(), item.c_str())
				: stricmp(name.c_str(), item.c_str()) == 0;

		if (equal) {
			std::copy(name.begin(), name.end(), item.begin());
			return true;
		}
	}

	return false;
}

static void ResolveCase(MutableStringView path, size_t itempos)
{
	if (!DirExists(StringView(path.c_str(), itempos))) {
		return;
	}

	size_t next = FindFirstOf(path, SPathDelimiter, itempos);
	if (next == MutableStringView::npos) {
		next = path.length();
	}

	char* nextDelim = &path[next];
	*nextDelim = '\0';

	bool found = access(path.c_str(), F_OK) == 0;
	if (!found) {
		// Exact match not found, so try to search for Filename
		// with different case
		char* curDelim = &path[itempos - 1];
		*curDelim = '\0';

		found = FindMatchInDir(path.c_str(), MutableStringView{path.c_str(), itempos, path.length()});
		*curDelim = PathDelimiter;
	}

	if (next < path.length()) {
		*nextDelim = PathDelimiter;
		if (found) return ResolveCase(path, next + 1);
	}
}

path_t& ResolveCase(path_t& filePath)
{
	// TODO: we should make this a build time option, it applies to mac and numerous other platforms too
#ifdef WIN32 // windows is case insensitive by defult
	if (!core || !core->config.CaseSensitive) {
#else
	if (core && !core->config.CaseSensitive) {
#endif
		return filePath;
	}

	// First test if there's a Filename with exactly same name
	// and if yes, return it and do not search in the Dir
	if (!access(filePath.c_str(), F_OK)) {
		return filePath;
	}

	size_t nextItem = filePath.find_first_of(PathDelimiter, 1);
	if (nextItem != path_t::npos) {
		MutableStringView msv(filePath);
		ResolveCase(msv, nextItem);
	} else if (!DirExists(filePath)) { // filePath is a single component
		FindMatchInDir(".", MutableStringView{filePath});
	}

	return filePath;
}

path_t& FixPath(path_t& path)
{
	if (path.empty()) {
		return path;
	}

	size_t count = 0;
	size_t last = path.find_first_of(PathDelimiter, 0);
	size_t cur = 0;
	while ((cur = path.find_first_of(PathDelimiter, last + 1)) != path_t::npos) {
		if (cur - last == 1) {
			// two or more neighboring delimiters
			size_t next = path.find_first_not_of(PathDelimiter, last);
			if (next == path_t::npos) {
				// truncate the rest
				path.resize(last);
				break;
			}
			std::copy(path.begin() + next, path.end(), &path[cur]);
			count += next - last - 1;
		} else {
			last = cur;
		}
	}
	
	if (count) {
		path.erase(path.length() - count);
	}

	if (path.back() == PathDelimiter) {
		path.pop_back();
	}

	return ResolveCase(path);
}

path_t& ResolveFilePath(path_t& filePath)
{
#ifndef WIN32
	if (filePath[0] == '~') {
		path_t home = HomePath();
		if (home.length()) {
			PathAppend(home, filePath.substr(1));
			filePath.swap(home);
		}
	}
#endif

	return FixPath(filePath);
}

path_t ExtractFileFromPath(const path_t& fullPath)
{
	size_t pos = fullPath.find_last_of(PathDelimiter);
	if (pos != path_t::npos)
		return fullPath.substr(pos + 1);
	else if ((pos = fullPath.find_last_of(PathListSeparator)) != path_t::npos)
		return fullPath.substr(pos + 1);
	else
		return fullPath;
}

static bool MakeDirectory(StringView path)
{
	// 'path' may be in the middle of a string
	// so we cheat to avoid copying, we will change it back
	char term = '\0';
	char* end = const_cast<char*>(path.end());
	std::swap(*end, term); // use swap because end may be a delimiter or a terminator
#ifdef WIN32
#define mkdir(path, mode) _mkdir(path)
#endif
	bool ret = mkdir(path.c_str(), S_IRWXU) == 0 || errno == EEXIST;
	std::swap(*end, term);
	return ret;
#ifdef WIN32
#undef mkdir
#endif
}

bool MakeDirectory(const path_t& path)
{
	return MakeDirectory(StringView(path));
}

bool MakeDirectories(const path_t& path)
{
	auto parts = Explode<path_t, StringView>(path, PathDelimiter);
	const char* begin = path.data();

	for (const auto& part : parts) {
		if (part.empty()) continue;

		const char* end = part.begin() + part.length();
		if(!MakeDirectory(StringView(begin, end - begin))) {
			return false;
		}
	}

	return true;
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

DirectoryIterator::DirectoryIterator(path_t path)
: Path(std::move(FixPath(path)))
{
	SetFlags(Files|Directories);
	Rewind();
}

DirectoryIterator::~DirectoryIterator()
{
	if (Directory)
		closedir(static_cast<DIR*>(Directory));
}

void DirectoryIterator::SetFlags(int flags, bool reset)
{
	// store the inverse
	entrySkipFlags = Flags(~flags);
	if (reset) Rewind();
}

void DirectoryIterator::SetFilterPredicate(FileFilterPredicate p, bool chain)
{
	if (chain && predicate) {
		predicate = std::make_shared<AndPredicate<path_t>>(predicate, p);
	} else {
		predicate = p;
	}
	Rewind();
}

bool DirectoryIterator::IsDirectory()
{
	return DirExists(GetFullPath());
}

path_t DirectoryIterator::GetName()
{
	if (Entry == nullptr) return "";
	return static_cast<dirent*>(Entry)->d_name;
}

path_t DirectoryIterator::GetFullPath()
{
	if (Entry) {
		return PathJoin<false>(Path, static_cast<dirent*>(Entry)->d_name);
	} else {
		return Path;
	}
}

DirectoryIterator& DirectoryIterator::operator++()
{
	bool cont = false;
	do {
		errno = 0;
		Entry = readdir(static_cast<DIR*>(Directory));
		cont = false;
		if (Entry) {
			const path_t& name = GetName();

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
				cont = !predicate->operator()(name);
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
	Directory = opendir(Path.c_str());
	if (Directory == NULL) {
		Entry = NULL;
		//Log(WARNING, "DirectoryIterator", "Cannot open directory: {}\nError: {}", Path, strerror(errno));
	} else {
		this->operator++();
	}
}

}
