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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/CREImporter/CREImp.cpp,v 1.36 2004/07/25 13:39:24 edheldil Exp $
 *
 */

#include "../../includes/win32def.h"
#include "CREImp.h"
#include "../Core/Interface.h"
#include "../../includes/ie_stats.h"

static int RandColor;
static TableMgr *rndcol=NULL;

CREImp::CREImp(void)
{
	str = NULL;
	autoFree = false;
}

CREImp::~CREImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool CREImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "CRE V1.0", 8 ) == 0) {
		CREVersion = IE_CRE_V1_0;
		return true;
	}
	if (strncmp( Signature, "CRE V1.2", 8 ) == 0) {
		CREVersion = IE_CRE_V1_2;
		return true;
	}
	if (strncmp( Signature, "CRE V2.2", 8 ) == 0) {
		CREVersion = IE_CRE_V2_2;
		return true;
	}
	if (strncmp( Signature, "CRE V9.0", 8 ) == 0) {
		CREVersion = IE_CRE_V9_0;
		return true;
	}
	printf( "[CREImporter]: Not a CRE File or File Version not supported: %8.8s\n", Signature );
	return false;
}

CREMemorizedSpell* CREImp::GetMemorizedSpell()
{
	CREMemorizedSpell* spl = new CREMemorizedSpell();

	str->Read( &spl->SpellResRef, 8 );
	str->Read( &spl->Flags, 4 );

	return spl;
}

CREKnownSpell* CREImp::GetKnownSpell()
{
	CREKnownSpell* spl = new CREKnownSpell();

	str->Read( &spl->SpellResRef, 8 );
	str->Read( &spl->Level, 2 );
	str->Read( &spl->Type, 2 );

	return spl;
}

void CREImp::ReadScript(Actor *act, int ScriptLevel)
{
	char aScript[9];
	str->Read( aScript, 8 );
	aScript[8] = 0;
	if (( stricmp( aScript, "NONE" ) == 0 ) || ( aScript[0] == '\0' )) {
		act->Scripts[ScriptLevel] = NULL;
		return;
	}
	act->Scripts[ScriptLevel] = new GameScript( aScript, 0, act->locals );
	if(act->Scripts[ScriptLevel]) {
		act->Scripts[ScriptLevel]->MySelf = act;
	}
}

CRESpellMemorization* CREImp::GetSpellMemorization()
{
	CRESpellMemorization* spl = new CRESpellMemorization();

	str->Read( &spl->Level, 2 );
	str->Read( &spl->Number, 2 );
	str->Read( &spl->Number2, 2 );
	str->Read( &spl->Type, 2 );
	str->Read( &spl->MemorizedIndex, 4 );
	str->Read( &spl->MemorizedCount, 4 );

	return spl;
}

CREItem* CREImp::GetItem()
{
	CREItem *itm = new CREItem();

	str->Read( itm->ItemResRef, 8 );
	str->Read( &itm->Unknown08, 2 );
	str->Read( &itm->Usages[0], 2 );
	str->Read( &itm->Usages[1], 2 );
	str->Read( &itm->Usages[2], 2 );
	str->Read( &itm->Flags, 4 );

	return itm;
}

void CREImp::SetupColor(long &stat)
{
	if(!RandColor) {
		RandColor = core->LoadTable( "RANDCOLR" );
		rndcol = core->GetTable( RandColor );
	}

	if (stat >= 200) {
		if(rndcol) {
			stat = atoi( rndcol->QueryField( ( rand() % 10 ) + 1, stat - 200 ) );
		}
		else {
			stat -= 200;
		}
	}
}

