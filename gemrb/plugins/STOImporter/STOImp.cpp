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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/STOImporter/STOImp.cpp,v 1.8 2005/03/06 14:18:31 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/AnimationMgr.h"
#include "STOImp.h"

STOImp::STOImp(void)
{
	str = NULL;
	autoFree = false;
}

STOImp::~STOImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool STOImp::Open(DataStream* stream, bool autoFree)
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
	if (strncmp( Signature, "STORV1.0", 8 ) == 0) {
		version = 10;
	} else if (strncmp( Signature, "STORV1.1", 8 ) == 0) {
		version = 11;
	} else if (strncmp( Signature, "STORV9.0", 8 ) == 0) {
		version = 90;
	} else {
		printf( "[STOImporter]: This file is not a valid STO File\n" );
		return false;
	}

	return true;
}

Store* STOImp::GetStore()
{
	unsigned int i;
	Store* s = new Store();

	str->ReadDword( &s->Type );
	str->ReadDword( &s->StoreName );
	str->ReadDword( &s->Flags );
	str->ReadDword( &s->SellMarkup );
	str->ReadDword( &s->BuyMarkup );
	str->ReadDword( &s->DepreciationRate );
	str->ReadWord( &s->StealFailureChance );
	str->ReadWord( &s->Capacity );
	str->Read( s->unknown, 8 );
	str->ReadDword( &s->PurchasedCategoriesOffset );
	str->ReadDword( &s->PurchasedCategoriesCount );
	str->ReadDword( &s->ItemsOffset );
	str->ReadDword( &s->ItemsCount );
	str->ReadDword( &s->Lore );
	str->ReadDword( &s->IDPrice );
	str->ReadResRef( s->RumoursTavern );
	str->ReadDword( &s->DrinksOffset );
	str->ReadDword( &s->DrinksCount );
	str->ReadResRef( s->RumoursTemple );
	str->ReadDword( &s->AvailableRooms );
	for (i = 0; i < 4; i++) {
		str->ReadDword( &s->RoomPrices[i] );
	}
	str->ReadDword( &s->CuresOffset );
	str->ReadDword( &s->CuresCount );
	str->Read( s->unknown2, 36 );

	memset( s->unknown3, 0, 80 );
	if (version == 90) {
		str->Read( s->unknown3, 80 );
	}

	str->Seek( s->PurchasedCategoriesOffset, GEM_STREAM_START );
	s->purchased_categories = GetPurchasedCategories( s );

	str->Seek( s->ItemsOffset, GEM_STREAM_START );
	for (i = 0; i < s->ItemsCount; i++) {
		STOItem* it = GetItem();
		s->items.push_back( it );
	}

	str->Seek( s->DrinksOffset, GEM_STREAM_START );
	for (i = 0; i < s->DrinksCount; i++) {
		STODrink* dr = GetDrink();
		s->drinks.push_back( dr );
	}

	str->Seek( s->CuresOffset, GEM_STREAM_START );
	for (i = 0; i < s->CuresCount; i++) {
		STOCure* cu = GetCure();
		s->cures.push_back( cu );
	}

	return s;
}

STOItem* STOImp::GetItem()
{
	STOItem* it = new STOItem();

	str->ReadResRef( it->ItemResRef );
	str->ReadWord( &it->unknown );
	for(int i=0;i<3;i++) {
		str->ReadWord( it->Usages+i );
	}
	str->ReadDword( &it->Flags );
	str->ReadDword( &it->AmountInStock );
	str->ReadDword( &it->InfiniteSupply );
	if (version == 11) {
		str->ReadDword( &it->Trigger );
		str->Read( it->unknown2, 56 );
	} else {
		it->Trigger = 0;
		memset( it->unknown2, 0, 56 );
	}

	return it;
}

STODrink* STOImp::GetDrink()
{
	STODrink* dr = new STODrink();

	str->ReadResRef( dr->RumourResRef );
	str->ReadDword( &dr->DrinkName );
	str->ReadDword( &dr->Price );
	str->ReadDword( &dr->Strength );

	return dr;
}

STOCure* STOImp::GetCure()
{
	STOCure* cu = new STOCure();

	str->ReadResRef( cu->CureResRef );
	str->ReadDword( &cu->Price );

	return cu;
}

ieDword* STOImp::GetPurchasedCategories(Store* s)
{
	int size = s->PurchasedCategoriesCount * sizeof( ieDword );
	ieDword* pc = ( ieDword* ) malloc( size );

	for (unsigned int i = 0; i < s->PurchasedCategoriesCount; i++) {
		str->ReadDword( &pc[i] );
	}

	return pc;
}

