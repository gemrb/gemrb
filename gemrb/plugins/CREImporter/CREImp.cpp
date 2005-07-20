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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/CREImporter/CREImp.cpp,v 1.86 2005/07/20 21:46:28 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "CREImp.h"
#include "../Core/Interface.h"
#include "../Core/EffectMgr.h"
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

bool CREImp::Open(DataStream* stream, bool aF)
{
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	autoFree = aF;
	if (stream == NULL) {
		return false;
	}
	char Signature[8];
	str->Read( Signature, 8 );
	IsCharacter = false;
	if (strncmp( Signature, "CHR ",4) == 0) {
		IsCharacter = true;
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
	if (strncmp( Signature, "CRE V0.0", 8 ) == 0) {
		CREVersion = IE_CRE_GEMRB;
		return true;
	}

	printf( "[CREImporter]: Not a CRE File or File Version not supported: %8.8s\n", Signature );
	return false;
}

void CREImp::ReadChrHeader(Actor *act)
{
	ieDword tmpDword;
	char name[33];
	char Signature[8];

	str->Rewind();
	str->Read (Signature, 8);
	str->Read (name, 32);
	name[32]=0;
	tmpDword = *(ieDword *) name;
	if (tmpDword != 0 && tmpDword !=1) {
		act->SetText( name, 0 ); //setting longname
	}
}

bool CREImp::SeekCreHeader(char *Signature)
{
	if (strncmp( Signature, "CHR V1.0", 8) == 0) {
		str->Seek(0x3c, GEM_CURRENT_POS);
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
	str->ReadDword( &MemorizedIndex );
	str->ReadDword( &MemorizedCount );

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
		for (i=(int) stat+1;i<RandColor;i++) {
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
	str->ReadDword( &act->LongStrRef );
	char* poi = core->GetString( act->LongStrRef );
	act->SetText( poi, 1 ); //setting longname
	free( poi );
	str->ReadDword( &act->ShortStrRef );
	poi = core->GetString( act->ShortStrRef );
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
	for (int i=0;i<7;i++) {
		act->BaseStats[IE_METAL_COLOR+i]=tmp2[i];
		SetupColor(act->BaseStats[IE_METAL_COLOR+i]);
	}

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

	// Read saved effects
	if (core->IsAvailable(IE_EFF_CLASS_ID) ) {
		ReadEffects( act );
	} else {
		printf("Effect importer is unavailable!\n");
	}
	// Reading inventory, spellbook, etc
	ReadInventory( act, Inventory_Size );
	act->inventory.AddAllEffects();

	//applying effects
	act->Init();

	// Setting up derived stats
	act->SetAnimationID( ( ieWord ) act->BaseStats[IE_ANIMATION_ID] );
	if (act->BaseStats[IE_STATE_ID] & STATE_DEAD) {
		act->SetStance( IE_ANI_TWITCH );
		act->Active=0;
	} else {
		act->SetStance( IE_ANI_AWAKE );
	}
	if (IsCharacter) {
		ReadChrHeader(act);
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
	for (i=0;i<21;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for (i=0;i<100;i++) {
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
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);

	str->Seek( 44, GEM_CURRENT_POS );
	str->ReadDword( &act->BaseStats[IE_XP_MAGE] ); // Exp for secondary class
	str->ReadDword( &act->BaseStats[IE_XP_THIEF] ); // Exp for tertiary class
	for (i = 0; i<10; i++) {
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
	strnspccpy(act->KillVar, KillVar, 32);

	str->ReadDword( &KnownSpellsOffset );
	str->ReadDword( &KnownSpellsCount );
	str->ReadDword( &SpellMemorizationOffset );
	str->ReadDword( &SpellMemorizationCount );
	str->ReadDword( &MemorizedSpellsOffset );
	str->ReadDword( &MemorizedSpellsCount );

	str->ReadDword( &ItemSlotsOffset );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &ItemsCount );
	str->ReadDword( &EffectsOffset );
	str->ReadDword( &EffectsCount );

	str->ReadResRef( act->Dialog );
}

void CREImp::ReadInventory(Actor *act, unsigned int Inventory_Size)
{
	CREItem** items;
	unsigned int i,j,k;

	str->Seek( ItemsOffset, GEM_STREAM_START );
	items = (CREItem **) calloc (ItemsCount, sizeof(CREItem *) );

	for (i = 0; i < ItemsCount; i++) {
		items[i] = core->ReadItem(str); //could be NULL item
	}
	act->inventory.SetSlotCount(Inventory_Size);

	str->Seek( ItemSlotsOffset, GEM_STREAM_START );
	for (i = 0; i < Inventory_Size; i++) {
		ieWord index;
		str->Read( &index, 2 );

		if (index != 0xFFFF) {
			if (index>=ItemsCount) {
				printf("[CREImp]: Invalid item index (%d) in creature!\n", index);
				continue;
			}
			if (items[index]) {
				act->inventory.SetSlotItem( items[index], i );
				printf( "SLOT %d %s\n", i, items[index]->ItemResRef);
				if (core->QuerySlotEffects( i )) {
					printf( "EQUIP 0x%04x\n", items[index]->Flags );
					if ( act->inventory.EquipItem( i ) ) {
						printf( "EQUIP2 0x%04x\n", items[index]->Flags );
					}

				}
				items[index] = NULL;
				continue;
			}
			printf("[CREImp]: Duplicate or (no-drop) item (%d) in creature!\n", index);
		}
	}

	i = ItemsCount;
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

	CREKnownSpell **known_spells=(CREKnownSpell **) calloc(KnownSpellsCount, sizeof(CREKnownSpell *) );
	CREMemorizedSpell **memorized_spells=(CREMemorizedSpell **) calloc(MemorizedSpellsCount, sizeof(CREKnownSpell *) );

	str->Seek( KnownSpellsOffset, GEM_STREAM_START );
	for (i = 0; i < KnownSpellsCount; i++) {
		known_spells[i]=GetKnownSpell();
	}

	str->Seek( MemorizedSpellsOffset, GEM_STREAM_START );
	for (i = 0; i < MemorizedSpellsCount; i++) {
		memorized_spells[i]=GetMemorizedSpell();
	}

	str->Seek( SpellMemorizationOffset, GEM_STREAM_START );
	for (i = 0; i < SpellMemorizationCount; i++) {
		CRESpellMemorization* sm = GetSpellMemorization();

		j=KnownSpellsCount;
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
		for (j = 0; j < MemorizedCount; j++) {
			k = MemorizedIndex+j;
			if (memorized_spells[k]) {
				sm->memorized_spells.push_back( memorized_spells[k]);
				memorized_spells[k] = NULL;
				continue;
			}
			printf("[CREImp]: Duplicate memorized spell (%d) in creature!\n", k);
		}
		act->spellbook.AddSpellMemorization( sm );
	}

	i=KnownSpellsCount;
	while(i--) {
		if (known_spells[i]) {
			printMessage("CREImp"," ", YELLOW);
			printf("Dangling spell in creature: %s!\n", known_spells[i]->SpellResRef);
			delete known_spells[i];
		}
	}
	free(known_spells);

	i=MemorizedSpellsCount;
	while(i--) {
		if (memorized_spells[i]) {
			printMessage("CREImp"," ", YELLOW);
			printf("Dangling spell in creature: %s!\n", memorized_spells[i]->SpellResRef);
			delete memorized_spells[i];
		}
	}
	free(memorized_spells);
}

void CREImp::ReadEffects(Actor *act)
{
	unsigned int i;

	str->Seek( EffectsOffset, GEM_STREAM_START );

	for (i = 0; i < EffectsCount; i++) {
		//str->Seek( EffectsOffset + i * (TotSCEFF ? 264 : 48), GEM_STREAM_START );
		Effect fx;
		GetEffect( &fx );
		// NOTE: AddEffect() allocates a new effect
		act->fxqueue.AddEffect( &fx ); // FIXME: don't reroll dice, time, etc!!
	}
}

void CREImp::GetEffect(Effect *fx)
{
	EffectMgr* eM = ( EffectMgr* ) core->GetInterface( IE_EFF_CLASS_ID );

	eM->Open( str, false );
	if (TotSCEFF) {
		 eM->GetEffectV20( fx );
	} else {
		 eM->GetEffectV1( fx );
	}
	core->FreeInterface( eM );

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
	for (i=0;i<21;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for (i=0;i<100;i++) {
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
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);
	 
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

	str->ReadDword( &KnownSpellsOffset );
	str->ReadDword( &KnownSpellsCount );
	str->ReadDword( &SpellMemorizationOffset );
	str->ReadDword( &SpellMemorizationCount );
	str->ReadDword( &MemorizedSpellsOffset );
	str->ReadDword( &MemorizedSpellsCount );
	 
	str->ReadDword( &ItemSlotsOffset );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &ItemsCount );
	str->ReadDword( &EffectsOffset );
	str->ReadDword( &EffectsCount );

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
	str->Seek( 4, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=tmpByte;
	str->Seek( 34, GEM_CURRENT_POS ); //unknowns
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CLASSLEVELSUM]=tmpByte; //total levels
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELBARBARIAN]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELBARD]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELCLERIC]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELDRUID]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELFIGHTER]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELMAGE]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELMONK]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELPALADIN]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELRANGER]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELTHIEF]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELSORCEROR]=tmpByte;
	str->Seek( 22, GEM_CURRENT_POS ); //levels for classes
	for (i=0;i<64;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	ReadScript( act, SCR_AREA);
	ReadScript( act, SCR_RESERVED);
	str->Seek( 4, GEM_CURRENT_POS );
	str->ReadDword( &act->BaseStats[IE_FEATS1]);
	str->ReadDword( &act->BaseStats[IE_FEATS2]);
	str->ReadDword( &act->BaseStats[IE_FEATS3]);
	str->Seek( 12, GEM_CURRENT_POS );
	//proficiencies
	for (i=0;i<26;i++) {
		str->Read( &tmpByte, 1);
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	//skills
	str->Seek( 38, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALCHEMY]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ANIMALS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_BLUFF]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CONCENTRATION]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_DIPLOMACY]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_INTIMIDATE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SEARCH]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SPELLCRAFT]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_MAGICDEVICE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 51, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	//we got 7 more hated races
	for (i=0;i<7;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_HATEDRACE2+i]=tmpByte;
	}	
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
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);

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
	act->KillVar[0]=0;

	KnownSpellsOffset = 0;
	KnownSpellsCount = 0;
	SpellMemorizationOffset = 0;
	SpellMemorizationCount = 0;
	MemorizedSpellsOffset = 0;
	MemorizedSpellsCount = 0;
	//skipping spellbook offsets
	str->Seek( 606, GEM_CURRENT_POS);

	str->ReadDword( &ItemSlotsOffset );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &ItemsCount );
	str->ReadDword( &EffectsOffset );
	str->ReadDword( &EffectsCount );

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
	for (i=0;i<21;i++) {
		str->Read( &tmpByte, 1 ); 
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 ); 
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for (i=0;i<100;i++) {
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
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);
	
	str->Seek( 46, GEM_CURRENT_POS );
	char KillVar[33]; //use this as needed
	str->Read(KillVar,32);
	KillVar[32]=0;
	str->Seek( 26, GEM_CURRENT_POS );
 
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
	strnspccpy(act->KillVar, KillVar, 32);

	str->ReadDword( &KnownSpellsOffset );
	str->ReadDword( &KnownSpellsCount );
	str->ReadDword( &SpellMemorizationOffset );
	str->ReadDword( &SpellMemorizationCount );
	str->ReadDword( &MemorizedSpellsOffset );
	str->ReadDword( &MemorizedSpellsCount );

	str->ReadDword( &ItemSlotsOffset );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &ItemsCount );
	str->ReadDword( &EffectsOffset );
	str->ReadDword( &EffectsCount );

	str->ReadResRef( act->Dialog );
}

