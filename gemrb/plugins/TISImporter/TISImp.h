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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/TISImporter/TISImp.h,v 1.6 2004/08/05 20:41:09 guidoj Exp $
 *
 */

#ifndef TISIMP_H
#define TISIMP_H

#include "../Core/TileSetMgr.h"

class TISImp : public TileSetMgr {
private:
	DataStream* str;
	bool autoFree;
	ieDword headerShift;
	ieDword TilesCount, TilesSectionLen, TileSize;
public:
	TISImp(void);
	~TISImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Tile* GetTile(unsigned short* indexes, int count,
		unsigned short* secondary = NULL);
	Sprite2D* GetTile(int index);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
