/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/PNGImporter/PNGImp.cpp,v 1.1 2006/08/19 20:16:24 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "PNGImp.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"
#include "../Core/Video.h"

static ieDword red_mask = 0x00ff0000;
static ieDword green_mask = 0x0000ff00;
static ieDword blue_mask = 0x000000ff;

PNGImp::PNGImp(void)
{
	str = NULL;
	autoFree = false;
	Palette = NULL;
	pixels = NULL;
	if (DataStream::IsEndianSwitch()) {
		red_mask = 0x000000ff;
		green_mask = 0x0000ff00;
		blue_mask = 0x00ff0000;
	}
}

PNGImp::~PNGImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
	if (Palette) {
		free( Palette );
	}
	if (pixels) {
		free( pixels );
	}
}

bool PNGImp::Open(DataStream* stream, bool autoFree, bool convert)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	//we release the previous pixel data
	if (pixels) {
		free( pixels );
		pixels = NULL;
	}
	if (Palette) {
		free( Palette );
		Palette = NULL;
	}
	str = stream;
	this->autoFree = autoFree;
  // FIXME: add png code here
	return true;
}

bool PNGImp::OpenFromImage(Sprite2D* sprite, bool autoFree)
{
	if (sprite == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}

	if (pixels) {
		free( pixels );
		pixels = NULL;
	}

	if (Palette) {
		free( Palette );
		Palette = NULL;
	}

	// the previous stream was destructed and there won't be next
	this->autoFree = false;
	// FIXME: add png code here

	// FIXME: free the sprite if autoFree?
	if (autoFree) {
		core->GetVideoDriver()->FreeSprite(sprite);
	}

	return true;
}

Sprite2D* PNGImp::GetImage()
{
	Sprite2D* spr = NULL;
  // FIXME: add png code here
	return spr;
}
/** No descriptions */
void PNGImp::GetPalette(int index, int colors, Color* pal)
{
  // FIXME: add png code here
}


void PNGImp::PutImage(DataStream *output)
{
  // FIXME: add png code here
}