int CREImp::GetStoredFileSize(Actor *actor)
{
	int headersize;
	int Inventory_Size;
	int i;

	switch (actor->version) {
		case IE_CRE_GEMRB:
			headersize = 0x2d4;
			Inventory_Size=actor->inventory.GetSlotCount();
			break;
		case IE_CRE_V1_0://bg1/bg2
			headersize = 0x2d4;
			Inventory_Size=38;
			break;
		case IE_CRE_V1_2: //pst
			headersize = 0x378;
			Inventory_Size=46;
			break;
		case IE_CRE_V2_2://iwd2
			headersize = 0x33c; //?
			Inventory_Size=50; 
			break;
		case IE_CRE_V9_0://iwd
			headersize = 0x33c;
			Inventory_Size=38; 
			break;
		default:
			return -1;
	}
	KnownSpellsOffset = headersize;

	//adding known spells
	KnownSpellsCount = actor->spellbook.GetTotalKnownSpellsCount();
	headersize += KnownSpellsCount * 12;
	SpellMemorizationOffset = headersize;

	//adding spell pages
	SpellMemorizationCount = actor->spellbook.GetTotalPageCount();
	headersize += SpellMemorizationCount * 16;
	MemorizedSpellsOffset = headersize;

	MemorizedSpellsCount = actor->spellbook.GetTotalMemorizedSpellsCount();
	headersize += MemorizedSpellsCount * 12;
	EffectsOffset = headersize;

	//adding effects
	EffectsCount = actor->locals->GetCount();
	if (TotSCEFF) {
		headersize += EffectsCount * 264;
	} else {
		headersize += EffectsCount * 48;
	}
	ItemsOffset = headersize;

	//counting items (calculating item storage)
	ItemsCount = 0;
	for (i=0;i<Inventory_Size;i++) {
		CREItem *it = actor->inventory.GetSlotItem(i);
		if (it) {
			ItemsCount++;
		}
	}
	headersize += ItemsCount * 20;
	ItemSlotsOffset = headersize;

	//adding itemslot table size and equipped slot field
	return headersize + (Inventory_Size)*sizeof(ieWord)+sizeof(ieDword);
}

