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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/SPLImporter/SPLImp.cpp,v 1.8 2004/12/17 23:23:08 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/AnimationMgr.h"
#include "SPLImp.h"

SPLImp::SPLImp(void)
{
	str = NULL;
	autoFree = false;
}

SPLImp::~SPLImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool SPLImp::Open(DataStream* stream, bool autoFree)
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
	if (strncmp( Signature, "SPL V1  ", 8 ) == 0) {
		version = 1;
	} else if (strncmp( Signature, "SPL V2.0", 8 ) == 0) {
		version = 20;
	} else {
		printf( "[SPLImporter]: This file is not a valid SPL File\n" );
		return false;
	}

	return true;
}

Spell* SPLImp::GetSpell()
{
	unsigned int i;
	Spell* s = new Spell();

	str->Read( &s->SpellName, 4 );
	str->Read( &s->SpellNameIdentified, 4 );
	str->Read( s->CompletionSound, 8 );
	str->Read( &s->Flags, 4 );
	str->Read( &s->SpellType, 2 );
	str->Read( &s->ExclusionSchool, 2 );
	str->Read( &s->PriestType, 2 );
	str->Read( &s->CastingGraphics, 2 );
	str->Read( &s->unknown1, 1 );
	str->Read( &s->PrimaryType, 2 );
	str->Read( &s->SecondaryType, 1 );
	str->Read( &s->unknown2, 4 );
	str->Read( &s->unknown3, 4 );
	str->Read( &s->unknown4, 4 );
	str->Read( &s->SpellLevel, 4 );
	str->Read( &s->unknown5, 2 );
	str->Read( &s->SpellbookIcon, 8 );
	str->Read( &s->unknown6, 2 );
	str->Read( &s->unknown7, 4 );
	str->Read( &s->unknown8, 4 );
	str->Read( &s->unknown9, 4 );
	str->Read( &s->SpellDesc, 4 );
	str->Read( &s->SpellDescIdentified, 4 );
	str->Read( &s->unknown10, 4 );
	str->Read( &s->unknown11, 4 );
	str->Read( &s->unknown12, 4 );
	str->Read( &s->ExtHeaderOffset, 4 );
	str->Read( &s->ExtHeaderCount, 2 );
	str->Read( &s->FeatureBlockOffset, 4 );
	str->Read( &s->CastingFeatureOffset, 2 );
	str->Read( &s->CastingFeatureCount, 2 );

	memset( s->unknown13, 0, 16 );
	if (version == 20) {
		str->Read( s->unknown13, 16 );
	}


	for (i = 0; i < s->ExtHeaderCount; i++) {
		str->Seek( s->ExtHeaderOffset + i * 40, GEM_STREAM_START );
		SPLExtHeader* eh = GetExtHeader( s );
		s->ext_headers.push_back( eh );
	}

	str->Seek( s->FeatureBlockOffset + 48*s->CastingFeatureOffset,
			GEM_STREAM_START );
	for (i = 0; i < s->CastingFeatureCount; i++) {
		SPLFeature* f = GetFeature();
		s->casting_features.push_back( f );
	}

	DataStream* bamfile = core->GetResourceMgr()->GetResource( s->SpellbookIcon,
													IE_BAM_CLASS_ID );
	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		printf( "[SPLImporter]: No BAM Importer Available.\n" );
		return NULL;
	}
	AnimationMgr* bam = ( AnimationMgr* )
		core->GetInterface( IE_BAM_CLASS_ID );
	bam->Open( bamfile );

	s->SpellIconBAM = bam;


	return s;
}

SPLExtHeader* SPLImp::GetExtHeader(Spell* s)
{
	SPLExtHeader* eh = new SPLExtHeader();

	str->Read( &eh->SpellForm, 1 );
	str->Read( &eh->unknown1, 1 );
	str->Read( &eh->Location, 1 );
	str->Read( &eh->unknown2, 1 );
	str->Read( eh->MemorisedIcon, 8 );
	str->Read( &eh->Target, 1 );
	str->Read( &eh->TargetNumber, 1 );
	str->Read( &eh->Range, 2 );
	str->Read( &eh->RequiredLevel, 2 );
	str->Read( &eh->CastingTime, 4 );
	str->Read( &eh->DiceSides, 2 );
	str->Read( &eh->DiceThrown, 2 );
	str->Read( &eh->Enchanted, 2 );
	str->Read( &eh->FeatureCount, 2 );
	str->Read( &eh->FeatureOffset, 2 );
	str->Read( &eh->Charges, 2 );
	str->Read( &eh->ChargeDepletion, 2 );
	str->Read( &eh->Projectile, 2 );

	str->Seek( s->FeatureBlockOffset + 48*eh->FeatureOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < eh->FeatureCount; i++) {
		SPLFeature* f = GetFeature();
		eh->features.push_back( f );
	}

	return eh;
}

SPLFeature* SPLImp::GetFeature()
{
	SPLFeature* f = new SPLFeature();

	str->Read( &f->Opcode, 2 );
	str->Read( &f->Target, 1 );
	str->Read( &f->Power, 1 );
	str->Read( &f->Parameter1, 4 );
	str->Read( &f->Parameter2, 4 );
	str->Read( &f->TimingMode, 1 );
	str->Read( &f->Resistance, 1 );
	str->Read( &f->Duration, 4 );
	str->Read( &f->Probability1, 1 );
	str->Read( &f->Probability2, 1 );
	str->Read( f->Resource, 8 );
	str->Read( &f->DiceThrown, 4 );
	str->Read( &f->DiceSides, 4 );
	str->Read( &f->SavingThrowType, 4 );
	str->Read( &f->SavingThrowBonus, 4 );
	str->Read( &f->unknown, 4 );

	return f;
}
