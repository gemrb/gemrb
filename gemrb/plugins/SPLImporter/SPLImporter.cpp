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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "SPLImporter.h"

#include "EffectMgr.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "TableMgr.h" //needed for autotable

using namespace GemRB;

//cannot call this at the time of initialization because the tablemanager isn't alive yet
static void Initializer()
{
	AutoTable tm = gamedata->LoadTable("cgtable");
	if (!tm) {
		Log(ERROR, "SPLImporter", "Cannot find cgtable.2da.");
		return;
	}

	size_t count = tm->GetRowCount();
	gamedata->castingGlows.resize(count);
	gamedata->castingSounds.resize(count);
	for (size_t i = 0; i < count; i++) {
		gamedata->castingGlows[i] = tm->QueryField(i, 0);
		gamedata->castingSounds[i] = atoi(tm->QueryField(i, 1));
		// * marks an empty resource
		if (IsStar(gamedata->castingGlows[i])) {
			gamedata->castingGlows[i].Reset();
		}
	}

}

static int GetCGSound(ieDword CastingGraphics)
{
	if (gamedata->castingGlows.empty()) {
		Initializer();
	}

	if (CastingGraphics >= gamedata->castingSounds.size()) {
		return -1;
	}
	int ret = -1;
	if (core->HasFeature(GF_CASTING_SOUNDS) ) {
		ret = gamedata->castingSounds[CastingGraphics];
		if (core->HasFeature(GF_CASTING_SOUNDS2) ) {
			ret |= 0x100;
		}
	} else if (!core->HasFeature(GF_CASTING_SOUNDS2)) {
		ret = gamedata->castingSounds[CastingGraphics];
	}
	return ret;
}

SPLImporter::SPLImporter(void)
{
	str = NULL;
	version = 0;
}

SPLImporter::~SPLImporter(void)
{
	delete str;
}

bool SPLImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "SPL V1  ", 8 ) == 0) {
		version = 1;
	} else if (strncmp( Signature, "SPL V2.0", 8 ) == 0) {
		version = 20;
	} else {
		Log(WARNING, "SPLImporter", "This file is not a valid SPL file! Actual signature: %s", Signature);
		return false;
	}

	return true;
}

Spell* SPLImporter::GetSpell(Spell *s, bool /*silent*/)
{
	str->ReadDword(s->SpellName);
	str->ReadDword(s->SpellNameIdentified);
	str->ReadResRef( s->CompletionSound );
	str->ReadDword(s->Flags);
	str->ReadWord(s->SpellType);
	str->ReadWord(s->ExclusionSchool);
	str->ReadWord(s->PriestType);
	str->ReadWord(s->CastingGraphics);
	s->CastingSound = GetCGSound(s->CastingGraphics);
	str->Read( &s->unknown1, 1 );
	str->ReadWord(s->PrimaryType);
	str->Read( &s->SecondaryType, 1 );
	str->ReadDword(s->unknown2);
	str->ReadDword(s->unknown3);
	str->ReadDword(s->unknown4);
	str->ReadDword(s->SpellLevel);
	str->ReadWord(s->unknown5);
	str->ReadResRef( s->SpellbookIcon );
	//this hack is needed in ToB at least
	if (!s->SpellbookIcon.IsEmpty() && core->HasFeature(GF_SPELLBOOKICONHACK)) {
		ResRef tmp = s->SpellbookIcon;
		s->SpellbookIcon.SNPrintF("%.7sc", tmp.CString());
	}

	str->ReadWord(s->unknown6);
	str->ReadDword(s->unknown7);
	str->ReadDword(s->unknown8);
	str->ReadDword(s->unknown9);
	str->ReadDword(s->SpellDesc);
	str->ReadDword(s->SpellDescIdentified);
	str->ReadDword(s->unknown10);
	str->ReadDword(s->unknown11);
	str->ReadDword(s->unknown12);
	str->ReadDword(s->ExtHeaderOffset);
	ieWord headerCount;
	str->ReadWord(headerCount);
	str->ReadDword(s->FeatureBlockOffset);
	str->ReadWord(s->CastingFeatureOffset);
	str->ReadWord(s->CastingFeatureCount);

	memset( s->unknown13, 0, 14 );
	if (version == 20) {
		//these fields are used in simplified duration
		str->Read( &s->TimePerLevel, 1);
		str->Read( &s->TimeConstant, 1 );
		str->Read( s->unknown13, 14 );
		//moving some bits, because bg2 uses them differently
		//the low byte is unused, so we can keep the iwd2 bits there
		s->Flags|=(s->Flags>>8)&0xc0;
		s->Flags&=~0xc000;
	} else {
		//in case of old format, use some unused fields for gemrb's simplified duration
		//to simulate IWD2's useful feature (this is needed for some pst projectiles)
		if (s->Flags&SF_SIMPLIFIED_DURATION) {
			s->TimePerLevel = s->unknown2 & 255;
			s->TimeConstant = s->unknown3 & 255;
		} else {
			s->TimePerLevel = 0;
			s->TimeConstant = 0;
		}
	}

	s->ext_headers = std::vector<SPLExtHeader>(headerCount);

	for (ieWord i = 0; i < headerCount; i++) {
		str->Seek( s->ExtHeaderOffset + i * 40, GEM_STREAM_START );
		GetExtHeader(s, &s->ext_headers[i]);
	}

	s->casting_features.reserve(s->CastingFeatureCount);
	str->Seek( s->FeatureBlockOffset + 48*s->CastingFeatureOffset,
			GEM_STREAM_START );
	for (int i = 0; i < s->CastingFeatureCount; i++) {
		s->casting_features.push_back(GetFeature(s));
	}

	return s;
}

