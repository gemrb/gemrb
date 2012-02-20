/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005 The GemRB Project
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

#include "EFFImporter.h"

#include "win32def.h"

#include "Interface.h"

using namespace GemRB;

EFFImporter::EFFImporter(void)
{
	str = NULL;
	autoFree = false;
}

EFFImporter::~EFFImporter(void)
{
	if (autoFree) {
		delete str;
	}
}

bool EFFImporter::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (this->autoFree) {
		delete str;
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "EFF V2.0", 8 ) == 0) {
		version = 20;
	} else {
		version = 1;
	}
	str->Seek( -8, GEM_CURRENT_POS );
	return true;
}

//level resistance is checked when DiceSides or DiceThrown
//are greater than 0 (sometimes they used -1 for our amusement)
//if level>than maximum affected or level<than minimum affected, then the
//effect is resisted
// copy the info into the EFFV2 fields (separate), so it is clearer
static inline void fixAffectedLevels(Effect *fx) {
	if (fx->DiceSides > 0 || fx->DiceThrown > 0) {
		fx->MinAffectedLevel = fx->DiceThrown;
		fx->MaxAffectedLevel = fx->DiceSides;
	}
}

Effect* EFFImporter::GetEffect(Effect *fx)
{
	if (version == 1) {
		return GetEffectV1( fx );
	}
	else {
		// Skip over Signature1
		str->Seek( 8, GEM_CURRENT_POS );
		return GetEffectV20( fx );
	}
}

Effect* EFFImporter::GetEffectV1(Effect *fx)
{
	ieByte tmpByte;
	ieWord tmpWord;

	memset( fx, 0, sizeof( Effect ) );

	str->ReadWord( &tmpWord );
	fx->Opcode = tmpWord;
	str->Read( &tmpByte, 1 );
	fx->Target = tmpByte;
	str->Read( &tmpByte, 1 );
	fx->Power = tmpByte;
	str->ReadDword( &fx->Parameter1 );
	str->ReadDword( &fx->Parameter2 );
	str->Read( &tmpByte, 1 );
	fx->TimingMode = tmpByte;
	str->Read( &tmpByte, 1 );
	fx->Resistance = tmpByte;
	str->ReadDword( &fx->Duration );
	str->Read( &tmpByte, 1 );
	fx->Probability1 = tmpByte;
	str->Read( &tmpByte, 1 );
	fx->Probability2 = tmpByte;
	str->ReadResRef( fx->Resource );
	str->ReadDword( &fx->DiceThrown );
	str->ReadDword( &fx->DiceSides );
	str->ReadDword( &fx->SavingThrowType );
	str->ReadDword( &fx->SavingThrowBonus );
	str->ReadWord( &fx->IsVariable );
	str->ReadWord( &fx->IsSaveForHalfDamage );
	fixAffectedLevels( fx );

	fx->PosX=0xffffffff;
	fx->PosY=0xffffffff;
	return fx;
}

Effect* EFFImporter::GetEffectV20(Effect *fx)
{
	ieDword tmp;
	memset( fx, 0, sizeof( Effect ) );

	str->Seek(8, GEM_CURRENT_POS);
	str->ReadDword( &fx->Opcode );
	str->ReadDword( &fx->Target );
	str->ReadDword( &fx->Power );
	str->ReadDword( &fx->Parameter1 );
	str->ReadDword( &fx->Parameter2 );
	str->ReadWord( &fx->TimingMode );
	str->ReadWord( &fx->unknown2 );
	str->ReadDword( &fx->Duration );
	str->ReadWord( &fx->Probability1 );
	str->ReadWord( &fx->Probability2 );
	str->ReadResRef( fx->Resource );
	str->ReadDword( &fx->DiceThrown );
	str->ReadDword( &fx->DiceSides );
	str->ReadDword( &fx->SavingThrowType );
	str->ReadDword( &fx->SavingThrowBonus );
	str->ReadWord( &fx->IsVariable ); //if this field was set to 1, this is a variable
	str->ReadWord( &fx->IsSaveForHalfDamage ); //if this field was set to 1, save for half damage
	str->ReadDword( &fx->PrimaryType );
	str->Seek( 4, GEM_CURRENT_POS );
	str->ReadDword( &fx->MinAffectedLevel );
	str->ReadDword( &fx->MaxAffectedLevel );
	str->ReadDword( &fx->Resistance );
	str->ReadDword( &fx->Parameter3 );
	str->ReadDword( &fx->Parameter4 );
	str->ReadDword( &fx->Parameter5 );
	str->ReadDword( &fx->Parameter6 );
	str->ReadResRef( fx->Resource2 );
	str->ReadResRef( fx->Resource3 );	
	str->ReadDword( &fx->PosX);
	str->ReadDword( &fx->PosY);
	//FIXME: these two points are actually different
	str->ReadDword( &fx->PosX);
	str->ReadDword( &fx->PosY);
	str->ReadDword( &fx->SourceType );
	str->ReadResRef( fx->Source );
	str->ReadDword( &fx->SourceFlags );
	str->ReadDword( &fx->Projectile );
	str->ReadDword( &tmp );
	fx->InventorySlot=(ieDwordSigned) (tmp);
	//Variable simply overwrites the resource fields (Keep them grouped)
	//They have to be continuous
	if (fx->IsVariable) {
		str->Read( fx->Resource, 32 );
		strnlwrcpy( fx->Resource, fx->Resource, 32 );
	} else {
		str->Seek( 32, GEM_CURRENT_POS);
	}
	str->ReadDword( &fx->CasterLevel );
	str->Seek( 4, GEM_CURRENT_POS );
	str->ReadDword( &fx->SecondaryType );
	str->Seek( 60, GEM_CURRENT_POS );

	return fx;
}

