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

#ifndef H_AUDIO_BACKEND
#define H_AUDIO_BACKEND

#include "globals.h"

#include "Holder.h"
#include "MapReverb.h"
#include "PlaybackConfig.h"
#include "Plugin.h"
#include "Region.h"
#include "SoundMgr.h"

namespace GemRB {

// for SDL, and per channel; don't increase too much since it will make the volume slider delay
static constexpr size_t STREAM_QUEUE_MIN_SIZE = 16384;

struct GEM_EXPORT AudioBufferFormat {
	uint8_t bits = 0;
	uint8_t channels = 0;
	uint16_t sampleRate = 0;

	size_t GetNumBytesForMs(uint32_t ms) const
	{
		return (sampleRate * channels * (bits / 8) * ms) / 1000;
	}

	bool operator==(const AudioBufferFormat& other) const
	{
		return bits == other.bits && channels == other.channels && sampleRate == other.sampleRate;
	}
};

class GEM_EXPORT SoundBufferHandle {
public:
	virtual ~SoundBufferHandle() = default;

	virtual bool Disposable() = 0;
};

class GEM_EXPORT SoundSourceHandle {
public:
	virtual ~SoundSourceHandle() = default;

	virtual bool Enqueue(Holder<SoundBufferHandle> buffer) = 0;
	virtual bool HasFinishedPlaying() const = 0;
	virtual void Reconfigure(const AudioPlaybackConfig& config) = 0;
	virtual void Stop() = 0;
	virtual void StopLooping() = 0;
	virtual void SetPitch(int pitch) = 0;
	virtual void SetPosition(const AudioPoint&) = 0;
	virtual void SetVolume(int volume) = 0;
};

class GEM_EXPORT SoundStreamSourceHandle {
public:
	virtual ~SoundStreamSourceHandle() = default;

	virtual bool Feed(const AudioBufferFormat& format, const char* memory, size_t size) = 0;
	virtual bool HasProcessed() = 0;
	virtual void Pause() = 0;
	virtual void Reclaim() { /* for SDLAudio */ }
	virtual void Resume() = 0;
	virtual void Stop() = 0;

	virtual void SetVolume(int volume) = 0;
};

class GEM_EXPORT AudioBackend : public Plugin {
public:
	static const TypeID ID;
	enum class StreamMode { POLLING,
				WAITING };

	virtual bool Init() = 0;

	virtual Holder<SoundSourceHandle> CreatePlaybackSource(const AudioPlaybackConfig& config, bool priority = false) = 0;
	virtual Holder<SoundStreamSourceHandle> CreateStreamable(
		const AudioPlaybackConfig& config,
		size_t minQueueSize = STREAM_QUEUE_MIN_SIZE) = 0;
	virtual Holder<SoundBufferHandle> LoadSound(ResourceHolder<SoundMgr> resource, const AudioPlaybackConfig& config) = 0;

	virtual const AudioPoint& GetListenerPosition() const = 0;
	virtual void SetListenerPosition(const AudioPoint&) = 0;
	virtual void SetReverbProperties(const MapReverbProperties&) { /* Goodie feature per backend */ };

	virtual StreamMode GetStreamMode() const = 0;
};

}

#endif
