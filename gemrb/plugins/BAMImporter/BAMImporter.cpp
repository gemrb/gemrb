/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */


#include "BAMImporter.h"

#include "FileCache.h"
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "BAMSprite2D.h"
#include "Video.h"
#include "System/FileStream.h"

#include "System/swab.h"

using namespace GemRB;

BAMImporter::BAMImporter(void)
{
	str = NULL;
	frames = NULL;
	cycles = NULL;
	FramesCount = 0;
	CyclesCount = 0;
	CompressedColorIndex = DataStart = 0;
	FramesOffset = PaletteOffset = FLTOffset = 0;
}

BAMImporter::~BAMImporter(void)
{
	delete str;
	delete[] frames;
	delete[] cycles;
}

bool BAMImporter::Open(DataStream* stream)
{
	unsigned int i;

	if (stream == NULL) {
		return false;
	}
	delete str;
	delete[] frames;
	delete[] cycles;
	frames = nullptr;
	cycles = nullptr;
	palette = nullptr;

	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "BAMCV1  ", 8 ) == 0) {
		str->Seek( 4, GEM_CURRENT_POS );
		DataStream* cached = CacheCompressedStream(stream, stream->filename);
		delete str;
		if (!cached)
			return false;
		str = cached;
		str->Read( Signature, 8 );
	}
	if (strncmp( Signature, "BAM V1  ", 8 ) != 0) {
		return false;
	}
	str->ReadWord( &FramesCount );
	str->Read( &CyclesCount, 1 );
	str->Read( &CompressedColorIndex, 1 );
	str->ReadDword( &FramesOffset );
	str->ReadDword( &PaletteOffset );
	str->ReadDword( &FLTOffset );
	str->Seek( FramesOffset, GEM_STREAM_START );
	frames = new FrameEntry[FramesCount];
	DataStart = str->Size();
	for (i = 0; i < FramesCount; i++) {
		str->ReadWord( &frames[i].Width );
		str->ReadWord( &frames[i].Height );
		str->ReadWord( &frames[i].XPos );
		str->ReadWord( &frames[i].YPos );
		str->ReadDword( &frames[i].FrameData );
		if ((frames[i].FrameData & 0x7FFFFFFF) < DataStart)
			DataStart = (frames[i].FrameData & 0x7FFFFFFF);
	}
	cycles = new CycleEntry[CyclesCount];
	for (i = 0; i < CyclesCount; i++) {
		str->ReadWord( &cycles[i].FramesCount );
		str->ReadWord( &cycles[i].FirstFrame );
	}
	str->Seek( PaletteOffset, GEM_STREAM_START );
	palette = new Palette();
	// no need to switch this
	for (i = 0; i < 256; i++) {
		// bgra format
		str->Read( &palette->col[i].b, 1 );
		str->Read( &palette->col[i].g, 1 );
		str->Read( &palette->col[i].r, 1 );
		unsigned char a;
		str->Read( &a, 1 );

		// BAM v2 (EEs) supports alpha, but for backwards compatibility an alpha of 0 is still 255
		palette->col[i].a = (a) ? a : 255;
	}
	// old bamworkshop semicorrupted shadow entry: recreate a plausible one instead of pink
	if (palette->col[1].r == 255 && palette->col[1].g == 101 && palette->col[1].b == 151) {
		palette->col[1].r = palette->col[1].g = palette->col[1].b = 35;
		palette->col[1].a = 200;
	}

	return true;
}

int BAMImporter::GetCycleSize(unsigned char Cycle)
{
	if(Cycle >= CyclesCount ) {
		return -1;
	}
	return cycles[Cycle].FramesCount;
}

Holder<Sprite2D> BAMImporter::GetFrameInternal(unsigned short findex, unsigned char mode,
										bool RLESprite, unsigned char* data)
{
	Holder<Sprite2D> spr;

	if (RLESprite) {
		assert(data);
		unsigned char* framedata = data;
		framedata += (frames[findex].FrameData & 0x7FFFFFFF) - DataStart;
		spr = new BAMSprite2D (Region(0,0, frames[findex].Width, frames[findex].Height),
							   framedata, palette, CompressedColorIndex);
	} else {
		void* pixels = GetFramePixels(findex);
		Region r(0,0, frames[findex].Width, frames[findex].Height);
		spr = core->GetVideoDriver()->CreateSprite8(r, pixels, palette, true, CompressedColorIndex);
	}

	spr->Frame.x = (ieWordSigned)frames[findex].XPos;
	spr->Frame.y = (ieWordSigned)frames[findex].YPos;
	if (mode == IE_SHADED) {
		// CHECKME: is this ever used? Should we modify the sprite's palette
		// without creating a local copy for this sprite?
		PaletteHolder pal = spr->GetPalette();
		pal->CreateShadedAlphaChannel();
	}
	return spr;
}

