/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2012 The GemRB Project
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
 *
 */

#ifndef VLCPLAY_H
#define VLCPLAY_H

#include <vlc/vlc.h>

#include "Interface.h"
#include "MoviePlayer.h"
#include "VideoContext.h"

namespace GemRB {

/*
 NOTE: using VLC player may result in poor video playback.
 VLC will convert video to match our supported formats (RGB565 and YV12)
 as well as scaleing. These operations are expensive and should be avoided
 by using a video properly sized and in a supported colorspace.

 There are a number of things we could do here to speed things up if necessary:

 1. we could enhance our video drivers to allow for additional pixel formats. (at least via bpp for RGB video)
 2. instead of VLC -> context -> video texture/surface we could provide a way to do VLC -> texture/surface
 3. in VLCPlayer::setup() we could use the provided pitches and lines instead of forcing VLC to convert
 (see lazyness notes in VLCPlayer.cpp)
*/

class VLCPlayer : public MoviePlayer {
private:
	libvlc_instance_t *libvlc;
	libvlc_media_t *media;
	libvlc_media_player_t *mediaPlayer;

	VideoContext* ctx;

	// libvlc_video_set_callbacks
	static void display(void *data, void *id);
	static void unlock(void *data, void *id, void *const *planes);
	static void* lock(void *data, void **planes);

	// libvlc_video_set_format_callbacks
	static unsigned setup(void **opaque, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines);
	static void cleanup(void *opaque);
public:
	VLCPlayer(void);
	~VLCPlayer(void);
	bool Open(DataStream* stream);
	int Play();
	void Stop();
	void CallBackAtFrames(ieDword cnt, ieDword *arg, ieDword *arg2);
};

}

#endif
