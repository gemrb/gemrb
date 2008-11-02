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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../Core/Interface.h"
#include "../Core/Video.h"
#include "../Core/Audio.h"
#include "../Core/Variables.h"
#include "MVEPlay.h"
#include "libmve.h"
#include "../../includes/ie_types.h"

static const char MVESignature[] = "Interplay MVE File\x1A";
static const int MVE_SIGNATURE_LEN = 19;

static Video *video = NULL;
static unsigned char g_palette[768];
static int g_truecolor;
static ieDword maxRow = 0;
static ieDword rowCount = 0;
static ieDword frameCount = 0;
static ieDword *cbAtFrame = NULL;
static ieDword *strRef = NULL;

MVEPlay::MVEPlay(void)
{
	str = NULL;
	autoFree = false;
	video = core->GetVideoDriver();
}

MVEPlay::~MVEPlay(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

/** the crudest hack ever been made :), we should really beg for a */
/** BIK player, miracles happen */
bool MVEPlay::PlayBik(DataStream *stream)
{
	char Tmp[256];

	sprintf(Tmp,"%stmp.bik",core->CachePath);
	unlink(Tmp);
	int fhandle = open(Tmp,O_CREAT|O_TRUNC|O_RDWR, S_IWRITE|S_IREAD);
	if (fhandle<1) {
		return false;
	}
	stream->Seek(0,GEM_STREAM_START);
	int size=stream->Size();
	printf("Copying %d bytes... to %s\n",size, Tmp);
	while(size) {
		int chunk = size>256?256:size;
		stream->Read(Tmp, chunk);
		write(fhandle,Tmp, chunk);
		size -= chunk;
	}
	close(fhandle);
#ifdef WIN32
	sprintf(Tmp,"BinkPlayer.exe %stmp.bik",core->CachePath);
#else
	sprintf(Tmp,"BinkPlayer %stmp.bik",core->CachePath);
#endif
	int ret=system(Tmp);
	sprintf(Tmp,"%stmp.bik",core->CachePath);
	unlink(Tmp);
	return ret!=0;
}

bool MVEPlay::Open(DataStream* stream, bool autoFree)
{
	validVideo = false;
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[MVE_SIGNATURE_LEN];
	str->Read( Signature, MVE_SIGNATURE_LEN );
	if (memcmp( Signature, MVESignature, MVE_SIGNATURE_LEN ) != 0) {
		if (memcmp( Signature, "BIK", 3 ) == 0) {
			if( PlayBik(stream) ) {
				printf( "Warning!!! This is a Bink Video File...\nUnfortunately we cannot provide a Bink Video Player\nWe are sorry!\n" );
			}
			return true;
		}
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
	doPlay( str );
	return 0;
}

int MVEPlay::doPlay(const DataStream* mve)
{
	int result;
	int done = 0;
	int bpp = 0;
	MVE_videoSpec vSpec;

	memset( g_palette, 0, 768 );

	ieDword volume;
	core->GetDictionary()->Lookup( "Volume Movie", volume );
	MVE_sndInit( core->GetAudioDrv()->CanPlay()?1:-1, volume );
	MVE_memCallbacks( malloc, free );
	MVE_ioCallbacks( fileRead );
	MVE_sfCallbacks( showFrame );
	MVE_palCallbacks( setPalette );
	MVE_audioCallbacks( setAudioStream, freeAudioStream, queueBuffer ) ;

	int w,h;

	video->InitMovieScreen(w,h);
	MVE_rmPrepMovie( ( void * ) mve, -1, -1, 1 );

	vSpec.screenWidth = w;
	vSpec.screenHeight = h;

	MVE_getVideoSpec( &vSpec );
	bpp = vSpec.truecolor ? 16 : 8;

	g_truecolor = vSpec.truecolor;

	while (!done && ( result = MVE_rmStepMovie() ) == 0) {
		done = video->PollMovieEvents();
	}

	MVE_rmEndMovie();

	return 0;
}

unsigned int MVEPlay::fileRead(void* handle, void* buf, unsigned int count)
{
	unsigned numread;

	numread = ( ( DataStream * ) handle )->Read( buf, count );//fread(buf, 1, count, (FILE *)handle);
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
    return core->GetAudioDrv()->SetupNewStream(0, 0, 0, volume,
                                            false, false) ;
}

void MVEPlay::freeAudioStream(int stream)
{
    core->GetAudioDrv()->ReleaseStream(stream, true);
}

void MVEPlay::queueBuffer(int stream, unsigned short bits,
                int channels, short* memory,
                int size, int samplerate)
{
    core->GetAudioDrv()->QueueBuffer(stream, bits, channels,
                memory, size, samplerate) ;
}

