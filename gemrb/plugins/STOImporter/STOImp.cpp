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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/Inventory.h"
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
	} else if (strncmp( Signature, "STORV0.0", 8 ) == 0) {
		//GemRB's internal version with all known fields supported
		version = 0;
	} else {
		printf( "[STOImporter]: This file is not a valid STO File\n" );
		return false;
	}

	return true;
}

Store* STOImp::GetStore(Store *s)
{
	unsigned int i;

	if (!s)
		return NULL;

	// saving in original version requires the original version
	// otherwise it is set to 0 at construction time
	if (core->SaveAsOriginal) {
		s->version = version;
	}

	str->ReadDword( &s->Type );
	str->ReadDword( &s->StoreName );
	str->ReadDword( &s->Flags );
	str->ReadDword( &s->SellMarkup );
	str->ReadDword( &s->BuyMarkup );
	str->ReadDword( &s->DepreciationRate );
	str->ReadWord( &s->StealFailureChance );
	str->ReadWord( &s->Capacity ); //will be overwritten for V9.0
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

	if (version == 90) { //iwd stores
		ieDword tmp;

		str->ReadDword( &tmp );
		s->Capacity = (ieWord) tmp;
		str->Read( s->unknown3, 80 );
	} else {
		memset( s->unknown3, 0, 80 );
	}

	//Allocation must be done in the same place as destruction.
	//Yeah, this is intentionally so ugly, someone who doesn't like this
	//may fix it.
	core->DoTheStoreHack(s);

	str->Seek( s->PurchasedCategoriesOffset, GEM_STREAM_START );
	GetPurchasedCategories( s );

	str->Seek( s->ItemsOffset, GEM_STREAM_START );
	for (i = 0; i < s->ItemsCount; i++) {
	STOItem *item = s->items[i];
		GetItem(item);
		//it is important to handle this field as signed
		if (item->InfiniteSupply>0) {
			//if there are no triggers, GetRealStockSize is simpler
			//also it is compatible only with pst/gemrb saved stores
			s->HasTriggers=true;
		}
	}

	str->Seek( s->DrinksOffset, GEM_STREAM_START );
	for (i = 0; i < s->DrinksCount; i++) {
		GetDrink(s->drinks+i);
	}

	str->Seek( s->CuresOffset, GEM_STREAM_START );
	for (i = 0; i < s->CuresCount; i++) {
		GetCure(s->cures+i);
	}

	return s;
}

void STOImp::GetItem(STOItem *it)
{
	str->ReadResRef( it->ItemResRef );
	str->ReadWord( &it->PurchasedAmount );
	for (int i=0;i<CHARGE_COUNTERS;i++) {
		str->ReadWord( it->Usages+i );
	}
	str->ReadDword( &it->Flags );
	str->ReadDword( &it->AmountInStock );
	//if there was no item on stock, how this could be 0
	//we hack-fix this here so it won't cause trouble
	if (!it->AmountInStock) {
		it->AmountInStock = 1;
	}
	//another hack-fix
	Item *item = gamedata->GetItem( it->ItemResRef );
	if (item) {
		if (!item->LoreToID) {
			it->Flags |= IE_INV_ITEM_IDENTIFIED;
		}
		gamedata->FreeItem( item, it->ItemResRef, false );
	}
	str->ReadDword( (ieDword *) &it->InfiniteSupply );
	ieDwordSigned tmp;

	switch (version) {
		case 11: //pst
			if (it->InfiniteSupply) {
				it->InfiniteSupply=-1;
			}
			str->ReadDword( (ieDword *) &tmp );
			if (tmp>0) {
				it->InfiniteSupply=tmp;
			}
			str->Read( it->unknown2, 56 );
			break;
		case 0: //gemrb version stores trigger ref in infinitesupply
			memset( it->unknown2, 0, 56 );
			break;
		default:
			if (it->InfiniteSupply) {
				it->InfiniteSupply=-1;
			}
			memset( it->unknown2, 0, 56 );
	}
}

void STOImp::GetDrink(STODrink *dr)
{
	str->ReadResRef( dr->RumourResRef );
	str->ReadDword( &dr->DrinkName );
	str->ReadDword( &dr->Price );
	str->ReadDword( &dr->Strength );
}

void STOImp::GetCure(STOCure *cu)
{
	str->ReadResRef( cu->CureResRef );
	str->ReadDword( &cu->Price );
}

void STOImp::GetPurchasedCategories(Store* s)
{
	for (unsigned int i = 0; i < s->PurchasedCategoriesCount; i++) {
		str->ReadDword( &s->purchased_categories[i] );
	}
}

