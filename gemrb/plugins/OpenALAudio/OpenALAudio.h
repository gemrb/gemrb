// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_OPENAL_AUDIO
#define H_OPENAL_AUDIO

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <utility>

#if __APPLE__
	#include <OpenAL/OpenAL.h> // umbrella include for all the headers we want
#else
	#include "al.h"
	#include "alc.h"
	#ifdef HAVE_OPENAL_EFX_H
		#include "efx.h"
	#endif
#endif

#include "Audio/AudioBackend.h"

namespace GemRB {

using ALPair = std::pair<ALuint, ALuint>;

class OpenALBufferHandle : public SoundBufferHandle {
public:
	explicit OpenALBufferHandle(ALPair buffers);
	OpenALBufferHandle(const OpenALBufferHandle& other) = delete;
	OpenALBufferHandle(OpenALBufferHandle&& other) noexcept;
	~OpenALBufferHandle() override;

	bool Disposable() override;
	ALPair GetBuffers() const;
	AudioBufferFormat GetFormat() const;

private:
	ALPair buffers;
};

class OpenALSourceHandle : public SoundSourceHandle {
public:
	OpenALSourceHandle(ALPair sources, const AudioPlaybackConfig&);
	OpenALSourceHandle(const OpenALSourceHandle& other) = delete;
	OpenALSourceHandle(OpenALSourceHandle&& other) noexcept;
	~OpenALSourceHandle() override;

	bool Enqueue(Holder<SoundBufferHandle> handle) override;
	bool HasFinishedPlaying() const override;
	void Reconfigure(const AudioPlaybackConfig& config) override;
	void Stop() override;
	void StopLooping() override;
	void SetPosition(const AudioPoint&) override;
	void SetPitch(int pitch) override;
	void SetVolume(int volume) override;

private:
	AudioBufferFormat lastFormat;
	ALPair sources;
	int channelVolume = 0;
};

class OpenALSoundStreamHandle : public SoundStreamSourceHandle {
public:
	OpenALSoundStreamHandle(ALint source, int channelVolume);
	OpenALSoundStreamHandle(const OpenALSoundStreamHandle& other) = delete;
	OpenALSoundStreamHandle(OpenALSoundStreamHandle&& other) noexcept;
	~OpenALSoundStreamHandle() override;

	bool Feed(const AudioBufferFormat& format, const char* memory, size_t size) override;
	bool HasProcessed() override;
	void Pause() override;
	void Resume() override;
	void Stop() override;

	void SetVolume(int volume) override;

private:
	ALuint source = 0;
	int channelVolume = 0;
	void UnloadFinishedSourceBuffers() const;
};

class OpenALBackend : public AudioBackend {
public:
	~OpenALBackend() override;

	bool Init() override;

	Holder<SoundSourceHandle> CreatePlaybackSource(const AudioPlaybackConfig& config, bool priority = false) override;
	Holder<SoundStreamSourceHandle> CreateStreamable(const AudioPlaybackConfig& config, size_t) override;
	Holder<SoundBufferHandle> LoadSound(ResourceHolder<SoundMgr> resource, const AudioPlaybackConfig& config) override;

	const AudioPoint& GetListenerPosition() const override;
	void SetListenerPosition(const AudioPoint& p) override;
	void SetReverbProperties(const MapReverbProperties& props) override;

	StreamMode GetStreamMode() const override { return StreamMode::POLLING; }

private:
	ALCcontext* alContext = nullptr;
	AudioPoint listenerPosition;

	void InitEFX();
	ALPair GetBuffers(ResourceHolder<SoundMgr> resource, bool spatial) const;
};
}

#endif
