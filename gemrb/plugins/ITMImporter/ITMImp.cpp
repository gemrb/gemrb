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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ITMImporter/ITMImp.cpp,v 1.11 2004/11/21 22:58:12 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/AnimationMgr.h"
#include "ITMImp.h"

ITMImp::ITMImp(void)
{
	str = NULL;
	autoFree = false;
}

ITMImp::~ITMImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
	str = NULL;
}

bool ITMImp::Open(DataStream* stream, bool autoFree)
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
	if (strncmp( Signature, "ITM V1  ", 8 ) == 0) {
		version = 10;
	} else if (strncmp( Signature, "ITM V1.1", 8 ) == 0) {
		version = 11;
	} else if (strncmp( Signature, "ITM V2.0", 8 ) == 0) {
		version = 20;
	} else {
		printf( "[ITMImporter]: This file is not a valid ITM File\n" );
		return false;
	}

	return true;
}

Item* ITMImp::GetItem()
{
	unsigned int i;
	Item* s = new Item();
	ieByte k1,k2,k3,k4;

	str->ReadDword( &s->ItemName );
	str->ReadDword( &s->ItemNameIdentified );
	str->ReadResRef( s->ReplacementItem );
	str->ReadDword( &s->Flags );
	str->ReadWord( &s->ItemType );
	str->ReadDword( &s->UsabilityBitmask );
	str->Read( s->InventoryIconType,2 );
	str->ReadWord( &s->MinLevel );
	str->Read( &s->MinStrength,1 );
	str->Read( &s->unknown2,1 );
	str->Read( &s->MinStrengthBonus,1 );
	str->Read( &k1,1 );
	str->Read( &s->MinIntelligence,1 );
	str->Read( &k2,1 );
	str->Read( &s->MinDexterity,1 );
	str->Read( &k3,1 );
	str->Read( &s->MinWisdom,1 );
	str->Read( &k4,1 );
	s->KitUsability=(k1<<24) | (k2<<16) | (k3<<8) | k1; //bg2/iwd2 specific
	str->Read( &s->MinConstitution,1 );
	str->Read( &s->WeaProf,1 ); //bg2 specific
	str->Read( &s->MinCharisma,1 );
	str->Read( &s->unknown3,1 );
	str->ReadDword( &s->Price );
	str->ReadWord( &s->StackAmount );
	str->ReadResRef( s->ItemIcon );
	str->ReadWord( &s->LoreToID );
	str->ReadResRef( s->GroundIcon );
	str->ReadDword( &s->Weight );
	str->ReadDword( &s->ItemDesc );
	str->ReadDword( &s->ItemDescIdentified );
	str->ReadResRef( s->CarriedIcon );
	str->ReadDword( &s->Enchantment );
	str->ReadDword( &s->ExtHeaderOffset );
	str->ReadWord( &s->ExtHeaderCount );
	str->ReadDword( &s->FeatureBlockOffset );
	str->ReadWord( &s->EquippingFeatureOffset );
	str->ReadWord( &s->EquippingFeatureCount );

	s->Dialog[0] = 0;
	s->DialogName = 0;
	s->WieldColor = 0;
	memset( s->unknown, 0, 26 );

	if (version == 20) {
		str->Read( s->unknown, 16 );
	} else if (version == 11) {
		str->ReadResRef( s->Dialog );
		str->ReadDword( &s->DialogName );
		str->ReadWord( &s->WieldColor );
		str->Read( s->unknown, 26 );
	}

	for (i = 0; i < s->ExtHeaderCount; i++) {
		str->Seek( s->ExtHeaderOffset + i * 56, GEM_STREAM_START );
		ITMExtHeader* eh = GetExtHeader( s );
		s->ext_headers.push_back( eh );
	}

	//48 is the size of the feature block
	str->Seek( s->FeatureBlockOffset + 48*s->EquippingFeatureOffset,
			GEM_STREAM_START );
	for (i = 0; i < s->EquippingFeatureCount; i++) {
		ITMFeature* f = GetFeature();
		s->equipping_features.push_back( f );
	}


	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		printf( "[ITMImporter]: No BAM Importer Available.\n" );
		return NULL;
	}

	DataStream* bamfile;
	AnimationMgr* bam;

	if (s->ItemIcon[0]) {
		bamfile = core->GetResourceMgr()->GetResource( s->ItemIcon, IE_BAM_CLASS_ID );
		bam = ( AnimationMgr * ) core->GetInterface( IE_BAM_CLASS_ID );
		bam->Open( bamfile );
		s->ItemIconBAM = bam;
	}

	if (s->GroundIcon[0]) {
		bamfile = core->GetResourceMgr()->GetResource( s->GroundIcon, IE_BAM_CLASS_ID );
		bam = ( AnimationMgr * ) core->GetInterface( IE_BAM_CLASS_ID );
		bam->Open( bamfile );
		s->GroundIconBAM = bam;
	}

	if (s->CarriedIcon[0] && core->HasFeature(GF_HAS_DESC_ICON) ) {
		bamfile = core->GetResourceMgr()->GetResource( s->CarriedIcon, IE_BAM_CLASS_ID );
		bam = ( AnimationMgr * ) core->GetInterface( IE_BAM_CLASS_ID );
		bam->Open( bamfile );
		s->CarriedIconBAM = bam;
	}

	return s;
}

