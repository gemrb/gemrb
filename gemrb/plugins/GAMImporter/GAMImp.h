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
 *
 */

#ifndef GAMIMP_H
#define GAMIMP_H

#include "../Core/SaveGameMgr.h"
#include "../Core/ActorMgr.h"

class GAMImp : public SaveGameMgr {
private:
	DataStream* str;
	bool autoFree;
	int version;
	unsigned int PCSize;

public:
	GAMImp(void);
	~GAMImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Game* GetGame();
public:
	void release(void)
	{
		delete this;
	}
private:
	Actor* GetActor( ActorMgr* aM, bool is_in_party );
	PCStatsStruct* GetPCStats();
	GAMJournalEntry* GetJournalEntry();
};

#endif

