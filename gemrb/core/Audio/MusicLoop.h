// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_AUDIO_MUSIC_LOOP
#define H_AUDIO_MUSIC_LOOP

#include "AudioBackend.h"
#include "Resource.h"

#include <mutex>
#include <thread>
#include <vector>

namespace GemRB {

class GEM_EXPORT MusicLoop {
public:
	MusicLoop();
	MusicLoop(const MusicLoop&) = delete;
	~MusicLoop();

	void Load(ResourceHolder<SoundMgr> music);
	void ResetMusics();
	void Pause();
	void Resume();
	void Stop();
	void UpdateVolume();

private:
	Holder<SoundStreamSourceHandle> streamHandle;
	std::vector<short> loadBuffer;

	ResourceHolder<SoundMgr> currentMusic;
	AudioBufferFormat currentFormat;

	std::thread loopThread;
	std::recursive_mutex mutex;

	bool loop = true;
	bool rampUp = false;
	bool needToPoll = true;

	bool FillBuffers(uint16_t numBuffers);
	void Loop();
};

}

#endif
