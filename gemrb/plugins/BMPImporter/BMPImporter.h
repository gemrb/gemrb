// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BMPIMP_H
#define BMPIMP_H

#include "ImageMgr.h"

namespace GemRB {

class BMPImporter : public ImageMgr {
private:
	//BITMAPINFOHEADER
	ieDword Size = 0;
	ieDword Compression = 0;
	ieDword ImageSize = 0;
	/*, ColorsUsed, ColorsImportant*/
	ieWord Planes = 0;
	ieWord BitCount = 0;
	bool hasAlpha = false;

	//COLORTABLE
	ieDword NumColors = 0;
	Color* PaletteColors = nullptr;

	//RASTERDATA
	void* pixels = nullptr;

	//OTHER
	unsigned int PaddedRowLength = 0;

public:
	BMPImporter() noexcept = default;
	BMPImporter(const BMPImporter&) = delete;
	~BMPImporter() override;
	BMPImporter& operator=(const BMPImporter&) = delete;
	bool Import(DataStream* stream) override;
	Holder<Sprite2D> GetSprite2D() override;
	Holder<Sprite2D> GetSprite2D(Region&&) override { return {}; }
	int GetPalette(int colors, Palette& pal) override;

private:
	void Read8To8(const void* rpixels);
	void Read4To8(const void* rpixels);
};

}

#endif
