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

#include "PLTImporter.h"

#include "RGBAColor.h"

#include "Interface.h"

#include "Logging/Logging.h"
#include "Video/Video.h"

using namespace GemRB;

PLTImporter::~PLTImporter(void)
{
	free(pixels);
}

bool PLTImporter::Import(DataStream* str)
{
	char Signature[8];
	unsigned short unknown[4];

	str->Read(Signature, 8);
	if (strncmp(Signature, "PLT V1  ", 8) != 0) {
		Log(WARNING, "PLTImporter", "Not a valid PLT File.");
		core->UseCorruptedHack = true;
		return false;
	}

	str->Read(unknown, 8);
	str->ReadDword(Width);
	str->ReadDword(Height);

	pixels = malloc(Width * Height * 2);
	str->Read(pixels, Width * Height * 2);
	return true;
}

Holder<Sprite2D> PLTImporter::GetSprite2D(unsigned int type, ieDword paletteIndex[8])
{
	static int pperm[8] = { 3, 6, 0, 5, 4, 1, 2, 7 };
	ColorPal<256> Palettes[8];
	for (int i = 0; i < 8; i++) {
		Palettes[i] = core->GetPalette256(paletteIndex[pperm[i]] >> (8 * type));
	}
	unsigned char* p = (unsigned char*) malloc(Width * Height * 4);
	unsigned char* dest = p;
	const unsigned char* src = nullptr;
	for (int y = Height - 1; y >= 0; y--) {
		src = (unsigned char*) pixels + (y * Width * 2);
		for (unsigned int x = 0; x < Width; x++) {
			unsigned char intensity = *src++;
			unsigned char palindex = *src++;
			*dest++ = Palettes[palindex][intensity].b;
			*dest++ = Palettes[palindex][intensity].g;
			*dest++ = Palettes[palindex][intensity].r;
			if (intensity == 0xff)
				*dest++ = 0x00;
			else
				*dest++ = 0xff;
		}
	}
	constexpr uint32_t red_mask = 0x00ff0000;
	constexpr uint32_t green_mask = 0x0000ff00;
	constexpr uint32_t blue_mask = 0x000000ff;
	PixelFormat fmt(4, red_mask, green_mask, blue_mask, 0);
	fmt.HasColorKey = true;
	fmt.ColorKey = green_mask;

	Holder<Sprite2D> spr = VideoDriver->CreateSprite(Region(0, 0, Width, Height), p, fmt);
	spr->Frame.x = 0;
	spr->Frame.y = 0;
	return spr;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x8D0C64F, "PLT File Importer")
PLUGIN_IE_RESOURCE(PLTImporter, "plt", (ieWord) IE_PLT_CLASS_ID)
END_PLUGIN()
