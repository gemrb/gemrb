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

#include "ITMImporter.h"

#include "win32def.h"

#include "EffectMgr.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "SymbolMgr.h"
#include "TableMgr.h" //needed for autotable

#include <map>

using namespace GemRB;

int *profs = NULL;
int profcount = -1;

static EffectRef fx_tohit_vs_creature_ref = { "ToHitVsCreature", -1 };
static EffectRef fx_damage_vs_creature_ref = { "DamageVsCreature", -1 };
static EffectRef zzRefs[] = { fx_tohit_vs_creature_ref, fx_damage_vs_creature_ref };
std::map<char,int> zzmap;

//cannot call this at the time of initialization because the tablemanager isn't alive yet
static void Initializer()
{
	if (profs) {
		free(profs);
		profs = NULL;
	}
	profcount = 0;
	AutoTable tm("proftype");
	if (!tm) {
		Log(ERROR, "ITMImporter", "Cannot find proftype.2da.");
		return;
	}
	profcount = tm->GetRowCount();
	profs = (int *) calloc( profcount, sizeof(int) );
	for (int i = 0; i < profcount; i++) {
		profs[i] = atoi(tm->QueryField( i, 0 ) );
	}

	// check for iwd1 zz-weapon bonus table
	AutoTable tm2("zzweaps");
	int indR = core->LoadSymbol("race");
	Holder<SymbolMgr> sm = core->GetSymbol(indR);
	if (!tm2 || !sm || indR == -1) {
		return;
	}
	// resolve table into directly usable form
	int zzcount = tm2->GetRowCount();
	for (int i = 0; i < zzcount; i++) {
		const char *rowname = tm2->GetRowName(i);
		const char *field = tm2->QueryField(i, 0);
		long val = atoi(field);
		if (val == 0) {
			// not numeric, do an IDS lookup
			val = sm->GetValue(field);
		}
		zzmap[*rowname] = val;
	}
}

static void ReleaseMemoryITM()
{
	free(profs);
	profs = NULL;
	profcount = -1;
}

static int GetProficiency(ieDword ItemType)
{
	if (profcount<0) {
		Initializer();
	}

	if (ItemType>=(ieDword) profcount) {
		return 0;
	}
	return profs[ItemType];
}

ITMImporter::ITMImporter(void)
{
	str = NULL;
	version = 0;
}

ITMImporter::~ITMImporter(void)
{
	delete str;
	str = NULL;
}

bool ITMImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "ITM V1  ", 8 ) == 0) {
		version = 10;
	} else if (strncmp( Signature, "ITM V1.1", 8 ) == 0) {
		version = 11;
	} else if (strncmp( Signature, "ITM V2.0", 8 ) == 0) {
		version = 20;
	} else {
		Log(WARNING, "ITMImporter", "This file is not a valid ITM file! Actual signature: %s", Signature);
		return false;
	}

	return true;
}

// iwd1 has hardcoded bonuses (weapons named ZZ*) vs race or alignment mask
// bg1 already handled it with external effects, so this was actually not necessary
// Example: ZZ05WE, +1 bonus vs lawful alignments or Giant Killer (+1, +4 vs. Giants)
// NOTE: the original did not increment enchantment level nor weapon speed
static void AddZZFeatures(Item *s)
{
	// the targeting code (3rd char) is: digit = align(ment), letter = race
	char targetIDS = toupper(s->Name[2]);
	ieByte IDSval = zzmap[targetIDS];
	ieByte IDSfile = 4;
	if (atoi(&targetIDS)) {
		IDSfile = 8;
	}

	// the numeric code (4th char) is translated to attack bonus with this pattern:
	// 0: -5, 1: -4, 2: -3, 3: -2, 4: -1,
	// 5: +1, 6:+2 ... 9: +5
	// this bonus is on top of the default one, so less descriptions are wrong than it may seem
	int bonus = atoi(&s->Name[3]);
	if (bonus < 5) {
		bonus -= 5;
	} else {
		bonus -= 4;
	}

	// append the new equipping effects (tohit+damage)
	for (unsigned int i=0; i < sizeof(zzRefs)/sizeof(*zzRefs); i++) {
		Effect *fx = EffectQueue::CreateEffect(zzRefs[i], IDSval, IDSfile, FX_DURATION_INSTANT_WHILE_EQUIPPED);
		fx->Parameter3 = bonus;
		CopyResRef(fx->Source, s->Name);
		// use the space reserved earlier
		memcpy(s->equipping_features + (s->EquippingFeatureCount - 1 - i), fx, sizeof(Effect));
		delete fx;
	}
}

