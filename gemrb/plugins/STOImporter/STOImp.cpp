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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/STOImporter/STOImp.cpp,v 1.11 2005/06/05 10:55:00 avenger_teambg Exp $
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
		GetItem(s->items[i]);
		if (s->items[i]->TriggerRef>0) { 
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
	str->ReadWord( &it->unknown );
	for (int i=0;i<3;i++) {
		str->ReadWord( it->Usages+i );
	}
	str->ReadDword( &it->Flags );
	str->ReadDword( &it->AmountInStock );
	str->ReadDword( &it->InfiniteSupply );
	switch (version) {
		case 11: //pst
			str->ReadDword( &it->TriggerRef );
			str->Read( it->unknown2, 56 );
			break;
		case 0: //gemrb version stores trigger ref in infinitesupply
			if (it->InfiniteSupply != (ieDword) -1) {
				it->TriggerRef = it->InfiniteSupply;
				//don't care about unknown2 in gemrb
				break;
			//falling through
			}
		default:
			it->TriggerRef = 0;
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

	memcpy( Signature, "STORV0.0", 8);
	Signature[5]='0'+version/10;
	Signature[7]='0'+version%10;
	stream->Write( Signature, 8);
	stream->WriteDword( &s->Type);
	stream->WriteDword( &s->StoreName);
	stream->WriteDword( &s->Flags);
	stream->WriteDword( &s->SellMarkup);
	stream->WriteDword( &s->BuyMarkup);
	stream->WriteDword( &s->DepreciationRate);
	stream->WriteWord( &s->StealFailureChance);

	ieWord tmp;
	switch (s->version) {
	case 10: case 0: // bg2, gemrb
		tmp = s->Capacity;
		break;
	default:
		tmp = 0;
		break;
	}
	stream->WriteWord( &tmp);

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
	return 0;
}

int STOImp::PutItem(DataStream *stream, STOItem *it)
{
	stream->WriteResRef( it->ItemResRef);
	stream->WriteWord( &it->unknown);
	for (unsigned int i=0;i<3;i++) {
	        str->WriteWord( it->Usages+i );
	}
	str->WriteDword( &it->Flags );
	str->WriteDword( &it->AmountInStock );
	switch (version) {
	case 0: //gemrb
		if (it->InfiniteSupply) {
			ieDword tmp = (ieDword) -1;
			str->WriteDword( &tmp );
		} else {
			str->WriteDword( &it->TriggerRef );
		}
		break;
	case 10: case 90: //bg, iwd
		str->WriteDword( &it->InfiniteSupply);
		break;
	case 11: //pst
		str->WriteDword( &it->InfiniteSupply);
		str->WriteDword( &it->TriggerRef);
		str->Write( it->unknown2, 56);
		break;
	}

	return 0;
}

int STOImp::PutCure(DataStream *stream, STOCure *c)
{
	stream->WriteResRef( c->CureResRef);
	stream->WriteDword( &c->Price);

	return 0;
}

int STOImp::PutDrink(DataStream *stream, STODrink *d)
{
	stream->WriteResRef( d->RumourResRef); //?
	stream->WriteDword( &d->DrinkName);
	stream->WriteDword( &d->Price);
	stream->WriteDword( &d->Strength);

	return 0;
}

//saves the store into a datastream, be it memory or file
int STOImp::PutStore(DataStream *stream, Store *store)
{
	unsigned int i;
	int ret;

	if (!stream || !store) {
		return -1;
	}

	ret = PutHeader (stream, store);
	if (ret) {
		return ret;
	}

	for (i=0;i<store->DrinksCount;i++) {
		ret = PutDrink( stream, store->drinks+i);
		if (ret) {
			return ret;
		}
	}

	for (i=0;i<store->CuresCount;i++) {
		ret = PutCure( stream, store->cures+i);
		if (ret) {
			return ret;
		}
	}

	ret = PutPurchasedCategories (stream, store);

	for (i=0;i<store->ItemsCount;i++) {
		ret = PutItem( stream, store->items[i]);
		if (ret) {
			return ret;
		}
	}

	return ret;
}
