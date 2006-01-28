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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Palette.cpp,v 1.1 2006/01/28 19:56:33 wjpalenstijn Exp $
 *
 */

#include "Palette.h"

#define MINCOL 2
#define MUL    2

void Palette::CreateShadedAlphaChannel()
{
	for (int i = 0; i < 256; ++i) {
		unsigned int r = col[i].r;
		unsigned int g = col[i].g;
		unsigned int b = col[i].b;
		unsigned int m = (r + g + b) / 3;
		if (m > MINCOL)
			if (( r == 0 ) && ( g == 0xff ) && ( b == 0 ))
				col[i].a = 0xff;
			else
				col[i].a = ( m * MUL > 0xff ) ? 0xff : m * MUL;
		else
			col[i].a = 0;
	}
	alpha = true;
}

Palette* Palette::Copy()
{
	Palette* pal = new Palette(col, alpha);
	return pal;
}