Actor* CREImp::GetActor()
{
	Actor* act = new Actor();
	act->InParty = false;
	unsigned long strref;
	str->Read( &strref, 4 );
	char* poi = core->GetString( strref );
	act->SetText( poi, 0 );
	free( poi );
	str->Read( &strref, 4 );
	poi = core->GetString( strref );
	act->SetText( poi, 1 );
	free( poi );
	act->BaseStats[IE_VISUALRANGE] = 30; //this is just a hack
	str->Read( &act->BaseStats[IE_MC_FLAGS], 2 );
	str->Seek( 2, GEM_CURRENT_POS );
	str->Read( &act->BaseStats[IE_XPVALUE], 4 );
	str->Read( &act->BaseStats[IE_XP], 4 );
	str->Read( &act->BaseStats[IE_GOLD], 4 );
	str->Read( &act->BaseStats[IE_STATE_ID], 4 );
	str->Read( &act->BaseStats[IE_HITPOINTS], 2 );
	str->Read( &act->BaseStats[IE_MAXHITPOINTS], 2 );
	str->Read( &act->BaseStats[IE_ANIMATION_ID], 4 );//animID is a dword 
	str->Read( &act->BaseStats[IE_METAL_COLOR], 1 );
	str->Read( &act->BaseStats[IE_MINOR_COLOR], 1 );
	str->Read( &act->BaseStats[IE_MAJOR_COLOR], 1 );
	str->Read( &act->BaseStats[IE_SKIN_COLOR], 1 );
	str->Read( &act->BaseStats[IE_LEATHER_COLOR], 1 );
	str->Read( &act->BaseStats[IE_ARMOR_COLOR], 1 );
	str->Read( &act->BaseStats[IE_HAIR_COLOR], 1 );
	
        SetupColor(act->BaseStats[IE_METAL_COLOR]);
        SetupColor(act->BaseStats[IE_MINOR_COLOR]);
        SetupColor(act->BaseStats[IE_MAJOR_COLOR]);
        SetupColor(act->BaseStats[IE_SKIN_COLOR]);
        SetupColor(act->BaseStats[IE_LEATHER_COLOR]);
        SetupColor(act->BaseStats[IE_ARMOR_COLOR]);
        SetupColor(act->BaseStats[IE_HAIR_COLOR]);

	unsigned char TotSCEFF;
	str->Read( &TotSCEFF, 1 );
	str->Read( act->SmallPortrait, 8 );
	act->SmallPortrait[8] = 0;
	str->Read( act->LargePortrait, 8 );
	act->LargePortrait[8] = 0;

	unsigned int Inventory_Size;

	switch(CREVersion) {
		case IE_CRE_V1_2:
			Inventory_Size=46;
			GetActorPST(act);
			break;
		case IE_CRE_V1_0: //bg1 too
			Inventory_Size=38;
			GetActorBG(act);
			break;
		case IE_CRE_V2_2:
			Inventory_Size=50;
			GetActorIWD2(act);
			break;
		case IE_CRE_V9_0:
			Inventory_Size=38;
			GetActorIWD1(act);
			break;
		default:
			Inventory_Size=0;
			printf("Unknown creature signature!\n");
	}
	act->SetAnimationID( ( unsigned short ) act->BaseStats[IE_ANIMATION_ID] );
	// Reading inventory, spellbook, etc
	ReadInventory(act, Inventory_Size);
	return act;
}

