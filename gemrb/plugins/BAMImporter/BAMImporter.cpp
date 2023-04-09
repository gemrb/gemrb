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
#include "PluginMgr.h"
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

	version = BAMVersion::V1;
	if (strncmp(Signature, "BAM V2  ", 8) == 0) {
		version = BAMVersion::V2;
	} else if (strncmp(Signature, "BAM V1  ", 8) != 0) {
		return false;
	}

	ieDword frameCount;
	if (version == BAMVersion::V1) {
		str->ReadScalar<ieDword, ieWord>(frameCount);
	} else {
		str->ReadDword(frameCount);
	}
	frames.resize(frameCount);

	ieDword cycleCount;
	if (version == BAMVersion::V1) {
		str->ReadScalar<ieDword, ieByte>(cycleCount);
	} else {
		str->ReadDword(cycleCount);
	}
	cycles.resize(cycleCount);

	ieDword dataBlockCount = 0;
	if (version == BAMVersion::V1) {
		str->Read(&CompressedColorIndex, 1);
	} else {
		str->ReadDword(dataBlockCount);
	}

	ieDword PaletteOffset = 0;

	str->ReadDword(FramesOffset);
	if (version == BAMVersion::V1) {
		str->ReadDword(PaletteOffset);
		str->ReadDword(FLTOffset);
		DataStart = str->Size();
	} else {
		str->ReadDword(CyclesOffset);
		str->ReadScalar<strpos_t, ieDword>(DataStart);
	}

	str->Seek(FramesOffset, GEM_STREAM_START);

	for (auto& frame : frames) {
		// ReadRegion is ordered x,y,w,h
		// for some reason these rects are w,h,x,y
		str->ReadSize(frame.bounds.size);
		str->ReadPoint(frame.bounds.origin);

		if (version == BAMVersion::V1) {
			ieDword offset;
			str->ReadScalar(offset);
			frame.RLE = (offset & 0x80000000) == 0;
			frame.location.dataOffset = offset & 0x7FFFFFFF;
			DataStart = std::min(DataStart, frame.location.dataOffset);
		} else {
			str->ReadWord(frame.location.v2.dataBlockIdx);
			str->ReadWord(frame.location.v2.dataBlockCount);
		}
	}

	if (version == BAMVersion::V2) {
		str->Seek(CyclesOffset, GEM_STREAM_START);
	}

	for (auto& cycle : cycles) {
		str->ReadWord(cycle.FramesCount);
		str->ReadWord(cycle.FirstFrame);
	}

	if (version == BAMVersion::V2) {
		return true;
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
	uint8_t* dataBegin = data + frameInfo.location.dataOffset;

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

Holder<Sprite2D> BAMImporter::GetV2Frame(const FrameEntry& frame) {
	size_t frameSize = frame.bounds.size.Area() * 4;
	uint8_t *frameData = static_cast<uint8_t*>(malloc(frameSize));
	std::fill(frameData, frameData + frameSize, 0);

	size_t dataBlockOffset = DataStart + frame.location.v2.dataBlockIdx * sizeof(BAMV2DataBlock);
	str->Seek(dataBlockOffset, GEM_STREAM_START);

	BAMV2DataBlock dataBlock;
	for (uint16_t i = 0; i < frame.location.v2.dataBlockCount; ++i) {
		str->ReadDword(dataBlock.pvrzPage);
		str->ReadScalar<int, ieDword>(dataBlock.source.x);
		str->ReadScalar<int, ieDword>(dataBlock.source.y);
		str->ReadScalar<int, ieDword>(dataBlock.size.w);
		str->ReadScalar<int, ieDword>(dataBlock.size.h);
		str->ReadScalar<int, ieDword>(dataBlock.destination.x);
		str->ReadScalar<int, ieDword>(dataBlock.destination.y);

		Blit(frame, dataBlock, frameData);
	}

	PixelFormat fmt = PixelFormat::ARGB32Bit();
	return {core->GetVideoDriver()->CreateSprite(frame.bounds, frameData, fmt)};
}

void BAMImporter::Blit(const FrameEntry& frame, const BAMV2DataBlock& dataBlock, uint8_t *frameData) {
	// The page is likely to be the same for many sequential accesses
	if (!lastPVRZ || dataBlock.pvrzPage != lastPVRZPage) {
		auto resRef = fmt::format("mos{:04d}", dataBlock.pvrzPage);
		StringView resRefView(resRef.c_str(), 7);

		lastPVRZ = gamedata->GetResourceHolder<ImageMgr>(resRefView, true);
		lastPVRZPage = dataBlock.pvrzPage;
	}

	auto sprite = lastPVRZ->GetSprite2D(Region{dataBlock.source.x, dataBlock.source.y, dataBlock.size.w, dataBlock.size.h});
	if (!sprite) {
		return;
	}

	const uint8_t* spritePixels = static_cast<uint8_t*>(sprite->LockSprite());
	for (int h = 0; h < dataBlock.size.h; ++h) {
		size_t offset = h * sprite->Frame.w * 4;
		size_t destOffset =
			4 * (frame.bounds.w * (dataBlock.destination.y + h) + dataBlock.destination.x);

		std::copy(
			spritePixels + offset,
			spritePixels + offset + sprite->Frame.w * 4,
			frameData + destOffset
		);
	}

	sprite->UnlockSprite();
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
	std::vector<Holder<Sprite2D>> animframes;

	if (version == BAMVersion::V1) {
		str->Seek( DataStart, GEM_STREAM_START );
		strpos_t length = str->Remains();
		if (length == 0) return nullptr;

		auto FLT = CacheFLT();
		uint8_t *data = (uint8_t*)malloc(length);
		str->Read(data, length);

		for (const auto& frameInfo : frames) {
			bool RLECompressed = allowCompression && frameInfo.RLE;
			animframes.push_back(GetFrameInternal(frameInfo, RLECompressed, data - DataStart));
		}
		free(data);

		return new AnimationFactory(resref, std::move(animframes), cycles, std::move(FLT));
	} else {
		std::vector<index_t> FLT(frames.size());

		for (index_t i = 0; i < frames.size(); ++i) {
			FLT[i] = i;
		}

		for (const auto& frame : frames) {
			animframes.push_back(GetV2Frame(frame));
		}

		return new AnimationFactory(resref, std::move(animframes), cycles, std::move(FLT));
	}
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
