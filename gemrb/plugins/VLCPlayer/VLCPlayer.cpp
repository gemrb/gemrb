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

#include "Video/Video.h"

using namespace GemRB;

VLCPlayer::VLCPlayer(void)
{
	libvlc = libvlc_new(0, nullptr);
}

VLCPlayer::~VLCPlayer(void)
{
	DestroyPlayer();
	libvlc_media_player_release(mediaPlayer);
	libvlc_release(libvlc);
}

bool VLCPlayer::Import(DataStream* stream)
{
	DestroyPlayer();
	if (stream) {
		// we don't actually need anything from the stream. libVLC will open and use the file internally
		libvlc_media_t* media = libvlc_media_new_path(libvlc, stream->originalfile.c_str());
		mediaPlayer = libvlc_media_player_new_from_media(media);
		libvlc_media_release(media); //player retains the media

		libvlc_video_set_callbacks(mediaPlayer, lock, nullptr, nullptr, this);
		libvlc_video_set_format_callbacks(mediaPlayer, setup, nullptr);

		bool success = libvlc_media_player_play(mediaPlayer) == 0;

		// FIXME: this is technically a data race!
		while (success && movieFormat == Video::BufferFormat::DISPLAY);

		return success;
	}
	return false;
}

bool VLCPlayer::DecodeFrame(VideoBuffer& buf)
{
	int pitches[3];

	switch (movieFormat) {
		case Video::BufferFormat::RGB555:
			pitches[0] = movieSize.w * 2;
			break;
		case Video::BufferFormat::YV12:
			pitches[Y] = movieSize.w;
			pitches[U] = movieSize.w / 2;
			pitches[V] = movieSize.w / 2;
			break;
		default: // 32 bit
			pitches[0] = movieSize.w * 4;
			break;
	}

	buf.CopyPixels(Region(0, 0, movieSize.w, movieSize.h),
		       planes[0], &pitches[0], // Y or RGB
		       planes[1], &pitches[1], // U
		       planes[2], &pitches[2]); // V
	return true;
}

void VLCPlayer::DestroyPlayer()
{
	if (mediaPlayer) {
		libvlc_media_player_stop(mediaPlayer);
		libvlc_media_player_release(mediaPlayer);
	}

	for (auto& plane : planes) {
		delete[] plane;
	}
}

// static vlc callbacks

void* VLCPlayer::lock(void* data, void** planes)
{
	const VLCPlayer* player = static_cast<const VLCPlayer*>(data);

	planes[0] = player->planes[0];
	planes[1] = player->planes[1];
	planes[2] = player->planes[2];

	return nullptr; // we are using a single buffer so return nullptr
}

unsigned VLCPlayer::setup(void** opaque, char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines)
{
	VLCPlayer* player = static_cast<VLCPlayer*>(*opaque);
	int w = *width;
	int h = *height;
	player->movieSize.w = w;
	player->movieSize.h = h;

	if (strcmp(chroma, "RV16") == 0) { // 16bit RGB
		player->movieFormat = Video::BufferFormat::RGB555;

		pitches[0] = w * 2;
		lines[0] = h;

		player->planes[0] = new char[pitches[0] * lines[0]];
	} else if (strcmp(chroma, "YV12") == 0 || strcmp(chroma, "I420") == 0) {
		player->movieFormat = Video::BufferFormat::YV12;
		memcpy(chroma, "YV12", 4); // we prefer this plane order

		pitches[Y] = w;
		pitches[U] = w / 2;
		pitches[V] = w / 2;
		lines[Y] = h;
		lines[U] = h / 2;
		lines[V] = h / 2;

		player->planes[Y] = new char[pitches[Y] * lines[Y]];
		player->planes[U] = new char[pitches[U] * lines[U]];
		player->planes[V] = new char[pitches[V] * lines[V]];
	} else { // default to 32bit
		player->movieFormat = Video::BufferFormat::RGBA8888;
		memcpy(chroma, "RV32", 4);

		pitches[0] = w * 4;
		lines[0] = h;

		player->planes[0] = new char[pitches[0] * lines[0]];
	}

	return 1; // indicates the number of buffers allocated
}

#include "plugindef.h"

GEMRB_PLUGIN(0x218963DD, "VLC Video Player")
// TODO: VLC is quite capable of playing various movie formats
// it seems silly to hardcode a single value or add formats piecemeal.
// it would be a shame to force modders or new content creators to use a specific format
PLUGIN_RESOURCE(VLCPlayer, "mov") // at least some mac ports used Quicktime MOV format
PLUGIN_RESOURCE(VLCPlayer, "wbm") // EE movies
END_PLUGIN()
