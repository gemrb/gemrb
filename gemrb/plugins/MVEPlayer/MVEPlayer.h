// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MVEPLAY_H
#define MVEPLAY_H

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
	GstMveDemuxStream gstData {};
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
