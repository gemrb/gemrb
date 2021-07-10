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
#include "Video/Video.h"
#include "Video/RLE.h"
#include "System/FileStream.h"

#include "System/swab.h"

using namespace GemRB;

BAMImporter::BAMImporter(void)
{
	str = NULL;
	cycles = NULL;
	FramesCount = 0;
	CyclesCount = 0;
	CompressedColorIndex = DataStart = 0;
	FramesOffset = PaletteOffset = FLTOffset = 0;
}

BAMImporter::~BAMImporter(void)
{
	delete str;
	delete[] cycles;
}

bool BAMImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	delete[] cycles;
	frames.clear();
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
	
	frames.resize(FramesCount);
	DataStart = str->Size();
	for (auto& frame : frames) {
		ieWord w, h;
		ieWordSigned x , y;
		str->ReadScalar(w);
		frame.bounds.w = w;
		str->ReadScalar(h);
		frame.bounds.h = h;
		str->ReadScalar(x);
		frame.bounds.x = x;
		str->ReadScalar(y);
		frame.bounds.y = y;
		ieDword offset;
		str->ReadScalar(offset);
		frame.RLE = (offset & 0x80000000) == 0;
		frame.dataOffset = offset & 0x7FFFFFFF;
		DataStart = std::min(DataStart, frame.dataOffset);
	}
	
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

Holder<Sprite2D> BAMImporter::GetFrameInternal(const FrameEntry& frameInfo, bool RLESprite, uint8_t* data)
{
	Holder<Sprite2D> spr;
	Video* video = core->GetVideoDriver();
	const Region& rgn = frameInfo.bounds;
	uint8_t* dataBegin = data + frameInfo.dataOffset;
	
	if (RLESprite) {
		PixelFormat fmt = PixelFormat::RLE8Bit(palette, CompressedColorIndex);
		uint8_t* dataEnd = FindRLEPos(dataBegin, rgn.w, Point(rgn.w, rgn.h - 1), CompressedColorIndex);
		ptrdiff_t dataLen = dataEnd - dataBegin;
		void* pixels = malloc(dataLen);
		memcpy(pixels, dataBegin, dataLen);
		spr = video->CreateSprite(rgn, pixels, fmt);
	} else {
		void* pixels = nullptr;
		if (frameInfo.RLE) {
			pixels = DecodeRLEData(dataBegin, rgn.size, CompressedColorIndex);
		} else {
			pixels = malloc(rgn.w * rgn.h);
			memcpy(pixels, dataBegin, rgn.w * rgn.h);
		}
		PixelFormat fmt = PixelFormat::Paletted8Bit(palette, true, CompressedColorIndex);
		spr = video->CreateSprite(rgn, pixels, fmt);
	}

	return spr;
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

	str->Seek( DataStart, GEM_STREAM_START );
	unsigned long length = str->Remains();
	if (length == 0) return af;
	uint8_t *data = (uint8_t*)malloc(length);
	str->Read(data, length);

	for (i = 0; i < FramesCount; ++i) {
		const FrameEntry& frameInfo = frames[i];
		bool RLECompressed = allowCompression && frameInfo.RLE;
		Holder<Sprite2D> frame = GetFrameInternal(frameInfo, RLECompressed, data - DataStart);
		af->AddFrame(frame);
	}
	for (i = 0; i < CyclesCount; ++i) {
		af->AddCycle( cycles[i] );
	}
	af->LoadFLT ( FLT, count );
	free(data);
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
