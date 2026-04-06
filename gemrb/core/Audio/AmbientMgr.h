// SPDX-FileCopyrightText: 2004 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef AMBIENTMGR_H
#define AMBIENTMGR_H

#include "exports.h"
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
class Map;

class GEM_EXPORT AmbientMgr {
public:
	AmbientMgr();
	~AmbientMgr();

	void Reset();
	void RemoveAmbients(const std::vector<Ambient*>& oldAmbients);
	void SetAmbients(const std::vector<Ambient*>& a, const Map*);

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
	const Map* currentMap = nullptr;

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

		tick_t Tick(tick_t ticks, Point listener, ieDword timeslice, const Map*);
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

		BufferCacheEntry GetBuffer(ResRef resource, const AudioPlaybackConfig& config) const;
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
