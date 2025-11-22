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

#include "MusicLoop.h"

#include "Interface.h"
#include "MusicMgr.h"

#include <chrono>

namespace GemRB {

// balance between memory, number of calls and slow I/O
static constexpr uint32_t CHUNK_BUFFER_SIZE_MS = 125;

MusicLoop::MusicLoop()
{
	auto audio = core->GetAudioDrv();
	streamHandle =
		audio->CreateStreamable(
			core->GetAudioSettings().ConfigPresetMusic());

	needToPoll = audio->GetStreamMode() == AudioBackend::StreamMode::POLLING;
	loopThread = std::thread(&MusicLoop::Loop, this);
}

MusicLoop::~MusicLoop()
{
	streamHandle->Stop();
	loop = false;
	loopThread.join();
}

void MusicLoop::Load(ResourceHolder<SoundMgr> music)
{
	std::lock_guard<std::recursive_mutex> l(mutex);

	// Enqueue remaining music to avoid hard cuts
	if (currentMusic) {
		while (FillBuffers(1)) {}
	}

	currentFormat.bits = 16;
	currentFormat.channels = 2; // GetChannels may report garbage
	currentFormat.sampleRate = music->GetSampleRate();

	auto loadBufferSize = currentFormat.GetNumBytesForMs(CHUNK_BUFFER_SIZE_MS) / 2;
	// OGGReader does not like odd buffer sizes
	if (loadBufferSize % 2 == 1) {
		loadBufferSize -= 1;
	}
	loadBuffer.resize(loadBufferSize);

	currentMusic = std::move(music);
	rampUp = true;

	streamHandle->Reclaim();
}

void MusicLoop::UpdateVolume()
{
	streamHandle->SetVolume(core->GetAudioSettings().GetMusicVolume());
}

void MusicLoop::Pause()
{
	streamHandle->Pause();
}

void MusicLoop::Resume()
{
	streamHandle->Resume();
}

void MusicLoop::Stop()
{
	streamHandle->Stop();
	{
		std::lock_guard<std::recursive_mutex> l(mutex);
		currentMusic.reset();
	}
}

void MusicLoop::Loop()
{
	while (loop) {
		// Without at least some waiting, the mutex may stay locked
		std::this_thread::sleep_for(std::chrono::milliseconds(needToPoll ? 30 : 1));
		std::lock_guard<std::recursive_mutex> l(mutex);

		if (!currentMusic) {
			continue;
		}

		streamHandle->SetVolume(core->GetAudioSettings().GetMusicVolume());

		if (rampUp) {
			FillBuffers(8);
			rampUp = false;
			continue;
		}

		if (streamHandle->HasProcessed() && !FillBuffers(1)) {
			core->GetMusicMgr()->PlayNext();
			if (currentMusic) {
				FillBuffers(1);
			}
		}
	}
}

// false signals that the source has been exhausted
bool MusicLoop::FillBuffers(uint16_t numBuffers8thSec)
{
	for (uint16_t i = 0; i < numBuffers8thSec; ++i) {
		auto samples = currentMusic->read_samples(loadBuffer.data(), loadBuffer.size());

		if (samples == 0) {
			return false;
		}

		auto bytesToFeed = samples * 2;
		if (!streamHandle->Feed(
			    currentFormat,
			    reinterpret_cast<const char*>(loadBuffer.data()),
			    bytesToFeed)) {
			return true;
		}

		if (samples < loadBuffer.size()) {
			return false;
		}
	}

	return true;
}

}
