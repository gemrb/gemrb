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

#ifndef H_AUDIO_PLAYBACK
#define H_AUDIO_PLAYBACK

#include "AudioBackend.h"
#include "AudioSettings.h"
#include "BufferCache.h"

#include <set>
#include <vector>

// still in use by Python
static constexpr uint8_t GEM_SND_SPEECH = 4;

namespace GemRB {

class PlaybackHandle {
public:
	PlaybackHandle() = default;
	PlaybackHandle(Holder<SoundSourceHandle> source, time_t length, int32_t height = 0);

	time_t GetLengthMs() const;
	bool IsPlaying() const;
	void SetPosition(const Point& p);
	void Stop();
	void StopLooping();

private:
	Holder<SoundSourceHandle> source;
	time_t length = 0;
	int32_t height = 0;
};

class GEM_EXPORT AudioPlayback {
public:
	explicit AudioPlayback(const std::vector<ResRef>& defaultSounds);

	Holder<PlaybackHandle> Play(StringView resource, AudioPreset preset, SFXChannel channel, const Point& point);
	Holder<PlaybackHandle> Play(StringView resource, AudioPreset preset, SFXChannel channel);
	Holder<PlaybackHandle> Play(StringView resource, const AudioPlaybackConfig& config);
	Holder<PlaybackHandle> PlayDefaultSound(size_t index, SFXChannel channel);
	Holder<PlaybackHandle> PlayDefaultSound(size_t index, const AudioPlaybackConfig& config);
	time_t PlaySpeech(StringView resource, const AudioPlaybackConfig& config, bool interrupt = true);
	void StopSpeech();

private:
	AudioBufferCache bufferCache { 40 };
	Holder<SoundSourceHandle> speech;
	std::set<Holder<SoundSourceHandle>> activeSources;

	const std::vector<ResRef>& defaultSounds;

	void Housekeeping();
	BufferCacheEntry GetBuffer(StringView resource, const AudioPlaybackConfig& config);
};

}

#endif
