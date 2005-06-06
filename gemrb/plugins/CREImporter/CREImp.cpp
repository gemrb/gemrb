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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA	02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/CREImporter/CREImp.cpp,v 1.72 2005/06/06 22:21:21 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "CREImp.h"
#include "../Core/Interface.h"
#include "../../includes/ie_stats.h"

#define MAXCOLOR 12
typedef unsigned char ColorSet[MAXCOLOR];
static int RandColor=-1;
static int RandRows;
static ColorSet* randcolors=NULL;

void ReleaseMemoryCRE()
{
	if (randcolors) {
		delete [] randcolors;
		randcolors = NULL;
	}
}

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
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	if (stream == NULL) {
		return false;
	}
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "CHR ",4) == 0) {
		//skips chr signature, reads cre signature
		if (!SeekCreHeader(Signature)) {
			return false;
		}
	}
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

bool CREImp::SeekCreHeader(char *Signature)
{
	if (strncmp( Signature, "CHR V1.0", 8) == 0) {
		str->Seek(0x5c, GEM_CURRENT_POS);
		goto done;
	}
	if (strncmp( Signature, "CHR V2.2", 8) == 0) {
		str->Seek(0x21c, GEM_CURRENT_POS);
		goto done;
	}
	return false;
done:
	str->Read( Signature, 8);
	return true;
}

CREMemorizedSpell* CREImp::GetMemorizedSpell()
{
	CREMemorizedSpell* spl = new CREMemorizedSpell();

	str->ReadResRef( spl->SpellResRef );
	str->ReadDword( &spl->Flags );

	return spl;
}

CREKnownSpell* CREImp::GetKnownSpell()
{
	CREKnownSpell* spl = new CREKnownSpell();

	str->ReadResRef( spl->SpellResRef );
	str->ReadWord( &spl->Level );
	str->ReadWord( &spl->Type );

	return spl;
}

void CREImp::ReadScript(Actor *act, int ScriptLevel)
{
	ieResRef aScript;
	str->ReadResRef( aScript );
	if (( stricmp( aScript, "NONE" ) == 0 ) || ( aScript[0] == '\0' )) {
		act->Scripts[ScriptLevel] = NULL;
		return;
	}
	act->Scripts[ScriptLevel] = new GameScript( aScript, ST_ACTOR, act->locals );
	if (act->Scripts[ScriptLevel]) {
		act->Scripts[ScriptLevel]->MySelf = act;
	}
}

CRESpellMemorization* CREImp::GetSpellMemorization()
{
	CRESpellMemorization* spl = new CRESpellMemorization();

	str->ReadWord( &spl->Level );
	str->ReadWord( &spl->Number );
	str->ReadWord( &spl->Number2 );
	str->ReadWord( &spl->Type );
	str->ReadDword( &spl->MemorizedIndex );
	str->ReadDword( &spl->MemorizedCount );

	return spl;
}

void CREImp::SetupColor(ieDword &stat)
{
	if (RandColor==-1) {
		RandColor = 0;
		int table = core->LoadTable( "RANDCOLR" );
		TableMgr *rndcol = core->GetTable( table );
		if (rndcol) {
			RandColor=rndcol->GetColumnCount();
			RandRows=rndcol->GetRowCount();
			if (RandRows>MAXCOLOR) RandRows=MAXCOLOR;
		}
		if (RandRows>1 && RandColor>0) {
			randcolors = new ColorSet[RandColor];
			int cols = RandColor;
			while(cols--)
			{
				for (int i=0;i<RandRows;i++) {
					randcolors[cols][i]=atoi( rndcol->QueryField( i, cols ) );
				}
				randcolors[cols][0]-=200;
			}
		}
		else {
			RandColor=0;
		}
		core->DelTable( table );
	}

	if (stat<200) return;
	if (RandColor>0) {
		stat-=200;
		//assuming an ordered list, so looking in the middle first
		int i;
		for (i=(int) stat;i>=0;i--) {
			if (randcolors[i][0]==stat) {
				stat=randcolors[i][ rand()%RandRows + 1];
				return;
			}
		}
		for(i=(int) stat+1;i<RandColor;i++) {
			if (randcolors[i][0]==stat) {
				stat=randcolors[i][ rand()%RandRows + 1];
				return;
			}
		}
	}
}