int CREImp::PutInventory(DataStream *stream, Actor *actor, unsigned int size)
{
	unsigned int i;
	ieDword tmpDword;
	ieWord ItemCount = 0;
	ieWord *indices =(ieWord *) malloc(size*sizeof(ieWord) );
	
	for (i=0;i<size;i++) {
		indices[i]=(ieWord) -1;
	}
	for (i=0;i<size;i++) {
		CREItem *it = actor->inventory.GetSlotItem(i);
		if (!it) {
			continue;
		}
		stream->WriteResRef( it->ItemResRef);
		stream->WriteWord( &it->PurchasedAmount);
		stream->WriteWord( &it->Usages[0]);
		stream->WriteWord( &it->Usages[1]);
		stream->WriteWord( &it->Usages[2]);
		stream->WriteDword( &it->Flags);
		indices[i] = ItemCount++;
	}
	for (i=0;i<size;i++) {
		stream->WriteWord( indices+i);
	}
	tmpDword = actor->inventory.GetEquippedSlot();
	stream->WriteDword( &tmpDword);
	free(indices);
	return 0;
}

int CREImp::PutHeader(DataStream *stream, Actor *actor)
{
	char Signature[8];
	ieByte tmpByte;
	ieWord tmpWord;
	int i;
	char filling[51];

	memset(filling,0,sizeof(filling));
	memcpy( Signature, "CRE V0.0", 8);
	Signature[5]+=actor->version/10;
	Signature[7]+=actor->version%10;
	stream->Write( Signature, 8);
	stream->WriteDword( &actor->ShortStrRef);
	stream->WriteDword( &actor->LongStrRef);
	stream->WriteDword( &actor->BaseStats[IE_MC_FLAGS]);
	stream->WriteDword( &actor->BaseStats[IE_XP]);
	stream->WriteDword( &actor->BaseStats[IE_XPVALUE]);
	stream->WriteDword( &actor->BaseStats[IE_GOLD]);
	stream->WriteDword( &actor->BaseStats[IE_STATE_ID]);
	tmpWord = actor->BaseStats[IE_HITPOINTS];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_MAXHITPOINTS];
	stream->WriteWord( &tmpWord);
	stream->WriteDword( &actor->BaseStats[IE_ANIMATION_ID]);
	for (i=0;i<7;i++) {
		Signature[i] = (char) actor->BaseStats[IE_METAL_COLOR+i];
	}
	//old effect type (additional check needed for bg1)
	if (actor->version==IE_CRE_V1_2) { //pst effect
		Signature[7] = 0;
	} else {
		Signature[7] = 1;
	}
	stream->Write( Signature, 8);
	stream->WriteResRef( actor->SmallPortrait);
	stream->WriteResRef( actor->LargePortrait);
	tmpByte = actor->BaseStats[IE_REPUTATION];
	stream->Write( &tmpByte, 1 );
	tmpByte = actor->BaseStats[IE_HIDEINSHADOWS];
	stream->Write( &tmpByte, 1 );
	//from here it differs, slightly
	tmpWord = actor->BaseStats[IE_ARMORCLASS];
	stream->WriteWord( &tmpWord);
	//iwd2 doesn't store this, probably we shouldn't either?
	if (actor->version != IE_CRE_V2_2) {
		tmpWord = actor->Modified[IE_ARMORCLASS];
		stream->WriteWord( &tmpWord);
	}
	tmpWord = actor->BaseStats[IE_ACCRUSHINGMOD];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_ACMISSILEMOD];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_ACPIERCINGMOD];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_ACSLASHINGMOD];
	stream->WriteWord( &tmpWord);
	tmpByte = actor->BaseStats[IE_THAC0];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_NUMBEROFATTACKS];
	stream->Write( &tmpByte, 1);
	if (actor->version == IE_CRE_V2_2) {
		tmpByte = actor->BaseStats[IE_SAVEFORTITUDE];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_SAVEREFLEX];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_SAVEWILL];
		stream->Write( &tmpByte,1);
	} else {
		tmpByte = actor->BaseStats[IE_SAVEVSDEATH];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_SAVEVSWANDS];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_SAVEVSPOLY];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_SAVEVSBREATH];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_SAVEVSSPELL];
		stream->Write( &tmpByte,1);
	}
	tmpByte = actor->BaseStats[IE_RESISTFIRE];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTCOLD];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTELECTRICITY];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTACID];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTMAGIC];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTMAGICFIRE];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTMAGICCOLD];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTSLASHING];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTCRUSHING];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTPIERCING];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RESISTMISSILE];
	stream->Write( &tmpByte,1);
	if (actor->version == IE_CRE_V2_2) {
		tmpByte = actor->BaseStats[IE_MAGICDAMAGERESISTANCE];
		stream->Write( &tmpByte,1);
		stream->Write( Signature, 4);
	} else {
		tmpByte = actor->BaseStats[IE_DETECTILLUSIONS];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_SETTRAPS];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LORE];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LOCKPICKING];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_STEALTH];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_TRAPS];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_PICKPOCKET];
		stream->Write( &tmpByte,1);
	}
	tmpByte = actor->BaseStats[IE_FATIGUE];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_INTOXICATION];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_LUCK];
	stream->Write( &tmpByte,1);

	if (actor->version == IE_CRE_V2_2) {
		//this is rather fuzzy
		//turnundead level, + 33 bytes of zero
		tmpByte = actor->BaseStats[IE_TURNUNDEADLEVEL];
		stream->Write(&tmpByte,1);
		stream->Write( filling,33);
		//total levels
		tmpByte = actor->BaseStats[IE_CLASSLEVELSUM];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELBARBARIAN];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELBARD];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELCLERIC];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELDRUID];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELFIGHTER];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELMONK];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELPALADIN];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELRANGER];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELTHIEF];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELSORCEROR];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVELMAGE];
		stream->Write( &tmpByte,1);
		//some stuffing
		stream->Write( filling, 22);
		//string references
		for (i=0;i<64;i++) {
			stream->WriteDword( &actor->StrRefs[i]);
		}
		stream->WriteResRef( actor->Scripts[SCR_AREA]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_RESERVED]->GetName() );
		//unknowns before feats
		stream->Write( filling,4);
		//feats
		stream->WriteDword( &actor->BaseStats[IE_FEATS1]);
		stream->WriteDword( &actor->BaseStats[IE_FEATS2]);
		stream->WriteDword( &actor->BaseStats[IE_FEATS3]);
		stream->Write( filling, 12);
		//proficiencies
		for (i=0;i<26;i++) {
			tmpByte = actor->BaseStats[IE_PROFICIENCYBASTARDSWORD+i];
			stream->Write( &tmpByte,1);
		}
		stream->Write( filling, 38);
		//alchemy
		tmpByte = actor->BaseStats[IE_ALCHEMY];
		stream->Write( &tmpByte,1);
		//animals
		tmpByte = actor->BaseStats[IE_ANIMALS];
		stream->Write( &tmpByte,1);
		//bluff
		tmpByte = actor->BaseStats[IE_BLUFF];
		stream->Write( &tmpByte,1);
		//concentration
		tmpByte = actor->BaseStats[IE_CONCENTRATION];
		stream->Write( &tmpByte,1);
		//diplomacy
		tmpByte = actor->BaseStats[IE_DIPLOMACY];
		stream->Write( &tmpByte,1);
		//disarm trap
		tmpByte = actor->BaseStats[IE_TRAPS];
		stream->Write( &tmpByte,1);
		//hide
		tmpByte = actor->BaseStats[IE_HIDEINSHADOWS];
		stream->Write( &tmpByte,1);
		//intimidate
		tmpByte = actor->BaseStats[IE_INTIMIDATE];
		stream->Write( &tmpByte,1);
		//lore
		tmpByte = actor->BaseStats[IE_LORE];
		stream->Write( &tmpByte,1);
		//move silently
		tmpByte = actor->BaseStats[IE_STEALTH];
		stream->Write( &tmpByte,1);
		//open lock
		tmpByte = actor->BaseStats[IE_LOCKPICKING];
		stream->Write( &tmpByte,1);
		//pickpocket
		tmpByte = actor->BaseStats[IE_PICKPOCKET];
		stream->Write( &tmpByte,1);
		//search
		tmpByte = actor->BaseStats[IE_SEARCH];
		stream->Write( &tmpByte,1);
		//spellcraft
		tmpByte = actor->BaseStats[IE_SPELLCRAFT];
		stream->Write( &tmpByte,1);
		//use magic device
		tmpByte = actor->BaseStats[IE_MAGICDEVICE];
		stream->Write( &tmpByte,1);
		//tracking
		tmpByte = actor->BaseStats[IE_TRACKING];
		stream->Write( &tmpByte,1);
		stream->Write( filling, 51);
		tmpByte = actor->BaseStats[IE_HATEDRACE];
		stream->Write( &tmpByte,1);
		for (i=0;i<7;i++) {
			tmpByte = actor->BaseStats[IE_HATEDRACE2+i];
			stream->Write( &tmpByte,1);
		}
		tmpByte = actor->BaseStats[IE_SUBRACE];
		stream->Write( &tmpByte,1);
		stream->Write( filling, 1); //unknown
		tmpByte = actor->BaseStats[IE_SEX]; //
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_STR];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_INT];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_WIS];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_DEX];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_CON];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_CHR];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_MORALE];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_MORALEBREAK];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_MORALERECOVERYTIME];
		stream->Write( &tmpByte,1);
		// unknown byte
		stream->Write( &Signature,1);
		stream->WriteDword( &actor->BaseStats[IE_KIT] );
		stream->WriteResRef( actor->Scripts[SCR_OVERRIDE]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_CLASS]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_RACE]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_GENERAL]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_DEFAULT]->GetName() );
	} else {
		for (i=0;i<21;i++) {
			tmpByte = actor->BaseStats[IE_PROFICIENCYBASTARDSWORD+i];
			stream->Write( &tmpByte,1);
		}
		tmpByte = actor->BaseStats[IE_TRACKING];
		stream->Write( &tmpByte,1);
		for (i=0;i<4;i++)
		{
			stream->Write( Signature, 8);
		}
		for (i=0;i<100;i++) {
			stream->WriteDword( &actor->StrRefs[i]);
		}
		tmpByte = actor->BaseStats[IE_LEVEL];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVEL2];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_LEVEL3];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_SEX]; //
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_STR];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_STREXTRA];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_INT];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_WIS];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_DEX];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_CON];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_CHR];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_MORALE];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_MORALEBREAK];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_HATEDRACE];
		stream->Write( &tmpByte,1);
		tmpByte = actor->BaseStats[IE_MORALERECOVERYTIME];
		stream->Write( &tmpByte,1);
		// unknown byte
		stream->Write( &Signature,1);
		stream->WriteDword( &actor->BaseStats[IE_KIT] );
		stream->WriteResRef( actor->Scripts[SCR_OVERRIDE]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_CLASS]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_RACE]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_GENERAL]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_DEFAULT]->GetName() );
	}
	//now follows the fuzzy part in separate putactor... functions
	return 0;
}