ITMExtHeader* ITMImp::GetExtHeader(Item* s)
{
	unsigned int i;
	ITMExtHeader* eh = new ITMExtHeader();

	str->Read( &eh->AttackType,1 );
	str->Read( &eh->IDReq,1 );
	str->Read( &eh->Location,1 );
	str->Read( &eh->unknown1,1 );
	str->ReadResRef( eh->UseIcon );
	str->Read( &eh->Target,1 );
	str->Read( &eh->TargetNumber,1 );
	str->ReadWord( &eh->Range );
	str->ReadWord( &eh->ProjectileType );
	str->ReadWord( &eh->Speed );
	str->ReadWord( &eh->THAC0Bonus );
	str->ReadWord( &eh->DiceSides );
	str->ReadWord( &eh->DiceThrown );
	str->ReadWord( &eh->DamageBonus );
	str->ReadWord( &eh->DamageType );
	str->ReadWord( &eh->FeatureCount );
	str->ReadWord( &eh->FeatureOffset );
	str->ReadWord( &eh->Charges );
	str->ReadWord( &eh->ChargeDepletion );
	str->ReadDword( &eh->RechargeFlags );
	//str->Read( &eh->UseStrengthBonus,1 );
	//str->Read( &eh->Recharge,1 );
	//str->ReadWord( &eh->unknown2 );
	str->ReadWord( &eh->ProjectileAnimation );
	for (i = 0; i < 3; i++) {
		str->ReadWord( &eh->MeleeAnimation[i] );
	}
	str->ReadWord( &eh->BowArrowQualifier );
	str->ReadWord( &eh->CrossbowBoltQualifier );
	str->ReadWord( &eh->MiscProjectileQualifier );

	//48 is the size of the feature block
	str->Seek( s->FeatureBlockOffset + 48*eh->FeatureOffset, GEM_STREAM_START );
	for (i = 0; i < eh->FeatureCount; i++) {
		ITMFeature* f = GetFeature();
		eh->features.push_back( f );
	}


	return eh;
}

ITMFeature* ITMImp::GetFeature()
{
	ITMFeature* f = new ITMFeature();

	str->ReadWord( &f->Opcode );
	str->Read( &f->Target,1 );
	str->Read( &f->Power,1 );
	str->ReadDword( &f->Parameter1 );
	str->ReadDword( &f->Parameter2 );
	str->Read( &f->TimingMode,1 );
	str->Read( &f->Resistance,1 );
	str->ReadDword( &f->Duration );
	str->Read( &f->Probability1,1 );
	str->Read( &f->Probability2,1 );
	str->ReadResRef( f->Resource );
	str->ReadDword( &f->DiceThrown );
	str->ReadDword( &f->DiceSides );
	str->ReadDword( &f->SavingThrowType );
	str->ReadDword( &f->SavingThrowBonus );
	str->ReadDword( &f->unknown );

	return f;
}
