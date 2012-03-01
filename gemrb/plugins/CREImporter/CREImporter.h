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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef CREIMPORTER_H
#define CREIMPORTER_H

#include "ActorMgr.h"
#include "Spellbook.h"

namespace GemRB {

class CREItem;
struct Effect;

#define IE_CRE_GEMRB            0
#define IE_CRE_V1_0		10  //bg1
#define IE_CRE_V1_1		11  //bg2 (still V1.0)
#define IE_CRE_V1_2		12
#define IE_CRE_V2_2		22
#define IE_CRE_V9_0		90

class CREImporter : public ActorMgr {
private:
	DataStream* str;
	unsigned char CREVersion;
	ieDword KnownSpellsOffset;
	ieDword KnownSpellsCount;
	ieDword SpellMemorizationOffset;
	ieDword SpellMemorizationCount;
	ieDword MemorizedSpellsOffset;
	ieDword MemorizedSpellsCount;
	ieDword MemorizedIndex;
	ieDword MemorizedCount;
	ieDword ItemSlotsOffset;
	ieDword ItemsOffset;
	ieDword ItemsCount;
	ieDword EffectsOffset;
	ieDword EffectsCount;
	ieByte TotSCEFF;
	ieByte IsCharacter;
	ieDword CREOffset;
	ieDword VariablesCount;
	ieDword OverlayOffset;
	ieDword OverlayMemorySize;
	//used in CHR header
	int QWPCount; //weapons
	int QSPCount; //spells
	int QITCount; //items
public:
	CREImporter(void);
	~CREImporter(void);
	bool Open(DataStream* stream);
	Actor* GetActor(unsigned char is_in_party);

	//returns saved size, updates internal offsets before save
	int GetStoredFileSize(Actor *ac);
	//saves file
	int PutActor(DataStream *stream, Actor *actor, bool chr=false);
private:
	/** sets up some variables based on creature version for serializing the object */
	void SetupSlotCounts();
	/** writes out the chr header */
	void WriteChrHeader(DataStream *stream, Actor *actor);
	/** reads the chr header data (into PCStatStructs) */
	void ReadChrHeader(Actor *actor);
	/** skips the chr header */
	bool SeekCreHeader(char *Signature);
	void GetActorPST(Actor *actor);
	ieDword GetActorGemRB(Actor *act);
	void GetActorBG(Actor *actor);
	void GetActorIWD1(Actor *actor);
	void GetActorIWD2(Actor *actor);
	void GetIWD2Spellpage(Actor *act, ieIWD2SpellType type, int level, int count);
	void ReadInventory(Actor*, unsigned int);
	void ReadEffects(Actor* actor);
	void GetEffect(Effect *fx);
	void ReadScript(Actor *actor, int ScriptLevel);
	void ReadDialog(Actor *actor);
	CREKnownSpell* GetKnownSpell();
	CRESpellMemorization* GetSpellMemorization(Actor *act);
	CREMemorizedSpell* GetMemorizedSpell();
	CREItem* GetItem();
	void SetupColor(ieDword&);
	int GetHpAdjustment(Actor *actor);

	int PutActorGemRB(DataStream *stream, Actor *actor, ieDword InvSize);
	int PutActorPST(DataStream *stream, Actor *actor);
	int PutActorBG(DataStream *stream, Actor *actor);
	int PutActorIWD1(DataStream *stream, Actor *actor);
	int PutActorIWD2(DataStream *stream, Actor *actor);
	int PutKnownSpells(DataStream *stream, Actor *actor);
	int PutSpellPages(DataStream *stream, Actor *actor);
	int PutMemorizedSpells(DataStream *stream, Actor *actor);
	int PutEffects(DataStream *stream, Actor *actor);
	int PutVariables(DataStream *stream, Actor *actor);
	int PutInventory(DataStream *stream, Actor *actor, unsigned int size);
	int PutHeader(DataStream *stream, Actor *actor);
};

}

#endif
