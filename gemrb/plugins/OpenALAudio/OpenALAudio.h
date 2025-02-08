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

#ifndef H_OPENAL_AUDIO
#define H_OPENAL_AUDIO

#include "globals.h"

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
	int channelVolume;
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
	ALuint source;
	int channelVolume;
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
