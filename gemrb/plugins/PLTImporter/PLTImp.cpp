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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/PLTImporter/PLTImp.cpp,v 1.3 2003/11/25 13:48:00 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "PLTImp.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"

PLTImp::PLTImp(void)
{
	str = NULL;
	autoFree = false;
	pixels = NULL;
}

PLTImp::~PLTImp(void)
{
	if(str && autoFree)
		delete(str);
	if(pixels)
		free(pixels);
	for(int i = 0; i < 8; i++) {
		if(Palettes[i])
			free(Palettes[i]);
	}
}

bool PLTImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	
	char Signature[8];
	unsigned short unknown[4];
	
	str->Read(Signature, 8);
	if(strncmp(Signature, "PLT V1  ", 8) != 0) {
		printf("[PLTImporter]: Not a valid PLT File.\n");
		return false;
	}

	str->Read(unknown, 8);
	str->Read(&Width, 4);
	str->Read(&Height, 4);

	pixels = malloc(Width*Height*2);
	str->Read(pixels, Width*Height*2);

	for(int i = 0; i < 8; i++) {
		Palettes[i] = NULL;
		palIndexes[i] = i;
	}

	return true;

}

Sprite2D * PLTImp::GetImage()
{
	for(int i = 0; i < 8; i++) {
		if(Palettes[i])
			free(Palettes[i]);
		Palettes[i] = core->GetPalette(palIndexes[i], 256);
	}
	unsigned char * p = (unsigned char*)malloc(Width*Height*4);
	unsigned char * dest = p;
	unsigned char * src = (unsigned char*)pixels;
	for(int y = Height-1; y >= 0; y--) {
		src = (unsigned char*)pixels+(y*Width*2);
		for(int x = 0; x < Width; x++) {
			unsigned char intensity = *src++;
			unsigned char palindex = *src++;
			if(intensity == 0xff)
				*dest++ = 0x00;
			else
				*dest++ = 0xff;
			*dest++ = Palettes[palindex][intensity].b;
			*dest++ = Palettes[palindex][intensity].g;
			*dest++ = Palettes[palindex][intensity].r;
		}
	}
	Sprite2D * spr = core->GetVideoDriver()->CreateSprite(Width, Height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff, p, true, 0x00ff0000);
	spr->XPos = 0;
	spr->YPos = 0;
	return spr;
}
/** Set Palette color Hack */
void PLTImp::GetPalette(int index, int colors, Color * pal){
	palIndexes[index] = colors;
}
