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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/CREImporter/CREImp.h,v 1.11 2005/06/11 20:17:59 avenger_teambg Exp $
 *
 */

#ifndef CREIMP_H
#define CREIMP_H

#include "../Core/ActorMgr.h"

#define IE_CRE_GEMRB            0
#define IE_CRE_V1_0		10
#define IE_CRE_V1_2		12
#define IE_CRE_V2_2		22
#define IE_CRE_V9_0		90

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

	//returns saved size, updates internal offsets before save
	int GetStoredFileSize(Actor *ac);
	//saves file
	int PutActor(DataStream *stream, Actor *actor);
public:
	void release(void)
	{
		delete this;
	}
private:
	bool SeekCreHeader(char *Signature);
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
	void SetupColor(ieDword&);

	int PutActorGemRB(DataStream *stream, Actor *actor);
	int PutActorPST(DataStream *stream, Actor *actor);
	int PutActorBG(DataStream *stream, Actor *actor);
	int PutActorIWD1(DataStream *stream, Actor *actor);
	int PutActorIWD2(DataStream *stream, Actor *actor);
	int PutVariables(DataStream *stream, Actor *actor);
	int PutInventory(DataStream *stream, Actor *actor, unsigned int size);
	int PutHeader(DataStream *stream, Actor *actor);
};

#endif
