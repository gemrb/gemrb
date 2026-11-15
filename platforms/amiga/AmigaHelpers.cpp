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

};