void CREImp::GetActorPST(Actor *act)
{
	str->Read( &act->BaseStats[IE_REPUTATION], 1 );
	str->Read( &act->BaseStats[IE_HIDEINSHADOWS], 1 );
	str->Read( &act->BaseStats[IE_ARMORCLASS], 2 );
	str->Read( &act->Modified[IE_ARMORCLASS], 2 );
	str->Read( &act->BaseStats[IE_ACCRUSHINGMOD], 2 );
	str->Read( &act->BaseStats[IE_ACMISSILEMOD], 2 );
	str->Read( &act->BaseStats[IE_ACPIERCINGMOD], 2 );
	str->Read( &act->BaseStats[IE_ACSLASHINGMOD], 2 );
	str->Read( &act->BaseStats[IE_THAC0], 1 );
	str->Read( &act->BaseStats[IE_NUMBEROFATTACKS], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSDEATH], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSWANDS], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSPOLY], 1 );	
	str->Read( &act->BaseStats[IE_SAVEVSBREATH], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSSPELL], 1 );
	str->Read( &act->BaseStats[IE_RESISTFIRE], 1 );		
	str->Read( &act->BaseStats[IE_RESISTCOLD], 1 );
	str->Read( &act->BaseStats[IE_RESISTELECTRICITY], 1 );
	str->Read( &act->BaseStats[IE_RESISTACID], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGIC], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGICFIRE], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGICCOLD], 1 );
	str->Read( &act->BaseStats[IE_RESISTSLASHING], 1 );
	str->Read( &act->BaseStats[IE_RESISTCRUSHING], 1 );
	str->Read( &act->BaseStats[IE_RESISTPIERCING], 1 );
	str->Read( &act->BaseStats[IE_RESISTMISSILE], 1 );
	//this is used for unused prof points count
	str->Read( &act->BaseStats[IE_DETECTILLUSIONS], 1 );
	str->Read( &act->BaseStats[IE_SETTRAPS], 1 ); //this is unused in pst
	str->Read( &act->BaseStats[IE_LORE], 1 );
	str->Read( &act->BaseStats[IE_LOCKPICKING], 1 );
	str->Read( &act->BaseStats[IE_STEALTH], 1 );
	str->Read( &act->BaseStats[IE_TRAPS], 1 );
	str->Read( &act->BaseStats[IE_PICKPOCKET], 1 );
	str->Read( &act->BaseStats[IE_FATIGUE], 1 );
	str->Read( &act->BaseStats[IE_INTOXICATION], 1 );
	str->Read( &act->BaseStats[IE_LUCK], 1 );
	for(int i=0;i<21;i++) {
		str->Read( &act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i], 1 );
	}
	str->Read( &act->BaseStats[IE_TRACKING], 1 );
	str->Seek( 32, GEM_CURRENT_POS );
	str->Read( &act->StrRefs[0], 100 * 4 );
	str->Read( &act->BaseStats[IE_LEVEL], 1 );
	str->Read( &act->BaseStats[IE_LEVEL2], 1 );
	str->Read( &act->BaseStats[IE_LEVEL3], 1 );
	int tmp=0;
	str->Read( &tmp, 1 );
	str->Read( &act->BaseStats[IE_STR], 1 );
	str->Read( &act->BaseStats[IE_STREXTRA], 1 );
	str->Read( &act->BaseStats[IE_INT], 1 );
	str->Read( &act->BaseStats[IE_WIS], 1 );
	str->Read( &act->BaseStats[IE_DEX], 1 );
	str->Read( &act->BaseStats[IE_CON], 1 );
	str->Read( &act->BaseStats[IE_CHR], 1 );
	str->Read( &act->Modified[IE_MORALEBREAK], 1 );
	str->Read( &act->BaseStats[IE_MORALEBREAK], 1 );
	str->Read( &act->BaseStats[IE_HATEDRACE], 1 );
	str->Read( &act->BaseStats[IE_MORALERECOVERYTIME], 1 );
	str->Read( &tmp, 1 );
	str->Read( &act->BaseStats[IE_KIT], 4 );
	ReadScript(act, 0);
	ReadScript(act, 2);
	ReadScript(act, 3);
	ReadScript(act, 4);
	ReadScript(act, 5);

	str->Seek( 24, GEM_CURRENT_POS );
	str->Read( &act->ZombieDisguise, 4 );
	str->Seek( 83, GEM_CURRENT_POS );
	str->Read( &act->ColorsCount, 1 );
	str->Read( &act->AppearanceFlags1, 2 );
	str->Read( &act->AppearanceFlags2, 2 );

	for (int i = 0; i < 7; i++)
		str->Read( &act->Colors[i], 2 );

	str->Read( &act->unknown2F2, 2 );
	str->Read( &act->unknown2F4, 1 );

	for (int i = 0; i < 7; i++)
		str->Read( &act->ColorPlacements[i], 1 );

	for (int i = 0; i < 5; i++) {
		str->Read( &act->unknown2FC[i], 4 );
	}
	str->Read( &act->unknown310, 1 );


	str->Read( &act->BaseStats[IE_SPECIES], 1 ); // offset: 0x311
	str->Read( &act->BaseStats[IE_TEAM], 1 );
	str->Read( &act->BaseStats[IE_FACTION], 1 );
	 
	str->Read( &act->BaseStats[IE_EA], 1 );
	str->Read( &act->BaseStats[IE_GENERAL], 1 );
	str->Read( &act->BaseStats[IE_RACE], 1 );
	str->Read( &act->BaseStats[IE_CLASS], 1 );
	str->Read( &act->BaseStats[IE_SPECIFIC], 1 );
	str->Read( &act->BaseStats[IE_SEX], 1 );
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &act->BaseStats[IE_ALIGNMENT], 1 );
	str->Seek( 4, GEM_CURRENT_POS );
	str->Read( act->scriptName, 32 );

	str->Read( &act->KnownSpellsOffset, 4 );
	str->Read( &act->KnownSpellsCount, 4 );
	str->Read( &act->SpellMemorizationOffset, 4 );
	str->Read( &act->SpellMemorizationCount, 4 );
	str->Read( &act->MemorizedSpellsOffset, 4 );
	str->Read( &act->MemorizedSpellsCount, 4 );

	str->Read( &act->ItemSlotsOffset, 4 );
	str->Read( &act->ItemsOffset, 4 );
	str->Read( &act->ItemsCount, 4 );

	str->Seek( 8, GEM_CURRENT_POS );

	str->Read( act->Dialog, 8 );
	act->BaseStats[IE_ARMOR_TYPE] = 0;
}

