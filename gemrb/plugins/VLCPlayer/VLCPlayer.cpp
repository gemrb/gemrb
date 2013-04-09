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

#include "VLCPlayer.h"
#include "Video.h"

using namespace GemRB;

VLCPlayer::VLCPlayer(void)
{
	libvlc = libvlc_new(0, NULL);
	mediaPlayer = NULL;
	media = NULL;
	ctx = NULL;
}

VLCPlayer::~VLCPlayer(void)
{
	if (ctx) {
		Stop(); // this call will lead to the dallocation of ctx
	}
	libvlc_release(libvlc);
}

bool VLCPlayer::Open(DataStream* stream)
{
	if (mediaPlayer) {
		Stop();
	}
	if (stream) {
		// we don't actually need anything from the stream. libVLC will open and use the file internally
		media = libvlc_media_new_path(libvlc, stream->originalfile);
		mediaPlayer = libvlc_media_player_new_from_media(media);
		libvlc_media_release(media); //player retains the media

		libvlc_video_set_callbacks(mediaPlayer, lock, unlock, display, &ctx);
		libvlc_video_set_format_callbacks(mediaPlayer, setup, cleanup);

		return true;
	}
	return false;
}

void VLCPlayer::CallBackAtFrames(ieDword /*cnt*/, ieDword* /*arg*/, ieDword* /*arg2*/ )
{
	// TODO: probably should do something here.
	Log(MESSAGE, "VLCPlayer", "Unimplemented method: CallBackAtFrames");
}

void VLCPlayer::Stop()
{
	libvlc_media_player_stop(mediaPlayer);
	libvlc_media_player_release(mediaPlayer);
}

int VLCPlayer::Play()
{
	int ret = GEM_ERROR;
	Video* video = core->GetVideoDriver();
	if (mediaPlayer && video) {
		ret = libvlc_media_player_play(mediaPlayer);
		if (ret == GEM_OK) {
			// since ret is good we know that playback will start.
			// wait here until it does

			// FIXME: this is not very elegant and infinate loops are not a good thing :)
			// maybe there is a function we can use to check the vlc player status instead of polling until our context is valid
			while (!libvlc_media_player_is_playing(mediaPlayer) || ctx == NULL);

			bool done = false;
			while (!done && libvlc_media_player_is_playing(mediaPlayer)) {
				ctx->Lock();
				done = video->PollMovieEvents();

				if (ctx->isYUV()) {
					unsigned int strides[3];
					strides[0] = ctx->GetStride(0);
					strides[1] = ctx->GetStride(1);
					strides[2] = ctx->GetStride(2);

					void* planes[3];
					planes[0] = ctx->GetPlane(0);
					planes[1] = ctx->GetPlane(1);
					planes[2] = ctx->GetPlane(2);

					// TODO: center the video on the player.
					video->showYUVFrame((unsigned char**)planes, strides, 
										ctx->Width(), ctx->Height(),
										ctx->Width(), ctx->Height(),
										0, 0, 0);
				} else {
					video->showFrame((unsigned char*)ctx->GetPlane(0),
									 ctx->Width(), ctx->Height(), 0, 0,
									 ctx->Width(), ctx->Height(), 0, 0,
									 true, NULL, 0);
				}
				ctx->Unlock();
			}
		}
		Stop();
	}
	return ret;
}

// static vlc callbacks

void VLCPlayer::display(void* /*data*/, void *id){
	assert(id == NULL); // we are using a single buffer so id should always be NULL
}

void VLCPlayer::unlock(void *data, void *id, void *const * /*planes*/){
	assert(id == NULL); // we are using a single buffer so id should always be NULL
	VideoContext* context = *(VideoContext**)data;
	context->Unlock();
}

void* VLCPlayer::lock(void *data, void **planes){
	VideoContext* context = *(VideoContext**)data;
	context->Lock();
	planes[0] = context->GetPlane(0);
	planes[1] = context->GetPlane(1);
	planes[2] = context->GetPlane(2);
	return NULL; // we are using a single buffer so return NULL
}

void VLCPlayer::cleanup(void *opaque)
{
	VideoContext* context = *(VideoContext**)opaque;
	delete context;
	context = NULL;

	core->GetVideoDriver()->DestroyMovieScreen();
}

unsigned VLCPlayer::setup(void **opaque, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines)
{
	Video* video = core->GetVideoDriver();
	assert(video != NULL);

	/*
	 other player plugins assume video is made specifically for IE games (dimensions and chroma).
	 we won't assume this here because this player is mostly for modders to use their own videos.
	 
	 we will need to make sure the video is scaled down if it is larger than the window and we will
	 need to convert the chroma to one of the 2 formats the video driver is coded for.
	 
	 currently the video drivers expect RGB555 or IYUV, but the SDL 2.0 driver can actually handle ARGB as well.
	 infact the texture is natively ARGB8888, and we are converting RGB555 data to ARGB which means here we may be
	 converting like this ARGB -> RGB555 -> ARGB which is quite dumb :)
	 
	 TODO: figure out a way to support ARGB when using SDL 2
	 */

	bool yuv;
	if (strcmp(chroma, "RV16") == 0) { // 16bit RGB
		yuv = false;
	} else {
		yuv = true;
		memcpy(chroma, "YV12", 4);
	}

	int w = *width;
	int h = *height;
	video->InitMovieScreen(w, h, yuv); // may alter w and h

	// TODO: proportionally scale the video to fit the player window
	// for now we'll use the video native width and height.
	w = *width;
	h = *height;

	VideoContext* context = new VideoContext(w, h, yuv);
	**(VideoContext***)opaque = context; // a bit of a hack, but we need to set the player object's context.

	// being lazy and making VLC convert frame pitches and scan lines
	// note that I assume that InitMovieScreen with force w and h into a multiple of 32
	pitches[0] = w;
	pitches[1] = w;
	pitches[2] = w;

	lines[0] = h;
	lines[1] = h;
	lines[2] = h;

	return 1; // indicates the number of buffers allocated
}

#include "plugindef.h"

GEMRB_PLUGIN(0x218963DD, "VLC Video Player")
// TODO: VLC is quite capable of playing various movie formats
// it seems silly to hardcode a single value or add formats piecemeal.
// it would be a shame to force modders or new content creators to use a specific format
PLUGIN_RESOURCE(VLCPlayer, "mov") // at least some mac ports used Quicktime MOV format
END_PLUGIN()
