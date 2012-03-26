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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Palette.h"

#include "Interface.h"

namespace GemRB {

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

void Palette::Brighten()
{
	for (int i = 0; i<256;i++) {
		col[i].r = (col[i].r+256)/2;
		col[i].g = (col[i].g+256)/2;
		col[i].b = (col[i].b+256)/2;
		col[i].a = (col[i].a+256)/2;
	}
}

Palette* Palette::Copy()
{
	Palette* pal = new Palette(col, alpha);
	Release();
	return pal;
}

void Palette::SetupPaperdollColours(const ieDword* Colors, unsigned int type)
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


static inline void applyMod(const Color& src, Color& dest,
							const RGBModifier& mod) {
	if (mod.speed == -1) {
		if (mod.type == RGBModifier::TINT) {
			dest.r = ((unsigned int)src.r * mod.rgb.r)>>8;
			dest.g = ((unsigned int)src.g * mod.rgb.g)>>8;
			dest.b = ((unsigned int)src.b * mod.rgb.b)>>8;
		} else if (mod.type == RGBModifier::BRIGHTEN) {
			unsigned int r = (unsigned int)src.r * mod.rgb.r;
			if (r & (~0x7FF)) r = 0x7FF;
			dest.r = r >> 3;

			unsigned int g = (unsigned int)src.g * mod.rgb.g;
			if (g & (~0x7FF)) g = 0x7FF;
			dest.g = g >> 3;

			unsigned int b = (unsigned int)src.b * mod.rgb.b;
			if (b & (~0x7FF)) b = 0x7FF;
			dest.b = b >> 3;
		} else if (mod.type == RGBModifier::ADD) {
			unsigned int r = (unsigned int)src.r + mod.rgb.r;
			if (r & (~0xFF)) r = 0xFF;
			dest.r = r;

			unsigned int g = (unsigned int)src.g + mod.rgb.g;
			if (g & (~0xFF)) g = 0xFF;
			dest.g = g;

			unsigned int b = (unsigned int)src.b + mod.rgb.b;
			if (b & (~0xFF)) b = 0xFF;
			dest.b = b;
		} else {
			dest = src;
		}
	} else if (mod.speed > 0) {

		// TODO: a sinewave will probably look better
		int phase = (mod.phase % (2*mod.speed));
		if (phase > mod.speed) {
			phase = 512 - (256*phase)/mod.speed;
		} else {
			phase = (256*phase)/mod.speed;
		}

		if (mod.type == RGBModifier::TINT) {
			dest.r = ((unsigned int)src.r * (256*256 + phase*mod.rgb.r - 256*phase))>>16;
			dest.g = ((unsigned int)src.g * (256*256 + phase*mod.rgb.g - 256*phase))>>16;
			dest.b = ((unsigned int)src.b * (256*256 + phase*mod.rgb.b - 256*phase))>>16;
		} else if (mod.type == RGBModifier::BRIGHTEN) {
			unsigned int r = src.r + (256*256 + phase*mod.rgb.r - 256*phase);
			if (r & (~0x7FFFF)) r = 0x7FFFF;
			dest.r = r >> 11;

			unsigned int g = src.g * (256*256 + phase*mod.rgb.g - 256*phase);
			if (g & (~0x7FFFF)) g = 0x7FFFF;
			dest.g = g >> 11;

			unsigned int b = src.b * (256*256 + phase*mod.rgb.b - 256*phase);
			if (b & (~0x7FFFF)) b = 0x7FFFF;
			dest.b = b >> 11;
		} else if (mod.type == RGBModifier::ADD) {
			unsigned int r = src.r + ((phase*mod.rgb.r)>>8);
			if (r & (~0xFF)) r = 0xFF;
			dest.r = r;

			unsigned int g = src.g + ((phase*mod.rgb.g)>>8);
			if (g & (~0xFF)) g = 0xFF;
			dest.g = g;

			unsigned int b = src.b + ((phase*mod.rgb.b)>>8);
			if (b & (~0xFF)) b = 0xFF;
			dest.b = b;
		} else {
			dest = src;
		}
	} else {
		dest = src;
	}
}

void Palette::SetupRGBModification(const Palette* src, const RGBModifier* mods,
	unsigned int type)
{
	const RGBModifier* tmods = mods+(8*type);
	int i;

	for (i = 0; i < 4; ++i)
		col[i] = src->col[i];

	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x04+i],col[0x04+i],tmods[0]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x10+i],col[0x10+i],tmods[1]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x1c+i],col[0x1c+i],tmods[2]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x28+i],col[0x28+i],tmods[3]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x34+i],col[0x34+i],tmods[4]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x40+i],col[0x40+i],tmods[5]);
	for (i = 0; i < 12; ++i)
		applyMod(src->col[0x4c+i],col[0x4c+i],tmods[6]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x58+i],col[0x58+i],tmods[1]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x60+i],col[0x60+i],tmods[2]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x68+i],col[0x68+i],tmods[1]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x70+i],col[0x70+i],tmods[0]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x78+i],col[0x78+i],tmods[4]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x80+i],col[0x80+i],tmods[4]);
	for (i = 0; i < 8; ++i)
		applyMod(src->col[0x88+i],col[0x88+i],tmods[1]);
	for (i = 0; i < 24; ++i)
		applyMod(src->col[0x90+i],col[0x90+i],tmods[4]);

	for (i = 0; i < 8; ++i)
		col[0xA8+i] = src->col[0xA8+i];

	for (i = 0; i < 8; ++i)
		applyMod(src->col[0xB0+i],col[0xB0+i],tmods[3]);
	for (i = 0; i < 72; ++i)
		applyMod(src->col[0xB8+i],col[0xB8+i],tmods[4]);
}

void Palette::SetupGlobalRGBModification(const Palette* src,
	const RGBModifier& mod)
{
	int i;
	// don't modify the transparency and shadow colour
	for (i = 0; i < 2; ++i)
		col[i] = src->col[i];

	for (i = 2; i < 256; ++i)
		applyMod(src->col[i],col[i],mod);
}

}
