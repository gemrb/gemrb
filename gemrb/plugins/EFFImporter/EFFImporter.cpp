// SPDX-FileCopyrightText: 2005 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "EFFImporter.h"

#include "Interface.h"

using namespace GemRB;

EFFImporter::~EFFImporter(void)
{
	if (autoFree) {
		delete str;
	}
}

bool EFFImporter::Open(DataStream* stream, bool autoFree)
{
	if (stream == nullptr) {
		return false;
	}
	if (this->autoFree) {
		delete str;
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if (strncmp(Signature, "EFF V2.0", 8) == 0) {
		version = 20;
	} else {
		version = 1;
	}
	str->Seek(-8, GEM_CURRENT_POS);
	return true;
}

//level resistance is checked when DiceSides or DiceThrown
//are greater than 0 (sometimes they used -1 for our amusement)
//if level>than maximum affected or level<than minimum affected, then the
//effect is resisted
// copy the info into the EFFV2 fields (separate), so it is clearer
static inline void fixAffectedLevels(Effect* fx)
{
	if (fx->DiceSides > 0 || fx->DiceThrown > 0) {
		//cloudkill needs these in this order
		fx->MinAffectedLevel = fx->DiceSides;
		fx->MaxAffectedLevel = fx->DiceThrown;
	}
}

// tobex and ees use the same bit differently. Resolve the conflict here, so later use can be cleaner
static ieDword FixSaveFlags(const Effect* fx, bool fix = true)
{
	if (!(fx->SavingThrowType & (SF_IGNORE_DIFFICULTY | SF_LIMIT_EFFECT_STACKING))) return 0;
	if (core->HasFeature(GFFlags::HAS_EE_EFFECTS)) return 0;

	if (fix) {
		return (fx->SavingThrowType & ~SF_IGNORE_DIFFICULTY) | SF_LIMIT_EFFECT_STACKING;
	} else {
		return (fx->SavingThrowType & ~SF_LIMIT_EFFECT_STACKING) | SF_IGNORE_DIFFICULTY;
	}
}

std::unique_ptr<Effect> EFFImporter::GetEffect()
{
	if (version == 1) {
		return GetEffectV1();
	} else {
		// Skip over Signature1
		str->Seek(8, GEM_CURRENT_POS);
		return GetEffectV20();
	}
}

std::unique_ptr<Effect> EFFImporter::GetEffectV1()
{
	ieByte tmpByte;
	ieWord tmpWord;

	auto fx = std::make_unique<Effect>();

	str->ReadWord(tmpWord);
	fx->Opcode = tmpWord;
	str->Read(&tmpByte, 1);
	fx->Target = tmpByte;
	str->Read(&tmpByte, 1);
	fx->Power = tmpByte;
	str->ReadDword(fx->Parameter1);
	str->ReadDword(fx->Parameter2);
	str->Read(&tmpByte, 1);
	fx->TimingMode = tmpByte;
	str->Read(&tmpByte, 1);
	fx->Resistance = tmpByte;
	str->ReadDword(fx->Duration);
	str->Read(&tmpByte, 1);
	fx->ProbabilityRangeMax = tmpByte;
	str->Read(&tmpByte, 1);
	fx->ProbabilityRangeMin = tmpByte;
	str->ReadResRef(fx->Resource);
	str->ReadDword(fx->DiceThrown);
	str->ReadDword(fx->DiceSides);
	str->ReadDword(fx->SavingThrowType);
	ieDword flags = FixSaveFlags(fx.get());
	if (flags) fx->SavingThrowType = flags;
	str->ReadDword(fx->SavingThrowBonus);
	str->ReadWord(fx->IsVariable);
	str->ReadWord(fx->IsSaveForHalfDamage);
	fixAffectedLevels(fx.get());

	fx->Pos.Invalidate();
	fx->Source.Invalidate();
	return fx;
}

std::unique_ptr<Effect> EFFImporter::GetEffectV20()
{
	ieDword tmp;
	auto fx = std::make_unique<Effect>();

	str->Seek(8, GEM_CURRENT_POS);
	str->ReadDword(fx->Opcode);
	str->ReadDword(fx->Target);
	str->ReadDword(fx->Power);
	str->ReadDword(fx->Parameter1);
	str->ReadDword(fx->Parameter2);
	str->ReadWord(fx->TimingMode);
	str->ReadWord(fx->unknown2); // part of a dword TimingMode (but only true for v2 effects)
	str->ReadDword(fx->Duration);
	str->ReadWord(fx->ProbabilityRangeMax);
	str->ReadWord(fx->ProbabilityRangeMin);
	str->ReadResRef(fx->Resource);
	str->ReadDword(fx->DiceThrown);
	str->ReadDword(fx->DiceSides);
	str->ReadDword(fx->SavingThrowType);
	ieDword flags = FixSaveFlags(fx.get());
	if (flags) fx->SavingThrowType = flags;
	str->ReadDword(fx->SavingThrowBonus);
	str->ReadWord(fx->IsVariable); //if this field was set to 1, this is a variable
	str->ReadWord(fx->IsSaveForHalfDamage); //if this field was set to 1, save for half damage; part of Special dword with the preceding field
	str->ReadDword(fx->PrimaryType);
	str->ReadDword(fx->JeremyIsAnIdiot);
	str->ReadDword(fx->MinAffectedLevel);
	str->ReadDword(fx->MaxAffectedLevel);
	str->ReadDword(fx->Resistance);
	str->ReadDword(fx->Parameter3);
	str->ReadDword(fx->Parameter4);
	str->ReadDword(fx->Parameter5);
	str->ReadDword(fx->Parameter6); // IESDP: gametime in ticks (at first application time)
	str->ReadResRef(fx->Resource2);
	str->ReadResRef(fx->Resource3);
	str->ReadDword(tmp);
	fx->Source.x = tmp;
	str->ReadDword(tmp);
	fx->Source.y = tmp;
	str->ReadDword(tmp);
	fx->Pos.x = tmp;
	str->ReadDword(tmp);
	fx->Pos.y = tmp;
	str->ReadDword(fx->SourceType);
	str->ReadResRef(fx->SourceRef);
	str->ReadDword(fx->SourceFlags);
	str->ReadDword(fx->Projectile);
	str->ReadDword(tmp);
	fx->InventorySlot = (ieDwordSigned) (tmp);
	//Variable simply overwrites the resource fields (Keep them grouped)
	//They have to be continuous
	if (fx->IsVariable || fx->Opcode == 187) { // FAKE_VARIABLE_OPCODE
		str->ReadVariable(fx->VariableName);
	} else {
		str->Seek(32, GEM_CURRENT_POS);
	}
	str->ReadDword(fx->CasterLevel);
	// preserve FirstApply for testing purposes (we have EFFECT_REINIT_ON_LOAD)
	// Parameter5 is only used by a few effects so far, not present in our test saves
	if (fx->Parameter5 == 0 && core->config.UseAsLibrary) {
		str->ReadDword(fx->Parameter5);
	} else {
		str->Seek(4, GEM_CURRENT_POS);
	}
	str->ReadDword(fx->SecondaryType);
	str->Seek(60, GEM_CURRENT_POS); // padding

	// fix some too high opcode numbers present in versions before 0.9.5
	if (fx->Opcode >= 396) {
		if (fx->Opcode == 458) fx->Opcode = 0x24;
		if (fx->Opcode == 459) fx->Opcode = 0x25;
		if (fx->Opcode == 511) fx->Opcode = 0xc0;
		if (fx->Opcode == 396) fx->Opcode = 0x76;
		if (fx->Opcode == 398) fx->Opcode = 0x79;
	}

	return fx;
}

void EFFImporter::PutEffectV2(DataStream* stream, const Effect* fx)
{
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		stream->Write("EFF V2.0", 8);
	} else {
		stream->WriteFilling(8);
	}
	// iwd2 test save has actors with local var fx attached without fx->IsVariable set
	bool isVariable = fx->IsVariable || fx->Opcode == 187; // FAKE_VARIABLE_OPCODE
	stream->WriteDword(fx->Opcode);
	stream->WriteDword(fx->Target);
	stream->WriteDword(fx->Power);
	stream->WriteDword(fx->Parameter1);
	stream->WriteDword(fx->Parameter2);
	stream->WriteWord(fx->TimingMode);
	stream->WriteWord(fx->unknown2);
	stream->WriteDword(fx->Duration);
	stream->WriteWord(fx->ProbabilityRangeMax);
	stream->WriteWord(fx->ProbabilityRangeMin);
	if (isVariable) {
		stream->WriteFilling(8);
	} else {
		stream->WriteResRef(fx->Resource);
	}
	stream->WriteDword(fx->DiceThrown);
	stream->WriteDword(fx->DiceSides);
	ieDword flags = FixSaveFlags(fx, false);
	if (flags == 0) flags = fx->SavingThrowType;
	stream->WriteDword(flags);
	stream->WriteDword(fx->SavingThrowBonus);
	stream->WriteWord(fx->IsVariable);
	stream->WriteFilling(2); // SaveForHalfDamage
	stream->WriteDword(fx->PrimaryType);
	stream->WriteDword(fx->JeremyIsAnIdiot);
	stream->WriteFilling(8); // MinAffectedLevel, MaxAffectedLevel
	stream->WriteDword(fx->Resistance);
	stream->WriteDword(fx->Parameter3);
	stream->WriteDword(fx->Parameter4);
	stream->WriteDword(core->config.UseAsLibrary ? 0 : fx->Parameter5);
	stream->WriteDword(fx->Parameter6);
	if (isVariable) {
		stream->WriteFilling(16);
	} else {
		stream->WriteResRef(fx->Resource2);
		stream->WriteResRef(fx->Resource3);
	}
	ieDword tmpDword1 = fx->Source.x;
	ieDword tmpDword2 = fx->Source.y;
	stream->WriteDword(tmpDword1);
	stream->WriteDword(tmpDword2);
	tmpDword1 = fx->Pos.x;
	tmpDword2 = fx->Pos.y;
	stream->WriteDword(tmpDword1);
	stream->WriteDword(tmpDword2);
	stream->WriteDword(fx->SourceType);
	stream->WriteResRef(fx->SourceRef);
	stream->WriteDword(fx->SourceFlags);
	stream->WriteDword(fx->Projectile);
	tmpDword1 = (ieDword) fx->InventorySlot;
	stream->WriteDword(tmpDword1);
	if (isVariable) {
		//resource1-4 are used as a continuous memory
		stream->WriteVariableUC(fx->VariableName);
	} else {
		stream->WriteFilling(32);
	}
	stream->WriteDword(fx->CasterLevel);
	// try preserving fx->FirstApply for save testing
	if (core->config.UseAsLibrary) {
		stream->WriteDword(fx->Parameter5);
	} else {
		stream->WriteFilling(4);
	}
	stream->WriteDword(fx->SecondaryType);
	stream->WriteFilling(60);
}

#include "plugindef.h"

GEMRB_PLUGIN(0x14E81128, "EFF File Importer")
PLUGIN_CLASS(IE_EFF_CLASS_ID, EFFImporter)
END_PLUGIN()