void EFFImporter::PutEffectV2(DataStream *stream, const Effect *fx) {
	ieDword tmpDword1,tmpDword2;
	char filling[60];

	memset(filling,0,sizeof(filling) );

	stream->Write( filling,8 ); //signature
	stream->WriteDword( &fx->Opcode);
	stream->WriteDword( &fx->Target);
	stream->WriteDword( &fx->Power);
	stream->WriteDword( &fx->Parameter1);
	stream->WriteDword( &fx->Parameter2);
	stream->WriteWord( &fx->TimingMode);
	stream->WriteWord( &fx->unknown2);
	stream->WriteDword( &fx->Duration);
	stream->WriteWord( &fx->Probability1);
	stream->WriteWord( &fx->Probability2);
	if (fx->IsVariable) {
		stream->Write( filling,8 );
	} else {
		stream->WriteResRef(fx->Resource);
	}
	stream->WriteDword( &fx->DiceThrown );
	stream->WriteDword( &fx->DiceSides );
	stream->WriteDword( &fx->SavingThrowType );
	stream->WriteDword( &fx->SavingThrowBonus );
	stream->WriteWord( &fx->IsVariable );
	stream->Write( filling,2 ); // SaveForHalfDamage
	stream->WriteDword( &fx->PrimaryType );
	stream->Write( filling,12 ); // MinAffectedLevel, MaxAffectedLevel, Resistance
	stream->WriteDword( &fx->Resistance );
	stream->WriteDword( &fx->Parameter3 );
	stream->WriteDword( &fx->Parameter4 );
	stream->WriteDword( &fx->Parameter5 );
	stream->WriteDword( &fx->Parameter6 );
	if (fx->IsVariable) {
		stream->Write( filling,16 );
	} else {
		stream->WriteResRef(fx->Resource2);
		stream->WriteResRef(fx->Resource3);
	}
	tmpDword1 = (ieDword) fx->PosX;
	tmpDword2 = (ieDword) fx->PosY;
	stream->WriteDword( &tmpDword1 );
	stream->WriteDword( &tmpDword2 );
	//FIXME: these two points are actually different
	stream->WriteDword( &tmpDword1 );
	stream->WriteDword( &tmpDword2 );
	stream->WriteDword( &fx->SourceType );
	stream->WriteResRef( fx->Source );
	stream->WriteDword( &fx->SourceFlags );
	stream->WriteDword( &fx->Projectile );
	tmpDword1 = (ieDword) fx->InventorySlot;
	stream->WriteDword( &tmpDword1 );
	if (fx->IsVariable) {
		//resource1-4 are used as a continuous memory
		stream->Write(fx->Resource, 32);
	} else {
		stream->Write( filling,32 );
	}
	stream->WriteDword( &fx->CasterLevel);
	stream->Write( filling,4);
	stream->WriteDword( &fx->SecondaryType );
	stream->Write( filling,60 );
}

#include "plugindef.h"

GEMRB_PLUGIN(0x14E81128, "EFF File Importer")
PLUGIN_CLASS(IE_EFF_CLASS_ID, EFFImporter)
END_PLUGIN()
