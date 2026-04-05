// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