void CREImp::ReadInventory(Actor *act, unsigned int Inventory_Size)
{
	std::vector<CREItem*> items;
	unsigned int i;

	str->Seek( act->ItemsOffset, GEM_STREAM_START );
	for (i = 0; i < act->ItemsCount; i++) {
		items.push_back( GetItem() );
	}
	act->inventory.SetSlotCount(Inventory_Size);

	str->Seek( act->ItemSlotsOffset, GEM_STREAM_START );
	for (i = 0; i < Inventory_Size; i++) {
		ieWord index;
		str->Read( &index, 2 );

		if (index != 0xFFFF) {
			if(items[index]) {
				act->inventory.SetSlotItem( items[index], i );
				items[index] = NULL;
			}
			else {
				printf("[CREImp]: Duplicate item (%d) in creature!\n", index);
			}
		}
	}

	i=items.size();
	while(i--) {
		if(items[i]) {
			printf("[CREImp]: Dangling item in creature: %s!\n", items[i]->ItemResRef);
			delete items[i];
		}
	}
	//this dword contains the equipping info (which slot is selected)
	unsigned int Equipped;
	str->Read(&Equipped, 4);

//	act->inventory.dump();

	// Reading spellbook

	std::vector<CREKnownSpell*> known_spells;
	std::vector<CREMemorizedSpell*> memorized_spells;

	str->Seek( act->KnownSpellsOffset, GEM_STREAM_START );
	for (i = 0; i < act->KnownSpellsCount; i++) {
		known_spells.push_back( GetKnownSpell() );
	}

	str->Seek( act->MemorizedSpellsOffset, GEM_STREAM_START );
	for (i = 0; i < act->MemorizedSpellsCount; i++) {
		memorized_spells.push_back( GetMemorizedSpell() );
	}

	str->Seek( act->SpellMemorizationOffset, GEM_STREAM_START );
	for (i = 0; i < act->SpellMemorizationCount; i++) {
		CRESpellMemorization* sm = GetSpellMemorization();

		unsigned int j=known_spells.size();
		while(j--) {
			CREKnownSpell* spl = known_spells[j];
			if (!spl) {
				continue;
			}
			if (spl->Type == sm->Type && spl->Level == sm->Level) {
				sm->known_spells.push_back( spl );
				known_spells[j] = NULL;
			}
		}
		for (j = 0; j < sm->MemorizedCount; j++) {
			sm->memorized_spells.push_back( memorized_spells[sm->MemorizedIndex + j] );
			memorized_spells[sm->MemorizedIndex + j] = NULL;
		}
		act->spellbook.AddSpellMemorization( sm );
	}

	i=known_spells.size();
	while(i--) {
		if(known_spells[i]) {
			printf("[CREImp]: Dangling spell in creature: %s!\n", known_spells[i]->SpellResRef);
			delete known_spells[i];
		}
	}
	i=memorized_spells.size();
	while(i--) {
		if(memorized_spells[i]) {
			printf("[CREImp]: Dangling spell in creature: %s!\n", memorized_spells[i]->SpellResRef);
			delete memorized_spells[i];
		}
	}
//	act->spellbook.dump();

	act->Init(); //applies effects, updates Modified
}