int CREImp::PutActorGemRB(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	char filling[5];

	memset(filling,0,sizeof(filling));
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte,1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte,1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImp::PutActorBG(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	char filling[5];

	memset(filling,0,sizeof(filling));
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte,1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte,1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImp::PutActorPST(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	ieWord tmpWord;
	int i;
	char filling[44];

	memset(filling,0,sizeof(filling));
	stream->Write(filling, 44); //11*4 totally unknown
	stream->WriteDword( &actor->BaseStats[IE_XP_MAGE]);
	stream->WriteDword( &actor->BaseStats[IE_XP_THIEF]);
	for (i = 0; i<10; i++) {
		tmpWord = actor->BaseStats[IE_INTERNAL_0];
		stream->WriteWord( &tmpWord );
	}
	stream->Write(filling,4); //unknown
	stream->Write(actor->KillVar, 32);
	stream->Write(filling,3); //unknown
	tmpByte=actor->BaseStats[IE_COLORCOUNT];
	stream->Write( &tmpByte, 1);
	stream->WriteWord( &actor->AppearanceFlags1);
	stream->WriteWord( &actor->AppearanceFlags2);
	for (i=0;i<7;i++) {
		tmpWord = actor->BaseStats[IE_COLORS+i];
		stream->WriteWord( &tmpWord);
	}
	stream->Write(filling,31);
	tmpByte = actor->BaseStats[IE_SPECIES];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_TEAM];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_FACTION];
	stream->Write( &tmpByte,1);
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte,1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte,1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImp::PutActorIWD1(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	char filling[52];

	memset(filling,0,sizeof(filling));
	stream->Write(filling, 28); //7*4 totally unknown
	stream->Write(actor->KillVar, 32); //some variable names in iwd
	stream->Write(actor->KillVar, 32); //some variable names in iwd
	stream->Write(filling, 52); //13*4 totally unknown
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte,1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte,1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImp::PutActorIWD2(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	ieWord tmpWord;
	int i;
	char filling[146];

	memset(filling,0,sizeof(filling));
	stream->Write( filling,4);
	for (i=0;i<5;i++) {
		tmpWord = actor->BaseStats[IE_INTERNAL_0+i];
		stream->WriteWord ( &tmpWord);
	}
	stream->Write( filling, 66);
	tmpWord = actor->BaseStats[IE_SAVEDXPOS];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDYPOS];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDFACE];
	stream->WriteWord( &tmpWord);
	stream->Write( filling, 146);
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte,1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte,1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte,1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImp::PutKnownSpells( DataStream *stream, Actor *actor)
{
	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			unsigned int count = actor->spellbook.GetKnownSpellsCount(i, j);
			for (unsigned int k=0;k<count;k++) {
				CREKnownSpell *ck = actor->spellbook.GetKnownSpell(i, j, k);
				stream->WriteResRef(ck->SpellResRef);
				stream->WriteWord( &ck->Level);
				stream->WriteWord( &ck->Type);
			}
		}
	}
	return 0;
}

