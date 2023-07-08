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

#include "config.h"
#include "exports.h"
#include "ie_types.h"
#include "Platform.h"
#include "Predicates.h"

#include <string>
#include <sys/stat.h>

#ifdef WIN32
#include "win32def.h"
#include <direct.h>
#include <io.h>
#endif

namespace GemRB {

using path_t = std::string;

#ifdef WIN32

// prevents warnings of unused `p`
#define ResolveFilePath(p) (void)p

#else  // ! WIN32

#ifdef __APPLE__
// bundle path functions
enum BundleDirectory {
	BUNDLE,
	RESOURCES,
	PLUGINS
};
GEM_EXPORT path_t BundlePath(BundleDirectory dir = BUNDLE);
#endif

/** Handle ~ -> $HOME mapping and do initial case-sensitity check */
GEM_EXPORT void ResolveFilePath(path_t& FilePath);

#endif  // ! WIN32

#ifdef WIN32
const char PathDelimiter = '\\';
const char PathListSeparator = ';';
#else
const char PathDelimiter = '/';
const char PathListSeparator = ':';
#endif
const char SPathDelimiter[] = { PathDelimiter, '\0' };

GEM_EXPORT bool DirExists(const path_t& path);
GEM_EXPORT bool FileExists(const path_t& path);

inline const path_t& to_string(const path_t& path)
{
	return path;
}

// when case sensitivity is enabled dir will be transformed to fit the case of the actual items composing the path
GEM_EXPORT path_t& ResolveCase(path_t& dir);

/**
 * Joins NULL-terminated list of directories and copies it to 'target'.
 *
 * @param[in] ... list of path components to join
 * @return the joined path
 *
 * properly handles the case when paramater contain slashes.
 *
 * Example:
 * path_t path = PathJoin(core->GUIScriptsPath, core->GameType, 'GUIDefines.py');
 */
template<bool FixCase = true, typename... Args>
path_t PathJoin(Args const&... args)
{
	// eh, there is no std::to_string for things that are already strings
	using namespace std; // to use std::to_string for everything else

	path_t result;
	path_t s; // the holder for each of 'args' converted to a string
	// TODO: this could be much cleaner with a fold expression (c++17)
	// this voodoo relies on expanding args which requires a valid context
	// the only way I know how to do this with c++14 is to build an unbounded array
	// the type doesnt matter, nor do the element values
	int IGNORE_UNUSED unpack[] {0, // we need at least one value to form a valid array, so start with one
		// append to 'result'...
		(result +=
		 // if the next argument begins with a delimiter...
		 ((!(s = to_string(args)).empty() && (s[0] == PathDelimiter
			// or the current result ends with a delimiter...
			|| result.empty() || result.back() == PathDelimiter))
		  // don't add another one, otherwise append a delimiter
		  ? "" : SPathDelimiter)
		 // now append the current argument
		 + s
	,0)...}; // the other half of the voodoo is the comma operator to allow us to execute
			 // an expression then return an unrelated value which is another 0
	if (FixCase) {
		ResolveCase(result);
	}
	return result;
}

template<bool FixCase = true, typename DIR_T, typename BASE_T, typename EXT_T>
path_t PathJoinExt(const DIR_T& dir, const BASE_T& base, const EXT_T& ext)
{
	using namespace std;

	path_t path = to_string(dir) + SPathDelimiter + to_string(base);
	if (ext[0] != '\0') {
		path = path + "." + ext;
	}

	if (FixCase) {
		ResolveCase(path);
	}
	return path;
}

GEM_EXPORT void FixPath(path_t& path, bool needslash);

GEM_EXPORT void ExtractFileFromPath(char *file, const char *full_path);

GEM_EXPORT char* PathAppend(char* target, const char* name);
GEM_EXPORT void PathAppend(path_t& target, const path_t& name);

GEM_EXPORT bool MakeDirectories(const path_t& path) WARN_UNUSED;
GEM_EXPORT bool MakeDirectory(const path_t& path) WARN_UNUSED;

GEM_EXPORT path_t HomePath();

// default directory housing GUIScripts/Override/Unhardcoded
GEM_EXPORT path_t GemDataPath();

#ifdef SUPPORTS_MEMSTREAM
void* readonly_mmap(void *fd);
#endif
#ifdef WIN32
void munmap(void *start, size_t);
#endif

class GEM_EXPORT DirectoryIterator {
public:
	enum Flags {
		Files = 1,
		Directories = 2,
		Hidden = 4,
		All = ~0
	};

	using FileFilterPredicate = Predicate<ResRef>;
	/**
	 * @param[in] path Path to directory to search.
	 *
	 * WARNING: the lifetime of path must be longer than the lifetime
	 * of DirectoryIterator.
	 */
	explicit DirectoryIterator(path_t path);
	DirectoryIterator(const DirectoryIterator&) = delete;
	DirectoryIterator(DirectoryIterator&&) noexcept = default;
	~DirectoryIterator();
	DirectoryIterator& operator=(const DirectoryIterator&) = delete;

	void SetFilterPredicate(FileFilterPredicate* p, bool chain = false);
	bool IsDirectory();
	void SetFlags(int flags, bool reset = false);
	/**
	 * Returns name of current entry.
	 *
	 * The returned string is only valid until the iterator is advanced.
	 */
	path_t GetName();
	path_t GetFullPath();
	DirectoryIterator& operator++();
	explicit operator bool () const noexcept { return Entry != nullptr; }
	void Rewind();
private:
	FileFilterPredicate* predicate{};
	void* Directory = nullptr;
	void* Entry = nullptr;
	path_t Path;
	Flags entrySkipFlags;
};

}

#endif  // !VFS_H
