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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ITMImporter/ITMImp.cpp,v 1.1 2004/02/15 14:26:55 edheldil Exp $
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
	if(str && autoFree)
		delete(str);
}

bool ITMImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "ITM V1.0", 8) == 0) {
	        version = 10;
	} 
	else if(strncmp(Signature, "ITM V1.1", 8) == 0) {
	        version = 11;
	}
	else {
	        printf("[ITMImporter]: This file is not a valid ITM File (Signature: %s)\n", Signature);
		return false;
	}

	return true;
}

Item * ITMImp::GetItem ()
{
        Item * s = new Item ();

	str->Read(&s->ItemName, 4);
	str->Read(&s->ItemNameIdentified, 4);
	str->Read(s->ReplacementItem, 8);
	str->Read(&s->Flags, 4);
	str->Read(&s->ItemType, 2);
	str->Read(&s->UsabilityBitmask, 4);
	str->Read(s->InventoryIconType, 2);
	str->Read(&s->MinLevel, 2);
	str->Read(&s->MinStrength, 2);
	str->Read(&s->MinStrengthBonus, 2);
	str->Read(&s->MinIntelligence, 2);
	str->Read(&s->MinDexterity, 2);
	str->Read(&s->MinWisdom, 2);
	str->Read(&s->MinConstitution, 2);
	str->Read(&s->MinCharisma, 2);
	str->Read(&s->Price, 4);
	str->Read(&s->StackAmount, 2);
	str->Read(s->ItemIcon, 8);
	str->Read(&s->LoreToID, 2);
	str->Read(s->GroundIcon, 8);
	str->Read(&s->Weight, 4);
	str->Read(&s->ItemDesc, 4);
	str->Read(&s->ItemDescIdentified, 4);
	str->Read(s->CarriedIcon, 8);
	str->Read(&s->Enchantment, 4);
	str->Read(&s->ExtHeaderOffset, 4);
	str->Read(&s->ExtHeaderCount, 2);
	str->Read(&s->FeatureBlockOffset, 4);
	str->Read(&s->EquippingFeatureOffset, 2);
	str->Read(&s->EquippingFeatureCount, 2);

	if (version == 11) {
		for (int i = 0; i < 7; i++) {
		        str->Read(&s->unknown[i], 4);
		}
	}


	for (unsigned int i = 0; i < s->ExtHeaderCount; i++) {
	        str->Seek (s->ExtHeaderOffset + i * 56, GEM_STREAM_START);
	        ITMExtHeader *eh = GetExtHeader (s);
		s->ext_headers.push_back (eh);
	}

	str->Seek (s->FeatureBlockOffset + s->EquippingFeatureOffset, GEM_STREAM_START);
    	for (unsigned int i = 0; i < s->EquippingFeatureCount; i++) {
	        ITMFeature *f = GetFeature ();
		s->equipping_features.push_back (f);
	}


	if(!core->IsAvailable(IE_BAM_CLASS_ID)) {
		printf("[ITMImporter]: No BAM Importer Available.\n");
		return NULL;
	}

	DataStream *bamfile;
	AnimationMgr * bam;

	bamfile = core->GetResourceMgr()->GetResource(s->ItemIcon, IE_BAM_CLASS_ID);
	bam = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	bam->Open(bamfile);
	s->ItemIconBAM = bam;

	bamfile = core->GetResourceMgr()->GetResource(s->GroundIcon, IE_BAM_CLASS_ID);
	bam = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	bam->Open(bamfile);
	s->GroundIconBAM = bam;

	bamfile = core->GetResourceMgr()->GetResource(s->CarriedIcon, IE_BAM_CLASS_ID);
	bam = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	bam->Open(bamfile);
	s->CarriedIconBAM = bam;


	return s;
}

ITMExtHeader * ITMImp::GetExtHeader (Item *s)
{
        ITMExtHeader * eh = new ITMExtHeader ();

        str->Read(&eh->AttackType, 1);
        str->Read(&eh->IDReq, 1);
        str->Read(&eh->Location, 1);
        str->Read(&eh->unknown1, 1);
        str->Read(eh->UseIcon, 8);
        str->Read(&eh->Target, 1);
        str->Read(&eh->TargetNumber, 1);
        str->Read(&eh->Range, 2);
        str->Read(&eh->ProjectileType, 2);
        str->Read(&eh->Speed, 2);
        str->Read(&eh->THAC0Bonus, 2);
        str->Read(&eh->DiceSides, 2);
        str->Read(&eh->DiceThrown, 2);
        str->Read(&eh->DamageBonus, 2);
        str->Read(&eh->DamageType, 2);
        str->Read(&eh->FeatureCount, 2);
        str->Read(&eh->FeatureOffset, 2);
        str->Read(&eh->Charges, 2);
        str->Read(&eh->ChargeDepletion, 2);
        str->Read(&eh->UseStrengthBonus, 1);
        str->Read(&eh->Recharge, 1);
        str->Read(&eh->unknown2, 2);
        str->Read(&eh->ProjectileAnimation, 2);
	for (int i; i < 3; i++) {
	        str->Read(&eh->MeleeAnimation[i], 2);
	}
        str->Read(&eh->BowArrowQualifier, 2);
        str->Read(&eh->CrossbowBoltQualifier, 2);
        str->Read(&eh->MiscProjectileQualifier, 2);


	str->Seek(s->FeatureBlockOffset + eh->FeatureOffset, GEM_STREAM_START);
    	for (unsigned int i = 0; i < eh->FeatureCount; i++) {
	        ITMFeature *f = GetFeature ();
		eh->features.push_back (f);
	}
	

	return eh;
}

ITMFeature * ITMImp::GetFeature ()
{
        ITMFeature * f = new ITMFeature ();

        str->Read(&f->Opcode, 2);
        str->Read(&f->Target, 1);
        str->Read(&f->Power, 1);
        str->Read(&f->Parameter1, 4);
        str->Read(&f->Parameter2, 4);
        str->Read(&f->TimingMode, 1);
        str->Read(&f->Resistance, 1);
        str->Read(&f->Duration, 4);
        str->Read(&f->Probability1, 1);
        str->Read(&f->Probability2, 1);
        str->Read(f->Resource, 8);
        str->Read(&f->DiceThrown, 4);
        str->Read(&f->DiceSides, 2);
        str->Read(&f->SavingThrowType, 4);
        str->Read(&f->SavingThrowBonus, 4);
        str->Read(&f->unknown, 4);

	return f;
}
