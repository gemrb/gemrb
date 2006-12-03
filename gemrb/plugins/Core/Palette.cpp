/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Palette.cpp,v 1.4 2006/12/03 20:37:56 wjpalenstijn Exp $
 *
 */

#include "Palette.h"
#include "Interface.h"

#define MINCOL 2
#define MUL    2

void Palette::CreateShadedAlphaChannel()
{
	for (int i = 0; i < 256; ++i) {
		unsigned int r = col[i].r;
		unsigned int g = col[i].g;
		unsigned int b = col[i].b;
		unsigned int m = (r + g + b) / 3;
		if (m > MINCOL) {
			if (( r == 0 ) && ( g == 0xff ) && ( b == 0 )) {
				col[i].a = 0xff;
			} else {
				int tmp = m * MUL;
				col[i].a = ( tmp > 0xff ) ? 0xff : (unsigned char) tmp;
			}
		}
		else {
			col[i].a = 0;
		}
	}
	alpha = true;
}

Palette* Palette::Copy()
{
	Palette* pal = new Palette(col, alpha);
	Release();
	return pal;
}

void Palette::SetupPaperdollColours(ieDword* Colors, unsigned int type)
{
	unsigned int s = 8*type;
	//metal
	core->GetPalette( (Colors[0]>>s)&0xFF, 12, &col[0x04]);
	//minor
	core->GetPalette( (Colors[1]>>s)&0xFF, 12, &col[0x10]);
	//major
	core->GetPalette( (Colors[2]>>s)&0xFF, 12, &col[0x1c]);
	//skin
	core->GetPalette( (Colors[3]>>s)&0xFF, 12, &col[0x28]);
	//leather
	core->GetPalette( (Colors[4]>>s)&0xFF, 12, &col[0x34]);
	//armor
	core->GetPalette( (Colors[5]>>s)&0xFF, 12, &col[0x40]);
	//hair
	core->GetPalette( (Colors[6]>>s)&0xFF, 12, &col[0x4c]);
	
	//minor
	memcpy( &col[0x58], &col[0x11], 8 * sizeof( Color ) );
	//major
	memcpy( &col[0x60], &col[0x1d], 8 * sizeof( Color ) );
	//minor
	memcpy( &col[0x68], &col[0x11], 8 * sizeof( Color ) );
	//metal
	memcpy( &col[0x70], &col[0x05], 8 * sizeof( Color ) );
	//leather
	memcpy( &col[0x78], &col[0x35], 8 * sizeof( Color ) );
	//leather
	memcpy( &col[0x80], &col[0x35], 8 * sizeof( Color ) );
	//minor
	memcpy( &col[0x88], &col[0x11], 8 * sizeof( Color ) );

	int i;
	for (i = 0x90; i < 0xA8; i += 0x08)
		//leather
		memcpy( &col[i], &col[0x35], 8 * sizeof( Color ) );

	//skin
	memcpy( &col[0xB0], &col[0x29], 8 * sizeof( Color ) );

	for (i = 0xB8; i < 0xFF; i += 0x08)
		//leather
		memcpy( &col[i], &col[0x35], 8 * sizeof( Color ) );
}
