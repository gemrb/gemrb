// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AmigaHelpers.h"

#include "../../gemrb/core/Streams/File.h"

#include <cstdlib>

namespace GemRB {

/*
 * AmigaOS has no mmap(), so load the complete file into
 * an allocated memory block.
 */
bool AmigaMMap(void* fileHandle, strpos_t size, char*& data)
{
	const size_t allocationSize = size != 0 ? static_cast<size_t>(size) : 1;
	data = static_cast<char*>(std::malloc(allocationSize));
	if (!data) {
		return false;
	}

	bool fileMapped = false;
	if (size == 0) {
		fileMapped = true;
	} else {
		FILE* file = static_cast<FILE*>(fileHandle);
		const size_t bytesRead = fread(data, 1, allocationSize, file);
		fileMapped = bytesRead == allocationSize;
	}

	if (!fileMapped) {
		std::free(data);
		data = nullptr;
	}
	return fileMapped;
}

path_t& AmigaFixPath(path_t& path)
{
	if (path.empty()) {
		return path;
	}

	/*
	 * The native representation of the current directory is an empty
	 * path. "." and "./name" are not reliable with the Amiga runtime.
	 */
	if (path == ".") {
		path.clear();
		return path;
	}

	while (path.length() >= 2 &&
	       path[0] == '.' &&
	       path[1] == PathDelimiter) {
		path.erase(0, 2);
	}

	/*
	 * Normalize VOLUME:/path to VOLUME:path.
	 */
	const size_t colon = path.find(':');
	if (colon != path_t::npos &&
	    colon + 1 < path.length() &&
	    path[colon + 1] == PathDelimiter) {
		path.erase(colon + 1, 1);
	}

	// Collapse repeated separators.
	size_t pos = 0;
	while ((pos = path.find("//", pos)) != path_t::npos) {
		path.erase(pos, 1);
	}

	if (!path.empty() && path.back() == PathDelimiter) {
		path.pop_back();
	}

	if (path.empty()) {
		return path;
	}

	ResolveCase(path);

	return path;
}

void AmigaPathAppend(path_t& target, const path_t& name)
{
	if (name.empty()) {
		return;
	}

	/*
	 * On AmigaOS, an empty path denotes the current directory.
	 * "." must therefore not produce paths such as "./chitin.key".
	 */
	if (target == ".") {
		target.clear();
	}

	auto nameBegin = name.begin();
	// Strip a leading DOS-style backslash.
	if (nameBegin != name.end() && *nameBegin == '\\') {
		++nameBegin;
	}

	/*
	 * Convert an incoming "./name" component into "name".
	 */
	if (nameBegin != name.end() &&
	    *nameBegin == '.' &&
	    nameBegin + 1 != name.end() &&
	    *(nameBegin + 1) == PathDelimiter) {
		nameBegin += 2;
	}

	/*
	 * AmigaOS volume paths use "VOLUME:path", not "VOLUME:/path".
	 */
	if (!target.empty() &&
	    target.back() == ':' &&
	    nameBegin != name.end() &&
	    *nameBegin == PathDelimiter) {
		++nameBegin;
	}

	if (nameBegin == name.end()) {
		return;
	}

	const bool addDelimiter =
		!target.empty() &&
		target.back() != PathDelimiter &&
		target.back() != ':' &&
		*nameBegin != PathDelimiter;

	if (addDelimiter) {
		target.push_back(PathDelimiter);
	}

	target.append(nameBegin, name.end());
}
};
