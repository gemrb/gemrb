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

#ifndef MVEPLAY_H
#define MVEPLAY_H

#include "globals.h"

#include "MoviePlayer.h"
#include "gstmvedemux.h"

#include "Audio/AudioBackend.h"

namespace GemRB {

class MVEPlayer : public MoviePlayer {
public:
	MVEPlayer();

	bool Import(DataStream* stream) override;

protected:
	bool DecodeFrame(VideoBuffer&) override;

private:
	enum class FrameResult { OK,
				 END_OF_FRAME,
				 END_OF_MOVIE,
				 ERROR };

	bool endAfterNextFrame = false;

	Holder<SoundStreamSourceHandle> audioPlayer;
	AudioBufferFormat audioFormat;
	bool audioCompressed = false;
	std::vector<char> audioLoadBuffer;
	std::vector<char> audioDecompressedBuffer;

	Palette palette;
	microseconds duration = microseconds { 0 };

	std::vector<unsigned char> videoLoadBuffer;
	GstMveDemuxStream gstData;
	std::vector<char> codeMap;
	std::vector<char> videoBackBuffer;

	void CopyFrame(VideoBuffer&);
	FrameResult InitializeAudio(uint8_t);
	FrameResult InitializeVideo(uint8_t);
	FrameResult ProcessChunk();
	FrameResult ProcessChunksForFrame();
	FrameResult ProcessSegment(uint16_t, uint8_t, uint8_t);
	FrameResult QueueAudio(bool, uint16_t);
	FrameResult RenderFrame(uint16_t);
	FrameResult UpdateCodeMap(uint16_t);
	FrameResult UpdateFrameDuration();
	FrameResult UpdatePalette();
	FrameResult UpdateVideoMode();

	static void Wait(microseconds);
};

}

#endif