Actor* CREImp::GetActor()
{
	if (!str)
		return NULL;
	Actor* act = new Actor();
	if (!act)
		return NULL;
	// saving in original version requires the original version
	// otherwise it is set to 0 at construction time
	if (core->SaveAsOriginal) {
		act->version = CREVersion;
	}
	act->InParty = 0;
	ieDword strref;
	str->ReadDword( &strref );
	char* poi = core->GetString( strref );
	act->SetText( poi, 1 ); //setting longname
	free( poi );
	str->ReadDword( &strref );
	poi = core->GetString( strref );
	act->SetText( poi, 2 ); //setting shortname (for tooltips)
	free( poi );
	act->BaseStats[IE_VISUALRANGE] = 30; //this is just a hack
	act->BaseStats[IE_DIALOGRANGE] = 15; //this is just a hack
	str->ReadDword( &act->BaseStats[IE_MC_FLAGS] );
	str->ReadDword( &act->BaseStats[IE_XPVALUE] );
	str->ReadDword( &act->BaseStats[IE_XP] );
	str->ReadDword( &act->BaseStats[IE_GOLD] );
	str->ReadDword( &act->BaseStats[IE_STATE_ID] );
	ieWord tmp;
	str->ReadWord( &tmp );
	act->BaseStats[IE_HITPOINTS]=tmp;
	str->ReadWord( &tmp );
	act->BaseStats[IE_MAXHITPOINTS]=tmp;
	str->ReadDword( &act->BaseStats[IE_ANIMATION_ID] );//animID is a dword 
	ieByte tmp2[7];
	str->Read( tmp2, 7);
	for(int i=0;i<7;i++) {
		act->BaseStats[IE_METAL_COLOR+i]=tmp2[i];
		SetupColor(act->BaseStats[IE_METAL_COLOR+i]);
	}

	ieByte TotSCEFF;
	str->Read( &TotSCEFF, 1 );
	str->ReadResRef( act->SmallPortrait );
	str->ReadResRef( act->LargePortrait );

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

	//Hacking NONE to no error
	if (stricmp(act->Dialog,"NONE") == 0) {
		act->Dialog[0]=0;
	}

	// Reading inventory, spellbook, etc
	ReadInventory(act, Inventory_Size);
	// Setting up derived stats
	act->SetAnimationID( ( ieWord ) act->BaseStats[IE_ANIMATION_ID] );
	if (act->BaseStats[IE_STATE_ID] & STATE_DEAD) {
		act->SetStance( IE_ANI_TWITCH );
		act->Active=0;
	} else {
		act->SetStance( IE_ANI_AWAKE );
	}
	return act;
}