void CREImp::GetActorBG(Actor *act)
{
	str->Read( &act->BaseStats[IE_REPUTATION], 1 );
	str->Read( &act->BaseStats[IE_HIDEINSHADOWS], 1 );
	str->Read( &act->BaseStats[IE_ARMORCLASS], 2 );
	str->Read( &act->Modified[IE_ARMORCLASS], 2 );
	str->Read( &act->BaseStats[IE_ACCRUSHINGMOD], 2 );
	str->Read( &act->BaseStats[IE_ACMISSILEMOD], 2 );
	str->Read( &act->BaseStats[IE_ACPIERCINGMOD], 2 );
	str->Read( &act->BaseStats[IE_ACSLASHINGMOD], 2 );
	str->Read( &act->BaseStats[IE_THAC0], 1 );	
	str->Read( &act->BaseStats[IE_NUMBEROFATTACKS], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSDEATH], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSWANDS], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSPOLY], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSBREATH], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSSPELL], 1 );
	str->Read( &act->BaseStats[IE_RESISTFIRE], 1 );		
	str->Read( &act->BaseStats[IE_RESISTCOLD], 1 );
	str->Read( &act->BaseStats[IE_RESISTELECTRICITY], 1 );
	str->Read( &act->BaseStats[IE_RESISTACID], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGIC], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGICFIRE], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGICCOLD], 1 );
	str->Read( &act->BaseStats[IE_RESISTSLASHING], 1 );
	str->Read( &act->BaseStats[IE_RESISTCRUSHING], 1 );
	str->Read( &act->BaseStats[IE_RESISTPIERCING], 1 );
	str->Read( &act->BaseStats[IE_RESISTMISSILE], 1 );
	str->Read( &act->BaseStats[IE_DETECTILLUSIONS], 1 );
	str->Read( &act->BaseStats[IE_SETTRAPS], 1 );
	str->Read( &act->BaseStats[IE_LORE], 1 );
	str->Read( &act->BaseStats[IE_LOCKPICKING], 1 );
	str->Read( &act->BaseStats[IE_STEALTH], 1 );
	str->Read( &act->BaseStats[IE_TRAPS], 1 );
	str->Read( &act->BaseStats[IE_PICKPOCKET], 1 );
	str->Read( &act->BaseStats[IE_FATIGUE], 1 );
	str->Read( &act->BaseStats[IE_INTOXICATION], 1 );
	str->Read( &act->BaseStats[IE_LUCK], 1 );
	for(int i=0;i<21;i++) {
		str->Read( &act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i], 1 );
	}

	str->Read( &act->BaseStats[IE_TRACKING], 1 );
	str->Seek( 32, GEM_CURRENT_POS );
	str->Read( &act->StrRefs[0], 100 * 4 );
	str->Read( &act->BaseStats[IE_LEVEL], 1 );
	str->Read( &act->BaseStats[IE_LEVEL2], 1 );
	str->Read( &act->BaseStats[IE_LEVEL3], 1 );
	int tmp;
	str->Read( &tmp, 1);
	str->Read( &act->BaseStats[IE_STR], 1 );
	str->Read( &act->BaseStats[IE_STREXTRA], 1 );
	str->Read( &act->BaseStats[IE_INT], 1 );
	str->Read( &act->BaseStats[IE_WIS], 1 );
	str->Read( &act->BaseStats[IE_DEX], 1 );
	str->Read( &act->BaseStats[IE_CON], 1 );
	str->Read( &act->BaseStats[IE_CHR], 1 );
	str->Read( &act->Modified[IE_MORALEBREAK], 1 );
	str->Read( &act->BaseStats[IE_MORALEBREAK], 1 );
	str->Read( &act->BaseStats[IE_HATEDRACE], 1 );
	str->Read( &act->BaseStats[IE_MORALERECOVERYTIME], 1 );
	str->Read( &tmp, 1);
	str->Read( &act->BaseStats[IE_KIT], 4 );
	ReadScript(act, 0);
	ReadScript(act, 2);
	ReadScript(act, 3);
	ReadScript(act, 4);
	ReadScript(act, 5);
	 
	str->Read( &act->BaseStats[IE_EA], 1 );
	str->Read( &act->BaseStats[IE_GENERAL], 1 );
	str->Read( &act->BaseStats[IE_RACE], 1 );
	str->Read( &act->BaseStats[IE_CLASS], 1 );
	str->Read( &act->BaseStats[IE_SPECIFIC], 1 );
	str->Read( &act->BaseStats[IE_SEX], 1 );
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &act->BaseStats[IE_ALIGNMENT], 1 );
	str->Seek( 4, GEM_CURRENT_POS );
	str->Read( act->scriptName, 32 );

	str->Read( &act->KnownSpellsOffset, 4 );
	str->Read( &act->KnownSpellsCount, 4 );
	str->Read( &act->SpellMemorizationOffset, 4 );
	str->Read( &act->SpellMemorizationCount, 4 );
	str->Read( &act->MemorizedSpellsOffset, 4 );
	str->Read( &act->MemorizedSpellsCount, 4 );
	 
	str->Read( &act->ItemSlotsOffset, 4 );
	str->Read( &act->ItemsOffset, 4 );
	str->Read( &act->ItemsCount, 4 );

	str->Seek( 8, GEM_CURRENT_POS );

	str->Read( act->Dialog, 8 );
	act->BaseStats[IE_ARMOR_TYPE] = 0;
}

