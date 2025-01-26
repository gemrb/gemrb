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
 *
 */

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
	bool Enqueue(Holder<SoundBufferHandle>) override { return true; }
	bool HasFinishedPlaying() const override { return true; }
	void Reconfigure(const AudioPlaybackConfig&) override { /* null */ }
	void Stop() override { /* null */ }
	void StopLooping() override { /* null */ }
	void SetPitch(int) override { /* null */ }
	void SetPosition(const AudioPoint&) override { /* null */ }
	void SetVolume(int) override { /* null */ }
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
	Holder<SoundStreamSourceHandle> CreateStreamable(const AudioPlaybackConfig&) override;
	Holder<SoundBufferHandle> LoadSound(ResourceHolder<SoundMgr> resource, const AudioPlaybackConfig&) override;

	const AudioPoint& GetListenerPosition() const override { return point; }
	void SetListenerPosition(const AudioPoint& p) override { point = p; }

	StreamMode GetStreamMode() const override { return StreamMode::POLLING; }

private:
	AudioPoint point;
};
}

#endif
