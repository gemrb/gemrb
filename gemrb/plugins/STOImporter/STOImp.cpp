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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/STOImporter/STOImp.cpp,v 1.3 2004/04/13 23:26:23 doc_wagon Exp $
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
	Store* s = new Store();

	str->Read( &s->Type, 4 );
	str->Read( &s->StoreName, 4 );
	str->Read( &s->Flags, 4 );
	str->Read( &s->SellMarkup, 4 );
	str->Read( &s->BuyMarkup, 4 );
	str->Read( &s->DepreciationRate, 4 );
	str->Read( &s->StealFailureChance, 2 );
	str->Read( &s->Capacity, 2 );
	str->Read( s->unknown, 8 );
	str->Read( &s->PurchasedCategoriesOffset, 4 );
	str->Read( &s->PurchasedCategoriesCount, 4 );
	str->Read( &s->ItemsOffset, 4 );
	str->Read( &s->ItemsCount, 4 );
	str->Read( &s->Lore, 4 );
	str->Read( &s->IDPrice, 4 );
	str->Read( s->RumoursTavern, 8 );
	str->Read( &s->DrinksOffset, 4 );
	str->Read( &s->DrinksCount, 4 );
	str->Read( s->RumoursTemple, 8 );
	str->Read( &s->AvailableRooms, 4 );
	for (int i = 0; i < 4; i++) {
		str->Read( &s->RoomPrices[i], 4 );
	}
	str->Read( &s->CuresOffset, 4 );
	str->Read( &s->CuresCount, 4 );
	str->Read( s->unknown2, 36 );

	memset( s->unknown3, 0, 80 );
	if (version == 90) {
		str->Read( s->unknown3, 80 );
	}


	str->Seek( s->PurchasedCategoriesOffset, GEM_STREAM_START );
	s->purchased_categories = GetPurchasedCategories( s );

	str->Seek( s->ItemsOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < s->ItemsCount; i++) {
		STOItem* it = GetItem();
		s->items.push_back( it );
	}

	str->Seek( s->DrinksOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < s->DrinksCount; i++) {
		STODrink* dr = GetDrink();
		s->drinks.push_back( dr );
	}

	str->Seek( s->CuresOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < s->CuresCount; i++) {
		STOCure* cu = GetCure();
		s->cures.push_back( cu );
	}

	return s;
}

STOItem* STOImp::GetItem()
{
	STOItem* it = new STOItem();

	str->Read( it->ItemResRef, 8 );
	str->Read( &it->unknown, 2 );
	str->Read( &it->Usage1, 2 );
	str->Read( &it->Usage2, 2 );
	str->Read( &it->Usage3, 2 );
	str->Read( &it->Flags, 4 );
	str->Read( &it->AmountInStock, 4 );
	str->Read( &it->InfiniteSupply, 4 );

	memset( it->unknown2, 0, 56 );
	if (version == 11) {
		str->Read( it->unknown2, 56 );
	}

	return it;
}

STODrink* STOImp::GetDrink()
{
	STODrink* dr = new STODrink();

	str->Read( dr->RumourResRef, 8 );
	str->Read( &dr->DrinkName, 4 );
	str->Read( &dr->Price, 4 );
	str->Read( &dr->AlcoholicStrength, 4 );

	return dr;
}

STOCure* STOImp::GetCure()
{
	STOCure* cu = new STOCure();

	str->Read( cu->CureResRef, 8 );
	str->Read( &cu->Price, 4 );

	return cu;
}

ieDword* STOImp::GetPurchasedCategories(Store* s)
{
	int size = s->PurchasedCategoriesCount * sizeof( ieDword );
	ieDword* pc = ( ieDword* ) malloc( size );

	for (unsigned int i = 0; i < s->PurchasedCategoriesCount; i++) {
		str->Read( &pc[i], 4 );
	}

	return pc;
}