void CREImp::GetActorIWD2(Actor *act)
{
	str->Read( &act->BaseStats[IE_REPUTATION], 1 );
	str->Read( &act->BaseStats[IE_HIDEINSHADOWS], 1 );
	str->Read( &act->BaseStats[IE_ARMORCLASS], 2 );
	str->Read( &act->BaseStats[IE_ACCRUSHINGMOD], 2 );
	str->Read( &act->BaseStats[IE_ACMISSILEMOD], 2 );
	str->Read( &act->BaseStats[IE_ACPIERCINGMOD], 2 );
	str->Read( &act->BaseStats[IE_ACSLASHINGMOD], 2 );
	str->Read( &act->BaseStats[IE_THAC0], 1 );//Unknown in CRE V2.2
	str->Read( &act->BaseStats[IE_NUMBEROFATTACKS], 1 );//Unknown in CRE V2.2
	str->Read( &act->BaseStats[IE_SAVEVSDEATH], 1 );//Fortitude Save in V2.2
	str->Read( &act->BaseStats[IE_SAVEVSWANDS], 1 );//Reflex Save in V2.2
	str->Read( &act->BaseStats[IE_SAVEVSPOLY], 1 );//Will Save in V2.2
	str->Read( &act->BaseStats[IE_RESISTFIRE], 1 );		
	str->Read( &act->BaseStats[IE_RESISTCOLD], 1 );
	str->Read( &act->BaseStats[IE_RESISTELECTRICITY], 1 );
	str->Read( &act->BaseStats[IE_RESISTACID], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGIC], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGICFIRE], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGICCOLD], 1 );
	str->Read( &act->BaseStats[IE_RESISTSLASHING], 1 );
	str->Read( &act->BaseStats[IE_RESISTCRUSHING], 1 );
	str->Read( &act->BaseStats[IE_RESISTPIERCING], 1 );
	str->Read( &act->BaseStats[IE_RESISTMISSILE], 1 );
	str->Read( &act->BaseStats[IE_MAGICDAMAGERESISTANCE], 1 ); 
	str->Seek( 6, GEM_CURRENT_POS );
	str->Read( &act->BaseStats[IE_LUCK], 1 );
	str->Seek( 34, GEM_CURRENT_POS );
	str->Seek( 34, GEM_CURRENT_POS ); //levels
	str->Read( &act->StrRefs[0], 64 * 4 );
	ReadScript( act, 1);
	ReadScript( act, 2);
	str->Seek( 159, GEM_CURRENT_POS );
	str->Read( &act->BaseStats[IE_HATEDRACE], 1);
	str->Seek( 7, GEM_CURRENT_POS ); //actually we got 7 more hated races
	str->Read( &act->BaseStats[IE_SUBRACE], 1 );
	int tmp;
	str->Read( &tmp, 2);
	str->Read( &act->BaseStats[IE_STR], 1 );
	str->Read( &act->BaseStats[IE_INT], 1 );
	str->Read( &act->BaseStats[IE_WIS], 1 );
	str->Read( &act->BaseStats[IE_DEX], 1 );
	str->Read( &act->BaseStats[IE_CON], 1 );
	str->Read( &act->BaseStats[IE_CHR], 1 );
	str->Read( &act->Modified[IE_MORALEBREAK], 1 );
	str->Read( &act->BaseStats[IE_MORALEBREAK], 1 );
	//str->Read( &act->BaseStats[IE_HATEDRACE], 1 );
	str->Read( &tmp, 1);
	str->Read( &act->BaseStats[IE_MORALERECOVERYTIME], 1 );
	str->Read( &act->BaseStats[IE_KIT], 4 );
	ReadScript(act, 0);
	ReadScript(act, 2);
	ReadScript(act, 3);
	ReadScript(act, 4);
	ReadScript(act, 5);

	str->Seek( 232, GEM_CURRENT_POS );
	str->Read( &act->BaseStats[IE_EA], 1 );
	str->Read( &act->BaseStats[IE_GENERAL], 1 );
	str->Read( &act->BaseStats[IE_RACE], 1 );
	str->Read( &act->BaseStats[IE_CLASS], 1 );
	str->Read( &act->BaseStats[IE_SPECIFIC], 1 );
	str->Read( &act->BaseStats[IE_SEX], 1 );
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &act->BaseStats[IE_ALIGNMENT], 1 );
	str->Seek( 4, GEM_CURRENT_POS );
	str->Read( act->scriptName, 32 );

	act->KnownSpellsOffset = 0;
	act->KnownSpellsCount = 0;
	act->SpellMemorizationOffset = 0;
	act->SpellMemorizationCount = 0;
	act->MemorizedSpellsOffset = 0;
	act->MemorizedSpellsCount = 0;
	//skipping spellbook offsets
	str->Seek( 606, GEM_CURRENT_POS);

	str->Read( &act->ItemSlotsOffset, 4 );
	str->Read( &act->ItemsOffset, 4 );
	str->Read( &act->ItemsCount, 4 );

	str->Seek( 8, GEM_CURRENT_POS );

	str->Read( act->Dialog, 8 );
	act->BaseStats[IE_ARMOR_TYPE] = 0;
}

