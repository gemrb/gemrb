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

#include "EffectMgr.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "SymbolMgr.h"
#include "TableMgr.h" //needed for autotable

#include <map>

using namespace GemRB;

static std::vector<int> profs;
std::map<wchar_t,ieByte> zzmap;

//cannot call this at the time of initialization because the tablemanager isn't alive yet
static void Initializer()
{
	AutoTable tm = gamedata->LoadTable("proftype");
	if (!tm) {
		Log(ERROR, "ITMImporter", "Cannot find proftype.2da.");
		return;
	}
	TableMgr::index_t profcount = tm->GetRowCount();
	profs.resize(profcount);
	for (TableMgr::index_t i = 0; i < profcount; i++) {
		profs[i] = tm->QueryFieldSigned<int>(i, 0);
	}

	// check for iwd1 zz-weapon bonus table
	AutoTable tm2 = gamedata->LoadTable("zzweaps");
	int indR = core->LoadSymbol("race");
	auto sm = core->GetSymbol(indR);
	if (!tm2 || !sm || indR == -1) {
		return;
	}
	// resolve table into directly usable form
	TableMgr::index_t zzcount = tm2->GetRowCount();
	for (TableMgr::index_t i = 0; i < zzcount; i++) {
		const std::string& rowname = tm2->GetRowName(i);
		const auto& field = tm2->QueryField(i, 0);
		int val = atoi(field.c_str());
		if (val == 0) {
			// not numeric, do an IDS lookup
			val = sm->GetValue(field);
		}
		zzmap[towupper(rowname[0])] = val;
	}
}

static int GetProficiency(ieDword ItemType)
{
	if (profs.empty()) {
		Initializer();
	}

	if (ItemType >= profs.size()) {
		return 0;
	}
	return profs[ItemType];
}

bool ITMImporter::Import(DataStream* str)
{
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "ITM V1  ", 8 ) == 0) {
		version = 10;
	} else if (strncmp( Signature, "ITM V1.1", 8 ) == 0) {
		version = 11;
	} else if (strncmp( Signature, "ITM V2.0", 8 ) == 0) {
		version = 20;
	} else {
		Log(WARNING, "ITMImporter", "This file is not a valid ITM file! Actual signature: {}", Signature);
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
	wchar_t targetIDS = towupper(s->Name[2]);
	ieByte IDSval = zzmap[targetIDS];
	ieByte IDSfile = 4;
	if (targetIDS <= '9') {
		IDSfile = 8;
	}

	// the numeric code (4th char) is translated to attack bonus with this pattern:
	// 0: -5, 1: -4, 2: -3, 3: -2, 4: -1,
	// 5: +1, 6: +2 ... 9: +5
	// this bonus is on top of the default one, so less descriptions are wrong than it may seem
	int bonus = atoi(&s->Name[3]);
	if (bonus < 5) {
		bonus -= 5;
	} else {
		bonus -= 4;
	}

	static EffectRef zzRefs[] = { { "ToHitVsCreature", -1 }, { "DamageVsCreature", -1 } };

	// append the new equipping effects (tohit+damage)
	for (unsigned int i=0; i < sizeof(zzRefs)/sizeof(*zzRefs); i++) {
		Effect *fx = EffectQueue::CreateEffect(zzRefs[i], IDSval, IDSfile, FX_DURATION_INSTANT_WHILE_EQUIPPED);
		fx->Parameter3 = bonus;
		fx->SourceRef = s->Name;
		// use the space reserved earlier
		s->equipping_features[s->EquippingFeatureCount - 1 - i] = fx;
	}
}