void SPLImporter::GetExtHeader(const Spell *s, SPLExtHeader* eh)
{
	ieByte tmpByte;

	str->Read( &eh->SpellForm, 1 );
	//this byte is used in PST
	str->Read( &eh->Hostile, 1 );
	str->Read( &eh->Location, 1 );
	str->Read( &eh->unknown2, 1 );
	str->ReadResRef(eh->memorisedIcon);
	str->Read( &eh->Target, 1 );

	//this hack is to let gemrb target dead actors by some spells
	if (eh->Target == 1) {
		if (core->GetSpecialSpell(s->Name)&SPEC_DEAD) {
			eh->Target = 3;
		}
	}
	str->Read( &tmpByte,1 );
	if (!tmpByte) {
		tmpByte = 1;
	}
	eh->TargetNumber = tmpByte;
	str->ReadWord(eh->Range);
	str->ReadWord(eh->RequiredLevel);
	str->ReadDword(eh->CastingTime);
	str->ReadWord(eh->DiceSides);
	str->ReadWord(eh->DiceThrown);
	str->ReadWord(eh->DamageBonus);
	str->ReadWord(eh->DamageType);
	ieWord featureCount;
	str->ReadWord(featureCount);
	str->ReadWord(eh->FeatureOffset);
	str->ReadWord(eh->Charges);
	str->ReadWord(eh->ChargeDepletion);
	str->ReadWord(eh->ProjectileAnimation);

	//for some odd reasons 0 and 1 are the same
	if (eh->ProjectileAnimation) {
		eh->ProjectileAnimation--;
	}
	eh->features.reserve(featureCount);
	str->Seek( s->FeatureBlockOffset + 48*eh->FeatureOffset, GEM_STREAM_START );
	for (ieWord i = 0; i < featureCount; ++i) {
		eh->features.push_back(GetFeature(s));
	}
}

Effect *SPLImporter::GetFeature(const Spell *s)
{
	PluginHolder<EffectMgr> eM = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	eM->Open( str, false );
	Effect* fx = eM->GetEffect();
	fx->SourceRef = s->Name;
	fx->PrimaryType = s->PrimaryType;
	fx->SecondaryType = s->SecondaryType;
	return fx;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xA8D1014, "SPL File Importer")
PLUGIN_CLASS(IE_SPL_CLASS_ID, SPLImporter)
END_PLUGIN()
