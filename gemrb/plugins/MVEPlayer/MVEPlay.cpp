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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/MVEPlayer/MVEPlay.cpp,v 1.16 2005/03/25 21:30:39 avenger_teambg Exp $
 *
 */

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../Core/Interface.h"
#include "MVEPlay.h"
#include "libmve.h"
#include "../../includes/ie_types.h"

static const char MVESignature[] = "Interplay MVE File\x1A";
static const int MVE_SIGNATURE_LEN = 19;

static SDL_Surface* g_screen = NULL;
static unsigned char g_palette[768];
static int g_truecolor;

MVEPlay::MVEPlay(void)
{
	str = NULL;
	autoFree = false;
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

int MVEPlay::Play()
{
	if (!validVideo) {
		return 0;
	}
	//Start Movie Playback
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
	MVE_sndInit( core->GetSoundMgr()->CanPlay()?1:-1, volume );
	MVE_memCallbacks( malloc, free );
	MVE_ioCallbacks( fileRead );
	MVE_sfCallbacks( showFrame );
	MVE_palCallbacks( setPalette );

	g_screen = ( SDL_Surface * ) core->GetVideoDriver()->GetVideoSurface();
	SDL_LockSurface( g_screen );
	memset( g_screen->pixels, 0,
		g_screen->w * g_screen->h * g_screen->format->BytesPerPixel );
	SDL_UnlockSurface( g_screen );
	SDL_Flip( g_screen );

	MVE_rmPrepMovie( ( void * ) mve, -1, -1, 1 );

	MVE_getVideoSpec( &vSpec );

	bpp = vSpec.truecolor ? 16 : 8;

	g_truecolor = vSpec.truecolor;

	while (!done && ( result = MVE_rmStepMovie() ) == 0) {
		done = pollEvents();
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
	int i;
	unsigned char * pal;
	SDL_Surface* sprite;
	SDL_Rect srcRect, destRect;

	assert( bufw == w && bufh == h );

	if (g_truecolor) {
		sprite = SDL_CreateRGBSurfaceFrom( buf, bufw, bufh, 16, 2 * bufw,
					0x7C00, 0x03E0, 0x001F, 0 );
	} else {
		sprite = SDL_CreateRGBSurfaceFrom( buf, bufw, bufh, 8, bufw, 0x7C00,
					0x03E0, 0x001F, 0 );

		pal = g_palette;
		for (i = 0; i < 256; i++) {
			sprite->format->palette->colors[i].r = ( *pal++ ) << 2;
			sprite->format->palette->colors[i].g = ( *pal++ ) << 2;
			sprite->format->palette->colors[i].b = ( *pal++ ) << 2;
			sprite->format->palette->colors[i].unused = 0;
		}
	}

	srcRect.x = sx;
	srcRect.y = sy;
	srcRect.w = w;
	srcRect.h = h;
	destRect.x = dstx;
	destRect.y = dsty;
	destRect.w = w;
	destRect.h = h;

	SDL_BlitSurface( sprite, &srcRect, g_screen, &destRect );
	if (( g_screen->flags & SDL_DOUBLEBUF ) == SDL_DOUBLEBUF) {
		SDL_Flip( g_screen );
	} else {
		SDL_UpdateRects( g_screen, 1, &destRect );
	}
	SDL_FreeSurface( sprite );
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

int MVEPlay::pollEvents()
{
	SDL_Event event;

	while (SDL_PollEvent( &event )) {
		switch (event.type) {
			case SDL_QUIT:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				return 1;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:
					case SDLK_q:
						return 1;
					case SDLK_f:
						SDL_WM_ToggleFullScreen( g_screen );
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}

	return 0;
}
