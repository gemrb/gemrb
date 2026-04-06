// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SDLAUDIO_H
#define SDLAUDIO_H


#include "RingBuffer.h"

#include "Audio/AudioBackend.h"

#include <SDL_mixer.h>
#include <mutex>
#include <set>

namespace GemRB {

class SDLSoundBufferHandle : public SoundBufferHandle {
public:
	SDLSoundBufferHandle(Mix_Chunk*, std::vector<char>&&);
	~SDLSoundBufferHandle() override;

	bool Disposable() override;

	Mix_Chunk* GetChunk();
	void AssignChannel(int channel);

private:
	Mix_Chunk* chunk;
	std::vector<char> chunkBuffer;
	std::set<int> assignedChannels;
};

class SDLSoundSourceHandle : public SoundSourceHandle {
public:
	using PositionGetter = std::function<const AudioPoint&()>;

	SDLSoundSourceHandle(const AudioPlaybackConfig& config, PositionGetter positionGetter, int channel = -1);
	~SDLSoundSourceHandle() override;

	bool operator==(const SoundSourceHandle&) override;

	bool Enqueue(Holder<SoundBufferHandle>) override;
	const AudioPoint& GetPosition() const override;
	bool HasFinishedPlaying() const override;
	bool IsSpatial() const override;
	void ConfigChannel() const;
	void Reconfigure(const AudioPlaybackConfig& config) override;
	void Stop() override;
	void StopLooping() override;
	void SetOccluded(bool) override { /* nop */ };
	void SetPitch(int) override { /* no known implementation yet */ };
	void SetPosition(const AudioPoint&) override;
	void SetVolume(int) override;

	uint64_t GetID() const;

private:
	static uint64_t nextId;

	uint64_t id;
	AudioPlaybackConfig config;
	PositionGetter positionGetter;
	int channel = -1;
	bool reserved = false;

	bool CanOperateOnChannel() const;
};

class SDLSoundStreamSourceHandle : public SoundStreamSourceHandle {
public:
	explicit SDLSoundStreamSourceHandle(size_t);
	~SDLSoundStreamSourceHandle() override;

	bool Feed(const AudioBufferFormat&, const char*, size_t) override;
	bool HasProcessed() override;
	void Pause() override;
	void Resume() override;
	void Reclaim() override;
	void Stop() override;

	void SetVolume(int) override;

private:
	int volume = 128;
	static std::mutex mixMutex;
	RingBuffer<char> ringBuffer;
	bool fillWait = true;

	static void StreamCallback(const void*, uint8_t* stream, int len);
};

class MixCallbackState {
public:
	static SDLSoundStreamSourceHandle* handle;
	static std::mutex mutex;

private:
	MixCallbackState() = default;
};

class SDLAudioBackend : public AudioBackend {
public:
	~SDLAudioBackend() override;

	bool Init() override;

	bool HasOcclusionFeature() override { return false; }
	Holder<SoundSourceHandle> CreatePlaybackSource(const AudioPlaybackConfig&, bool priority = false) override;
	Holder<SoundStreamSourceHandle> CreateStreamable(const AudioPlaybackConfig&, size_t minQueueSize) override;
	Holder<SoundBufferHandle> LoadSound(ResourceHolder<SoundMgr> resource, const AudioPlaybackConfig&) override;

	const AudioPoint& GetListenerPosition() const override;
	void SetListenerPosition(const AudioPoint& p) override;

	StreamMode GetStreamMode() const override { return StreamMode::WAITING; }

	static int audioRate;
	static unsigned short audioFormat;
	static int audioChannels;

private:
	AudioPoint listenerPosition;
	size_t reservedCounter = 0;
	std::vector<std::weak_ptr<SDLSoundSourceHandle>> issuedChannels;

	void Housekeeping();
};
}

#endif