int CREImp::PutSpellPages( DataStream *stream, Actor *actor)
{
	ieWord tmpWord;
	ieDword tmpDword;
	ieDword SpellIndex = 0;

	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			tmpWord = j+1;
			stream->WriteWord( &tmpWord);
			tmpWord = actor->spellbook.GetMemorizableSpellsCount(i,j,false);
			stream->WriteWord( &tmpWord);
			tmpWord = actor->spellbook.GetMemorizableSpellsCount(i,j,true);
			stream->WriteWord( &tmpWord);
			tmpWord = type;
			stream->WriteWord( &tmpWord);
			stream->WriteDword( &SpellIndex);
			tmpDword = actor->spellbook.GetMemorizedSpellsCount(i,j);
			stream->WriteDword( &tmpDword);
			SpellIndex += tmpDword;
		}
	}
	return 0;
}

int CREImp::PutMemorizedSpells(DataStream *stream, Actor *actor)
{
	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			unsigned int count = actor->spellbook.GetMemorizedSpellsCount(i,j);
			for (unsigned int k=0;k<count;k++) {
				CREMemorizedSpell *cm = actor->spellbook.GetMemorizedSpell(i,j,k);

				stream->WriteResRef( cm->SpellResRef);
				stream->WriteDword( &cm->Flags);
			}
		}
	}
	return 0;
}
int CREImp::PutEffects( DataStream *stream, Actor *actor)
{
	char filling[0x268];

	memset(filling,0,sizeof(filling) );
	for(unsigned int i=0;i<EffectsCount-actor->locals->GetCount();i++) {
		if (TotSCEFF) {
			stream->Write( filling, 264);
		} else {
			stream->Write( filling, 48);
		}
	}
	return 0;
}

