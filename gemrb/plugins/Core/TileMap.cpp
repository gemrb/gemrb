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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileMap.cpp,v 1.5 2003/11/26 13:56:38 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TileMap.h"

TileMap::TileMap(void)
{
	XCellCount = 0;
	YCellCount = 0;
}

TileMap::~TileMap(void)
{
}

void TileMap::AddOverlay(TileOverlay * overlay)
{
	if(overlay->w > XCellCount)
		XCellCount = overlay->w;
	if(overlay->h > YCellCount)
		YCellCount = overlay->h;
	overlays.push_back(overlay);
}

void TileMap::DrawOverlay(unsigned int index, Region viewport)
{
	if(index < overlays.size())
		overlays[index]->Draw(viewport);
}