void CREImp::GetActorIWD1(Actor *act) //9.0
{
	str->Read( &act->BaseStats[IE_REPUTATION], 1 );
	str->Read( &act->BaseStats[IE_HIDEINSHADOWS], 1 );
	str->Read( &act->BaseStats[IE_ARMORCLASS], 2 );
	str->Read( &act->Modified[IE_ARMORCLASS], 2 );
	str->Read( &act->BaseStats[IE_ACCRUSHINGMOD], 2 );
	str->Read( &act->BaseStats[IE_ACMISSILEMOD], 2 );
	str->Read( &act->BaseStats[IE_ACPIERCINGMOD], 2 );
	str->Read( &act->BaseStats[IE_ACSLASHINGMOD], 2 );
	str->Read( &act->BaseStats[IE_THAC0], 1 );
	str->Read( &act->BaseStats[IE_NUMBEROFATTACKS], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSDEATH], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSWANDS], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSPOLY], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSBREATH], 1 );
	str->Read( &act->BaseStats[IE_SAVEVSSPELL], 1 );
	str->Read( &act->BaseStats[IE_RESISTFIRE], 1 );		
	str->Read( &act->BaseStats[IE_RESISTCOLD], 1 );
	str->Read( &act->BaseStats[IE_RESISTELECTRICITY], 1 );
	str->Read( &act->BaseStats[IE_RESISTACID], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGIC], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGICFIRE], 1 );
	str->Read( &act->BaseStats[IE_RESISTMAGICCOLD], 1 );
	str->Read( &act->BaseStats[IE_RESISTSLASHING], 1 );
	str->Read( &act->BaseStats[IE_RESISTCRUSHING], 1 );
	str->Read( &act->BaseStats[IE_RESISTPIERCING], 1 );
	str->Read( &act->BaseStats[IE_RESISTMISSILE], 1 );
	str->Read( &act->BaseStats[IE_DETECTILLUSIONS], 1 );
	str->Read( &act->BaseStats[IE_SETTRAPS], 1 );
	str->Read( &act->BaseStats[IE_LORE], 1 );
	str->Read( &act->BaseStats[IE_LOCKPICKING], 1 );
	str->Read( &act->BaseStats[IE_STEALTH], 1 );
	str->Read( &act->BaseStats[IE_TRAPS], 1 );
	str->Read( &act->BaseStats[IE_PICKPOCKET], 1 );
	str->Read( &act->BaseStats[IE_FATIGUE], 1 );
	str->Read( &act->BaseStats[IE_INTOXICATION], 1 );
	str->Read( &act->BaseStats[IE_LUCK], 1 );
	for(int i=0;i<21;i++) {
		str->Read( &act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i], 1 );
	}
	str->Read( &act->BaseStats[IE_TRACKING], 1 );
	str->Seek( 32, GEM_CURRENT_POS );
	str->Read( &act->StrRefs[0], 100 * 4 );
	str->Read( &act->BaseStats[IE_LEVEL], 1 );
	str->Read( &act->BaseStats[IE_LEVEL2], 1 );
	str->Read( &act->BaseStats[IE_LEVEL3], 1 );
	int tmp=0;
	//this is rumoured to be IE_SEX, but we use the gender field for this
	str->Read( &tmp, 1 ); 
	str->Read( &act->BaseStats[IE_STR], 1 );
	str->Read( &act->BaseStats[IE_STREXTRA], 1 );
	str->Read( &act->BaseStats[IE_INT], 1 );
	str->Read( &act->BaseStats[IE_WIS], 1 );
	str->Read( &act->BaseStats[IE_DEX], 1 );
	str->Read( &act->BaseStats[IE_CON], 1 );
	str->Read( &act->BaseStats[IE_CHR], 1 );
	str->Read( &act->Modified[IE_MORALEBREAK], 1 );
	str->Read( &act->BaseStats[IE_MORALEBREAK], 1 );
	str->Read( &act->BaseStats[IE_HATEDRACE], 1 );
	str->Read( &act->BaseStats[IE_MORALERECOVERYTIME], 1 );
	str->Read( &tmp, 1 );
	str->Read( &act->BaseStats[IE_KIT], 4 );
	ReadScript(act, 0);
	ReadScript(act, 2);
	ReadScript(act, 3);
	ReadScript(act, 4);
	ReadScript(act, 5);
	
	str->Seek( 104, GEM_CURRENT_POS );
 
	str->Read( &act->BaseStats[IE_EA], 1 );
	str->Read( &act->BaseStats[IE_GENERAL], 1 );
	str->Read( &act->BaseStats[IE_RACE], 1 );
	str->Read( &act->BaseStats[IE_CLASS], 1 );
	str->Read( &act->BaseStats[IE_SPECIFIC], 1 );
	str->Read( &act->BaseStats[IE_SEX], 1 );
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &act->BaseStats[IE_ALIGNMENT], 1 );
	str->Seek( 4, GEM_CURRENT_POS );
	str->Read( act->scriptName, 32 );

	str->Read( &act->KnownSpellsOffset, 4 );
	str->Read( &act->KnownSpellsCount, 4 );
	str->Read( &act->SpellMemorizationOffset, 4 );
	str->Read( &act->SpellMemorizationCount, 4 );
	str->Read( &act->MemorizedSpellsOffset, 4 );
	str->Read( &act->MemorizedSpellsCount, 4 );

	str->Read( &act->ItemSlotsOffset, 4 );
	str->Read( &act->ItemsOffset, 4 );
	str->Read( &act->ItemsCount, 4 );

	str->Seek( 8, GEM_CURRENT_POS );

	str->Read( act->Dialog, 8 );
	act->BaseStats[IE_ARMOR_TYPE] = 0;
}