Item* ITMImporter::GetItem(Item *s)
{
	ieByte k1,k2,k3,k4;

	if( !s) {
		return NULL;
	}
	str->ReadStrRef(s->ItemName);
	str->ReadStrRef(s->ItemNameIdentified);
	str->ReadResRef( s->ReplacementItem );
	str->ReadDword(s->Flags);
	str->ReadWord(s->ItemType);
	str->ReadDword(s->UsabilityBitmask);
	str->ReadRTrimString(s->AnimationType, 2);
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
	str->ReadDword(s->Price);
	str->ReadWord(s->MaxStackAmount);

	//hack for non stacked items, so MaxStackAmount could be used as a boolean
	if (s->MaxStackAmount==1) {
		s->MaxStackAmount = 0;
	}

	str->ReadResRef( s->ItemIcon );
	str->ReadWord(s->LoreToID);
	str->ReadResRef( s->GroundIcon );
	str->ReadDword(s->Weight);
	str->ReadStrRef(s->ItemDesc);
	str->ReadStrRef(s->ItemDescIdentified);
	str->ReadResRef( s->DescriptionIcon );
	str->ReadDword(s->Enchantment);
	str->ReadDword(s->ExtHeaderOffset);
	ieWord headerCount;
	str->ReadWord(headerCount);
	str->ReadDword(s->FeatureBlockOffset);
	str->ReadWord(s->EquippingFeatureOffset);
	str->ReadWord(s->EquippingFeatureCount);

	s->WieldColor = 0xffff;
	memset( s->unknown, 0, 26 );

	//skipping header data for iwd2
	if (version == ITM_VER_IWD2) {
		str->Read( s->unknown, 16 );
	}
	if (version == ITM_VER_PST) {
		//pst data
		str->ReadResRef( s->Dialog );
		str->ReadStrRef(s->DialogName);
		ieWord WieldColor;
		str->ReadWord(WieldColor);
		if (s->AnimationType[0]) {
			s->WieldColor = WieldColor;
		}
		str->Read( s->unknown, 26 );
	} else if (dialogTable) {
		//all non pst
		TableMgr::index_t row = dialogTable->GetRowIndex(s->Name);
		s->DialogName = dialogTable->QueryFieldAsStrRef(row, 0);
		s->Dialog = dialogTable->QueryField(row, 1);
	} else {
		s->DialogName = ieStrRef::INVALID;
		s->Dialog.Reset();
	}

	if (exclusionTable) {
		TableMgr::index_t row = exclusionTable->GetRowIndex(s->Name);
		s->ItemExcl = exclusionTable->QueryFieldUnsigned<ieDword>(row, 0);
	} else {
		s->ItemExcl = 0;
	}

	s->ext_headers = std::vector<ITMExtHeader>(headerCount);

	for (ieWord i = 0; i < headerCount; i++) {
		str->Seek( s->ExtHeaderOffset + i * 56, GEM_STREAM_START );
		ITMExtHeader* eh = &s->ext_headers[i];
		GetExtHeader( s, eh );
		// set the tooltip
		if (tooltipTable) {
			TableMgr::index_t row = tooltipTable->GetRowIndex(s->Name);
			eh->Tooltip = tooltipTable->QueryFieldAsStrRef(row, i);
		}
	}

	// handle iwd1/iwd2 weapon "peculiarity"
	bool zzWeapon = false;
	int extraFeatureCount = 0;
	if (s->Name.BeginsWith("ZZ") && version != 11) {
		zzWeapon = true;
		// reserve space in the effect array
		extraFeatureCount = 2;
	}

	//48 is the size of the feature block
	s->equipping_features.reserve(s->EquippingFeatureCount + extraFeatureCount);

	str->Seek( s->FeatureBlockOffset + 48*s->EquippingFeatureOffset,
			GEM_STREAM_START );
	for (unsigned int i = 0; i < s->EquippingFeatureCount; i++) {
		s->equipping_features.push_back(GetFeature(s));
	}

	// add remaining features
	if (zzWeapon) {
		AddZZFeatures(s);
	}

	return s;
}

//unfortunately, i couldn't avoid this hack, unless adding another array
#define IT_DAGGER     0x10
#define IT_SHORTSWORD 0x13

void ITMImporter::GetExtHeader(const Item *s, ITMExtHeader* eh)
{
	ieByte tmpByte;
	ieWord ProjectileType;

	str->Read( &eh->AttackType,1 );
	str->Read( &eh->IDReq,1 );
	str->Read( &eh->Location,1 );
	str->Read(&eh->AltDiceSides, 1);
	str->ReadResRef( eh->UseIcon );
	str->Read( &eh->Target,1 );
	str->Read( &tmpByte,1 );
	if (!tmpByte) {
		tmpByte = 1;
	}
	eh->TargetNumber = tmpByte;
	str->ReadWord(eh->Range);
	str->Read(&ProjectileType, 1);
	str->Read(&eh->AltDiceThrown, 1);
	str->Read(&eh->Speed, 1);
	str->Read(&eh->AltDamageBonus, 1);
	str->ReadWord(eh->THAC0Bonus);
	str->ReadWord(eh->DiceSides);
	str->ReadWord(eh->DiceThrown);
	str->ReadScalar<ieWordSigned>(eh->DamageBonus);
	str->ReadWord(eh->DamageType);
	ieWord featureCount;
	str->ReadWord(featureCount);
	str->ReadWord(eh->FeatureOffset);
	str->ReadWord(eh->Charges);
	str->ReadWord(eh->ChargeDepletion);
	str->ReadDword(eh->RechargeFlags);

	//hack for default weapon finesse
	if (s->ItemType==IT_DAGGER || s->ItemType==IT_SHORTSWORD) eh->RechargeFlags^=IE_ITEM_USEDEXTERITY;

	str->ReadWord(eh->ProjectileAnimation);
	//for some odd reasons 0 and 1 are the same
	if (eh->ProjectileAnimation) {
		eh->ProjectileAnimation--;
	}
	// bg2 ignored the projectile for melee weapons (rarely set, but gives staf13 AOE effects)
	if (!core->HasFeature(GFFlags::MELEEHEADER_USESPROJECTILE) && eh->AttackType == ITEM_AT_MELEE) {
		// HACK: use invtrav, so the effects are still applied on the attack target
		eh->ProjectileAnimation = 78;
	}

	for (unsigned short& i : eh->MeleeAnimation) {
		str->ReadWord(i);
	}

	ieWord tmp;
	ieDword pq = 0;
	str->ReadWord(tmp); //arrow
	if (tmp) pq |= PROJ_ARROW;
	str->ReadWord(tmp); //xbow
	if (tmp) pq |= PROJ_BOLT;
	str->ReadWord(tmp); //bullet
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
	eh->features.reserve(featureCount);
	str->Seek( s->FeatureBlockOffset + 48*eh->FeatureOffset, GEM_STREAM_START );
	for (ieWord i = 0; i < featureCount; i++) {
		eh->features.push_back(GetFeature(s));
	}
}

Effect* ITMImporter::GetFeature(const Item *s)
{
	PluginHolder<EffectMgr> eM = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	eM->Open( str, false );
	Effect* fx = eM->GetEffect();
	fx->SourceRef = s->Name;
	return fx;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xD913A54, "ITM File Importer")
PLUGIN_CLASS(IE_ITM_CLASS_ID, ImporterPlugin<ITMImporter>)
END_PLUGIN()