void CREImp::GetActorPST(Actor *act)
{
	int i;
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ARMORCLASS]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	//skipping a word
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_THAC0]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_NUMBEROFATTACKS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSBREATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSSPELL]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	//this is used for unused prof points count
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FREESLOTS]=tmpByte; //using another field than usually
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SETTRAPS]=tmpByte; //this is unused in pst
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=tmpByte;
	for(i=0;i<21;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for(i=0;i<100;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL2]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL3]=tmpByte;
	str->Read( &tmpByte, 1 );
	//skipping a byte
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STREXTRA]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INT]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_WIS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DEX]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CON]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CHR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALEBREAK]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALERECOVERYTIME]=tmpByte;
	str->Read( &tmpByte, 1 );
	//skipping a byte
	str->ReadDword( &act->BaseStats[IE_KIT] );
	ReadScript(act, 0);
	ReadScript(act, 2);
	ReadScript(act, 3);
	ReadScript(act, 4);
	ReadScript(act, 5);

	str->Seek( 44, GEM_CURRENT_POS );
	str->ReadDword( &act->BaseStats[IE_XP_MAGE] ); // Exp for secondary class
	str->ReadDword( &act->BaseStats[IE_XP_THIEF] ); // Exp for tertiary class
	for(i = 0; i<10; i++) {
		str->ReadWord( &tmpWord );
		act->BaseStats[IE_INTERNAL_0+i]=tmpWord;
	}
	str->Seek( 4, GEM_CURRENT_POS );
	char KillVar[33]; //use this as needed
	str->Read(KillVar,32);
	KillVar[32]=0;
	str->Seek( 3, GEM_CURRENT_POS ); // dialog radius, feet circle size???

	ieByte ColorsCount;

	str->Read( &ColorsCount, 1 );

	str->ReadWord( &act->AppearanceFlags1 );
	str->ReadWord( &act->AppearanceFlags2 );

	for (i = 0; i < 7; i++) {
		str->ReadWord( &tmpWord );
		act->BaseStats[IE_COLORS+i] = tmpWord;
	}
	act->BaseStats[IE_COLORCOUNT] = ColorsCount; //hack

	str->Seek(31, GEM_CURRENT_POS);
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SPECIES]=tmpByte; // offset: 0x311
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TEAM]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FACTION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_EA]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_GENERAL]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RACE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CLASS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SPECIFIC]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SEX]=tmpByte;
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	char scriptname[33];
	str->Read( scriptname, 32);
	scriptname[32]=0;
	act->SetScriptName(scriptname);
	strnuprcpy(act->KillVar, KillVar, 32);

	str->ReadDword( &act->KnownSpellsOffset );
	str->ReadDword( &act->KnownSpellsCount );
	str->ReadDword( &act->SpellMemorizationOffset );
	str->ReadDword( &act->SpellMemorizationCount );
	str->ReadDword( &act->MemorizedSpellsOffset );
	str->ReadDword( &act->MemorizedSpellsCount );

	str->ReadDword( &act->ItemSlotsOffset );
	str->ReadDword( &act->ItemsOffset );
	str->ReadDword( &act->ItemsCount );

	str->Seek( 8, GEM_CURRENT_POS );

	str->ReadResRef( act->Dialog );
}

void CREImp::ReadInventory(Actor *act, unsigned int Inventory_Size)
{
	CREItem** items;
	unsigned int i,j,k;

	str->Seek( act->ItemsOffset, GEM_STREAM_START );
	items = (CREItem **) calloc (act->ItemsCount, sizeof(CREItem *) );

	for (i = 0; i < act->ItemsCount; i++) {
		items[i] = core->ReadItem(str); //could be NULL item
	}
	act->inventory.SetSlotCount(Inventory_Size);

	str->Seek( act->ItemSlotsOffset, GEM_STREAM_START );
	for (i = 0; i < Inventory_Size; i++) {
		ieWord index;
		str->Read( &index, 2 );

		if (index != 0xFFFF) {
			if (index>=act->ItemsCount) {
				printf("[CREImp]: Invalid item index (%d) in creature!\n", index);
				continue;
			}
			if (items[index]) {
				act->inventory.SetSlotItem( items[index], i );
				items[index] = NULL;
				continue;
			}
			printf("[CREImp]: Duplicate or (no-drop) item (%d) in creature!\n", index);
		}
	}

	i = act->ItemsCount;
	while(i--) {
		if ( items[i]) {
			printf("[CREImp]: Dangling item in creature: %s!\n", items[i]->ItemResRef);
			delete items[i];
		}
	}
	free (items);

	//this dword contains the equipping info (which slot is selected)
	// 0,1,2,3 - weapon slots
	// 1000 - fist
	// weird values - quiver
	ieDword Equipped;
	str->ReadDword( &Equipped );
	act->inventory.SetEquippedSlot( Equipped );
	// Reading spellbook

	CREKnownSpell **known_spells=(CREKnownSpell **) calloc(act->KnownSpellsCount, sizeof(CREKnownSpell *) );
	CREMemorizedSpell **memorized_spells=(CREMemorizedSpell **) calloc(act->MemorizedSpellsCount, sizeof(CREKnownSpell *) );

	str->Seek( act->KnownSpellsOffset, GEM_STREAM_START );
	for (i = 0; i < act->KnownSpellsCount; i++) {
		known_spells[i]=GetKnownSpell();
	}

	str->Seek( act->MemorizedSpellsOffset, GEM_STREAM_START );
	for (i = 0; i < act->MemorizedSpellsCount; i++) {
		memorized_spells[i]=GetMemorizedSpell();
	}

	str->Seek( act->SpellMemorizationOffset, GEM_STREAM_START );
	for (i = 0; i < act->SpellMemorizationCount; i++) {
		CRESpellMemorization* sm = GetSpellMemorization();

		j=act->KnownSpellsCount;
		while(j--) {
			CREKnownSpell* spl = known_spells[j];
			if (!spl) {
				continue;
			}
			if ((spl->Type == sm->Type) && (spl->Level == sm->Level)) {
				sm->known_spells.push_back( spl );
				known_spells[j] = NULL;
				continue;
			}
		}
		for (j = 0; j < sm->MemorizedCount; j++) {
			k = sm->MemorizedIndex+j;
			if (memorized_spells[k]) {
				sm->memorized_spells.push_back( memorized_spells[k]);
				memorized_spells[k] = NULL;
				continue;
			}
			printf("[CREImp]: Duplicate memorized spell (%d) in creature!\n", k);
		}
		act->spellbook.AddSpellMemorization( sm );
	}

	i=act->KnownSpellsCount;
	while(i--) {
		if (known_spells[i]) {
			printf("[CREImp]: Dangling spell in creature: %s!\n", known_spells[i]->SpellResRef);
			delete known_spells[i];
		}
	}
	free(known_spells);

	i=act->MemorizedSpellsCount;
	while(i--) {
		if (memorized_spells[i]) {
			printf("[CREImp]: Dangling spell in creature: %s!\n", memorized_spells[i]->SpellResRef);
			delete memorized_spells[i];
		}
	}
	free(memorized_spells);

	act->Init(); //applies effects, updates Modified
}

