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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/CREImporter/CREImp.h,v 1.8 2004/04/14 23:53:36 avenger_teambg Exp $
 *
 */

#ifndef CREIMP_H
#define CREIMP_H

#include "../Core/ActorMgr.h"

#define IE_CRE_V1_0		0
#define IE_CRE_V1_2		1
#define IE_CRE_V2_2		2
#define IE_CRE_V9_0		3

class CREImp : public ActorMgr {
private:
	DataStream* str;
	bool autoFree;
	unsigned char CREVersion;
public:
	CREImp(void);
	~CREImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Actor* GetActor();
public:
	void release(void)
	{
		delete this;
	}
private:
	void GetActorPST(Actor *actor);
	void GetActorBG(Actor *actor);
	void GetActorIWD1(Actor *actor);
	void GetActorIWD2(Actor *actor);
	void ReadInventory(Actor*, unsigned int);
	void ReadScript(Actor *actor, int ScriptLevel);
	CREKnownSpell* GetKnownSpell();
	CRESpellMemorization* GetSpellMemorization();
	CREMemorizedSpell* GetMemorizedSpell();
	CREItem* GetItem();
	void SetupColor(long int&);
};

#endif
