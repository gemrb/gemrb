// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

class Map;

class PlaybackHandle {
public:
	PlaybackHandle() = default;
	PlaybackHandle(Holder<SoundSourceHandle> source, time_t length, int32_t height = 0);

	time_t GetLengthMs() const;
	const Holder<SoundSourceHandle>& GetSourceHandle() const;
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

	Holder<PlaybackHandle> Play(StringView resource, AudioPreset preset, SFXChannel channel, const Point& point, const Map* map = nullptr);
	Holder<PlaybackHandle> Play(StringView resource, AudioPreset preset, SFXChannel channel);
	Holder<PlaybackHandle> Play(StringView resource, const AudioPlaybackConfig& config, const Map* map = nullptr);
	Holder<PlaybackHandle> PlayDirectional(StringView resource, SFXChannel channel, const Point& point, orient_t orientation, const Map* map = nullptr);
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
