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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Store.h,v 1.9 2005/03/05 18:15:52 avenger_teambg Exp $
 *
 */

#ifndef STORE_H
#define STORE_H

#include <vector>
#include "../../includes/ie_types.h"

#include "AnimationMgr.h"


#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef enum StoreType { STT_STORE=0, STT_TAVERN=1, STT_INN=2, STT_TEMPLE=3,
STT_BG2CONT=4, STT_IWD2CONT=5 } StoreType;

typedef enum StoreActionType { STA_BUYSELL=0, STA_IDENTIFY=1, STA_STEAL=2,
STA_CURE=3, STA_DONATE=4, STA_DRINK=5, STA_ROOMRENT=6, STA_OPTIONAL=0x80} StoreActionType;

#define IE_STORE_BUY     1
#define IE_STORE_SELL    2
#define IE_STORE_ID      4
#define IE_STORE_STEAL   8
#define IE_STORE_DONATE  16     //gemrb extension
#define IE_STORE_CURE    32
#define IE_STORE_DRINK   64
#define IE_STORE_RENT    128    //gemrb extension
#define IE_STORE_QUALITY 0x600  //2 bits
#define IE_STORE_FENCE   0x2000 //

typedef struct STOItem {
	ieResRef ItemResRef;
	ieWord unknown;
	ieWord Usage1;
	ieWord Usage2;
	ieWord Usage3;
	ieDword Flags;
	// 2 cached values from associated item. LEAVE IT SIGNED!
	int Weight;
	int StackAmount;
	ieDword AmountInStock;
	ieDword InfiniteSupply;
	// V1.1
	char unknown2[56];
} STOItem;

typedef struct STODrink {
	ieResRef RumourResRef;
	ieStrRef DrinkName;
	ieDword Price;
	ieDword Strength;
} STODrink;

typedef struct STOCure {
	ieResRef CureResRef;
	ieDword Price;
} STOCure;


class GEM_EXPORT Store {
public:
	Store();
	~Store();

	std::vector< STOItem*> items;
	std::vector< STODrink*> drinks;
	std::vector< STOCure*> cures;

	ieDword* purchased_categories;

	ieResRef Name;
	ieDword Type;
	ieStrRef StoreName;
	ieDword Flags;
	ieDword SellMarkup;
	ieDword BuyMarkup;
	ieDword DepreciationRate;
	ieWord StealFailureChance;
	ieWord Capacity;
	char unknown[8];
	ieDword PurchasedCategoriesOffset;
	ieDword PurchasedCategoriesCount;
	ieDword ItemsOffset;
	ieDword ItemsCount;
	ieDword Lore;
	ieDword IDPrice;
	ieResRef RumoursTavern;
	ieDword DrinksOffset;
	ieDword DrinksCount;
	ieResRef RumoursTemple;
	ieDword AvailableRooms;
	ieDword RoomPrices[4];
	ieDword CuresOffset;
	ieDword CuresCount;
	char unknown2[36];

	// IWD2 only
	char unknown3[80];

public: //queries
	bool AcceptableItemType(ieDword type, ieDword invflags) const;
	STOCure *GetCure(int idx) const;
	STODrink *GetDrink(int idx) const;
	STOItem *GetItem(int idx) const;
};

#endif  // ! STORE_H
