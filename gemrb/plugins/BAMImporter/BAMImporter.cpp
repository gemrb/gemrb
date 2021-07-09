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
	str->ReadWord(FramesCount);
	str->Read( &CyclesCount, 1 );
	str->Read( &CompressedColorIndex, 1 );
	str->ReadDword(FramesOffset);
	str->ReadDword(PaletteOffset);
	str->ReadDword(FLTOffset);
	str->Seek( FramesOffset, GEM_STREAM_START );
	
	frames = new FrameEntry[FramesCount];
	DataStart = str->Size();
	for (i = 0; i < FramesCount; i++) {
		ieWord w, h;
		ieWordSigned x , y;
		str->ReadScalar(w);
		frames[i].bounds.w = w;
		str->ReadScalar(h);
		frames[i].bounds.h = h;
		str->ReadScalar(x);
		frames[i].bounds.x = x;
		str->ReadScalar(y);
		frames[i].bounds.y = y;
		ieDword offset;
		str->ReadScalar(offset);
		frames[i].RLE = (offset & 0x80000000) == 0;
		frames[i].dataOffset = offset & 0x7FFFFFFF;
		DataStart = std::min(DataStart, frames[i].dataOffset);
		
		if (!frames[i].RLE) {
			frames[i].dataLength = w * h;
		} else if (i > 0) {
			assert(frames[i].dataOffset > frames[i - 1].dataOffset);
			frames[i - 1].dataLength = frames[i].dataOffset - frames[i - 1].dataOffset;
		}
	}
	
	if (frames[FramesCount - 1].RLE)
		frames[FramesCount - 1].dataLength = str->Size() - frames[FramesCount - 1].dataOffset;
	
	cycles = new CycleEntry[CyclesCount];
	for (unsigned int i = 0; i < CyclesCount; i++) {
		str->ReadWord(cycles[i].FramesCount);
		str->ReadWord(cycles[i].FirstFrame);
	}
	str->Seek( PaletteOffset, GEM_STREAM_START );
	palette = new Palette();
	// no need to switch this
	for (auto& color : palette->col) {
		// bgra format
		str->Read(&color.b, 1);
		str->Read(&color.g, 1);
		str->Read(&color.r, 1);
		unsigned char a;
		str->Read( &a, 1 );

		// BAM v2 (EEs) supports alpha, but for backwards compatibility an alpha of 0 is still 255
		color.a = a ? a : 255;
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

Holder<Sprite2D> BAMImporter::GetFrameInternal(unsigned short findex, bool RLESprite,
											   uint8_t* data, uint8_t* end)
{
	Holder<Sprite2D> spr;
	Video* video = core->GetVideoDriver();
	Region r(0,0, frames[findex].bounds.w, frames[findex].bounds.h);
	ptrdiff_t dataLen = end - data;

	if (RLESprite && dataLen) {
		assert(data);
		unsigned char* framedata = data;
		framedata += frames[findex].dataOffset - DataStart;
		void* pixels = malloc(dataLen);
		memcpy(pixels, data, dataLen);
		spr = video->CreateSprite(r, pixels, PixelFormat::RLE8Bit(palette, CompressedColorIndex));
	} else {
		void* pixels = GetFramePixels(findex);
		PixelFormat fmt = PixelFormat::Paletted8Bit(palette, true, CompressedColorIndex);
		spr = video->CreateSprite(r, pixels, fmt);
	}

	spr->Frame.x = (ieWordSigned)frames[findex].bounds.x;
	spr->Frame.y = (ieWordSigned)frames[findex].bounds.y;
	return spr;
}

void* BAMImporter::GetFramePixels(unsigned short findex)
{
	if (findex >= FramesCount) {
		findex = cycles[0].FirstFrame;
	}
	
	//const FrameEntry& frameInfo = frames[findex];
	
	str->Seek(frames[findex].dataOffset, GEM_STREAM_START );
	unsigned long pixelcount = frames[findex].bounds.h * frames[findex].bounds.w;
	void* pixels = malloc( pixelcount );
	if (frames[findex].RLE) {
		//if RLE Compressed
		unsigned long RLESize;
		RLESize = ( unsigned long )
			( frames[findex].bounds.w * frames[findex].bounds.h * 3 ) / 2 + 1;
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

AnimationFactory* BAMImporter::GetAnimationFactory(const char* ResRef, bool allowCompression)
{
	unsigned int i, count;
	AnimationFactory* af = new AnimationFactory( ResRef );
	ieWord *FLT = CacheFLT( count );

	unsigned char* data = NULL;

	str->Seek( DataStart, GEM_STREAM_START );
	unsigned long length = str->Remains();
	if (length == 0) return af;
	//data = new unsigned char[length];
	data = (unsigned char *) malloc(length);
	str->Read( data, length );

	for (i = 0; i < FramesCount; ++i) {
		const FrameEntry& frameInfo = frames[i];
		bool RLECompressed = allowCompression && frameInfo.RLE;
		uint8_t* frameData = data + frameInfo.dataOffset - DataStart;
		Holder<Sprite2D> frame = GetFrameInternal(i, RLECompressed, frameData, frameData + frameInfo.dataLength);
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
	PixelFormat fmt = PixelFormat::Paletted8Bit(palette);
	return core->GetVideoDriver()->CreateSprite(Region(0,0,16,16), pixels, fmt);
}

#include "BAMFontManager.h"

#include "plugindef.h"

GEMRB_PLUGIN(0x3AD6427A, "BAM File Importer")
PLUGIN_IE_RESOURCE(BAMFontManager, "bam", (ieWord)IE_BAM_CLASS_ID)
PLUGIN_CLASS(IE_BAM_CLASS_ID, BAMImporter)
END_PLUGIN()
