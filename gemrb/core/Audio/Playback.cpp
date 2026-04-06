// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Playback.h"

#include "Interface.h"

namespace GemRB {

PlaybackHandle::PlaybackHandle(Holder<SoundSourceHandle> source, time_t length, int32_t height)
	: source(std::move(source)), length(length), height(height)
{}

time_t PlaybackHandle::GetLengthMs() const
{
	return length;
}

const Holder<SoundSourceHandle>& PlaybackHandle::GetSourceHandle() const
{
	return source;
}

bool PlaybackHandle::IsPlaying() const
{
	if (source) {
		return !source->HasFinishedPlaying();
	}

	return false;
}

void PlaybackHandle::SetPosition(const Point& p)
{
	if (source) {
		source->SetPosition({ p, height });
	}
}

void PlaybackHandle::StopLooping()
{
	if (source) {
		source->StopLooping();
	}
}

void PlaybackHandle::Stop()
{
	if (source) {
		source->Stop();
		source.reset();
	}
}

AudioPlayback::AudioPlayback(const std::vector<ResRef>& defaultSounds)
	: defaultSounds(defaultSounds)
{}

Holder<PlaybackHandle> AudioPlayback::Play(StringView resource, AudioPreset preset, SFXChannel channel)
{
	auto& settings = core->GetAudioSettings();
	AudioPlaybackConfig config;

	switch (preset) {
		case AudioPreset::Dialog:
			config = settings.ConfigPresetDialog(channel);
			break;
		case AudioPreset::EnvVoice:
			config = settings.ConfigPresetEnvVoice(channel);
			break;
		case AudioPreset::ScreenAction:
			config = settings.ConfigPresetScreenAction(channel);
			break;
		default:
			break;
	}

	return Play(resource, config);
}

Holder<PlaybackHandle> AudioPlayback::Play(StringView resource, AudioPreset preset, SFXChannel channel, const Point& point, const Map* map)
{
	auto config = preset == AudioPreset::SpatialVoice ? core->GetAudioSettings().ConfigPresetSpatialVoice(channel, point) : core->GetAudioSettings().ConfigPresetByChannel(channel, point);
	return Play(resource, config, map);
}

Holder<PlaybackHandle> AudioPlayback::PlayDirectional(StringView resource, SFXChannel channel, const Point& point, orient_t orientation, const Map* map)
{
	return Play(resource, core->GetAudioSettings().ConfigPresetDirectional(channel, point, orientation), map);
}

Holder<PlaybackHandle> AudioPlayback::Play(StringView resource, const AudioPlaybackConfig& config, const Map* map)
{
	Housekeeping();

	if (resource.empty()) {
		return {};
	}

	auto cacheEntry = GetBuffer(resource, config);
	if (!cacheEntry.handle) {
		return {};
	}

	auto source = core->GetAudioDrv()->CreatePlaybackSource(config);
	if (!source) {
		return {};
	}
	source->Enqueue(std::move(cacheEntry.handle));

	activeSources.insert(source);

	auto handle = MakeHolder<PlaybackHandle>(std::move(source), cacheEntry.length, config.position.z);
	if (config.spatial) {
		core->GetAudioSpatialMonitor().AddHandleToMonitor(handle, map);
	}

	return handle;
}

Holder<PlaybackHandle> AudioPlayback::PlayDefaultSound(size_t index, SFXChannel channel)
{
	return PlayDefaultSound(index, core->GetAudioSettings().ConfigPresetScreenAction(channel));
}

Holder<PlaybackHandle> AudioPlayback::PlayDefaultSound(size_t index, const AudioPlaybackConfig& config)
{
	Housekeeping();

	if (index >= defaultSounds.size()) {
		return {};
	}

	auto resource = defaultSounds[index];
	auto cacheEntry = GetBuffer(resource, config);
	if (!cacheEntry.handle) {
		return {};
	}

	auto source = core->GetAudioDrv()->CreatePlaybackSource(config);
	if (!source) {
		return {};
	}
	source->Enqueue(std::move(cacheEntry.handle));

	activeSources.insert(source);

	return MakeHolder<PlaybackHandle>(std::move(source), cacheEntry.length, config.position.z);
}

time_t AudioPlayback::PlaySpeech(StringView resource, const AudioPlaybackConfig& config, bool interrupt)
{
	Housekeeping();

	if (!speech) {
		speech = core->GetAudioDrv()->CreatePlaybackSource(config, true);
	}
	if (!speech) {
		return 0;
	}

	auto cacheEntry = GetBuffer(resource, config);
	if (!cacheEntry.handle) {
		return 0;
	}

	if (interrupt) {
		speech->Stop();
	}

	speech->Reconfigure(config);
	speech->Enqueue(std::move(cacheEntry.handle));

	return cacheEntry.length;
}

void AudioPlayback::StopSpeech()
{
	if (speech) {
		Housekeeping();
		speech->Stop();
	}
}

BufferCacheEntry AudioPlayback::GetBuffer(StringView resource, const AudioPlaybackConfig& config)
{
	auto cacheEntry = bufferCache.Lookup(resource);
	if (cacheEntry) {
		return *cacheEntry;
	}

	ResourceHolder<SoundMgr> acm = gamedata->GetResourceHolder<SoundMgr>(resource);
	if (!acm) {
		return {};
	}

	auto length = acm->GetLengthMs();
	auto handle = core->GetAudioDrv()->LoadSound(std::move(acm), config);
	if (!handle) {
		return {};
	}

	bufferCache.SetAt(resource, std::move(handle), length);

	return *bufferCache.Lookup(resource);
}

void AudioPlayback::Housekeeping()
{
	for (auto it = activeSources.begin(); it != activeSources.end();) {
		if ((**it).HasFinishedPlaying()) {
			it = activeSources.erase(it);
		} else {
			++it;
		}
	}
}

}
