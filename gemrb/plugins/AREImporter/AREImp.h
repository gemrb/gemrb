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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/AREImporter/AREImp.h,v 1.14 2004/04/25 22:41:40 avenger_teambg Exp $
 *
 */

#ifndef AREIMP_H
#define AREIMP_H

#include "../Core/MapMgr.h"

class AREImp : public MapMgr {
private:
	DataStream* str;
	bool autoFree;
	char WEDResRef[8];
	unsigned long LastSave;
	unsigned long AreaFlags;
	unsigned short AreaType, WRain, WSnow, WFog, WLightning, WUnknown;
	unsigned long ActorOffset, AnimOffset, AnimCount;
	unsigned long VerticesOffset;
	unsigned long DoorsCount, DoorsOffset;
	unsigned long EntrancesOffset, EntrancesCount;
	unsigned long SongHeader;
	unsigned short ActorCount, VerticesCount;
	unsigned long ContainersOffset, InfoPointsOffset, ItemsOffset;
	unsigned short ContainersCount, InfoPointsCount, ItemsCount;
	char Script[9];
public:
	AREImp(void);
	~AREImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Map* GetMap(const char* ResRef);
	void release(void)
	{
		delete this;
	}
private:
	CREItem* GetItem();
};

#endif
