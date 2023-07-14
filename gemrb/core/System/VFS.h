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
GEM_EXPORT bool PathJoin (char* target, const char* base, ...) SENTINEL;
GEM_EXPORT bool PathJoinExt (char* target, const char* dir, const char* file, const char* ext = NULL);
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
