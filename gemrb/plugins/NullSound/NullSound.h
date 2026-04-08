// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef NULLSOUND_H
#define NULLSOUND_H

#include "Audio/AudioBackend.h"

namespace GemRB {

class NullSoundBufferHandle : public SoundBufferHandle {
public:
	bool Disposable() override { return true; }
};

class NullSoundSourceHandle : public SoundSourceHandle {
public:
	bool operator==(const SoundSourceHandle&) override { return true; }

	bool Enqueue(Holder<SoundBufferHandle>) override { return true; }
	const AudioPoint& GetPosition() const override { return nullPoint; }
	bool HasFinishedPlaying() const override { return true; }
	bool IsSpatial() const override { return false; }
	void Reconfigure(const AudioPlaybackConfig&) override { /* null */ }
	void Stop() override { /* null */ }
	void StopLooping() override { /* null */ }
	void SetOccluded(bool) override { /* null */ }
	void SetPitch(int) override { /* null */ }
	void SetPosition(const AudioPoint&) override { /* null */ }
	void SetVolume(int) override { /* null */ }

private:
	static AudioPoint nullPoint;
};

class NullSoundStreamSourceHandle : public SoundStreamSourceHandle {
public:
	bool Feed(const AudioBufferFormat&, const char*, size_t) override
	{
		return true;
	}
	bool HasProcessed() override { return true; }
	void Pause() override { /* null */ }
	void Resume() override { /* null */ }
	void Stop() override { /* null */ }

	void SetVolume(int) override { /* null */ }
};

class NullSound : public AudioBackend {
public:
	bool Init() override { return true; };

	Holder<SoundSourceHandle> CreatePlaybackSource(const AudioPlaybackConfig&, bool priority = false) override;
	Holder<SoundStreamSourceHandle> CreateStreamable(const AudioPlaybackConfig&, size_t) override;
	Holder<SoundBufferHandle> LoadSound(ResourceHolder<SoundMgr> resource, const AudioPlaybackConfig&) override;

	bool HasOcclusionFeature() override { return false; }

	const AudioPoint& GetListenerPosition() const override { return point; }
	void SetListenerPosition(const AudioPoint& p) override { point = p; }

	StreamMode GetStreamMode() const override { return StreamMode::POLLING; }

private:
	AudioPoint point;
};
}

#endif