void* BAMImporter::GetFramePixels(unsigned short findex)
{
	if (findex >= FramesCount) {
		findex = cycles[0].FirstFrame;
	}
	str->Seek( ( frames[findex].FrameData & 0x7FFFFFFF ), GEM_STREAM_START );
	unsigned long pixelcount = frames[findex].Height * frames[findex].Width;
	void* pixels = malloc( pixelcount );
	bool RLECompressed = ( ( frames[findex].FrameData & 0x80000000 ) == 0 );
	if (RLECompressed) {
		//if RLE Compressed
		unsigned long RLESize;
		RLESize = ( unsigned long )
			( frames[findex].Width * frames[findex].Height * 3 ) / 2 + 1;
		//without partial reads, we should be careful
		unsigned long remains = str->Remains();
		if (RLESize > remains) {
			RLESize = remains;
		}
		unsigned char* inpix;
		inpix = (unsigned char*)malloc( RLESize );
		if (str->Read( inpix, RLESize ) == GEM_ERROR) {
			free( pixels );
			free( inpix );
			return NULL;
		}
		unsigned char * p = inpix;
		unsigned char * Buffer = (unsigned char*)pixels;
		unsigned int i = 0;
		while (i < pixelcount) {
			if (*p == CompressedColorIndex) {
				p++;
				// FIXME: Czech HOW has apparently broken frame
				// #141 in REALMS.BAM. Maybe we should put
				// this condition to #ifdef BROKEN_xx ?
				// Or maybe rather put correct REALMS.BAM
				// into override/ dir?
				if (i + ( *p ) + 1 > pixelcount) {
					memset( &Buffer[i], CompressedColorIndex, pixelcount - i );
					print("Broken frame %d", findex);
				} else {
					memset( &Buffer[i], CompressedColorIndex, ( *p ) + 1 );
				}
				i += *p;
			} else 
				Buffer[i] = *p;
			p++;
			i++;
		}
		free( inpix );
	} else {
		str->Read( pixels, pixelcount );
	}
	return pixels;
}

ieWord * BAMImporter::CacheFLT(unsigned int &count)
{
	count = 0;
	for (int i = 0; i < CyclesCount; i++) {
		unsigned int tmp = cycles[i].FirstFrame + cycles[i].FramesCount;
		if (tmp > count) {
			count = tmp;
		}
	}
	if (count == 0) return NULL;

	ieWord * FLT = ( ieWord * ) calloc( count, sizeof(ieWord) );
	str->Seek( FLTOffset, GEM_STREAM_START );
	str->Read( FLT, count * sizeof(ieWord) );
	if( DataStream::BigEndian() ) {
		swabs(FLT, count * sizeof(ieWord));
	}
	return FLT;
}

AnimationFactory* BAMImporter::GetAnimationFactory(const char* ResRef, unsigned char mode, bool allowCompression)
{
	unsigned int i, count;
	AnimationFactory* af = new AnimationFactory( ResRef );
	ieWord *FLT = CacheFLT( count );

	allowCompression = allowCompression && core->GetVideoDriver()->SupportsBAMSprites();
	unsigned char* data = NULL;

	if (allowCompression) {
		str->Seek( DataStart, GEM_STREAM_START );
		unsigned long length = str->Remains();
		if (length == 0) return af;
		//data = new unsigned char[length];
		data = (unsigned char *) malloc(length);
		str->Read( data, length );
		af->SetFrameData(data);
	}

	for (i = 0; i < FramesCount; ++i) {
		bool RLECompressed = allowCompression && (frames[i].FrameData & 0x80000000) == 0;
		Holder<Sprite2D> frame = GetFrameInternal(i, mode, RLECompressed, data);
		assert(!RLECompressed || frame->BAM);
		af->AddFrame(frame);
	}
	for (i = 0; i < CyclesCount; ++i) {
		af->AddCycle( cycles[i] );
	}
	af->LoadFLT ( FLT, count );
	free (FLT);
	return af;
}

/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
If the Global Animation Palette is NULL, returns NULL. */
Holder<Sprite2D> BAMImporter::GetPalette()
{
	unsigned char * pixels = ( unsigned char * ) malloc( 256 );
	unsigned char * p = pixels;
	for (int i = 0; i < 256; i++) {
		*p++ = ( unsigned char ) i;
	}
	return core->GetVideoDriver()->CreateSprite8(Region(0,0,16,16), pixels, palette );
}

#include "BAMFontManager.h"

#include "plugindef.h"

GEMRB_PLUGIN(0x3AD6427A, "BAM File Importer")
PLUGIN_IE_RESOURCE(BAMFontManager, "bam", (ieWord)IE_BAM_CLASS_ID)
PLUGIN_CLASS(IE_BAM_CLASS_ID, BAMImporter)
END_PLUGIN()
