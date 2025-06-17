/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
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
 */

#ifndef H_AUDIO_BUFFER_CACHE
#define H_AUDIO_BUFFER_CACHE

#include "AudioBackend.h"
#include "LRUCache.h"

namespace GemRB {

struct BufferCacheEntry {
	Holder<SoundBufferHandle> handle;
	time_t length = 0;

	BufferCacheEntry() = default;
	explicit BufferCacheEntry(Holder<SoundBufferHandle> handle, time_t length)
		: handle(std::move(handle)), length(length)
	{}

	void evictionNotice() const { /* No need. */ }
};

struct BufferInUse {
	bool operator()(BufferCacheEntry& entry) const
	{
		return entry.handle->Disposable();
	}
};

using AudioBufferCache = LRUCache<BufferCacheEntry, BufferInUse>;

}

#endif
