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

typedef struct PCStruct {
	unsigned short Unknown0;
	unsigned short PartyOrder;
	unsigned long  OffsetToCRE;
	unsigned long  CRESize;
	char           CREResRef[8];
	unsigned long  Orientation;
	char           Area[8];
	unsigned short XPos;
	unsigned short YPos;
	unsigned short ViewXPos;
	unsigned short ViewYPos;
	unsigned char  Unknown28[100];
	unsigned char  Unknown8C[16];
	char           ResRef[8][3];
	unsigned char  UnknownB4[12];
} PCStruct;

class GAMImp : public SaveGameMgr
{
private:
	DataStream * str;
	bool autoFree;
	int version;
	unsigned long PCSize;

public:
	unsigned int GameTime;
	unsigned short WhichFormation;
	unsigned short Formations[5];
	unsigned long PartyGold;
	unsigned long Unknown1c;
	unsigned long PCOffset;
	unsigned long PCCount;
	unsigned long UnknownOffset;
	unsigned long UnknownCount;
	unsigned long NPCOffset;
	unsigned long NPCCount;
	unsigned long GLOBALOffset;
	unsigned long GLOBALCount;
	char AREResRef[9];
	unsigned long Unknown48;
	unsigned long JournalCount;
	unsigned long JournalOffset;
	unsigned long Unknown54;
	unsigned long UnknownOffset54;
	unsigned long UnknownCount58;
	unsigned long KillVarsOffset;
	unsigned long KillVarsCount;
	unsigned long SomeBytesArrayOffset;
	char AnotherArea[9];
	char CurrentArea[9];
	unsigned char Unknowns[84];
public:
	GAMImp(void);
	~GAMImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	Game * GetGame();
public:
	void release(void)
	{
		delete this;
	}
};

#endif

