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

#include "mve_player.h"

#include "ie_types.h"

#include "Audio.h"
#include "Interface.h"
#include "Variables.h"
#include "Video.h"

#include <cassert>
#include <cstdio>

static const char MVESignature[] = "Interplay MVE File\x1A";
static const int MVE_SIGNATURE_LEN = 19;

static unsigned char g_palette[768];
static int g_truecolor;
static ieDword maxRow = 0;
static ieDword rowCount = 0;
static ieDword frameCount = 0;
static ieDword *cbAtFrame = NULL;
static ieDword *strRef = NULL;

MVEPlay::MVEPlay(void)
{
	video = core->GetVideoDriver();
}

MVEPlay::~MVEPlay(void)
{
}

bool MVEPlay::Open(DataStream* stream)
{
	str = stream;
	validVideo = false;

	char Signature[MVE_SIGNATURE_LEN];
	str->Read( Signature, MVE_SIGNATURE_LEN );
	if (memcmp( Signature, MVESignature, MVE_SIGNATURE_LEN ) != 0) {
		return false;
	}

	str->Seek( 0, GEM_STREAM_START );
	validVideo = true;
	return true;
}

void MVEPlay::CallBackAtFrames(ieDword cnt, ieDword *arg, ieDword *arg2 )
{
	maxRow = cnt;
	frameCount = 0;
	rowCount = 0;
	cbAtFrame = arg;
	strRef = arg2;
}

int MVEPlay::Play()
{
	if (!validVideo) {
		return 0;
	}
	//Start Movie Playback
	frameCount = 0;
	return doPlay( );
}

int MVEPlay::doPlay()
{
	int done = 0;
	MVEPlayer player(this);

	memset( g_palette, 0, 768 );

	//ieDword volume;
	//core->GetDictionary()->Lookup( "Volume Movie", volume );
	player.sound_init( core->GetAudioDrv()->CanPlay() );

	int w,h;

	video->InitMovieScreen(w,h);
	player.video_init(w, h);

	if (!player.start_playback()) {
		print("Failed to decode movie!");
		return 1;
	}

	g_truecolor = player.is_truecolour();

	while (!done && player.next_frame()) {
		done = video->PollMovieEvents();
	}

	video->DrawMovieSubtitle(0);
	return 0;
}

unsigned int MVEPlay::fileRead(void* buf, unsigned int count)
{
	unsigned numread;

	numread = str->Read( buf, count );
	return ( numread == count );
}

void MVEPlay::showFrame(unsigned char* buf, unsigned int bufw,
	unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
	unsigned int h, unsigned int dstx, unsigned int dsty)
{
	ieDword titleref = 0;

	if (cbAtFrame && strRef) {
		frameCount ++;
		if ((rowCount<maxRow) && (frameCount >= cbAtFrame[rowCount]) ) {
			rowCount++;
		}
		//draw subtitle here
		if (rowCount) {
			titleref = strRef[rowCount-1];
		}
	}
	video->showFrame(buf,bufw,bufh,sx,sy,w,h,dstx,dsty, g_truecolor, g_palette, titleref);
}

void MVEPlay::setPalette(unsigned char* p, unsigned start, unsigned count)
{
	//Set color 0 to be black
	g_palette[0] = g_palette[1] = g_palette[2] = 0;

	//Set color 255 to be our subtitle color
	g_palette[765] = g_palette[766] = g_palette[767] = 50;

	//movie libs palette into our array
	memcpy( g_palette + start * 3, p + start * 3, count * 3 );
}

int MVEPlay::setAudioStream()
{
	ieDword volume ;
	core->GetDictionary()->Lookup( "Volume Movie", volume) ;
	int source = core->GetAudioDrv()->SetupNewStream(0, 0, 0, volume, false, false) ;
	return source;
}

void MVEPlay::freeAudioStream(int stream)
{
	if (stream > -1)
		core->GetAudioDrv()->ReleaseStream(stream, true);
}

void MVEPlay::queueBuffer(int stream, unsigned short bits,
			int channels, short* memory,
			int size, int samplerate)
{
	if (stream > -1)
		core->GetAudioDrv()->QueueBuffer(stream, bits, channels, memory, size, samplerate) ;
}


#include "plugindef.h"

GEMRB_PLUGIN(0x218963DC, "MVE Video Player")
PLUGIN_IE_RESOURCE(MVEPlay, "mve", (ieWord)IE_MVE_CLASS_ID)
END_PLUGIN()
