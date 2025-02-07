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

#include "MVEPlayer.h"

#include "Interface.h"
#include "mve.h"

#include <array>

#ifdef WIN32
	#include <profileapi.h>
#endif

namespace GemRB {

static constexpr uint8_t MVE_SIGNATURE_LEN = 26;

MVEPlayer::MVEPlayer()
{
	audioPlayer = core->GetAudioDrv()->CreateStreamable(core->GetAudioSettings().ConfigPresetMovie());

	palette.SetColor(1, ColorBlack);
	// subtitle color
	palette.SetColor(255, Color { 50, 50, 50, 255 });
}

bool MVEPlayer::Import(DataStream* str)
{
	FixedSizeString<MVE_SIGNATURE_LEN> signatureBuffer;
	if (str->Read(signatureBuffer.begin(), MVE_SIGNATURE_LEN) < MVE_SIGNATURE_LEN) {
		return false;
	}

	if (signatureBuffer != StringView { "Interplay MVE File\x1A\x0\x1A\x0\x0\x1\x11\x33" }) {
		return false;
	}

	endAfterNextFrame = false;

	// Assumed to cover A/V setup
	return ProcessChunk() == FrameResult::OK && ProcessChunk() == FrameResult::OK;
}

bool MVEPlayer::DecodeFrame(VideoBuffer& videoBuffer)
{
	auto result = FrameResult::OK;
	if (framePos == 0) {
		result = ProcessChunksForFrame();
	}

	if (result == FrameResult::ERROR) {
		return false;
	}

	auto now = get_current_time();
	CopyFrame(videoBuffer);
	if (endAfterNextFrame) {
		return false;
	}

	auto copyTime = get_current_time() - now;
	framePos++;

	auto waitUsec = duration;
	now = get_current_time();

	result = ProcessChunksForFrame();
	if (result == FrameResult::ERROR) {
		return false;
	} else if (result == FrameResult::END_OF_MOVIE) {
		endAfterNextFrame = true;
	}

	waitUsec -= (get_current_time() - now);
	waitUsec -= copyTime;
	Wait(waitUsec);

	return true;
}

void MVEPlayer::Wait(microseconds us)
{
#ifdef WIN32
	// Win is known to be limited to 14-16ms precision by default
	// so do spin lock instead
	LARGE_INTEGER time1, time2, freq;

	QueryPerformanceCounter(&time1);
	QueryPerformanceFrequency(&freq);

	decltype(LARGE_INTEGER::QuadPart) elapsed = 0;
	do {
		QueryPerformanceCounter(&time2);
		elapsed = (time2.QuadPart - time1.QuadPart) * 1'000'000 / freq.QuadPart;
	} while (elapsed < us.count());
#else
	std::this_thread::sleep_for(us);
#endif
}

MVEPlayer::FrameResult MVEPlayer::InitializeAudio(uint8_t version)
{
	if (str->Seek(2, GEM_CURRENT_POS) != 0) {
		return FrameResult::ERROR;
	}

	std::array<uint8_t, 8> buffer;
	if (str->Read(buffer.data(), 8) < 8) {
		return FrameResult::ERROR;
	}

	uint16_t flags = GST_READ_UINT16_LE(buffer.data());
	/* bit 0: 0 = mono, 1 = stereo */
	audioFormat.channels = (flags & MVE_AUDIO_STEREO) + 1;
	/* bit 1: 0 = 8 bit, 1 = 16 bit */
	audioFormat.bits = (((flags & MVE_AUDIO_16BIT) >> 1) + 1) * 8;
	audioFormat.sampleRate = GST_READ_UINT16_LE(buffer.data() + 2);
	/* bit 2: 0 = uncompressed, 1 = compressed */
	audioCompressed = ((version > 0) && (flags & MVE_AUDIO_COMPRESSED));
	/* the docs say min_buffer_len is 16-bit for version 0, all other code just assumes 32-bit.. */
	auto audioBufferSize = GST_READ_UINT32_LE(buffer.data() + 4);

	if (audioBufferSize > audioLoadBuffer.size()) {
		audioLoadBuffer.resize(audioBufferSize);
		audioDecompressedBuffer.resize(audioCompressed ? audioBufferSize : 0);
	}

	return FrameResult::OK;
}

MVEPlayer::FrameResult MVEPlayer::InitializeVideo(uint8_t version)
{
	std::array<uint8_t, 8> buffer;
	if (str->Read(buffer.data(), 8) < 8) {
		return FrameResult::ERROR;
	}

	uint16_t format = 0;
	if (version > 1) {
		format = GST_READ_UINT16_LE(buffer.data() + 6);
	}
	movieFormat = format > 0 ? Video::BufferFormat::RGB555 : Video::BufferFormat::RGBPAL8;

	gstData.width = GST_READ_UINT16_LE(buffer.data()) << 3;
	gstData.height = GST_READ_UINT16_LE(buffer.data() + 2) << 3;

	size_t backBufSize = 2 * gstData.width * gstData.height * (format > 0 ? 2 : 1);
	if (videoBackBuffer.size() < backBufSize) {
		videoBackBuffer.resize(backBufSize);
	}
	std::fill(videoBackBuffer.begin(), videoBackBuffer.end(), 0);

	gstData.back_buf1 = reinterpret_cast<guint16*>(videoBackBuffer.data());
	gstData.back_buf2 = reinterpret_cast<guint16*>(videoBackBuffer.data() + backBufSize / 2);
	gstData.max_block_offset = (gstData.height - 7) * gstData.width - 8;

	return FrameResult::OK;
}

void MVEPlayer::CopyFrame(VideoBuffer& videoBuffer)
{
	auto s = videoBuffer.Size();
	int dX = unsigned(s.w - gstData.width) >> 1;
	int dY = unsigned(s.h - gstData.height) >> 1;

	videoBuffer.CopyPixels(
		Region { dX, dY, gstData.width, gstData.height },
		gstData.back_buf1,
		nullptr,
		&palette);
}

MVEPlayer::FrameResult MVEPlayer::ProcessChunksForFrame()
{
	auto result = FrameResult::OK;

	do {
		result = ProcessChunk();
	} while (result == FrameResult::OK);

	return result;
}

MVEPlayer::FrameResult MVEPlayer::ProcessChunk()
{
	std::array<uint8_t, 4> buffer;
	if (str->Read(buffer.data(), 4) < 4) {
		return FrameResult::ERROR;
	}

	auto lastResult = FrameResult::OK;
	uint16_t chunkSize = GST_READ_UINT16_LE(buffer.data());
	uint16_t offset = 0;

	while (offset < chunkSize) {
		if (str->Read(buffer.data(), 4) < 4) {
			return FrameResult::ERROR;
		}

		uint16_t segmentSize = GST_READ_UINT16_LE(buffer.data());
		uint8_t segmentType = buffer[2];
		uint8_t segmentVersion = buffer[3];

		lastResult = ProcessSegment(segmentSize, segmentType, segmentVersion);
		if (lastResult != FrameResult::OK) {
			break;
		}

		offset += 4 + segmentSize;
	}

	return lastResult;
}

MVEPlayer::FrameResult MVEPlayer::ProcessSegment(uint16_t size, uint8_t type, uint8_t version)
{
	switch (type) {
		case MVE_OC_CREATE_TIMER:
			return UpdateFrameDuration();
		case MVE_OC_AUDIO_BUFFERS:
			return InitializeAudio(version);
		case MVE_OC_VIDEO_BUFFERS:
			return InitializeVideo(version);
		case MVE_OC_AUDIO_DATA:
		case MVE_OC_AUDIO_SILENCE:
			return QueueAudio(type == MVE_OC_AUDIO_SILENCE, size);
		case MVE_OC_VIDEO_MODE:
			return UpdateVideoMode();
		case MVE_OC_PALETTE:
			return UpdatePalette();
		case MVE_OC_CODE_MAP:
			return UpdateCodeMap(size);
		case MVE_OC_VIDEO_DATA:
			return RenderFrame(size);
		case MVE_OC_END_OF_STREAM:
			str->Seek(size, GEM_CURRENT_POS);
			return FrameResult::END_OF_MOVIE;
		case MVE_OC_PLAY_VIDEO:
			str->Seek(size, GEM_CURRENT_POS);
			return FrameResult::END_OF_FRAME;
		case 0x13:
		case 0x14:
		case 0x15:
		case MVE_OC_END_OF_CHUNK:
		case MVE_OC_PALETTE_COMPRESSED:
		case MVE_OC_PLAY_AUDIO:
			/* ignore */
			str->Seek(size, GEM_CURRENT_POS);
			break;
		default:
			str->Seek(size, GEM_CURRENT_POS);
			Log(WARNING, "MVEPlayer", "Skipping unknown segment type {:#x}", type);
	}

	return FrameResult::OK;
}

MVEPlayer::FrameResult MVEPlayer::UpdateFrameDuration()
{
	std::array<uint8_t, 6> buffer;
	if (str->Read(buffer.data(), 6) < 6) {
		return FrameResult::ERROR;
	}

	/* new frame every (timerRate * timerSubdiv) microseconds */
	uint32_t timerRate = GST_READ_UINT32_LE(buffer.data());
	uint16_t timerSubdiv = GST_READ_UINT16_LE(buffer.data() + 4);

	duration = microseconds { timerRate * timerSubdiv };

	return FrameResult::OK;
}

MVEPlayer::FrameResult MVEPlayer::UpdateVideoMode()
{
	std::array<uint8_t, 6> buffer;
	if (str->Read(buffer.data(), 6) < 6) {
		return FrameResult::ERROR;
	}

	movieSize.w = GST_READ_UINT16_LE(buffer.data());
	movieSize.h = GST_READ_UINT16_LE(buffer.data() + 2);

	return FrameResult::OK;
}

MVEPlayer::FrameResult MVEPlayer::UpdatePalette()
{
	std::array<uint8_t, 4> buffer;
	if (str->Read(buffer.data(), 4) < 4) {
		return FrameResult::ERROR;
	}

	uint16_t startIdx = GST_READ_UINT16_LE(buffer.data());
	uint16_t numEntries = GST_READ_UINT16_LE(buffer.data() + 2);
	strret_t paletteNumBytes = numEntries * 3;

	std::vector<unsigned char> paletteData;
	paletteData.resize(paletteNumBytes);

	if (str->Read(paletteData.data(), paletteNumBytes) < paletteNumBytes) {
		return FrameResult::ERROR;
	}

	Palette::Colors palBuffer;
	for (unsigned int i = startIdx, j = 0; i < startIdx + numEntries; i++, j += 3) {
		palBuffer[i].r = paletteData[j] << 2;
		palBuffer[i].g = paletteData[j + 1] << 2;
		palBuffer[i].b = paletteData[j + 2] << 2;
		palBuffer[i].a = 0xff;
	}

	palette.CopyColors(startIdx, palBuffer.cbegin() + startIdx, palBuffer.cbegin() + startIdx + numEntries);

	return FrameResult::OK;
}

MVEPlayer::FrameResult MVEPlayer::QueueAudio(bool silence, uint16_t size)
{
	std::array<uint8_t, 6> buffer;
	if (str->Read(buffer.data(), 6) < 6) {
		return FrameResult::ERROR;
	}

	uint16_t streamMask = GST_READ_UINT16_LE(buffer.data() + 2);
	uint16_t audioSize = GST_READ_UINT16_LE(buffer.data() + 4);

	auto dataSize = size - 6;
	if (!audioPlayer) {
		str->Seek(dataSize, GEM_CURRENT_POS);
		return FrameResult::OK;
	}

	if (!(streamMask & MVE_DEFAULT_AUDIO_STREAM)) {
		str->Seek(dataSize, GEM_CURRENT_POS);
		return FrameResult::OK;
	}

	char* readBuffer = reinterpret_cast<char*>(audioLoadBuffer.data());
	if (silence) {
		std::fill(audioLoadBuffer.begin(), audioLoadBuffer.begin() + audioSize, 0);
	} else {
		if (audioCompressed) {
			if (str->Read(audioLoadBuffer.data(), dataSize) < dataSize) {
				return FrameResult::ERROR;
			}

			readBuffer = reinterpret_cast<char*>(audioDecompressedBuffer.data());
			ipaudio_uncompress(
				reinterpret_cast<short int*>(audioDecompressedBuffer.data()),
				audioSize,
				reinterpret_cast<unsigned char*>(audioLoadBuffer.data()),
				audioFormat.channels);
		} else {
			if (str->Read(audioLoadBuffer.data(), audioSize) < audioSize) {
				return FrameResult::ERROR;
			}
		}
	}

	// Queue now, reduce underrun risk
	audioPlayer->Feed(audioFormat, readBuffer, audioSize);

	return FrameResult::OK;
}

MVEPlayer::FrameResult MVEPlayer::UpdateCodeMap(uint16_t size)
{
	codeMap.resize(size);
	if (str->Read(codeMap.data(), size) < size) {
		return FrameResult::ERROR;
	}

	gstData.code_map = reinterpret_cast<guint8*>(codeMap.data());

	return FrameResult::OK;
}

MVEPlayer::FrameResult MVEPlayer::RenderFrame(uint16_t size)
{
	if (str->Seek(12, GEM_CURRENT_POS) != 0) {
		return FrameResult::ERROR;
	}

	uint16_t flags = 0;
	if (str->Read(&flags, 2) != 2) {
		return FrameResult::ERROR;
	}

	size -= 14;
	if (flags & MVE_VIDEO_DELTA_FRAME) {
		guint16* temp = gstData.back_buf1;
		gstData.back_buf1 = gstData.back_buf2;
		gstData.back_buf2 = temp;
	}

	if (videoLoadBuffer.size() < size) {
		videoLoadBuffer.resize(size);
	}

	if (str->Read(videoLoadBuffer.data(), size) != size) {
		return FrameResult::ERROR;
	}

	if (movieFormat == Video::BufferFormat::RGB555) {
		ipvideo_decode_frame16(&gstData, videoLoadBuffer.data(), size);
	} else {
		ipvideo_decode_frame8(&gstData, videoLoadBuffer.data(), size);
	}

	return FrameResult::OK;
}

}

#include "plugindef.h"

GEMRB_PLUGIN(0x218963DC, "MVE Video Player")
PLUGIN_IE_RESOURCE(MVEPlayer, "mve", (ieWord) IE_MVE_CLASS_ID)
END_PLUGIN()
