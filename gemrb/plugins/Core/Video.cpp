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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Video.cpp,v 1.6 2006/01/28 19:56:34 wjpalenstijn Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Video.h"
#include "Palette.h"

Video::Video(void)
{
	Evnt = NULL;
}

Video::~Video(void)
{
}

/** Set Event Manager */
void Video::SetEventMgr(EventMgr* evnt)
{
	//if 'evnt' is NULL then no Event Manager will be used
	Evnt = evnt;
}

/** Creates a Palette from Color */
Palette* Video::CreatePalette(Color color, Color back)
{
	Palette* pal = new Palette();
	pal->col[0].r = 0;
	pal->col[0].g = 0xff;
	pal->col[0].b = 0;
	pal->col[0].a = 0;
	for (int i = 1; i < 256; i++) {
		pal->col[i].r = back.r +
			( unsigned char ) ( ( ( color.r - back.r ) * ( i ) ) / 255.0 );
		pal->col[i].g = back.g +
			( unsigned char ) ( ( ( color.g - back.g ) * ( i ) ) / 255.0 );
		pal->col[i].b = back.b +
			( unsigned char ) ( ( ( color.b - back.b ) * ( i ) ) / 255.0 );
		pal->col[i].a = 0;
	}
	return pal;
}

void Video::FreePalette(Palette*& pal)
{
	if (pal) {
		pal->Release();
		pal = 0;
	}
}