Item* ITMImporter::GetItem(Item *s)
{
	unsigned int i;
	ieByte k1,k2,k3,k4;

	if( !s) {
		return NULL;
	}
	str->ReadDword( &s->ItemName );
	str->ReadDword( &s->ItemNameIdentified );
	str->ReadResRef( s->ReplacementItem );
	str->ReadDword( &s->Flags );
	str->ReadWord( &s->ItemType );
	str->ReadDword( &s->UsabilityBitmask );
	str->Read( s->AnimationType,2 ); //intentionally not reading word!
	for (i=0;i<2;i++) {
		if (s->AnimationType[i]==' ') {
			s->AnimationType[i]=0;
		}
	}
	str->Read( &s->MinLevel, 1 );
	str->Read( &s->unknown1, 1 );
	str->Read( &s->MinStrength,1 );
	str->Read( &s->unknown2, 1 );
	str->Read( &s->MinStrengthBonus, 1 );
	str->Read( &k1,1 );
	str->Read( &s->MinIntelligence, 1 );
	str->Read( &k2,1 );
	str->Read( &s->MinDexterity, 1 );
	str->Read( &k3,1 );
	str->Read( &s->MinWisdom, 1 );
	str->Read( &k4,1 );
	s->KitUsability=(k1<<24) | (k2<<16) | (k3<<8) | k4; //bg2/iwd2 specific
	str->Read( &s->MinConstitution, 1 );
	str->Read( &s->WeaProf, 1 ); //bg2 specific

	//hack for non bg2 weapon proficiencies
	if (!s->WeaProf) {
		s->WeaProf = GetProficiency(s->ItemType);
	}

	str->Read( &s->MinCharisma, 1 );
	str->Read( &s->unknown3, 1 );
	str->ReadDword( &s->Price );
	str->ReadWord( &s->MaxStackAmount );

	//hack for non stacked items, so MaxStackAmount could be used as a boolean
	if (s->MaxStackAmount==1) {
		s->MaxStackAmount = 0;
	}

	str->ReadResRef( s->ItemIcon );
	str->ReadWord( &s->LoreToID );
	str->ReadResRef( s->GroundIcon );
	str->ReadDword( &s->Weight );
	str->ReadDword( &s->ItemDesc );
	str->ReadDword( &s->ItemDescIdentified );
	str->ReadResRef( s->DescriptionIcon );
	str->ReadDword( &s->Enchantment );
	str->ReadDword( &s->ExtHeaderOffset );
	str->ReadWord( &s->ExtHeaderCount );
	str->ReadDword( &s->FeatureBlockOffset );
	str->ReadWord( &s->EquippingFeatureOffset );
	str->ReadWord( &s->EquippingFeatureCount );

	s->WieldColor = 0xffff;
	memset( s->unknown, 0, 26 );

	//skipping header data for iwd2
	if (version == ITM_VER_IWD2) {
		str->Read( s->unknown, 16 );
	}
	if (version == ITM_VER_PST) {
		//pst data
		str->ReadResRef( s->Dialog );
		str->ReadDword( &s->DialogName );
		ieWord WieldColor;
		str->ReadWord( &WieldColor );
		if (s->AnimationType[0]) {
			s->WieldColor = WieldColor;
		}
		str->Read( s->unknown, 26 );
	} else if (dialogTable) {
		//all non pst
		int row = dialogTable->GetRowIndex(s->Name);
		s->DialogName = atoi(dialogTable->QueryField(row, 0));
		CopyResRef(s->Dialog, dialogTable->QueryField(row, 1));
	} else {
		s->DialogName = -1;
		s->Dialog[0] = '\0';
	}

	if (exclusionTable) {
		int row = exclusionTable->GetRowIndex(s->Name);
		s->ItemExcl = atoi(exclusionTable->QueryField(row, 0));
	} else {
		s->ItemExcl = 0;
	}

	s->ext_headers = core->GetITMExt( s->ExtHeaderCount );

	for (i = 0; i < s->ExtHeaderCount; i++) {
		str->Seek( s->ExtHeaderOffset + i * 56, GEM_STREAM_START );
		ITMExtHeader* eh = &s->ext_headers[i];
		GetExtHeader( s, eh );
		// set the tooltip
		if (tooltipTable) {
			int row = tooltipTable->GetRowIndex(s->Name);
			eh->Tooltip = atoi(tooltipTable->QueryField(row, i));
		}
	}

	// handle iwd1/iwd2 weapon "peculiarity"
	bool zzWeapon = false;
	int extraFeatureCount = 0;
	if (!strnicmp(s->Name, "ZZ", 2) && version != 11) {
		zzWeapon = true;
		// reserve space in the effect array
		extraFeatureCount = 2;
	}

	//48 is the size of the feature block
	s->equipping_features = core->GetFeatures(s->EquippingFeatureCount + extraFeatureCount);

	str->Seek( s->FeatureBlockOffset + 48*s->EquippingFeatureOffset,
			GEM_STREAM_START );
	for (i = 0; i < s->EquippingFeatureCount; i++) {
		GetFeature(s->equipping_features+i, s);
	}

	// add remaining features
	if (zzWeapon) {
		AddZZFeatures(s);
	}

	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		Log(ERROR, "ITMImporter", "No BAM Importer available!");
		return NULL;
	}
	return s;
}

