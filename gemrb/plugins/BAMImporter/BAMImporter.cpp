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

#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Video/Video.h"
#include "Video/RLE.h"
#include "Streams/FileStream.h"

using namespace GemRB;

bool BAMImporter::Import(DataStream* str)
{
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "BAMCV1  ", 8 ) == 0) {
		str->Seek( 4, GEM_CURRENT_POS );
		str = DecompressStream(str);
		if (!str)
			return false;
		str->Read( Signature, 8 );
	}
	if (strncmp( Signature, "BAM V1  ", 8 ) != 0) {
		return false;
	}

	ieWord frameCount;
	str->ReadWord(frameCount);
	frames.resize(frameCount);

	ieByte cycleCount;
	str->Read(&cycleCount, 1);
	cycles.resize(cycleCount);
	
	str->Read( &CompressedColorIndex, 1 );
	str->ReadDword(FramesOffset);
	str->ReadDword(PaletteOffset);
	str->ReadDword(FLTOffset);
	str->Seek( FramesOffset, GEM_STREAM_START );
	
	DataStart = str->Size();
	for (auto& frame : frames) {
		// ReadRegion is ordered x,y,w,h
		// for some reason these rects are w,h,x,y
		str->ReadSize(frame.bounds.size);
		str->ReadPoint(frame.bounds.origin);
		ieDword offset;
		str->ReadScalar(offset);
		frame.RLE = (offset & 0x80000000) == 0;
		frame.dataOffset = offset & 0x7FFFFFFF;
		DataStart = std::min(DataStart, frame.dataOffset);
	}
	
	for (auto& cycle : cycles) {
		str->ReadWord(cycle.FramesCount);
		str->ReadWord(cycle.FirstFrame);
	}
	str->Seek( PaletteOffset, GEM_STREAM_START );
	palette = MakeHolder<Palette>();
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

BAMImporter::index_t BAMImporter::GetCycleSize(index_t cycle)
{
	if (cycle >= cycles.size()) {
		return AnimationFactory::InvalidIndex;
	}
	return cycles[cycle].FramesCount;
}

Holder<Sprite2D> BAMImporter::GetFrameInternal(const FrameEntry& frameInfo, bool RLESprite, uint8_t* data)
{
	Holder<Sprite2D> spr;
	Video* video = core->GetVideoDriver();
	const Region& rgn = frameInfo.bounds;
	uint8_t* dataBegin = data + frameInfo.dataOffset;
	
	if (RLESprite) {
		PixelFormat fmt = PixelFormat::RLE8Bit(palette, CompressedColorIndex);
		const uint8_t* dataEnd = FindRLEPos(dataBegin, rgn.w, Point(rgn.w, rgn.h - 1), CompressedColorIndex);
		ptrdiff_t dataLen = dataEnd - dataBegin;
		if (dataLen == 0) return nullptr;
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

std::vector<BAMImporter::index_t> BAMImporter::CacheFLT()
{
	index_t count = 0;
	for (const auto& cycle : cycles) {
		index_t tmp = cycle.FirstFrame + cycle.FramesCount;
		if (tmp > count) {
			count = tmp;
		}
	}
	if (count == 0) return {};

	std::vector<index_t> FLT(count);
	str->Seek( FLTOffset, GEM_STREAM_START );
	str->Read(&FLT[0], count * sizeof(ieWord));
	return FLT;
}

AnimationFactory* BAMImporter::GetAnimationFactory(const ResRef &resref, bool allowCompression)
{
	str->Seek( DataStart, GEM_STREAM_START );
	strpos_t length = str->Remains();
	if (length == 0) return nullptr;
	
	auto FLT = CacheFLT();
	uint8_t *data = (uint8_t*)malloc(length);
	str->Read(data, length);

	std::vector<Holder<Sprite2D>> animframes;
	for (const auto& frameInfo : frames) {
		bool RLECompressed = allowCompression && frameInfo.RLE;
		animframes.push_back(GetFrameInternal(frameInfo, RLECompressed, data - DataStart));
	}
	free(data);

	return new AnimationFactory(resref, std::move(animframes), cycles, std::move(FLT));
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
PLUGIN_CLASS(IE_BAM_CLASS_ID, ImporterPlugin<BAMImporter>)
END_PLUGIN()
