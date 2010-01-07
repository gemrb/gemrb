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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "PLTImporter.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"
#include "../Core/Video.h"

static int initial[8]={20,31,31,7,14,20,0,0};
static int pperm[8]={2,5,6,0,4,3,1,7};

static ieDword red_mask = 0xff000000;
static ieDword green_mask = 0x00ff0000;
static ieDword blue_mask = 0x0000ff00;

PLTImp::PLTImp(void)
{
	pixels = NULL;
	if (DataStream::IsEndianSwitch()) {
		red_mask = 0x000000ff;
		green_mask = 0x0000ff00;
		blue_mask = 0x00ff0000;
	}
}

PLTImp::~PLTImp(void)
{
	if (pixels) {
		free( pixels );
	}
	for (int i = 0; i < 8; i++) {
		if (Palettes[i]) {
			free( Palettes[i] );
		}
	}
}

bool PLTImp::Open(DataStream* stream, bool autoFree)
{
	if (!Resource::Open(stream, autoFree))
		return false;

	char Signature[8];
	unsigned short unknown[4];

	str->Read( Signature, 8 );
	if (strncmp( Signature, "PLT V1  ", 8 ) != 0) {
		printf( "[PLTImporter]: Not a valid PLT File.\n" );
		return false;
	}

	memset(Palettes,0,sizeof(Palettes) );
	memcpy(palIndices,initial,sizeof(palIndices));
	str->Read( unknown, 8 );
	str->ReadDword( &Width );
	str->ReadDword( &Height );

	pixels = malloc( Width * Height * 2 );
	str->Read( pixels, Width * Height * 2 );

	return true;
}

Sprite2D* PLTImp::GetImage()
{
	for (int i = 0; i < 8; i++) {
		if (Palettes[i])
			free( Palettes[i] );
		Palettes[i] = (Color *) malloc(256 * sizeof(Color) );
		core->GetPalette( palIndices[i], 256, Palettes[i] );
	}
	unsigned char * p = ( unsigned char * ) malloc( Width * Height * 4 );
	unsigned char * dest = p;
	unsigned char * src = ( unsigned char * ) pixels;
	for (int y = Height - 1; y >= 0; y--) {
		src = ( unsigned char * ) pixels + ( y * Width * 2 );
		for (unsigned int x = 0; x < Width; x++) {
			unsigned char intensity = *src++;
			unsigned char palindex = *src++;
			if (intensity == 0xff)
				*dest++ = 0x00;
			else
				*dest++ = 0xff;
			*dest++ = Palettes[palindex][intensity].b;
			*dest++ = Palettes[palindex][intensity].g;
			*dest++ = Palettes[palindex][intensity].r;
		}
	}
	Sprite2D* spr = core->GetVideoDriver()->CreateSprite( Width, Height, 32,
		red_mask, green_mask, blue_mask, 0, p,
		true, green_mask );
	spr->XPos = 0;
	spr->YPos = 0;
	return spr;
}

/** Set Palette color Hack */
void PLTImp::GetPalette(int index, int colors, Color* /*pal*/)
{
	palIndices[pperm[index]] = colors;
}

#include "../../includes/plugindef.h"

GEMRB_PLUGIN(0x8D0C64F, "PLT File Importer")
PLUGIN_IE_RESOURCE(&ImageMgr::ID, PLTImp, ".plt", (ieWord)IE_PLT_CLASS_ID)
END_PLUGIN()
