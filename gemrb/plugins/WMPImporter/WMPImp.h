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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WMPImporter/WMPImp.h,v 1.4 2005/02/20 13:00:54 avenger_teambg Exp $
 *
 */

#ifndef WMPIMP_H
#define WMPIMP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../../includes/ie_types.h"
#include "../Core/WorldMap.h"
#include "../Core/WorldMapMgr.h"


class WMPImp : public WorldMapMgr {
private:
	DataStream* str;
	bool autoFree;

	ieDword WorldMapsCount;
	ieDword WorldMapsOffset;

public:
	WMPImp(void);
	~WMPImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	WorldMap* GetWorldMap(unsigned int index);
	unsigned int GetWorldMapsCount();

	void release(void)
	{
		delete this;
	}
private:
	WMPAreaEntry* GetAreaEntry(WMPAreaEntry* ae);
	WMPAreaLink* GetAreaLink(WMPAreaLink* al);
};


#endif
