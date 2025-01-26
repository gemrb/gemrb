/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

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