//add as effect!
int CREImp::PutVariables( DataStream *stream, Actor *actor)
{
	char filling[92];
	POSITION pos=NULL;
	const char *name;
	ieDword tmpDword, value;

	memset(filling,0,sizeof(filling) );
	unsigned int VariablesCount = actor->locals->GetCount();
	for (unsigned int i=0;i<VariablesCount;i++) {
		actor->locals->GetNextAssoc( pos, name, value);
		stream->Write(filling,8);
		tmpDword = FAKE_VARIABLE_OPCODE;
		stream->WriteDword( &tmpDword);
		stream->Write(filling,8); //type, power
		stream->WriteDword( &value); //param #1
		stream->Write( filling, 40); //param #2, timing, duration, chance, resource, dices, saves
		tmpDword = FAKE_VARIABLE_MARKER;
		stream->WriteDword( &value); //variable marker
		stream->Write( filling, 92); //23 * 4
		stream->Write( name, 32);
		stream->Write( filling, 72); //18 * 4
	}
	return 0;
}

int CREImp::PutActor(DataStream *stream, Actor *actor)
{
	int ret;

	if (!stream || !actor) {
		return -1;
	}

	ret = PutHeader( stream, actor);
	if (ret) {
		return ret;
	}
	//here comes the fuzzy part
	unsigned int Inventory_Size;

	switch (actor->version) {
		case IE_CRE_GEMRB:
			TotSCEFF = 1;
			Inventory_Size=actor->inventory.GetSlotCount();
			ret = PutActorGemRB(stream, actor);
			break;
		case IE_CRE_V1_2:
			TotSCEFF = 0;
			Inventory_Size=46;
			ret = PutActorPST(stream, actor);
			break;
		case IE_CRE_V1_0: //bg1/bg2
			// somehow we have to know if it is bg1
			TotSCEFF = 1;
			Inventory_Size=38;
			ret = PutActorBG(stream, actor);
			break;
		case IE_CRE_V2_2:
			TotSCEFF = 1;
			Inventory_Size=50;
			ret = PutActorIWD2(stream, actor);
			break;
		case IE_CRE_V9_0:
			TotSCEFF = 1;
			Inventory_Size=38;
			ret = PutActorIWD1(stream, actor);
			break;
		default:
			return -1;
	}
	if (ret) {
		return ret;
	}

	//writing offsets
	if (actor->version==IE_CRE_V2_2) {
		//
	} else {
		stream->WriteDword( &KnownSpellsOffset);
		stream->WriteDword( &KnownSpellsCount);
		stream->WriteDword( &SpellMemorizationOffset );
		stream->WriteDword( &SpellMemorizationCount );
		stream->WriteDword( &MemorizedSpellsOffset );
		stream->WriteDword( &MemorizedSpellsCount );
	}
	stream->WriteDword( &ItemSlotsOffset );
	stream->WriteDword( &ItemsOffset );
	stream->WriteDword( &ItemsCount );
	stream->WriteDword( &EffectsOffset );
	stream->WriteDword( &EffectsCount );
	stream->WriteResRef( actor->Dialog);
	//spells, spellbook etc

	ret = PutKnownSpells( stream, actor);
	if (ret) {
		return ret;
	}
	ret = PutSpellPages(stream, actor);
	if (ret) {
		return ret;
	}
	ret = PutMemorizedSpells(stream, actor);
	if (ret) {
		return ret;
	}
	ret = PutEffects(stream, actor);
	if (ret) {
		return ret;
	}
	//effects and variables
	ret = PutVariables(stream, actor);
	if (ret) {
		return ret;
	}

	//items and inventory slots
	ret = PutInventory( stream, actor, Inventory_Size);
	if (ret) {
		return ret;
	}
	return 0;
}

