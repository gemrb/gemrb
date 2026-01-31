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

#include "versions.h"

#include "ActorMgr.h"
#include "Spellbook.h"

namespace GemRB {

class CREItem;
struct Effect;

class CREImporter : public ActorMgr {
private:
	CREVersion version = CREVersion::GemRB;
	ieDword KnownSpellsOffset = 0;
	ieDword KnownSpellsCount = 0;
	ieDword SpellMemorizationOffset = 0;
	ieDword SpellMemorizationCount = 0;
	ieDword MemorizedSpellsOffset = 0;
	ieDword MemorizedSpellsCount = 0;
	ieDword MemorizedIndex = 0;
	ieDword MemorizedCount = 0;
	ieDword ItemSlotsOffset = 0;
	ieDword ItemsOffset = 0;
	ieDword ItemsCount = 0;
	ieDword EffectsOffset = 0;
	ieDword EffectsCount = 0;
	ieByte TotSCEFF = 0xff;
	bool IsCharacter = false;
	ieDword CREOffset = 0;
	ieDword VariablesCount = 0;
	ieDword OverlayOffset = 0;
	ieDword OverlayMemorySize = 0;
	//used in CHR header
	int QWPCount = 0; // weapons
	int QSPCount = 0; // spells
	int QITCount = 0; // items

public:
	CREImporter(void);

	bool Import(DataStream* stream) override;
	Actor* GetActor(unsigned char is_in_party) override;

	ieWord FindSpellType(const ResRef& name, unsigned short& level, unsigned int clsMask, unsigned int kit) const override;

	//returns saved size, updates internal offsets before save
	int GetStoredFileSize(const Actor* ac) override;
	//saves file
	int PutActor(DataStream* stream, const Actor* actor, bool chr = false) override;

private:
	/** sets up some variables based on creature version for serializing the object */
	void SetupSlotCounts();
	/** writes out the chr header */
	void WriteChrHeader(DataStream* stream, const Actor* actor);
	/** reads the chr header data (into PCStatStructs) */
	void ReadChrHeader(Actor* actor);
	/** skips the chr header */
	bool SeekCreHeader(char* Signature);
	void GetActorPST(Actor* actor);
	size_t GetActorGemRB(Actor* act);
	void GetActorBG(Actor* actor);
	void GetActorIWD1(Actor* actor);
	void GetActorIWD2(Actor* actor);
	ieDword GetIWD2SpellpageSize(const Actor* actor, ieIWD2SpellType type, int level) const;
	void GetIWD2Spellpage(Actor* act, ieIWD2SpellType type, int level, int count);
	void ReadInventory(Actor*, size_t);
	void ReadSpellbook(Actor* act);
	void ReadEffects(Actor* actor);
	Effect* GetEffect();
	void ReadScript(Actor* actor, int ScriptLevel);
	void ReadDialog(Actor* actor);
	CREKnownSpell* GetKnownSpell();
	CRESpellMemorization* GetSpellMemorization(Actor* act);
	CREMemorizedSpell* GetMemorizedSpell();
	CREItem* GetItem();
	void SetupColor(ieDword&) const;

	int PutActorGemRB(DataStream* stream, const Actor* actor, ieDword InvSize) const;
	int PutActorPST(DataStream* stream, const Actor* actor) const;
	int PutActorBG(DataStream* stream, const Actor* actor) const;
	int PutActorIWD1(DataStream* stream, const Actor* actor) const;
	int PutActorIWD2(DataStream* stream, const Actor* actor) const;
	int PutIWD2Spellpage(DataStream* stream, const Actor* actor, ieIWD2SpellType type, int level) const;
	int PutKnownSpells(DataStream* stream, const Actor* actor) const;
	int PutSpellPages(DataStream* stream, const Actor* actor) const;
	int PutMemorizedSpells(DataStream* stream, const Actor* actor) const;
	int PutEffects(DataStream* stream, const Actor* actor) const;
	int PutVariables(DataStream* stream, const Actor* actor) const;
	int PutInventory(DataStream* stream, const Actor* actor, unsigned int size) const;
	int PutHeader(DataStream* stream, const Actor* actor) const;
};

}

#endif