void CREImp::GetActorBG(Actor *act)
{
	int i;
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ARMORCLASS]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	//skipping a word
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_THAC0]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_NUMBEROFATTACKS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSBREATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSSPELL]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DETECTILLUSIONS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SETTRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=tmpByte;
	for(i=0;i<21;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for(i=0;i<100;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL2]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL3]=tmpByte;
	str->Read( &tmpByte, 1);
	//skipping a byte
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_STR]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_STREXTRA]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_INT]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_WIS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_DEX]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CON]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CHR]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_MORALE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_MORALEBREAK]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_MORALERECOVERYTIME]=tmpByte;
	str->Read( &tmpByte, 1);
	//skipping a byte
	str->ReadDword( &act->BaseStats[IE_KIT] );
	ReadScript(act, 0);
	ReadScript(act, 2);
	ReadScript(act, 3);
	ReadScript(act, 4);
	ReadScript(act, 5);
	 
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_EA]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_GENERAL]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_RACE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CLASS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SPECIFIC]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SEX]=tmpByte;
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	char scriptname[33];
	str->Read( scriptname, 32);
	scriptname[32]=0;
	act->SetScriptName(scriptname);
	act->KillVar[0]=0;

	str->ReadDword( &act->KnownSpellsOffset );
	str->ReadDword( &act->KnownSpellsCount );
	str->ReadDword( &act->SpellMemorizationOffset );
	str->ReadDword( &act->SpellMemorizationCount );
	str->ReadDword( &act->MemorizedSpellsOffset );
	str->ReadDword( &act->MemorizedSpellsCount );
	 
	str->ReadDword( &act->ItemSlotsOffset );
	str->ReadDword( &act->ItemsOffset );
	str->ReadDword( &act->ItemsCount );

	str->Seek( 8, GEM_CURRENT_POS );

	str->ReadResRef( act->Dialog );
}

