// SPDX-FileCopyrightText: 2012 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef VLCPLAY_H
#define VLCPLAY_H

#include "MoviePlayer.h"

#include <condition_variable>
#include <vlc/vlc.h>

namespace GemRB {

/*
 NOTE: using VLC player may result in poor video playback.
 VLC will convert video to match our supported formats (RGB555 and YV12)
 as well as scaling. These operations are expensive and should be avoided
 by using a video properly sized and in a supported colorspace.

 There are a number of things we could do here to speed things up if necessary:

 1. we could enhance our video drivers to allow for additional pixel formats. (at least via bpp for RGB video)
 2. instead of VLC -> context -> video texture/surface we could provide a way to do VLC -> texture/surface
 3. in VLCPlayer::setup() we could use the provided pitches and lines instead of forcing VLC to convert
 (see laziness notes in VLCPlayer.cpp)
*/

class VLCPlayer : public MoviePlayer {
private:
	enum { Y,
	       U,
	       V };
	char* planes[3] {};

	libvlc_instance_t* libvlc;
	libvlc_media_player_t* mediaPlayer = nullptr;

	// libvlc_video_set_callbacks
	static void display(void* data, void* id);
	static void* lock(void* data, void** planes);

	// libvlc_video_set_format_callbacks
	static unsigned setup(void** opaque, char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines);

	bool DecodeFrame(VideoBuffer&) override;
	void DestroyPlayer();

	std::condition_variable formatWaitVar;

public:
	VLCPlayer();
	~VLCPlayer() override;

	bool Import(DataStream* stream) override;
	void Stop() override;
};

}

#endif
