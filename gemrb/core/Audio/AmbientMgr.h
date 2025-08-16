/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef AMBIENTMGR_H
#define AMBIENTMGR_H

#include "exports-core.h"
#include "globals.h"

#include "AudioBackend.h"
#include "BufferCache.h"
#include "Region.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace GemRB {

class Ambient;

class GEM_EXPORT AmbientMgr {
public:
	AmbientMgr();
	~AmbientMgr();

	void Reset();
	void RemoveAmbients(const std::vector<Ambient*>& oldAmbients);
	void SetAmbients(const std::vector<Ambient*>& a);

	void UpdateVolume();
	void SetVolume(unsigned short value);

	void Activate(StringView name); // hard play ;-)
	void Activate();
	void Deactivate(StringView name); // hard stop
	void Deactivate();
	bool IsActive(StringView name) const;

private:
	void AmbientsSet(const std::vector<Ambient*>&);

private:
	mutable std::mutex ambientsMutex;
	std::vector<Ambient*> ambients;
	std::atomic_bool active { false };

	mutable std::recursive_mutex mutex;
	std::thread player;
	std::condition_variable_any cond;
	std::atomic_bool playing { true };

	class AmbientSource {
	public:
		explicit AmbientSource(const Ambient* a, AudioBufferCache& cache) noexcept
			: ambient(a), bufferCache(cache) {};
		AmbientSource(const AmbientSource&) = delete;
		AmbientSource(AmbientSource&&) = default;
		AmbientSource& operator=(const AmbientSource&) = delete;
		AmbientSource& operator=(AmbientSource&&) = default;

		tick_t Tick(tick_t ticks, Point listener, ieDword timeslice);
		void HardStop();
		void SetVolume(unsigned short volume);
		const Ambient* GetAmbient() const { return ambient; };

	private:
		const Ambient* ambient;
		std::reference_wrapper<AudioBufferCache> bufferCache;

		Holder<SoundSourceHandle> source;
		BufferCacheEntry bufferCacheEntry;
		tick_t lastticks = 0;
		tick_t nextdelay = 0;
		size_t nextref = 0;
		unsigned int gain = 0;

		BufferCacheEntry GetBuffer(ResRef resource, const AudioPlaybackConfig& config);
		bool IsHeard(const Point& listener) const;
		tick_t Enqueue(const AudioPlaybackConfig& config);
	};

	AudioBufferCache bufferCache { 60 };
	std::vector<AmbientSource> ambientSources;

	int Play();
	tick_t Tick(tick_t ticks);
	void HardStop();
};

}

#endif