void CREImp::GetActorIWD2(Actor *act)
{
	int i;
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ARMORCLASS]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_THAC0]=(ieByteSigned) tmpByte;//Unknown in CRE V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_NUMBEROFATTACKS]=tmpByte;//Unknown in CRE V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;//Fortitude Save in V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;//Reflex Save in V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;// will Save in V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MAGICDAMAGERESISTANCE]=(ieByteSigned) tmpByte;
	str->Seek( 6, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=tmpByte;
	str->Seek( 34, GEM_CURRENT_POS ); //unknowns
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL]=tmpByte; //total levels
	str->Seek( 33, GEM_CURRENT_POS ); //levels for classes
	for(i=0;i<64;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	ReadScript( act, 1);
	ReadScript( act, 6);
	//4 unknown
	//12 feats
	//143 skills+unknowns
	str->Seek( 159, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Seek( 7, GEM_CURRENT_POS ); //actually we got 7 more hated races
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SUBRACE]=tmpByte;
	str->ReadWord( &tmpWord );
	//skipping 2 bytes
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INT]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_WIS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DEX]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CON]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CHR]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_MORALE]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_MORALEBREAK]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	//HatedRace is a list of races, so this is skipped here
	//act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_MORALERECOVERYTIME]=tmpByte;
	str->ReadDword( &act->BaseStats[IE_KIT] );
	ReadScript(act, 0);
	ReadScript(act, 2);
	ReadScript(act, 3);
	ReadScript(act, 4);
	ReadScript(act, 5);

	str->Seek( 232, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_EA]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_GENERAL]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_RACE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CLASS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SPECIFIC]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SEX]=tmpByte;
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	char scriptname[33];
	str->Read( scriptname, 32);
	scriptname[32]=0;
	act->SetScriptName(scriptname);
	//not sure
	act->KillVar[0]=0;

	act->KnownSpellsOffset = 0;
	act->KnownSpellsCount = 0;
	act->SpellMemorizationOffset = 0;
	act->SpellMemorizationCount = 0;
	act->MemorizedSpellsOffset = 0;
	act->MemorizedSpellsCount = 0;
	//skipping spellbook offsets
	str->Seek( 606, GEM_CURRENT_POS);

	str->ReadDword( &act->ItemSlotsOffset );
	str->ReadDword( &act->ItemsOffset );
	str->ReadDword( &act->ItemsCount );

	str->Seek( 8, GEM_CURRENT_POS );

	str->ReadResRef( act->Dialog );
}

void CREImp::GetActorIWD1(Actor *act) //9.0
{
	int i;
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord( &tmpWord ); 
	act->BaseStats[IE_ARMORCLASS]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord ); 
	//skipping a word
	str->ReadWord( &tmpWord ); 
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord ); 
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord ); 
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord ); 
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_THAC0]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_NUMBEROFATTACKS]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_SAVEVSBREATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_SAVEVSSPELL]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_DETECTILLUSIONS]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_SETTRAPS]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_LUCK]=tmpByte;
	for(i=0;i<21;i++) {
		str->Read( &tmpByte, 1 ); 
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for(i=0;i<100;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_LEVEL]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_LEVEL2]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_LEVEL3]=tmpByte;
	//this is rumoured to be IE_SEX, but we use the gender field for this
	str->Read( &tmpByte, 1 ); 
	//skipping a byte
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_STR]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_STREXTRA]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_INT]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_WIS]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_DEX]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_CON]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_CHR]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_MORALE]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_MORALEBREAK]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_MORALERECOVERYTIME]=tmpByte;
	str->Read( &tmpByte, 1 );
	//skipping a byte
	str->ReadDword( &act->BaseStats[IE_KIT] );
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
	char scriptname[33];
	str->Read( scriptname, 32);
	scriptname[32]=0;
	act->SetScriptName(scriptname);
	//not sure
	act->KillVar[0]=0;

	str->ReadDword( &act->KnownSpellsOffset );
	str->ReadDword( &act->KnownSpellsCount );
	str->ReadDword( &act->SpellMemorizationOffset );
	str->ReadDword( &act->SpellMemorizationCount );
	str->ReadDword( &act->MemorizedSpellsOffset );
	str->ReadDword( &act->MemorizedSpellsCount );

	str->ReadDword( &act->ItemSlotsOffset );
	str->ReadDword( &act->ItemsOffset );
	str->ReadDword( &act->ItemsCount );

	str->Seek( 8, GEM_CURRENT_POS );

	str->ReadResRef( act->Dialog );
}