//call this before any write, it updates offsets!
int STOImp::GetStoredFileSize(Store *s)
{
	int headersize, itemsize;

	//header
	switch (s->version) {
		case 90:
			//capacity on a dword and 80 bytes of crap
			headersize = 156 + 84;
			itemsize = 28;
			break;
		case 11:
			headersize = 156;
			//trigger ref on a dword and 56 bytes of crap
			itemsize = 28 + 60;
		default:
			headersize = 156;
			itemsize = 28;
			break;
	}

	//drinks
	s->DrinksOffset = headersize;
	headersize += s->DrinksCount * 20; //8+4+4+4

	//cures
	s->CuresOffset = headersize;
	headersize += s->CuresCount * 12; //8+4

	//purchased items
	s->PurchasedCategoriesOffset = headersize;
	headersize += s->PurchasedCategoriesCount * sizeof(ieDword);

	//items
	s->ItemsOffset = headersize;
	headersize += s->ItemsCount * itemsize;

	return headersize;
}

int STOImp::PutPurchasedCategories(DataStream *stream, Store* s)
{
	for (unsigned int i = 0; i < s->PurchasedCategoriesCount; i++) {
		stream->WriteDword( s->purchased_categories+i );
	}
	return 0;
}

int STOImp::PutHeader(DataStream *stream, Store *s)
{
	char Signature[8];
	ieDword tmpDword;
	ieWord tmpWord;

	version = s->version;
	memcpy( Signature, "STORV0.0", 8);
	Signature[5]+=version/10;
	Signature[7]+=version%10;
	stream->Write( Signature, 8);
	stream->WriteDword( &s->Type);
	stream->WriteDword( &s->StoreName);
	stream->WriteDword( &s->Flags);
	stream->WriteDword( &s->SellMarkup);
	stream->WriteDword( &s->BuyMarkup);
	stream->WriteDword( &s->DepreciationRate);
	stream->WriteWord( &s->StealFailureChance);

	switch (version) {
	case 10: case 0: // bg2, gemrb
		tmpWord = s->Capacity;
		break;
	default:
		tmpWord = 0;
		break;
	}
	stream->WriteWord( &tmpWord);

	stream->Write( s->unknown, 8);
	stream->WriteDword( &s->PurchasedCategoriesOffset);
	stream->WriteDword( &s->PurchasedCategoriesCount);
	stream->WriteDword( &s->ItemsOffset);
	stream->WriteDword( &s->ItemsCount);
	stream->WriteDword( &s->Lore);
	stream->WriteDword( &s->IDPrice);
	stream->WriteResRef( s->RumoursTavern);
	stream->WriteDword( &s->DrinksOffset);
	stream->WriteDword( &s->DrinksCount);
	stream->WriteResRef( s->RumoursTemple);
	stream->WriteDword( &s->AvailableRooms);
	for (int i=0;i<4;i++) {
		stream->WriteDword( s->RoomPrices+i );
	}
	stream->WriteDword( &s->CuresOffset);
	stream->WriteDword( &s->CuresCount);
	stream->Write (s->unknown3, 36);  //use these as padding
	if (version==90) {
		tmpDword = s->Capacity;
		stream->WriteDword( &tmpDword);
		stream->Write( s->unknown3, 80); //writing out original fillers
	}
	return 0;
}

int STOImp::PutItems(DataStream *stream, Store *store)
{
	for (unsigned int ic=0;ic<store->ItemsCount;ic++) {
		STOItem *it = store->items[ic];

		stream->WriteResRef( it->ItemResRef);
		stream->WriteWord( &it->PurchasedAmount);
		for (unsigned int i=0;i<CHARGE_COUNTERS;i++) {
			stream->WriteWord( it->Usages+i );
		}
		stream->WriteDword( &it->Flags );
		stream->WriteDword( &it->AmountInStock );
		if (version==11) {
			stream->WriteDword( (ieDword *) &it->InfiniteSupply);
			stream->WriteDword( (ieDword *) &it->InfiniteSupply);
			stream->Write( it->unknown2, 56);
		} else {
			stream->WriteDword( (ieDword *) &it->InfiniteSupply );
		}
	}
	return 0;
}

int STOImp::PutCures(DataStream *stream, Store *s)
{
	for (unsigned int i=0;i<s->CuresCount;i++) {
		STOCure *c = s->cures+i;
		stream->WriteResRef( c->CureResRef);
		stream->WriteDword( &c->Price);
	}
	return 0;
}

int STOImp::PutDrinks(DataStream *stream, Store *s)
{
	for (unsigned int i=0;i<s->DrinksCount;i++) {
		STODrink *d = s->drinks+i;
		stream->WriteResRef( d->RumourResRef); //?
		stream->WriteDword( &d->DrinkName);
		stream->WriteDword( &d->Price);
		stream->WriteDword( &d->Strength);
	}
	return 0;
}

//saves the store into a datastream, be it memory or file
int STOImp::PutStore(DataStream *stream, Store *store)
{
	int ret;

	if (!stream || !store) {
		return -1;
	}

	ret = PutHeader( stream, store);
	if (ret) {
		return ret;
	}

	ret = PutDrinks( stream, store);
	if (ret) {
		return ret;
	}

	ret = PutCures( stream, store);
	if (ret) {
		return ret;
	}

	ret = PutPurchasedCategories (stream, store);
	if (ret) {
		return ret;
	}

	ret = PutItems( stream, store);

	return ret;
}