//unfortunately, i couldn't avoid this hack, unless adding another array
#define IT_DAGGER     0x10
#define IT_SHORTSWORD 0x13

void ITMImporter::GetExtHeader(Item *s, ITMExtHeader* eh)
{
	ieByte tmpByte;
	ieWord ProjectileType;

	str->Read( &eh->AttackType,1 );
	str->Read( &eh->IDReq,1 );
	str->Read( &eh->Location,1 );
	str->Read( &eh->unknown1,1 );
	str->ReadResRef( eh->UseIcon );
	str->Read( &eh->Target,1 );
	str->Read( &tmpByte,1 );
	if (!tmpByte) {
		tmpByte = 1;
	}
	eh->TargetNumber = tmpByte;
	str->ReadWord( &eh->Range );
	str->ReadWord( &ProjectileType );
	str->ReadWord( &eh->Speed );
	str->ReadWord( &eh->THAC0Bonus );
	str->ReadWord( &eh->DiceSides );
	str->ReadWord( &eh->DiceThrown );
	//if your compiler doesn't like this, then we need a ReadWordSigned
	str->ReadWord( (ieWord *) &eh->DamageBonus );
	str->ReadWord( &eh->DamageType );
	str->ReadWord( &eh->FeatureCount );
	str->ReadWord( &eh->FeatureOffset );
	str->ReadWord( &eh->Charges );
	str->ReadWord( &eh->ChargeDepletion );
	str->ReadDword( &eh->RechargeFlags );

	//hack for default weapon finesse
	if (s->ItemType==IT_DAGGER || s->ItemType==IT_SHORTSWORD) eh->RechargeFlags^=IE_ITEM_USEDEXTERITY;

	str->ReadWord( &eh->ProjectileAnimation );
	//for some odd reasons 0 and 1 are the same
	if (eh->ProjectileAnimation) {
		eh->ProjectileAnimation--;
	}
	// bg2 ignored the projectile for melee weapons (rarely set, but gives staf13 AOE effects)
	if (!core->HasFeature(GF_MELEEHEADER_USESPROJECTILE) && eh->AttackType == ITEM_AT_MELEE) {
		// HACK: use invtrav, so the effects are still applied on the attack target
		eh->ProjectileAnimation = 78;
	}

	for (unsigned int i = 0; i < 3; i++) {
		str->ReadWord( &eh->MeleeAnimation[i] );
	}

	ieWord tmp;
	ieDword pq = 0;
	str->ReadWord( &tmp ); //arrow
	if (tmp) pq |= PROJ_ARROW;
	str->ReadWord( &tmp ); //xbow
	if (tmp) pq |= PROJ_BOLT;
	str->ReadWord( &tmp ); //bullet
	if (tmp) pq |= PROJ_BULLET;
	//this hack is required for Nordom's crossbow in PST
	if (!pq && (eh->AttackType == ITEM_AT_BOW)) {
		pq |= PROJ_BOLT;
	}

	//this hack is required for the practicing arrows in BG1
	if (!pq && (eh->AttackType == ITEM_AT_PROJECTILE)) {
		//0->0
		//1->1
		//2->2
		//3->4
		pq |= (1<<ProjectileType)>>1;
	}
	eh->ProjectileQualifier = pq;

	//48 is the size of the feature block
	eh->features = core->GetFeatures(eh->FeatureCount);
	str->Seek( s->FeatureBlockOffset + 48*eh->FeatureOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < eh->FeatureCount; i++) {
		GetFeature(eh->features+i, s);
	}
}

void ITMImporter::GetFeature(Effect *fx, Item *s)
{
	PluginHolder<EffectMgr> eM(IE_EFF_CLASS_ID);
	eM->Open( str, false );
	eM->GetEffect( fx );
	CopyResRef(fx->Source, s->Name);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xD913A54, "ITM File Importer")
PLUGIN_CLASS(IE_ITM_CLASS_ID, ITMImporter)
PLUGIN_CLEANUP(ReleaseMemoryITM)
END_PLUGIN()
