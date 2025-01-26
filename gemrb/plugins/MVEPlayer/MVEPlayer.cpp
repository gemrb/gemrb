/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#include "MVEPlayer.h"

#include "ie_types.h"

#include "Interface.h"
#include "Palette.h"

#include "Logging/Logging.h"
#include "Video/Video.h"

#include <cassert>
#include <cstdio>

using namespace GemRB;

static const char MVESignature[] = "Interplay MVE File\x1A";
static const int MVE_SIGNATURE_LEN = 19;

MVEPlay::MVEPlay() noexcept
	: decoder(this)
{
	video = VideoDriver;
	validVideo = false;
	vidBuf = NULL;
	g_palette = MakeHolder<Palette>();

	// these colors don't change
	g_palette->SetColor(1, ColorBlack);
	//Set color 255 to be our subtitle color
	g_palette->SetColor(255, Color(50, 50, 50, 255));
}

bool MVEPlay::Import(DataStream* str)
{
	validVideo = false;

	char Signature[MVE_SIGNATURE_LEN];
	str->Read(Signature, MVE_SIGNATURE_LEN);
	if (memcmp(Signature, MVESignature, MVE_SIGNATURE_LEN) != 0) {
		return false;
	}

	str->Seek(0, GEM_STREAM_START);

	validVideo = decoder.start_playback();
	return validVideo;
}

bool MVEPlay::DecodeFrame(VideoBuffer& buf)
{
	vidBuf = &buf;
	++framePos;
	return (validVideo && decoder.next_frame());
}

unsigned int MVEPlay::fileRead(void* buf, unsigned int count)
{
	strret_t numread = str->Read(buf, count);
	return (numread == count);
}

void MVEPlay::showFrame(const unsigned char* buf, unsigned int bufw, unsigned int bufh)
{
	if (vidBuf == NULL) {
		Log(WARNING, "MVEPlayer", "attempting to decode a frame without a video buffer (most likely during init).");
		return;
	}
	Size s = vidBuf->Size();
	int dest_x = unsigned(s.w - bufw) >> 1;
	int dest_y = unsigned(s.h - bufh) >> 1;
	vidBuf->CopyPixels(Region(dest_x, dest_y, bufw, bufh), buf, NULL, g_palette.get());
}

void MVEPlay::setPalette(unsigned char* p, unsigned start, unsigned count) const
{
	p = p + (start * 3);
	Palette::Colors buffer;
	for (unsigned int i = start; i < start + count; i++) {
		buffer[i].r = (*p++) << 2;
		buffer[i].g = (*p++) << 2;
		buffer[i].b = (*p++) << 2;
		buffer[i].a = 0xff;
	}
	g_palette->CopyColors(start, buffer.cbegin() + start, buffer.cbegin() + start + count);
}

Holder<SoundStreamSourceHandle> MVEPlay::setAudioStream() const
{
	return core->GetAudioDrv()->CreateStreamable(core->GetAudioSettings().ConfigPresetMovie());
}

void MVEPlay::queueBuffer(Holder<SoundStreamSourceHandle> handle, unsigned short bits,
			  int channels, short* memory,
			  int size, int samplerate) const
{
	if (handle) {
		AudioBufferFormat format;
		format.bits = bits;
		format.channels = channels;
		format.sampleRate = samplerate;

		if (!handle->Feed(format, reinterpret_cast<char*>(memory), size)) {
			Log(WARNING, "MVEPlayer", "Audio backend can't keep up.");
		}
	}
}


#include "plugindef.h"

GEMRB_PLUGIN(0x218963DC, "MVE Video Player")
PLUGIN_IE_RESOURCE(MVEPlay, "mve", (ieWord) IE_MVE_CLASS_ID)
END_PLUGIN()
