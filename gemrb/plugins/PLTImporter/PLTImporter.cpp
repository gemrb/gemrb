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
#include "win32def.h"

#include "Interface.h"
#include "Video.h"

using namespace GemRB;

static int pperm[8]={3,6,0,5,4,1,2,7};

static ieDword red_mask = 0x00ff0000;
static ieDword green_mask = 0x0000ff00;
static ieDword blue_mask = 0x000000ff;

PLTImporter::PLTImporter(void)
{
	pixels = NULL;
	if (DataStream::IsEndianSwitch()) {
		red_mask = 0x0000ff00;
		green_mask = 0x00ff0000;
		blue_mask = 0xff000000;
	}
}

PLTImporter::~PLTImporter(void)
{
	if (pixels) {
		free( pixels );
	}
}

bool PLTImporter::Open(DataStream* str)
{
	if (!str) {
		return false;
	}

	char Signature[8];
	unsigned short unknown[4];

	str->Read( Signature, 8 );
	if (strncmp( Signature, "PLT V1  ", 8 ) != 0) {
		print("[PLTImporter]: Not a valid PLT File.");
		return false;
	}

	str->Read( unknown, 8 );
	str->ReadDword( &Width );
	str->ReadDword( &Height );

	pixels = malloc( Width * Height * 2 );
	str->Read( pixels, Width * Height * 2 );
	delete str;
	return true;
}

Sprite2D* PLTImporter::GetSprite2D(unsigned int type, ieDword paletteIndex[8])
{
	Color Palettes[8][256];
	for (int i = 0; i < 8; i++) {
		core->GetPalette( (paletteIndex[pperm[i]] >> (8*type)) & 0xFF, 256, Palettes[i] );
	}
	unsigned char * p = ( unsigned char * ) malloc( Width * Height * 4 );
	unsigned char * dest = p;
	unsigned char * src = ( unsigned char * ) pixels;
	for (int y = Height - 1; y >= 0; y--) {
		src = ( unsigned char * ) pixels + ( y * Width * 2 );
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
	Sprite2D* spr = core->GetVideoDriver()->CreateSprite( Width, Height, 32,
		red_mask, green_mask, blue_mask, 0, p,
		true, green_mask );
	spr->XPos = 0;
	spr->YPos = 0;
	return spr;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x8D0C64F, "PLT File Importer")
PLUGIN_IE_RESOURCE(PLTImporter, "plt", (ieWord)IE_PLT_CLASS_ID)
END_PLUGIN()
